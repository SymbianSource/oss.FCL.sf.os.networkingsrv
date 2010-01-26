// Copyright (c) 2000-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Header for defining a dummy version of the CNifAgentRef class
// for testing purposes
// 
//

#if !defined(__TNIFAGENTREF_H__)
#define __TNIFAGENTREF_H__

#include <comms-infras/nifagt.h>
#include "TNifNotify.h"

class CNifAgentBase;
class CNifAgentRef : public CBase, public MNifAgentNotify
/**
@internalComponent
*/
	{
public:
	// Factory 
	IMPORT_C static CNifAgentRef* NewL(MDummyNifToAgtHarnessNotify *aNotify,
                                       const TBool aCSDAgent = true);
	
	// Destructor
	IMPORT_C ~CNifAgentRef();

	// Test commands to drive the Agent
	IMPORT_C void Connect();
	IMPORT_C void ReConnect();
	IMPORT_C void Disconnect();
	IMPORT_C void WaitForIncoming();

	// Retrieve Completion return code
	IMPORT_C void GetCompletionCode(TInt& aCompletionCode);
	IMPORT_C void GetProgressStage(TInt& aProgressStage);
	
	// MNifAgentNotify virtuals
	virtual void ConnectComplete(TInt aStatus);
	virtual void ReconnectComplete(TInt aStatus);
	virtual void AuthenticateComplete(TInt aStatus);
	virtual void ServiceStarted();
	virtual void ServiceClosed();
	virtual void DisconnectComplete();
	virtual void AgentProgress(TInt aStage, TInt aError);
	virtual void AgentProgress(TSubConnectionUniqueId aSubConnectionUniqueId, TInt aStage, TInt aError);
	virtual TInt Notification(TAgentToNifEventType aEvent, TAny* aInfo);
	virtual TInt IncomingConnectionReceived();
	virtual void AgentEvent(TNetworkAdaptorEventType aEventType, TUint aEvent, const TDesC8& aEventData, TAny* aSource);
    virtual TName Name() const;
	virtual void Close();

private:
	// Constructor
	CNifAgentRef(MDummyNifToAgtHarnessNotify *aNotify);

    // Agent Constructor
	void ConstructL(const TBool aCSDAgent = true);

private:
	CNifAgentBase* iAgent;
	CNifAgentBase* iAgentInfo;
	MDummyNifToAgtHarnessNotify* iNotify;
	TName iName;
	TInt iCompletionCode;
	TInt iProgressStage;
	RLibrary iLibrary;
	TBool iDisconnectRequired;
	};

#endif
