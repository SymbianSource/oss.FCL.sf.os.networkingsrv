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
// The DHCP IPv6 Control header file
// 
//

/**
 @file DHCPIP6Control.h
*/

#ifndef DHCPIP6CONTROL_H
#define DHCPIP6CONTROL_H

#include <e32base.h>
#include "DHCPControl.h"
#include "DHCPIP6StateMachine.h"

class CDHCPIP6Control : public CDHCPControl
/**
  * Implements the DHCP IP6 highest level state machine that controls
  * CDHCPIP6StateMachine tasks and transitions between them.
  * The class owns DHCP IP6 state machine (CDHCPIP6StateMachine)
  *
  * @internalTechnology
  *
  */
	{
public:
	CDHCPIP6Control(RSocketServ& aEsock, TConfigType aConfigType);
	virtual ~CDHCPIP6Control();

	virtual void ConfigureL(const TConnectionInfo& aInfo, const RMessage2* aMessage);

protected:

	virtual void TaskCompleteL(TInt aError);
	virtual void HandleGetRawOptionDataL(TDes8* aDes);
	virtual void HandleGetSipServerAddrL(TDes8* aDes);
	virtual void HandleGetSipServerDomainL(TDes8* aDes);
#ifdef SYMBIAN_TCPIPDHCP_UPDATE
	virtual void HandleGetDomainSearchListL(TDes8* aDes);
	virtual void HandleGetDNSServerListL(TDes8* aDes);
#endif //SYMBIAN_TCPIPDHCP_UPDATE
	virtual void GetRawOptionDataL(TUint aOpCode, TPtr8& aPtr );
	virtual void BindingFinishedL();
	virtual TInt HandleClientRequestL(TUint aName);	
	virtual TInt HandleClientRequestL(TUint aName, TInt aValue);	
#ifdef SYMBIAN_TCPIPDHCP_UPDATE	
	void RequestInformOrCompleteCallL(TDes8& aOpcode);
#endif //SYMBIAN_TCPIPDHCP_UPDATE
#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
	virtual void HandleGetTftpServerAddrL(TDes8& aDes) ; 
	virtual void HandleGetTftpServerNameL(TDes8& aDes);
	virtual void HandleGetMultipleParamsL(TDes8& aDesc);
	virtual void GetDhcpHdrSiaddrL(TDes8& aNxtAddress);
	virtual void GetDhcpHdrSnameL(TDes8& aHdrSvrName);
	virtual	TInt InformCompleteRequestHandlerL();
#endif //SYMBIAN_NETWORKING_DHCP_MSG_HEADERS

protected:
	CDHCPIP6StateMachine* DhcpStateMachine();
	};

inline CDHCPIP6Control::CDHCPIP6Control(RSocketServ& aEsock, TConfigType aConfigType) : 
   CDHCPControl(aEsock, aConfigType)
	{
	}

inline CDHCPIP6StateMachine* CDHCPIP6Control::DhcpStateMachine()
{
   return static_cast<CDHCPIP6StateMachine*>(iDhcpStateMachine);
}

#endif

