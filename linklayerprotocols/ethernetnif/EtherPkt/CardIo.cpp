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
//

/**
 @file
*/

#include <nifman.h>
#include <nifvar.h>
#include <nifutl.h>
#include <es_mbuf.h>
#include "PKTDRV.H"
#include "ETHINTER.H"
#include "Cardctl.h"

//#define __DebugCardLo__ 1
#if defined(__DebugCardLo__)
#include <e32svr.h>
 #define DPRINT(x) RDebug::Print x
 #pragma message ("Warning: this code contains RDebug::Print statements!  Do not submit to the mainline with __DebugCardCtl__ defined.")
#else
 #define DPRINT(x)
#endif
 
#ifdef __DebugCardLo__
// TCP packet tracing debug
const TUint8 ETHER2_TYPE_IP_MSB = 0x08;
const TUint8 ETHER2_TYPE_IP_LSB = 0x00;
const TUint8 IP_TYPE_TCP        = 0x06;
static inline TBool IsTcp(TDesC8 &aFrame)
{
	return (aFrame[12] == ETHER2_TYPE_IP_MSB && aFrame[13] == ETHER2_TYPE_IP_LSB && aFrame[23] == IP_TYPE_TCP);
}
static TInt GetTcpSeqNumber(TDesC8 &aFrame)
	{
	TInt seqNum = 0;
	if (IsTcp(aFrame))
		seqNum = aFrame[38] << 24 | aFrame[39] << 16 | aFrame[40] << 8| aFrame[41];
	return seqNum;
	}
static TInt GetTcpAckNumber(TDesC8 &aFrame)
	{
	TInt ackNum = 0;
	if (IsTcp(aFrame))
		ackNum = aFrame[42] << 24 | aFrame[43] << 16 | aFrame[44] << 8| aFrame[45];
	return ackNum;
	}
#endif


/**
Send active object class
When CIOBuffer's are passed to SendL() the class takes ownership and is
therefore resposible for freeing them in the RunL()
@internalComponent
*/
const TInt KTxQueueThreshold = 40;

/**
Constructor.
*/
CPcCardSender::CPcCardSender() : CActive(EPriorityStandard)
{
	
}

/**
Destructor.
Could be buffers on the transmit queue, free them as this class should be sole owner.
*/
CPcCardSender::~CPcCardSender()
{
	EmptyQueue();
	Cancel();
}

/**
Standard CActive construction.
@param aParent Pointer to the parent CPcCardControlEngine class.
@return A pointer to CPcCardSender object.
*/
CPcCardSender* CPcCardSender::NewL(CPcCardControlEngine* aParent)
{
	CPcCardSender *sd=new (ELeave) CPcCardSender;
	CleanupStack::PushL(sd);
	sd->InitL(aParent);
	CActiveScheduler::Add(sd);
	CleanupStack::Pop();
	return sd;
}

/**
Add the newly created object to an object container.
@param aParent Pointer to the parent CPcCardControlEngine class.
*/
void CPcCardSender::InitL(CPcCardControlEngine* aParent)
{
	iParent=aParent;
	iTxQueue.SetOffset(CIOBuffer::LinkOffset());
	iQueueLength = 0;
	iStopSending = EFalse;
}

/** 
Protocol inspects return
It blocks sending if it receives a return <= 0
This value should be propogated up through the stack
@internalComponent
*/
const TInt KStopSending		= 0;

/**
Protocol inspects return to indicate Keep sending the data.
@internalComponent
*/
const TInt KContinueSending	= 1;

/**
Writes the data to buffer.

@param aBuffer The data to be send.
@return 0 Tells the higher layer to stop sending the data.
		1 Tells higher layer that it can continue sending more data.
*/
TInt CPcCardSender::Send(CIOBuffer *aBuffer)
{
	DPRINT((_L(">pkt tx: requested write - tcp seq=%u ack=%u iQueueLength=%d threadId=%d"), 
			GetTcpSeqNumber(aBuffer->Ptr()), GetTcpAckNumber(aBuffer->Ptr()), iQueueLength, (TInt)RThread().Id() ));
	
	// Check to see if we need to start transmission
	// Pseudo interrupt queue
	TBool startTx = iTxQueue.IsEmpty();

	iTxQueue.AddLast(*aBuffer);
	iQueueLength++;
	if(startTx)
		{
		// Transmitter was idle start next transmit
		DPRINT((_L(">pkt tx: PRE write - tcp seq=%u ack=%u iQueueLength=%d threadId=%d"), 
				GetTcpSeqNumber(aBuffer->Ptr()), GetTcpAckNumber(aBuffer->Ptr()), iQueueLength, (TInt)RThread().Id() ));
		iParent->iCard.Write(iStatus,aBuffer->Ptr());
		SetActive();
		//DPRINT((_L(">pkt tx: PST write - tcp seq=%u ack=%u iQueueLength=%d threadId=%d"), GetTcpSeqNumber(aBuffer->Ptr()), GetTcpAckNumber(aBuffer->Ptr()), iQueueLength, RThread().Id() ));
		}
	else
	{
		DPRINT((_L(">pkt tx: PST skipped write queuing (leave for runl) - tcp seq=%u ack=%u iQueueLength=%d threadId=%d"), 
				GetTcpSeqNumber(aBuffer->Ptr()), GetTcpAckNumber(aBuffer->Ptr()), iQueueLength, (TInt)RThread().Id() ));
	}	
	// The stack could saturate us with data
	// Tell the stack to send no more
	// We will unblock the stack when the queue size drops below
	// the the threshold
	if(iQueueLength >= KTxQueueThreshold)
		{
		iStopSending = ETrue;
		return KStopSending;
		}
	else
		{
		return KContinueSending;
		}
}

/**
Free all queued buffers. Should be safe as this class owns them
*/
void CPcCardSender::EmptyQueue()
{
	DPRINT((_L(">pkt tx: emptying driver...")));	
	while(!iTxQueue.IsEmpty())
		{
		CIOBuffer* buf = iTxQueue.First();
		iTxQueue.Remove(*buf);
		delete buf;
		}
	iQueueLength = 0;
	iStopSending = EFalse;
}

/**
Write completion from the LDD. Pseudo transmit interrupt handler
*/
void CPcCardSender::RunL()
{
	DPRINT((_L(">pkt tx runl: completion - tcp seq=%u ack=%u iQueueLength=%d threadId=%d"), 
			GetTcpSeqNumber(iTxQueue.First()->Ptr()), GetTcpAckNumber(iTxQueue.First()->Ptr()), iQueueLength, (TInt)RThread().Id() ));
	// Check for error, all we can do is free the buffers
	if(iStatus.Int()!=KErrNone)
		{
		DPRINT((_L(">pkt tx runl error... emptying queue: - tcp seq=%u ack=%u iQueueLength=%d threadId=%d"), 
				GetTcpSeqNumber(iTxQueue.First()->Ptr()), GetTcpAckNumber(iTxQueue.First()->Ptr()), iQueueLength, (TInt)RThread().Id() ));
		EmptyQueue();
		return;
		}

	if(!iTxQueue.IsEmpty())
		{
		// Head of the queue has been transmitted
		// Remove it and free it
		CIOBuffer* buf = iTxQueue.First();
		iTxQueue.Remove(*buf);
		iQueueLength--;
		delete buf;
		

		// Check to see if there are still buffers queued.
		// Start next transmit if there are
		TBool startTx;
		(iTxQueue.IsEmpty()) ? (startTx = EFalse) : (startTx = ETrue);
		if(startTx)
			{
			buf = iTxQueue.First();
			DPRINT((_L(">pkt tx runl PRE write...: - tcp seq=%u ack=%u iQueueLength=%d threadId=%d"), 
					GetTcpSeqNumber(buf->Ptr()), GetTcpAckNumber(buf->Ptr()), iQueueLength, (TInt)RThread().Id() ));
			iParent->iCard.Write(iStatus,buf->Ptr());
			SetActive();
			}
		else
		{
			DPRINT((_L(">pkt tx runl skipped writing (queue empty)...:")));
		}
		// Resume sending if the protocol was previously blocked
		if(iStopSending && iQueueLength < KTxQueueThreshold)
			{
			DPRINT((_L(">pkt tx runl resume sending notification...: - tcp seq=%u ack=%u iQueueLength=%d threadId=%d"), 
					GetTcpSeqNumber(iTxQueue.First()->Ptr()), GetTcpAckNumber(iTxQueue.First()->Ptr()), iQueueLength, (TInt)RThread().Id() ));

			iStopSending = EFalse;
			iParent->ResumeSending();
			}
		}
}

/**
cancellation of an outstanding request.
*/
void CPcCardSender::DoCancel()
{
	iParent->iCard.WriteCancel();
}

/**
Read active object class.
Read kept permanently on the LDD
Read completion is notified immediately up through the stack
with the one receive buffer therefore no Q.
*/
CPcCardReceiver::CPcCardReceiver() : CActive(EPriorityMore)  , iRecvBufPtr(NULL,0) 
{
	
}

/**
Constructor.
*/
CPcCardReceiver::~CPcCardReceiver()
{
	Cancel();
	// One buffer only
	delete iRecvBuffer;
}

/**
Standard CActive construction.
@param aParent Pointer to the parent CPcCardControlEngine class.
@return A pointer to CPcCardReceiver object.
*/
CPcCardReceiver* CPcCardReceiver::NewL(CPcCardControlEngine* aParent)
{
	CPcCardReceiver *rv=new (ELeave) CPcCardReceiver;
	CleanupStack::PushL(rv);
	rv->InitL(aParent);
	CActiveScheduler::Add(rv);
	CleanupStack::Pop();
	return rv;
}

/**
Allocate the one and only read buffer.
@param aParent Pointer to the parent CPcCardControlEngine class.
*/
void CPcCardReceiver::InitL(CPcCardControlEngine* aParent)
{
	iParent=aParent;
	iRecvBufLength=KEtherBufSize;
	iRecvBuffer=HBufC8::NewMaxL(iRecvBufLength);
	TPtr8 temp=iRecvBuffer->Des();
	iRecvBufPtr.Set(temp);
}

/**
Pass the receive buffer to the Card.
*/
void CPcCardReceiver::QueueRead()
{
	iRecvBufPtr.SetMax();
	DPRINT((_L(">pkt queuing read...")));
	iParent->iCard.Read(iStatus,iRecvBufPtr);
	SetActive();
}

/**
Pseudo read interrupt handler.
*/
void CPcCardReceiver::RunL()
{
	if (iParent->CardOpen())
		{
		if (iStatus.Int()!=KErrNone)
			{
			DPRINT((_L(">pkt tx runl: bad status=%d tcp seq=%u ack=%u threadId=%d"), 
					iStatus.Int(), GetTcpSeqNumber(iRecvBufPtr), GetTcpAckNumber(iRecvBufPtr), (TInt)RThread().Id() ));
			QueueRead();
			return;
			}
		// Pass the buffer up the stack
		// and queue the next read, safe to reuse the buffer.
		if(iRecvBufPtr.Length())
			{
			DPRINT((_L(">pkt rx runl: tcp seq=%u ack=%u threadId=%d"), 
					GetTcpSeqNumber(iRecvBufPtr), GetTcpAckNumber(iRecvBufPtr), (TInt)RThread().Id() ));
			iParent->ProcessReceivedPacket(iRecvBufPtr);
			}
		QueueRead();
		}
	else
	{
		DPRINT((_L(">CPcCardReceiver::RunL Card NOT open")));
	}
}

/**
Cancellation of an outstanding request.
*/
void CPcCardReceiver::DoCancel()
{
	iParent->iCard.ReadCancel();
}

/**
Constructor.
*/
CPcCardEventHandler::CPcCardEventHandler() : CActive(EPriorityStandard) 
{
	
}

/**
Destructor.
*/
CPcCardEventHandler::~CPcCardEventHandler()
{
	Cancel();
}

/**
Allocate the one and only read buffer.
@param aParent Pointer to the parent CPcCardControlEngine class.
*/
void CPcCardEventHandler::InitL(CPcCardControlEngine* aParent)
{
	iParent = aParent;
}

/**
Standard CActive construction
@param aParent Pointer to the parent CPcCardControlEngine class.
@return A pointer to the CPcCardEventHandler object.
*/
CPcCardEventHandler* CPcCardEventHandler::NewL(CPcCardControlEngine* aParent)
{
	CPcCardEventHandler *p=new (ELeave) CPcCardEventHandler;
	CleanupStack::PushL(p);
	p->InitL(aParent);
	CActiveScheduler::Add(p);
	CleanupStack::Pop();
	return p;
}

/**
Handles an active object’s request completion event.
*/
void CPcCardEventHandler::RunL()
{
	// TODO Parse code in iStatus for type of event
}

/**
Cancellation of an outstanding request.
*/
void CPcCardEventHandler::DoCancel()
{
}

/**
Gets the Event generated by the device drivers.
*/
void CPcCardEventHandler::GetEvent()
{
	// Tell the device driver we want ALL Events
	iEventBuffer.SetLength(1);
	iEventBuffer[0] = 0xFF;
	SetActive();
}

/**
Constructor.
*/
CPcCardIOCTL::CPcCardIOCTL() : CActive(EPriorityStandard) 
{
	
}

/**
Destructor.
*/
CPcCardIOCTL::~CPcCardIOCTL()
{
	Cancel();
}

/**
Add the newly created object to an object container.
@param aParent Pointer to the parent CPcCardControlEngine class.
*/
void CPcCardIOCTL::InitL(CPcCardControlEngine* aParent)
{
	iParent = aParent;
}

TInt CPcCardIOCTL::Ioctl(const TUint8 aIOCTLCode)
{
	if(IsActive())
		return KErrNotReady;
	iIOCTLBuffer.SetLength(1);
	iIOCTLBuffer[0] = aIOCTLCode;
	iCurrentIOCTL = aIOCTLCode;
	SetActive();
	return KErrNone;
}


/**
Standard CActive construction.
@param aParent Pointer to the parent CPcCardControlEngine class.
@return A pointer to CPcCardIOCTL object.
*/
CPcCardIOCTL* CPcCardIOCTL::NewL(CPcCardControlEngine* aParent)
{
	CPcCardIOCTL *p=new (ELeave) CPcCardIOCTL;
	CleanupStack::PushL(p);
	p->InitL(aParent);
	CActiveScheduler::Add(p);
	CleanupStack::Pop();
	return p;
}

/**
Handles an active object’s request completion event.
*/
void CPcCardIOCTL::RunL()
{
			{
			iParent->iReceiver->QueueRead();
			iParent->LinkLayerUp();
			}
}

/**
Cancellation of an outstanding request.
*/
void CPcCardIOCTL::DoCancel()
{
}
