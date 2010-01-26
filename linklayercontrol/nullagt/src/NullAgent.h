// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// NullAgent
// 
//

/**
 @file 
 @internalComponent
*/


#if !defined(__NULL_AGENT_H__)
#define __NULL_AGENT_H__

#include <comms-infras/cagentbase.h>
#include <comms-infras/ni_log.h>
#include "nullagtprog.h"
#include <etelpckt.h> 
#ifndef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <comms-infras/nifprvar_internal.h>
#include <nifman_internal.h>
#endif

const TInt KMajorVersionNumber=8; //< Null Agent MajorVersionNumber
const TInt KMinorVersionNumber=0; //< Null Agent MinorVersionNumber
const TInt KBuildVersionNumber=1; //< Null Agent BuildVersionNumber

namespace NullAgent
	{
/**
panic codes for a CNullAgent
*/
	enum TNullAgentPanic
		{
		ENullNifmanNotifyPointer,
		ENullTAnyPointer
		};
}

_LIT(KNullAgentName,"nullagt"); //< Name of the Null Agent


GLDEF_C void NullAgentPanic(NullAgent::TNullAgentPanic aPanic);

class CNullAgentFactory : public CNifAgentFactory
/**
A Factory for creating a NullAgent

@internalComponent  
*/
	{
protected:
	void InstallL();
	CNifAgentBase *NewAgentL(const TDesC& aName);
	TInt Info(TNifAgentInfo& aInfo, TInt aIndex) const;
	};

class CNullAgent : public CAgentBase
/**
The CNullAgent class owns a CAsyncCallback , which is used to control the asynchronous 
ServiceStarted() and DisconnectComplete() call from the Agent to Nifman.

@internalComponent
*/
	{
public:
	static CNullAgent* NewL();
	virtual ~CNullAgent();
protected:
	void ConstructL();
	CNullAgent();
public:

	// from CNifAgentBase
	void Info(TNifAgentInfo& aInfo) const;
	void Connect(TAgentConnectType aType);
	void Connect(TAgentConnectType aType, CStoreableOverrideSettings* aOverrideSettings);
	void CancelConnect();
	void Disconnect(TInt aReason);
	void ServiceStarted(TInt aError);
	void ConnectionComplete(TInt aError);
	void DisconnectionComplete();

//	void MDPOLoginComplete(TInt aError);
//	void MDPOReadPctComplete(TInt aError);
//	void MDPODestroyPctComplete(TInt aError);
//	void MDPOQoSWarningComplete(TInt aError, TBool aResponse);

	TInt GetExcessData(TDes8& aBuffer);
	TInt Notification(TNifToAgentEventType aEvent, TAny* aInfo);
	void GetLastError(TInt& aError);
	TBool IsReconnect() const {return (ETrue);};

	TInt IncomingConnectionReceived();
	
#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW	
	virtual TUint32 GetBearerInfo() const ;
#endif //SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW

private:
	static TInt ServiceStartedCb(TAny* aThisPtr);
	static TInt ConnectCompleteCb(TAny* aThisPtr);
	static TInt DisconnectCompleteCb(TAny* aThisPtr);
	
	TUint CommDbModemBearerRate();

private:
	CAsyncCallBack iServiceStartedCallback;
	CAsyncCallBack iConnectCompleteCallback;
	CAsyncCallBack iDisconnectCallback;
	TBool iConnected;
	TBool iCancelled;
	
	/**
	TSY Configuration parameters 
	Used for provisioning RawIP NIF in a test environment only.	*/
	RPacketContext::TContextConfigGPRS iTsyConfig;
	};

#endif
