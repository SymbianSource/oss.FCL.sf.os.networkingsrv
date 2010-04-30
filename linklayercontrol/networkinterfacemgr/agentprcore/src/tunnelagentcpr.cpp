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

#include <comms-infras/ss_log.h>
#include <elements/sm_core.h>
#include <comms-infras/corecpractivities.h>

#include "tunnelagentcpr.h"
#include "tunnelagentcprstates.h"
#include <comms-infras/agentmessages.h>
#include <comms-infras/agentcprstates.h>
#include <comms-infras/agentcpr.h>
#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
#include <comms-infras/ss_nodemessages_subconn.h>
#endif // SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
#include <comms-infras/ss_msgintercept.h>

#if defined __CFLOG_ACTIVE || defined SYMBIAN_TRACE_ENABLE
#define KTunnelAgentCprTag KESockTunnelConnectionTag
_LIT8(KTunnelAgentCprTag, "tunnelagentcpr");
#endif


using namespace Messages;
using namespace MeshMachine;
using namespace ESock;
using namespace NetStateMachine;
using namespace AgentCprStates;
using namespace TunnelAgentCprStates;

//We reserve space for two preallocated activities that may start concurrently on the CPR
//node: destroy and data client stop.
static const TUint KDefaultMaxPreallocatedActivityCount = 2;
static const TUint KMaxPreallocatedActivitySize = sizeof(MeshMachine::CNodeRetryParallelActivity) + sizeof(MeshMachine::APreallocatedOriginators<4>);
static const TUint KTunnelCPRPreallocatedActivityBufferSize = KDefaultMaxPreallocatedActivityCount * KMaxPreallocatedActivitySize;

namespace TunnelAgentCprStartActivity
{
DECLARE_DEFINE_CUSTOM_NODEACTIVITY(ECFActivityStart, TunnelCprStart, TCFServiceProvider::TStart, PRActivities::CStartActivity::NewL)
    FIRST_NODEACTIVITY_ENTRY(CoreNetStates::TAwaitingStart, CoreNetStates::TNoTagBlockedByStop)
    NODEACTIVITY_ENTRY(KNoTag, CoreNetStates::TStartSelf, CoreNetStates::TAwaitingDataClientStarted, MeshMachine::TNoTagOrErrorTag)

    NODEACTIVITY_ENTRY(KNoTag, TunnelAgentCprStates::TJoinRealIAP, CoreStates::TAwaitingJoinComplete, MeshMachine::TNoTag)
    LAST_NODEACTIVITY_ENTRY(KNoTag, CoreNetStates::TSendStarted)
    
    NODEACTIVITY_ENTRY(KErrorTag, CoreNetStates::TStopSelf, CoreNetStates::TAwaitingDataClientStopped, MeshMachine::TErrorTag)
    LAST_NODEACTIVITY_ENTRY(KErrorTag, MeshMachine::TDoNothing)
NODEACTIVITY_END()
}
namespace TunnelGoneDownActivity
{
DECLARE_DEFINE_NODEACTIVITY(ECFActivityGoneDown, TunnelGoneDown, TCFControlClient::TGoneDown)
    // Our Service Provider has gone down unexpectedly (we haven't issued a TStop)
    FIRST_NODEACTIVITY_ENTRY(CoreNetStates::TAwaitingGoneDown, MeshMachine::TNoTag)
    LAST_NODEACTIVITY_ENTRY(KNoTag, MeshMachine::TDoNothing)
NODEACTIVITY_END()
}

#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
namespace TunnelAgentCprNotificationActivity
{
DECLARE_DEFINE_NODEACTIVITY(ECFActivityNotification, TunnelAgentCprNotification, TCFSubConnControlClient::TPlaneNotification)
	NODEACTIVITY_ENTRY(KNoTag, CoreNetStates::TPassPlaneEventToControlClients, CoreNetStates::TAwaitingConEvent, MeshMachine::TNoTag)
NODEACTIVITY_END()
}
#endif // SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
namespace TunnelAgentCprStates
{
DECLARE_DEFINE_ACTIVITY_MAP(TunnelAgentCprActivities)
	ACTIVITY_MAP_ENTRY(TunnelAgentCprStartActivity, TunnelCprStart)
      ACTIVITY_MAP_ENTRY(TunnelGoneDownActivity, TunnelGoneDown)
#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	ACTIVITY_MAP_ENTRY(TunnelAgentCprNotificationActivity, TunnelAgentCprNotification)
#endif // SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
ACTIVITY_MAP_END_BASE(AgentCprStates, agentCprActivities)
}


/**
Creates an Tunnel Agent Connection Provider
@param aFactory The parent factory which has created the Cpr
@return Pointer to the newly created Cpr
*/
EXPORT_C CTunnelAgentConnectionProvider* CTunnelAgentConnectionProvider::NewL(ESock::CConnectionProviderFactoryBase& aFactory)
	{
	CTunnelAgentConnectionProvider* self = new (ELeave) CTunnelAgentConnectionProvider(aFactory);
    CleanupStack::PushL(self);
    self->ConstructL(KTunnelCPRPreallocatedActivityBufferSize);
    CleanupStack::Pop(self);
	return self;
	}


CTunnelAgentConnectionProvider::CTunnelAgentConnectionProvider(CConnectionProviderFactoryBase& aFactory)
   : CAgentConnectionProvider(aFactory, TunnelAgentCprStates::TunnelAgentCprActivities::Self())
	{
	LOG_NODE_CREATE(KTunnelAgentCprTag, CTunnelAgentConnectionProvider);
	}


/**
D'tor
*/
EXPORT_C CTunnelAgentConnectionProvider::~CTunnelAgentConnectionProvider()
	{
	LOG_NODE_DESTROY(KTunnelAgentCprTag, CTunnelAgentConnectionProvider);
	}



