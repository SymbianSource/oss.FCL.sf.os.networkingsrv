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
// The DHCP IPv4 Control header file
// 
//

/**
 @file DHCPIP4Control.h
*/

#ifndef __DHCPIP4CONTROL_H__
#define __DHCPIP4CONTROL_H__

#include <e32base.h>
#include "DHCPControl.h"
#include "DHCPIP4StateMachine.h"

class RSocketServ;
class CDHCPIP4Control : public CDHCPControl
/**
  * Implements the DHCP IP4 control plain
  * class owns DHCP IP4 state machine (CDHCPIP4StateMachine)
  *
  * @internalTechnology
  *
  */
	{
public:
	CDHCPIP4Control(RSocketServ& aEsock, TConfigType aConfigType);
	virtual ~CDHCPIP4Control();

	virtual void ConfigureL(const TConnectionInfo& aInfo, const RMessage2* aMessage);
	virtual void Cancel();
	virtual void TimerExpired();	//timer callback

	virtual TInt HandleClientRequestL(TUint aName);
	virtual TInt HandleClientRequestL(TUint aName, TInt aValue);
	
	virtual void TaskCompleteL( TInt aError );
	
	virtual void LinkLocalCreated();

protected:
	CDHCPIP4StateMachine* DhcpStateMachine();
	virtual void HandleGetRawOptionDataL(TDes8* aDes);
	virtual void HandleGetSipServerAddrL(TDes8* aDes);
	virtual void HandleGetSipServerDomainL(TDes8* aDes);
	void GetRawOptionDataL(TUint aOpCode, TPtr8& aPtr );
   	virtual void BindingFinishedL();
   	virtual TBool ShouldInformAfterFailedInit(void);
#ifdef  SYMBIAN_NETWORKING_DHCPSERVER	
	virtual void HandleSetRawOptionCodeL(TDes8* aDes);
	void GetRawOptionDataFromDNSBufL(TPtr8& aPtr);
	void SetRawOptionCodeL(TDes8* aDes);
#endif // SYMBIAN_NETWORKING_DHCPSERVER   	
#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
	virtual void HandleGetTftpServerAddrL(TDes8& aDes) ;
	virtual void HandleGetTftpServerNameL(TDes8& aDes) ;
	virtual void HandleGetMultipleParamsL(TDes8& aReqList);
	virtual	TInt InformCompleteRequestHandlerL();
	void RequestInformOrCompleteCallL(TPtr8& aOpcode);
	virtual void GetDhcpHdrSiaddrL(TDes8& aNxtAddress);
	virtual void GetDhcpHdrSnameL(TDes8& aHdrSvrName);
#endif // SYMBIAN_NETWORKING_DHCP_MSG_HEADERS	

private:

	TBool iStaticAddress;
	TBool iClientShouldCompleteWhenLinkLocalCreated;
	};

inline CDHCPIP4Control::CDHCPIP4Control(RSocketServ& aEsock, TConfigType aConfigType) : 
	CDHCPControl(aEsock,aConfigType)
	{
	}

inline CDHCPIP4StateMachine* CDHCPIP4Control::DhcpStateMachine()
{
   return static_cast<CDHCPIP4StateMachine*>(iDhcpStateMachine);
}

#endif

