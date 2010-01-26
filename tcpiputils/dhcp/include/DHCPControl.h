// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// The DHCP Control header file
// 
//

/**
 @file DHCPControl.h
*/

#ifndef DHCPCONTROL_H
#define DHCPCONTROL_H

#include <e32base.h>
#include <es_enum.h>
#include <comms-infras/statemachine.h>
#include "ExpireTimer.h"
#ifdef SYMBIAN_NETWORKING_PLATSEC
#include <comms-infras/rconfigdaemonmess.h>
#else
#include <comms-infras/cs_daemonmess.h>
#endif

class CDHCPStateMachine;
class CDHCPDb;
class CDHCPConfigListener;
#ifdef SYMBIAN_NETWORKING_ADDRESS_PROVISION
class CDhcpHwAddrManager;
#endif //SYMBIAN_NETWORKING_ADDRESS_PROVISION
class CDHCPControl : public CBase, public MStateMachineNotify, public MExpireTimer
/**
  * Base class for DHCP control
  * 
  *
  * @internalTechnology
  *
  */
	{
public:
	virtual ~CDHCPControl();
	
	virtual void ConfigureL(const TConnectionInfo& aInfo, const RMessage2* aMessage);
	virtual void Cancel();
	void HandleClientRequestL(const RMessage2& aMessage);	
		
	virtual TBool OnCompletion(CStateMachine* aStateMachine); 

	virtual void TimerExpired();	//timer callback
	
	virtual void LinkLocalCreated()	{}

public:
	enum TConfigType
		{
		EConfigToBeDecided,
		EConfigIPAddress,
		EConfigNoIPAddress
		};

protected:
	CDHCPControl(RSocketServ& aEsock,TConfigType aConfigType) :
		 iEsock(aEsock),
       iState(EStart),
		 iConfigType( aConfigType )
			 {
			 }
			 
	virtual TInt HandleClientRequestL(TUint aName, TDes8* aDes);
	virtual TInt HandleClientRequestL(TUint aName);	
	virtual TInt HandleClientRequestL(TUint aName, TInt aValue);
	void SaveAndHandleClientRequestL(const RMessage2& aMessage,TUint aOptionName,TInt aValue = 0);	 

	void HandleInterfaceDebugL(const RMessage2& aMessage);

	virtual void TaskCompleteL(TInt aError);
	virtual void HandleGetRawOptionDataL(TDes8* aDes) = 0;
	virtual void HandleGetSipServerAddrL(TDes8* aDes) = 0;
	virtual void HandleGetSipServerDomainL(TDes8* aDes) = 0;
#ifdef SYMBIAN_TCPIPDHCP_UPDATE
	virtual void HandleGetDomainSearchListL(TDes8* /* aDes */){};
	virtual void HandleGetDNSServerListL(TDes8* /*aDes */){};
#endif //SYMBIAN_TCPIPDHCP_UPDATE
#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
	virtual void HandleGetMultipleParamsL(TDes8& aDesc)=0;
	virtual void HandleGetTftpServerAddrL(TDes8& aDes)=0 ;
	virtual void HandleGetTftpServerNameL(TDes8& aDes)=0 ;
	virtual	TInt InformCompleteRequestHandlerL()=0;
	virtual void GetDhcpHdrSiaddrL(TDes8& aNxtAddress)=0;
	virtual void GetDhcpHdrSnameL(TDes8& aHdrSvrName)=0;
#endif // SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
	
	void FindInterfaceNameL(const TConnectionInfo& aInfo, TInt aFamily);
	TBool InformNegotiationIsRequiredForConnectionStartCompletion(void) const;
	virtual TBool ShouldInformAfterFailedInit();

	TBool CompleteClientMessage(TInt aError, TInt aFunctionToCancel = -1);
	TBool CompleteClientIoctlMessage(TInt aError) {return CompleteClientMessage(aError, EConfigDaemonIoctl);}
	TBool CompleteClientConfigureMessage(TInt aError) {return CompleteClientMessage(aError, EConfigDaemonConfigure);}
#ifdef SYMBIAN_NETWORKING_DHCPSERVER	
	TBool CompleteServerConfigureMessage(TInt aError) {return CompleteClientMessage(aError, EConfigDaemonConfigure);}
	TBool CompleteServerIoctlMessage(TInt aError) {return CompleteClientMessage(aError, EConfigDaemonIoctl);}
	virtual void HandleSetRawOptionCodeL(TDes8* aDes);
	void ServiceAnyOutstandingServerIoctlL();
#endif // SYMBIAN_NETWORKING_DHCPSERVER	
	void ServiceAnyOutstandingIoctlL();
  	void UpdateDns(TDesC8* aHostName, TDesC8* aDomainName);
	void ConfigureL( TBool aStaticAddress );
   	virtual void BindingFinishedL();
   	void SaveMessageBufferForLaterReference();


protected:
	CDHCPStateMachine* iDhcpStateMachine;	// owns (created by subclass)
	const RMessage2* iMessage;
	RSocketServ& iEsock;
	RConnection iConnection;
	TName iInterfaceName;

	CDHCPDb* iDhcpDb; //owns (created by subclass)
	CDHCPConfigListener* iDhcpConfigListener; // owns
#ifdef SYMBIAN_NETWORKING_ADDRESS_PROVISION
	CDhcpHwAddrManager* iDhcpHwAddrManager; //owns (created by subclass)
#endif //SYMBIAN_NETWORKING_ADDRESS_PROVISION

protected:
	CExpireTimer* iTimer;   // owns
	enum TState
		{
		EStart,
		EInitInProgress,
		EInitialised,
		ERenewInProgress,
		ERebindInProgress,
		EDeclineInProgress,
		EReleaseInProgress,
		EInformInProgress,
		EDeferredInform, // awaiting an ioctl call to trigger an inform negotiation
		EReconfigureInProgress,
		EDeclineInitialisedInProgress,
		EEnd
		};
	TState iState;
	TBool iInitStartedByRenew;
	TBool iDhcpDaemonDeregister; // to decide RemoveConfigureAddress not to be called while completion of DHCPREL msg
	TConfigType iConfigType;
	RBuf8 iValidMsg; //to save msg buffer in case user wants to retrieve any info
#ifdef SYMBIAN_NETWORKING_DHCPSERVER
	HBufC8* iDNSRawOption;
public:
	TBool iDHCPServerImpl;
#endif // SYMBIAN_NETWORKING_DHCPSERVER		
	};

#endif

