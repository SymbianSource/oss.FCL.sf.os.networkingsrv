// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Implements the DHCPv4 States representing each interface
// 
//

/**
 @file DHCPStates.cpp
 @internalTechnology
*/

#include "DHCPStates.h"
#ifdef _DEBUG
#include "DHCPServer.h"
#endif
#ifdef SYMBIAN_ESOCK_V3
#include <networking/ipeventtypes.h>
#include <comms-infras/netsignalevent.h>
#endif
#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
#include "DHCPAuthentication.h"  //DHCPv4::KReqMaxRetry is defined in this file
#endif // SYMBIAN_NETWORKING_DHCP_MSG_HEADERS			
#include "DHCPStatesDebug.h"

CDHCPState::~CDHCPState()
/**
  * Destructor for this state base class
  *
  * @internalTechnology
  */
	{
	delete iNext;
	}

CDHCPState* CDHCPState::ProcessAckNakL(TRequestStatus* aStatus)
/**
  * Handle acknowledgement and negative acknowlegements
  *
  * @internalTechnology
  */
	{
	CDHCPStateMachine& rDHCP = Dhcp();
	return rDHCP.HandleReplyL( aStatus );
	}

void CDHCPState::Cancel()
/**
  * Cancel any pending outstanding request
  *
  * @internalTechnology
  */
	{
	}
	
		

#ifdef SYMBIAN_ESOCK_V3
SFactoryChannel::~SFactoryChannel()
	{
	if ( iC32Root.Handle() )
		{
		iC32Root.Close();
		}
	}

void SFactoryChannel::SendMessageL( NetMessages::CMessage& aQuery )
	{
	//connect to root server and find an instance
	if ( !iC32Root.Handle() )
		{
		User::LeaveIfError(iC32Root.Connect());
		}
	//send a message to find a channel instance
	TInt nLen = aQuery.Length();
	iBuf.Close();
	iBuf.CreateL( nLen );
	TPtr8 ptr( const_cast<TUint8*>(iBuf.Ptr()), iBuf.MaxLength() );
	User::LeaveIfError( aQuery.Store( ptr ) );
	iBuf.SetLength( ptr.Length() );
	//this should be very quick so we can afford to hold different interfaces up for a while
	//we should really have a solution for servers running each session in a different thread
	User::LeaveIfError( iC32Root.SendMessage( iModule, CommsFW::EFactoryMsg, ptr ) );
	TPtrC8 ptrC( ptr );
	User::LeaveIfError( aQuery.GetTypeId().Check( ptrC ) );
	User::LeaveIfError( aQuery.Load( ptrC ) );
	}

SDhcpSignal::~SDhcpSignal()
	{
	delete iQuery;
	delete iNetSubscribe;
	}

void SDhcpSignal::SubscribeL(const TName& aInterfaceName, TInt aEventId, NetSubscribe::TEvent& aEvent )
	{
	iModule.Copy( _L( "ESock_DIP" ) );
	if ( !iQuery )
		{
		__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("SDhcpSignal::SubscribeL NetMessages::CTypeIdQuery::NewL")));
		iQuery = NetMessages::CTypeIdQuery::NewL();
		iQuery->iUid = IPEvent::KFactoryImplementationUid;
		iQuery->iTypeId = IPEvent::KProtocolId;
		iQuery->iHandle = NULL;
		iQuery->iOid.Copy( aInterfaceName.Left( iQuery->iOid.MaxLength() ) );
		}
	if ( !iQuery->iHandle )
		{
		__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("SDhcpSignal::SubscribeL handle = 0")));
		SFactoryChannel::SendMessageL( *iQuery );

		STypeId typeId = STypeId::CreateSTypeId( NetSubscribe::KTransportUid, NetSubscribe::EPublishSubscribe );
		ASSERT( !iNetSubscribe );
		iNetSubscribe = NetSubscribe::CNetSubscribe::NewL( typeId );

		NetSubscribe::SSignalId signalId( IPEvent::KEventImplementationUid, aEventId, iQuery->iHandle );
		aEvent.SubscribeL(*iNetSubscribe, signalId);
		__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("SDhcpSignal::SubscribeL registered for signal handle = %d"), iQuery->iHandle));
		//and wait for the signal
		}
	else
		{//deregister & release the handle
		__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("SDhcpSignal::SubscribeL deregister & release the handle")));
		aEvent.Cancel(*iNetSubscribe);
		SFactoryChannel::SendMessageL( *iQuery );
		}
	}
#endif

CAsynchEvent* CDHCPAddressAcquisition::ProcessL(TRequestStatus& aStatus)
/**
  * Interface function, execute state machine
  *
  * @internalTechnology
  */
	{
    DHCP_DEBUG_PUBLISH_STATE(DHCPDebug::EDHCPIPAddressAcquisition);
	CDHCPStateMachine& rDHCP = Dhcp();
	rDHCP.InitialiseSocketL();

	rDHCP.CreateDiscoverMsgL();

	// one second retransmit passed to CloseNsendMsgL
	rDHCP.CloseNSendMsgL(KOneSecond,rDHCP.iMaxRetryCount,CDHCPStateMachine::EAllAvailableServers);
	TRequestStatus* p = &aStatus;
	User::RequestComplete(p, KErrNone); //move to the next state
	return iNext;
	}

CAsynchEvent* CDHCPSelect::ProcessL(TRequestStatus& aStatus)
/**
  * Interface function, execute state machine
  *
  * @internalTechnology
  */
	{
	DHCP_DEBUG_PUBLISH_STATE(DHCPDebug::EDHCPSelect);
	CDHCPStateMachine& rDHCP = Dhcp();
	rDHCP.iReceiving = EFalse;
	rDHCP.HandleOfferL();
	rDHCP.CreateOfferAcceptanceRequestMsgL();
	// one second initial retransmit passed to CloseNsendMsgL
	rDHCP.CloseNSendMsgL(KOneSecond, rDHCP.iMaxRetryCount, CDHCPStateMachine::EAllAvailableServers);
	TRequestStatus* p = &aStatus;
	User::RequestComplete(p, KErrNone); //move to the next state
	
	return iNext;
	}

CAsynchEvent* CDHCPInformationConfig::ProcessL(TRequestStatus& aStatus)
/**
  * Interface function, execute state machine
  *
  * @internalTechnology
  */
	{
	DHCP_DEBUG_PUBLISH_STATE(DHCPDebug::EDHCPInformationConfig);
	CDHCPStateMachine& rDHCP = Dhcp();
#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS	
	if (rDHCP.iDhcpInformAckPending)
		{
		rDHCP.BindSocketForUnicastL();
		}
	else
		{
#endif// SYMBIAN_NETWORKING_DHCP_MSG_HEADERS		
	rDHCP.InitialiseSocketL();
#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS	
		}
#endif // SYMBIAN_NETWORKING_DHCP_MSG_HEADERS			
	rDHCP.CreateInformMsgL();
#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS	
	if (rDHCP.iDhcpInformAckPending) //new condition added for dynamic dhcp inform message-For IPv4 only
		{
		rDHCP.CloseNSendMsgL(KOneSecond, DHCPv4::KReqMaxRetry,CDHCPStateMachine::EUnicast);
		}
	else
		{
#endif//SYMBIAN_NETWORKING_DHCP_MSG_HEADERS		
		// one second initial retransmit passed to CloseNsendMsgL
		rDHCP.CloseNSendMsgL(KOneSecond, rDHCP.iMaxRetryCount,CDHCPStateMachine::EAllAvailableServers);
#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
		}	
#endif//SYMBIAN_NETWORKING_DHCP_MSG_HEADERS		
	TRequestStatus* p = &aStatus;
	User::RequestComplete(p, KErrNone); //move to the next state
	return iNext;
	}

CAsynchEvent* CDHCPRebootConfirm::ProcessL(TRequestStatus& aStatus)
/**
  * Interface function, execute state machine
  *
  * @internalTechnology
  */
	{
	DHCP_DEBUG_PUBLISH_STATE(DHCPDebug::EDHCPRebootConfirm);
	CDHCPStateMachine& rDHCP = Dhcp();
	rDHCP.InitialiseSocketL();
	rDHCP.CreateRebootRequestMsgL();
	// one second retransmit passed to CloseNsendMsgL
	rDHCP.CloseNSendMsgL(KOneSecond, rDHCP.iMaxRetryCount,CDHCPStateMachine::EAllAvailableServers);
	TRequestStatus* p = &aStatus;
	User::RequestComplete(p, KErrNone); //move to the next state
	return iNext;
	}

CAsynchEvent* CDHCPRequest::ProcessL(TRequestStatus& aStatus)
/**
  * Interface function, execute state machine
  *
  * @internalTechnology
  */	
	{
	DHCP_DEBUG_PUBLISH_STATE(DHCPDebug::EDHCPRequest);
	CDHCPStateMachine& rDHCP = Dhcp();
	if (!rDHCP.iReceiving)
		{
		return rDHCP.ReceiveL(&aStatus);
		}
	return ProcessAckNakL(&aStatus);
	}

CAsynchEvent* CDHCPRebind::ProcessL(TRequestStatus& aStatus)
/**
  * Interface function, execute state machine
  *
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPRebind::ProcessL")));
	
	DHCP_DEBUG_PUBLISH_STATE(DHCPDebug::EDHCPRebind);
	CDHCPStateMachine& rDHCP = Dhcp();
	if (!rDHCP.iReceiving)
		{
		rDHCP.CreateRebindRequestMsgL();
		// the socket will be closed so as to free up resources...
		// therefore we will have to set it up again...
		rDHCP.InitialiseSocketL();
		// 10 second initial retransmit passed to CloseNsendMsgL
		rDHCP.CloseNSendMsgL(KOneSecond * 10/*10 secs*/,rDHCP.iMaxRetryCount, CDHCPStateMachine::EAllAvailableServers);
//		rDHCPIPv4.StartDeltaTimer(iTimeToWait, *this);
		return rDHCP.ReceiveL(&aStatus);
		}
	return ProcessAckNakL(&aStatus);
	}

void CDHCPWaitForDADBind::Cancel()
	{
	CDHCPStateMachine& rDHCP = Dhcp();
	rDHCP.CancelTimer();
	
	TRequestStatus* p = &iStateMachine->iStatus;
	User::RequestComplete(p, KErrCancel); 
	rDHCP.SetAsyncCancelHandler(NULL);
	}
	
CAsynchEvent* CDHCPWaitForDADBind::ProcessL(TRequestStatus& aStatus)
/**
  * Interface function, execute state machine
  *
  * @internalTechnology
  */
	{
	DHCP_DEBUG_PUBLISH_STATE(DHCPDebug::EDHCPWaitForDADBind);
	CDHCPStateMachine& rDHCP = Dhcp();
	aStatus = KRequestPending;
	rDHCP.SetAsyncCancelHandler(this);
	rDHCP.CancelMessageSender();
	iBoundAt.HomeTime();
	rDHCP.StartTimer(TTimeIntervalMicroSeconds32(KHalfSecondInMicroSeconds), *this); // 0.5 sec to wait for TCP/IP6 stack to complete DAD
	rDHCP.UpdateHistory(EBinding);
	iErr = KErrNone;
	return NULL; //no matter what that's the end
	}

void CDHCPWaitForDADBind::TimerExpired()
/**
  * Interface function, called by timer when timer has popped
  * in this case we try to bind a socket to the source address
  * to check that DAD has been resolved by the TCP/IP6 stack.
  * If it hasn't then we have to wait for another 2 seconds, 
  * up to a maximum of 30 seconds...
  * There is an improvement to this, by checking the status
  * of the route for the interface...no time to implement this
  * improvement, but it would be more robust to have it as
  * this would enable differentiation between when DAD has not been
  * resolved and when it has found a duplicate address...which the 
  * current method does not allow, as the bind will only return KErrNotFound
  * if the source address is not recognised...would still have to poll though...
  *
  * @internalTechnology
  */
	{
	CDHCPStateMachine& rDHCP = Dhcp();


#ifdef _DEBUG
	if (CDHCPServer::DebugFlags() & KDHCP_Dad)
		{
		iErr = KErrNotFound;	// simulate a duplicate address	
		// but we only want to do this once...
		// so reset the debug flag
		CDHCPServer::DebugFlags() &= ~KDHCP_Dad;
		}
	else
#endif
		{
		iErr = rDHCP.BindToSource();
		if (iErr!=KErrNone)
			{
			TTime now;
			now.HomeTime();
			TTimeIntervalSeconds thirtySeconds(30);
			if ((now-thirtySeconds)<iBoundAt)
				{
				rDHCP.StartTimer(TTimeIntervalMicroSeconds32(KHalfSecondInMicroSeconds), *this); //another 0.5 secs
				return; //wait
				}
			}
		}
	}

#if 0
void CDHCPWaitForDADIPNotifier::TimerExpired()
	{
	//finish => something's gone wrong => no response from IP notifier
	TRequestStatus* p = &iStateMachine->iStatus;
	User::RequestComplete(p, KErrTimedOut); 
	}
#endif

CAsynchEvent* CDHCPRenew::ProcessL(TRequestStatus& aStatus)
/**
  * Interface function, execute state machine
  *
  * @internalTechnology
  */
	{
	DHCP_DEBUG_PUBLISH_STATE(DHCPDebug::EDHCPRenew);
	CDHCPStateMachine& rDHCP = Dhcp();
	if (!rDHCP.iReceiving)
		{
		rDHCP.CreateRenewRequestMsgL();
		// the socket will be closed so as to free up resources...
		// therefore we will have to set it up again...
		rDHCP.BindSocketForUnicastL();
		// 10 second initial retransmit passed to CloseNsendMsgL
		rDHCP.CloseNSendMsgL(KOneSecond * 10/*10 secs*/, rDHCP.iMaxRetryCount, CDHCPStateMachine::EUnicast);
		return rDHCP.ReceiveL(&aStatus);
		}
	return ProcessAckNakL(&aStatus);
	}

CAsynchEvent* CDHCPRelease::ProcessL(TRequestStatus& aStatus)
/**
  * Interface function, execute state machine
  *
  * @internalTechnology
  */
	{
	DHCP_DEBUG_PUBLISH_STATE(DHCPDebug::EDHCPRelease);
	CDHCPStateMachine& rDHCP = Dhcp();
	rDHCP.CreateReleaseMsgL();
	rDHCP.InitialiseSocketL();
	// the socket will be closed so as to free up resources...
	// therefore we will have to set it up again...
	rDHCP.BindSocketForUnicastL();
	rDHCP.CloseNSendMsgL( aStatus, CDHCPStateMachine::EUnicast);
	return iNext;
	}



CAsynchEvent* CDHCPDecline::ProcessL(TRequestStatus& aStatus)
/**
  * Interface function, execute state machine
  *
  * @internalTechnology
  */
	{
	DHCP_DEBUG_PUBLISH_STATE(DHCPDebug::EDHCPDecline);
	CDHCPStateMachine& rDHCP = Dhcp();
	rDHCP.CreateDeclineMsgL();
	// need to open a new socket to broadcast a decline
	// as the old socket has been closed and then used to
	// try to bind to source address...
	rDHCP.InitialiseSocketL();
	rDHCP.CloseNSendMsgL(aStatus, CDHCPStateMachine::EAllAvailableServers);
	return iNext;
	}

#ifdef SYMBIAN_NETWORKING_DHCPSERVER	
CAsynchEvent* CDHCPWaitForClientMsgs::ProcessL(TRequestStatus& aStatus)
/**
  * Interface function, execute state machine
  *
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPWaitForClientMsgs::ProcessL")));
	
	DHCP_DEBUG_PUBLISH_STATE(DHCPDebug::EDHCPWaitForClientMsgs);
	CDHCPStateMachine& rDHCP = Dhcp();
	rDHCP.iReceiving = EFalse;
	rDHCP.InitialiseServerSocketL();
	rDHCP.ReceiveOnPort67L(&aStatus);

	return iNext;
	}
	
	
CAsynchEvent* CDHCPHandleClientMsgs::ProcessL(TRequestStatus&/* aStatus*/)
/**
  * Interface function, execute state machine
  *
  * @internalTechnology
  */
	{
	return iNext;
	}	
	

CAsynchEvent* CDHCPProvideOffer::ProcessL(TRequestStatus& aStatus)
/**
  * Interface function, execute state machine
  *
  * @internalTechnology
  */
	{
	DHCP_DEBUG_PUBLISH_STATE(DHCPDebug::EDHCPProvideOffer);
	CDHCPStateMachine& rDHCP = Dhcp();
	rDHCP.CreateOfferMsgL();
	rDHCP.CloseNSendServerMsgL(aStatus, CDHCPStateMachine::EAllAvailableServers);
	return iNext;
	}
	
CAsynchEvent* CDHCPSendRequestResponse::ProcessL(TRequestStatus& aStatus)
/**
  * Interface function, execute state machine
  *
  * @internalTechnology
  */
	{
	DHCP_DEBUG_PUBLISH_STATE(DHCPDebug::EDHCPSendAckNak);
	CDHCPStateMachine& rDHCP = Dhcp();
	rDHCP.HandleRequestMsgL();
	rDHCP.InitialiseServerSocketL();

	if(rDHCP.IsClientIdentified())
		{
		rDHCP.CloseNSendServerMsgL(aStatus, CDHCPStateMachine::EUnicast);
		rDHCP.iSvrSpecificState = ESvrRenewInProgress;		
		}
	else
		{
		rDHCP.CloseNSendServerMsgL(aStatus, CDHCPStateMachine::EAllAvailableServers);
		rDHCP.iSvrSpecificState = ESvrDiscoverInProgress;		
		}
	
	rDHCP.SetClientIdentified(ETrue);
	return iNext;
	}		
	
CAsynchEvent* CDHCPSendInformResponse::ProcessL(TRequestStatus& aStatus)
/**
  * Interface function, execute state machine
  *
  * @internalTechnology
  */
	{
	DHCP_DEBUG_PUBLISH_STATE(DHCPDebug::EDHCPSendAckNak);
	CDHCPStateMachine& rDHCP = Dhcp();

	rDHCP.HandleInformMsgL();
	rDHCP.InitialiseServerSocketL();

    //Sends DHCPAck only the Client NetworkId is same as the Server's NetworkId
	if(rDHCP.CheckNetworkId())
		{
		rDHCP.CloseNSendServerMsgL(aStatus, CDHCPStateMachine::EUnicast);
		rDHCP.SetClientIdentified(ETrue);	
		rDHCP.iSvrSpecificState = ESvrInformInProgress;	
		}
	else
		{
		TRequestStatus* p = &aStatus;
		User::RequestComplete(p, KErrNone);
		}	
	return iNext;
	}
	
CAsynchEvent* CDHCPHandleRelease::ProcessL(TRequestStatus& aStatus)
/**
  * Interface function, execute state machine
  *
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4StateMachine::ProcessReleaseL")));
	CDHCPStateMachine& rDHCP = Dhcp();
	
	rDHCP.SetClientIdentified(EFalse);
	rDHCP.iSvrSpecificState = ESvrReleaseInProgress;	
	
	TRequestStatus* p = &aStatus;
	User::RequestComplete(p, KErrNone); 
	return iNext;
	}
	
CAsynchEvent* CDHCPHandleDecline::ProcessL(TRequestStatus& aStatus)
/**
  * Interface function, execute state machine
  *
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4StateMachine::ProcessReleaseL")));
	CDHCPStateMachine& rDHCP = Dhcp();
	rDHCP.SetClientIdentified(EFalse);
	TRequestStatus* p = &aStatus;
	User::RequestComplete(p, KErrNone); 
	return iNext;
	}			

void CDHCPBindServer::Cancel()
	{
	CDHCPStateMachine& rDHCP = Dhcp();
	rDHCP.CancelTimer();
	
	TRequestStatus* p = &iStateMachine->iStatus;
	User::RequestComplete(p, KErrCancel); 
	rDHCP.SetAsyncCancelHandler(NULL);
	}	

CAsynchEvent* CDHCPBindServer::ProcessL(TRequestStatus& aStatus)
/**
  * Interface function, execute state machine
  *
  * @internalTechnology
  */
	{
	CDHCPStateMachine& rDHCP = Dhcp();
	aStatus = KRequestPending;
	rDHCP.SetAsyncCancelHandler(this);
	rDHCP.CancelMessageSender();

	TInt err = rDHCP.BindServerInterface();
	if(KErrNone == err)
		{
		TRequestStatus* p = &aStatus;
		User::RequestComplete(p, KErrNone);
		rDHCP.SetAsyncCancelHandler(NULL);
		}
	else
		rDHCP.StartTimer(TTimeIntervalMicroSeconds32(KHalfSecondInMicroSeconds), *this); // 0.5 sec to wait for TCP/IP6 stack to complete Bind

	iErr = KErrNone;
	return NULL; //no matter what that's the end
	}

void CDHCPBindServer::TimerExpired()
/**
  * Interface function, called by timer when timer has popped
  * in this case we try to bind the server socket to the static IP address from comms database.
  * If the bind fails then we have to wait for another 2 seconds, 
  * up to a maximu of 30 seconds...
  *
  * @internalTechnology
  */
	{
	CDHCPStateMachine& rDHCP = Dhcp();
	iErr = rDHCP.BindServerInterface();
	
	if(iErr != KErrNone)
		{
		rDHCP.StartTimer(TTimeIntervalMicroSeconds32(KHalfSecondInMicroSeconds), *this); // 0.5 sec to wait for TCP/IP6 stack to complete Bind
		}
	}
	
#endif // SYMBIAN_NETWORKING_DHCPSERVER			
