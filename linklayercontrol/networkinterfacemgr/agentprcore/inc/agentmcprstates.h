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


#ifndef SYMBIAN_AGENTMCPRSTATES_H
#define SYMBIAN_AGENTMCPRSTATES_H

#include <comms-infras/coremcprstates.h>


class CAgentMetaConnectionProvider;


namespace AgentMCprStates
{
typedef MeshMachine::TNodeContext<CAgentMetaConnectionProvider, MCprStates::TContext> TContext;

// ---------------- NoBearer Activity Overrides ----------------

EXPORT_DECLARE_SMELEMENT_HEADER( TSendBindTo, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, AgentMCprStates::TContext )
	IMPORT_C virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TSendBindTo )


// ---------------- Start Meta Connection Activity Overrides ----------------

EXPORT_DECLARE_SMELEMENT_HEADER( TSendProvision, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, AgentMCprStates::TContext )
	IMPORT_C virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TSendProvision )

} // namespace AgentMCprStates




#endif
// SYMBIAN_AGENTMCPRSTATES_H


