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
// The DHCP States header file
// 
//

/**
 @file DHCPStates.h
*/

#ifndef __DHCPSTATES_H__
#define __DHCPSTATES_H__

#include <e32base.h>
#include <comms-infras/asynchevent.h>
#include "DHCPStateMachine.h"
#ifdef SYMBIAN_ESOCK_V3
#include <comms-infras/idquerynetmsg.h>
#include <comms-infras/netsubscribe.h>
#include <c32root.h>

namespace NetSubscribe
{
class TEvent;
}

class SFactoryChannel
	{
public:
	~SFactoryChannel();
	SFactoryChannel()
		{
		}
	void SendMessageL( NetMessages::CMessage& aMsg );
	
protected:
	CommsFW::TCFModuleName iModule;
	Elements::TRBuf8 iBuf;
	RRootServ iC32Root;
	};

class SDhcpSignal : public SFactoryChannel
	{
public:
	~SDhcpSignal();
	SDhcpSignal()
		{
		}
	void SubscribeL( const TName& aInterfaceName, TInt aEventId, NetSubscribe::TEvent& aEvent );
	
protected:
	NetSubscribe::CNetSubscribe* iNetSubscribe;
	NetMessages::CTypeIdQuery* iQuery;
	};
#endif

class CDHCPState : public CAsynchEvent
	{
public:
	CDHCPState(CDHCPStateMachine& aDHCP) :
		CAsynchEvent(&aDHCP) 
		{
		}
	virtual ~CDHCPState();

	virtual CDHCPState* ProcessAckNakL(TRequestStatus* aStatus);
	CDHCPStateMachine& Dhcp()
		{
		return static_cast<CDHCPStateMachine&>(*iStateMachine);
		}
	
	virtual void Cancel();
public:
	enum EDHCPHistory //must be a bit mask
		{
		EBinding = 1,
		EBindToSource = 2
		};

	};

class CDHCPAddressAcquisition : public CDHCPState
	{
public:
	CDHCPAddressAcquisition(CDHCPStateMachine& aDHCP) :
		CDHCPState(aDHCP)
		{
		}
	//create msg sends it off & initiates receive
	virtual CAsynchEvent* ProcessL(TRequestStatus& aStatus);
	};

class CDHCPSelect : public CDHCPState
	{
public:
	CDHCPSelect(CDHCPStateMachine& aDHCP) :
		CDHCPState(aDHCP)
		{
		}
	//create msg sends it off & initiates receive
	virtual CAsynchEvent* ProcessL(TRequestStatus& aStatus);
	};

class CDHCPRebootConfirm : public CDHCPState
	{
public:
	CDHCPRebootConfirm(CDHCPStateMachine& aDHCP) :
		CDHCPState(aDHCP)
		{
		}
	//create msg sends it off & initiates receive
	virtual CAsynchEvent* ProcessL(TRequestStatus& aStatus);
	};

class CDHCPInformationConfig : public CDHCPState
	{
public:
	CDHCPInformationConfig(CDHCPStateMachine& aDHCP) :
		CDHCPState(aDHCP)
		{
		}
	//create msg sends it off & initiates receive
	virtual CAsynchEvent* ProcessL(TRequestStatus& aStatus);
	};

class CDHCPRequest : public CDHCPState
	{
public:
	CDHCPRequest(CDHCPStateMachine& aDHCP) :
		CDHCPState(aDHCP)
		{
		}
	virtual CAsynchEvent* ProcessL( TRequestStatus& aStatus );
	};

class CDHCPWaitForDADBind : public CDHCPState, public MExpireTimer
	{//uses RSocket::Bind to validate DAD
public:
	CDHCPWaitForDADBind(CDHCPStateMachine& aDHCP) :
		CDHCPState(aDHCP)
		{
		}
	virtual void TimerExpired();

	//starts bind timer
	virtual CAsynchEvent* ProcessL(TRequestStatus& aStatus);
	
	virtual void Cancel();
protected:
	TTime iBoundAt;
	TInt iErr;
	};

#if 0
class CDHCPWaitForDADIPNotifier : public CDHCPState, public CSubscribeChannel, public MExpireTimer
	{//uses public&subscribe (IPEventNotifier component) to validate DAD
public:
	CDHCPWaitForDADIPNotifier(CDHCPStateMachine& aDHCP) :
		CDHCPState(aDHCP)
		{
		}
	virtual void TimerExpired();

protected:
	TTime iBoundAt;
	};
#endif

class CDHCPRenew : public CDHCPState
	{
public:
	CDHCPRenew(CDHCPStateMachine& aDHCP) : CDHCPState(aDHCP)
		{
		}
	//   ACK received => continue with next state (Bound)
	virtual CAsynchEvent* ProcessL(TRequestStatus& aStatus);
	};

class CDHCPRebind : public CDHCPState
	{
public:
	CDHCPRebind(CDHCPStateMachine& aDHCP) : CDHCPState(aDHCP)
		{
		}
	//   ACK received => continue with next state (Bound)
	virtual CAsynchEvent* ProcessL(TRequestStatus& aStatus);
	};

class CDHCPRelease : public CDHCPState
	{
public:
	CDHCPRelease(CDHCPStateMachine& aDHCP) :
		CDHCPState(aDHCP)
		{
		}
	//create msg sends it off
	virtual CAsynchEvent* ProcessL(TRequestStatus& aStatus);
	};



class CDHCPDecline : public CDHCPState
	{
public:
	CDHCPDecline(CDHCPStateMachine& aDHCP) :
		CDHCPState(aDHCP)
		{
		}
	//create msg sends it off
	virtual CAsynchEvent* ProcessL(TRequestStatus& aStatus);
	};

#ifdef SYMBIAN_NETWORKING_DHCPSERVER
class CDHCPWaitForClientMsgs : public CDHCPState
	{
public:
	CDHCPWaitForClientMsgs(CDHCPStateMachine& aDHCP) :
		CDHCPState(aDHCP)
		{
		}
	//create msg sends it off
	virtual CAsynchEvent* ProcessL(TRequestStatus& aStatus);
	};
	
class CDHCPHandleClientMsgs : public CDHCPState
	{
public:
	CDHCPHandleClientMsgs(CDHCPStateMachine& aDHCP) :
		CDHCPState(aDHCP)
		{
		}
	//create msg sends it off
	virtual CAsynchEvent* ProcessL(TRequestStatus& aStatus);
	};	

class CDHCPHandleDiscover : public CDHCPState
	{
public:
	CDHCPHandleDiscover(CDHCPStateMachine& aDHCP) :
		CDHCPState(aDHCP)
		{
		}
	//create msg sends it off
	virtual CAsynchEvent* ProcessL(TRequestStatus& aStatus);
	};
	
class CDHCPProvideOffer : public CDHCPState
	{
public:
	CDHCPProvideOffer(CDHCPStateMachine& aDHCP) :
		CDHCPState(aDHCP)
		{
		}
	//create msg sends it off
	virtual CAsynchEvent* ProcessL(TRequestStatus& aStatus);
	};
	
class CDHCPSendRequestResponse : public CDHCPState
	{
public:
	CDHCPSendRequestResponse(CDHCPStateMachine& aDHCP) :
		CDHCPState(aDHCP)
		{
		}
	//create msg sends it off
	virtual CAsynchEvent* ProcessL(TRequestStatus& aStatus);
	};
	
	
class CDHCPSendInformResponse : public CDHCPState
	{
public:
	CDHCPSendInformResponse(CDHCPStateMachine& aDHCP) :
		CDHCPState(aDHCP)
		{
		}
	//create msg sends it off
	virtual CAsynchEvent* ProcessL(TRequestStatus& aStatus);
	};						

class CDHCPHandleDecline : public CDHCPState
	{
public:
	CDHCPHandleDecline(CDHCPStateMachine& aDHCP) :
		CDHCPState(aDHCP)
		{
		}
	//create msg sends it off
	virtual CAsynchEvent* ProcessL(TRequestStatus& aStatus);
	};						
	
class CDHCPHandleRelease : public CDHCPState
	{
public:
	CDHCPHandleRelease(CDHCPStateMachine& aDHCP) :
		CDHCPState(aDHCP)
		{
		}
	//create msg sends it off
	virtual CAsynchEvent* ProcessL(TRequestStatus& aStatus);
	};						
	
// To implement a timer for server socket to bind with server IP address	
class CDHCPBindServer : public CDHCPState, public MExpireTimer
	{
public:
	CDHCPBindServer(CDHCPStateMachine& aDHCP) :
		CDHCPState(aDHCP)
		{
		}
	virtual void TimerExpired();

	//starts bind timer
	virtual CAsynchEvent* ProcessL(TRequestStatus& aStatus);
	
	virtual void Cancel();
protected:
	TInt iErr;
	};	
	
#endif // SYMBIAN_NETWORKING_DHCPSERVER
	
#endif
