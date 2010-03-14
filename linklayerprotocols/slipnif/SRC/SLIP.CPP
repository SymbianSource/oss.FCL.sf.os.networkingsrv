// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
//

/**
 @file SLIP.CPP
*/

#include <nifmbuf.h>
#include <in_iface.h>
#include <cdblen.h>
#include <cdbcols.h>
#include "SLIP.H"
#include "SLIP_VER.H"

const TUint KSlashChar='\\'; //< back slash character

//
// SLIP ESock bits
//

CSlip::CSlip(CNifIfFactory& aFactory)
	: CNifIfLink(aFactory)
/**
Constructor
*/
	{
	__DECLARE_NAME(_S("CSlip"));
	}

CSlip::~CSlip()
/**
Destructor
*/
	{
	}

void CSlip::Info(TNifIfInfo& aInfo) const
	{

	FillInInfo(aInfo);
	}

void CSlip::FillInInfo(TNifIfInfo& aInfo)
	{
	aInfo.iVersion = TVersion(KSlipMajorVersionNumber, KSlipMinorVersionNumber, KSlipBuildVersionNumber);
	aInfo.iFlags = KNifIfIsBase | KNifIfUsesNotify | KNifIfIsLink | KNifIfCreatedByFactory;
   _LIT(KSlip, "slip");
	aInfo.iName = KSlip;
	aInfo.iProtocolSupported=KProtocolInetIp;
	}

void CSlip::BindL(TAny* aId)
	{
	if (iIpProtocol)
		User::Leave(KErrAlreadyExists);
	iIpProtocol = (CProtocolBase*)aId;
	}

TInt CSlip::Send(RMBufChain& aPacket, TAny*)
	{
	iSendPktQ.Append(aPacket);
	iSendMBufs += aPacket.NumBufs()-1;
	if (!(iFlags & KSlipSendInUse))
		DoSend();

	return (iFlags & KSlipSendInUse) ? 0 : 1;
	}

TInt CSlip::State()
	{

    return (iFlags & KSlipIsUp) ? ((iFlags & KSlipSendBusy) ? EIfBusy : EIfUp) : EIfDown;
	}

TInt CSlip::Control(TUint aLevel, TUint aName, TDes8& aOption, TAny* /*aSource*/)
	{
	if (aLevel!=KSOLInterface)
		return KErrNotSupported;

	switch (aName)
		{
	case KSoIfInfo:
		{
		TSoIfInfo& opt = *(TSoIfInfo*)aOption.Ptr();
		TNifIfInfo info;
		FillInInfo(info);

		opt.iName = info.iName;
		TPortName port;

		TBuf<KCommsDbSvrMaxColumnNameLength> columnName=TPtrC(MODEM);
		columnName.Append(TChar(KSlashChar));
		columnName.Append(TPtrC(MODEM_CSY_NAME));		
		iNotify->ReadDes(columnName, port);
		port.LowerCase();
		_LIT(portNameFormat,"::%S");
		opt.iName.AppendFormat(portNameFormat, &port);

		opt.iFeatures = KIfIsPointToPoint | KIfIsDialup;
		opt.iMtu = KSlipDefaultMtu;
		opt.iSpeedMetric = (iFlags & KSlipIsUp) ? SpeedMetric() : 0;
		return KErrNone;
		}

	case KSoIfHardwareAddr:
		return KErrNotSupported;

	case KSoIfConfig:
		return KErrNotSupported;
		}
		
	return KErrNotSupported;
	}

//
// SLIP NifMan bits
//

CNifIfBase* CSlip::GetBinderL(const TDesC& aName)
/**
Interface Manager call to create a new Binder protocol
which SLIP ignores

@param aName new binder protocol
*/
	{
	
   _LIT(KDescIp, "ip");
   _LIT(KDescIcmp, "icmp");
	if(aName.CompareF(KDescIp) && aName.CompareF(KDescIcmp))
		User::Leave(KErrNotSupported);

	return 0;
	}

TInt CSlip::Start()
/**
Interface Manager call to start and claim the io port
*/
	{
	TInt res=PacketModeOn();
	if(res!=KErrNone)
		return res;
	iFlags |= KSlipIsUp;
	iRecvBuf.SetMax();
	CommRead(iRecvBuf);
	iIpProtocol->StartSending((CProtocolBase*)this);
	iNotify->LinkLayerUp();
	iNotify->IfProgress(EIfProgressLinkUp, KErrNone);
	return KErrNone;
	}

void CSlip::Stop(TInt aReason, MNifIfNotify::TAction aAction)
/**
Interface Manager call to stop and release the io port
*/
	{
	iFlags &= ~KSlipIsUp;
	PacketModeOff();
	iSendPktQ.Free();
	iSendPkt.Free();
	iSendMBuf->Free();
	iSendMBuf = NULL;
	iRecvQ.Free();
	iRecvMBuf->Free();
	iRecvMBuf = NULL;
	iRecvLen = 0;
	iFlags &= KSlipRecvEscPend;
	iNotify->LinkLayerDown(aReason, aAction);
	iNotify->IfProgress(EIfProgressLinkDown, aReason);
	}

//
// SLIP Internal
//

void CSlip::DoSend()
	{
	TBool flowon = EFalse;
	
	TUint8* bptr = (TUint8*)iSendBuf.Ptr();
	TUint8* bend = (bptr+iSendBuf.MaxLength())-1; // -1 allows no check in switch below

	TUint8* mptr = NULL;	// CSW: set to NULL for ASSERT below
	TUint8* mend = NULL;	// CSW: set to NULL for ASSERT below

	if (iSendMBuf!=NULL)
		{
		mptr = iSendMPtr;
		mend = iSendMBuf->EndPtr();
		}
	
	while (bptr<bend)
		{
		if (iSendMBuf==NULL)
			{
			if (iSendMBuf = iSendPkt.Remove(), iSendMBuf==NULL)
				{
				// End of frame
				*bptr++ = KSlipEndCh;
				RMBufPacket pkt;				
				if (!iSendPktQ.Remove(pkt))
					goto SendIt;
				pkt.Unpack();
				pkt.FreeInfo();
				iSendPkt.Assign(pkt);
				iSendMBuf = iSendPkt.Remove();
				}
			else
				{
				if ((iFlags & KSlipSendBusy) && iSendMBufs<=iSendLoWat)
					{
					// Deferred Flow Enable
					flowon = ETrue;
					iFlags &= ~KSlipSendBusy;
					}

				mptr = iSendMBuf->Ptr();
				mend = iSendMBuf->EndPtr();
				if (mptr==mend)
					{
					iSendMBuf->Free();
					iSendMBuf = NULL;
					continue; // with next MBuf
					}
				}
			}
		__ASSERT_DEBUG( mptr, SlipPanic( PanicSlipAssert1 ) );
		switch (*mptr)
			{
		case KSlipEndCh:
			*bptr++ = KSlipEscCh;
			*bptr++ = KSlipEscEndCh;
			break;
		case KSlipEscCh:
			*bptr++ = KSlipEscCh;
			*bptr++ = KSlipEscEscCh;
			break;
		default:
			*bptr++ = *mptr;
			break;
			}

		if (++mptr==mend)
			{
			iSendMBuf->Free();
			iSendMBuf = NULL;
			}
		}

SendIt:
		iSendMPtr = mptr;
		TInt n = bend-bptr;
		if (n>0)
			{
			iSendBuf.SetLength(bend-bptr);
			CommWrite(iSendBuf);
			iFlags |= KSlipSendInUse;
			}

		if (flowon)
			iIpProtocol->StartSending((CProtocolBase*)this);
	}

void CSlip::CommWriteComplete(TInt aStatus)
	{
	iFlags &= ~KSlipSendInUse;
	switch (aStatus)
		{
	case KErrCommsLineFail:
		LinkDown(aStatus);
		break;
	default:
		if (iFlags & KSlipIsUp)
			DoSend();
		}
	}

void CSlip::CommReadComplete(TInt aStatus)
	{
	switch (aStatus)
		{
	case KErrCommsFrame:
	case KErrCommsOverrun:
	case KErrCommsParity:
		iRecvQ.Free();
		iRecvLen = 0;
		iRecvMBuf->Free();
		iRecvMBuf = NULL;
		iFlags &= KSlipRecvEscPend;
		break;
	case KErrNone:
		DoRecv();
		break;
	case KErrCommsLineFail:
	default:
		LinkDown(aStatus);
		break;
		}
	if (iFlags & KSlipIsUp)
		{
		iRecvBuf.SetMax();
		CommRead(iRecvBuf);
		}
	}


void CSlip::DoRecv()
	{
	TUint8* bptr = (TUint8*)iRecvBuf.Ptr();
	TUint8* bend = bptr+iRecvBuf.Length();
	if (bend==bptr)
		return;

	TUint8* mptr = NULL;	// CSW: set to NULL for ASSERT below
	TUint8* mend = NULL;	// CSW: set to NULL for ASSERT below
	TUint8 c;
	
	TInt ret;
	
	if (iRecvMBuf!=NULL)
		{
		mptr = iRecvMBuf->Ptr();
		mend = iRecvMBuf->EndPtr();
		}

	while (bptr<bend)
		{
		if (!(iFlags & KSlipRecvEscPend))
			c = *bptr++;
		else
			{
			c = KSlipEscCh;
			iFlags &= ~KSlipRecvEscPend;
			}

		switch (c)
			{
		case KSlipEndCh:
			// State is END_RECVD
			if (iRecvLen>0)
				{
				__ASSERT_DEBUG( mptr, SlipPanic( PanicSlipAssert3 ) );
				__ASSERT_DEBUG( mend, SlipPanic( PanicSlipAssert4 ) );
				iRecvMBuf->SetLength(mend-mptr);
				RMBufPacket pkt;
				TRAPD(mem, pkt.CreateL(iRecvQ, iRecvLen));
				if(mem!=KErrNone)
					{
					//We are out of memory
					return;
					}
				pkt.Pack();
				iIpProtocol->Process(pkt, (CProtocolBase*)this);
				iRecvMBuf = NULL; // Force a new allocation
				iRecvLen = 0;
				}
			break;			
		case KSlipEscCh:
			// State is ESC_RECVD
			if (bptr==bend)
				{
				iFlags |= KSlipRecvEscPend;
				__ASSERT_DEBUG( mptr, SlipPanic( PanicSlipAssert5 ) );
				iRecvMPtr = mptr;
				return;
				}
			c = *bptr++;
			if (c==KSlipEscEndCh)
				c = KSlipEndCh;
			else if (c==KSlipEscEscCh)
				c = KSlipEscCh;
			// Fall through...
		default:
			// Allocation defered to here to ensure that
			// no data is lost due to pre-allocs failing.
			if (iRecvMBuf==NULL)
				{
				TRAP(ret, iRecvMBuf = RMBuf::AllocL());
				if (ret!=KErrNone)
					{
					// Junk the packet so far
					iRecvQ.Free();
					iRecvLen = 0;
					iFlags &= ~KSlipRecvEscPend;
					return;
					}
				else
					{
					mptr = iRecvMBuf->Ptr();
					mend = iRecvMBuf->EndPtr();
					}
				}
			__ASSERT_DEBUG( mptr, SlipPanic( PanicSlipAssert6 ) );
			__ASSERT_DEBUG( mend, SlipPanic( PanicSlipAssert7 ) );
			*mptr++ = c;
			iRecvLen++;
			if (mptr==mend)
				{
				iRecvQ.Append(iRecvMBuf);
				iRecvMBuf = NULL; // Force a new allocation
				}
			break;
			}
		}
		iFlags &= KSlipRecvEscPend;
		__ASSERT_DEBUG( mptr, SlipPanic( PanicSlipAssert8 ) );
		iRecvMPtr = mptr;
	}


void CSlip::LinkDown(TInt aReason)
	{

	if(aReason!=KErrCommsLineFail)
		{
		iNotify->LinkLayerDown(aReason, MNifIfNotify::EDisconnect);
		iNotify->IfProgress(EIfProgressLinkDown, aReason);
		}
	else
		iNotify->LinkLayerDown(aReason, MNifIfNotify::EReconnect);
	}

TInt CSlip::SpeedMetric()
	{
	switch (iOrigConfig().iRate)
		{
	case EBps50:
		return 50;
	case EBps75:
		return 75;
	case EBps110:
		return 110;
	case EBps134:
		return 134;
	case EBps150:
		return 150;
	case EBps300:
		return 300;
	case EBps600:
		return 600;
	case EBps1200:
		return 1200;
	case EBps1800:
		return 1800;
	case EBps2000:
		return 2000;
	case EBps2400:
		return 2400;
	case EBps3600:
		return 3600;
	case EBps4800:
		return 4800;
	case EBps7200:
		return 7200;
	case EBps9600:
		return 9600;
	case EBps19200:
		return 19200;
	case EBps38400:
		return 38400;
	case EBps57600:
		return 57600;
	case EBps115200:
		return 115200;
//	case EBps230400:
//		return 230400;
//	case EBps460800:
//		return 230400;
//	case EBps921600:
//		return 921600;
	case EBpsSpecial:
		return iOrigConfig().iSpecialRate;
	default:
		;
		}
	return 0;
	}

TInt CSlip::PacketModeOn()
/**
Configure the comm port for our needs
*/
	{
	TInt err;
	if (iFlags & KSlipCommConfigOk)
		return KErrNone;	

	TName port;
	TBuf<KCommsDbSvrMaxColumnNameLength> columnName=TPtrC(MODEM);
	columnName.Append(TChar(KSlashChar));
	columnName.Append(TPtrC(MODEM_PORT_NAME));		
	iNotify->ReadDes(columnName, port);

	if (err = CommOpen(port, ECommShared), err!=KErrNone)
		return err;
	
	iCommPort.Config(iOrigConfig);
	TCommConfig cbuf = iOrigConfig;
	TCommConfigV01 &cfg=cbuf();
	cfg.iTerminatorCount = 1;
	cfg.iTerminator[0] = KSlipEndCh;
	cfg.iHandshake |= (0
		//	| KConfigObeyCTS
		//	| KConfigFailCTS
			| KConfigObeyDSR
			| KConfigFailDSR
			| KConfigObeyDCD
			| KConfigFailDCD
		//	| KConfigFreeRTS
		//	| KConfigFreeDTR
			);
	if (err = iCommPort.SetConfig(cbuf), err!=KErrNone)
		{
		CommClose();
		return err;
		}

	iFlags |= KSlipCommConfigOk;
	return KErrNone;
	}

TInt CSlip::PacketModeOff()
	{
	TInt err = KErrNone;
	
	if (iFlags & KSlipCommConfigOk)
		{
		CommCancel();
		iFlags &= ~KSlipCommConfigOk;
		err = iCommPort.SetConfig(iOrigConfig);
		CommClose();
		}
	return err;
	}

TInt CSlip::Notification(TAgentToNifEventType /*aEvent*/, void * /*aInfo*/)
	{
	// no timers in SLIP, so nothing to do!
	return KErrNone;
	}

void CSlip::Restart(CNifIfBase*)
/**
SLIP doesn't support different network-layer binders, so we can't restart one!
*/
	{}
