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
// dummy NCPIP.CPP
// 
//

#include <e32uid.h>
#include <cdblen.h>
#include <networking/vj.h>
#include "NCPIP.H"
#include "PPPLOG.H"
#include "PPP_VER.H"
#include <networking/pppprog.h>

//#define IPADDR(a,b,c,d) (TUint32)(((a)<<24)|((b)<<16)|((c)<<8)|(d))
//#define _LOC_ADDR IPADDR(10,0,0,1)
//#define _REM_ADDR IPADDR(10,0,0,2)

#if defined(__VC32__) && (_MSC_VER < 1300)
 #define PMF(x) x
#else
 #define PMF(x) &x
#endif

_LIT(KVjComp, "vjcomp");

//
// PPP for ESock
//

// warning C4355: 'this' : used in base member initializer list
#pragma warning (disable:4355)

CPppNcpIp::CPppNcpIp(CPppLcp* aLcp)
	:   CNifIfBase(),
        MPppFsm(aLcp, EPppPhaseNetwork, KPppIdIpcp),
		iIpRecvr(this, PMF(CPppNcpIp::RecvIp), PMF(CPppNcpIp::SendFlowOn), aLcp, EPppPhaseNetwork, 
			KPppIdIp, PMF(CPppNcpIp::IpFrameError), PMF(CPppNcpIp::IpKillProtocol)),
		iVjCompTcpRecvr(this, PMF(CPppNcpIp::RecvVjCompTcp), PMF(CPppNcpIp::SendFlowOn), aLcp, EPppPhaseNetwork, 
			KPppIdVjCompTcp, PMF(CPppNcpIp::VjCompTcpFrameError), PMF(CPppNcpIp::VjCompTcpKillProtocol)),
		iVjUncompTcpRecvr(this, PMF(CPppNcpIp::RecvVjUncompTcp), PMF(CPppNcpIp::SendFlowOn), aLcp, EPppPhaseNetwork, 
			KPppIdVjUncompTcp, PMF(CPppNcpIp::VjUncompTcpFrameError), PMF(CPppNcpIp::VjUncompTcpKillProtocol))
	{
	__DECLARE_FSM_NAME(_S("IPCP"));
	}
#pragma warning (default:4355)

CPppNcpIp::~CPppNcpIp()
	{
	CObjectCon* con;
	TInt		i;

	/*Deregister();
	iIpRecvr.Deregister();
	iVjCompTcpRecvr.Deregister();
	iVjUncompTcpRecvr.Deregister();
	delete iSendCallBack;*/

	if (iVJCompressor)
		{
		iVJCompressor->Close();
		iVJCompressor = NULL;
		}

	if (iVJDecompressor)
		{
		iVJDecompressor->Close();
		iVJDecompressor = NULL;
		}

	//
	// Delete all the Containers
	//
	if(iVJCompressorCon)
		{
		con = iVJCompressorCon;

		for(i=0 ; i<con->Count() ; ++i)
			CNifFactory::ControlledDelete((*con)[i]);
			iVJCompressorCon = NULL;
		}


	if(iVJDeCompressorCon)
		{
		con = iVJDeCompressorCon;
		for(i=0 ; i<con->Count() ; ++i)
			CNifFactory::ControlledDelete((*con)[i]);
		iVJDeCompressorCon = NULL;
		}

	/*iSendQ.Free();*/
	}


void CPppNcpIp::ConstructL()
//
// Construct the Link Protocol Object
//
	{
	/*Register();
	iIpRecvr.Register();
	iVjCompTcpRecvr.Register();
	iVjUncompTcpRecvr.Register();
	
	TCallBack scb(SendCallBack, this);	
	iSendCallBack = new(ELeave) CAsyncCallBack(scb, KIpcpSendPriority);*/
	

	//iVJCompressorCon = (iPppLcp->ContainerForDlls())->CreateL();
	//iVJDeCompressorCon = (iPppLcp->ContainerForDlls())->CreateL();

    iVJCompressorCon = CObjectCon::NewL();
    iVJDeCompressorCon = CObjectCon::NewL();
    
	/*FsmConstructL();
	FsmOpen();*/

	// Create a unique interface name	
	/*TBuf<KCommsDbSvrMaxColumnNameLength> columnName=TPtrC(KCDTypeNameModemBearer);
	columnName.Append(TChar(KSlashChar));
	columnName.Append(TPtrC(KCDTypeNamePortName));
	TBuf<KCommsDbSvrMaxColumnNameLength> port;
	TInt ret = iNotify->ReadDes(columnName, port);
	if (ret==KErrNone)
		{
		port.LowerCase();
		iIfName.Format(_L("ipcp::%S"), &port);
		}
	else
		iIfName.Format(_L("ipcp[0x%08x]"), this);*/
	}

void CPppNcpIp::BindL(TAny* /*aId*/)
//
//
//
	{
	/*if(iNetwork)
		User::Leave(KErrInUse);
	iNetwork = (CProtocolBase*) aId;*/
	}

void CPppNcpIp::Info(TNifIfInfo& /*aInfo*/) const
//
//
//
	{
	
	/*FillInInfo(aInfo);
	aInfo.iName.Copy(iIfName);*/
	}

void CPppNcpIp::FillInInfo(TNifIfInfo& /*aInfo*/)
//
//
//
	{

	/*aInfo.iVersion = TVersion(KPPPMajorVersionNumber, KPPPMinorVersionNumber, KPPPBuildVersionNumber);
	aInfo.iFlags = KNifIfIsBase | KNifIfUsesNotify | KNifIfCreatedByLink;
   _LIT(KIpcp, "ipcp");
	aInfo.iName = KIpcp;
	aInfo.iProtocolSupported=KProtocolInetIp;*/
	}

TInt CPppNcpIp::State()
//
//
//
	{

	/*if (FsmIsThisLayerOpen())
        return (iUpperFlowOn) ? EIfUp : EIfBusy;
	else
		return EIfDown;*/
		return 0;
	}

TInt CPppNcpIp::Control(TUint /*aLevel*/,TUint /*aName*/,TDes8& /*aOption*/, TAny*)
//
//
//
	{
	/*if (aLevel==KSOLInterface)
		{
		switch (aName)
			{
		case KSoIfInfo:
			{
			TSoIfInfo& opt = *(TSoIfInfo*)aOption.Ptr();

			opt.iName.Copy(iIfName);
			opt.iFeatures = KPppIpIfaceFeatures | KIfIsDialup;

			TInt rxsz, txsz;
			if (FsmIsThisLayerOpen())
				{
				iPppLcp->PppLink()->GetSendRecvSize(rxsz, txsz);
				opt.iMtu = txsz==0 ? KPppDefaultFrameSize : txsz;
				opt.iSpeedMetric = iPppLcp->PppLink()->SpeedMetric();
				}
			else
				{
				opt.iMtu = KPppDefaultFrameSize;
				opt.iSpeedMetric = 0;
				}
			iPppLcp->SetMaxTransferSize(opt.iMtu);

			return KErrNone;
			}

		case KSoIfHardwareAddr:
			return KErrNotSupported;

		case KSoIfConfig:
			{
			TSoInetIfConfig& opt = *(TSoInetIfConfig*)aOption.Ptr();
			if (opt.iFamily!=KAfInet)
				return KErrNotSupported;
			TInetAddr::Cast(opt.iConfig.iAddress).SetAddress(iLocalAddr);
			TInetAddr::Cast(opt.iConfig.iNetMask).SetAddress(iNetworkMask);
			TInetAddr::Cast(opt.iConfig.iBrdAddr).SetAddress(iRemoteAddr);

			// Nokia suggested fix to prevent a default gateway of 0.0.0.0
			// being set if PPP peer won't give us an IP address for some
			// reaspon. See defect report TEE-5DWD4F (in V4, Hurricane) and
			// subsequent discussion for details...
//			TInetAddr::Cast(opt.iConfig.iDefGate).SetAddress(iRemoteAddr);

#ifdef _DEBUG
			// Force peer IP address to 0.0.0.0 to allow this fix to be
			// tested with any phone, operator, or connection mechanism.
			//
			// Shouldn't do any harm, and it'll only be present in debug
			// builds, but nevertheless I reccomend this be commented out
			// when not actually needed!
			//
			// JMG 15-01-03
		
//			iRemoteAddr = 0 ;

#endif
			TInetAddr::Cast(opt.iConfig.iDefGate).SetAddress(iRemoteAddr ? iRemoteAddr : iLocalAddr);

			TInetAddr::Cast(opt.iConfig.iNameSer1).SetAddress(iPrimaryDns);
			TInetAddr::Cast(opt.iConfig.iNameSer2).SetAddress(iSecondaryDns);
#ifdef _DEBUG
			TUint ipAddressByte0 = iRemoteAddr & 0xFF;
			TUint ipAddressByte1 = (iRemoteAddr >> 8) & 0xFF;
			TUint ipAddressByte2 = (iRemoteAddr >> 16) & 0xFF;
			TUint ipAddressByte3 = (iRemoteAddr >> 24) & 0xFF;
			LOG(iPppLcp->iLogger->Printf(_L("IPCP Remote Address = %03u.%03u.%03u.%03u\r\n"),ipAddressByte3, ipAddressByte2,ipAddressByte1,ipAddressByte0);)
#endif
			return KErrNone;
			}

		case KSoIfCompareAddr:
			if(((TInetAddr&)aOption).Address()!=iLocalAddr)
				return KErrBadName;
			return KErrNone;


		case KSoIfGetConnectionInfo:
			TSoIfConnectionInfo& opt = *(TSoIfConnectionInfo*)aOption.Ptr();
			TInt err = KErrNone;
			TBuf<2*KCommsDbSvrMaxColumnNameLength+1> fieldName;

			fieldName.Copy(TPtrC(KCDTypeNameIAP));
			fieldName.Append(KSlashChar);
			fieldName.Append(TPtrC(KCDTypeNameRecordName));
			if ((err = iNotify->ReadInt(fieldName, opt.iIAPId)) != KErrNone)
				return err;

			fieldName.Copy(TPtrC(KCDTypeNameIAP));
			fieldName.Append(KSlashChar);
			fieldName.Append(TPtrC(KCDTypeNameIAPNetwork));
			if ((err = iNotify->ReadInt(fieldName, opt.iNetworkId)) != KErrNone)
				return err;

			return KErrNone;
			}
		}
	return KErrNotSupported;*/
	return KErrNone;
	}

void CPppNcpIp::SendFlowOn()
	{
	/*LOG( iPppLcp->iLogger->Printf(_L("IPCP Flow On iUpperFlowOn=%d\r\n"), iUpperFlowOn); )	
	iLowerFlowOn = ETrue;
	if (!iSendQ.IsEmpty())
		iSendCallBack->CallBack();
	if (iSendQ.IsEmpty() && iNetwork)
		{
		iUpperFlowOn = ETrue;
		LOG( iPppLcp->iLogger->Printf(_L("StartSending to IP from SendFlowOn()\r\n"));)
		iNetwork->StartSending((CProtocolBase*)this);
		}*/
	}

TInt CPppNcpIp::SendCallBack(TAny* aCProtocol)
	{
	((CPppNcpIp*)aCProtocol)->DoSend();
	return 0;
	}

void CPppNcpIp::DoSend()
	{
	/*if (FsmIsThisLayerOpen())
		{
		RMBufPacket pkt;
		while (iSendQ.Remove(pkt))
			{
			RMBufPktInfo*info = pkt.Unpack();
			TPppAddr addr;
			TUint Protocol;
			addr = info->iDstAddr;
			Protocol = addr.GetProtocol();
			pkt.Pack();
			if (iPppLcp->PppLink()->Send(pkt, Protocol) <= 0)
				{
				LOG( iPppLcp->iLogger->Printf(_L("IPCP Flow Off\r\n")); )	
				iLowerFlowOn = EFalse;
				break;
				}
			}
		if (iLowerFlowOn&&!iUpperFlowOn)
			{
			iUpperFlowOn = ETrue;
			LOG( iPppLcp->iLogger->Printf(_L("StartSending to IP from DoSend()\r\n")); )
			iNetwork->StartSending((CProtocolBase*)this);
			}
		}*/
	}


//
// NCP Upcalls from FSM
//

TInt CPppNcpIp::FsmLayerStarted()
//
// Open the layer below
//
	{
	/*return iPppLcp->PppOpen();*/
	return KErrNone;
	}

void CPppNcpIp::FsmLayerFinished(TInt /*aReason*/)
//
// Close the layer below
//
	{
	/*iPppLcp->PppClose(aReason);
	iNotify->IfProgress(EPppProgressLinkDown, KErrNone);*/
	}	

void CPppNcpIp::FsmLayerUp()
//
// Signal up event to next layer above
//
	{
	/*SendFlowOn();
	iNotify->LinkLayerUp();
    if(!iPppLcp->QueryMIP())
        {
        // Only send the LinkUp progress if we're not
        // doing Mobile IP.  If we are doing MIP then
        // the LinkUp progress should be generated in
        // the MIP plugin module after it has finished
        // its own negotiation
        iNotify->IfProgress(EPppProgressLinkUp, KErrNone);
        }*/
	}

void CPppNcpIp::FsmLayerDown(TInt /*aReason*/)
//
// Signal down event to next layer above
//
	{

	/*if(aReason==KErrNone)
		return;
    
	if (aReason==KErrCommsLineFail && FsmIsThisLayerOpen())
		{
		iNotify->LinkLayerDown(aReason, MNifIfNotify::EReconnect);
		}
	else
		{
		iNotify->LinkLayerDown(aReason, MNifIfNotify::EDisconnect);
		}
	iNotify->IfProgress(EPppProgressLinkDown, aReason);*/
	}


TInt CPppNcpIp::PresetAddr(TUint32&/* aAddr*/, const TDesC& /*aVarName*/)
/**
 * Preset IP adress in aAddr with value from CommDB. The field name
 * is given in aVarName. Examples are "IpAddr" and "IpNameServer1"
 *
 * @param aAddr    IP address to be set
 * @param aVarName name of CommDB field
 * @return KErrNone if success, global error code otherwise
 */
	{
	/*if (aAddr!=0)
		return KErrNone;
	
	TBuf<KCommsDbSvrMaxFieldLength> name;
	TInetAddr addr;

	(void)iNotify->ReadDes(aVarName, name); // ignore return value

	const TInt ret = addr.Input(name);
	if (ret==KErrNone)
		aAddr = addr.Address();

	return ret;*/
	return KErrNone;
	}

	
void CPppNcpIp::FsmFillinConfigRequestL(RPppOptionList& /*aRequestList*/)
//
// Fill in Config Request to be sent
//
	{
	/*TBool RequestDynamicIPAddress;

	// See whether we are configured to request a dynamic IP address. If so, make
	// sure we get one by setting iLocalAddr to 0.0.0.0
	if (iNotify->ReadBool(TPtrC(KCDTypeNameIpAddrFromServer), RequestDynamicIPAddress) != KErrNone)
		{
		// Default behaviour
		RequestDynamicIPAddress = ETrue;
		}
	if(!RequestDynamicIPAddress)
		{
		LOG(iPppLcp->iLogger->Printf(_L("Requesting static IP address\n"));)
		PresetAddr(iLocalAddr, TPtrC(KCDTypeNameIpAddr));
		}
	else
		{
		LOG(iPppLcp->iLogger->Printf(_L("Requesting dynamic IP address\n"));)
		iLocalAddr = 0;
		}

	TBool RequestDynamicDNSAddress;
	// See whether we are configured to request a dynamic DNS address. If so, make
	// sure we get one by setting iPrimaryDns and iSecondaryDns to 0.0.0.0
	if (iNotify->ReadBool(TPtrC(KCDTypeNameIpDNSAddrFromServer), RequestDynamicDNSAddress) != KErrNone)
		{
		// Default behaviour
		RequestDynamicDNSAddress = ETrue;
		}

	if(!RequestDynamicDNSAddress)
		{	// Static addresses
		LOG(iPppLcp->iLogger->Printf(_L("Requesting static DNS address\n"));)
		PresetAddr(iPrimaryDns, TPtrC(KCDTypeNameIpNameServer1));
		PresetAddr(iSecondaryDns, TPtrC(KCDTypeNameIpNameServer2));
		}
	else
		{	// set to 0 for dynamic address request
		LOG(iPppLcp->iLogger->Printf(_L("Requesting dynamic DNS address\n"));)
		iPrimaryDns = 0;
		iSecondaryDns = 0;
		}
	iPrimaryNbns = IPADDR(0,0,0,0);
	iSecondaryNbns = IPADDR(0,0,0,0);

	PresetAddr(iNetworkMask, TPtrC(KCDTIdIpNetMask));
	if(iNetworkMask)
		{
		TUint32 mask=~1u, bits=iNetworkMask;
		while((bits&1)==0)
			{
			bits>>=1;
			mask<<=1;
			}
		iNetworkMask = mask;
		}

    PresetAddr(iRemoteAddr, TPtrC(KCDTIdIpGateway)); // Field use differently in server mode

	if (iNotify->ReadBool(TPtrC(KCDTIdEnableIpHeaderComp), iVJCompressionOn) != KErrNone)
		{
		// Default behaviour
		iVJCompressionOn = FALSE;
		}
	else if (iVJCompressionOn)
		{
      _LIT(KVJCompDll, "vjcomp.dll");
		iVJCompressionOn = iPppLcp->DoesDllExist(KVJCompDll);
		}

   if (iVJCompressionOn)
      {
	   TUint32	CompressionOptions;
	
	   CompressionOptions = (KPppIdVjCompTcp << 16) | ((KMaxVjSlots-1) << 8) | KVjCompSlotId;

	   aRequestList.CreateAndAddL(KPppIpcpOptIpCompressionProtocol, CompressionOptions);
      }

   if(!iPppLcp->QueryMIP())
	{
	// Don't ask for IP address if we use CDMA MIP.
	aRequestList.CreateAndAddL(KPppIpcpOptIpAddress, iLocalAddr);
	}

	if (RequestDynamicDNSAddress)
		{
		aRequestList.CreateAndAddL(KPppIpcpOptPrimaryDnsAddress, iPrimaryDns);
	//	aRequestList.CreateAndAddL(KPppIpcpOptPrimaryNbnsAddress, iPrimaryNbns); 
		aRequestList.CreateAndAddL(KPppIpcpOptSecondaryDnsAddress, iSecondaryDns);
	//	aRequestList.CreateAndAddL(KPppIpcpOptSecondaryNbnsAddress, iSecondaryNbns);
		}
	// Nbns negotiation removed to speed things up*/
	//
	}

void CPppNcpIp::FsmCheckConfigRequest(RPppOptionList& /*aRequestList*/, RPppOptionList& /*aAckList*/, RPppOptionList& /*aNakList*/, RPppOptionList& /*aRejList*/)
//
// Check options in a recvd config request
//
	{
/*	RPppOption opt;
	while (aRequestList.Remove(opt))
		{
		switch (opt.OptType())
			{
		case KPppIpcpOptIpCompressionProtocol:
			
      if (iVJCompressionOn)
         {

			TUint	Length;
			TUint16	CompressionProtocol; 
			TUint8*	Options;

			// Remove Warning
			CompressionProtocol = 0;

			Length = opt.ValueLength();

			if (Length >= 2)
				CompressionProtocol = BigEndian::Get16(opt.ValuePtr());

			if (CompressionProtocol == KPppIdVjCompTcp)
			   {
				//
				//	VJ Compression
				//
				Options = opt.ValuePtr();
				Options += 2;
			 
				
				if (*Options <= KMaxVjSlots)
					{
					iMaxVJSlots = *Options++;
					iCompressConnId = *Options++;
					//
					//	If these values are OK then ACK
					//
					aAckList.Append(opt);
					}
				else
					{
					aNakList.Append(opt);
					}
			   }
			else
			   {
				aRejList.Append(opt);
		   	}
         }

			aRejList.Append(opt);
			break;
		case KPppIpcpOptIpAddress:
			{
			TUint32 addr = BigEndian::Get32(opt.ValuePtr());
			if (addr==0)
				{
				if (iPppLcp->PppLinkMode()==EPppLinkIsServer)
					{
					BigEndian::Put32(opt.ValuePtr(), iRemoteAddr);
					aNakList.Append(opt);
					}
				else
					aRejList.Append(opt);
				}
			else
				aAckList.Append(opt);
			}
			break;
		case KPppIpcpOptIpAddresses:
         {
			TUint32 remoteAddr = BigEndian::Get32(opt.ValuePtr());
			TUint32 addr = BigEndian::Get32(opt.ValuePtr()+4);

         if (remoteAddr == 0)
            {
				if (iPppLcp->PppLinkMode()!=EPppLinkIsServer)
					aRejList.Append(opt);
            }
         else if (addr == 0)
            {
				aNakList.Append(opt);
				}
			else
            {
				aAckList.Append(opt);
            }
         }

//		case KPppIpcpOptPrimaryDnsAddress:
//			aAckList.Append(opt);
//			break;
//		case KPppIpcpOptSecondaryDnsAddress:
//			aAckList.Append(opt);
//			break;
		default:
			aRejList.Append(opt);
			break;
			}
		}*/
	}

CVJCompressorIf* CPppNcpIp::LoadVJCompressorL(TInt aMaxVJSlots, TInt aCompressConnId)
	{
	CVJCompFactory* Factory=NULL;
	CVJCompressorIf*	VJCompressor;
											 
	Factory = (CVJCompFactory*)FindPppFactoryL(KVjComp, TUid::Uid(KSharedLibraryUidValue), *iVJCompressorCon);

	CleanupStack::PushL(TCleanupItem(CNifFactory::Cleanup, Factory));
	VJCompressor = Factory->NewVJCompressorL(aMaxVJSlots, aCompressConnId);
	CleanupStack::PopAndDestroy(); // Close extra reference on Factory

	return VJCompressor;

	}

CVJDeCompressorIf* CPppNcpIp::LoadVJDeCompressorL(TInt aMaxVJSlots)
	{
	CVJCompFactory*		Factory=NULL;
	CVJDeCompressorIf*	VJDeCompressor;
											 
	Factory = (CVJCompFactory*)FindPppFactoryL(KVjComp, TUid::Uid(KSharedLibraryUidValue), *iVJDeCompressorCon);
	CleanupStack::PushL(TCleanupItem(CNifFactory::Cleanup, Factory));
	VJDeCompressor = Factory->NewVJDeCompressorL(aMaxVJSlots);
	CleanupStack::PopAndDestroy(); // Close extra reference on Factory

	return VJDeCompressor;

	}


void CPppNcpIp::FsmApplyConfigRequest(RPppOptionList& /*aRequestList*/)
//
// Apply options in a recvd config request (that was ACK'd)
//
	{
	/*TMBufPktQIter iter(aRequestList);
	RPppOption opt;
	while (opt = iter++, !opt.IsEmpty())
		{
		switch (opt.OptType())
			{
		case KPppIpcpOptIpCompressionProtocol:
			//
			// Load the VJ DLL
			//
			if (iVJCompressor != NULL)
				{
				iVJCompressor->Close();
				iVJCompressor = NULL;
				}

			TRAPD( Ret, iVJCompressor = LoadVJCompressorL();)
			if (Ret != KErrNone)
				{
				//
				// Couldn't load the DLL for some reason!!!
				// We're in trouble now, the only reason I can
				// forsee is Out of memory PRR 18/3/98
				//
				FsmAbort(Ret);
				}
			break;

		case KPppIpcpOptIpAddress:
			iRemoteAddr = BigEndian::Get32(opt.ValuePtr());
			if (iRemoteAddr==0)			
				FsmAbort(KErrNotReady);
			break;
		case KPppIpcpOptIpAddresses:
			iRemoteAddr = BigEndian::Get32(opt.ValuePtr());
			if (iRemoteAddr==0)			
				FsmAbort(KErrNotReady);
			iLocalAddr = BigEndian::Get32(opt.ValuePtr()+4);
			if (iLocalAddr==0)			
				FsmAbort(KErrNotReady);
			break;

//		case KPppIpcpOptPrimaryDnsAddress:
//			iPrimaryDns = BigEndian::Get32(opt.ValuePtr());
//			break;
//		case KPppIpcpOptSecondaryDnsAddress:
//			iSecondaryDns = BigEndian::Get32(opt.ValuePtr());
//			break;
			}
		}*/
	}

void CPppNcpIp::FsmRecvConfigAck(RPppOptionList& /*aReplyList*/)
//
// Recvd a Config Ack - apply the options
//
	{
	/*TMBufPktQIter iter(aReplyList);
	RPppOption opt;
	while (opt = iter++, !opt.IsEmpty())
		{
		switch (opt.OptType())
			{
		case KPppIpcpOptIpCompressionProtocol:
			if (iVJDecompressor != NULL)
				{
				iVJDecompressor->Close();
				iVJDecompressor = NULL;
				}

			TRAPD( Ret, iVJDecompressor = LoadVJDeCompressorL();)
			if (Ret != KErrNone)
				{
				//
				// Couldn't load the DLL for some reason!!!
				//
				iPppLcp->Stop(Ret,MNifIfNotify::EDisconnect);
				}
			break;
		case KPppIpcpOptIpAddress:
			iLocalAddr = BigEndian::Get32(opt.ValuePtr());
			if (iLocalAddr==0)			
				FsmAbort(KErrNotReady);
			break;
		case KPppIpcpOptPrimaryDnsAddress:
			if(BigEndian::Get32(opt.ValuePtr()))
			    iPrimaryDns = BigEndian::Get32(opt.ValuePtr());
			break;
		case KPppIpcpOptSecondaryDnsAddress:
			if(BigEndian::Get32(opt.ValuePtr()))
			    iSecondaryDns = BigEndian::Get32(opt.ValuePtr());
			break;
		case KPppIpcpOptPrimaryNbnsAddress:
			if(BigEndian::Get32(opt.ValuePtr()))
			    iPrimaryNbns = BigEndian::Get32(opt.ValuePtr());
			break;
		case KPppIpcpOptSecondaryNbnsAddress:
			if(BigEndian::Get32(opt.ValuePtr()))
			    iSecondaryNbns = BigEndian::Get32(opt.ValuePtr());
			break;
			}
		}*/
	}

void CPppNcpIp::FsmRecvConfigNak(RPppOptionList& /*aReplyList*/, RPppOptionList& /*aReqList*/)
//
// Recvd a Config Nak - The associated original request is in aReqList
//
	{
/*	TMBufPktQIter iter(aReplyList);
	RPppOption opt;
	while (opt = iter++, !opt.IsEmpty())
		{
		switch (opt.OptType())
			{
		case KPppIpcpOptIpCompressionProtocol:
			aReqList.ReplaceOption(opt);
			break;
		case KPppIpcpOptIpAddress:
			aReqList.ReplaceOption(opt);
			break;
		case KPppIpcpOptPrimaryDnsAddress:
			aReqList.ReplaceOption(opt);
			break;
		case KPppIpcpOptSecondaryDnsAddress:
			aReqList.ReplaceOption(opt);
			break;
		default:
			aReqList.ReplaceOption(opt);
			break;
			}
		}*/
	}

void CPppNcpIp::FsmRecvConfigReject(RPppOptionList& /*aReplyList*/, RPppOptionList& /*aReqList*/)
//
// Recvd a Config Reject - The associated original request is in aReqList
//
	{
	/*TMBufPktQIter iter(aReplyList);
	RPppOption opt;
	while (opt = iter++, !opt.IsEmpty())
		{
		switch (opt.OptType())
			{
		case KPppIpcpOptIpCompressionProtocol:
			aReqList.RemoveOption(opt);
			break;
		case KPppIpcpOptIpAddress:
			aReqList.RemoveOption(opt);
			FsmAbort(KErrNotReady);
			break;
		case KPppIpcpOptPrimaryDnsAddress:
			aReqList.RemoveOption(opt);
			break;
		case KPppIpcpOptSecondaryDnsAddress:
			aReqList.RemoveOption(opt);
			break;
		default:
			aReqList.RemoveOption(opt);
			break;
			}
		}*/
	}

void CPppNcpIp::KillProtocol()
	{
	// This call would cause all NCPs to fail to connect
	// iPppLcp->Stop(KErrCouldNotConnect,MNifIfNotify::EDisconnect);

	// This shuts down only this NCP
	/*(FsmAbort(KErrCouldNotConnect);*/
	}

TBool CPppNcpIp::FsmRecvUnknownCode(TUint8 /*aCode*/, TUint8 /*aId*/, TInt /*aLength*/, RMBufChain& /*aPacket*/)
//
// Recvd an unrecognised opcode - has a default implementation
//
	{
	return EFalse;
	
	}


TBool CPppNcpIp::FsmOptionsValid(RPppOptionList& /*aList*/, RPppOptionList& /*aRequestList*/)
/**
Perform validation checking on the option list of a ConfigAck or ConfigReject.

@param aList option list of incoming ConfigAck or ConfigReject
@return ETrue if options valid, else EFalse.  EFalse return causes packet to be discarded.
*/
	{
	return ETrue;
	}

TBool CPppNcpIp::FsmConfigRequestOptionsValid(RPppOptionList& /*aList*/)
/**
Perform validation checking on the option list of a ConfigRequest.

@param aList option list of incoming ConfigRequest
@return ETrue if options valid, else EFalse.  EFalse return causes packet to be discarded.
*/
	{
	return ETrue;
	}




//


void CPppNcpIp::RecvIp(RMBufChain& /*aPacket*/)
	{
	/*if (iNetwork)
		iNetwork->Process(aPacket, (CProtocolBase*)this);
	else
		aPacket.Free();*/
	}


//
// All three of these are grouped together because of the way 
// this was intially designed.
// Kill Protocol is called because the protocol was rejected
// in the case of NCPIP I can't see this happening, but if it does
// we need to kill off PPP
//
void CPppNcpIp::IpKillProtocol()
	{
	/*iNotify->LinkLayerDown(KErrAccessDenied, MNifIfNotify::EDisconnect);
	return;*/
	}

void CPppNcpIp::VjCompTcpKillProtocol()
	{
	return;
	}

void CPppNcpIp::VjUncompTcpKillProtocol()
	{
	return;
	}


void CPppNcpIp::IpFrameError()
	{
	return;
	}

void CPppNcpIp::VjCompTcpFrameError()
	{
	/*if (iVJDecompressor)
		{
		LOG(iPppLcp->iLogger->Printf(_L("CRC CRC CRC \n\n\n\n"));)
		iVJDecompressor->CRCError();
		}
	return;*/
	}

void CPppNcpIp::VjUncompTcpFrameError()
	{
	return;
	}

void CPppNcpIp::RecvVjCompTcp(RMBufChain& /*aPacket*/)
	{
	/*if (iVJDecompressor)
		{
		if (!iVJDecompressor->DecompVJComp(aPacket))
			{
			aPacket.Free();
			}
		else
			{
			RecvIp(aPacket);
			}
		}
	else
		{
		aPacket.Free();
		}
	return;*/
	}

void CPppNcpIp::RecvVjUncompTcp(RMBufChain& /*aPacket*/)
	{
	/*if (iVJDecompressor)
		{
		if (!iVJDecompressor->DecompVJUncomp(aPacket))
			{
			aPacket.Free();
			}
		else
			{
			RecvIp(aPacket);
			}
		}
	else
		{
		aPacket.Free();
		}*/
	}

void CPppNcpIp::FsmTerminationPhaseComplete()
	{
	}

TInt CPppNcpIp::SendProtFrame(RMBufChain& /*aPacket*/, TUint /*aProtocol*/)
	{

	/*RMBufPktInfo* info = RMBufPacket::PeekInfoInChain(aPacket);

	TPppAddr addr;
	addr.SetProtocol(aProtocol);
	info->iDstAddr = addr;

	iSendQ.Append(aPacket);
	iSendCallBack->CallBack();

	if (!FsmIsThisLayerOpen() || !iLowerFlowOn)
		{
		iUpperFlowOn = EFalse;
		}

	return iUpperFlowOn;*/
	return ETrue;
	}

TInt CPppNcpIp::Send(RMBufChain& /*aPacket*/, TAny*)
	{

	/*RMBufPacket packet;
	packet.Assign(aPacket);

	TInt Protocol;
	if (iVJCompressor)
		{
		Protocol = iVJCompressor->VJCompressFrame(packet);
		return SendProtFrame(packet, Protocol );
		}
	else
		{
		//	Send normal IP frame
		return SendProtFrame(packet, KPppIdIp);
		}*/
		return ETrue;

	}

TInt CPppNcpIp::Notification(TAgentToNifEventType /*aEvent*/, void* /*aInfo*/)
	{
	/*__ASSERT_DEBUG(aEvent==EAgentToNifEventTypeModifyInitialTimer, PppPanic(EPppPanic_IncorrectNcpNotif));
    if(aEvent==EAgentToNifEventTypeModifyInitialTimer)
        {
        ChangeTimers(ETrue);
        }*/
	return KErrNone;
	}
