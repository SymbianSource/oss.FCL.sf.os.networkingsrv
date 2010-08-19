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

#include <PINGENG.H>
#include <e32hal.h>
#include <icmp6_hdr.h>
#include <in_chk.h>
#include <commdbconnpref.h>
#include <connpref.h>

const TInt KDefaultPings =  4;
const TInt KDefaultInterval = 1000000;
const TInt KDefaultWait = 10000000;
const TInt KDefaultPingSize = 32;
const TInt KDefaultBacklog = 512;

const TInt KMaxTimeOnQue=KDefaultBacklog*KDefaultInterval;
const TInt KMaxSendTime=60000000;

const TUint KIcmpHeaderSize = 8;
const TUint KMinIpHeaderSize = 20;

class CPingTimer : public CTimer
	{
friend class CPingEng;
protected:
	CPingTimer(CPingEng& aParent);
	void RunL();
private:
	CPingEng& iParent;
	};

class CPingSender : public CActive
	{
friend class CPingEng;
protected:
	CPingSender(CPingEng& aParent);
	~CPingSender();
	void RunL();
	void DoCancel();
private:
	CPingEng& iParent;
	};

class CPingReceiver : public CActive
	{
friend class CPingEng;
protected:
	CPingReceiver(CPingEng& aParent);
	~CPingReceiver();
	void RunL();
	void DoCancel();
private:
	CPingEng& iParent;
	};


class HPingHeader : public TInet6HeaderICMP_Echo
	{
public:
	static HPingHeader* NewL(TInt aSize = KIcmpHeaderSize, TUint aIPVersion = KAfInet);
	~HPingHeader();

	TBool VerifyRecvEcho(TInt aId);
	TBool VerifyNonEcho(TInt aId);
	void SetVersion(TUint aIPVersion);
	void FormatSend(TUint aId, TUint aSeqNum);
	TInt MaxLength();
	TInt DataLength();
	TPtr8* Grab();
	TPtrC8 IcmpContents();

private:
	void ConstructL(TInt aSize, TUint aIPVersion);
	TBool SetHeader(TUint aOffset = 0);
	
	HBufC8* iData;
	TPtr8* iDataPtr;
	TUint iIPVersion;
	TInt iSize;
	};

enum TPingEngPanic
	{

	ETimerPriorityGreaterThanSender,	// 0
	ESenderPrirityGreaterThanReceiver	// 1
	};

LOCAL_C void Panic(TPingEngPanic aPanic)
//
// Panic the user
//
	{

	User::Panic(_L("PingEng"), aPanic);
	}

EXPORT_C TPingOptions::TPingOptions()
//
// Default ping options
//
	{

	iNumberOfPings=KDefaultPings;
	iInterval = KDefaultInterval;
	iWait = KDefaultWait;
	iPingSize=KDefaultPingSize;
	iResolveAddress=EFalse;
	iPrompt=EFalse;
	iConnIap=0;
	iConnSnap=0;
	iPreload=0;
	iBacklog=KDefaultBacklog;
	}

EXPORT_C CPingEng* CPingEng::NewL(MPingNotificationHandler& aUi)
//
// Create a new ping engine
//
	{

	CPingEng* p= new(ELeave) CPingEng(aUi);
	CleanupStack::PushL(p);
	p->ConstructL();
	CleanupStack::Pop(p);
	return p;
	}

CPingEng::CPingEng(MPingNotificationHandler& aUi)
: iUi(aUi)
//
// Declare a name
//
	{	
	
	__DECLARE_NAME(_S("CPingEng"));
	}

EXPORT_C CPingEng::~CPingEng()
//
// Destroy wot ping created
	{

	iSocket.Close();
	iResolver.Close();
	iConnect.Close();
	iSocketServ.Close();
	delete iReceiver;
	delete iSender;
	delete iTimer;
	delete iRecvData;
	delete iSendData;

	EmptyPingRecordQue();
	}

void CPingEng::EmptyPingRecordQue()
	{

	while (!iQue.IsEmpty())
		{
		DeletePingRecord(iQue.First());
		}
	}

void CPingEng::DeletePingRecord(TPingRecord* aRecord)
	{
	
	iQue.Remove(*aRecord);
	delete aRecord;
	iNoInQue--;	
	}

void CPingEng::ConstructL()
//
// Construct and heap objects
//
	{

	iQue.SetOffset(_FOFF(TPingRecord, iLink));
	iTimer = new (ELeave) CPingTimer(*this);
	iTimer->ConstructL();
	iSender = new (ELeave) CPingSender(*this);
	iReceiver = new (ELeave) CPingReceiver(*this);
	User::LeaveIfError(iSocketServ.Connect());
	}

EXPORT_C void CPingEng::SetPriorities(TInt aTimerPriority, TInt aSenderPriority, TInt aReceiverPriority)
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

EXPORT_C void CPingEng::StartL(const TPingOptions& aOptions)
//
// Start a ping
//
	{

	// Reset All Variables
	iNrTransmitted=0;
	iNrReceived=0;
	iNrDuplicates=0;
	iMinTime=KMaxTInt;
	iMaxTime=0;
	iSumTime=0;
	iId=User::TickCount()&KMaxTUint16;
	iNameEntry().iName.SetLength(0);
	iNameEntry().iFlags=0;
	iIsLoopback = EFalse;

	iOptions=aOptions;

	if(iOptions.iDestname.Length()>0)
		{	

		TInetAddr& addr = (TInetAddr&)iNameEntry().iAddr;

		const TBool addressInputValid = addr.Input(iOptions.iDestname) == KErrNone;

		if (addressInputValid)
		    {
            iIsLoopback = addr.IsLoopback();
		    }
		        
		iConnect.Close();
		
		// Make a connection only if it is required.
		//
		if (!iIsLoopback)
            {
            // This branch will unfortunately be taken if the call to TInetAddr.Input() fails.

            
    		User::LeaveIfError(iConnect.Open(iSocketServ, KConnectionTypeDefault));

			if(iOptions.iConnSnap)
				{
				TConnSnapPref connPref(iOptions.iConnSnap);
				User::LeaveIfError(iConnect.Start(connPref));
				}
			else
				{
				TCommDbConnPref commDbPref;
				
				if(iOptions.iConnIap)
					{
					commDbPref.SetIapId(iOptions.iConnIap);
					}

				if(!iOptions.iPrompt)
					{
					commDbPref.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);
					}
				else
					{
					commDbPref.SetDialogPreference(ECommDbDialogPrefPrompt);
					}
				User::LeaveIfError(iConnect.Start(commDbPref));
				}			
			}

		
        TInt err;
        
		if (addressInputValid)
			{
			if(iOptions.iResolveAddress)
				{
				if((err=iResolver.Open(iSocketServ, KAfInet, KProtocolInetUdp, iConnect))==KErrNone)
					{
					iState=ELookingUp;
					iResolver.GetByAddress(addr, iNameEntry, iSender->iStatus);
					iSender->SetActive();
					iTimer->After(KMaxSendTime);
					}
				else
					{
					DoError(err);
					}
				}
			else
				{
				iSender->iStatus=KErrNone;
				iState=ELookingUp;
				SendCompleteL();
				}
			}
		else
			{
			if((err=iResolver.Open(iSocketServ, KAfInet, KProtocolInetUdp, iConnect))==KErrNone)
				{
				iState=ELookingUp;
				iResolver.GetByName(iOptions.iDestname, iNameEntry, iSender->iStatus);
				iSender->SetActive();
				iTimer->After(KMaxSendTime);
				}
			else
				{
				DoError(err);
				}
			}
		}
	else
		{
		DoError(KErrBadName);
		}
	}

EXPORT_C void CPingEng::CancelAndFinished()
//
// Cancel from the UI
//
	{
	
	if(iState!=EStopped || iTimer->IsActive())
		{
		Cancel();
		iUi.Finished(iNameEntry(), iNrTransmitted, iNrReceived, iNrDuplicates, iMinTime, iMaxTime, iSumTime, KErrCancel);
		}
	}

EXPORT_C void CPingEng::Cancel()
//
// Cancel a ping in progress
//
	{

	iSender->Cancel();
	iReceiver->Cancel();	
	iTimer->Cancel();
	iSocket.Close();
	iConnect.Close();
	iResolver.Close();

	EmptyPingRecordQue();

	iState=EStopped;
	}

void CPingEng::DoError(TInt aError)
//
// Generate an error from somewhere
//
	{
	
	Cancel();
	iTimer->SetActive();
	TRequestStatus* p = &iTimer->iStatus;
	User::RequestComplete(p, aError);
	}

void CPingEng::TimerComplete()
//
// Timer event completed
//
	{

	if(iTimer->iStatus==KErrNone)
		{

		if((iOptions.iNumberOfPings && iNrTransmitted>=iOptions.iNumberOfPings)
			|| iSender->IsActive())
			{
		
			Cancel();
			iUi.Finished(iNameEntry(), iNrTransmitted, iNrReceived, iNrDuplicates, iMinTime, iMaxTime, iSumTime, KErrTimedOut);
			}
		else
			{
			PurgeQue();
			NextSend();
			}
		}
	else
		{
		iUi.Finished(iNameEntry(), iNrTransmitted, iNrReceived, iNrDuplicates, iMinTime, iMaxTime, iSumTime, iTimer->iStatus.Int());
		}
	}

void CPingEng::SendCompleteL()
//
// A send operation has completed
//
	{
	if(iState==ELookingUp)
		{
		iResolver.Close();
		}

	if(iSender->iStatus!=KErrNone)
		{
 		DoError(iSender->iStatus.Int());
		return;
		}

	iTimer->Cancel();

	if(iState==ELookingUp) // Intiate the ping process
		{
		delete iSendData;
		iSendData = NULL;
		delete iRecvData;
		iRecvData = NULL;
		TInt res=KErrNone;

		// Set up socket and recieve/send packets depending on address type.
		switch(iNameEntry().iAddr.Family())
			{
		case KAfInet:
			{
			iSendData = HPingHeader::NewL(iOptions.iPingSize);
			iRecvData = HPingHeader::NewL(iOptions.iPingSize);
			if(!iRecvData || !iSendData)
				{
				DoError(KErrNoMemory);
				return;
				}
			
			if (iIsLoopback)
			    {
    			res=iSocket.Open(iSocketServ, KAfInet, KSockDatagram, KProtocolInetIcmp);
			    }
			else
		        {
      			res=iSocket.Open(iSocketServ, KAfInet, KSockDatagram, KProtocolInetIcmp, iConnect);
		        }
			break;
			}
		case KAfInet6:
			{
			iSendData = HPingHeader::NewL(iOptions.iPingSize, KAfInet6);
			iRecvData = HPingHeader::NewL(iOptions.iPingSize, KAfInet6);
			if(!iRecvData || !iSendData)
				{
				DoError(KErrNoMemory);
				return;
				}

			if (iIsLoopback)
			    {
    			res=iSocket.Open(iSocketServ, KAfInet, KSockDatagram, KProtocolInet6Icmp);
			    }
			else
			    {
    			res=iSocket.Open(iSocketServ, KAfInet, KSockDatagram, KProtocolInet6Icmp, iConnect);
			    }
			break;
			}
		default:
			DoError(KErrGeneral);
			return;
			}
			
		if(res==KErrNone)
			{
			res=iSocket.SetOpt(KSORecvBuf, KSOLSocket, iRecvData->MaxLength());
			}
		if(res==KErrNone)
			{
			res=iSocket.SetOpt(KSOSendBuf, KSOLSocket, iSendData->MaxLength());
			}

		if(res!=KErrNone)
			{
			DoError(res);
			return;
			}

		// Start the receiver
		iSocket.RecvFrom(*(iRecvData->Grab()), iSrcAddr, 0, iReceiver->iStatus);
		iReceiver->SetActive();
		
		// Tell the UI
		iUi.Pinging(iNameEntry(),iOptions.iPingSize);
		iState=ESending;
	    	NextSend();
		}
	else // Start appropriate timer
		{

		iUi.Sent();
		if(!iOptions.iNumberOfPings || iNrTransmitted<iOptions.iNumberOfPings)
			{
			if(iOptions.iPreload || !iOptions.iInterval.Int())
				{
				if(iOptions.iPreload>0)
					{
					--iOptions.iPreload;
					}
				iTimer->iStatus=KErrNone;
				TimerComplete();
				}
			else
				{
				iTimer->After(iOptions.iInterval);
				}
			}
		else if(iNrReceived)
			{
			iTimer->After(iMaxTime>500000 ? (iMaxTime*5) : 1000000);
			}
		else
			{
			iTimer->After(iOptions.iWait);
			}
		}
	}


void CPingEng::NextSend()
//
// Initiate the next send
//
	{
	TPingRecord* r = new TPingRecord(iNrTransmitted);
	if(!r)
		{
		DoError(KErrNoMemory);
		return;
		}

	iQue.AddLast(*r);
	iNoInQue++;

	if(iNoInQue > iOptions.iBacklog)
		{
		DeletePingRecord(iQue.First());
		}

	iSendData->FormatSend(iId, iNrTransmitted++);
	iSocket.SendTo(*(iSendData->Grab()), iNameEntry().iAddr, iSendFlags, iSender->iStatus);
	iSender->SetActive();
	iTimer->After(KMaxSendTime);

	}

void CPingEng::SendDoCancel()
//
// A send operation requires cancelling
//
	{

	if(iState==ELookingUp)
		{
		iResolver.Cancel();
		}
	else if(iState==ESending)
		{
		iSocket.CancelSend();
		}
	}

void CPingEng::RecvComplete()
//
// A recv operation has completed
//
	{

	if(iReceiver->iStatus==KErrNone)
		{
		
		if(iRecvData->VerifyRecvEcho(iId))
			{
			TSglQueIter<TPingRecord> iter(iQue);
		    	TPingRecord* p=0;
			while((p=iter++)!=0)
				{
				if(iRecvData->Sequence() == p->iSeqNr)
					{
					++iNrReceived;

					TTime now;
					now.UniversalTime();
					TTimeIntervalMicroSeconds32 word = I64LOW(now.MicroSecondsFrom(p->iSendTime).Int64());
					
					iMaxTime = word.Int()>iMaxTime ? word.Int() : iMaxTime;
					iMinTime = word.Int()<iMinTime ? word.Int() : iMinTime;
					iSumTime += word.Int();

					iUi.Reply(iSrcAddr, iRecvData->DataLength(), p->iSeqNr, word);

					DeletePingRecord(p);

					if(iQue.IsEmpty() && iOptions.iNumberOfPings && iNrReceived>=iOptions.iNumberOfPings 
						 && iNrTransmitted>=iOptions.iNumberOfPings)
						{
						Cancel();
						iUi.Finished(iNameEntry(), iNrTransmitted, iNrReceived, iNrDuplicates, iMinTime, iMaxTime, iSumTime, KErrNone);
						return;
						}
					break;
					}
				}
			if(p==0)
				{
		        	++iNrDuplicates;
				}
			}
		else if(iRecvData->VerifyNonEcho(iId))
			{
			if(iNameEntry().iAddr.Family()==KAfInet)
				{
				iUi.Icmp4Message(iSrcAddr, iRecvData->Type(), iRecvData->Code(), iRecvData->IcmpContents());
				}
			else
				{
				iUi.Icmp6Message(iSrcAddr, iRecvData->Type(), iRecvData->Code());
				}
			}
			
		iSocket.RecvFrom(*(iRecvData->Grab()), iSrcAddr, 0, iReceiver->iStatus);
		iReceiver->SetActive();
		}
	else
		{
		DoError(iReceiver->iStatus.Int());
		}
	}

void CPingEng::PurgeQue()
//
// Purge the que of things that linger about for too long
//
	{

	TTime shortTimeAgo;
	shortTimeAgo.UniversalTime();
	shortTimeAgo -= TTimeIntervalMicroSeconds32(Max(KMaxTimeOnQue, iOptions.iWait.Int()));

	TSglQueIter<TPingRecord> iter(iQue);
	TPingRecord* p;
	while((p=iter++)!=0)
		{
		if(p->iSendTime < shortTimeAgo)

			{
			DeletePingRecord(p);
			}
		else
			{
			break;
			}
		}
	}

void CPingEng::RecvDoCancel()
//
// A send operation requires cancelling
//
	{

	iSocket.CancelRecv();
	}

CPingTimer::CPingTimer(CPingEng& aParent)
//
// To time events
//
	: CTimer(EPingTimerPriority), iParent(aParent)
	{
	
	CActiveScheduler::Add(this);
	__DECLARE_NAME(_S("CPingTimer"));
	}

void CPingTimer::RunL()
//
//	Timer is complete
//
	{

	iParent.TimerComplete();
	}

CPingSender::CPingSender(CPingEng& aParent)
//
// C'tor
//
	: CActive(EPingSenderPriority), iParent(aParent)
	{
	
	CActiveScheduler::Add(this);
	__DECLARE_NAME(_S("CPingSender"));
	}

CPingSender::~CPingSender()
//
// D'tor cancels
//
	{

	Cancel();
	}

void CPingSender::RunL()
//
// Upcall to parent
//
	{

	iParent.SendCompleteL();
	}

void CPingSender::DoCancel()
//
// Get parent to cancel send
//
	{
	
	iParent.SendDoCancel();
	}

CPingReceiver::CPingReceiver(CPingEng& aParent)
//
// C'tor
//
	: CActive(EPingReceiverPriority), iParent(aParent)
	{
	
	CActiveScheduler::Add(this);
	__DECLARE_NAME(_S("CPingReceiver"));
	}

CPingReceiver::~CPingReceiver()
//
// D'tor cancels
//
	{

	Cancel();
	}

void CPingReceiver::RunL()
//
// Upcall to parent
//
	{

	iParent.RecvComplete();
	}

void CPingReceiver::DoCancel()
//
// Get parent to cancel send
//
	{
	
	iParent.RecvDoCancel();
	}

TPingRecord::TPingRecord(TUint aSeqNr)
//
// Create new ping record
//
	{
	
	iSeqNr = aSeqNr&KMaxTUint16;
	iSendTime.UniversalTime();
	}

HPingHeader::~HPingHeader()
//
// D'tor deletes 
//
	{
	
	delete iData;
	delete iDataPtr;
	}

HPingHeader* HPingHeader::NewL(TInt aSize, TUint aIPVersion)
//
// Create a new ping header
//
	{
	
	HPingHeader* h = new(ELeave) HPingHeader();

	CleanupStack::PushL(h);
	h->ConstructL(aSize, aIPVersion);
	CleanupStack::Pop(h);	

	return h;
	}

void HPingHeader::ConstructL(TInt aSize, TUint aIPVersion)
	{
	
	iData = HBufC8::NewL(aSize);
	iDataPtr = new(ELeave) TPtr8(iData->Des());
	iSize = aSize < KIcmpHeaderSize ? KIcmpHeaderSize : aSize;
	
	iData->Des().FillZ();	
	SetVersion(aIPVersion);
	}

void HPingHeader::SetVersion(TUint aIPVersion)
//
// To set IP version of packet
// 
	{
	
	iIPVersion = aIPVersion; 
	}

TInt HPingHeader::MaxLength()
//
// To get packet data maximum length
// 
	{
	
	return iData->Des().MaxLength(); 
	}

TInt HPingHeader::DataLength()
//
// To get packet data length
// 
	{
	
	return iData->Des().Length(); 
	}

TPtrC8 HPingHeader::IcmpContents()
//
// To get the icmp contents from packet data
// 
	{
       
	return iData->Des().Mid(4, iData->Length()-4); 
	}

TPtr8* HPingHeader::Grab()
//
// To get data far a send or receive operation
// 
	{

	iDataPtr->Copy(iData->Des());
	return iDataPtr;
	}
	
void HPingHeader::FormatSend(TUint aId, TUint aSeqNum)
//
// Format an ICMP packet to send
//
	{
	
	TUint type;
	TUint code;
	TChecksum sum;

	// Configure version
	if(iIPVersion == KAfInet)
		{
		type = KIPv4PingTypeEchoRequest;
		code = KIPv4PingCodeEcho;
		}
	else
		{
		type = KIPv6PingTypeEchoRequest;
		code = KIPv6PingCodeEcho;
		}

	// Fill header
	SetType(static_cast<TUint8>(type));
	SetCode(static_cast<TUint8>(code));
	SetIdentifier(static_cast<TUint16>(aId));
	SetSequence(static_cast<TUint16>(aSeqNum));

    // Zero checksum
    SetChecksum(0);
    
    // Copy the header to the data descriptor
    iData->Des().Copy(reinterpret_cast<TUint8*>(this), KIcmpHeaderSize);
    
    // Adjust the length for the appropriate ping size
    iData->Des().SetLength(iSize);

    // Fill ping data
    if (iSize > KIcmpHeaderSize)
        {
    	TInt i = 0;
    	TUint8 dataChar = 'a';
        TInt dataLen = iSize - KIcmpHeaderSize;
        TPtr8 dataPtr = iData->Des().MidTPtr(KIcmpHeaderSize, dataLen);

    	for (i = 0; i < dataLen; i++ )
    	    {
    	    dataPtr[i] = dataChar;
    	    if (dataChar == 'z')
    	        {
    	        dataChar = 'a';
    	        }
    	    else
    	        {
    	        dataChar++;
    	        }
    	    }
        }
    
	// Compute checksum
	sum.Add(reinterpret_cast<const TUint16*>(iData->Ptr()), iSize);
	SetChecksum(sum.Sum());
	}

TBool HPingHeader::SetHeader(TUint aOffset)
//
// Set the header from an Icmp reply
// 
	{
	
	const TUint8* buffData; 
	
	// Check size
	if(DataLength() < (TInt)KIcmpHeaderSize)
		{
		return EFalse;
		}
	
	buffData = iData->Des().Ptr();

	if(!buffData)
		{
		return EFalse;
		}
	
	// Fill TInet6HeaderICMP_Echo from the buffer
	for(int k=0;k<(TInt)KIcmpHeaderSize;k++)
		{
		i[k] = *(buffData + k + aOffset);
		}

	return ETrue;
	}

TBool HPingHeader::VerifyRecvEcho(TInt aId)
//
// Verifiy header is valid echo reply
//
	{
	
	TBool ret = ETrue;
	TUint typeCheck;
	TUint codeCheck;

	// Fill TInet6HeaderICMP_Echo from packet data
	ret = SetHeader();
	
	// Look at IP version
	if(iIPVersion == KAfInet)
		{
		typeCheck = KIPv4PingTypeEchoReply;
		codeCheck = KIPv4PingCodeEcho;
		}
	else
		{
		typeCheck = KIPv6PingTypeEchoReply;
		codeCheck = KIPv6PingCodeEcho;
		}
		
	// Wrong packet type or code
	if(ret && (Type() != typeCheck || Code() != codeCheck))
		{
		ret = EFalse;
		}

	// Wrong packet identifier
	if(ret && Identifier() != aId)
		{
		ret = EFalse;
		}

	return ret;
	}

TBool HPingHeader::VerifyNonEcho(TInt aId)
//
// Verify header which is not echo reply
// 
	{
	
	// Fill TInet6HeaderICMP_Echo from packet data
	TBool ret = SetHeader();

	// Look at IP version
	if(ret && iIPVersion == KAfInet)	// IP4
		{
		switch(Type())
			{
		case KIPv4PingTypeUnreachable:
		case KIPv4PingTypeSourceQuench:
		case KIPv4PingTypeRedirect:
		case KIPv4PingTypeTimeExceeded:
		case KIPv4PingTypeBadParameter:
			break;
		default:
			ret = EFalse;
			}
	
		if(ret && (DataLength() < (TInt)KIcmpHeaderSize))
			{
		    	ret = EFalse;
			}

		if(ret)
			{
			ret = SetHeader(KIcmpHeaderSize + KMinIpHeaderSize);
			if(ret && (Type() != KIPv4PingTypeEchoRequest || Identifier() != aId))
				{
				ret = EFalse;
				}
			}
		}
	else
		{				// IP6
		switch(Type())
			{
		case KIPv6PingTypeUnreachable:
		case KIPv6PingTypePacketTooBig:
		case KIPv6PingTypeTimeExeeded:
		case KIPv6PingTypeParamProblem:
			break;
		default:
			ret = EFalse;
			}
		}

	return ret;
	}

