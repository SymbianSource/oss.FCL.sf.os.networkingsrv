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


//#include <ss_nodestates.h>

#include <comms-infras/ss_coreprstates.h>

#include <comms-infras/coremcprstates.h>
#include <comms-infras/coremcpractivities.h>
#include "agentmcpractivities.h"


using namespace ESock;
using namespace NetStateMachine;
using namespace CoreStates;


// Activity Map
namespace AgentMCprActivities
{
DEFINE_EXPORT_ACTIVITY_MAP(agentMCprActivities)
ACTIVITY_MAP_END_BASE(MCprActivities, coreMCprActivities)
} // namespace AgentMCprActivities


