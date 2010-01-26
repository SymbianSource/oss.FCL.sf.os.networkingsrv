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


//#include <comms-infras/ss_nodestates.h>
//#include <comms-infras/corescprstates.h>
#include <comms-infras/corescpractivities.h>
#include <comms-infras/ss_nodemessages_dataclient.h>

#include "agentscprstates.h"
#include "agentscpractivities.h"
#include "agentmessages.h"

using namespace ESock;
using namespace NetStateMachine;
using namespace MeshMachine;

// Authenticate Activity
namespace AgentSCprAuthenticationActivity
{
DECLARE_DEFINE_NODEACTIVITY(ECFActivityAuthentication, AgentSCprAuthentication, TLinkMessage::TAuthenticate)
    FIRST_NODEACTIVITY_ENTRY(AgentSCprStates::TAwaitingAuthenticate, MeshMachine::TNoTag)
    NODEACTIVITY_ENTRY(KNoTag, AgentSCprStates::TSendAuthenticate, AgentSCprStates::TAwaitingAuthenticateComplete, MeshMachine::TNoTag)
    LAST_NODEACTIVITY_ENTRY(KNoTag, AgentSCprStates::TSendAuthenticateComplete)
NODEACTIVITY_END()
}

namespace AgentSCprStartActivity
{
DECLARE_DEFINE_CUSTOM_NODEACTIVITY(ECFActivityStartDataClient, AgentSCprStart, TCFDataClient::TStart, MeshMachine::CNodeRetryActivity::NewL)
	FIRST_NODEACTIVITY_ENTRY(CoreNetStates::TAwaitingDataClientStart, MeshMachine::TNoTag)
    NODEACTIVITY_ENTRY(KNoTag, AgentSCprStates::TStartAgent, CoreNetStates::TAwaitingStarted, MeshMachine::TNoTag)
    NODEACTIVITY_ENTRY(KNoTag, PRStates::TStartDataClients, CoreNetStates::TAwaitingDataClientsStarted, MeshMachine::TNoTag)
    LAST_NODEACTIVITY_ENTRY(KNoTag, PRStates::TSendDataClientStarted)
NODEACTIVITY_END()
}


namespace AgentSCprNotificationFromFlowActivity
{
DECLARE_DEFINE_NODEACTIVITY(ECFNotificationFromFlowActivity, AgentSCprNotificationFromFlow, TLinkMessage::TFlowToAgentNotification)
    FIRST_NODEACTIVITY_ENTRY(AgentSCprStates::TAwaitingNotificationFromFlow, MeshMachine::TNoTag)
    LAST_NODEACTIVITY_ENTRY(KNoTag, AgentSCprStates::TNotifyAgent)
NODEACTIVITY_END()
}

namespace AgentSCprDataClientGoneDownActivity
{
DECLARE_DEFINE_NODEACTIVITY(ECFActivityDataClientGoneDown, AgentSCprDataClientGoneDown, TCFControlProvider::TDataClientGoneDown)
    FIRST_NODEACTIVITY_ENTRY(MeshMachine::TAwaitingAny, MeshMachine::TNoTag)
    LAST_NODEACTIVITY_ENTRY(KNoTag, AgentSCprStates::TProcessDataClientGoneDown)
NODEACTIVITY_END()
}

namespace AgentSCprStopActivity
{
// Note: use of TStopYourFlows/TAwaitingFlowDown/TNoTag (i.e. no looping) as there is only one flow at the Agent level.
DECLARE_DEFINE_CUSTOM_NODEACTIVITY(ECFActivityStopDataClient, AgentSCprStop, TCFDataClient::TStop, MeshMachine::CNodeRetryActivity::NewL)
	FIRST_NODEACTIVITY_ENTRY(CoreNetStates::TAwaitingDataClientStop, PRDataClientStopActivity::TNoTagOrProviderStoppedBlockedByStart)
	NODEACTIVITY_ENTRY(KNoTag, SCprStates::TStopYourFlows, CoreNetStates::TAwaitingDataClientStopped, MeshMachine::TTag<CoreNetStates::KProviderStopped>)

	THROUGH_NODEACTIVITY_ENTRY(CoreNetStates::KProviderStopped, MeshMachine::TDoNothing, AgentSCprStates::TNoTagOrProviderStopped)
	NODEACTIVITY_ENTRY(KNoTag, AgentSCprStates::TStopAgent, AgentSCprStates::TAwaitingAgentDown, MeshMachine::TTag<CoreNetStates::KProviderStopped>)
	THROUGH_NODEACTIVITY_ENTRY(CoreNetStates::KProviderStopped, PRStates::TDestroyOrphanedDataClients, MeshMachine::TTag<CoreNetStates::KProviderStopped>)
	LAST_NODEACTIVITY_ENTRY(CoreNetStates::KProviderStopped, PRStates::TSendDataClientStopped)
NODEACTIVITY_END()
}

// This activity runs in response to the completion of an agent initiated disconnect
namespace AgentSCprDataClientStoppedActivity
{
DECLARE_DEFINE_NODEACTIVITY(ECFSelfStopDataClientStoppedActivity, AgentSCprDataClientStopped, TCFDataClient::TStopped)
	FIRST_NODEACTIVITY_ENTRY(CoreNetStates::TAwaitingDataClientStopped, AgentSCprStates::TNoTagOrProviderStarted)
	LAST_NODEACTIVITY_ENTRY(KNoTag, AgentSCprStates::TSendError)
	LAST_NODEACTIVITY_ENTRY(CoreNetStates::KProviderStarted, AgentSCprStates::TSendDataClientGoneDown)
NODEACTIVITY_END()
}


// Activity Map
namespace AgentSCprActivities
{
DEFINE_EXPORT_ACTIVITY_MAP(agentSCprActivities)
   ACTIVITY_MAP_ENTRY(AgentSCprNotificationFromFlowActivity, AgentSCprNotificationFromFlow)
   ACTIVITY_MAP_ENTRY(AgentSCprAuthenticationActivity, AgentSCprAuthentication)
   ACTIVITY_MAP_ENTRY(AgentSCprDataClientGoneDownActivity, AgentSCprDataClientGoneDown)
   ACTIVITY_MAP_ENTRY(AgentSCprStartActivity, AgentSCprStart)
   ACTIVITY_MAP_ENTRY(AgentSCprStopActivity, AgentSCprStop)
   ACTIVITY_MAP_ENTRY(AgentSCprDataClientStoppedActivity, AgentSCprDataClientStopped)
ACTIVITY_MAP_END_BASE(SCprActivities, coreSCprActivities)
}

