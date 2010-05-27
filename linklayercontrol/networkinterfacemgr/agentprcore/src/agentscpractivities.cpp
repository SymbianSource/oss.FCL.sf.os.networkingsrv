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
#include <elements/nm_messages_child.h>

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

#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
namespace AgentSCprParamRequestActivity
{
DECLARE_DEFINE_NODEACTIVITY(ECFActivityParamRequest, AgentSCprParamRequest, TCFScpr::TSetParamsRequest)
    FIRST_NODEACTIVITY_ENTRY(PRStates::TAwaitingParamRequest, CoreNetStates::TNoTagOrBearerPresent)
    NODEACTIVITY_ENTRY(CoreNetStates::KBearerPresent, PRStates::TPassToServiceProvider, CoreNetStates::TAwaitingParamResponse, MeshMachine::TTag<CoreNetStates::KBearerPresent>)
    LAST_NODEACTIVITY_ENTRY(CoreNetStates::KBearerPresent, PRStates::TStoreParamsAndPostToOriginators)
    LAST_NODEACTIVITY_ENTRY(KNoTag, PRStates::TRespondWithCurrentParams)
NODEACTIVITY_END()
}
#endif //#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW

namespace AgentSCprDestroyActivity
{
//Overridden destroy for cleaning up the agent if its still about
DECLARE_DEFINE_CUSTOM_NODEACTIVITY(ECFActivityDestroy, AgentSCprDestroy, Messages::TEChild::TDestroy, CoreActivities::CDestroyActivity::New)
    FIRST_NODEACTIVITY_ENTRY(MeshMachine::TAwaitingDestroy, CoreActivities::CDestroyActivity::TNoTagBlockedByActivitiesOrLeavingDataClient)

    ROUTING_NODEACTIVITY_ENTRY(KNoTag, AgentSCprStates::TNoTagOrProviderStopped)
    NODEACTIVITY_ENTRY(KNoTag, AgentSCprStates::TStopAgent, AgentSCprStates::TAwaitingAgentDown, MeshMachine::TTag<CoreNetStates::KProviderStopped>)
    ROUTING_NODEACTIVITY_ENTRY(CoreNetStates::KProviderStopped, CoreStates::TNoTagOrNoClients)
        
    //The node mustn't go out of scope with clients present. The node must get rid of them first.
    NODEACTIVITY_ENTRY(KNoTag, CoreActivities::CDestroyActivity::TMakeClientsLeaveOrProcessClientLeave, CoreStates::TAwaitingClientLeave,  CoreActivities::CDestroyActivity::TNoTagOrNoTagBackwards)
    THROUGH_NODEACTIVITY_ENTRY(KNoTag, CoreActivities::CDestroyActivity::TProcessClientLeave, TTag<CoreNetStates::KNoClients>)

    THROUGH_NODEACTIVITY_ENTRY(CoreNetStates::KNoClients, PRStates::TProcessDestroy, MeshMachine::TNoTag)
    NODEACTIVITY_ENTRY(KNoTag, MeshMachine::TDoNothing, MeshMachine::TAwaitingLeaveComplete, CoreActivities::CDestroyActivity::TNoTagOrNoTagBackwards)
    LAST_NODEACTIVITY_ENTRY(KNoTag, CoreNetStates::TSendClientLeavingAndRemoveControlProvider)
NODEACTIVITY_END()
}
// Activity Map
namespace AgentSCprActivities
{
DEFINE_EXPORT_ACTIVITY_MAP(agentSCprActivities)
   ACTIVITY_MAP_ENTRY(AgentSCprDestroyActivity, AgentSCprDestroy)
   ACTIVITY_MAP_ENTRY(AgentSCprNotificationFromFlowActivity, AgentSCprNotificationFromFlow)
   ACTIVITY_MAP_ENTRY(AgentSCprAuthenticationActivity, AgentSCprAuthentication)
   ACTIVITY_MAP_ENTRY(AgentSCprDataClientGoneDownActivity, AgentSCprDataClientGoneDown)
   ACTIVITY_MAP_ENTRY(AgentSCprStartActivity, AgentSCprStart)
   ACTIVITY_MAP_ENTRY(AgentSCprStopActivity, AgentSCprStop)
   ACTIVITY_MAP_ENTRY(AgentSCprDataClientStoppedActivity, AgentSCprDataClientStopped)
#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
   ACTIVITY_MAP_ENTRY(AgentSCprParamRequestActivity, AgentSCprParamRequest)
#endif //#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
ACTIVITY_MAP_END_BASE(SCprActivities, coreSCprActivities)
}

