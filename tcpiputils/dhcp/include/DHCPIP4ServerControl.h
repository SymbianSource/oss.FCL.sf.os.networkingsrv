// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// The DHCP IPv4 Server Control header file
// 
//

/**
 @file DHCPIP4ServerControl.h
*/

#ifndef __DHCPIP4SERVERCONTROL_H__
#define __DHCPIP4SERVERCONTROL_H__

#include <e32base.h>
#include "DHCPIP4Control.h"
#include "DHCPIP4StateMachine.h"

#ifdef SYMBIAN_DNS_PROXY
#include "dnsproxyclient.h"
class CDNSProxyUpdateIf;
#endif // SYMBIAN_DNS_PROXY

#ifdef SYMBIAN_DNS_PROXY
const TInt KMaxAddStr = 50;
#endif

class RSocketServ;
class CDHCPIP4ServerControl : public CDHCPIP4Control
/** This class is the control object for DHCPv4 Server implementation
  * It maintains the upper states for the DHCP server implementation - The upper states are
  * - Wait For DISCOVER / INFORM
  * - Wait For RENEW / RELEASE
  * - Wait For REBIND / RELEASE
  */
	{
public:
	CDHCPIP4ServerControl(RSocketServ& aEsock, TConfigType aConfigType);
	virtual ~CDHCPIP4ServerControl();

	virtual void ConfigureL(const TConnectionInfo& aInfo, const RMessage2* aMessage);
	virtual void TimerExpired();	//timer callback

protected:
	CDHCPIP4StateMachine* DhcpStateMachine();
   	void TaskCompleteL(TInt aError);

private:
#ifdef SYMBIAN_DNS_PROXY
    void ConfigureDnsProxyL(const RMessage2* aMessage);
#endif // SYMBIAN_DNS_PROXY
    
private:
#ifdef SYMBIAN_DNS_PROXY
	CDNSProxyUpdateIf* iDnsProxyPlugin;
    RDNSClient     iDnsProxySession;
    TRequestStatus iProxyStatus;
    TInetAddr      iInterfaceAddr;
    TInetAddr      iOfferedAddr;
    TBool          iUpdateDnsProxyDb;
    TBuf<KMaxAddStr> iAddrStr;
#endif // SYMBIAN_DNS_PROXY

	};

inline CDHCPIP4ServerControl::CDHCPIP4ServerControl(RSocketServ& aEsock, TConfigType aConfigType) :
	CDHCPIP4Control(aEsock,aConfigType)
	{
	}

inline CDHCPIP4StateMachine* CDHCPIP4ServerControl::DhcpStateMachine()
	{
   	return static_cast<CDHCPIP4StateMachine*>(iDhcpStateMachine);
	}

#endif // __DHCPIP4SERVERCONTROL_H__

