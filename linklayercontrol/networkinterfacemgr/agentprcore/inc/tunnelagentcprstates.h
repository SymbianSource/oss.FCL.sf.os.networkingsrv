// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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


#ifndef SYMBIAN_TUNNELAGENTCPRSTATES_H
#define SYMBIAN_TUNNELAGENTCPRSTATES_H

#include "agentcprstates.h"
#include "tunnelagentcpr.h"

class CTunnelAgentConnectionProvider;


namespace TunnelAgentCprStates
{
enum TTunnelAgentCprPanics
	{
	ETunnelAgentCprNoTunnelService = 2,
	ETunnelAgentCprNoServiceProvider = 3,
	};

typedef MeshMachine::TNodeContext<CTunnelAgentConnectionProvider, CprStates::TContext> TContext;

const TUint KAgentAdapterNull = 0;
const TUint KAgentAdapterNotNull = 1;


EXPORT_DECLARE_SMELEMENT_HEADER(TJoinRealIAP, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, TunnelAgentCprStates::TContext)
	IMPORT_C virtual void DoL();
DECLARE_SMELEMENT_FOOTER(TJoinRealIAP)

} // namespace TunnelAgentCprStates

#endif
// SYMBIAN_TUNNELAGENTCPRSTATES_H


