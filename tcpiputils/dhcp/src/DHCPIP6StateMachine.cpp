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
// Implements the DHCP IP6 statemachine helper functions
// 
//

/**
 @file DHCPIP6StateMachine.cpp
 @internalTechnology
*/

#include "DHCPIP6States.h"
#include "DhcpIP6MsgRcvr.h"
#include "DHCPServer.h"

#ifdef _DEBUG
#include <e32property.h>
#endif

using namespace DHCPv6;

CDHCPIP6StateMachine::~CDHCPIP6StateMachine()
/**
  * Destructor of the If base class
  *
  * @internalTechnology
  *
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6StateMachine::~CDHCPIP6StateMachine")));
   delete iMessageReader;
   iServerId.Close();
	}

#ifdef SYMBIAN_NETWORKING_ADDRESS_PROVISION
CDHCPIP6StateMachine* CDHCPIP6StateMachine::NewL(RSocketServ& aEsock, RConnection& aConnection, const TName& aInterfaceName, CDhcpHwAddrManager* aDhcpHwAddrManager)
/**
  * Creates a new instance of this class
  *
  * @internalTechnology
  *
  */
	{
	CDHCPIP6StateMachine* stateMachine = new (ELeave) CDHCPIP6StateMachine(aEsock, aConnection, aInterfaceName, aDhcpHwAddrManager);
	CleanupStack::PushL(stateMachine);
	stateMachine->ConstructL();
	CleanupStack::Pop(stateMachine);
	return stateMachine;
	}
#else
CDHCPIP6StateMachine* CDHCPIP6StateMachine::NewL(RSocketServ& aEsock, RConnection& aConnection, const TName& aInterfaceName)
/**
  * Creates a new instance of this class
  *
  * @internalTechnology
  *
  */
	{
	CDHCPIP6StateMachine* stateMachine = new (ELeave) CDHCPIP6StateMachine(aEsock, aConnection, aInterfaceName);
	CleanupStack::PushL(stateMachine);
	stateMachine->ConstructL();
	CleanupStack::Pop(stateMachine);
	return stateMachine;
	}
#endif //SYMBIAN_NETWORKING_ADDRESS_PROVISION
void CDHCPIP6StateMachine::ConstructL()
/**
  * Creates socket and connections for the object
  *
  *
  *	@internalTechnology
  *
  */
	{
   CDHCPStateMachine::ConstructL();

	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6StateMachine::ConstructL")));
	
  	ReAllocL(KDhcpInitMsgSizeIP6);
  	iDhcpMessage = new(ELeave)CDHCPMessageHeaderIP6(iFragment);
	iMessageReader = new(ELeave)CDhcpIP6MessageReader( *this );
	iMessageSender = new(ELeave)CDHCPIP6MessageSender(this,iSocket,&iTaskStartedAt,KAfInet6);
	}

void CDHCPIP6StateMachine::GetServerAddress( TInetAddr& aAddress )
{
	iInterfaceConfigInfo.GetServerAddress( aAddress );
}

void CDHCPIP6StateMachine::SetCurrentAddress(const TInetAddr& aCurrentAddress, const TInetAddr& /*aSubnetMask*/)
   {
   CDHCPStateMachine::SetCurrentAddress( aCurrentAddress );
   }

void CDHCPIP6StateMachine::StartInitL(MStateMachineNotify* aStateMachineNotify, EInitialisationContext aInitialisationContext, TInt aUserTimeOut )
/**
  * StartInitL
  *
  * This function is called to start the IP address solicitation state machine
  * we don't support rapid commit option for now
  *
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6StateMachine::StartSolicitationL")));
	ASSERT(!iFirstState);//the states should be deleted by now or we are starting off
	
	SetIdle( EFalse );
	iInterfaceConfigInfo.Reset();

#ifdef SYMBIAN_ESOCK_V3
	if ( aInitialisationContext == CDHCPStateMachine::EFirstCall )
		{
		iFirstState = new(ELeave) CDHCPIP6ListenToNeighbor(*this);
		}
	else
#else
	(void)aInitialisationContext;
#endif
		{
		iFirstState = new(ELeave) CDHCPIP6Solicit(*this, aUserTimeOut);
		CDHCPState* state1 = new(ELeave)CDHCPIP6Select(*this);
		iFirstState->SetNext( state1 );
		CDHCPState* state2 = new(ELeave) CDHCPIP6Request(*this);
		state1->SetNext( state2 );
		state2->SetNext( new(ELeave) CDHCPIP6WaitForDAD(*this) );

		if (iStartInitCalls == CDHCPStateMachine::EFirstCall)
			{
			//This is the first time we're going to wait for a DCHP server. 
			//If there is no server it we will complete the client after one attempt 
			iStartInitCalls = aInitialisationContext;
			static_cast<CDHCPIP6Select*>(state1)->SetMaxRetryCount(1);
			}
		else
			{
			//We never found a DCHP server the first time, but this time try harder.
			static_cast<CDHCPIP6Select*>(state1)->SetMaxRetryCount(INT_MAX);
			}
		}

	CDHCPStateMachine::Start(aStateMachineNotify);
	}
#ifdef SYMBIAN_TCPIPDHCP_UPDATE
void CDHCPIP6StateMachine::StartInformL(MStateMachineNotify* aStateMachineNotify)
/**
  * This function is called to send inform message if options are not 
  * found in iValidMsg buffer
  *
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6StateMachine::StartInformL Dynamic Inform Request")));
	ASSERT(!iFirstState);
	iFirstState = new(ELeave) CDHCPIP6InformRequest(*this);
	CDHCPState* request = new(ELeave) CDHCPIP6Request(*this);
	iFirstState->SetNext( request );
	CDHCPStateMachine::Start(aStateMachineNotify);
	}
#endif //SYMBIAN_TCPIPDHCP_UPDATE
void CDHCPIP6StateMachine::StartInformL(MStateMachineNotify* aStateMachineNotify, TBool /*aStaticAddress*/)
/**
  * StartInformL
  *
  * This function is called to start the inform for state-less configuration
  *
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6StateMachine::EInformRequest")));
	ASSERT(!iFirstState); //the states should be deleted by now or we are starting off

    if (!iServerId.Length())
        {
	    iInterfaceConfigInfo.Reset();
        }

	iFirstState = new(ELeave) CDHCPIP6InformRequest(*this);
   CDHCPState* state1 = new(ELeave) CDHCPIP6Request(*this);
   iFirstState->SetNext( state1 );
	//static address is set but no wait for DAD here
	
	CDHCPStateMachine::Start(aStateMachineNotify);
	iCfgInfoOnly = ETrue;
	}

#ifdef SYMBIAN_NETWORKING_DHCPSERVER

void CDHCPIP6StateMachine::CreateOfferMsgL()
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6StateMachine::CreateOfferMsgL")));	
	}
	
void CDHCPIP6StateMachine::HandleRequestMsgL()//CreateRequestResponseMsgL()
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6StateMachine::CreateRequestResponseMsgL")));	
	}
	
void CDHCPIP6StateMachine::HandleInformMsgL()
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6StateMachine::HandleInformMsgL")));		
	}	

void CDHCPIP6StateMachine::PrepareToSendServerMsgL( CDHCPStateMachine::EAddressType /*aEAddressType*/)
/**
  * PrepareToSendL
  *
  * This function is called to set the correct destination address
  * for messages before they are sent.
  *
  * @internalTechnology
  */	
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6StateMachine::PrepareToSendL")));
	DhcpMessage()->Dump();
	iMessageSender->Cancel();
	}

void CDHCPIP6StateMachine::CloseNSendServerMsgL(TRequestStatus& /*aStatus*/, CDHCPStateMachine::EAddressType /*aEAddressType*/)
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6StateMachine::CloseNSendServerMsgL")));			
	}

CDHCPState* CDHCPIP6StateMachine::ReceiveOnPort67L( TRequestStatus* aStatus )
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6StateMachine::ReceiveOn67")));
	
	iDhcpMessage->InitialiseL();
   	iMessageReader->SetNext( iActiveEvent );
   	ASSERT( iReceiving == EFalse );
   	iReceiving = ETrue;
   	return static_cast<CDHCPState*>(iMessageReader->ProcessL( *aStatus ));
	}

void CDHCPIP6StateMachine::InitialiseServerSocketL()
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6StateMachine::InitialiseServerSocketL")));	
	}	

#endif // SYMBIAN_NETWORKING_DHCPSERVER

void CDHCPIP6StateMachine::StartRebootL(MStateMachineNotify* aStateMachineNotify)
/**
  * StartRebootL
  *
  * This function is called to start the confirm request state machine
  * used when trying to start using a previously assigned address still
  * with valid lease.
  *
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6StateMachine::StartConfirmL")));
	ASSERT(!iFirstState);//the states should be deleted by now or we are starting off

   iFirstState = new(ELeave) CDHCPIP6Confirm(*this);
   CDHCPState* state1 = new(ELeave) CDHCPIP6Request(*this);
   iFirstState->SetNext( state1 );
   state1->SetNext( new(ELeave) CDHCPIP6WaitForDAD(*this) );

	CDHCPStateMachine::Start(aStateMachineNotify);
	}

void CDHCPIP6StateMachine::StartRenewL(MStateMachineNotify* aStateMachineNotify,TInt aUserTimeOut)
   {
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6StateMachine::StartRenewL")));
	ASSERT(!iFirstState);

   iUserRebindTimeout = aUserTimeOut;
   iFirstState = new(ELeave) CDHCPIP6Renew(*this,aUserTimeOut);
   iFirstState->SetNext( new(ELeave) CDHCPIP6WaitForDAD(*this) );

	CDHCPStateMachine::Start(aStateMachineNotify);	
   }

void CDHCPIP6StateMachine::StartRebindL(MStateMachineNotify* aStateMachineNotify)
/**
  * StartRebindL
  *
  * This function is called to start the rebind state machine
  *
  * @internalTechnology
  */	
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6StateMachine::StartRebindL")));
	ASSERT(!iFirstState);//the states should be deleted by now or we are starting off
	
	iFirstState = new(ELeave) CDHCPIP6Rebind(*this,iUserRebindTimeout);
   iFirstState->SetNext( new(ELeave) CDHCPIP6WaitForDAD(*this) );

	CDHCPStateMachine::Start(aStateMachineNotify);
	}

void CDHCPIP6StateMachine::StartReconfigureL(MStateMachineNotify* aStateMachineNotify)
/**
  * StartReconfigureL
  *
  * This function is called to initialise waiting for the server reconfigure message.
  * The task ends once a valid reconfigure messageg's been received
  *
  * @internalTechnology
  */
{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6StateMachine::StartReconfigureL")));

	ASSERT(!iFirstState);//the states should be deleted by now or we are starting off
	
	iXid.SetXid( 0 );
	iFirstState = new(ELeave) CDHCPIP6Reconfigure(*this);

	CDHCPStateMachine::Start(aStateMachineNotify);
}

void CDHCPIP6StateMachine::StartDeclineL(MStateMachineNotify* aStateMachineNotify)
/**
  * StartDeclineL
  *
  * This function is called to start the decline task
  *
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6StateMachine::StartDeclineL")));
	ASSERT(!iFirstState);//the states should be deleted by now or we are starting off
	
	if ( iInterfaceConfigInfo.AnyAddressToDecline() )
		{
		iFirstState = new(ELeave) CDHCPIP6Decline(*this);
   	iFirstState->SetNext( new CDHCPIP6ReplyNoBinding(*this) );	
		}
  	CDHCPStateMachine::Start(aStateMachineNotify);
	}

void CDHCPIP6StateMachine::StartReleaseL(MStateMachineNotify* aStateMachineNotify)
/**
  * StartReleaseL
  *
  * This function is called to start the release state machine
  *
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6StateMachine::StartReleaseL")));
	ASSERT(!iFirstState);//the states should be deleted by now or we are starting off
	
	iFirstState = new(ELeave) CDHCPIP6Release(*this);
   iFirstState->SetNext( new(ELeave)CDHCPIP6ReplyNoBinding(*this) );	
	
	CDHCPStateMachine::Start(aStateMachineNotify);
	}

void CDHCPIP6StateMachine::PrepareToSendL( CDHCPStateMachine::EAddressType /*aEAddressType*/ )
/**
  * PrepareToSendL
  *
  * This function is called to set the correct destination address
  * for messages before they are sent.
  *
  * @internalTechnology
  */	
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6StateMachine::PrepareToSendL")));
	DhcpMessage()->Dump();
	iMessageSender->Cancel();
	}

void CDHCPIP6StateMachine::CloseNSendMsgL(TRequestStatus& /*aStatus*/, CDHCPStateMachine::EAddressType aEAddressType)
/**
  * Handles sending of packets for this object
  *
  * @internalTechnology
  *
  */
	{
	CDHCPStateMachine::CloseNSendMsgL(0,KInfinity,aEAddressType); //IPv6 calculates its own delay and retry values..so KInfinity is ignored
	}

void CDHCPIP6StateMachine::CancelMessageReceiver()
   {//doesn't cancel statemachine only the reader state
   if ( IsActive() && iReceiving )
      {
      ASSERT( !iErrorEvent );
   	CStateMachine::Cancel(KErrNone);//waits for the cancel to complete
      iActiveEvent = iMessageReader->Next();
      iReceiving = EFalse;
      //restart the state machine
      CStateMachine::Start(NULL, NULL, iStateMachineNotify);
      }
   }

CDHCPState* CDHCPIP6StateMachine::ReceiveL(TRequestStatus* aStatus)
/**
  * Starts iMessageReader state
  *
  * @internalTechnology
  *
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6StateMachine::Receive")));
	
	iDhcpMessage->InitialiseL();
   iMessageReader->SetNext( iActiveEvent );
   ASSERT( iReceiving == EFalse );
   iReceiving = ETrue;
   return static_cast<CDHCPState*>(iMessageReader->ProcessL( *aStatus ));
	}

void CDHCPIP6StateMachine::OnCompletion()
{
   if (LastError() == KErrEof && iReceiving)
      {//end of datagram while reading a message => not an error => restart state machine
      iActiveEvent = iMessageReader->Next();
      
      CStateMachine::Start(NULL, NULL, iStateMachineNotify); //sets last error to KErrNone
      }
   else
      {
      CDHCPStateMachine::OnCompletion();
      }
}

void CDHCPIP6StateMachine::AssembleClientIDsL()
/**
  * Sets the client id - hardware address and type pair
  * 
  *
  */
	{
   /*client id and the way we generate it
     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |               3               |    hardware type (16 bits)    |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    .                                                               .
    .             link-layer address (variable length)              .
    .                                                               .
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   */
	FetchHWAddress();

	TUint htype = 0;
	if(iHardwareAddr.Family() != KAFUnspec)
		{
		iClientId.CreateL(KDuidTypeLen + KDuidHardwareTypeLen + iHardwareAddr.Length() - KHwAddrOffset);
		htype = iHardwareAddr.Family();		// should this be Port() ?
		}
	else 
		{
    	// This means we're on a point-to-point link (i.e. one with no hardware addressing).
    	//  We'll make client id look like an ethernet/mac address combo to reduce the chance of stupid servers getting upset
		__CFLOG_VAR((KLogSubSysDHCP, KLogCode,
						_L8("CDHCPIP6StateMachine::AssembleClientIDsL - no hardware address (suggesting point-to-point)"
						     " so generating a random 6 byte client id")));
		iClientId.CreateL(KDuidTypeLen + KDuidHardwareTypeLen + KDuidEthMacAddrSize);
		htype = 1;		// fake up as Ethernet
		}
		
	TBigEndian::SetValue(const_cast<TUint8*>(iClientId.Ptr()), KDuidTypeLen, KDuidLLTypeCode);	
	TBigEndian::SetValue(const_cast<TUint8*>(iClientId.Ptr()) + KDuidTypeLen, KDuidHardwareTypeLen, htype);
	iClientId.SetLength(KDuidTypeLen + KDuidHardwareTypeLen);
	
	if(iHardwareAddr.Family()!=KAFUnspec)
		{
	    iClientId.Append(iHardwareAddr.Mid(KHwAddrOffset, iHardwareAddr.Length() - KHwAddrOffset));
		}
	else 
		{
		// Note: if you increase the number of bytes appended here, then be sure to
		// increase the size passed to iClientId.CreateL() above.
    	TUint32 data = Math::Random();
		iClientId.Append(TPckgC<TUint32>(data));
		iClientId.Append(TPckgC<TUint16>(data));
		}
	}

TUint CDHCPIP6StateMachine::GetMessageType() const
/**
  * Parses the message and returns the message type from the DHCP message
  * i.e. whether the message is an advertise. reply, reconfigure
  *
  * @internalTechnology 
  */
	{
	CDHCPMessageHeaderIP6* v6Msg = DhcpMessage();
	if ( v6Msg->Parse(iXid,iClientId,iServerId) == KErrNone )
      {
	   v6Msg->Dump();	
   	return v6Msg->GetMessageType();
      }
   return EUnknown;
	}

void CDHCPIP6StateMachine::SetMessageHeaderL( TMessageType aMsgType )
/**
  * Sets the message header
  *
  * @internalTechnology
  */
	{
	iXid.Init6(); //generate random transaction Id
	iTaskStartedAt.HomeTime();	// set time stamp to now
	CDHCPMessageHeaderIP6* v6Msg = DhcpMessage();
	v6Msg->InitialiseL();
	v6Msg->SetXid(iXid.Xid());
	v6Msg->SetMessageType(static_cast<TUint8>(aMsgType));
	
	// +++++++++++++++++++++++ Elapsed Time +++++++++++++++++++++++++++++++++++++++++/	
	//EElapsedTime option MUST be the first option in the option part of the message
	//see CDHCPIP6MessageSender::SendingContinues fn
   v6Msg->AddOptionL(EElapsedTime, KElapsedTimeOptionLen)->SetBigEndian( 0 );
	// +++++++++++++++++++++++ Client Id +++++++++++++++++++++++++++++++++++++++++/	
	v6Msg->AddOptionL(EClientId, iClientId.Length())->GetBodyDes().Copy(iClientId);
	// +++++++++++++++++++++++ Server ID +++++++++++++++++++++++++++++++++++++++++/
   //included only when it exists
   if ( iServerId.Length() )
      {
      v6Msg->AddOptionL(EServerId, iServerId.Length())->GetBodyDes().Copy(iServerId);
      }
	}

void CDHCPIP6StateMachine::CreateDiscoverMsgL()
/**
  * Puts the specifics of the solicit message into
  * the message buffer
  *
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6StateMachine::CreateDiscoverMsgL")));
	
	iServerId.Close();
	//initiate message header
	SetMessageHeaderL(ESolicit);
	
	CDHCPMessageHeaderIP6* v6Msg = DhcpMessage();

	// +++++++++++++++++++++++ IA_NA/TA options+++++++++++++++++++++++++++++++++++/
	iInterfaceConfigInfo.AppendIAOptionsL(*v6Msg, ESolicit);
	
	// +++++++++++++++++++++++ reconfiguration accept ++++++++++++++++++++++++++++/
#if defined(DHCP_RECONFIGURE_NO_AUTHENTICATION)
	v6Msg->AddOptionL(EReconfAccept, 0);
#endif

	// +++++++++++++++++++++++ append requested options list +++++++++++++++++++++/
	static_cast<CDHCPOptionRequestOption*>(v6Msg->AddOptionL( EOro, KDHCPOptionRequestLen ))->AppendRequestedOptions();
	}

void CDHCPIP6StateMachine::CreateOfferAcceptanceRequestMsgL()
/**
  * Puts the specifics of the request
  * message into the message buffer
  *
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6StateMachine::CreateOfferAcceptanceRequestMsgL")));
	
	SetMessageHeaderL( ERequest );
	CDHCPMessageHeaderIP6* v6Msg = DhcpMessage();
	
	// +++++++++++++++++++++++ IA_NA/TA options+++++++++++++++++++++++++++++++++++/
   iInterfaceConfigInfo.AppendIAOptionsL( *v6Msg, ERequest );
	// +++++++++++++++++++++++ reconfiguration accept ++++++++++++++++++++++++++++/
#if defined(DHCP_RECONFIGURE_NO_AUTHENTICATION)
   v6Msg->AddOptionL(EReconfAccept, 0);
#endif
	// +++++++++++++++++++++++ append requested options list +++++++++++++++++++++/
   static_cast<CDHCPOptionRequestOption*>(v6Msg->AddOptionL( EOro, KDHCPOptionRequestLen ))->AppendRequestedOptions();
   
	}

void CDHCPIP6StateMachine::CreateRebootRequestMsgL()
/**
  * Puts the specifics of a confirm specific after reboot or other means of 
  * changing the interface into the message buffer 
  *
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6StateMachine::CreateRebootRequestMsgL")));
	
	iServerId.Close();
	SetMessageHeaderL( EConfirm );
	CDHCPMessageHeaderIP6* v6Msg = DhcpMessage();
	
	// +++++++++++++++++++++++ IA_NA/TA options+++++++++++++++++++++++++++++++++++/
   iInterfaceConfigInfo.AppendIAOptionsL( *v6Msg, EConfirm );
	// +++++++++++++++++++++++ IA_TA +++++++++++++++++++++++++++++++++++++++++++++/
   //iInterfaceConfigInfo.Init6ialiseNA_Option( *v6Msg->AddOptionL( EIaTa, KDHCPOptionIA_TATInitLength ) );
	// +++++++++++++++++++++++ append requested options list +++++++++++++++++++++/
   static_cast<CDHCPOptionRequestOption*>(v6Msg->AddOptionL( EOro, KDHCPOptionRequestLen ))->AppendRequestedOptions();
   
	}

void CDHCPIP6StateMachine::CreateInformMsgL()
/**
  * Puts the specifics of an inform-request message
  * into the message buffer
  *
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6StateMachine::CreateInformMsgL")));
	 
	SetMessageHeaderL( EInformationRequest );
	CDHCPMessageHeaderIP6* v6Msg = DhcpMessage();
#ifdef SYMBIAN_TCPIPDHCP_UPDATE	
	//This is used for multiple parameter option request 
	if (iSavedExtraParameters.Ptr())
		{
		TUint8 requestedOptions[KDHCPOptionRequestLen] = {0,EServerId, 0,EIaNa, 0,ESipServerD, 0,ESipServerA,
																0,EDNSServers, 0,EDomainList};

		TPtr8 ptr(requestedOptions, KDHCPOptionRequestLen, KDHCPOptionRequestLen );
		
		RBuf8 appendOpCodeList;
		appendOpCodeList.CreateL(ptr);
		TInt oplength = appendOpCodeList.Length();
		//get the no of opcode to be fetched
		TInt optLen= iSavedExtraParameters.Length();
		//Reallocate the opcodelist with the new opcodes(example if the client requests 
		//for 2options in the multiple opcode then the new memory will be allocated for 2 opcodes(2*2= 4 as it is 16bit))
		appendOpCodeList.ReAllocL(KDHCPOptionRequestLen+(optLen*2));
		for (TUint8 i=1;i<=optLen;i++)
			{
			//append null at the begining of the opcode always
			appendOpCodeList.Append(TChar::EAlphaGroup);
			//get the opcode number and append at the end
			TPtr8 ptrs= iSavedExtraParameters.RightTPtr(i);
			appendOpCodeList.Append(ptrs.Ptr(),1);
			}
		//add the required opcodes in the addoption 
		static_cast<CDHCPOptionRequestOption*>(v6Msg->AddOptionL(EOro, appendOpCodeList.Length()))->GetBodyDes().Copy(appendOpCodeList);
		appendOpCodeList.Close();
		}
#endif //SYMBIAN_TCPIPDHCP_UPDATE		
	// +++++++++++++++++++++++ append requested options list +++++++++++++++++++++/
   static_cast<CDHCPOptionRequestOption*>(v6Msg->AddOptionL( EOro, KDHCPOptionRequestLen ))->AppendRequestedOptions();
	}

void CDHCPIP6StateMachine::CreateRenewRequestMsgL()
/**
  * Puts the specifics of the renew message
  * into the mesage buffer
  *
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6StateMachine::CreateRenewRequestMsgL")));
	
	SetMessageHeaderL( ERenew );
	CDHCPMessageHeaderIP6* v6Msg = DhcpMessage();
	
	// +++++++++++++++++++++++ IA_NA/TA options+++++++++++++++++++++++++++++++++++/
   iInterfaceConfigInfo.AppendIAOptionsL( *v6Msg, ERenew );
	// +++++++++++++++++++++++ IA_TA +++++++++++++++++++++++++++++++++++++++++++++/
   //iInterfaceConfigInfo.Init6ialiseNA_Option( *v6Msg->AddOptionL( EIaTa, KDHCPOptionIA_TATInitLength ) );
	// +++++++++++++++++++++++ append requested options list +++++++++++++++++++++/
	
   static_cast<CDHCPOptionRequestOption*>(v6Msg->AddOptionL( EOro, KDHCPOptionRequestLen ))->AppendRequestedOptions();
	}	

void CDHCPIP6StateMachine::CreateRebindRequestMsgL()
/**
  * Puts the specifics of the rebind message
  * into the message buffer
  * 
  * @internalTechnology
  */
   {
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6StateMachine::CreateRebindRequestMsgL")));
	
	SetMessageHeaderL( ERebind );
	CDHCPMessageHeaderIP6* v6Msg = DhcpMessage();
	
	iInterfaceConfigInfo.ResetUseUnicast();
	
	// +++++++++++++++++++++++ IA_NA/TA options+++++++++++++++++++++++++++++++++++/
   iInterfaceConfigInfo.AppendIAOptionsL( *v6Msg, ERebind );
	// +++++++++++++++++++++++ IA_TA +++++++++++++++++++++++++++++++++++++++++++++/
   //iInterfaceConfigInfo.Init6ialiseNA_Option( *v6Msg->AddOptionL( EIaTa, KDHCPOptionIA_TATInitLength ) );
	// +++++++++++++++++++++++ append requested options list +++++++++++++++++++++/
   static_cast<CDHCPOptionRequestOption*>(v6Msg->AddOptionL( EOro, KDHCPOptionRequestLen ))->AppendRequestedOptions();
   }

void CDHCPIP6StateMachine::CreateReleaseMsgL()
/**
  * Puts the specifics of the release message
  * into the message buffer
  * 
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6StateMachine::CreateReleaseMsgL")));
	
	SetMessageHeaderL( DHCPv6::ERelease );
	CDHCPMessageHeaderIP6* v6Msg = DhcpMessage();

	iInterfaceConfigInfo.SetAddressStatus( DHCPv6::KAddrIndexAll, DHCPv6::EMarkForRelease );
    
	// +++++++++++++++++++++++ IA_NA/TA options+++++++++++++++++++++++++++++++++++/
   iInterfaceConfigInfo.AppendIAOptionsL( *v6Msg, DHCPv6::ERelease );

    RemoveConfiguredAddress();
	}

void CDHCPIP6StateMachine::CreateDeclineMsgL()
/**
  * Puts the specifics of the decline message
  * into the message buffer
  *
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6StateMachine::CreateDeclineMsgL")));
	
	SetMessageHeaderL( EDecline );
	CDHCPMessageHeaderIP6* v6Msg = DhcpMessage();

	// +++++++++++++++++++++++ IA_NA/TA options+++++++++++++++++++++++++++++++++++/
   iInterfaceConfigInfo.AppendIAOptionsL( *v6Msg, EDecline );
	
	}

void CDHCPIP6StateMachine::HandleOfferL()
/**
  * Handles the selected advertisement from a dhcp server, providing
  * an offer of configuration parameters
  *
  * @see CDHCPIP6Select
  * @internalTechnology
  *
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6StateMachine::HandleOffer")));
	
	CDHCPMessageHeaderIP6* v6Msg = DhcpMessage();
	//remember the address of the server and check for unicast option
	iInterfaceConfigInfo.CheckForUnicast( *v6Msg );
	}

CDHCPState* CDHCPIP6StateMachine::HandleReplyL( TRequestStatus* aStatus )
/**
  * Called after Request, Confirm, Renew, Rebind or
  * Information-request message
  *
  * @internalTechnology
  *
  */
{
   iReceiving = EFalse;
   
   CDHCPState* state = NULL;

   if ( GetMessageType() != EReply )
    {
		return ReceiveL(aStatus);
    }

    switch ( DhcpMessage()->GetStatusCode() )
        {
         case ESuccess:
				HandleAckL();
                User::RequestComplete(aStatus, KErrNone);
	            state = static_cast<CDHCPState*>(iActiveEvent->Next());
                
                // No need to notify server status - the TCP/IP6 stack will
                // move straight to the address assigned state when we call
                // ConfigureL...

		 break;
 
         case EUnspecFail:
         case ENoAddrsAvail:
         case ENoBinding:
         case ENotOnLink:
				User::RequestComplete(aStatus, KErrAccessDenied);
 
                state = NULL;
         break;

         case EUseMulticast:
            {
                iInterfaceConfigInfo.ResetUseUnicast();
                //return to the previous state and use multicast
                CAsynchEvent* p = iFirstState;
                while ( p->Next() != iActiveEvent )
                   {
                   p = p->Next();
                   }
                state =static_cast<CDHCPState*>(p);
            }
         break;
           
            		 
         default:
			state = ReceiveL(aStatus);
         break;
         
         };// switch
	

    return state;
}

void CDHCPIP6StateMachine::HandleAckL()
/**
  * Handles a succesfull reply from a dhcp server, storing
  * configuration parameters and a committed ip address.
  * Checks status code for IA(s) and IP address options and leaves
  * with KErrAccessDenied if no address is avaiable.
  *
  * @internalTechnology
  *
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6StateMachine::HandleAck")));
	
	CDHCPMessageHeaderIP6* v6Msg = DhcpMessage();
	//get addresses out of IA options
	iInterfaceConfigInfo.ParseIAOptionsL(*v6Msg);
	//check unicast option & server address
	iInterfaceConfigInfo.CheckForUnicast(*v6Msg);

	//check name servers from msg options
	if( iNameServerAddressesFromServer )
		{
		CDHCPOptionDNSServers* pServers = static_cast<CDHCPOptionDNSServers*>(v6Msg->GetOptions().FindOption( EDNSServers ));
		if ( pServers )
			{
			pServers->GetDomainNameServer( 0, iNameServer1 );
			pServers->GetDomainNameServer( 1, iNameServer2 );
			}
		}
	//get a host name somehow?
	//still draft only draft-ietf-dhc-dhcpv6-fqdn-00.txt with option-code      OPTION_CLIENT_FQDN (TBD)

	#ifdef _LOG
		TBuf<39> addrStr;
		
		interfaceInfo.iAddress.Output( addrStr );
		
		__CFLOG_1( KLogSubSysDHCP, KLogCode, _L( "DHCP client address is %S" ), &addrStr );
	#endif

	ConfigureInterfaceL( 0 );
	}

void CDHCPIP6StateMachine::ConfigureInterfaceL( TInt aPos )
/**
  * Set the interface IP address and other params
  * into the TCP/IP6 stack.
  * Picks next valid address starting from given possition
  *
  * What we set depends on the setup
  * in commDB for the service.  If ipAddressFromServer
  * is true then we set the ip address that has
  * been assigned by DHCP, along with the netmask and gateway.
  * If ipAddressFromServer is false, then we set the static ip
  * address as long as it has been okayed by the DHCP server after
  * we have sent an inform.  We will then set the netmask and gateway
  * choosing from those in commDB if they have been given values, or 
  * those returned in the DHCP Server ACK if a zero address is in commDB
  * The same principle applies to DNS Server addresses.
  *
  * @param aPos possition to start in the address list
  * @see SIdentityAssociationConfigInfo
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6StateMachine::ConfigureInterfaceL - Cancel Message Sender - aPos %d"), aPos));
	iMessageSender->Cancel();
	
	if (!IsGettingCfgInfoOnly())
		{
		const SIPAddressInfo* info = iInterfaceConfigInfo.GetValidAddressInfo( aPos );
		if ( !info )
			{//no more addresses to configure
			User::Leave( KErrNotFound );
			}
		//the values already checked on TInterfaceConfigInfo::ParseIAOptionsL
		iRenewalTimeT1 = iInterfaceConfigInfo.RenewTime();
		iRebindTimeT2 = iInterfaceConfigInfo.RebindTime();
		iLeaseTime = info->iValidLifeTime;
		
		// This block caters for the following 2 cases:
        //  1. Server sent 0 lease time, meaning "forever". This behaviour wasn't defined in the 
        //     original RFC but many servers work this way so we need to support it.
        //  2. Server sent extremely large lease time (e.g. 0xffffffff as specified in RFC).
        //     As TTimeIntervalSeconds has a maximum of 0x7fffffff, we must enforce this maximum.
        //
        if ( iLeaseTime == 0 || iLeaseTime > KReallyLongLease )
            {
            iLeaseTime = KReallyLongLease;  // 68 years should be long enough
            }
        
		if ( (iRenewalTimeT1 == iRebindTimeT2) || (iRenewalTimeT1==0 && iRebindTimeT2==0) )
            {
            // may have only been provided with a lease time...
            // we need time to renew the lease before it runs out
            // so we'd better set some times from the overall lease time
            TUint32 temp = iLeaseTime/4;
            iRenewalTimeT1=iLeaseTime/2;
            iRebindTimeT2=iRenewalTimeT1+temp;      
            }
		else if (iRenewalTimeT1<iRebindTimeT2 && iRebindTimeT2<iLeaseTime && iRenewalTimeT1<iLeaseTime)
            {
            // do nothing as we have got the valid values we were expecting
            }
        else
            {
            User::Leave(KErrArgument);
            }   
		
		iCurrentAddress.SetAddress( info->iAddress.Ip6Address() );
		}
	else if ( aPos != 0 )
		{//no more addresses to configure only one statically configured address
		User::Leave( KErrNotFound );
		}

	TSoInet6InterfaceInfo interfaceInfo;
   //fill interfaceInfo current address is iCurrentAddress
	interfaceInfo.iHwAddr = iHardwareAddr;
	interfaceInfo.iAddress = iCurrentAddress;
//	interfaceInfo.iNetMask = iSubnetMask;
	interfaceInfo.iDefGate = iDefGateway;
	if (iNameServerAddressesFromServer)
		{
		interfaceInfo.iNameSer1 = iNameServer1;
		interfaceInfo.iNameSer2 = iNameServer2;
		}
	else
		{
		//We need to set the family to KAFUnspec to ensure that the Stack does not overwrite the existing address
		interfaceInfo.iNameSer1.SetFamily(KAFUnspec);
		interfaceInfo.iNameSer2.SetFamily(KAFUnspec);
		}
	interfaceInfo.iName = iInterfaceName;
	interfaceInfo.iMtu = 0;
	interfaceInfo.iSpeedMetric = 0;
	interfaceInfo.iFeatures = 0; // zero value better than junk value
	
	interfaceInfo.iState = EIfUp;

	interfaceInfo.iDelete = EFalse;
	interfaceInfo.iAlias = EFalse;
	interfaceInfo.iDoId = ETrue;
	interfaceInfo.iDoState = ETrue;
	interfaceInfo.iDoAnycast = EFalse;
	interfaceInfo.iDoProxy = EFalse;
	interfaceInfo.iDoPrefix = EFalse;
	
	CDHCPStateMachine::ConfigureInterfaceL( interfaceInfo );
	}

void CDHCPIP6StateMachine::InitialiseSocketL()
/**
  * Sets up socket, by opening one associated with the connection
  * and sets the interface to use for traffic
  *
  * @internalTechnology
  *
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6StateMachine::InitialiseSocketL")));
	
	iSocket.Close();
	
	User::LeaveIfError(iSocket.Open(iEsock, KAfInet, KSockDatagram, KProtocolInetUdp, iConnection));
	User::LeaveIfError(iSocket.SetOpt(KSoReuseAddr, KSolInetIp, 1));
#ifdef _DEBUG
	TInt destPort;
	RProperty::Get(KMyPropertyCat, KMyPropertyDestPortv6, destPort);
	User::LeaveIfError(iSocket.SetLocalPort(destPort - 1));
#else
	User::LeaveIfError(iSocket.SetLocalPort(KDhcpv6SrcPort));
#endif	
	TPckgBuf<TSoInetIfQuery> query;
	query().iName = iInterfaceName;
	User::LeaveIfError(iSocket.GetOpt(KSoInetIfQueryByName, KSolInetIfQuery, query));
	User::LeaveIfError(iSocket.SetOpt(KSoInterfaceIndex, KSolInetIp, query().iIndex));
	User::LeaveIfError(iSocket.SetOpt(KSoInetEnumInterfaces, KSolInetIfCtrl));
	// make socket invisable for interface counting
	User::LeaveIfError(iSocket.SetOpt(KSoKeepInterfaceUp, KSolInetIp, 0));
	}

void CDHCPIP6StateMachine::RemoveConfiguredAddress(const TInetAddr* /* aInetAddr*/)
/**
  * This function can be called as a result of DAD failing
  * or the lease expiring! It removes the address from the interface
  * inside the TCP/IP6 stack as the address cannot continue to be used.
  *
  * @see "Implementation of IPv4/IPv6 Basic Socket API for Symbian OS"
  * document for explanation of TSoInet6InterfaceInfo and its use
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6StateMachine::RemoveConfiguredAddress")));
	
	TSoInet6InterfaceInfo interfaceInfo;
	interfaceInfo.iHwAddr = iHardwareAddr;
	interfaceInfo.iAddress = iCurrentAddress;
	interfaceInfo.iName = iInterfaceName;
	interfaceInfo.iDelete = ETrue;
	interfaceInfo.iAlias = EFalse;
	interfaceInfo.iDoId = ETrue;
	interfaceInfo.iState = EIfUp;
	interfaceInfo.iDoState = ETrue;
	interfaceInfo.iDoAnycast = EFalse;
	// zero value better than junk value
	interfaceInfo.iMtu = 0;
	interfaceInfo.iSpeedMetric = 0;
	interfaceInfo.iFeatures = 0; 
	CDHCPStateMachine::RemoveConfiguredAddress( interfaceInfo );
    iInterfaceConfigInfo.Reset();
	}

void CDHCPIP6StateMachine::AssignAddresses( TInetAddr& aDest, const TInetAddr& aSrc ) const
	{
	aDest = aSrc;

#ifdef _DEBUG
	// Simulate initialisation, renewal or rebind failure by using the wrong port.
	if( ( CDHCPServer::DebugFlags() & KDHCP_FailDiscover ) || ( CDHCPServer::DebugFlags() & KDHCP_FailRenew ) || ( CDHCPServer::DebugFlags() & KDHCP_FailRebind ) )
		{
		aDest.SetPort(KDhcpv6WrongSrcPort);
		}
	else
		{
		TInt destPort;
		RProperty::Get(KMyPropertyCat, KMyPropertyDestPortv6, destPort);
		aDest.SetPort(destPort - 1);
		}
#else
	aDest.SetPort(KDhcpv6SrcPort);
#endif
	}

void CDHCPIP6StateMachine::BindSocketForUnicastL()
/**
  * Initialises socket in case it hasn't already been initialised
  *
  * @internalTechnology
  */
	{
    __CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6StateMachine::BindSocketForRenewL")));
    UpdateHistory(CDHCPState::EBindToSource);
    if (!iSocket.SubSessionHandle())
        {
        InitialiseSocketL();
        }
	}
