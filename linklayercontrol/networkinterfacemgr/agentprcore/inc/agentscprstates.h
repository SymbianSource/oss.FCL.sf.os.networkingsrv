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


#ifndef SYMBIAN_AGENTSCPRSTATES_H
#define SYMBIAN_AGENTSCPRSTATES_H

#include <comms-infras/corescprstates.h>


class CAgentSubConnectionProvider;


namespace AgentSCprStates
{
	typedef MeshMachine::TNodeContext<CAgentSubConnectionProvider, SCprStates::TContext> TContext;
    const TUint KAgentAdapterNull = 0;
    const TUint KAgentAdapterNotNull = 1;

// ---------------- Start Activity Overrides ----------------
EXPORT_DECLARE_SMELEMENT_HEADER(TStartAgent, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, AgentSCprStates::TContext)
	IMPORT_C virtual void DoL();
DECLARE_SMELEMENT_FOOTER(TStartAgent)

EXPORT_DECLARE_SMELEMENT_HEADER(TReconnectAgent, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, AgentSCprStates::TContext)
	IMPORT_C virtual void DoL();
DECLARE_SMELEMENT_FOOTER(TReconnectAgent)

EXPORT_DECLARE_SMELEMENT_HEADER(TJoinAgent, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, AgentSCprStates::TContext)
	IMPORT_C virtual void DoL();
DECLARE_SMELEMENT_FOOTER(TJoinAgent)

DECLARE_AGGREGATED_TRANSITION2(
    JoinAndStartAgent,
    AgentSCprStates::TJoinAgent,
    AgentSCprStates::TStartAgent
    )
        
// ---------------- Notification Activity ----------------
EXPORT_DECLARE_SMELEMENT_HEADER(TAwaitingNotificationFromFlow, MeshMachine::TState<TContext>, NetStateMachine::MState, AgentSCprStates::TContext)
	IMPORT_C virtual TBool Accept();
DECLARE_SMELEMENT_FOOTER(TAwaitingNotificationFromFlow)


EXPORT_DECLARE_SMELEMENT_HEADER(TNotifyAgent, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, AgentSCprStates::TContext)
	IMPORT_C virtual void DoL();
DECLARE_SMELEMENT_FOOTER(TNotifyAgent)
   


// ---------------- Authentication Activity ----------------

EXPORT_DECLARE_SMELEMENT_HEADER(TAwaitingAuthenticateComplete, MeshMachine::TState<TContext>, NetStateMachine::MState, AgentSCprStates::TContext)
	IMPORT_C virtual TBool Accept();
	IMPORT_C virtual void Cancel();
DECLARE_SMELEMENT_FOOTER(TAwaitingAuthenticateComplete)


EXPORT_DECLARE_SMELEMENT_HEADER(TSendAuthenticate, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, AgentSCprStates::TContext)
	IMPORT_C virtual void DoL();
DECLARE_SMELEMENT_FOOTER(TSendAuthenticate)

EXPORT_DECLARE_SMELEMENT_HEADER(TAwaitingAuthenticate, MeshMachine::TState<CoreStates::TContext>, NetStateMachine::MState, CoreStates::TContext)
	IMPORT_C virtual TBool Accept();
DECLARE_SMELEMENT_FOOTER(TAwaitingAuthenticate)

EXPORT_DECLARE_SMELEMENT_HEADER(TSendAuthenticateComplete, MeshMachine::TStateTransition<CoreStates::TContext>, NetStateMachine::MStateTransition, CoreStates::TContext)
	IMPORT_C virtual void DoL();
DECLARE_SMELEMENT_FOOTER(TSendAuthenticateComplete)


// ---------------- Stop Activity Overrides ----------------
EXPORT_DECLARE_SMELEMENT_HEADER( TNoTagOrProviderStopped, MeshMachine::TStateFork<TContext>, NetStateMachine::MStateFork, TContext )
	IMPORT_C virtual TInt TransitionTag();
DECLARE_SMELEMENT_FOOTER( TNoTagOrProviderStopped )

EXPORT_DECLARE_SMELEMENT_HEADER( TNoTagOrProviderStarted, MeshMachine::TStateFork<TContext>, NetStateMachine::MStateFork, TContext )
	IMPORT_C virtual TInt TransitionTag();
DECLARE_SMELEMENT_FOOTER( TNoTagOrProviderStarted )

DECLARE_SERIALIZABLE_STATE(
	NoTagOrProviderStoppedBlockedByStart,
	CoreNetStates::TActivityStartMutex,
	TNoTagOrProviderStopped
	)
	

EXPORT_DECLARE_SMELEMENT_HEADER(TStopAgent, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, AgentSCprStates::TContext)
	IMPORT_C virtual void DoL();
DECLARE_SMELEMENT_FOOTER(TStopAgent)

EXPORT_DECLARE_SMELEMENT_HEADER(TSendDataClientGoneDown, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, AgentSCprStates::TContext)
	IMPORT_C virtual void DoL();
DECLARE_SMELEMENT_FOOTER(TSendDataClientGoneDown)

EXPORT_DECLARE_SMELEMENT_HEADER(TSendError, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, AgentSCprStates::TContext)
	IMPORT_C virtual void DoL();
DECLARE_SMELEMENT_FOOTER(TSendError)

EXPORT_DECLARE_SMELEMENT_HEADER(TAwaitingAgentDown, MeshMachine::TState<TContext>, NetStateMachine::MState, AgentSCprStates::TContext)
	IMPORT_C virtual TBool Accept();
DECLARE_SMELEMENT_FOOTER(TAwaitingAgentDown)


// ---------------- Unsolicited Flow Down Activity ----------------
EXPORT_DECLARE_SMELEMENT_HEADER(TProcessDataClientGoneDown, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, AgentSCprStates::TContext)
	IMPORT_C virtual void DoL();
DECLARE_SMELEMENT_FOOTER(TDataClientGoneDown)

} // namespace AgentSCprStates

#endif
// SYMBIAN_AGENTSCPRSTATES_H


