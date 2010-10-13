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
// Implements the DHCP IP4 statemachine helper functions
// 
//

/**
 @file DHCPIP4StateMachine.cpp
 @internalTechnology
*/
#include <babitflags.h>
#include <e32math.h>
#include "DHCPIP4States.h"
#include "DHCPServer.h"
#ifndef SYMBIAN_COMMS_REPOSITORY
#include <commdb.h>
#else
#include <metadatabase.h>
#include <commsdattypesv1_1.h>
#endif

#ifdef SYMBIAN_NETWORKING_ADDRESS_PROVISION
#include "dhcphwaddrmanager.h"
#endif //SYMBIAN_NETWORKING_ADDRESS_PROVISION
using namespace DHCPv4;
#ifdef SYMBIAN_COMMS_REPOSITORY
using namespace CommsDat;
#endif

#ifdef SYMBIAN_NETWORKING_DHCPSERVER
const TUint16 KBroadCastFlagMask = 0x8000;
#endif

CDHCPIP4StateMachine::~CDHCPIP4StateMachine()
/**
  * Destructor of the If base class
  *
  * @internalTechnology
  *
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4StateMachine::~CDHCPIP4StateMachine")));
	}
	
#ifndef SYMBIAN_NETWORKING_DHCPSERVER
CDHCPIP4StateMachine* CDHCPIP4StateMachine::NewL(RSocketServ& aEsock, RConnection& aConnection, const TName& aInterfaceName)
/**
  * Creates a new instance of this class
  *
  * @internalTechnology
  *
  */
	{
	CDHCPIP4StateMachine* stateMachine = new (ELeave) CDHCPIP4StateMachine(aEsock, aConnection, aInterfaceName);
	CleanupStack::PushL(stateMachine);
	stateMachine->ConstructL();
	CleanupStack::Pop(stateMachine);
	return stateMachine;
	}
#endif // SYMBIAN_NETWORKING_DHCPSERVER

void CDHCPIP4StateMachine::ConstructL()
/**
  * Creates socket and connections for the object
  *
  *
  *	@internalTechnology
  *
  */
	{
   CDHCPStateMachine::ConstructL();

	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4StateMachine::ConstructL")));
	
  	ReAllocL(KDhcpMaxMsgSizeIP4);
  	iDhcpMessage = new(ELeave)CDHCPMessageHeaderIP4(iFragment);
	iMessageSender = new(ELeave)CMessageSender(this,iSocket,&iTaskStartedAt,KAfInet);

	}

void CDHCPIP4StateMachine::StartInitL(MStateMachineNotify* aStateMachineNotify, EInitialisationContext /*aInitialisationContext*/, TInt /*aUserTimeOut*/)
/**
  * StartInitL
  *
  * This function is called to start the init state machine
  *
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4StateMachine::StartInitL")));
	ASSERT(!iFirstState);
	
	iFirstState = new(ELeave) CDHCPIP4Init(*this);
	CDHCPState* select = new(ELeave) CDHCPIP4Select(*this);
	iFirstState->SetNext( select );
	CDHCPState* request = new(ELeave) CDHCPIP4Request(*this);
	select->SetNext( request );
	request->SetNext( new(ELeave) CDHCPIP4WaitForDAD(*this) );

	CDHCPStateMachine::Start(aStateMachineNotify);
	}

void CDHCPIP4StateMachine::StartInformL(MStateMachineNotify* aStateMachineNotify, TBool /*aStaticAddress*/)
/**
  * StartIniformL
  *
  * This function is called to start the inform static address state machine
  *
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4StateMachine::StartInformL")));
	ASSERT(!iFirstState);

	iFirstState = new(ELeave) CDHCPIP4Inform(*this);
	CDHCPState* request = new(ELeave) CDHCPIP4Request(*this);
   iFirstState->SetNext( request );
   request->SetNext( new(ELeave) CDHCPIP4WaitForDAD(*this) );
	
	CDHCPStateMachine::Start(aStateMachineNotify);
	iCfgInfoOnly = ETrue;
	}

#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
void CDHCPIP4StateMachine::StartInformL(MStateMachineNotify* aStateMachineNotify)
/**
  * This function is called to send inform message if options are not 
  * found in iValidMsg buffer
  *
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4StateMachine::StartInformL Dynamic Inform Request")));
	ASSERT(!iFirstState);

	iFirstState = new(ELeave) CDHCPIP4Inform(*this);
	CDHCPState* request = new(ELeave) CDHCPIP4Request(*this);
	iFirstState->SetNext( request );
	CDHCPStateMachine::Start(aStateMachineNotify);

	}
#endif // SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
void CDHCPIP4StateMachine::StartRebootL(MStateMachineNotify* aStateMachineNotify)
/**
  * StartRebootL
  *
  * This function is called to start the reboot request state machine
  * used when trying to start using a previously assigned address still
  * with valid lease.
  *
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4StateMachine::StartRebootL")));
	ASSERT(!iFirstState);

   iFirstState = new(ELeave) CDHCPIP4Reboot(*this);
	CDHCPState* request = new(ELeave) CDHCPIP4Request(*this);
   iFirstState->SetNext( request );
   request->SetNext( new(ELeave) CDHCPIP4WaitForDAD(*this) );

	CDHCPStateMachine::Start(aStateMachineNotify);
	}

void CDHCPIP4StateMachine::StartRenewL(MStateMachineNotify* aStateMachineNotify,TInt /*aUserTimeOut*/)
/**
  * StartRenewL
  *
  * This function is called to start the renew of lease state machine
  *
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4StateMachine::StartRenew")));
	ASSERT(!iFirstState);

  	iFirstState = new(ELeave) CDHCPIP4Renew(*this);
   iFirstState->SetNext( new(ELeave) CDHCPIP4WaitForDAD(*this) );

   CDHCPStateMachine::Start(aStateMachineNotify);
	}

void CDHCPIP4StateMachine::StartRebindL(MStateMachineNotify* aStateMachineNotify)
/**
  * StartRebindL
  *
  * This function is called to start the rebind state machine
  *
  * @internalTechnology
  */	
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4StateMachine::StartRebindL")));
	ASSERT(!iFirstState);
	
	iFirstState = new(ELeave) CDHCPIP4Rebind(*this);
   iFirstState->SetNext( new(ELeave) CDHCPIP4WaitForDAD(*this) );

	CDHCPStateMachine::Start(aStateMachineNotify);
	}

void CDHCPIP4StateMachine::StartDeclineL(MStateMachineNotify* aStateMachineNotify)
/**
  * StartDeclineL
  *
  * This function is called to start the decline state machine
  *
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4StateMachine::StartDeclineL")));
	ASSERT(!iFirstState);
	
	iFirstState = new(ELeave) CDHCPIP4Decline(*this);
	
	CDHCPStateMachine::Start(aStateMachineNotify);
	}

void CDHCPIP4StateMachine::StartReleaseL(MStateMachineNotify* aStateMachineNotify)
/**
  * StartReleaseL
  *
  * This function is called to start the release state machine
  *
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4StateMachine::StartReleaseL")));
	ASSERT(!iFirstState);
	
	iFirstState = new(ELeave) CDHCPIP4Release(*this);
	//iFirstState->SetNext( new(ELeave) CDHCPRemoveConfiguredAddress(*this));
	
	CDHCPStateMachine::Start(aStateMachineNotify);
	}

CDHCPState* CDHCPIP4StateMachine::ReceiveL(TRequestStatus* aStatus)
/**
  * Posts receive from on socket
  *
  * @note The iDHCPServerAddr in param 2 of the socket receivefrom
  * is written into by the socket to say where the received message
  * came from.
  * @internalTechnology
  *
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4StateMachine::Receive")));
	
	DhcpMessage()->InitialiseL();
  	iIncomingMsgDataPtr.Set(iDhcpMessage->Message().Des());
   ASSERT( iReceiving == EFalse );
 	iReceiving = ETrue;
	iSocket.RecvFrom(iIncomingMsgDataPtr, iDHCPServerAddr, 0, *aStatus);
   return static_cast<CDHCPState*>(iActiveEvent);
	}

#ifdef SYMBIAN_NETWORKING_DHCPSERVER
#ifdef SYMBIAN_NETWORKING_ADDRESS_PROVISION
CDHCPIP4StateMachine* CDHCPIP4StateMachine::NewL(RSocketServ& aEsock, RConnection& aConnection, const TName& aInterfaceName,  CDhcpHwAddrManager* aDhcpHwAddrManager, TBool aDHCPServerImpl)
#else
CDHCPIP4StateMachine* CDHCPIP4StateMachine::NewL(RSocketServ& aEsock, RConnection& aConnection, const TName& aInterfaceName,TBool aDHCPServerImpl)
#endif //SYMBIAN_NETWORKING_ADDRESS_PROVISION
/**
  * Creates a new instance of this class
  *
  * @internalTechnology
  *
  */
	{
#ifdef SYMBIAN_NETWORKING_ADDRESS_PROVISION
	CDHCPIP4StateMachine* stateMachine = new (ELeave) CDHCPIP4StateMachine(aEsock, aConnection, aInterfaceName, aDhcpHwAddrManager, aDHCPServerImpl);
#else
	CDHCPIP4StateMachine* stateMachine = new (ELeave) CDHCPIP4StateMachine(aEsock, aConnection, aInterfaceName,aDHCPServerImpl);
#endif //SYMBIAN_NETWORKING_ADDRESS_PROVISION	
	CleanupStack::PushL(stateMachine);
	stateMachine->ConstructL();
	CleanupStack::Pop(stateMachine);
	return stateMachine;
	}

TUint8 CDHCPIP4StateMachine::GetClientMessageTypeL() const
/**
  * Returns the message type from the DHCP message
  * i.e. whether the message is an offer, ack, nak...
  *
  * @internalTechnology 
  */
	{
	CDHCPMessageHeaderIP4* v4Msg = DhcpMessage();
	v4Msg->ParseClientMsgL();
	v4Msg->Dump();	

	return v4Msg->iOptions.GetMessageType();
	}

	
CDHCPState* CDHCPIP4StateMachine::ReceiveOnPort67L(TRequestStatus* aStatus)
/**
  * Posts receive from on socket
  *
  * @note The iDHCPClientAddr in param 2 of the socket receivefrom
  * is written into by the socket to say where the received message
  * came from.
  * @internalTechnology
  *
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4StateMachine::ReceiveOnPort67L")));
	
	DhcpMessage()->InitialiseL();
  	iIncomingMsgDataPtr.Set(iDhcpMessage->Message().Des());
	ASSERT( iReceiving == EFalse );
 	iReceiving = ETrue;
	iSvrSocket.RecvFrom(iIncomingMsgDataPtr, iDHCPClientAddr, 0, *aStatus);
	return static_cast<CDHCPState*>(iActiveEvent);
	}
	
void CDHCPIP4StateMachine::PrepareToSendServerMsgL( CDHCPStateMachine::EAddressType aEAddressType)
/**
  * PrepareToSendServerMsgL
  *
  * This function is called to set the correct destination address
  * for messages before they are sent.
  *
  * @internalTechnology
  */	
	{
	// nothing is done with iClientId in server implementation
	// but it is passed to reuse the existing function.
	DhcpMessage()->FinishL(iClientId); 
	DhcpMessage()->Dump();
	if (aEAddressType==EUnicast)
		{
		if(iBroadCastFlag)
			{
			__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4StateMachine::SetDestination - broadcast")));
			iDHCPClientAddr.SetV4MappedAddress(KInetAddrBroadcast);		
			}
		else
			{
			__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4StateMachine::SetDestination - unicast")));
			iDHCPClientAddr.SetV4MappedAddress(iYiaddr);// client ip address
			}
		}
	else
		{
		__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4StateMachine::SetDestination - broadcast")));
		iDHCPClientAddr.SetV4MappedAddress(KInetAddrBroadcast);	
		}

// While DHCP server listens on a custom port, the server message for client
// needs to be the custom server port + 1 and not the default client port (i.e., 68)
#ifdef _DEBUG
	iDHCPClientAddr.SetPort(GetDestPort()+1);
#else
	iDHCPClientAddr.SetPort(KDhcpClientPort);
#endif
	}
	
void CDHCPIP4StateMachine::GetClientAddress( TInetAddr& aAddress )
/**
  * GetClientAddress
  *
  * Gets the assigned client address
  *
  * @internalTechnology	
*/
	{
	aAddress = iDHCPClientAddr;
	}

TUint32 CDHCPIP4StateMachine::GenerateClientIPAddress()
/**
  * GenerateClientIPAddress
  *
  * Generate valid IP address to be offered to the client
  *
  *@internalTechnology	
*/
	{
	// get the server IP address	
	TUint32 serverAddr = iCurrentAddress.Address();
	TUint32 hostId = (serverAddr & ~KInetAddrNetMaskC) + 1;
	if (hostId >= 255)
	 {
	 hostId = 1;
	 }
	return (serverAddr & KInetAddrNetMaskC) | hostId;
	}
	
void CDHCPIP4StateMachine::SetMessageHeaderAsServerL( TDHCPv4MessageType aMsgType )
/**
  * SetMessageHeaderAsServerL
  *
  * Fills in the DHCP header information to be sent to the client
  *
  * @internalTechnology
  */
	{
	TTime now;
	TTimeIntervalSeconds secs;
	now.HomeTime();	
	(void)now.SecondsFrom(iTaskStartedAt, secs);

	CDHCPMessageHeaderIP4* v4Msg = DhcpMessage();
	v4Msg->InitialiseL();
	v4Msg->SetSecs(static_cast<TUint16>(secs.Int()));	// time field is 16 bits, time elapsed will be less than 16 bits, hence no data loss
	v4Msg->SetXid(iXid.Xid());
	// set to indicate we are using DHCP server implementation
	v4Msg->iDHCPServerImpl = ETrue; 
	v4Msg->SetCIAddr(iCiaddr);
	// +++++++++++++++++++++ Message Type (1 byte) ++++++++++++++++++++++++++++++++/
	v4Msg->AddOptionL(EDHCPMessageType, 1)->SetBigEndian(aMsgType);
	
	switch(aMsgType)
		{
		case EDHCPOffer:
		case EDHCPAck:
			// ++++++++Subnet mask, Router and Server IP set with server address ++++++++++/			
			v4Msg->AddOptionL(EDHCPSubnetMask,KIp4AddrByteLength)->SetBigEndian(KInetAddrNetMaskC); // 255.255.255.0
			v4Msg->AddOptionL(EDHCPRouter,KIp4AddrByteLength)->SetBigEndian(iCurrentAddress.Address());
			v4Msg->AddOptionL(EDHCPServerID,KIp4AddrByteLength)->SetBigEndian(iCurrentAddress.Address());	
			break;

		case EDHCPNak:
			// ++++++++Server IP set with server address ++++++++++/
			v4Msg->AddOptionL(EDHCPServerID,KIp4AddrByteLength)->SetBigEndian(iCurrentAddress.Address());	
			break;
		}
	
	TUint8 htype,hlen;
	//  fill in something "traditional" so as not to upset a stupid server.
	//  This is a best guess based on lack of specs in this area
	htype = 1; // ethernet
	hlen = 6; // mac address length
	v4Msg->SetCHAddr(iClientHWAddr);

	v4Msg->SetHeader(EDHCPBootReply,  htype /*hardware addr type*/,  hlen /*hardware addr length*/,  0 /*hops*/);
	v4Msg->SetFlags(iFlag);
#ifdef _DEBUG
	if (CDHCPServer::DebugFlags() & KDHCP_RequestIP4BroadcastOffer)
		{
		v4Msg->SetFlags(0x8000); // force broadcast
		}
#endif
	}
			
#endif // SYMBIAN_NETWORKING_DHCPSERVER

void CDHCPIP4StateMachine::PrepareToSendL( CDHCPStateMachine::EAddressType aEAddressType )
/**
  * PrepareToSendL
  *
  * This function is called to set the correct destination address
  * for messages before they are sent.
  *
  * @internalTechnology
  */	
	{
#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
	if (iDhcpInformAckPending)
		{
		DhcpMessage()->FinishL(iClientId,&iSavedExtraParameters);
		}
	else
		{
#endif // SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
	DhcpMessage()->FinishL(iClientId);
#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
		}	
#endif // SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
	DhcpMessage()->Dump();
	if (aEAddressType==EUnicast)
		{
		__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4StateMachine::SetDestination - unicast")));
		iDHCPServerAddr.SetV4MappedAddress(iServerAddress);
		}
	else
		{
		__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4StateMachine::SetDestination - broadcast")));
		iDHCPServerAddr.SetV4MappedAddress(KInetAddrBroadcast);	
		}
#ifndef _DEBUG
	iDHCPServerAddr.SetPort(KDhcpDestPort);
#else
	// Simulate initialisation, renewal or rebind failure by using the wrong port.
	if( ( CDHCPServer::DebugFlags() & KDHCP_FailDiscover ) || ( CDHCPServer::DebugFlags() & KDHCP_FailRenew ) || ( CDHCPServer::DebugFlags() & KDHCP_FailRebind ) )
		{
		iDHCPServerAddr.SetPort(KDhcpWrongDestPort);
		}
	else
		{
		iDHCPServerAddr.SetPort(GetDestPort());
		}
#endif
	}


void CDHCPIP4StateMachine::AssembleClientIDsL()
/**
  * Sets the client id - this just has to be a cookie for the server to use for indexing.
  * 
  *   On an addressed link (e.g. ethernet), hardware address type/value pair is a good combo to use
  *      as it mirrors a few other popular implementations..
  */
	{
    FetchHWAddress();

    iClientId.CreateL(KMaxSockAddrSize);
    iClientId.SetLength(1);		// set length to 1 byte

	if(iHardwareAddr.Family()!=KAFUnspec)
		{
		// we're on a link with hardware addressing e.g. ethernet
		__CFLOG_VAR((KLogSubSysDHCP, KLogCode,
						_L8("CDHCPIP4StateMachine::AssembleClientIDsL - hardware address type=%d, length=%d"),
						iHardwareAddr.Port(), iHardwareAddr.Length() - KHwAddrOffset));

		iClientId[0] = static_cast<TUint8>(iHardwareAddr.Port());	// fill in the hwAddrType
	    iClientId.Append(iHardwareAddr.Mid(KHwAddrOffset, iHardwareAddr.Length() - KHwAddrOffset));
		}
	else // family was unspecified meaning no hardware address
    	{
    	// This means we're on a point-to-point link (i.e. one with no hardware addressing).
    	//  We'll make client id look like an ethernet/mac address combo to reduce the chance of stupid servers getting upset
		__CFLOG_VAR((KLogSubSysDHCP, KLogCode,
						_L8("CDHCPIP4StateMachine::AssembleClientIDsL - no hardware address (suggesting point-to-point)"
						     " so generating a random 4 byte client id")));
		iClientId[0] = static_cast<TUint8>(1);	// force hwAddrType to look like ethernet
    	TUint32 data = Math::Random();
		iClientId.Append(TPckgC<TUint32>(data));
		iClientId.Append(TPckgC<TUint16>(data));
    	}
	}


void CDHCPIP4StateMachine::GetServerAddress( TInetAddr& aAddress )
{
	aAddress = iDHCPServerAddr;
}


void CDHCPIP4StateMachine::SetCurrentAddress(const TInetAddr& aCurrentAddress, const TInetAddr& aSubnetMask)
/**
  * The SetCurrentAddress function
  *
  * Stores ip address and subnet mask and assembles the broadcast address
  *
  * @internalTechnology
  */
	{
	iCurrentAddress = aCurrentAddress;
	iSubnetMask = aSubnetMask;
	TInetAddr broadcast;
	broadcast.SubNetBroadcast(iCurrentAddress, iSubnetMask);
	iBroadcastAddress = broadcast.Address();
	}

void CDHCPIP4StateMachine::SetMessageHeaderL( TDHCPv4MessageType aMsgType )
/**
  * Puts a standard header into the message
  *
  * @internalTechnology
  */
	{
	TTime now;
	TTimeIntervalSeconds secs;
	now.HomeTime();	
	(void)now.SecondsFrom(iTaskStartedAt, secs);

	CDHCPMessageHeaderIP4* v4Msg = DhcpMessage();
	v4Msg->InitialiseL();
	v4Msg->SetSecs(static_cast<TUint16>(secs.Int()));	// time field is 16 bits, time elapsed will be less than 16 bits, hence no data loss
	v4Msg->SetXid(iXid.Xid());

	TUint8 htype,hlen;
	if(iHardwareAddr.Family()!=KAFUnspec)
		{
		// we have link layer addressing so fill in the blanks..
		htype = static_cast<TUint8>(iHardwareAddr.Port()); // hwAddr type (hidden in port field)...there won't be any loss of data
		hlen = static_cast<TUint8>(iHardwareAddr.Length()-KHwAddrOffset); // length won't be too long that we lose data. Data starts 8 bytes in, thats why length is minus an offset(8)
		v4Msg->SetCHAddr(iHardwareAddr);
		}
	else
		{
		// We have no link layer addressing so fill in something "traditional" so as not to upset a stupid server.
		//  This is a best guess based on lack of specs in this area
		htype = 1; // ethernet
		hlen = 6; // mac address length
		}

	v4Msg->SetHeader(EDHCPBootRequest,  htype /*hardware addr type*/,  hlen /*hardware addr length*/,  0 /*hops*/);
	v4Msg->SetFlags(0x0000);
#ifdef _DEBUG
	if (CDHCPServer::DebugFlags() & KDHCP_RequestIP4BroadcastOffer)
		{
		v4Msg->SetFlags(0x8000); // force broadcast
		}
#endif
	// +++++++++++++++++++++ Message Type (1 byte) ++++++++++++++++++++++++++++++++/
	v4Msg->AddOptionL(EDHCPMessageType, 1)->SetBigEndian(aMsgType);
	}




TUint8 CDHCPIP4StateMachine::GetMessageTypeL() const
/**
  * Returns the message type from the DHCP message
  * i.e. whether the message is an offer, ack, nak...
  *
  * @internalTechnology 
  */
	{
	CDHCPMessageHeaderIP4* v4Msg = DhcpMessage();
	v4Msg->ParseL();
	v4Msg->Dump();	
	
	return v4Msg->iOptions.GetMessageType();
	}

TBool CDHCPIP4StateMachine::CheckXid() const
/**
  * Checks xid of incoming packet against that of last message sent
  * must be called after GetMessageType so that message has been parsed!
  *
  * @internalTechnology
  */
	{
	CDHCPMessageHeaderIP4* v4Msg = DhcpMessage();
	// check the Xid matches the xid we put in the discover
	if (v4Msg->GetXid()!=iXid.Xid())
		{
		__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4StateMachine::GetMessageTypeL - non-matching xid")));
		return EFalse;	// does not match so we are not interested in this message
		}
	return ETrue;
	}

void CDHCPIP4StateMachine::CreateDiscoverMsgL()
/**
  * Puts the specifics of the discover message into
  * the message buffer
  *
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4StateMachine::CreateDiscoverMsgL")));
	
	if(iTaskStartedAt == 0)
		{
		// set time stamp to now
		iTaskStartedAt.HomeTime();
		iMessageSender->SetTaskStartedTime(iTaskStartedAt);
		}
	iXid.SetXid(static_cast<TUint32>(iXid.Rnd(0,KMaxTInt)));
	SetMessageHeaderL(EDHCPDiscover);

	CDHCPMessageHeaderIP4* v4Msg = DhcpMessage();
	
	// +++++++++++++++++++++ requested IP address +++++++++++++++++++++++++++++++++/
	TInetAddr unspAddr;  //creates unspecified address
	// now check to see if a previous address exists in CommDB with an existing lease
	if (!iCurrentAddress.Match(unspAddr))
		{
		// if we have a known address (perhaps from an expired lease) then request it
		// an address is 4 bytes long
		v4Msg->AddOptionL(EDHCPRequestedIPAddr, KIp4AddrByteLength)->SetBigEndian(iCurrentAddress.Address());
		}
	// XXX: Add hostname to discover (maybe request), update device hostname
	// if offered value differs - repeat for IPv6
	
	// +++++++++++++++++++++ FQDN update request ++++++++++++++++++++++++++++++++++/
	CreateFqdnUpdateRequestL();
	}

void CDHCPIP4StateMachine::CreateOfferAcceptanceRequestMsgL()
/**
  * Puts the specifics of the offer acceptance request
  * message into the message buffer
  *
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4StateMachine::CreateOfferAcceptanceRequestMsgL")));
	
	SetMessageHeaderL(EDHCPRequest);
	CDHCPMessageHeaderIP4* v4Msg = DhcpMessage();
	
	// +++++++++++++++++++++ requested IP address +++++++++++++++++++++++++++++++++/
	v4Msg->AddOptionL(EDHCPRequestedIPAddr, KIp4AddrByteLength)->SetBigEndian(iCurrentAddress.Address());
	
	// +++++++++++++++++++++ server ID ++++++++++++++++++++++++++++++++++++++++++++/
	v4Msg->AddOptionL(EDHCPServerID, KIp4AddrByteLength)->SetBigEndian(iDHCPServerID.Address());
	
	// +++++++++++++++++++++ FQDN update request ++++++++++++++++++++++++++++++++++/
	CreateFqdnUpdateRequestL();	
	}


void CDHCPIP4StateMachine::CreateRebootRequestMsgL()
/**
  * Puts the specifics of a reboot request message
  * into the message buffer 
  *
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4StateMachine::CreateRebootRequestMsgL")));
	
	iTaskStartedAt.HomeTime();	// set time stamp to now
	iMessageSender->SetTaskStartedTime(iTaskStartedAt);
	iXid.SetXid(static_cast<TUint32>(iXid.Rnd(0,KMaxTInt)));
	SetMessageHeaderL(EDHCPRequest);

	CDHCPMessageHeaderIP4* v4Msg = DhcpMessage();
	
	// +++++++++++++++++++++ requested IP address +++++++++++++++++++++++++++++++++/
	v4Msg->AddOptionL(EDHCPRequestedIPAddr, KIp4AddrByteLength)->SetBigEndian(iCurrentAddress.Address());
	}


void CDHCPIP4StateMachine::CreateInformMsgL()
/**
  * Puts the specifics of an inform message
  * into the message buffer
  *
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4StateMachine::CreateInformMsgL")));
	 
	iXid.SetXid(static_cast<TUint32>(iXid.Rnd(0,KMaxTInt)));
	iTaskStartedAt.HomeTime();	// set time stamp to now
	SetMessageHeaderL(EDHCPInform);

	CDHCPMessageHeaderIP4* v4Msg = DhcpMessage();
	v4Msg->SetCIAddr(iCurrentAddress.Address());
	//v4Msg->SetFlags(0);		// indicates that the response should be unicast back
	
	}

void CDHCPIP4StateMachine::CreateRenewRequestMsgL()
/**
  * Puts the specifics of the renew message
  * into the mesage buffer
  *
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4StateMachine::CreateRenewRequestMsgL")));
	
	 if(iTaskStartedAt != 0)
		{
		iTaskStartedAt.HomeTime();
		iMessageSender->SetTaskStartedTime(iTaskStartedAt);
		}
	SetMessageHeaderL(EDHCPRequest);

	CDHCPMessageHeaderIP4* v4Msg = DhcpMessage();
	v4Msg->SetCIAddr(iCurrentAddress.Address());
	v4Msg->SetFlags(0);		// indicates that the response should be unicast back
	}	

void CDHCPIP4StateMachine::CreateRebindRequestMsgL()
/**
  * Puts the specifics of the rebind message
  * into the mesage buffer which happened to be the same as the renew message
  *
  * @internalTechnology
  */
   {
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4StateMachine::CreateRebindRequestMsgL")));
   CDHCPIP4StateMachine::CreateRenewRequestMsgL(); //to avoid virtual call generation
   }

void CDHCPIP4StateMachine::CreateReleaseMsgL()
/**
  * Puts the specifics of the release message
  * into the message buffer
  * 
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4StateMachine::CreateReleaseMsgL")));

	SetMessageHeaderL(EDHCPReleaseMsg);
	
	CDHCPMessageHeaderIP4* v4Msg = DhcpMessage();
	v4Msg->SetCIAddr(iCurrentAddress.Address());
	v4Msg->SetFlags(0);		// probably don't need to set...as there is no respsonse...
	
	// +++++++++++++++++++++ server ID ++++++++++++++++++++++++++++++++++++++++++++/
	v4Msg->AddOptionL(EDHCPServerID, KIp4AddrByteLength)->SetBigEndian(iDHCPServerID.Address());
	//remove the configured address ater Completion of the Release Message
	}

void CDHCPIP4StateMachine::CreateDeclineMsgL()
/**
  * Puts the specifics of the decline message
  * into the message buffer
  *
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4StateMachine::CreateDeclineMsgL")));
	
	SetMessageHeaderL(EDHCPDecline);

	CDHCPMessageHeaderIP4* v4Msg = DhcpMessage();
	
	// +++++++++++++++++++++ server ID ++++++++++++++++++++++++++++++++++++++++++++/
	v4Msg->AddOptionL(EDHCPServerID, KIp4AddrByteLength)->SetBigEndian(iDHCPServerID.Address());
	
	// +++++++++++++++++++++ requested IP address +++++++++++++++++++++++++++++++++/
	v4Msg->AddOptionL(EDHCPRequestedIPAddr, KIp4AddrByteLength)->SetBigEndian(iCurrentAddress.Address());
	RemoveConfiguredAddress();
	iRetryDhcpIpCount++;
	}

#ifdef SYMBIAN_NETWORKING_DHCPSERVER
void CDHCPIP4StateMachine::CreateOfferMsgL()
/**
  *  CreateOfferMsgL
  *
  * Puts the specifics of the Offer message into the message buffer
  * 
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4StateMachine::CreateOfferMsgL")));

	CDHCPMessageHeaderIP4* v4Msg = DhcpMessage();
	iXid.SetXid(v4Msg->GetXid());
	iTaskStartedAt.HomeTime();
	// remember the client hardware address from message for later use
	v4Msg->GetCHAddr(iClientHWAddr);
	
	// check if the broadcast flag is set for this client then we sent broadcast message to it
	TUint16 flags  = v4Msg->GetFlags();
	if(flags & KBroadCastFlagMask)
		iBroadCastFlag = ETrue;

	// Create DHCPOFFER message
	iFlag = 0x8000;			
	SetMessageHeaderAsServerL(EDHCPOffer);	
	
	// generate the client IP addres that is to be offered to the client
	TUint32 offeredAddress = GenerateClientIPAddress();
	
	if(iSvrState == ESvrWaitForAnyDHCPMsgs)
		{
		if(iInformClientAddr.Address())
			v4Msg->SetYIAddr(iInformClientAddr.Address());
		else
			v4Msg->SetYIAddr(offeredAddress);
		}
	else
		{
		v4Msg->SetYIAddr(offeredAddress);
		}	
	SetClientIdentified(EFalse);	
	AddMessageOptionsL();
	}

void CDHCPIP4StateMachine::HandleInformMsgL()
/**
  *  HandleInformMsgL
  *
  * Handles DHCPInform message and sends DHCPAck
  * 
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4StateMachine::CreateResponseMsgL")));
	
	CDHCPMessageHeaderIP4* v4Msg = DhcpMessage();
	
	iXid.SetXid(v4Msg->GetXid());
	iTaskStartedAt.HomeTime();
	
	v4Msg->GetCHAddr(iClientHWAddr);
	
	iCiaddr = v4Msg->GetCIAddr();
	iYiaddr = v4Msg->GetYIAddr();
	iFlag = 0x0000;	
	
	SetMessageHeaderAsServerL(EDHCPAck);	
	iYiaddr = iCiaddr;
	}

void CDHCPIP4StateMachine::CheckClientParamListL()
	{
	CDHCPMessageHeaderIP4* v4Msg = DhcpMessage();
	// check for the Parameter request list option code
	DHCPv4::COptionNode* pOption = v4Msg->iOptions.FindOption(EDHCPParameterReqList);
	
	if(pOption)
		{
		TUint8* headerPtr = pOption->Ptr();	
		
		// const TUint8 KParamReqTypeOffset = 0; never used
		const TUint8 KParamReqLengthOffset = 1;
				
		//  TUint encoding = headerPtr[KParamReqTypeOffset];   never used
		TUint length = headerPtr[KParamReqLengthOffset]; // n octets
	
		TUint8* bodyPtr = pOption->GetBodyPtr();
		
		TUint paramReqAddrOffset;
		TUint8 paramReqValue;
		
		for(TInt i=1; i <= length; i++)
			{
			// read the requested parameter list 
			paramReqAddrOffset = 1 * i;	
			Mem::Copy(&paramReqValue,(bodyPtr + paramReqAddrOffset),1);  
			
			AddParamRequestOptionL(paramReqValue);
			}
		}
	}

void CDHCPIP4StateMachine::AddParamRequestOptionL(TUint8 aParamReqValue)
	{
	CDHCPMessageHeaderIP4* v4Msg = DhcpMessage();
	// default values determined to be used widely are provided by the DHCP server
	// for these requested parameters
	
	// few of the widely requested options are provided, remainning options are ignored
	switch(aParamReqValue)	
		{
	case EDHCPRouter:
		v4Msg->AddOptionL(EDHCPRouter, KIp4AddrByteLength)->SetBigEndian(iCurrentAddress.Address());	
		break;
	case EDHCPBroadcastAddr:
		v4Msg->AddOptionL(EDHCPBroadcastAddr, KIp4AddrByteLength)->SetBigEndian(KInetAddrBroadcast);	
		break;	
	case EDHCPServerID:
		v4Msg->AddOptionL(EDHCPServerID,KIp4AddrByteLength)->SetBigEndian(iCurrentAddress.Address());	
		break;
	case EDHCPSubnetMask:
		v4Msg->AddOptionL(EDHCPSubnetMask,KIp4AddrByteLength)->SetBigEndian(KInetAddrNetMaskC); 
		break;	
	case EDHCPEnd:
		v4Msg->AddOptionL(EDHCPEnd, 0);
		break;
		}
	}

void CDHCPIP4StateMachine::HandleRequestMsgL()
/**
  * CreateRequestResponseMsgL
  *
  * Creates DHCPACK or DHCPNAK in response to DHCPREQUEST (Renew, Rebind and Init-Reboot)
  * 
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4StateMachine::CreateRequestResponseMsgL")));
	
	CDHCPMessageHeaderIP4* v4Msg = DhcpMessage();
	
	iXid.SetXid(v4Msg->GetXid());
	iTaskStartedAt.HomeTime();
	
	v4Msg->GetCHAddr(iClientHWAddr);

	iCiaddr = v4Msg->GetCIAddr();
	
	// check if the broadcast flag is set for this client then we sent broadcast message to it
	TUint16 flags  = v4Msg->GetFlags();
	if(flags & KBroadCastFlagMask)
		iBroadCastFlag = ETrue;
	 
	TUint32 offeredAddress = GetIPAddressToOffer();
		
	// Check the requested parameter list from client and provide them
	CheckClientParamListL();
	
	TInetAddr reqIPAddress ;
	reqIPAddress.SetAddress(v4Msg->iOptions.GetRequestedIPAddress());
				
	//Check if the message contains the Requested IPAddress 
	if(reqIPAddress.Address())
		{
		//Compare the requested IP Address with the generated offered IP Address.
		// If same as the offered IP Address,then send DHCPACK
		if(reqIPAddress.Address() == offeredAddress)
			{
			iFlag = 0x8000;			
			SetMessageHeaderAsServerL(EDHCPAck);
			iYiaddr = offeredAddress;
			v4Msg->SetYIAddr(iYiaddr);
			AddMessageOptionsL();
			}
		else
			{
			// cannot provide requested IP address
			iFlag = 0x8000;
			SetMessageHeaderAsServerL(EDHCPNak);
			}
		}
	else				
		{
		// No Requested IP address when DHCPRequest is received during
		// Renewal and Rebind of IPAddress
		iFlag = 0x0000;
		SetMessageHeaderAsServerL(EDHCPAck);
		iYiaddr = iCiaddr;
		v4Msg->SetYIAddr(iYiaddr);
		AddMessageOptionsL();
	   	}
	}

TUint32 CDHCPIP4StateMachine::GetIPAddressToOffer()
	{
	TUint32 offeredAddress;
	// check if we ever offered any client which was statically configured
	// and we responded to DHCPINFORM from it
	// We offer the same IP address to any second client so that if the first client
	// existed then the second client fails with DAD
	if(iSvrState == ESvrWaitForAnyDHCPMsgs)
		{
		if(iInformClientAddr.Address())
			offeredAddress = iInformClientAddr.Address();
		else 
			offeredAddress = GenerateClientIPAddress();
		}
	else
		{
		// Generate a valid IP address which can be offered to the client 
		offeredAddress = GenerateClientIPAddress();
		}
#ifdef SYMBIAN_DNS_PROXY	
	iOfferedAddress = offeredAddress;
#endif
	return offeredAddress;	
	}

void CDHCPIP4StateMachine::AddMessageOptionsL()
/**
  * AddMessageOptionsL
  *
  * Adds Lease Time,Renew Time and Rebind Time options
  * 
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4StateMachine::AddMessageOptionsL")));
	
	CDHCPMessageHeaderIP4* v4Msg = DhcpMessage();

// Providing support for handling custom lease time being set for DHCP server
// in DEBUG builds using RProperty API's
	TInt defLeaseTime;
#ifdef _DEBUG
	RProperty::Get(KMyPropertyCat, KMyDefaultLeaseTime, defLeaseTime);
#else
	defLeaseTime = KDefaultLeaseTime;
#endif
	// +++++++++++++++++ Default Fixed Lease Time - 6 hours ++++++++++++++++++++++++++++++++++++++++/
	v4Msg->AddOptionL(EDHCPLeaseTime, KIp4AddrByteLength)->SetBigEndian(defLeaseTime);
	// +++++++++++++++++ Renewal Time (T1) - 50% of Lease time ++++++++++++++++++++++++++++++++++++++++/				
	v4Msg->AddOptionL(EDHCPRenewalT1,KIp4AddrByteLength)->SetBigEndian(defLeaseTime/2);
	// +++++++++++++++++ Rebind Time (T2) - 75% of Lease time ++++++++++++++++++++++++++++++++++++++++/				
	v4Msg->AddOptionL(EDHCPRebindT2,KIp4AddrByteLength)->SetBigEndian(defLeaseTime/2+defLeaseTime/4);
	// These are the options which the client might ask later
	// Simplified DHCP server is not capable of generating values for these option codes
	// They will be set later using Ioctl() call
	v4Msg->AddOptionL(EDHCPNameServer,KIp4AddrByteLength);		
	v4Msg->AddOptionL(EDHCPSIPServers,KIp4AddrByteLength);		
		
	if(!iDNSInformation)
		{
		v4Msg->AddOptionL(EDHCPDomainNameServer,KIp4AddrByteLength);
		
#ifdef SYMBIAN_DNS_PROXY
		if(iProxyDnsSrvAddr.Address())		
		   v4Msg->AddOptionL(EDHCPDomainNameServer,KIp4AddrByteLength)->SetBigEndian(iProxyDnsSrvAddr.Address());
		if(iProxyDomainName.Length())
		   v4Msg->AddOptionL(EDHCPDomainName, iProxyDomainName.Length())->GetBodyDes().Copy(iProxyDomainName);
#endif
		}
	else
		{
		// point to the option code value in the raw option buffer
		TPtr8 dnsPtr(const_cast<TUint8*>(iDNSInformation->Ptr()), iDNSInformation->Length(), iDNSInformation->Length());
		dnsPtr.Set((TUint8*)(dnsPtr.Ptr()+1),dnsPtr.Length()-1,dnsPtr.MaxLength()-1);
	
		v4Msg->AddOptionL(EDHCPDomainNameServer,dnsPtr.Length())->GetBodyDes().Copy(dnsPtr);
		}
	}

#endif // SYMBIAN_NETWORKING_DHCPSERVER
	
TInt CDHCPIP4StateMachine::CreateIPv4LinkLocal()
/**
  * Notifies the TCP/IP6 stack whenever the assignment process fails
  * so that a link local may be created.
  * 
  *
  * @internalTechnology
  */
	{
	__CFLOG( KLogSubSysDHCP, KLogCode, _L8( "CDHCPIP4StateMachine::CreateIPv4LinkLocal" ) );

	iSocket.Close();	
	
	TInt err = iSocket.Open(iEsock, KAfInet, KSockDatagram, KProtocolInetUdp, iConnection);
	
	if( err == KErrNone )
		{
		// make socket invisible for interface counting
		(void)iSocket.SetOpt(KSoKeepInterfaceUp, KSolInetIp, 0);

		TPckgBuf<TSoInet6InterfaceInfo> configInfo;

		configInfo().iHwAddr = iHardwareAddr;
		configInfo().iName = iInterfaceName;
		configInfo().iDelete = EFalse;
		configInfo().iAlias = EFalse;
		configInfo().iDoId = EFalse;
		configInfo().iState = EIfUp;
		configInfo().iDoState = ETrue;
		
		// zero value better than junk value
		configInfo().iMtu = 0;
		configInfo().iSpeedMetric = 0;
		configInfo().iFeatures = 0; 

		err = iSocket.SetOpt( KSoInetCreateIPv4LLOnInterface, KSolInetIfCtrl, configInfo );
		}
		
	return err;
	}

void CDHCPIP4StateMachine::HandleOfferL()
/**
  * Handles an offer from a dhcp server, providing
  * an offer of configuration parameters
  *
  * @internalTechnology
  *
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4StateMachine::HandleOffer")));
	
	CDHCPMessageHeaderIP4* v4Msg = DhcpMessage();
	// at this stage just the offered ip address is set into the current address for efficiency
	iCurrentAddress.SetAddress(v4Msg->GetYIAddr());	
	
	TUint32 addr = v4Msg->iOptions.GetServerId();
	if ( !addr )
		User::Leave(KErrNotFound);
	iDHCPServerID.SetAddress(addr);
	}

CDHCPState* CDHCPIP4StateMachine::HandleReplyL( TRequestStatus* aStatus )
{
	iReceiving = EFalse;
	
	TInt err = KErrNone;

	if (CheckXid())
		{
		switch (GetMessageTypeL())
			{
			case EDHCPAck:
#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS 	
				if (!iDhcpInformAckPending)
				{
#endif//SYMBIAN_NETWORKING_DHCP_MSG_HEADERS	
				HandleAckL();
#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS 					
				}
#endif//SYMBIAN_NETWORKING_DHCP_MSG_HEADERS	

				break;
			case EDHCPNak:
				err = KErrAccessDenied;

				break;
			default:
				return ReceiveL(aStatus);
			}
		}
	else
		{
		return ReceiveL(aStatus);
		}
		
	User::RequestComplete(aStatus, err);
	
	if( err == KErrNone )
	{
		return static_cast<CDHCPState*>(iActiveEvent->Next());
	}
	
	return NULL;
}

void CDHCPIP4StateMachine::HandleAckL()
/**
  * Handles an acknowledgement from a dhcp server, storing
  * configuration parameters and a committed ip address
  *
  * @internalTechnology
  *
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4StateMachine::HandleAck")));
	
	if(iRetryDhcpIpCount >= KDHCPv4MaxRetryCount)
 	{
	TPckgBuf<TSoInetInterfaceInfo> opt;
	TInetAddr addr;
	while (iSocket.GetOpt(KSoInetNextInterface, KSolInetIfCtrl, opt) == KErrNone)
		{
	  	if (opt().iName == InterfaceName())
			{
		  	addr = opt().iAddress;
		  	if (addr.IsLinkLocal() && addr.IsV4Compat() )
		  		{
		  		RemoveConfiguredAddress(&addr);
		  		break;
		  		}
		  	}
		}
	}
	ConfigureInterfaceL();

	CDHCPMessageHeaderIP4* v4Msg = DhcpMessage();
	if (!IsUsingStaticAddress())
		{
		iRenewalTimeT1 = v4Msg->iOptions.GetRenewalTime();
		iRebindTimeT2 = v4Msg->iOptions.GetRebindTime();
		iLeaseTime = v4Msg->iOptions.GetLeaseTime();
		
		#ifdef _LOG
			TBuf<39> addrStr;
			
			TInetAddr( v4Msg->GetYIAddr(), 0 ).Output( addrStr );
			
			__CFLOG_1( KLogSubSysDHCP, KLogCode, _L( "DHCP server assigned client IP address %S" ), &addrStr );
		#endif
		
#ifdef _DEBUG
		if (CDHCPServer::DebugFlags() & KDHCP_SetShortLease)
			{
			iLeaseTime = KDHCP_ShortLeaseLeaseTime;
			iRenewalTimeT1 = KDHCP_ShortLeaseRenewTime;
			iRebindTimeT2 = KDHCP_ShortLeaseRebindTime;
			CDHCPServer::DebugFlags() &= ~( KDHCP_SetShortLease | KDHCP_SetShortRetryTimeOut );	// we only want this to have an affect the first time...
			}
#endif
		// -- Infinite leases -- INC078424 --
		//
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
			
		if (iRenewalTimeT1 == iRebindTimeT2)
			{
			// may have only been provided with a lease time...
			// we need time to renew the lease before it runs out
			// so we'd better set some times from the overall lease time
			
			__CFLOG_VAR((KLogSubSysDHCP, KLogCode,
				_L8("Invalid Renew/Rebind times received: (RenewT1: %d, RebindT2: %d, Lease: %d)"),
						iRenewalTimeT1, iRebindTimeT2, iLeaseTime));
						
			TUint32 temp = iLeaseTime/4;
			iRenewalTimeT1=iLeaseTime/2;
			iRebindTimeT2=iRenewalTimeT1+temp;		
			
			__CFLOG_VAR((KLogSubSysDHCP, KLogCode,
				_L8("New Renew/Rebind Time calculated: (RenewT1: %d, RebindT2: %d)"),
						iRenewalTimeT1, iRebindTimeT2));	
						
			}
		else if (iRenewalTimeT1<iRebindTimeT2 && iRebindTimeT2<iLeaseTime)
			{
			// do nothing as we have got the valid values we were expecting
			__CFLOG_VAR((KLogSubSysDHCP, KLogCode,
				_L8("Times received from server: (RenewT1: %d, RebindT2: %d, lease: %d)"),
						iRenewalTimeT1, iRebindTimeT2, iLeaseTime));
			}
		else
			{
			__CFLOG_VAR((KLogSubSysDHCP, KLogCode,
					_L8("HandleAckL Error: invalid times received from server (RenewT1: %d, RebindT2: %d, lease: %d)"),
							iRenewalTimeT1, iRebindTimeT2, iLeaseTime));
			User::Leave(KErrArgument);
			}	
		
		}

	delete iHostName;
	delete iDomainName;
	iHostName = NULL;
	iDomainName = NULL;

	v4Msg->iOptions.CopyHostNameL(iHostName);
	v4Msg->iOptions.CopyDomainNameL(iDomainName);
	}

#ifdef _DEBUG
TInt CDHCPIP4StateMachine::GetDestPort()
	{
	TInt destPort;
	User::LeaveIfError(RProperty::Get(KMyPropertyCat, KMyPropertyDestPortv4, destPort));
	return destPort;
	}
#endif

void CDHCPIP4StateMachine::InitialiseSocketL()
/**
  * Sets up socket, by opening one associated with the connection
  * and sets the interface to use for traffic
  *
  * @internalTechnology
  *
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4StateMachine::InitialiseSocketL")));
	
	iSocket.Close();
	
	User::LeaveIfError(iSocket.Open(iEsock, KAfInet, KSockDatagram, KProtocolInetUdp, iConnection));
	User::LeaveIfError(iSocket.SetOpt(KSoReuseAddr, KSolInetIp, 1));
#ifndef _DEBUG
	User::LeaveIfError(iSocket.SetLocalPort(KDhcpSrcPort));
#else
	User::LeaveIfError(iSocket.SetLocalPort(GetDestPort() + 1));
#endif
    TInetAddr existingGlobalAddr = this->GetInterfaceGlobalAddress();
    if( existingGlobalAddr.IsUnspecified() || existingGlobalAddr.IsLinkLocal() )
    	{
    	User::LeaveIfError(iSocket.SetOpt(KSoNoSourceAddressSelect, KSolInetIp, 1));
    	}
	// make socket invisible for interface counting
	User::LeaveIfError(iSocket.SetOpt(KSoKeepInterfaceUp, KSolInetIp, 0));
	
	TPckgBuf<TSoInetIfQuery> query;
	query().iName = iInterfaceName;
	User::LeaveIfError(iSocket.GetOpt(KSoInetIfQueryByName, KSolInetIfQuery, query));
	User::LeaveIfError(iSocket.SetOpt(KSoInterfaceIndex, KSolInetIp, query().iIndex));
	User::LeaveIfError(iSocket.SetOpt(KSoInetEnumInterfaces, KSolInetIfCtrl));
	// loopback of broadcast/multicast packets to the stack is disabled
	User::LeaveIfError(iSocket.SetOpt(KSoIp6MulticastLoop, KSolInetIp, 0));
	}

void CDHCPIP4StateMachine::RemoveConfiguredAddress(const TInetAddr * aAddr)
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
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4StateMachine::RemoveConfiguredAddress")));
	
	TSoInet6InterfaceInfo interfaceInfo;
	interfaceInfo.iHwAddr = iHardwareAddr;
	if(aAddr)
		{
		interfaceInfo.iAddress.SetV4MappedAddress(aAddr->Address());
		interfaceInfo.iState = EIfDown;
		}
	else
		{
		interfaceInfo.iAddress.SetV4MappedAddress(iCurrentAddress.Address());
		interfaceInfo.iState = EIfUp;	
		}
	interfaceInfo.iDefGate.SetV4MappedAddress(iDefGateway.Address());
	interfaceInfo.iName = iInterfaceName;
	interfaceInfo.iDelete = ETrue;
	interfaceInfo.iAlias = EFalse;
	interfaceInfo.iDoId = ETrue;
	
	interfaceInfo.iDoState = ETrue;
	interfaceInfo.iDoAnycast = EFalse;
	
	// zero value better than junk value
	interfaceInfo.iMtu = 0;
	interfaceInfo.iSpeedMetric = 0;
	interfaceInfo.iFeatures = 0; 
    CDHCPStateMachine::RemoveConfiguredAddress( interfaceInfo );
    
	// Clear the DHCP server address now that we no longer have a lease.
    iDHCPServerAddr.SetAddress( KInet6AddrNone );
    //clear the client address & lease time
   	iCurrentAddress = TInetAddr();
   	iLeaseTime = 0;

	}

void CDHCPIP4StateMachine::AssignAddresses( TInetAddr& aDest, const TInetAddr& aSrc ) const
{
	aDest.SetFamily(KAfInet);
	aDest.SetAddress(aSrc.Address());
#ifdef SYMBIAN_NETWORKING_DHCPSERVER
	if(iServerImpl)
		{
// The DHCP server can listen on custom ports
// Supported only on DEBUG builds
#ifdef _DEBUG
		aDest.SetPort(GetDestPort());
#else
		aDest.SetPort(KDhcpServerPort);
#endif
		}
	else
#endif // SYMBIAN_NETWORKING_DHCPSERVER	
#ifndef _DEBUG
	aDest.SetPort(KDhcpSrcPort);
#else
	{
	// Simulate initialisation, renewal or rebind failure by using the wrong port.
	if( ( CDHCPServer::DebugFlags() & KDHCP_FailDiscover ) || ( CDHCPServer::DebugFlags() & KDHCP_FailRenew ) || ( CDHCPServer::DebugFlags() & KDHCP_FailRebind ) )
		{
		aDest.SetPort(KDhcpWrongSrcPort);
		}
	else
		{
		aDest.SetPort(GetDestPort() + 1);
		}
	}
#endif
	
}

void CDHCPIP4StateMachine::BindSocketForUnicastL()
/**
  * Open a new socket and bind it to the configured source address
  * ready for a unicast renew message
  *
  * @internalTechnology
  */
	{
    __CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPStateMachine::BindSocketForRenewL")));
    iSocket.Close();	// destroy the old socket

    UpdateHistory(CDHCPState::EBindToSource);
    // Start a new one.
    User::LeaveIfError(iSocket.Open(iEsock, KAfInet, KSockDatagram, KProtocolInetUdp, iConnection));
	User::LeaveIfError(iSocket.SetOpt(KSoReuseAddr, KSolInetIp, 1));
    TInetAddr bindTo;
    AssignAddresses( bindTo, iCurrentAddress );

    User::LeaveIfError(iSocket.Bind(bindTo));

	// So we can still receive packets from naughty servers who send broadcasts back to 
	//  unicast requests, we do this:
	User::LeaveIfError(iSocket.SetOpt(KSoNoSourceAddressSelect, KSolInetIp, 1));
	User::LeaveIfError(iSocket.SetOpt(KSoUdpAddressSet, KSolInetUdp, 0));
	// make socket invisable for interface counting
	User::LeaveIfError(iSocket.SetOpt(KSoKeepInterfaceUp, KSolInetIp, 0));
	}

void CDHCPIP4StateMachine::ConfigureInterfaceL()
/**
  * Set the interface IP address and other params
  * into the TCP/IP6 stack.
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
  * @internalTechnology
  */
	{

	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4StateMachine::ConfigureInterfaceL - Cancel Message Sender")));
	iMessageSender->Cancel();

	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4StateMachine::ConfigureInterfaceL - KSoNoSourceAddressSelect")));
	User::LeaveIfError(iSocket.SetOpt(KSoNoSourceAddressSelect, KSolInetIp, 0));

	TSoInet6InterfaceInfo interfaceInfo;
	
	CDHCPMessageHeaderIP4* v4Msg = DhcpMessage();

	if (!IsUsingStaticAddress())
		{
		iCurrentAddress.SetAddress(v4Msg->GetYIAddr());
		iServerAddress = v4Msg->iOptions.GetServerId();
		
		TInetAddr nullAddr;  //creates unspecif. (null) address

		// we want to use CommDb settings if present
		// but those ifs aren't the solution for that as
		// these values will potentially fail for renegotiation
		// for now we should live with this, as there is no time
		// to change it.
		if (iSubnetMask.Match(nullAddr))
			{
			iSubnetMask.SetAddress(v4Msg->iOptions.GetSubnetMask());
			}

		if (iBroadcastAddress.Match(nullAddr))
			{
			iBroadcastAddress.SetV4MappedAddress(v4Msg->iOptions.GetBroadcastAddress());
			}
		
		if (iDefGateway.Match(nullAddr))
			{
			iDefGateway.SetV4MappedAddress(v4Msg->iOptions.GetRouterAddress());
			}		
		}

	if( iNameServerAddressesFromServer )
		{	
		TInt num = v4Msg->iOptions.NumberOfDomainServers();
		if (num>0)
			{
			iNameServer1.SetAddress(v4Msg->iOptions.GetDomainNameServer(0));
			if (num>1)
				{
				iNameServer2.SetAddress(v4Msg->iOptions.GetDomainNameServer(1));
				}
			}
		}

	
	interfaceInfo.iHwAddr = iHardwareAddr;
	interfaceInfo.iAddress.SetV4MappedAddress(iCurrentAddress.Address());
	interfaceInfo.iNetMask.SetAddress(iSubnetMask.Address());
	interfaceInfo.iBrdAddr.SetV4MappedAddress(iBroadcastAddress.Address());
	interfaceInfo.iDefGate.SetV4MappedAddress(iDefGateway.Address());
	if( iNameServerAddressesFromServer )
		{
		interfaceInfo.iNameSer1.SetV4MappedAddress(iNameServer1.Address());
		interfaceInfo.iNameSer2.SetV4MappedAddress(iNameServer2.Address());	
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

	CDHCPStateMachine::ConfigureInterfaceL( interfaceInfo );
	}

void CDHCPIP4StateMachine::CreateFqdnUpdateRequestL()
	{	
#ifndef SYMBIAN_COMMS_REPOSITORY
	CCommsDatabase* commDb = CCommsDatabase::NewL();
	CleanupStack::PushL(commDb);
	
	CCommsDbTableView* tableView = commDb->OpenViewMatchingUintLC(TPtrC16(NETWORK), TPtrC16(COMMDB_ID), GetNetworkIdL());
	
	TDomainName hostName;
	if (tableView->GotoFirstRecord() == KErrNone)
		{
		tableView->ReadTextL(TPtrC16(HOST_NAME), hostName);
		}
#else
#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
	CMDBSession* session = CMDBSession::NewLC(KCDVersion1_2);
#else
	CMDBSession* session = CMDBSession::NewLC(KCDVersion1_1);
#endif

	// Reveal hidden or private IAP records if a licensee has chosen to protect a record
	// using one of these flags - the API to do this is public so internal components
	// have to support the use of such records.
	session->SetAttributeMask(ECDHidden | ECDPrivate);
	
	CCDNetworkRecord* networkRecord = static_cast<CCDNetworkRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdNetworkRecord));;
	CleanupStack::PushL(networkRecord);
	networkRecord->SetRecordId(GetNetworkIdL());
  	networkRecord->LoadL(*session);

	TDomainName hostName;
	hostName.Copy(networkRecord->iHostName);
#endif
		
	if (hostName.Length() > 0)
		{
		CDnsUpdateOption* dnsUpdateOption = new(ELeave) CDnsUpdateOption();
		CleanupStack::PushL(dnsUpdateOption);
		
		dnsUpdateOption->SetFlag(CDnsUpdateOption::EDnsUpdateFlagE);
		dnsUpdateOption->SetFlag(CDnsUpdateOption::EDnsUpdateFlagS);
		dnsUpdateOption->SetDomainName(hostName);
		
		RBuf8 optionData;
		optionData.CleanupClosePushL();
		
		dnsUpdateOption->ToStringL(optionData);
		
		CDHCPMessageHeaderIP4* v4Msg = DhcpMessage();
		COptionNode* optionNode = 
			v4Msg->AddOptionL(EDHCPDNSUpdate, optionData.Length());
		
		optionNode->SetBody(optionData);
		
		CleanupStack::PopAndDestroy(&optionData);
		CleanupStack::PopAndDestroy(dnsUpdateOption);
		}	
	
#ifndef SYMBIAN_COMMS_REPOSITORY
	CleanupStack::PopAndDestroy(2, commDb);
#else
	CleanupStack::PopAndDestroy(2, session);
#endif
	}
	
#ifdef SYMBIAN_NETWORKING_DHCPSERVER	
void CDHCPIP4StateMachine::InitialiseServerSocketL()
/**
  * Sets up socket, by opening one associated with the connection
  * and sets the interface to use for traffic
  *%
  * @internalTechnology
  *
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4StateMachine::InitialiseServerSocket")));
	
	iSvrSocket.Close();
	
	User::LeaveIfError(iSvrSocket.Open(iEsock, KAfInet, KSockDatagram, KProtocolInetUdp, iConnection));
	User::LeaveIfError(iSvrSocket.SetOpt(KSoReuseAddr, KSolInetIp, 1));
	
// The DHCP server can listen on custom ports
// Supported only on DEBUG builds
#ifdef _DEBUG
	User::LeaveIfError(iSvrSocket.SetLocalPort(GetDestPort()));
#else
	User::LeaveIfError(iSvrSocket.SetLocalPort(KDhcpServerPort));
#endif
	}

	
void CDHCPIP4StateMachine::InitServerStateMachineL(MStateMachineNotify* aStateMachineNotify)
/**
  * Reset the lower state machine for DHCP server implementation.
  * Set the state machine to wait for client messages on port 67 and 
  * to handle the messages
  *
  * @internalTechnology
  */
	{
	ASSERT(!iFirstState);
	// after start up wait on port 67 for DHCP client msgs 
	iFirstState = new(ELeave) CDHCPIP4WaitForClientMsgs(*this);
	// handle the client messages
	CDHCPState* handleClientMsg = new(ELeave) CDHCPIP4HandleClientMsgs(*this);
	iFirstState->SetNext( handleClientMsg);
	
	CDHCPStateMachine::Start(aStateMachineNotify);
	}	

void CDHCPIP4StateMachine::InitServerBinding(MStateMachineNotify* aStateMachineNotify)
/**
  * 
  *
  * @internalTechnology
  */
	{
	ASSERT(!iFirstState);
	// binds the server with the static IP address from Comms database
	iFirstState = new(ELeave) CDHCPIP4BindServer(*this);

	CDHCPStateMachine::Start(aStateMachineNotify);
	}	


void CDHCPIP4StateMachine::ProcessDiscoverL()
/** Set the state machine to handle discover message using lower state machine
  * discover-> offer -> request -> ack
  *  
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4StateMachine::ProcessDiscoverL()")));
	
	// the active state was to handle the client message - here Discover message
	CDHCPState* handleDiscoverMsg = static_cast<CDHCPState*>(iActiveEvent); 

#ifdef SYMBIAN_DNS_PROXY
	HBufC8*   hostName = HBufC8::NewLC(KMaxName);
	CDHCPMessageHeaderIP4* v4Msg = DhcpMessage();
	v4Msg->iOptions.CopyHostNameL(hostName);
	iClientHostName.Copy(hostName->Des());
	CleanupStack::PopAndDestroy(hostName);
#endif // SYMBIAN_DNS_PROXY	
	
	CDHCPState* provideOffer = new(ELeave) CDHCPIP4ProvideOffer(*this);
    handleDiscoverMsg->SetNext( provideOffer );
    			
    CDHCPState* sendRequestResponse = new(ELeave) CDHCPIP4SendRequestResponse(*this);
    provideOffer->SetNext( sendRequestResponse );
    		
    iSvrSpecificState = ESvrDiscoverInProgress;
	}
	
void CDHCPIP4StateMachine::ProcessInformL()
/** Set the state machine to handle inform message using lower state machine
  * inform -> ack
  *  
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4StateMachine::ProcessInformL()")));

#ifdef SYMBIAN_DNS_PROXY
	ReadDhcpMsgParamsL();
#endif // SYMBIAN_DNS_PROXY		
	
	CDHCPState* handleInformMsg = static_cast<CDHCPState*>(iActiveEvent); 
				
	CDHCPState* sendInformResponse = new(ELeave) CDHCPIP4SendInformResponse(*this);
	handleInformMsg->SetNext(sendInformResponse);
		    
	iSvrSpecificState = ESvrInformInProgress;
	}
    
void CDHCPIP4StateMachine::ProcessRequestL()
/** Set the state machine to handle Request message using lower state machine
  * Request -> Ack / Nak
  *  
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4StateMachine::ProcessRequestL()")));
		
	CDHCPState* handleRequestMsg = static_cast<CDHCPState*>(iActiveEvent); 

	CDHCPState* sendRequestResponse = new(ELeave) CDHCPIP4SendRequestResponse(*this);
    handleRequestMsg->SetNext(sendRequestResponse);
		    
    iSvrSpecificState = ESvrRenewInProgress;
	}
			

void CDHCPIP4StateMachine::CheckClientMsgL()
	{
	switch(GetClientMessageTypeL())
		{
		case EDHCPDiscover:
			ProcessDiscoverL();
	    	break;
    	
		case EDHCPInform:
			if(iSvrState != ESvrWaitForAnyDHCPMsgs)
				{
				ProcessInformL();
				}
  			break;
  		
		case EDHCPRequest:
			ProcessRequestL();
			break;

		case EDHCPReleaseMsg:
#ifdef SYMBIAN_DNS_PROXY
			ReadDhcpMsgParamsL();
#endif // SYMBIAN_DNS_PROXY
			iSvrSpecificState = ESvrReleaseInProgress;
			break;

    	case EDHCPDecline:
#ifdef SYMBIAN_DNS_PROXY
			ReadDhcpMsgParamsL();
#endif // SYMBIAN_DNS_PROXY
    		iSvrSpecificState = ESvrDeclineInProgress;
			break;
		}
	}
	
#ifdef SYMBIAN_DNS_PROXY
void CDHCPIP4StateMachine::ReadDhcpMsgParamsL()
	{
	HBufC8*   hostName = HBufC8::NewLC(KMaxName);
	CDHCPMessageHeaderIP4* v4Msg = DhcpMessage();
	TUint32 staticAddr = v4Msg->GetCIAddr();
	iClientStaticAddr.SetAddress(staticAddr);
	v4Msg->iOptions.CopyHostNameL(hostName);
	iClientHostName.FillZ();
	iClientHostName.Copy(hostName->Des());
	CleanupStack::PopAndDestroy(hostName);
	}
#endif // SYMBIAN_DNS_PROXY
#ifdef SYMBIAN_NETWORKING_ADDRESS_PROVISION
TBool CDHCPIP4StateMachine::ClientHwAddrProvisioned()
	{
/** DHCP server server checks in which mode it has to offer the IP to clients.
  * 1. If no HW address is provisioned, don't provide IP to any client.
  * 2. Provide IP for any client if the DHCP server has already received the MAC as 0xFFFFFFFFFFFF through IOCTL call.
  * 3. Provide IP for only clients whose MAC address is provisioned using the IOCTL call.
  * 4. Reset the HW address list if DHCP server receives HW address as 0x000000000000
  *  
  * @internalTechnology
  * @return - ETrue if the client is eligible to get the IP, otherwise EFalse.
  */
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4StateMachine::ClientHwAddrProvisioned()")));
	if(iDhcpHwAddrManager->IsHwAddressProvisioned())
		{
		if(iDhcpHwAddrManager->IsAnyClientAllowed())
			{
			__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("DHCP server assigns IP without MAC provisioning")));
				return ETrue;
			}
		else
			{
			Uint64 clientHwAddress = 0;
			TSockAddr hwAddr;
			CDHCPMessageHeaderIP4* v4Msg = DhcpMessage();
			v4Msg->GetCHAddr(hwAddr);
			TPtrC8 hwAddrPtr(hwAddr.Mid(KHwAddrOffset, KHwAddrLength));
			TInt index = 0;
			TInt length = hwAddrPtr.Length();
			//Convert buffer data to Uint64
			for(; index < length; index++)
				{
				clientHwAddress <<= 8;
				clientHwAddress += hwAddrPtr[index];
				}
			//Check the received MAC address is provisioned. If true provide the DHCP OFFER.
			//Otherwise go back to wait for any DHCP client messages state
			if(iDhcpHwAddrManager->Provisioned(clientHwAddress))
				{
				__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("Client Harware address provisioned")));
				return ETrue;
				}
			}
		}
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("Client Harware address not provisioned")));
	return EFalse;
	}
#endif //SYMBIAN_NETWORKING_ADDRESS_PROVISION
#endif // SYMBIAN_NETWORKING_DHCPSERVER
