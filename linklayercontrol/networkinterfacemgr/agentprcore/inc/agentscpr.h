// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

/**
 @file
 @internalTechnology
 @prototype
*/


#ifndef SYMBIAN_AGENTSCPR_H
#define SYMBIAN_AGENTSCPR_H

#include <comms-infras/corescpr.h>
#include <comms-infras/nifprvar.h>
#include <comms-infras/ss_metaconnprov.h>
#include <comms-infras/agentmessages.h>
#include <cdblen.h>
#include <nifman.h>

#include <comms-infras/nifprvar_internal.h>
#include <nifman_internal.h>



class CAgentAdapter;
class CAgentNotificationHandler;

namespace AgentSCprStates
{
    class TSendBindTo;
    class TJoinAgent;
    class TStartAgent;
    class TStopAgent;
    class TSendAuthenticate;
    class TAwaitingAuthenticateComplete;
    class TSendAuthenticateComplete;
    class TNotifyAgent;
    class TProcessDataClientGoneDown;
    class TNoTagOrProviderStopped;
    class TNoTagOrProviderStarted;
    class TSendDataClientGoneDown;
    class TSendError;
    class TSendDataClientStarted;
    class TAwaitingDataClientStop;
} // AgentSCprStates


class CAgentSubConnectionProvider : public CCoreSubConnectionProvider
	{
	friend class CAgentNotificationHandler;
	friend class AgentSCprStates::TSendBindTo;
	friend class AgentSCprStates::TJoinAgent;
	friend class AgentSCprStates::TStartAgent;
	friend class AgentSCprStates::TStopAgent;
	friend class AgentSCprStates::TSendAuthenticate;
	friend class AgentSCprStates::TAwaitingAuthenticateComplete;
	friend class AgentSCprStates::TSendAuthenticateComplete;
	friend class AgentSCprStates::TNotifyAgent;
	friend class AgentSCprStates::TProcessDataClientGoneDown;
	friend class AgentSCprStates::TNoTagOrProviderStopped;
	friend class AgentSCprStates::TNoTagOrProviderStarted;
	friend class AgentSCprStates::TSendDataClientGoneDown;
	friend class AgentSCprStates::TSendError;
    friend class AgentSCprStates::TSendDataClientStarted;
    friend class AgentSCprStates::TAwaitingDataClientStop;
	friend class CAgentAdapter;

public:
    IMPORT_C static CAgentSubConnectionProvider* NewL(ESock::CSubConnectionProviderFactoryBase& aFactory);
    IMPORT_C ~CAgentSubConnectionProvider();
    void StartAgentL();
    void StopAgent(TInt aReason);

protected:
	IMPORT_C CAgentSubConnectionProvider(ESock::CSubConnectionProviderFactoryBase& aFactory,
	    const MeshMachine::TNodeActivityMap& aActivityMap);
	IMPORT_C void CleanupProvisioningInfo ();

   	// Messages::ANode Interface
	IMPORT_C virtual void ReceivedL(const Messages::TRuntimeCtxId& aSender, const Messages::TNodeId& aRecipient, Messages::TSignatureBase& aMessage);

    IMPORT_C const CAgentProvisionInfo* AgentProvisionInfo () const;

    IMPORT_C void ConnectAgent (TAgentConnectType aConnectType);
    IMPORT_C TInt NotificationToAgent (TFlowToAgentEventType aEvent, TAny* aInfo);

private:
    // Methods that are called from the CAgentAdapter
    void ServiceStarted();
    void ConnectionUpL();
    void ConnectionDownL();
    void AuthenticateCompleteL(TInt aStatus);
    void PromptForReconnectComplete(TInt aStatus);
    void ProgressL(TInt aStage);
    void Error (const Elements::TStateChange& aProgress);
    void NetworkAdaptorEvent(TNetworkAdaptorEventType aEventType, TUint aEvent, const TDesC8& aEventData, TAny* aSource=NULL);
    TInt NotificationFromAgent(TAgentToFlowEventType aEvent, TAny* aInfo=NULL);

private:
    void NotificationFromFlow(TFlowToAgentEventType aEvent);
    void ProvisionAgentInfoL();
    TInt PostMessageToFlow(const Messages::TRuntimeCtxId& aSender, const Messages::TSignatureBase& aMessage);
    inline void SetActivityIdForAdapter(TUint aActivityId);
    void CancelStartOrSendStopToSelf(TInt aError);


private:
    TBool iScprOwnedNotificationHandler;
    TBool iAuthenticateInProgress;
    TBool iStopRequested;		// ETrue means an agent stop has been requested - not that the SCPr has received a TCFDataClient::TStop
    TInt iStoppingReason;

    Elements::TStateChange iLastProgress;
    TBuf<KCommsDbSvrMaxUserIdPassLength> iUsername;
    TBuf<KCommsDbSvrMaxUserIdPassLength> iPassword;
    TUint iActivityIdForAdapter;
    };


inline void CAgentSubConnectionProvider::SetActivityIdForAdapter(TUint aActivityId)
	{
	iActivityIdForAdapter = aActivityId;
	}


#endif
// SYMBIAN_AGENTSCPR_H

