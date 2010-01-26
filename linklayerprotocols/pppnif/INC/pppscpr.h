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
// PPP SCpr
// 
//

/**
 @file
 @internalComponent
*/

#ifndef SYMBIAN_PPPSCPR_H
#define SYMBIAN_PPPSCPR_H

#include <comms-infras/agentscpr.h>
#include <comms-infras/agentscprstates.h>
#include <comms-infras/agentscpractivities.h>


enum TPppSCprActivities
    {
    ECFPppLinkStatusChangeActivity = ECFAgentSCprCustomActivityBase
    };


namespace PppSCprStates
    {
    class TProcessPppLinkStatusChange;
    }


/**
@internalTechnology
@released Since 9.4

PPP subconnection provider
*/
class CPppSubConnectionProvider : public CAgentSubConnectionProvider
    {
    friend class PppSCprStates::TProcessPppLinkStatusChange;

public:
	static CPppSubConnectionProvider* NewL(ESock::CSubConnectionProviderFactoryBase& aFactory);

private:
	CPppSubConnectionProvider(ESock::CSubConnectionProviderFactoryBase& aFactory,
	    const MeshMachine::TNodeActivityMap& aActivityMap);
    };





//-=========================================================
//
// States & Transitions
//
//-=========================================================

namespace PppSCprStates
{
typedef MeshMachine::TNodeContext<CPppSubConnectionProvider, AgentSCprStates::TContext> TContext;

DECLARE_SMELEMENT_HEADER( TAwaitingPppLinkStatusChange, MeshMachine::TState<TContext>, NetStateMachine::MState, PppSCprStates::TContext)
	virtual TBool Accept();
DECLARE_SMELEMENT_FOOTER( TAwaitingPppLinkStatusChange)


DECLARE_SMELEMENT_HEADER( TProcessPppLinkStatusChange, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, PppSCprStates::TContext)
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TProcessPppLinkStatusChange)
}

#endif
// SYMBIAN_PPPSCPR_H
