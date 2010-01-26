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
 @file DHCPIP4States.cpp
 @internalTechnology
*/

#include "DHCPIP4States.h"
#include "DHCPMsg.h"
#include "DHCPServer.h"

#include "DHCPStatesDebug.h"

CAsynchEvent* CDHCPIP4Select::ProcessL(TRequestStatus& aStatus)
/**
  * Interface function, execute state machine
  *
  * @internalTechnology
  */
	{
	DHCP_DEBUG_PUBLISH_STATE(DHCPDebug::EDHCPIP4Select);
	CDHCPIP4StateMachine& rDHCPIPv4 = DHCPIPv4();
	
	if (rDHCPIPv4.iReceiving)
		{		
		if((rDHCPIPv4.GetMessageTypeL() == DHCPv4::EDHCPOffer) && rDHCPIPv4.CheckXid())
			{
			return CDHCPSelect::ProcessL(aStatus);
			}
		else
			{
			rDHCPIPv4.iReceiving = EFalse;
			}	
		}				
	return rDHCPIPv4.ReceiveL(&aStatus);	
	}

#ifdef SYMBIAN_NETWORKING_DHCPSERVER	
CAsynchEvent* CDHCPIP4HandleClientMsgs::ProcessL(TRequestStatus& aStatus)
/**
  * Interface function, execute state machine
  *
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4HandleClientMsgs::ProcessL")));
	
	DHCP_DEBUG_PUBLISH_STATE(DHCPDebug::EDHCPIP4HandleClientMsgs);
	CDHCPIP4StateMachine& rDHCPv4  = DHCPIPv4();
	rDHCPv4.iReceiving = EFalse;
	
#ifdef SYMBIAN_NETWORKING_ADDRESS_PROVISION
	TBool provisioned = rDHCPv4.ClientHwAddrProvisioned();
	if(provisioned)
		{
#endif //SYMBIAN_NETWORKING_ADDRESS_PROVISION
		rDHCPv4.CheckClientMsgL();
#ifdef SYMBIAN_NETWORKING_ADDRESS_PROVISION
		}
#endif //SYMBIAN_NETWORKING_ADDRESS_PROVISION
	// self complete to continue the state machine
	TRequestStatus* p = &aStatus;
	User::RequestComplete(p, KErrNone);
	return iNext;
	}	
	
CAsynchEvent* CDHCPIP4SendRequestResponse::ProcessL(TRequestStatus& aStatus)
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4SendRequestResponse::ProcessL")));
	
	DHCP_DEBUG_PUBLISH_STATE(DHCPDebug::EDHCPIP4SendRequestResponse);
	CDHCPIP4StateMachine& rDHCPIPv4 = DHCPIPv4();
	
	if (rDHCPIPv4.iReceiving)
		{		
		if((rDHCPIPv4.GetClientMessageTypeL() == DHCPv4::EDHCPRequest))
			{
			if(rDHCPIPv4.CheckXid())
				{
				// this is from the same client which we last serviced
				return CDHCPSendRequestResponse::ProcessL(aStatus);	
				}
			else 
				{
				// this request is not from the same client as the one we recently serviced  
				// check if this was a renew/rebind request from a client whom we provided an IP address in the past
				// then we need to service this client and not ignore it
				TUint32 clientAddr = rDHCPIPv4.GetClientIPAddress();
				if(rDHCPIPv4.GetIPAddressToOffer() == clientAddr ||
						!clientAddr)
					{
					rDHCPIPv4.SetClientIdentified(ETrue);
					return 	CDHCPSendRequestResponse::ProcessL(aStatus);	
					}
				else
					{
					// unexpected packet, proceed to recieve the packet and discard.
					__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4SendRequestResponse::ProcessL Unexpected packet")));
					rDHCPIPv4.iReceiving = EFalse;
					} 
				}	
			}
		else
			{
			rDHCPIPv4.iReceiving = EFalse;
			}	
		}				
	return rDHCPIPv4.ReceiveOnPort67L(&aStatus);
	}
	
CAsynchEvent* CDHCPIP4SendInformResponse::ProcessL(TRequestStatus& aStatus)
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4SendInformResponse::ProcessL")));
	
	DHCP_DEBUG_PUBLISH_STATE(DHCPDebug::EDHCPIP4SendInformResponse);	
	CDHCPIP4StateMachine& rDHCPIPv4 = DHCPIPv4();
	
	if (rDHCPIPv4.iReceiving)
		{		
		if((rDHCPIPv4.GetClientMessageTypeL() == DHCPv4::EDHCPInform))
			{
			return CDHCPSendInformResponse::ProcessL(aStatus);
			}
		else
			{
			rDHCPIPv4.iReceiving = EFalse;
			}	
		}				
	return rDHCPIPv4.ReceiveOnPort67L(&aStatus);
	}	

void CDHCPIP4BindServer::TimerExpired()
	{
	CDHCPStateMachine& rDHCP = Dhcp();
	CDHCPBindServer::TimerExpired();
	if ( !rDHCP.TimerActive() )
		{
		//finish either => bound or not
		TRequestStatus* p = &iStateMachine->iStatus;
		User::RequestComplete(p, iErr); 
		rDHCP.SetAsyncCancelHandler(NULL);
		}
	}
		
#endif // SYMBIAN_NETWORKING_DHCPSERVER

void CDHCPIP4WaitForDAD::TimerExpired()
	{
	CDHCPStateMachine& rDHCP = Dhcp();
	CDHCPWaitForDADBind::TimerExpired();
	if ( !rDHCP.TimerActive() )
		{
		//finish either => bound or not
		TRequestStatus* p = &iStateMachine->iStatus;
		User::RequestComplete(p, iErr); 
		rDHCP.SetAsyncCancelHandler(NULL);
		}
	}
	


