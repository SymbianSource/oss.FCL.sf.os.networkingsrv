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
// The DHCPv4 States header file
// 
//

/**
 @file DHCPIP4States.h
*/

#ifndef __DHCPIP4STATES_H__
#define __DHCPIP4STATES_H__

#include "DHCPStates.h"
#include "DHCPIP4StateMachine.h"

class CDHCPIP4Init : public CDHCPAddressAcquisition
	{
public:
	CDHCPIP4Init(CDHCPIP4StateMachine& aDHCPIPv4) :
		CDHCPAddressAcquisition(aDHCPIPv4)
		{
		}
	};

class CDHCPIP4Reboot : public CDHCPRebootConfirm
	{
public:
	CDHCPIP4Reboot(CDHCPIP4StateMachine& aDHCPIPv4) :
		CDHCPRebootConfirm(aDHCPIPv4)
		{
		}
	};

class CDHCPIP4Inform : public CDHCPInformationConfig
	{
public:
	CDHCPIP4Inform(CDHCPIP4StateMachine& aDHCPIPv4) :
		CDHCPInformationConfig(aDHCPIPv4)
		{
		}
	};

class CDHCPIP4Select : public CDHCPSelect
	{
public:
	CDHCPIP4Select(CDHCPIP4StateMachine& aDHCPIPv4) :
		CDHCPSelect(aDHCPIPv4)
		{
		}
	
   virtual CAsynchEvent* ProcessL(TRequestStatus& aStatus);

   CDHCPIP4StateMachine& DHCPIPv4()
		{
		return static_cast<CDHCPIP4StateMachine&>(*iStateMachine);
		}
	};

class CDHCPIP4Request : public CDHCPRequest
	{
public:
	CDHCPIP4Request(CDHCPIP4StateMachine& aDHCPIPv4) :
		CDHCPRequest(aDHCPIPv4)
		{
		}
	};

class CDHCPIP4WaitForDAD : public CDHCPWaitForDADBind
	{
public:
	CDHCPIP4WaitForDAD(CDHCPIP4StateMachine& aDHCPIPv4) :
		CDHCPWaitForDADBind(aDHCPIPv4)
		{
		}
	virtual void TimerExpired();
	};

class CDHCPIP4Renew : public CDHCPRenew
	{
public:
	CDHCPIP4Renew(CDHCPIP4StateMachine& aDHCPIPv4) : CDHCPRenew(aDHCPIPv4)
		{
		}
	};

class CDHCPIP4Rebind : public CDHCPRebind
	{
public:
	CDHCPIP4Rebind(CDHCPIP4StateMachine& aDHCPIPv4) : CDHCPRebind(aDHCPIPv4)
		{
		}
	};

class CDHCPIP4Release : public CDHCPRelease
	{
public:
	CDHCPIP4Release(CDHCPIP4StateMachine& aDHCPIPv4) :
		CDHCPRelease(aDHCPIPv4)
		{
		}
	};

class CDHCPIP4Decline : public CDHCPDecline
	{
public:
	CDHCPIP4Decline(CDHCPIP4StateMachine& aDHCPIPv4) :
		CDHCPDecline(aDHCPIPv4)
		{
		}
	};

#ifdef SYMBIAN_NETWORKING_DHCPSERVER
class CDHCPIP4WaitForClientMsgs : public CDHCPWaitForClientMsgs
	{
public:
	CDHCPIP4WaitForClientMsgs(CDHCPIP4StateMachine& aDHCPIPv4) :
		CDHCPWaitForClientMsgs(aDHCPIPv4)
		{
		}
	};	

// To implement a timer for server socket to bind with server IP address
class CDHCPIP4BindServer : public CDHCPBindServer
	{
public:
	CDHCPIP4BindServer(CDHCPIP4StateMachine& aDHCPIPv4) :
		CDHCPBindServer(aDHCPIPv4)
		{
		}
	virtual void TimerExpired();	
	};
	
class CDHCPIP4HandleClientMsgs : public CDHCPHandleClientMsgs
	{
public:
	CDHCPIP4HandleClientMsgs(CDHCPIP4StateMachine& aDHCPIPv4) :
		CDHCPHandleClientMsgs(aDHCPIPv4)
		{
		}
	virtual CAsynchEvent* ProcessL(TRequestStatus& aStatus);	
		
	CDHCPIP4StateMachine& DHCPIPv4()
		{
		return static_cast<CDHCPIP4StateMachine&>(*iStateMachine);
		}	
  	};
	
class CDHCPIP4ProvideOffer : public CDHCPProvideOffer
	{
public:
	CDHCPIP4ProvideOffer(CDHCPIP4StateMachine& aDHCPIPv4) :
		CDHCPProvideOffer(aDHCPIPv4)
		{
		}
	};

class CDHCPIP4HandleDecline : public CDHCPHandleDecline
	{
public:
	CDHCPIP4HandleDecline(CDHCPIP4StateMachine& aDHCPIPv4) :
		CDHCPHandleDecline(aDHCPIPv4)
		{
		}
	};

class CDHCPIP4HandleRelease : public CDHCPHandleRelease
	{
public:
	CDHCPIP4HandleRelease(CDHCPIP4StateMachine& aDHCPIPv4) :
		CDHCPHandleRelease(aDHCPIPv4)
		{
		}
	};
	
class CDHCPIP4SendRequestResponse : public CDHCPSendRequestResponse
	{
public:
	CDHCPIP4SendRequestResponse(CDHCPIP4StateMachine& aDHCPIPv4) :
		CDHCPSendRequestResponse(aDHCPIPv4)
		{
		}
	virtual CAsynchEvent* ProcessL(TRequestStatus& aStatus);	
		
	CDHCPIP4StateMachine& DHCPIPv4()
		{
		return static_cast<CDHCPIP4StateMachine&>(*iStateMachine);
		}	
	};		
		
class CDHCPIP4SendInformResponse : public CDHCPSendInformResponse
	{
public:
	CDHCPIP4SendInformResponse(CDHCPIP4StateMachine& aDHCPIPv4) :
		CDHCPSendInformResponse(aDHCPIPv4)
		{
		}
	virtual CAsynchEvent* ProcessL(TRequestStatus& aStatus);	
		
	CDHCPIP4StateMachine& DHCPIPv4()
		{
		return static_cast<CDHCPIP4StateMachine&>(*iStateMachine);
		}	
	};	
#endif // SYMBIAN_NETWORKING_DHCPSERVER
		
#endif
