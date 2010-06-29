// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <TRENG.H>
#include <e32hal.h>
#include <icmp6_hdr.h>
#include <in_chk.h>
#include <commdbconnpref.h>

const TUint KDefaultMaxTtl = 30;
const TInt KDefaultWait = 5000000;
const TInt KDefaultNrProbes = 3;

const TInt KMinIpHeaderSize = 20;
const TInt KMaxSendTime=60000000;
const TInt KRecvDataSize=512;
const TInt KResolvTime=10000000;

const TInt KIcmpHeaderSize = 8;

class CTraceRtTimer : public CTimer
	{
friend class CTraceRtEng;
protected:
	CTraceRtTimer(CTraceRtEng& aParent);
	void RunL();
private:
	CTraceRtEng* iParent;
	};

class CTraceRtSender : public CActive
	{
friend class CTraceRtEng;
protected:
	CTraceRtSender(CTraceRtEng& aParent);
	~CTraceRtSender();
	void RunL();
	void DoCancel();
private:
	CTraceRtEng* iParent;
	};

class CTraceRtReceiver : public CActive
	{
friend class CTraceRtEng;
protected:
	CTraceRtReceiver(CTraceRtEng& aParent);
	~CTraceRtReceiver();
	void RunL();
	void DoCancel();
private:
	CTraceRtEng* iParent;
	};


// Class to manage ICMP headers information
class HTraceRtHeader : public TInet6HeaderICMP_Echo
	{
public:
	static HTraceRtHeader* NewL(TInt aSize = KIcmpHeaderSize);
	~HTraceRtHeader();

	TBool VerifyRecv(TInt aIdent, TInt aSeq);
	void FormatSend(TUint aId, TUint aSeq);
	TInt MaxLength();
	TInt DataLength();
	TPtr8* Grab();

private:
	void ConstructL(TInt aSize);
	TBool SetHeader(TUint aOffset = 0);
	
	HBufC8* iData;
	TPtr8* iDataPtr;	// Packet data
	};

enum TTraceRtEngPanic
	{

    ETimerPriorityGreaterThanSender,	// 0
	ESenderPrirityGreaterThanReceiver	// 1
	};

LOCAL_C void Panic(TTraceRtEngPanic aPanic)
//
// Panic the user
//
	{

	User::Panic(_L("TraceRtEng"), aPanic);
	}

EXPORT_C TTraceRtOptions::TTraceRtOptions()
//
// Default TraceRt options
//
	{

	iMaxTtl=KDefaultMaxTtl;
	iResolveAddress=ETrue;
	iWait=KDefaultWait;
	iTos=0;
	iDestname.SetLength(0);
	iNrProbes=KDefaultNrProbes;
	iPrompt = EFalse;
	}

EXPORT_C CTraceRtEng* CTraceRtEng::NewL(MTraceRtNotificationHandler& aUi)
//
// Create a new TraceRt engine
//
	{

	CTraceRtEng* p= new(ELeave) CTraceRtEng;
	CleanupStack::PushL(p);
	p->ConstructL(aUi);
	CleanupStack::Pop();
	return p;
	}

EXPORT_C CTraceRtEng::CTraceRtEng()
//
// Declare a name
//
	{	
	
	__DECLARE_NAME(_S("CTraceRtEng"));
	}

EXPORT_C CTraceRtEng::~CTraceRtEng()
//
// Destroy wot TraceRt created
	{

	iIcmpSocket.Close();
	iResolver.Close();
	iConnect.Close();
	iSocketServ.Close();
	delete iReceiver;
	delete iSender;
	delete iTimer;
	delete iRecvData;
	delete iSendData;
	}

EXPORT_C void CTraceRtEng::ConstructL(MTraceRtNotificationHandler& aUi)
//
// Construct and heap objects
//
	{

	iUi = &aUi;
	iTimer = new (ELeave) CTraceRtTimer(*this);
	iTimer->ConstructL();
	iSender = new (ELeave) CTraceRtSender(*this);
	iReceiver = new (ELeave) CTraceRtReceiver(*this);
	User::LeaveIfError(iSocketServ.Connect());
	iSendData = NULL;
	iRecvData = NULL;
	iResolv = EFalse;
	}

EXPORT_C void CTraceRtEng::SetPriorities(TInt aTimerPriority, TInt aSenderPriority, TInt aReceiverPriority)
//
//	Set various active object priorities
//
	{	

	__ASSERT_ALWAYS(aTimerPriority < aSenderPriority, Panic(ETimerPriorityGreaterThanSender));
	__ASSERT_ALWAYS(aSenderPriority < aReceiverPriority, Panic(ESenderPrirityGreaterThanReceiver));

	iTimer->SetPriority(aTimerPriority);
	iSender->SetPriority(aSenderPriority);
	iReceiver->SetPriority(aReceiverPriority);
	}

EXPORT_C void CTraceRtEng::Start(const TTraceRtOptions& aOptions)
//
// Start a TraceRt
//
	{

	// Reset All Variables
	iProbeNr=0;
	iTtl=1;
	iSeq=0;
	iNameEntry().iName.SetLength(0);
	iNameEntry().iFlags=0;
	TInetAddr::Cast(iNameEntry().iAddr).SetAddress(0);
	iGotThere=EFalse;
	iUnreachCount=0;
	iIdent=User::TickCount()&KMaxTUint16;
	iLastCode=KTraceRtCodeTimeout;

	iOptions=aOptions;


    if(iOptions.iMaxTtl>255 || iOptions.iTos>255
		|| iOptions.iNrProbes>100 || iOptions.iNrProbes<1)
		{
		DoError(KErrNotSupported);
		return;
		}

	if(iSendData)
		{
		delete iSendData;
		}
	iSendData = HTraceRtHeader::NewL(KIcmpHeaderSize);

	if(iRecvData)
		{
		delete iRecvData;
		}
	iRecvData = HTraceRtHeader::NewL(KRecvDataSize);

	if(!iRecvData || !iSendData)
		{
		DoError(KErrNoMemory);
		}
	else if(iOptions.iDestname.Length()>0)
		{	
		iConnect.Close();
		User::LeaveIfError(iConnect.Open(iSocketServ, KConnectionTypeDefault));
		
		TCommDbConnPref commDbPref;
		if(!iOptions.iPrompt)
			{
			commDbPref.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);
			}
		else
			{
			commDbPref.SetDialogPreference(ECommDbDialogPrefPrompt);
			}
		User::LeaveIfError(iConnect.Start(commDbPref));
		
		TInt err=iResolver.Open(iSocketServ, KAfInet, KProtocolInetUdp, iConnect);
		if(err==KErrNone)
			{

			TInetAddr& addr = (TInetAddr&)iNameEntry().iAddr;
			if ((err=addr.Input(iOptions.iDestname))==KErrNone)
				{

				if(iOptions.iResolveAddress)
					{
					iState=ELookingUpHost;
					iResolver.GetByAddress(addr, iNameEntry, iSender->iStatus);
					iSender->SetActive();
					iTimer->After(KResolvTime);
					}
				else
					{
					iSender->iStatus=KErrNone;
					iState=ELookingUpHost;
					SendComplete();
					}
				}
			else
				{
				iState=ELookingUpHost;
				iResolver.GetByName(iOptions.iDestname, iNameEntry, iSender->iStatus);

				iSender->SetActive();
				iTimer->After(KResolvTime);
				}
			}
		else
			{
			DoError(err);
			}
		}
	else
		{
		DoError(KErrBadName);
		}
	}

EXPORT_C void CTraceRtEng::CancelAndFinished()
//
// Cancel from the UI
//
	{
	
	if(iState!=EStopped || iTimer->IsActive()) 
		{
		Cancel();
		iUi->Finished(KErrCancel);
		}
	}

EXPORT_C void CTraceRtEng::Cancel()
//
// Cancel a TraceRt in progress
//
	{

	iSender->Cancel();
	iReceiver->Cancel();	
	iTimer->Cancel();
	
	iIcmpSocket.Close();
	iResolver.Close();

	iState=EStopped;
	}

void CTraceRtEng::DoError(TInt aError)
//
// Generate an error from somewhere
//
	{
	
	Cancel();
	iTimer->SetActive();
	TRequestStatus* p = &iTimer->iStatus;
	User::RequestComplete(p, aError);
	}

void CTraceRtEng::TimerComplete()
//
// Timer event completed
//
	{

	if(iTimer->iStatus==KErrNone)
		{

		if(iSender->IsActive())
			{
			if(iResolv)
				{
				// We didn't manage to get the name
				// from the address, so we pass on
				iResolv = EFalse;
				// Reset sender
				iSender->Cancel();
				// Clear previous name
				iNameEntry().iName.SetLength(0);
				// Continue
				NextSend();
				}
			else
				{
				Cancel();
				iUi->Finished(KErrTimedOut);
				}
			}
		else
			{
			iUi->Reply(iProbeNr, 0, KTraceRtCodeTimeout);
			switch(iLastCode)
				{
			case KTraceRtCodeUnreachNet:
			case KTraceRtCodeUnreachHost:
				++iUnreachCount;
				break;
			default:
				break;
				}
			NextSend();
			}
		}
	else
		{
		Cancel();
		iUi->Finished(iTimer->iStatus.Int());
		}
	}

void CTraceRtEng::SendComplete()
//
// A send operation has completed
//
	{

	TInt err=iSender->iStatus.Int();
	iTimer->Cancel();
	iResolv = EFalse;

	switch(iState)
		{

	case ELookingUpHost:
		iDstAddr = iNameEntry().iAddr;
		if(iDstAddr.Address()!=0)
			{		
			TInt res=iIcmpSocket.Open(iSocketServ, KAfInet, KSockDatagram, KProtocolInetIcmp, iConnect);
			
			if(res==KErrNone)
				{
				res=iIcmpSocket.SetOpt(KSORecvBuf, KSOLSocket, iRecvData->MaxLength());
				}
			if(res==KErrNone)
				{
				res=iIcmpSocket.SetOpt(KSOSendBuf, KSOLSocket, iSendData->MaxLength());
				}
			if(res==KErrNone)
				{
				res=iIcmpSocket.SetOpt(KSoIpTOS, KSolInetIp, (TInt)iOptions.iTos);
				}

			if(res!=KErrNone)
				{
				DoError(res);
				return;
				}
			
			// Tell the UI
			iUi->Starting(iNameEntry(), iOptions.iMaxTtl, iSendData->MaxLength());
			iState=ESending;
			NextSend();
			}
		else
			{
			DoError(err!=KErrNone ? err : KErrNotFound);
			}
		break;

	case ELookingUpReply:
		if(err!=KErrNone)
			{
			iNameEntry().iName.SetLength(0);
			}
		NextSend();
		break;

	default:
		if(err==KErrNone)
			{
			iTimer->After(iOptions.iWait);
			iIcmpSocket.RecvFrom(*(iRecvData->Grab()), iSrcAddr, 0, iReceiver->iStatus);
			iReceiver->SetActive();
			}
		else
			{
			DoError(err);
			}
		}
	}

void CTraceRtEng::NextSend()
//
// Initiate the next send
//
	{

	iReceiver->Cancel();

	if(iProbeNr>=iOptions.iNrProbes)
		{

		if(iState==ELookingUpReply || !iOptions.iResolveAddress || TInetAddr::Cast(iNameEntry().iAddr).Address()==0)
			{

			if(!iOptions.iResolveAddress)
				{
				iNameEntry().iName.SetLength(0);
				}

			iUi->FromHost(iNameEntry());
			if(iGotThere || iUnreachCount>=iOptions.iNrProbes )
				{
				Cancel();
				iUi->Finished(KErrNone);
				return;
				}

			TInetAddr::Cast(iNameEntry().iAddr).SetAddress(0);
			++iTtl;
			iProbeNr=0;
			iLastCode=KTraceRtCodeTimeout;
			iUnreachCount=0;
			iState=ESending;
			}
		else
			{
			iState=ELookingUpReply;

			// Work around for bug in pre-61 versions of TCPIP
			TProtocolDesc info;
			if(iIcmpSocket.Info(info)==KErrNone)
				{
				TVersion &v=info.iVersion;
				if(v.iMajor==1 && v.iMinor==0 && v.iBuild<61)
					{
					iResolver.Close();
					TInt res=iResolver.Open(iSocketServ, KAfInet, KProtocolInetUdp, iConnect);
					if(res!=KErrNone)
						{
						DoError(res);
						return;
						}
					}
				}
			//  Work around ends

			iResolver.GetByAddress(iNameEntry().iAddr, iNameEntry, iSender->iStatus);
			iSender->SetActive();
			iResolv = ETrue;
			iTimer->After(KResolvTime);
			return;
			}
		}

	if(iTtl>iOptions.iMaxTtl)
		{
		Cancel();
		iUi->Finished(KErrNone);
		return;
		}

	if(iProbeNr==0)
		{
		iUi->Probe(iTtl);
		}
	
	iSendData->FormatSend(++iSeq, iIdent);
	TInt res=iIcmpSocket.SetOpt(KSoIpTTL, KSolInetIp, (TInt)iTtl);
	if(res!=KErrNone)
	{
		DoError(res);
		return;
	}
	iIcmpSocket.SendTo(*(iSendData->Grab()), iDstAddr, 0, iSender->iStatus);
	iSendTime.UniversalTime();
	iSender->SetActive();
	iTimer->After(KMaxSendTime);
//	pg 13/03/2000 - Base removing this api
//	UserHal::ResetAutoSwitchOffTimer();

	++iProbeNr;
	return;
	}

void CTraceRtEng::SendDoCancel()
//
// A send operation requires cancelling
//
	{

	if(iState==ELookingUpHost || iState==ELookingUpReply)
		{
		iResolver.Cancel();
		}
	else if(iState==ESending)
		{
		iIcmpSocket.CancelSend();
		}
	}

void CTraceRtEng::RecvComplete()
//
// A recv operation has completed
//
	{

	if(iReceiver->iStatus==KErrNone)
		{
		
		if(iState!=ESending)
			{
			return;
			}

		if(iRecvData->VerifyRecv(iSeq, iIdent))
			{
			
			iNameEntry().iAddr = iSrcAddr;
			TTime now;
			now.UniversalTime();
			TTimeIntervalMicroSeconds delta = now.MicroSecondsFrom(iSendTime);
			TUint code;

			switch(iRecvData->Type())
				{
			case KTraceRtTypeTimeExceeded:
				code = KTraceRtCodeTimedOutInTransit;
				break;
			case KTraceRtTypeEchoReply:
				code = KTraceRtCodeEchoReply;
				iGotThere=ETrue;
				break;
			default:
				code=iRecvData->Code();
				++iUnreachCount;
				}

			iLastCode=code;

			delta = delta.Int64()/TInt64(1000);
			iUi->Reply(iProbeNr, I64LOW(delta.Int64()), code);

			iTimer->Cancel();	
			NextSend();
			return;
			}
		if(!iSender->IsActive())
			{
			iLastCode = iRecvData->Code();
			iIcmpSocket.RecvFrom(*(iRecvData->Grab()), iSrcAddr, 0, iReceiver->iStatus);
			iReceiver->SetActive();
			}
		}
	else
		{
		DoError(iReceiver->iStatus.Int());
		}
	}

void CTraceRtEng::RecvDoCancel()
//
// A send operation requires cancelling
//
	{

	iIcmpSocket.CancelRecv();
	}

CTraceRtTimer::CTraceRtTimer(CTraceRtEng& aParent)
//
// To time events
//
	: CTimer(ETraceRtTimerPriority)
	{
	
	iParent = &aParent;
	CActiveScheduler::Add(this);
	__DECLARE_NAME(_S("CTraceRtTimer"));
	}

void CTraceRtTimer::RunL()
//
//	Timer is complete
//
	{

	iParent->TimerComplete();
	}

CTraceRtSender::CTraceRtSender(CTraceRtEng& aParent)
//
// C'tor
//
	: CActive(ETraceRtSenderPriority)
	{

	iParent = &aParent;
	CActiveScheduler::Add(this);
	__DECLARE_NAME(_S("CTraceRtSender"));
	}

CTraceRtSender::~CTraceRtSender()
//
// D'tor cancels
//
	{

	Cancel();
	}

void CTraceRtSender::RunL()
//
// Upcall to parent
//
	{

	iParent->SendComplete();
	}

void CTraceRtSender::DoCancel()
//
// Get parent to cancel send
//
	{
	
	iParent->SendDoCancel();
	}

CTraceRtReceiver::CTraceRtReceiver(CTraceRtEng& aParent)
//
// C'tor
//
	: CActive(ETraceRtReceiverPriority)
	{

	iParent = &aParent;
	CActiveScheduler::Add(this);
	__DECLARE_NAME(_S("CTraceRtReceiver"));
	}

CTraceRtReceiver::~CTraceRtReceiver()
//
// D'tor cancels
//
	{

	Cancel();
	}

void CTraceRtReceiver::RunL()
//
// Upcall to parent
//
	{

	iParent->RecvComplete();
	}

void CTraceRtReceiver::DoCancel()
//
// Get parent to cancel send
//
	{
	
	iParent->RecvDoCancel();
	}


HTraceRtHeader::~HTraceRtHeader()
//
// D'tor deletes 
//
	{
	
	delete iData;
	delete iDataPtr;
	}

HTraceRtHeader* HTraceRtHeader::NewL(TInt aSize)
//
// Create a new trace route header
//
	{	
	
	HTraceRtHeader* h = new(ELeave) HTraceRtHeader();

	CleanupStack::PushL(h);
	h->ConstructL(aSize);
	CleanupStack::Pop(h);	

	return h;
	}

void HTraceRtHeader::ConstructL(TInt aSize)
	{
	
	iData = HBufC8::NewL(aSize);
	iDataPtr = new(ELeave) TPtr8(iData->Des());
	
	iData->Des().FillZ();
	}

TInt HTraceRtHeader::MaxLength()
	{
	
	return iData->Des().MaxLength(); 
	}

TInt HTraceRtHeader::DataLength()
	{
	
	return iData->Des().Length(); 
	}

TPtr8* HTraceRtHeader::Grab()
	{
	
	iDataPtr->Copy(iData->Des());
	return iDataPtr;
	}

TBool HTraceRtHeader::SetHeader(TUint aOffset)
//
// Set the header from an Icmp reply
// 
	{
	
	const TUint8* buffData;
	
	// Check size
	if(DataLength() < KIcmpHeaderSize)
		{
		return EFalse;
		}
	
	buffData = iData->Des().Ptr();

	if(!buffData)
		{
		return EFalse;
		}
	
	// Fill TInet6HeaderICMP_Echo from the buffer
	for(int k=0;k<KIcmpHeaderSize;k++)
		{
		i[k] = *(buffData + k + aOffset);
		}

	return ETrue;
	}

TBool HTraceRtHeader::VerifyRecv(TInt aSeq, TInt aIdent)
//
// Verify header is valid
//
	{
	
	TBool ret = SetHeader();

	if(ret)
		{
		ret = EFalse;
		
		if(Type() == KTraceRtTypeEchoReply && Identifier() == aIdent && Sequence() == aSeq)
			{
			ret = ETrue;
			}

		if(!ret && ((Type() == KTraceRtTypeTimeExceeded && Code() == KTraceRtCodeExceedInTransit) || Type() == KTraceRtTypeUnreachable))
			{
			TUint code = Code();
			TUint type = Type();
			ret = SetHeader(KMinIpHeaderSize + KIcmpHeaderSize);

			if(ret)
				{
				ret = EFalse;
			
				if(Type() != KTraceRtTypeEchoRequest || Identifier() != aIdent || Sequence() != aSeq)
					{
					ret = EFalse;
					}
				else
					{
					ret = ETrue;
					}
				}
			SetCode(static_cast<TUint8>(code));
			SetType(static_cast<TUint8>(type));
			}
		}

	return ret;
	}


void HTraceRtHeader::FormatSend(TUint aSeq, TUint aIdent)
//
// Format an ICMP Header
//
	{
	
	TChecksum sum;

	// Fill header
	SetType(KTraceRtTypeEchoRequest);
	SetCode(KTraceRtCodeEcho);
	SetIdentifier(static_cast<TUint16>(aIdent));
	SetSequence(static_cast<TUint16>(aSeq));

	// Compute cheksum
	SetChecksum(0);
	sum.Add(reinterpret_cast<TUint16*>(this), HeaderLength());
	SetChecksum(sum.Sum());

	// Copy the ICMP Header in the buffer
	iData->Des().Copy((TUint8*)this, HeaderLength());
	}

