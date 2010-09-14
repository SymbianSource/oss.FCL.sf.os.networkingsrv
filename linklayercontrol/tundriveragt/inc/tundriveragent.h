/**
*   Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
*   All rights reserved.
*   This component and the accompanying materials are made available
*   under the terms of "Eclipse Public License v1.0"
*   which accompanies this distribution, and is available
*   at the URL "http://www.eclipse.org/legal/epl-v10.html".
*   
*   Initial Contributors:
*   Nokia Corporation - initial contribution.
*   
*   Contributors:
*
*   Description:
*   Header file TunDriver Agent
* 
*
*/

/**
 @file tundriveragent.h
 @internalTechnology
*/


#if !defined __TUNDRIVERAGT_H__
#define __TUNDRIVERAGT_H__

#include <comms-infras/cagentbase.h>
#include <comms-infras/agentmessages.h>
#include "tundriveragtprog.h"

#ifndef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <comms-infras/nifprvar_internal.h>
#include <nifman_internal.h>
#endif

const TInt KMajorVersionNumber=1; //< TunDriver Agent MajorVersionNumber
const TInt KMinorVersionNumber=0; //< TunDriver Agent MinorVersionNumber
const TInt KBuildVersionNumber=1; //< TunDriver Agent BuildVersionNumber

_LIT(KTunDriverAgentName,"tundriveragt"); //< Name of the TunDriver Agent


class CTunDriverAgentFactory : public CNifAgentFactory
/**
A Factory for creating a TunDriverAgent

@internalComponent  
*/
	{
protected:
	void InstallL();
	CNifAgentBase *NewAgentL(const TDesC& aName);
	TInt Info(TNifAgentInfo& aInfo, TInt aIndex) const;
	};

class CTunDriverAgent : public CAgentBase
/**
The CTunDriverAgent class owns a CAsyncCallback , which is used to control the asynchronous 
ServiceStarted() and DisconnectComplete() call from the Agent to Nifman.

@internalComponent
*/
	{
public:
	static CTunDriverAgent* NewL();
	virtual ~CTunDriverAgent();
protected:
	void ConstructL();
	CTunDriverAgent();
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

	TInt GetExcessData(TDes8& aBuffer);
	TInt Notification(TNifToAgentEventType aEvent, TAny* aInfo);
	void GetLastError(TInt& aError);
	TBool IsReconnect() const {return (ETrue);};

	TInt IncomingConnectionReceived();
	
private:
	static TInt ServiceStartedCb(TAny* aThisPtr);
	static TInt ConnectCompleteCb(TAny* aThisPtr);
	static TInt DisconnectCompleteCb(TAny* aThisPtr);

private:
	CAsyncCallBack iServiceStartedCallback;
	CAsyncCallBack iConnectCompleteCallback;
	CAsyncCallBack iDisconnectCallback;
	TBool iConnected;
	TBool iCancelled;
	
	/** An integer to keep last error code in memory. */
	TInt  iLastErrorCode;
	    
	/** Boolean to define wether disconnecting operation is ongoing */
	TBool iDisconnecting;
	TTunDriverAgentProgress iAgentProgress;
	};

#endif // _TUNDRIVERAGT_H
