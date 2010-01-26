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
 @file DHCPIP6States.cpp
 @internalTechnology
*/

#include "DHCPIP6States.h"
#include "DhcpIP6Msg.h"
#include "DHCPIP6MsgSender.h"

#include "DHCPServer.h"
#include "DHCPStatesDebug.h"

using namespace DHCPv6;

#ifdef SYMBIAN_ESOCK_V3
CDHCPIP6ListenToNeighbor::~CDHCPIP6ListenToNeighbor()
	{
	if ( iNetSubscribe )
		{
		iEvent.Cancel(*iNetSubscribe);
		}
	}


	
	
	
CAsynchEvent* CDHCPIP6ListenToNeighbor::ProcessL(TRequestStatus& aStatus)
	{
	DHCP_DEBUG_PUBLISH_STATE(DHCPDebug::EDHCPIP6ListenToNeighbor);
	//(static_cast<TEvent*>(this)->*iHandler)();
	CDHCPIP6StateMachine& stmachine = DHCPIPv6();
	//set us as an error event as well so that we catch the leave and can deregister if needed
	stmachine.SetErrorEvent( this );
	stmachine.CancelTimer();
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6ListenToNeighbor::ProcessL err %d"), stmachine.LastError()));
	if ( iErr == KErrNone )
		{
		iErr = stmachine.LastError();
		if ( iErr == KErrNone || (iQuery && iQuery->iHandle) )
			{//we can get here only when we want to subscribe or unsubscribe
			SubscribeL( stmachine.InterfaceName(), IPEvent::EMFlagReceived, iEvent );
			}
		}
	if ( iErr == KErrNone && iQuery && iQuery->iHandle )
		{//start timer in case we never get the signal
		__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6ListenToNeighbor::ProcessL wait signal StartTimer")));
	
	
		// The 14 second timeout comes from the 3x4 seconds the IP stack waits for RouterAdvs once it's sent out
		// its 3 RouterSols ... plus a bit for processing. Search for iRtrSolicitationInterval in tcpip/src/iface.cpp
		stmachine.StartTimer( TTimeIntervalSeconds(14)/*seconds*/, *this );
			
		aStatus = KRequestPending;
		stmachine.SetAsyncCancelHandler(this);
		}
	else
		{//we're deregistered => start statemachine based on 'M' flag value
		ASSERT( this == DHCPIPv6().iFirstState );
	    CleanupStack::PushL(this);
		stmachine.iFirstState = NULL;
		stmachine.SetErrorEvent( NULL );

		if (iErr == KErrTimedOut)
			{
			TimerExpired();
			}
		else 
			{
			// RA received
			if ( iMFlag )
				{
				__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("MFlag true. - Proceeding with Stateful")));
				stmachine.StartInitL( NULL, CDHCPStateMachine::ESubsequentCalls );
				}
			else
				{
				if( iOFlag )
					{
					__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("MFlag false, OFlag True - stateful autoconfiguration to get non-IPv6-address information")));
					CompleteClientAndStartInform();		// complete client, as 'O' flag set - stateful autoconfiguration to get non-IPv6-address information
					}
				else 
					{
					BecomeIdle();	// 'M' and 'O' flags are false	
					}
				}
			}
		CleanupStack::PopAndDestroy(this);
		return stmachine.iFirstState;
		}
	stmachine.SetLastError( KErrNone ); //ignore the error
	return this;
	}

/*static*/
void CDHCPIP6ListenToNeighbor::SignalHandlerFn( TAny* aThis, const Meta::SMetaData* aData )
	{
	//Router Advt received, decide upon 'M' and 'O' flags
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6ListenToNeighbor::SignalHandlerFn()")));
	CDHCPIP6ListenToNeighbor* inst = reinterpret_cast<CDHCPIP6ListenToNeighbor*>(aThis);
	CDHCPIP6StateMachine& stmachine = inst->DHCPIPv6();
	inst->iMFlag = static_cast<const IPEvent::CMFlagReceived*>(aData)->GetMFlag();
	inst->iOFlag = static_cast<const IPEvent::CMFlagReceived*>(aData)->GetOFlag();
	stmachine.CancelTimer();
	TRequestStatus* p = &stmachine.iStatus;
	User::RequestComplete( p, KErrNone );
    stmachine.SetAsyncCancelHandler(NULL);
	}

void CDHCPIP6ListenToNeighbor::Cancel()
	{	
	CDHCPIP6StateMachine& stmachine = DHCPIPv6();
	stmachine.CancelTimer();
	TRequestStatus* p = &stmachine.iStatus;
   	User::RequestComplete( p, KErrCancel );
	stmachine.SetAsyncCancelHandler(NULL);
	}
		

void CDHCPIP6ListenToNeighbor::BecomeIdle()
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6ListenToNeighbor::BecomeIdle()")));
	CDHCPIP6StateMachine& stmachine = DHCPIPv6();
	TRequestStatus* p = &stmachine.iStatus;
	stmachine.SetIdle( ETrue );
   	User::RequestComplete( p, KErrNone );
   	stmachine.SetAsyncCancelHandler(NULL);
	}

void CDHCPIP6ListenToNeighbor::CompleteClientAndStartInform()
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6ListenToNeighbor::CompleteClientAndStartInform()")));
	CDHCPIP6StateMachine& stmachine = DHCPIPv6();
	TRequestStatus* p = &stmachine.iStatus;
	stmachine.SetCompleteClientRequestTrue();
   	User::RequestComplete( p, KErrNone );
   	stmachine.SetAsyncCancelHandler(NULL);
	}


void CDHCPIP6ListenToNeighbor::TimerExpired()
	{
	//start state-ful config since no signal received
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6ListenToNeighbor::TimerExpired()")));
	CDHCPIP6StateMachine& stmachine = DHCPIPv6();
	TRequestStatus* p = &stmachine.iStatus;
   	User::RequestComplete( p, KErrTimedOut );
   	stmachine.SetAsyncCancelHandler(NULL);
	}
#endif
/** our selection criteria so far
 *  -  the first advertise msg received or the one with the highest
 *     preference value
 *
 *  some smarter selection criteria for server advertisements:
 *  -  Within a group of Advertise messages with the same server
 *     preference value, a client MAY select those servers whose
 *     Advertise messages advertise information of interest to the
 *     client.  For example, the client may choose a server that returned
 *     an advertisement with configuration options of interest to the
 *     client.
 *
 *  -  The client MAY choose a less-preferred server if that server has a
 *     better set of advertised parameters, such as the available
 *     addresses advertised in IAs.
*/
CAsynchEvent* CDHCPIP6Solicit::ProcessL(TRequestStatus& aStatus)
   {
   DHCP_DEBUG_PUBLISH_STATE(DHCPDebug::EDHCPIP6Solicit);
   CDHCPIP6MessageSender* pSender = DHCPIPv6().MessageSender();
   //ask for a signal after the first RT elapses
   pSender->SetMaxRetryCount( 1 );
   pSender->SetMaxRetryTimeout( DHCPv6::KSolMaxRt );
   TInt n = KIP6MaxSecs;
   if( iUserDefinedTimeout != 0 )
	   	{
	   	n = iUserDefinedTimeout;
	   	}   	
   pSender->SetMaxRetryDuration( n );
   pSender->SetInitialRetryTimeout( DHCPv6::KSolTimeout );
   pSender->SetFirstSendDelay( DHCPv6::KSolMaxDelay );
   //here we know that select follows after solicit
   pSender->SetListener(static_cast<CDHCPIP6Select*>(iNext));
   return CDHCPAddressAcquisition::ProcessL( aStatus );
   }

void CDHCPIP6Select::SetMaxRetryCount(TInt aMaxRetryCount)
	{
	iMaxRetryCount = aMaxRetryCount;
	}

const TInt KAdvertOverridePref = 255;
CAsynchEvent* CDHCPIP6Select::ProcessL(TRequestStatus& aStatus)
   {//called when a message's been received or state's changed for us
   DHCP_DEBUG_PUBLISH_STATE(DHCPDebug::EDHCPIP6Select);
   CDHCPIP6StateMachine& rDHCP = DHCPIPv6();
   CDHCPIP6MessageSender* pSender = rDHCP.MessageSender();
   CDHCPMessageHeaderIP6* v6Msg = rDHCP.DhcpMessage();
	if(rDHCP.iReceiving || iDone)
		{	// we're waiting to receive data from the socket
		/*
		collect advertisements so long we are waiting for the first retransmition timeout.
		Keep the first one and check the others for
		preference option with a pref value of 255. If the client receives an Advertise message 
		that does not include a Preference option with a preference value of 255, the
		client continues to wait until the first RT elapses 
		if pSender->Notifier() != this than the first RT's elapsed => we're happy
		with any message that is a valid advertisement
		*/
     	rDHCP.iReceiving = EFalse;
		if(rDHCP.GetMessageType() == DHCPv6::EAdvertise)
			{	
			/* 
			examine the message and make decision based on the aforementioned 
         	selection criteria
         	*/         	
         DHCPv6::COptionNode* option = v6Msg->GetOptions().FindOption(DHCPv6::EPreference);
         	 TInt serverPreference;
	         if (option)
	         	{
	         	serverPreference = option->GetBigEndian();
	         	}
	         else
	         	{
	         	serverPreference = 0;
	         	}           	
         	if(pSender->EventListener() != this || iDone || serverPreference == KAdvertOverridePref)
					{
					// We're not interested in further notifications
					pSender->SetListener( &rDHCP );
					
					// Set-up sender, consume selected advertisement & initiate request
					// message
					pSender->SetMaxRetryCount(DHCPv6::KReqMaxRc);
					pSender->SetMaxRetryTimeout(DHCPv6::KReqMaxRt);
					pSender->SetMaxRetryDuration(KIP6MaxSecs);
					pSender->SetInitialRetryTimeout(DHCPv6::KReqTimeout);
					pSender->SetFirstSendDelay(0);
					
					if ( !iDone/*see CDHCPIP6Select::MSReportError*/ && iSelectedMessage.Length() )
						{
						// Copy the selected message back the max length must be enough since we've read
						//iSelectedMessage into the v6Msg buffer
            		v6Msg->Message().Des().Copy(iSelectedMessage);
						}
         
            	return CDHCPSelect::ProcessL(aStatus);
					}
				else
					{	// Work out if this advertisment's better for us
					
					if(serverPreference > iBestServerPreference )
						{
						iBestServerPreference = serverPreference;
						
						// Store a copy away for later
						TInt ret= iSelectedMessage.ReAlloc(v6Msg->Message().Length());
						if (ret == KErrNone)
							iSelectedMessage.Copy(v6Msg->Message());
						else
							User::LeaveIfError(ret);						
						}
					}
			}
		}
		
   	// wait for a message (next or the first)
	return rDHCP.ReceiveL(&aStatus);
   }

/*
	This method is called when the retransmission timer expires.
*/
TInt CDHCPIP6Select::MSReportError(TInt aError)
   {//called when the first RT's expired
   CDHCPIP6StateMachine& rDHCP = DHCPIPv6();
   CDHCPIP6MessageSender* pSender = rDHCP.MessageSender();
	if(aError == KErrTimedOut)
		{
		/*
		the first RT's elapsed so check the collected messages
		if we have one (the first one) we're done if not we start the sender again
		*/

		if (iBestServerPreference != -1)
			{
			pSender->Cancel();
			iDone = ETrue;
			rDHCP.CancelMessageReceiver();

			// Copy the selected message back the max length must be enough since we've read
			//iSelectedMessage into the v6Msg buffer
         rDHCP.DhcpMessage()->Message().Des().Copy(iSelectedMessage);
			
			return aError;
			}
		else
			{
			// We're not interested in further notifications
			pSender->SetListener(&rDHCP);
			pSender->SetMaxRetryCount(iMaxRetryCount);
			
			return KErrNone; // Causes the sender to continue
			}
		}
		
	// Something else's gone wrong => report it up
	pSender->SetListener(&rDHCP);
	
	return rDHCP.MSReportError(aError);
	}

CDHCPIP6Select::~CDHCPIP6Select()
	{
	iSelectedMessage.Close();
	}

CAsynchEvent* CDHCPIP6InformRequest::ProcessL(TRequestStatus& aStatus)
   {
   DHCP_DEBUG_PUBLISH_STATE(DHCPDebug::EDHCPIP6InformRequest);
   CDHCPIP6MessageSender* pSender = DHCPIPv6().MessageSender();
   pSender->SetMaxRetryCount( INT_MAX );
   pSender->SetMaxRetryTimeout( DHCPv6::KInfMaxRt );
   pSender->SetMaxRetryDuration( KIP6MaxSecs );
   pSender->SetInitialRetryTimeout( DHCPv6::KInfTimeout );
   pSender->SetFirstSendDelay( DHCPv6::KInfMaxDelay );
   return CDHCPInformationConfig::ProcessL( aStatus );
   }

CAsynchEvent* CDHCPIP6Release::ProcessL(TRequestStatus& aStatus)
   {
   DHCP_DEBUG_PUBLISH_STATE(DHCPDebug::EDHCPIP6Release);
   CDHCPIP6MessageSender* pSender = DHCPIPv6().MessageSender();
   pSender->SetMaxRetryCount( DHCPv6::KRelMaxRc );
   pSender->SetMaxRetryTimeout( KIP6MaxSecs );
   pSender->SetMaxRetryDuration( KIP6MaxSecs );
   pSender->SetInitialRetryTimeout( DHCPv6::KRelTimeout );
   pSender->SetFirstSendDelay( 0 );
   CDHCPRelease::ProcessL( aStatus ); //this fn doesn't actually complete status for v6
   iStateMachine->SetActiveEvent( iNext ); //to shift the state after the receiver is ready
	return DHCPIPv6().ReceiveL( &aStatus );
   }

CAsynchEvent* CDHCPIP6Decline::ProcessL(TRequestStatus& aStatus)
   {
   DHCP_DEBUG_PUBLISH_STATE(DHCPDebug::EDHCPIP6Decline);
   CDHCPIP6MessageSender* pSender = DHCPIPv6().MessageSender();
   pSender->SetMaxRetryCount( DHCPv6::KDecMaxRc );
   pSender->SetMaxRetryTimeout( KIP6MaxSecs );
   pSender->SetMaxRetryDuration( KIP6MaxSecs );
   pSender->SetInitialRetryTimeout( DHCPv6::KDecTimeout );
   pSender->SetFirstSendDelay( 0 );
   CDHCPDecline::ProcessL( aStatus );  //this fn doesn't actually complete status for v6
   iStateMachine->SetActiveEvent( iNext ); //to shift the state after the receiver is ready
	return DHCPIPv6().ReceiveL( &aStatus );
   }

CDHCPState* CDHCPIP6ReplyNoBinding::ProcessAckNakL(TRequestStatus* aStatus)
   {
   CDHCPIP6StateMachine& rDHCP = DHCPIPv6();
   if ( rDHCP.GetMessageType() == DHCPv6::EReply )
      {
      rDHCP.CancelMessageSender();
		rDHCP.iReceiving = EFalse;
		User::RequestComplete(aStatus, KErrNone);
      return static_cast<CDHCPState*>(iNext);
      }
   return rDHCP.ReceiveL( aStatus );
   }

CAsynchEvent* CDHCPIP6Confirm::ProcessL(TRequestStatus& aStatus)
   {
   DHCP_DEBUG_PUBLISH_STATE(DHCPDebug::EDHCPIP6Confirm);
   CDHCPIP6MessageSender* pSender = DHCPIPv6().MessageSender();
   pSender->SetMaxRetryCount( INT_MAX );
   pSender->SetMaxRetryTimeout( DHCPv6::KCnfMaxRt );
   pSender->SetMaxRetryDuration( DHCPv6::KCnfMaxRd );
   pSender->SetInitialRetryTimeout( DHCPv6::KCnfTimeout );
   pSender->SetFirstSendDelay( DHCPv6::KCnfMaxDelay );
   return CDHCPRebootConfirm::ProcessL( aStatus );
   }

#if 0
CAsynchEvent* CDHCPIP6WaitForDAD::ProcessL(TRequestStatus& aStatus)
	{
   DHCP_DEBUG_PUBLISH_STATE(DHCPDebug::EDHCPIP6WaitForDAD);
   CDHCPIP6StateMachine& rDHCP = DHCPIPv6();
	rDHCP.StartTimer(KOneSecond*20, *this); // 20 secs timer in case there's no response from IPNotifier.
	if (iAddressIndex == 1 && !CSubscribeChannel::ListenL(aStatus,event,rDHCP.C32Root()))
		{
		return this;
		}
	else 
		{
		//read channel data and if success then attempt to bind to verify the result

		if ( rDHCP.ConfigureInterface( iAddressIndex++ ) )
			{//set the next address as iCurrentAddress
			//and listen for DAD
			CSubscribeChannel::ListenL(aStatus,event,rDHCP.C32Root());
			return this;
			}
		}
	return iNext;
	}
#else
CAsynchEvent* CDHCPIP6WaitForDAD::ProcessL(TRequestStatus& aStatus)
	{
   DHCP_DEBUG_PUBLISH_STATE(DHCPDebug::EDHCPIP6WaitForDAD);
   CDHCPIP6StateMachine& rDHCP = DHCPIPv6();
	if (iAddressIndex == 0)
		{//start the first wait
		iAddressIndex++;
		CDHCPWaitForDADBind::ProcessL( aStatus );
		return this;
		}
	else if ( iErr != KErrNone )
		{//Mark the iAddressIndex as invalid and attempt to bind the next one
		rDHCP.iInterfaceConfigInfo.SetAddressStatus( iAddressIndex -1, DHCPv6::EMarkForDecline );
		rDHCP.ConfigureInterfaceL( iAddressIndex++ );
		//listen for DAD
		CDHCPWaitForDADBind::ProcessL( aStatus );
		return this;
		}
	else
		{//Mark all but iAddressIndex as invalid
		}
	TRequestStatus* p = &aStatus;
	User::RequestComplete(p, iErr);
	return iNext;
	}

void CDHCPIP6WaitForDAD::TimerExpired()
	{
	CDHCPWaitForDADBind::TimerExpired();
	CDHCPStateMachine& rDHCP = Dhcp();
	if ( !rDHCP.TimerActive() )
		{
		//finish either => bound or not => bouncwe back to CDHCPIP6WaitForDAD::ProcessL
		TRequestStatus* p = &iStateMachine->iStatus;
		User::RequestComplete(p, KErrNone); //proceed back to the process function
		rDHCP.SetAsyncCancelHandler(NULL);
		}
	}
#endif

CAsynchEvent* CDHCPIP6Renew::ProcessL(TRequestStatus& aStatus)
   {
   DHCP_DEBUG_PUBLISH_STATE(DHCPDebug::EDHCPIP6Renew);
   CDHCPIP6StateMachine& rDHCP = DHCPIPv6();
   CDHCPIP6MessageSender* pSender = rDHCP.MessageSender();
   pSender->SetMaxRetryCount( INT_MAX );
   pSender->SetMaxRetryTimeout( DHCPv6::KRenMaxRt );
   TInt n = rDHCP.iRebindTimeT2 - rDHCP.iRenewalTimeT1;
   if(iUserDefinedTimeout)
	   	{
	   	n = iUserDefinedTimeout;
	   	}
   else if ( n <= 0 )
	   	{
	   	n = 2; //2 seconds
	   	}   	
   pSender->SetMaxRetryDuration( n );
   pSender->SetInitialRetryTimeout( DHCPv6::KRenTimeout );
   pSender->SetFirstSendDelay( 0 );
   return CDHCPRenew::ProcessL( aStatus );
   }

CAsynchEvent* CDHCPIP6Rebind::ProcessL(TRequestStatus& aStatus)
   {
   DHCP_DEBUG_PUBLISH_STATE(DHCPDebug::EDHCPIP6Rebind);
   CDHCPIP6StateMachine& rDHCP = DHCPIPv6();
   CDHCPIP6MessageSender* pSender = rDHCP.MessageSender();
   pSender->SetMaxRetryCount( INT_MAX );
   pSender->SetMaxRetryTimeout( DHCPv6::KRebMaxRt );
   TInt n = rDHCP.iLeaseTime - rDHCP.iRebindTimeT2;
   if(iUserDefinedTimeout)
	   	{
	   	n = iUserDefinedTimeout;
	   	}
   else if ( n <= 0 )
	   	{
	   	n = 2; //2 seconds
	   	}   
   pSender->SetMaxRetryDuration( n );
   pSender->SetInitialRetryTimeout( DHCPv6::KRebTimeout );
   pSender->SetFirstSendDelay( 0 );
   return CDHCPRebind::ProcessL( aStatus );
   }

CAsynchEvent* CDHCPIP6Reconfigure::ProcessL(TRequestStatus& aStatus)
    {
	DHCP_DEBUG_PUBLISH_STATE(DHCPDebug::EDHCPIP6Reconfigure);
	CDHCPIP6StateMachine& rDHCP = DHCPIPv6();
	if (!rDHCP.iReceiving)
	    {
        rDHCP.InitialiseSocketL();
	    }
	return CDHCPRequest::ProcessL(aStatus);
    }

CDHCPState* CDHCPIP6Reconfigure::ProcessAckNakL(TRequestStatus* aStatus)
   {//check for reconfigure reply msg
   CDHCPIP6StateMachine& rDHCP = DHCPIPv6();
   if ( rDHCP.GetMessageType() == DHCPv6::EReconfigure )
      {
		rDHCP.iReceiving = EFalse;
		User::RequestComplete(aStatus, KErrNone);
      return static_cast<CDHCPState*>(iNext);
      }
   return rDHCP.ReceiveL( aStatus );
   }
