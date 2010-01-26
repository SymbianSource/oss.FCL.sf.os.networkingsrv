// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// engine.cpp - udp packet sender engine
//

#include <es_sock.h>
#include <e32base.h>

#if EPOC_SDK >= 0x06000000
#else
#include <eikdialg.hrh>
#endif

#include "udpsend.h"
#include "engine.h"

#include <in6_opt.h>	// New Socket options, such as Cork

class CReceiverSink;

// Used by CUDPSendEngine
// Sends packets. Cannot be done directly by CUDPSendEngine because there are conflicts with
//					diferent TRequestStatus.

class CPingSingleSender : public CActive
	{
	friend class CReceiverSink;
public:
	//constructor
	CPingSingleSender();

	//destructor
	~CPingSingleSender();
	void ConstructL(CUDPSendEngine *aUDPSendEngine);
	TBool StartL(const TDes& aHostname);
	void NextPacket();
	void SendPacket();
	void Error(const TDesC& aDes,TInt aError=KErrNone);
	void ErrorL(const TDesC& aDes,TInt aError);
	inline TUint PacketsSent()
		{return iPacketsSent;}
	void FloodMode(TUint aCount);
protected:
	// will send all the packets
	void RunL();

	//Cancel Packet Sending
	void DoCancel();

private:
	void ComposeUDPPacket();

private:
	CUDPSendEngine *iUDPSendEngine;
	CUDPSendView *iAppView;

	RSocketServ iSocketServer;
	RSocket		iSocket;
	HBufC8* iSentDataBuffer;
	TPtr8 iSentData;
	TUint iUnsent;
	TUint iPacketsSent;
	CReceiverSink *iSink;
	TBool iFlooding;
	// Send statistics
	TUint32 iTotalBytes;
	TTime iStartTime;
	};

class CReceiverSink : public CActive
	// CReveiverSink keeps receiver active on the socket and
	// consumes all incoming data.
	{
public:
	CReceiverSink(CPingSingleSender &aSender);
	~CReceiverSink();
protected:
	void RunL();
	void DoCancel();
private:
	CPingSingleSender &iSender;
	TBuf8<1000> iBuf;
	};

// Sends packets with an active Timer
// Created by CPing
// Not intended to run alone

CUDPSendEngine::CUDPSendEngine():CTimer(EPriorityStandard)//,iSentData(0,0)
	{
	CActiveScheduler::Add(this);	//Adds itself to the scheduler only the first time
	}

CUDPSendEngine::~CUDPSendEngine()
	{
	if (IsActive())
		Cancel();

	delete iSender;
	}


void CUDPSendEngine::ConstructL(CUDPSendView *aAppView)
	{
	//Base class 2nd phase constructor
	CTimer::ConstructL();
	iAppView=aAppView;

	//Default values
	iDestPort=7;
	iTotalPackets=5;
	iDelay=SECOND/MILISECOND;		//in ms.
	iPacketSize=56;
	iSendSynch=0;
	iPriority=0;
	iFlowLabel=0;
	iHostname=_L("127.0.0.1");
	iTcpLinger = -1;	// Linger disabled by default
	iTcpNagling = 1;	// Conventional Nagle, which is default for TCP connections
	
	}


void CUDPSendEngine::StartL()
	{	
	iSender=new (ELeave) CPingSingleSender;
	iSender->ConstructL(this);
	if (SendFirstPacketL())
		{
		if (iDelay)
			IssueRequest();	//First RunL
		iRunning=ETrue;
		}
	else
		Stop();
	}


void CUDPSendEngine::Stop()
	{
	iRunning=EFalse;
	if (IsActive())
		Cancel();

	delete iSender;
	iSender=NULL;
	}

//Issues next RunL execution
void CUDPSendEngine::IssueRequest()
	{
	After(MILISECOND * iDelay);	//Also sets the object as Active
	}




// will send all the packets. One packet each Time
void CUDPSendEngine::RunL()
	{
	ASSERT(iSender != NULL);

	if (iSender->PacketsSent() < iTotalPackets)
		{
		SendPacket();
		IssueRequest();
		}
	}


//Creates a AO that sends a packets waits for it to be send and dies
TBool CUDPSendEngine::SendFirstPacketL()
	{
	if (LaunchHostnameDialogL())
		{
		ASSERT(iSender != NULL);
		if (iDelay == 0)
			iSender->FloodMode(iTotalPackets-1);
		return iSender->StartL(iHostname);
		}
	else
		return EFalse;
	}


//Creates a AO that sends a packets waits for it to be send and dies
void CUDPSendEngine::SendPacket()
	{	
	ASSERT(iSender != NULL);
	iSender->NextPacket();	
	}

//Cancel Packet Sending
void CUDPSendEngine::DoCancel()
	{
	CTimer::DoCancel();
	iRunning=EFalse;
	}


// Launches a dialog to ask for new options

TBool CUDPSendEngine::LaunchHostnameDialogL()
	{
	CHostNameDialog *dialog=new (ELeave) CHostNameDialog(this);
	TInt button=dialog->ExecuteLD(R_UDPSEND_HOSTNAME);	//Final D means the dialog is destructed by itself
	return (button==EEikBidOk);	// If button is CANCEL then the algorithm is not added
	}



//
//	CUDPSender
//


// Used by CUDPSendEngine
// Sends packets. Cannot be done directly by CUDPSendEngine because there are conflicts with
//					diferent TRequestStatus.

CPingSingleSender::CPingSingleSender():CActive(EPriorityStandard),iSentData(0,0)
	{
	CActiveScheduler::Add(this);	//Adds itself to the scheduler only the first time
	}

CPingSingleSender::~CPingSingleSender()
	{
	TTime stamp;
	stamp.UniversalTime();
	TTimeIntervalMicroSeconds time_used = stamp.MicroSecondsFrom(iStartTime);
	TBuf<75> aux;
	aux.Format(_L("Sent %d bytes/s, total=%d bytes\n"),
		(TInt)((1000000.0 * iTotalBytes) / time_used.Int64()), iTotalBytes);
	iAppView->Write(aux);

	delete iSink;
	iSink = 0;
	if (IsActive())
		Cancel();

	iAppView->Write(_L("Close pending...\n"));
	iSocket.Close();
	iAppView->Write(_L("Close finished\n"));
	iSocketServer.Close();
	delete iSentDataBuffer;
	}

void CPingSingleSender::ConstructL(CUDPSendEngine *aUDPSendEngine)
	{
	iUDPSendEngine=aUDPSendEngine;
	iAppView=aUDPSendEngine->AppView();

	TInt err=iSocketServer.Connect();	//KESockDefaultMessageSlots
	if (err!=KErrNone)
		{
		ErrorL(_L("Socket Server Error (Connect)"),err);
		return;
		}
	}


void CPingSingleSender::ComposeUDPPacket()
	{
	if (iUDPSendEngine->iPacketSize > 0)
		{
		iSentData[0] = (TUint8)iPacketsSent;	//used as sequence number
		}
	}

void CPingSingleSender::FloodMode(TUint aCount)
	{
	iFlooding = TRUE;
	iUnsent = aCount;
	}

TBool CPingSingleSender::StartL(const TDes& aHostname)
	{	
	TRequestStatus status;	//locally used for connect
	TInt err=KErrNone;
	TBuf<75> aux;
	TBuf<5> protocol;
	TInetAddr addr;

	switch (iUDPSendEngine->iProtocol)
		{
	case UDP:
		protocol.Format(_L("udp"));
		break;
	case TCP:
		protocol.Format(_L("tcp"));
		break;
	default:
		Error(_L("Bad Protocol type"));
		return EFalse;
		}

	err=addr.Input(aHostname);
	if (err!=KErrNone)
		{
		// If not numerical IP address, we do a cheap and ugly name resolver tryout.
		RHostResolver resolver;
		err = resolver.Open(iSocketServer, KAfInet, KProtocolInetIcmp);
		if (err != KErrNone)
			{
			Error(_L("Name resolver open failed"));
			return EFalse;
			}
			
		TNameEntry entry;
		err = resolver.GetByName(aHostname, entry);
		if (err != KErrNone)
			{
			Error(_L("Wrong Address"));
			return EFalse;
			}
			
		addr = TInetAddr::Cast(entry().iAddr);
		resolver.Close();
		}	
	
	if (addr.Family() != KAfInet && addr.Family() != KAfInet6)
		{
		Error(_L("Wrong Address Family"));
		return EFalse;
		}

	err=iSocket.Open(iSocketServer,protocol);
	protocol.UpperCase();
	aux.Format(_L("Sending %S packets\n"), &protocol);
	iAppView->Write(aux);
	if (err!=KErrNone)
		{
		Error(_L("Socket (Open)"),err);
		return EFalse;
		}
	
	switch(iUDPSendEngine->iTcpNagling)
		{
	case 0:	// No delay
		err = iSocket.SetOpt(KSoTcpNoDelay, KSolInetTcp, 1);
		break;
	case 1:	// Nagle (default)
		err = KErrNone;
		break;
	case 2:	// Cork
		err = iSocket.SetOpt(KSoTcpCork, KSolInetTcp, 1);
		break;
	default:
		err = KErrGeneral;	// :-)
		}
	
	if (err != KErrNone)
		{
		Error(_L("SetOpt (Nagling)"), err);
		return EFalse;
		}

	if (iUDPSendEngine->iPriority)
		{
		err = iSocket.SetOpt(KSoIpTOS, KSolInetIp, iUDPSendEngine->iPriority);
		if (err != KErrNone)
			{
			Error(_L("SetOpt KSoIpTOS"), err);
			return EFalse;
			}
		}
	if (iUDPSendEngine->iProtocol == UDP)
		{
		err = iSocket.SetOpt(KSoUdpSynchronousSend, KSolInetUdp, iUDPSendEngine->iSendSynch);
		if (err != KErrNone)
			{
			Error(_L("SetOpt SynchSend"), err);
			return EFalse;
			}
		}
	if (iUDPSendEngine->iTcpLinger != -1)
		{
		TPckgBuf<TSoTcpLingerOpt> opt;
		opt().iOnOff = 1;
		opt().iLinger = iUDPSendEngine->iTcpLinger;
		err = iSocket.SetOpt(KSoTcpLinger, KSolInetTcp, opt);
		if (err != KErrNone)
			{
			Error(_L("SetOpt (Linger)"), err);
			return EFalse;
			}
		}
	
	addr.SetPort(iUDPSendEngine->iDestPort);
	addr.SetFlowLabel(iUDPSendEngine->iFlowLabel);
	iSocket.Connect(addr,status);
	User::WaitForRequest(status);
	if (status!=KErrNone)
		{
		Error(_L("Socket Error (Connect)"),status.Int());
		return EFalse;
		}

	delete iSentDataBuffer;
	iSentDataBuffer = HBufC8::NewL(iUDPSendEngine->iPacketSize);
	TPtr8 auxPtr(iSentDataBuffer->Des());
	iSentData.Set(auxPtr);
	iSentData.FillZ(iUDPSendEngine->iPacketSize);

	iPacketsSent=0;
	
	iSink = new CReceiverSink(*this);

	aux.Format(_L("Sending %u * %u bytes to port=%u of %S\n"),
		iUDPSendEngine->iTotalPackets, iSentData.Length(), iUDPSendEngine->iDestPort, &aHostname);
	iAppView->Write(aux);
	if (iFlooding)
		iAppView->Write(_L("Flooding mode -- only total reported at end\n"));
	iTotalBytes = 0;
	iStartTime.UniversalTime();
	SendPacket();
	return ETrue;	//No errors
	}


void CPingSingleSender::SendPacket()
	{
	ComposeUDPPacket();
	iSocket.Write(iSentData,iStatus);
	iPacketsSent++;
	// Don't output per packet display when flooding!
	if (!iFlooding)
		{
		TBuf<75> aux;
		aux.Format(_L("Packet %d Sent (%d delayed)!\n"), iPacketsSent, iUnsent);
		iAppView->Write(aux);
		}
	SetActive();
	}

//Cancel Packet Sending
void CPingSingleSender::DoCancel()
	{
	iSocket.CancelAll();
	}

void CPingSingleSender::NextPacket()
	{
	if (iPacketsSent + iUnsent < iUDPSendEngine->iTotalPackets)
		{
		if (IsActive())		//Still a packet being sent
			iUnsent++;		//Send it when socket becomes ready.
		else
			SendPacket();
		}
	}

// will send all the packets. One packet each Time
void CPingSingleSender::RunL()
	{
	if (iStatus!=KErrNone)
		{
		Error(_L("Write"),iStatus.Int());
		}
	iTotalBytes += iSentData.Length();	
	if (iUnsent)
		{
		iUnsent--;
		SendPacket();
		}
	else if (iPacketsSent >= iUDPSendEngine->iTotalPackets)
		iUDPSendEngine->Stop();	// KILLS this and everything!
	}


void CPingSingleSender::Error(const TDesC& aText,TInt aError)
	{
	TBuf<150> aux;
	TBuf<100> errtxt;

	aux.Format(aText);
	if (aError!=KErrNone)
		{
		aux.AppendFormat(_L(": "));
		CEikonEnv::Static()->GetErrorText(errtxt,aError);
		aux.AppendFormat(errtxt);
		}
	aux.AppendFormat(_L("\n"));
	iAppView->Write(aux);

	if (IsActive())
		Cancel();
	}

void CPingSingleSender::ErrorL(const TDesC& aText,TInt aError)
	{
	Error(aText,aError);
	User::Leave(0);	//The way to have errors in Active Objects
	}


// Sink

CReceiverSink::CReceiverSink(CPingSingleSender &aSender) : CActive(EPriorityStandard+1), iSender(aSender)
	{
	CActiveScheduler::Add(this);
	iSender.iSocket.Recv(iBuf, 0, iStatus);
	SetActive();
	}
	
CReceiverSink::~CReceiverSink()
	{
	Cancel();
	}

void CReceiverSink::RunL()
	{
	TBuf<60> msg;
	msg.Format(_L("Received %d (of max %d) bytes with iStatus=%d\n"), iBuf.Length(), iBuf.MaxLength(), iStatus.Int());
	iSender.iAppView->Write(msg);
	if (iStatus.Int() == KErrNone)
		{
		iSender.iSocket.Recv(iBuf, 0, iStatus);
		SetActive();
		}
	}

void CReceiverSink::DoCancel()
	{
	iSender.iSocket.CancelRecv();
	}
