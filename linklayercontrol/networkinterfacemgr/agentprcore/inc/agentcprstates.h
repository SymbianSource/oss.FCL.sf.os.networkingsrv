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


#ifndef SYMBIAN_AGENTCPRSTATES_H
#define SYMBIAN_AGENTCPRSTATES_H

#include <comms-infras/corecprstates.h>

class CAgentConnectionProvider;

namespace AgentCprStates
{
typedef MeshMachine::TNodeContext<CAgentConnectionProvider, CprStates::TContext> TContext;

const TUint KAgentAdapterNull = 0;
const TUint KAgentAdapterNotNull = 1;


// ---------------- NoBearer Activity Overrides ----------------
DECLARE_SMELEMENT_HEADER(TSendBindTo, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, AgentCprStates::TContext)
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER(TSendBindTo)

#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
DECLARE_SMELEMENT_HEADER( TUpdateBundle, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, AgentCprStates::TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TUpdateBundle)

DECLARE_AGGREGATED_TRANSITION2(
   TUpdateBundleAndRespondWithRetrievedParams,
   TUpdateBundle,
   PRStates::TRespondWithRetrievedParams
   )
#endif //SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW

} // namespace AgentCprStates

#endif
// SYMBIAN_AGENTCPRSTATES_H


