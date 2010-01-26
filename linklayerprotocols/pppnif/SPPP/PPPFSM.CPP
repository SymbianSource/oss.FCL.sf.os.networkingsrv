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


#include "PPPLCP.H"
#include "PPPBASE.H"
#include "PPPLOG.H"
#include "ncpip.h"
#include "PPPCCP.H" // for KPppIdCcp
#include <es_ini.h>

//
// PPP State machine
//

MPppFsm::MPppFsm(CPppLcp* aPppLcp, TPppPhase aPhase, TUint aPppId)
	: MPppRecvr(aPppLcp, aPhase, aPppId),	
	// Values may be initialized via  .ini file, but unless specified otherwise:
	iTerminateRequestEnabled(ETrue), // RFC-compliant termination phase (TerminateRequest) is enabled
	iTerminateAckEnabled(ETrue),     // RFC-compliant termination phase (TerminateAck) is enabled
	iFsmTerminationCauseError(KErrNone),
	iMaxTerminateRequest(KPppFsmTerminateRequestRetries),      // Wait after Sending TerminateRequest
	iTerminateRequestTimeout(KPppFsmTerminateRequestTimeout),  // Wait for TerminateAck after sending TerminateRequest	
	iTerminateAckTimeout(KPppFsmTerminateAckTimeout),           // Wait after sending TerminateAck
	iFsmTermination(EFalse),
	iNoEvidenceOfPeer(EFalse),
	iWaitTimeNoIncrease(EFalse)
	{
	}

MPppFsm::~MPppFsm()
	{
	TimerDelete();
	if (!iRequestList.IsEmpty())
		iRequestList.Free();
	}

void MPppFsm::FsmConstructL()
/**
Construct the state machine object
*/
	{
	ReadIniFileL();
	// Dump the configuration resulting from the .ini file:
	
	LOG(iPppLcp->iLogger->Printf(_L("%s FSM: TerminateRequestEnabled[%d], MaxTerminateRequest[%d], TerminateRequestTimeout[%d]"),\
		__iFsmName,	
		iTerminateRequestEnabled,
		iMaxTerminateRequest,
		iTerminateRequestTimeout);)
	
	LOG(iPppLcp->iLogger->Printf(_L("%s FSM: TerminateAckEnabled[%d], TerminateAckTimeout[%d]"),\
		__iFsmName,
		iTerminateAckEnabled,
		iTerminateAckTimeout);)
	
	TimerConstructL(KPppFsmTimerPriority);
	}

void MPppFsm::TerminateLink()
/**
Tear down the protocol, regardless of the state of the FSM.
*/
	{
	// Support for Termination Phase.
	// This is not according to the RFC, but it is done to avoid 
	// any regressions in the existing PPP.	
	switch(iState) 
		{		
		case EPppFsmStopping: // We have received TerminateRequest, sent TerminateAck and
							  // are waiting for one timeout to expire.
			break;	// Stay in EPppFsmStopping state            
		default: 
			SendInitialTerminateRequest();
			SetState(EPppFsmClosing);
		}			
	}

//
// Open/Close calls from higher level protocols
//

TInt MPppFsm::FsmOpen()
/**
Begin the state machine and protocol.
Called by the derived class during initialization.

@return KErrNone if successful, otherwise one of the system-wide error codes
*/
	{
	LOG( iPppLcp->iLogger->Printf(_L("%s FSM Open Request"), __iFsmName); )
	TInt err = KErrNone;
	
	InitMaxFailure();
	
	switch (iState)
		{
	case EPppFsmInitial:
		err = FsmLayerStarted();
		SetState(EPppFsmStarting);
		break;
	case EPppFsmStarting:
		break;
	case EPppFsmClosed:
		err = FsmLayerStarted();
		SendInitialConfigRequest();
		SetState(EPppFsmReqSent);
		break;
	case EPppFsmStopped:
	case EPppFsmClosing:
	case EPppFsmStopping:
	case EPppFsmOpened:
		// Check that we *really* want to do this!
		LowerLayerDown();
		LowerLayerUp();
		break;
	case EPppFsmReqSent:
	case EPppFsmAckRecvd:
	case EPppFsmAckSent:
		break;
		}
	iPppAbortCode = KErrNone;
	return err;
	}

void MPppFsm::FsmClose(TInt aReason)
/**
Close the state machine and protocol.
Called by the derived class on closing.

@param aReason Reason for closing
*/
	{
	LOG( iPppLcp->iLogger->Printf(_L("%s FSM Close Request, error[%d]"), __iFsmName, aReason); )
	switch (iState)
		{
	case EPppFsmInitial:
	case EPppFsmClosed:
	case EPppFsmClosing:
		break;
	case EPppFsmStarting:
		FsmLayerFinished();
		SetState(EPppFsmInitial);
		break;
	case EPppFsmStopped:
		SetState(EPppFsmClosed);
		break;
	case EPppFsmStopping:
		SetState(EPppFsmClosing);
		break;
	case EPppFsmOpened:
		FsmLayerDown(aReason);
		// continue into next statement
	case EPppFsmReqSent:
	case EPppFsmAckRecvd:
	case EPppFsmAckSent:
		SendInitialTerminateRequest();
		SetState(EPppFsmClosing);
		break;
		}
	}

void MPppFsm::FsmAbort(TInt aReason)
/**
Abort the protocol.

@param aReason Reason for aborting
*/
// 29.11.00.  Changed slightly so that this does the same as RXJ- 
// in the PPP RFC for all states.
	{
	LOG( iPppLcp->iLogger->Printf(_L("%s FSM Abort Request, error[%d]"), __iFsmName, aReason); )
	switch (iState)
		{
	case EPppFsmInitial:
	case EPppFsmStarting:
		// bad event
		break;
	case EPppFsmClosed:
	case EPppFsmStopped:
		FsmLayerFinished();
		break;
	case EPppFsmClosing:
		FsmLayerFinished();
		SetState(EPppFsmClosed);
		break;
	case EPppFsmStopping:
		FsmLayerFinished();
		SetState(EPppFsmStopped);
		break;
	case EPppFsmOpened:
		FsmLayerDown();
		SendInitialTerminateRequest();
		SetState(EPppFsmStopping);
		break;
	case EPppFsmReqSent:
	case EPppFsmAckRecvd:
	case EPppFsmAckSent:
		FsmLayerFinished(aReason);
		SetState(EPppFsmStopped);
		break;
		}
	iPppAbortCode = aReason;
	}



//
// Upcall from Timer
//

EXPORT_C void MPppFsm::TimerComplete(TInt /*aStatus*/)
/**
Called by MTimer on timer expiry.

@param aStatus Aynchronous request completion status (ignored)
*/
	{
	LOG( iPppLcp->iLogger->Printf(_L("%s FSM Timer Expired %d "), __iFsmName, iRestartCount); )

	switch (iState)
		{
	case EPppFsmInitial:
	case EPppFsmStarting:
	case EPppFsmClosed:
	case EPppFsmStopped:
	case EPppFsmOpened:
		// Bad Event
		return;
	default:
		break;
		}

	if (iRestartCount>0)
		{
		switch (iState)
			{
		case EPppFsmClosing:
		case EPppFsmStopping:
			SendTerminateRequest();
			break;
		case EPppFsmReqSent:
		case EPppFsmAckRecvd:
		case EPppFsmAckSent:
			//PG RFC 1661 4.6 double timeout period
			// Allow for configuration of non-doubling of timeout period (initially for
			// conformance testing) - default behaviour is for doubling to occur.
			if (!iWaitTimeNoIncrease && iWaitTime*2 < KPppFsmRequestMaxTimeout)
				{
				iWaitTime = iWaitTime * 2;
				}
			SendConfigRequest();
			if(iState == EPppFsmAckRecvd)
				SetState(EPppFsmReqSent);
			break;
		default:
			break;
			}
		}
	else		
		{		
 		switch(iState)
  			{
  			case EPppFsmClosing: // We sent out TerminateRequests, no TerminateAcks received.
 				FsmLayerFinished(iFsmTerminationCauseError); // Authoritative Close of the Link --> KErrCancel
  				break;
  			case EPppFsmStopping: // We've sent TerminateAck and waited one restart period
  				FsmLayerFinished(iFsmTerminationCauseError); // Peer initiated disconnection.
  				break;
  			default:
  				// Don't abort if we're configured to persist.
  				if(!iPersist)
  					{
  					iFsmTermination = ETrue; // Used later to bring down the FSM as opposed to restarting it.
  					FsmLayerFinished(KErrTimedOut);
  					}
  				else
 					{
 					LOG( iPppLcp->iLogger->Printf(_L("%s FSM going idle"), __iFsmName); )
 					}
  				break;
  			}
		
		switch (iState)
			{
		case EPppFsmClosing:
			SetState(EPppFsmClosed);
			break;
		case EPppFsmStopping:
		case EPppFsmReqSent:
		case EPppFsmAckRecvd:
		case EPppFsmAckSent:
			SetState(EPppFsmStopped);
			break;
		case EPppFsmInitial:
		case EPppFsmStarting:
		case EPppFsmClosed:
		case EPppFsmStopped:
		case EPppFsmOpened:
		default:
			break;
			}
		}
	}

//
// Upcalls from Recvr
//

EXPORT_C void MPppFsm::LowerLayerUp()
/**
Called when the lower layer protocol has come up.
*/
	{
	LOG( iPppLcp->iLogger->Printf(_L("%s FSM Up Event"), __iFsmName); )
	switch (iState)
		{
	case EPppFsmInitial:
		SetState(EPppFsmOpened);
		break;
	case EPppFsmStarting:
		SendInitialConfigRequest();
		SetState(EPppFsmReqSent);
		break;
	case EPppFsmClosed:
	case EPppFsmStopped:
	case EPppFsmClosing:
	case EPppFsmStopping:
	case EPppFsmReqSent:
	case EPppFsmAckRecvd:
	case EPppFsmAckSent:
	case EPppFsmOpened:
		// Bad Event
		break;
		}
	}


EXPORT_C void MPppFsm::LowerLayerDown(TInt aStatus)
/**
Signals the Down event.
Called when the lower layer protocol has gone down.
This means that our layer can no longer receive or transmit anything.

In case of LCP, this means that the physical link is closed (e.g. Peer dropped DTR)
If we are LCP, this may mean that PPP is finished, because the link is no longer available.

@param aStatus Error code indicating the reason the layer is going down
*/
	{
	LOG( iPppLcp->iLogger->Printf(_L("%s FSM Down Event, error[%d]"), __iFsmName, aStatus); )
	
	switch (iState)
		{
	case EPppFsmInitial:
		// Bad Event
		break;
	case EPppFsmStarting:
		FsmLayerDown(aStatus);
		break;
	case EPppFsmClosed:
		SetState(EPppFsmInitial);
		break;
	case EPppFsmClosing: // We are waiting for Terminate Ack (or TerminateRequest retransmission)
		SetState(EPppFsmInitial);
		break;
	case EPppFsmStopped:
		if(!iFsmTermination) // We are not terminating, so we restart the FSM.
			{
			FsmLayerStarted();
			SetState(EPppFsmStarting);
			return; // The alternative is calling FsmTerminationPhaseComplete from every case.	
			}
		// If we are terminating, we terminate the FSM and notify Nifman that the NIF is finished.				
		SetState(EPppFsmStarting);
		break;
	case EPppFsmStopping: // We are waiting 1 restart period after transmitting Terminate Ack 
		SetState(EPppFsmStarting);
		break;
	case EPppFsmReqSent:
	case EPppFsmAckRecvd:
	case EPppFsmAckSent:
		FsmLayerDown(aStatus);
		SetState(EPppFsmStarting);
		break;
	case EPppFsmOpened:
		FsmLayerDown(aStatus);
		SetState(EPppFsmStarting);
		break;
		}
	// This call is critical to make sure PPP reports its demise correctly. Otherwise, Nifman remains unaware, and 
	// ESock / Nifman / PPP hangs.	
	FsmTerminationPhaseComplete(); // ASSUMPTION: LCP signals to Nifman. Others do nothing.	
	// Note: early return from EPppFsmStopped.
	}

EXPORT_C void MPppFsm::FrameError()
/**
Called when a bad frame is received.
No action is currently taken.

@see RecvFrame() is called instead when a good frame is received
*/
	{
	return;
	}

EXPORT_C void MPppFsm::KillProtocol()
/**
Called when the lower level protocol is killed.
*/
	{
	//
	// This came to light as a result of the CCP work
	// This happens if a protocol is told to shut down,
	// I'm not sure how to handle it?? Do we just Stop it???
	//
	return;
	}

EXPORT_C TBool MPppFsm::RecvFrame(RMBufChain& aPacket)
/**
Receives and processes a PPP frame.
Called by CPppLcp.

@see FrameError() is called instead when a bad frame is received

@param aPacket MBuf chain containing packet

@return EFalse
*/
	{
	RMBufPacket pkt;
	pkt.Assign(aPacket);
	pkt.Unpack();
	RMBufPktInfo* info = pkt.Info();

	TUint8 op;
	TUint8 id;
	TInt len;
	
	// Extract and drop LCP header
	pkt.Align(4);
	TUint8* ptr = pkt.First()->Ptr();
	op = *ptr++;
	id = *ptr++;
	len = BigEndian::Get16(ptr);
	
	// Check packet length is OK
	if (info->iLength<len || info->iLength<4 )
		{
		// Too short!
		pkt.Free();
		return EFalse;
		}
	else if (info->iLength > len)
		pkt.TrimEnd(len);
	
	// Received a potentially valid packet; probably we have a meaningful relationship with peer
	iNoEvidenceOfPeer = EFalse;	
	// If op is unknown
	switch (op)
		{
	case KPppLcpConfigAck:
	case KPppLcpConfigNak:
	case KPppLcpConfigReject:
		// Filter out bad acks
		if (iState>=EPppFsmReqSent && id!=iRequestId)
			break;
		else
			iRequestId |= KPppRequestIdAnswered;
		// fall through ...
	case KPppLcpConfigRequest:
		// Option negotiation ops
		// Split the MBuf chain in separate options

		//if(len<=4)
		if(len<4)
			break;

		//
		// Hmmm sometimes we receive a Config request of Length 4
		// i.e. no options Send a Config ACK
		// 
		if ( (len == 4) && (op == KPppLcpConfigRequest))
			{
			 if (ProcessEmptyConfigReq())
				{
				if (iPppId == KPppIdCcp)
					{
					// For empty CCP Config Requests send a Protocol Reject,
					// otherwise we can end up continually exchanging empty
					// Config Requests and Config ACK's.  KPppIdLcp is passed to
					// CPppHdlcLink::Send() via FsmRejectPacket() to ensure
					// transmission of an *LCP* Protocol Reject of CCP rather than
					// a *CCP* Protocol Reject of CCP (subtle difference).
					pkt.Pack();
					FsmRejectPacket(pkt, KPppLcpProtocolReject, KPppIdLcp);
					}
				else
					{
					// Send an acknowledgement
					ptr = pkt.First()->Ptr();
					*ptr = KPppLcpConfigAck;
					pkt.Pack();
					SendFrame(pkt);
					}
				}
			 else
				{
				break;
				}
			}
		else if (len == 4 && (op != KPppLcpConfigAck && iPppLcp->QueryExternalIPConfiguration()))
			{
			// With Mobile IP, there is the possibility of having no options to negotiate, in which
			// case an empty ConfigReq would be sent out.  This would be acknowledged by an empty
			// ConfigAck, so ensure that we process the latter.
			break;
			}
		else
			{
			pkt.TrimStart(4);
			ProcessConfig(op, id, len-4, pkt);
			}

		break;	
	case KPppLcpTerminateAck:
		// Filter out bad acks
		if ((iState==EPppFsmClosing || iState==EPppFsmStopping) && id!=iTerminateId)
			break;
		else
			iTerminateId |= KPppRequestIdAnswered;
		// fall through ...
	case KPppLcpTerminateRequest:
		if(len<4)
			break;

		pkt.TrimStart(4);
		ProcessTerminate(op, id, len-4, pkt);
		break;
	case KPppLcpCodeReject:
	case KPppLcpProtocolReject:
		if(len<=4)
			break;
		pkt.TrimStart(4);
		ProcessReject(op, id, len-4, pkt);
		break;
	default:		
		if (!FsmRecvUnknownCode(op, id, len, pkt))
			{
			pkt.Pack();
			FsmRejectPacket(pkt);
			}
		break;
		}
	pkt.Free();
	return EFalse;
	}
	
//
// Rest of PPP
//

TBool MPppFsm::FsmRecvUnknownCode(TUint8 /*aCode*/, TUint8 /*aId*/, TInt /*aLength*/, RMBufChain& /*aPacket*/)
/**
Handle a received packet with an unknown code.

@param aCode Packet code (ignored)
@param aId Packet identifier (ignored)
@param aLength Length of packet (ignored)
@param aPacket MBuf chain containing packet (ignored)

@return EFalse
*/
	{
	return EFalse;
	}

void MPppFsm::ProcessReject(TUint8 aCode, TUint8 /*aId*/, TInt /* aLength */, RMBufChain& aPacket)
/**
Handle a Code Reject or Protocol Reject.
If the reject rejects a code (or protocol) that should
be known terminate layer.
(Note, As the behaviour of protocol reject is the same
as code reject, this function handles protocol reject for LCP)

@param aCode Packet code
@param aId Packet identifier (ignored)
@param aLength Length of packet (ignored)
@param aPacket MBuf chain containing packet.
*/
	{
	TBool isCatastrophic = EFalse;
	
	// Valid reject packets should have a len >=4
	if(aPacket.Length()<4)
		return;

	// Determine if the reject is acceptable
	if (aCode==KPppLcpProtocolReject)
		{
		
		aPacket.Align(2);
		TUint prot = BigEndian::Get16(aPacket.First()->Ptr());
		aPacket.TrimStart(2);
		switch (prot)
			{
			case KPppIdLcp:
				isCatastrophic = ETrue;
				break;

			case KPppIdCompressedData:
				// Shut down and unload the current compressor
				LOG( iPppLcp->iLogger->Printf(_L("Compressed Packet rejected: Unloading compressor.")); )
				iPppLcp->PppUnloadCompressor() ;
				break ;

			default:
				{
				/*
				*	Tut tut the existing code only handled Protocol rejects for the 
				*	LCP protocol. What about all the others?
				*/
				iFsmTermination = ETrue; // Used later to bring down the FSM as opposed to restarting it.
				iPppLcp->StopProtocol(prot);
				break;
				}
			}
		}
	else
		{
		TUint8 code = aPacket.First()->Get(0);
		switch (code)
			{
		case KPppLcpConfigRequest:
		case KPppLcpConfigAck:
		case KPppLcpConfigNak:
		case KPppLcpConfigReject:
		case KPppLcpTerminateRequest:
		case KPppLcpTerminateAck:
			isCatastrophic = ETrue;
			break;
		case KPppLcpCodeReject:
		case KPppLcpProtocolReject:
			iFsmTermination = ETrue;
			isCatastrophic = ETrue;
			break;
		default:
			break;
			}
		}

	if (!isCatastrophic)
		{
		if (iState==EPppFsmAckRecvd)
			SetState(EPppFsmReqSent);
		return;
		}

	LOG( iPppLcp->iLogger->Printf(_L("%s Catastrophic Reject, packet code[%d]"), __iFsmName, aCode); )

	switch (iState)
		{
	case EPppFsmInitial:
	case EPppFsmStarting:
		// bad event
		break;
	case EPppFsmClosed:
	case EPppFsmStopped:
		FsmLayerFinished();
		break;
	case EPppFsmClosing:
		FsmLayerFinished();
		SetState(EPppFsmClosed);
		break;
	case EPppFsmStopping:
	case EPppFsmReqSent:
	case EPppFsmAckRecvd:
	case EPppFsmAckSent:
		FsmLayerFinished();
		SetState(EPppFsmStopped);
		break;
	case EPppFsmOpened:
		FsmLayerDown();
		SendInitialTerminateRequest();
		SetState(EPppFsmStopped);
		break;
		}
	}
	
void MPppFsm::ProcessTerminate(TUint8 aCode, TUint8 aId, TInt /*aLength*/, RMBufChain& /*aPacket*/)
/**
Handle a Terminate Request packet and cleanly terminate the connection.

@param aCode Packet code
@param aId Packet identifier
@param aLength Length of packet (ignored)
@param aPacket MBuf chain containing packet (ignored)
*/
	{
	LOG( iPppLcp->iLogger->Printf(_L("%s Terminate Request, packet code[%d]"), __iFsmName, aCode); )

	if (aCode==KPppLcpTerminateRequest)
		{
		LOG( iPppLcp->iLogger->Printf(_L("Rx: TerminateRequest Id[%d]"), aId); )
		iFsmTermination = ETrue; // Used later to bring down the FSM as opposed to restarting it.
		iFsmTerminationCauseError = KErrDisconnected;
		switch (iState)
			{
		case EPppFsmInitial:
		case EPppFsmStarting:
			// bad event
			break;
		case EPppFsmClosed:
		case EPppFsmStopped:
		case EPppFsmClosing:
		case EPppFsmStopping:
			SendTerminateAck(aId);
			break;
		case EPppFsmReqSent:
		case EPppFsmAckRecvd:
		case EPppFsmAckSent:
			SendTerminateAck(aId);
			FsmLayerFinished(KErrCouldNotConnect);
			SetState(EPppFsmReqSent);
			break;			
		case EPppFsmOpened:
			ZeroRestartCount();
			if(iTerminateAckEnabled) // Fully RFC compliant shutdown sequence, as opposed to "Legacy" shutdown.
									 // Once "Legacy" shutdown is removed, a check for TerminateAckEnabled may
									 // be safely removed as well.
				{                    
				SendTerminateAck(aId);
				
				//  RFC1661 3.7: wait at least one restart period
				TimerCancel();
 				TimerAfter(iTerminateAckTimeout * 1000); 
 				// If iTerminateAckTimeout is zero, we proceed to terminate the FSM and close the link almost
 				// immediately. This may result in TerminateAck send being cancelled when we close the actual link. 
 		
				FsmLayerDown(KErrDisconnected);
				}
			else // "Legacy" shutdown sequence			     
			     // Once "legacy" behaviour is not required any more, this section of code may be safely removed.
				{
				// We do not send terminate Ack.
				FsmLayerDown();
				FsmLayerFinished(KErrCommsLineFail);
				// Note: 
				// KErrCommsLineFail is given a special interpreation by the NCP, which may inform Nifman to 
				// renegotiate the link, rather than terminate. 
				}
				
			SetState(EPppFsmStopping);
			break;
		default:
			break;
			}					
		} // KPppLcpterminateRequest
	else // KPppLcpTerminateAck
		{
		LOG( iPppLcp->iLogger->Printf(_L("Rx: TerminateAck Id[%d]"), aId); )
		
		if (iPppAbortCode!=KErrNone)
			{
			iPppLcp->PhaseAborted(iPppAbortCode);
			}
		else
			{
			switch (iState)
				{
			case EPppFsmInitial:
			case EPppFsmStarting:
				// bad event
				break;
			case EPppFsmClosed:
			case EPppFsmStopped:				
				break;
			case EPppFsmClosing: // Terminate Request Sent
				FsmLayerFinished(iFsmTerminationCauseError);
				SetState(EPppFsmClosed);
				break;
			case EPppFsmStopping: // Terminate Ack sent
				FsmLayerFinished(iFsmTerminationCauseError);
				SetState(EPppFsmStopped);
				break;
			case EPppFsmReqSent:
			case EPppFsmAckSent:
				KillProtocol();
 				break;
			case EPppFsmAckRecvd:
				SetState(EPppFsmReqSent);
				break;
			case EPppFsmOpened:
				FsmLayerDown();
				SendConfigRequest();
				SetState(EPppFsmReqSent);
				break;
				}
			}
		}
	}

TBool MPppFsm::ProcessEmptyConfigReq()
/**
Handle Config Request with no options.

@return ETrue on a valid state change
*/
	{	
	LOG( iPppLcp->iLogger->Printf(_L("%s Empty Config Request"), __iFsmName); )


   TBool retCode = EFalse;


	// About to drop out of opened state to need to reset things
	// BEFORE the new request is processed.
	if (iState==EPppFsmOpened)
		{
		FsmLayerDown();
		InitialiseConfigRequest();
		}
	

	// State processing

	switch (iState)
		{
	case EPppFsmInitial:
	case EPppFsmStarting:
		// Bad event
	case EPppFsmClosing:
	case EPppFsmStopping:
		break;

	case EPppFsmClosed:
		//SendTerminateAck(aId);
		break;
	case EPppFsmStopped:
      {
		SendInitialConfigRequest();
		SetState(EPppFsmAckSent);
      retCode = ETrue;
		break;
		}
	case EPppFsmReqSent:
      {
      retCode = ETrue;
		SetState(EPppFsmAckSent);
		break;
		}
	case EPppFsmAckRecvd:
		{
      retCode = ETrue;
		SetState(EPppFsmOpened);
		FsmLayerUp();
		break;
		}
	case EPppFsmAckSent:
		{
      retCode = ETrue;
		break;
		}
	case EPppFsmOpened:
		{	
		// This will cause empty config request to restart config negotiation
		SendInitialConfigRequest();
		SetState(EPppFsmAckSent);
		retCode=ETrue;
		break;
		}
      }
   return retCode;
	}	

void MPppFsm::ProcessConfig(TUint8 aCode, TUint8 aId, TInt /* aLength */, RMBufChain& aPacket)
/**
Handle ConfigRequest, ConfigAck, ConfigNak and ConfigReject

@param aCode Packet code
@param aId Packet identifier
@param aLength Length of packet (ignored)
@param aPacket MBuf chain containing packet.
*/
	{	
	LOG( iPppLcp->iLogger->Printf(_L("%s Process Config, packet code[%d]"), __iFsmName, aCode); )

	// Split the recvd packet into separate options
	// placing each option in a queue where it can be
	// easily parsed and manipulated in the upcall handlers.
	RPppOptionList rcvlist;	// Recvd list of options

	TRAPD(err, rcvlist.SetL(aPacket));
	if (err!=KErrNone)
		return;
	
	enum TCheckResult { ENop, EAck, ENak, ERej } reply = ENop;

	RPppOptionList acklist;	// List of Ack'd options - only valid of the following are empty
	RPppOptionList naklist;	// List of Nak'd options - only valid of the following is empty
	RPppOptionList rejlist;	// List of Rejected options

	if (aCode==KPppLcpConfigRequest)
		{
		// About to drop out of opened state to need to reset things
		// BEFORE the new request is processed.
		if (iState==EPppFsmOpened)
			{
			FsmLayerDown();
			InitialiseConfigRequest();
			}

		// If any duplicated RFC1661 options, then discard the packet		
		if (!FsmConfigRequestOptionsValid(rcvlist))
			{
			LOG( iPppLcp->iLogger->Printf(_L("%s FSM - ConfigRequest discarded due to duplicate RFC1661 options"), __iFsmName); )
			rcvlist.Free();
			return;
			}

		// Check options, and split into list of OK, Bad and Unknown
		// Note - the naklist options will have been updated to
		// contain acceptable values.

		FsmCheckConfigRequest(rcvlist, acklist, naklist, rejlist);

		if (!rejlist.IsEmpty())
			{
			reply = ERej;
			naklist.Free();
			acklist.Free();
			}
		else if (!naklist.IsEmpty())
			{
			reply = ENak;
			acklist.Free();
			}
		else if (!acklist.IsEmpty())
			{
			reply = EAck;
			FsmApplyConfigRequest(acklist);
			}
		else
			{
			// Panic - options lost by derived class!
			}
		}		
		
	// State processing

	switch (iState)
		{
	case EPppFsmInitial:
	case EPppFsmStarting:
		// Bad event
	case EPppFsmClosing:
	case EPppFsmStopping:
		break;

	case EPppFsmClosed:
		SendTerminateAck(aId);
		break;

	case EPppFsmStopped:
		switch (aCode)
			{
		case KPppLcpConfigRequest:
			SendInitialConfigRequest();
			switch (reply)
				{
			case EAck:
				SendConfigReply(acklist, KPppLcpConfigAck, aId);
				SetState(EPppFsmAckSent);
				break;
			case ENak:
				SendConfigReply(naklist, KPppLcpConfigNak, aId);
				SetState(EPppFsmReqSent);
				break;
			case ERej:
				SendConfigReply(rejlist, KPppLcpConfigReject, aId);
				SetState(EPppFsmReqSent);
				break;
			default:
				break;
				}
			break;
		case KPppLcpConfigAck:
		case KPppLcpConfigNak:
		case KPppLcpConfigReject:
			SendTerminateAck(aId);
			break;
		default:
			break;
			}
		break;
	
	case EPppFsmReqSent:
		switch (aCode)
			{
		case KPppLcpConfigRequest:
			switch (reply)
				{
			case EAck:
				SendConfigReply(acklist, KPppLcpConfigAck, aId);
				SetState(EPppFsmAckSent);
				break;
			case ENak:
				// RFC 1661 4.6
				if(MaxFailureExceeded())
					SendConfigReply(naklist, KPppLcpConfigReject, aId);
				else
					{
					DecrementMaxFailure();
					SendConfigReply(naklist, KPppLcpConfigNak, aId);
					}
				SetState(EPppFsmReqSent);
				break;
			case ERej:
				SendConfigReply(rejlist, KPppLcpConfigReject, aId);
				SetState(EPppFsmReqSent);
				break;
			default:
				break;
				}
			break;
		case KPppLcpConfigAck:
			if (FsmAckOptionsValid(rcvlist, iRequestList))
				{
				InitRestartCountForConfig();
				FsmRecvConfigAck(rcvlist);
				SetState(EPppFsmAckRecvd);
				}
			else
				{
				LOG( iPppLcp->iLogger->Printf(_L("%s FSM - ConfigAck discarded due to option mismatch with original ConfigRequest"), __iFsmName); )
				}
			break;
		case KPppLcpConfigNak:
			InitRestartCountForConfig();
			FsmRecvConfigNak(rcvlist, iRequestList);
			SendConfigRequestAfterNak(rcvlist);
			break;
		case KPppLcpConfigReject:
			InitRestartCountForConfig();
			FsmRecvConfigReject(rcvlist, iRequestList);
			SendConfigRequestAfterReject(rcvlist);
			break;
		default:
			break;
			}
		break;
	
	case EPppFsmAckRecvd:
		switch (aCode)
			{
		case KPppLcpConfigRequest:
			switch (reply)
				{
			case EAck:
				SendConfigReply(acklist, KPppLcpConfigAck, aId);
				SetState(EPppFsmOpened);
				FsmLayerUp();
				break;
			case ENak:
				// RFC 1661 4.6
				if(MaxFailureExceeded())
					SendConfigReply(naklist, KPppLcpConfigReject, aId);
				else
					{
					DecrementMaxFailure();
					SendConfigReply(naklist, KPppLcpConfigNak, aId);
					}
				break;
			case ERej:
				SendConfigReply(rejlist, KPppLcpConfigReject, aId);
				break;
			default:
				break;
				}
			break;
		case KPppLcpConfigAck:
		case KPppLcpConfigNak:
		case KPppLcpConfigReject:
			SendConfigRequest();
			SetState(EPppFsmReqSent);
			break;
		default:
			break;
			}
		break;

	case EPppFsmAckSent:
		switch (aCode)
			{
		case KPppLcpConfigRequest:
			switch (reply)
				{
			case EAck:
				SendConfigReply(acklist, KPppLcpConfigAck, aId);
				break;
			case ENak:
				// RFC 1661 4.6
				if(MaxFailureExceeded())
					SendConfigReply(naklist, KPppLcpConfigReject, aId);
				else
					{
					DecrementMaxFailure();
					SendConfigReply(naklist, KPppLcpConfigNak, aId);
					}
				SetState(EPppFsmReqSent);
				break;
			case ERej:
				SendConfigReply(rejlist, KPppLcpConfigReject, aId);
				SetState(EPppFsmReqSent);
				break;
			default:
				break;
				}
			break;
		case KPppLcpConfigAck:
			if (FsmAckOptionsValid(rcvlist, iRequestList))
				{
				InitRestartCountForConfig();
				FsmRecvConfigAck(rcvlist);
				SetState(EPppFsmOpened);
				FsmLayerUp();
				}
			else
				{
				LOG( iPppLcp->iLogger->Printf(_L("%s FSM - ConfigAck discarded due to option mismatch with original ConfigRequest"), __iFsmName); )
				}
			break;
		case KPppLcpConfigNak:
			InitRestartCountForConfig();
			FsmRecvConfigNak(rcvlist, iRequestList);
			SendConfigRequestAfterNak(rcvlist);
			break;
		case KPppLcpConfigReject:
			if (FsmRejectOptionsValid(rcvlist, iRequestList))
				{
				InitRestartCountForConfig();
				FsmRecvConfigReject(rcvlist, iRequestList);
				SendConfigRequestAfterReject(rcvlist);
				}
			else
				{
				LOG( iPppLcp->iLogger->Printf(_L("%s FSM - ConfigReject discarded due to option mismatch with original ConfigRequest"), __iFsmName); )
				}
			break;
		default:
			break;
			}
		break;

	case EPppFsmOpened:
		// Config Reset done above
		switch (aCode)
			{
		case KPppLcpConfigRequest:
			SendConfigRequest();
			switch (reply)
				{
			case EAck:
				SendConfigReply(acklist, KPppLcpConfigAck, aId);
				SetState(EPppFsmAckSent);
				break;
			case ENak:
				SendConfigReply(naklist, KPppLcpConfigNak, aId);
				SetState(EPppFsmReqSent);
				break;
			case ERej:
				SendConfigReply(rejlist, KPppLcpConfigReject, aId);
				SetState(EPppFsmReqSent);
				break;
			default:
				break;
				}
			break;
		case KPppLcpConfigAck:
			SendConfigRequest();
			FsmRecvConfigAck(rcvlist);
			SetState(EPppFsmReqSent);
			break;
		case KPppLcpConfigNak:
			FsmRecvConfigNak(rcvlist, iRequestList);
			SendConfigRequestAfterNak(rcvlist);
			SetState(EPppFsmReqSent);
			break;
		case KPppLcpConfigReject:
			FsmRecvConfigReject(rcvlist, iRequestList);
			SendConfigRequestAfterReject(rcvlist);
			SetState(EPppFsmReqSent);
			break;
		default:
			break;
			}
		break;

	default:
		// Invalid state
		LOG( iPppLcp->iLogger->Printf(_L("%s Invalid state %d"), __iFsmName, iState); )
		break;
		}
	rcvlist.Free();	
	acklist.Free();
	naklist.Free();
	rejlist.Free();
	}	

void MPppFsm::SendConfigRequest()
/**
Send the config request in iRequestList
*/
	{
	if (iPppAbortCode!=KErrNone)
		return;
	

	// With Mobile IP, there is the possibility of having no options to negotiate, in which case
	// we would send an empty ConfigRequest rather than nothing.

	TBool emptyList = iRequestList.IsEmpty();
	if (emptyList && !iPppLcp->QueryExternalIPConfiguration())
		{
		LOG( iPppLcp->iLogger->Printf(_L("%s FSM Request list empty"), __iFsmName); )
		return;
		}

	RMBufPacket pkt;	
	iRequestId &= 0xff;
	TRAPD(err, iRequestList.CreatePacketL(pkt, iPppId, KPppLcpConfigRequest, (TUint8)iRequestId, emptyList));
	if (err!=KErrNone)
		{
		//__DEBUGGER();
		return;
		}
	SendFrame(pkt);
	--iRestartCount;
	TimerCancel();
	TInt temp=iWaitTime;
	if (iPppId==KPppIdIpcp && iLengthenTimers)
		temp=KPppFsmLengthenedRequestTimeout;
	TimerAfter(temp*1000);
	}


TInt MPppFsm::InitialiseConfigRequest()
/**
Delete any existing request list, then create a new one.
Initialise counters for sending config requests.

@return Error code

@post iRequestList is initialized
*/
	{
	iLastCfgReqFcs = 0;
	iConsecCfgReq = 0;
	iRequestId = FsmNewId();
	InitRestartCountForConfig();
	if (!iRequestList.IsEmpty())
		iRequestList.Free();
	TRAPD(err, FsmFillinConfigRequestL(iRequestList));
	return err;
	}

void MPppFsm::SendInitialConfigRequest()
/**
Initialise request list and send a request if successful.
*/
	{
	if (InitialiseConfigRequest()==KErrNone)
		SendConfigRequest();
	}

void MPppFsm::SendConfigRequestAfterNak(RPppOptionList& /*aOptsList*/)
/**
Update the options list and send a new config request after a Nak.

@param aOptList Options list (ignored)
*/
	{
	iRequestId = FsmNewId();

	// Calc a 32bit CRC of the request list data
	// and compare with last recorded RequestAfterNAK CRC.
	// If the same, then it is more likely that negotiation
	// is not converging.
	TPppFcs32 fcs;
	iRequestList.Crc32(fcs);

	if (fcs.Fcs()==iLastCfgReqFcs)
		{
		if (++iConsecCfgReq>KPppFsmNonConvergeLimit)
			{
			LOG( iPppLcp->iLogger->Printf(_L("NonConvergence limit reached")); )
			FsmAbort(KErrTimedOut);
			return;
			}
		}
	else
		{
		iLastCfgReqFcs = fcs.Fcs();
		iConsecCfgReq = 0;
		}

	SendConfigRequest();
	}

void MPppFsm::SendConfigRequestAfterReject(RPppOptionList& /*aOptsList*/)
/**
Update the options list and send a new config request after a Reject.

@param aOptList Options list (ignored)
*/
	{
	iRequestId = FsmNewId();
	SendConfigRequest();
	}

void MPppFsm::SendConfigReply(RPppOptionList& aOptList, TUint8 aType, TUint8 aId)
/**
Reply to a config request with a set of options.

@param aOptList Options list
@param aType Packet type (Ack, Nak, Rej)
@param aId Packet identifier
*/
	{
	if (iPppAbortCode!=KErrNone)
		return;
	
	RMBufPacket pkt;	
	TRAPD(err, aOptList.CreatePacketL(pkt, iPppId, aType, aId, EFalse));
	if (err!=KErrNone)
		{
		//__DEBUGGER();
		return;
		}
	SendFrame(pkt);
//	TimerCancel();
//	TimerAfter(iWaitTime*1000);
	}

void MPppFsm::SendInitialTerminateRequest()
/**
Send the first Terminate Request packet to begin connection teardown.
*/
	{
	iTerminateId = FsmNewId();
	InitRestartCountForTerminate();
	if(iRestartCount > 0)
		{
		SendTerminateRequest();
		}
	else // We are configured to send zero Terminate Requests. 
		{
		TimerCancel();
		TimerAfter(0);	// As if we finished transmitting Terminate Requests.
		}
	}

void MPppFsm::SendTerminateRequest()
/**
Send a Terminate Request packet.

@pre SendInitialTerminateRequest() must have previously been called
*/
	{
	RMBufPacket pkt;
	const TUint pktLen = 4;
	TUint8* ptr = NewPacket(pkt, pktLen);
	if (ptr == NULL)
		{
		__ASSERT_DEBUG(EFalse,PppPanic(EPppPanic_PPPNoMemory));
		return;
		}
		*ptr++ = KPppLcpTerminateRequest;
		iTerminateId &= 0xff;
		*ptr++ = (TUint8)iTerminateId;
	BigEndian::Put16(ptr, (TUint16)pktLen);
		pkt.Pack();
		SendFrame(pkt);

		--iRestartCount;
		TimerCancel();
		TInt temp=iWaitTime;
		if (iLengthenTimers)
		temp=KPppFsmLengthenedTerminateTimeout;
	TimerAfter(temp*1000);
	LOG( iPppLcp->iLogger->Printf(_L("Tx: TerminateRequest Id[%d]. Restart Count[%d]"), iTerminateId, iRestartCount); )
   	}

void MPppFsm::SendTerminateAck(TUint8 aId)
/**
Send a Terminate Ack packet.

@param aId Packet identifier
*/
	{
	RMBufPacket pkt;
	const TUint pktLen = 4;
	TUint8* ptr = NewPacket(pkt, pktLen);
	if (ptr == NULL)
		{
		__ASSERT_DEBUG(EFalse,PppPanic(EPppPanic_PPPNoMemory));
		return;
		}
		*ptr++ = KPppLcpTerminateAck;
		*ptr++ = aId;
	BigEndian::Put16(ptr, (TUint16)pktLen);
		pkt.Pack();
		SendFrame(pkt);	
 	 	
 	 	LOG(iPppLcp->iLogger->Printf(_L("Tx: TerminateAck Id[%d]."),aId);)
  		}	
	
void MPppFsm::InitRestartCountForConfig()
/**
Initialize the restart count and wait time
with values appropriate for the config phase.
*/
	{

	// RFC 1661 4.6 Max-Configure
	iWaitTime = iWaitTimeConfig;
	iRestartCount = iMaxRestartConfig;
	}
	
void MPppFsm::InitRestartCountForTerminate()
/**
Initialize the restart count and wait time for
with values appropriate for the termination phase.
*/
	{
	if(iNoEvidenceOfPeer)
		{
		// If we've been told to terminate by our control and haven't yet heard anything from the peer then we 
		// don't attempt to send it a disconnect. This is a little unpleasant because if we've sent it a config
		// packet it may be left waiting for us until it times out, however the more likely state is that we
		// haven't yet completed sending the packet to it because the BCA is still waiting on its bearer to
		// set up. Unfortunately we don't seem to have a palatable way to abort this BCA write from here.
 	 	LOG(iPppLcp->iLogger->Printf(_L("InitRestartCountForTerminate: no evidence of peer contact so terminating without notice"));)
		ZeroRestartCount();
		}
	else
		{
		iWaitTime     = iTerminateRequestTimeout; 
 		iRestartCount = iMaxTerminateRequest;
		}
	}
	
void MPppFsm::ZeroRestartCount()
/**
Clear the restart count.
*/
	{
	iRestartCount = 0;
	}
	
TUint8 MPppFsm::FsmNewId()
/**
Generate a new nonzero packet identifier.

@return Packet identifier
*/
	{
	if (++iCurrentId==0)
		++iCurrentId;
	return iCurrentId;
	}

void MPppFsm::SetState(TPppFsmState aState)
/**
Set the next state in the FSM.
Cancels the timer when appropriate.

@param aState Next state
*/
	{
	LOG( iPppLcp->iLogger->DumpState(__iFsmName, iState, aState); )
	iState = aState;
	if (iState<EPppFsmClosing || iState==EPppFsmOpened)
		{
		if(iState == EPppFsmStarting)
			{
			iNoEvidenceOfPeer = ETrue;
			}
		TimerCancel();
		}
	}


void MPppFsm::FsmRejectPacket(RMBufChain& aPacket, TUint aReason, TUint aPppId)
/**
Send a Code Reject or Protocol Reject packet.

@param aPacket MBuf chain containing packet; it will be used to send the reject message.
@param aReason Reason for rejecting (KPppLcpCodeReject or KPppLcpProtocolReject)
@param aPppId PPP protocol ID
*/
	{
	RMBufPacket pkt;
	pkt.Assign(aPacket);
	pkt.Unpack();
	RMBufPktInfo* info = pkt.Info();
	
	// This function reuses the rejected packet chain to send the reject message,
	// so reserve some space at the beginning for the reject header.
	TInt prep = 0;
	if (aReason==KPppLcpCodeReject)
		prep = 4;
	else if (aReason==KPppLcpProtocolReject)
		prep = 6;
	else
		{
		// Unknown reject reason
		pkt.Free();
		return;
		}

	TRAPD(err, pkt.PrependL(prep));
	if (err!=KErrNone)
		{
		pkt.Free();
		return;
		}
	info->iLength += prep;

	// If the frame to be sent is too large, lop off the end to make it fit.
	__ASSERT_DEBUG(iPppLcp->MaxTransferSize() > 0, PppPanic(EPppPanic_InvalidData));
	if (info->iLength > iPppLcp->MaxTransferSize())
		{
		pkt.TrimEnd(iPppLcp->MaxTransferSize());
		info->iLength = iPppLcp->MaxTransferSize();
		}

	TUint8* ptr = pkt.First()->Ptr();

	*ptr++ = TUint8(aReason);	// xxx
	*ptr++ = FsmNewId();
	BigEndian::Put16(ptr, (TUint16)info->iLength);
	ptr += 2;
	if (aReason==KPppLcpProtocolReject)
		{
		BigEndian::Put16(ptr, (TUint16)TPppAddr::Cast((info->iDstAddr)).GetProtocol());
		TPppAddr::Cast((info->iDstAddr)).SetProtocol(KPppIdLcp);
		}
		
	pkt.Pack();
	if (aPppId == KPppIdAsIs)
		SendFrame(pkt);
	else
		iPppLcp->PppLink()->Send(pkt, aPppId);
	}

void MPppFsm::ReadIniFileL()
/**
Reads the contents of the ppp.ini file.

@leave Error code if file cannot be read
*/
	// Added September 1999
	// Currently can read from ini file :-
	// Max-Configure
	// Max-Failure
	// Restart Timer
	// Enable TerminateRequest / TerminateAck
	// Max TerminateRequest
	// TerminateRequest timeout
	// TerminateAck timeout		
	{
	// Check the ini file exists and can be opened
	CESockIniData* ini = NULL;
	TRAPD(res,
		if (iPppLcp->PppLinkMode() == CPppLcpConfig::EPppLinkIsServer)
			{
			ini = CESockIniData::NewL(PPP_SERVER_INI_FILE);
			}
		else
			{
			ini = CESockIniData::NewL(PPP_INI_FILE);
			}
		)
	if(res!=KErrNone)
		{
		if(res==KErrNotFound)
			{
			// No .ini file; use default values
			iMaxFailureConfig = KPppMaxFailureDefault;
			iMaxRestartConfig = KPppFsmRequestRetries;
			iWaitTimeConfig = KPppFsmRequestTimeout;
			
			
			// Termination Phase Support.
			// If the .ini file is missing, full support is enabled.
			
			iTerminateRequestEnabled = ETrue;
			iMaxTerminateRequest     = KPppFsmTerminateRequestRetries;
			iTerminateRequestTimeout = KPppFsmTerminateRequestTimeout;
 			
			iTerminateAckEnabled = ETrue;
			iTerminateAckTimeout = KPppFsmTerminateAckTimeout;
			
			return;
			}
		User::Leave(res);
		}
	CleanupStack::PushL(ini);
	TInt	entry;
//
// Max-Failure
	// Read enable switch
	if (ini->FindVar(LCPSECTIONNAME,PPPMAXFAILUREENTRYNAME_ENABLE, entry))
		{
		if (entry == 0) 
			iMaxFailureConfig = KPppMaxFailureDefault;
		else
			{
			if (ini->FindVar(LCPSECTIONNAME,PPPMAXFAILUREENTRYNAME_COUNT, entry))
				iMaxFailureConfig = entry;
			else
				iMaxFailureConfig = KPppMaxFailureDefault;
			}
		}
	else
		iMaxFailureConfig = KPppMaxFailureDefault;

//
// Max-Configure
	// Read enable switch
	if (ini->FindVar(LCPSECTIONNAME,PPPMAXRESTARTENTRYNAME_ENABLE, entry))
		{
		if (entry == 0) 
			iMaxRestartConfig = KPppFsmRequestRetries;
		else
			{
			if (ini->FindVar(LCPSECTIONNAME,PPPMAXRESTARTENTRYNAME_COUNT, entry))
				iMaxRestartConfig = entry;
			else
				iMaxRestartConfig = KPppFsmRequestRetries;
			}
		}
	else
		iMaxRestartConfig = KPppFsmRequestRetries;
//
// Restart Timer
	// Read enable switch
	if (ini->FindVar(LCPSECTIONNAME,PPPRESTARTTIMERENTRYNAME_ENABLE, entry))
		{
		if (entry == 0) 
			iWaitTimeConfig = KPppFsmRequestTimeout;
		else
			{
			if (ini->FindVar(LCPSECTIONNAME,PPPRESTARTTIMERENTRYNAME_PERIOD, entry))
				iWaitTimeConfig = entry;
			else
				iWaitTimeConfig = KPppFsmRequestTimeout;
			}
		}
	else
		iWaitTimeConfig = KPppFsmRequestTimeout;
	
	
	//
	// Support PPP Termination Sequence Configurability, required for
 	// CDMA support.
 	//
 	if(ini->FindVar(LCPSECTIONNAME, TERMINATE_REQUEST_ENABLE, entry))
 		{
 		if(0 == entry)
 			{
 			iTerminateRequestEnabled = EFalse; 			
 			} 		
 		}
 		
 	// Max Terminate Requests to be sent.
 	if(iTerminateRequestEnabled)
 		{
 		if(ini->FindVar(LCPSECTIONNAME, MAX_TERMINATE_REQUEST_ENABLE, entry))
	 		{
	 		if(0 != entry)	
	 			{
	 		   	if (ini->FindVar(LCPSECTIONNAME, MAX_TERMINATE_REQUEST_COUNT, entry))
	 				{
	 				iMaxTerminateRequest = entry;
	 				}
	 			}
	 		} 	
	 		
	 	// Normal Terminate Request timeout
	 	if(ini->FindVar(LCPSECTIONNAME, TERMINATE_REQUEST_TIMER_ENABLE, entry))
	 		{
	 		if(0 != entry)	
	 			{
	 			if (ini->FindVar(LCPSECTIONNAME, TERMINATE_REQUEST_TIMER_PERIOD, entry))
	 				{
	 				iTerminateRequestTimeout = entry;
	 				}
	 			}
	 		}	 		 
		}
	else
		{
		// Special Case: Support legacy shutdown behaviour: send only one Terminate Request.
		// To disable sending TerminatRequest entirely, Enable TerminateRequest, and set MaxTerminateRequest to 0.
		// When support for legacy shutdown is not necessary, this code can be safely removed.
		iMaxTerminateRequest = 1;		
		}
		
	//
	// Terminate ACK configurability
	//
	// Is sending a configure ACK enabled?
 	if(ini->FindVar(LCPSECTIONNAME, TERMINATE_ACK_ENABLE, entry))
 		{
 		if(0 == entry)	
 			{
 			iTerminateAckEnabled = EFalse;
 			}
 		}
 	
 	// Read Terminate ACK settings only if it is enabled
 	if(iTerminateAckEnabled)
 		{
 		if(ini->FindVar(LCPSECTIONNAME, TERMINATE_ACK_TIMER_ENABLE, entry))
 			{	
 			if(0 != entry)
 				{	
 				if(ini->FindVar(LCPSECTIONNAME, TERMINATE_ACK_TIMER_PERIOD, entry))
 					{
 					iTerminateAckTimeout = entry;
 					} 				
 				} 			
 			} 			 			
 		}
 
 	// Setting that controls doubling of timeout value on restart timer
 	if (ini->FindVar(LCPSECTIONNAME, PPP_RESTARTTIMER_ENTRYNAME_MODE, entry))
 		{
 		if (entry == 1)
 			{
 			iWaitTimeNoIncrease = ETrue;
 			}
 		}
 
 		
 	CleanupStack::PopAndDestroy();
	}

TUint8* MPppFsm::NewPacket(RMBufPacket& aPkt, TUint aLength)
/**
Allocate a new packet buffer and info header. The caller must fill in the
the packet and call its Pack() method before sending.

@param aPkt reference to a packet
@param aLength length of the buffer
@return pointer to the beginning of the first packet in the chain or NULL on error

@see RPppOptionList::CreatePacketL
*/
	{
	TRAPD(err, aPkt.AllocL(aLength));
	if (err != KErrNone)
		{
		return NULL;
		}
	RMBufPktInfo* info=NULL;
	TRAP(err,info = aPkt.NewInfoL());
	if (err != KErrNone)
		{
		aPkt.Free();
		return NULL;
		}
	info->iLength = aLength;
	TPppAddr::Cast((info->iDstAddr)).SetProtocol(iPppId);
	return aPkt.First()->Ptr();
	}


TBool MPppFsm::FsmAckOptionsValid(RPppOptionList& /*aList*/, RPppOptionList& /*aRequestList*/)
/**
Perform validation checking on the option list of a ConfigAck or ConfigReject.

@param aList option list of incoming ConfigAck or ConfigReject
@return ETrue if options valid, else EFalse.  EFalse return causes packet to be discarded.
*/
	{
	return ETrue;
	}

TBool MPppFsm::FsmRejectOptionsValid(RPppOptionList& /*aList*/, RPppOptionList& /*aRequestList*/)
/**
Perform validation checking on the option list of a ConfigAck or ConfigReject.

@param aList option list of incoming ConfigAck or ConfigReject
@return ETrue if options valid, else EFalse.  EFalse return causes packet to be discarded.
*/
	{
	return ETrue;
	}
TBool MPppFsm::FsmConfigRequestOptionsValid(RPppOptionList& /*aList*/)
/**
Perform validation checking on the option list of a ConfigRequest.

@param aList option list of incoming ConfigRequest
@return ETrue if options valid, else EFalse.  EFalse return causes packet to be discarded.
*/
	{
	return ETrue;
	}

