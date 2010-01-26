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
// IrLAN data send/receive active objects 
// 
//

/**
 @file
*/

#include <nifman.h>
#include <nifvar.h>
#include <nifutl.h>
#include <es_mbuf.h>
#include "PKTDRV.H"
#include "IRLAN.H"
#include "IRLANUTL.H"
#include "IRLANBUF.H"
#include "INTSOCK.H"

//#define __TRACEWIN__
#ifdef __TRACEWIN__
  #include <log.h>
#else
  #define LOG(a)
#endif

//############################ IrLAN Sender ############################

/**
Constructor.
*/
CIrlanSender::CIrlanSender() : CActive(EIrlanSenderPriority)
{
	__DECLARE_NAME(_S("CIrlanSender"));
}

/**
Destructor.
*/
CIrlanSender::~CIrlanSender()
{
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN	~CIrlanSender\r\n")));
#endif
}

/**
Create a new CIrlanSender object.
@param aParent A pointer to CIrlanControlEngine object.
@param aSock A pointer to CInternalSocket object.
@return A pointer to CIrlanSender class.
*/
CIrlanSender* CIrlanSender::NewL(CIrlanControlEngine* aParent,CInternalSocket* aSock)
{
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN	CIrlanSender::NewL\r\n")));
#endif
	CIrlanSender *sd=new (ELeave) CIrlanSender;
	CleanupStack::PushL(sd);
	sd->InitL(aParent,aSock);
	CActiveScheduler::Add(sd);
	CleanupStack::Pop();
	return sd;
}

/**
Add the newly created object to an object container
@param aParent A pointer to CIrlanControlEngine object.
@param aSock A pointer to CInternalSocket object.
*/
void CIrlanSender::InitL(CIrlanControlEngine* aParent,CInternalSocket* aSock)
{
	iParent=aParent;
	iDataSock=aSock;
}

/**
Waits to read the data from the Queue.
@param aPdu A reference to the packet to be sent (really an RMBufPkt)
*/
void CIrlanSender::QueueSend(RMBufChain& aPdu)
{
	if (iDataSock->Write(aPdu,NULL,iStatus,0)!=static_cast<TUint>(KErrNone))
		IrlanUtil::Panic(EIrlanDataWrite);
	SetActive();
}

/**
Initiate sending for tha data.
*/
void CIrlanSender::KickSender()
{
	if (IsActive())
		return;
	else
		SeeIfPacketToSend();
}

/**
Check in Queue whether the packet to send.
*/
void CIrlanSender::SeeIfPacketToSend()
{
	RMBufChain pkt;
	if (iParent->iDataSendQ.Remove(pkt))
		{
		QueueSend(pkt);
		pkt.Free();
		}
}

/**
Handles an active object’s request completion event.
*/
void CIrlanSender::RunL()
{
	TInt ret=iStatus.Int();
	if (ret!=KErrNone)
		{
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN	Data send failed with err=%d\r\n"),ret));
#endif
		iParent->HandleErrorL();
		return;
		}
	SeeIfPacketToSend();
}

/**
cancellation of an outstanding request.
*/
void CIrlanSender::DoCancel()
{
	iDataSock->CancelSend();
}

//############################ IrLAN Receiver ############################

/**
Constructor.
*/
CIrlanReceiver::CIrlanReceiver() : CActive(EIrlanReceiverPriority)
{
	__DECLARE_NAME(_S("CIrlanReceiver"));
}

/**
Destructor.
*/
CIrlanReceiver::~CIrlanReceiver()
{
	DoCancel();
}

/**
Create a new CIrlanReceiver object.
@param aParent A pointer to CIrlanControlEngine object.
@param aSock A pointer to CInternalSocket object.
@return A pointer to CIrlanReceiver class.
*/
CIrlanReceiver* CIrlanReceiver::NewL(CIrlanControlEngine* aParent,CInternalSocket* aSock)
{
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN	CIrlanReceiver::NewL\r\n")));
#endif
	CIrlanReceiver *rv=new (ELeave) CIrlanReceiver;
	CleanupStack::PushL(rv);
	rv->InitL(aParent,aSock);
	CActiveScheduler::Add(rv);
	CleanupStack::Pop();
	return rv;
}

/**
Add the newly created object to an object container

@param aParent A pointer to CIrlanControlEngine object.
@param aSock A pointer to CInternalSocket object.
*/
void CIrlanReceiver::InitL(CIrlanControlEngine* aParent,CInternalSocket* aSock)
{
	iParent=aParent;
	iDataSock=aSock;
}

/**
Waits to read the data from the Queue.
*/
void CIrlanReceiver::QueueRead()
{
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN	QUEUEING WAIT FOR DATA\r\n")));
#endif

	iParent->iRecvBufPtr.SetLength(iParent->iRecvBufLength);
	TInt ret;
	ret = iDataSock->Recv(iParent->iRecvBufPtr,NULL,iStatus,0);
	if (ret!=KErrNone)
		{// what do we do here - panic?
		IrlanUtil::Panic(EIrlanDataRead);
		}
	SetActive();
}

/**
Handles an active object’s request completion event.
*/
void CIrlanReceiver::RunL()
{
	TInt ret=iStatus.Int();
	if (ret!=KErrNone)
		{
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN	Data receive failed with err=%d\r\n"),ret));
#endif
		iParent->HandleErrorL();
		return;
		}
	iParent->ProcessReceivedPacketL();
	QueueRead();
}

/**
cancellation of an outstanding request.
*/
void CIrlanReceiver::DoCancel()
{
	iDataSock->CancelRecv();
}
