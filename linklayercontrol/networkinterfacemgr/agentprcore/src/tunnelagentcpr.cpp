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
#include <comms-infras/ss_corepractivities.h>

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
    self->ConstructL();
    CleanupStack::Pop(self);
	return self;
	}


CTunnelAgentConnectionProvider::CTunnelAgentConnectionProvider(CConnectionProviderFactoryBase& aFactory)
   : CAgentConnectionProvider(aFactory, TunnelAgentCprStates::TunnelAgentCprActivities::Self()),
     TIfStaticFetcherNearestInHierarchy(this)
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

//
// MPlatSecApiExt
//

void CTunnelAgentConnectionProvider::ReturnInterfacePtrL(MPlatsecApiExt*& aInterface)
    {
    aInterface = this;
    }

TInt CTunnelAgentConnectionProvider::SecureId(TSecureId& /*aResult*/) const
	{
	return KErrNotSupported;
	}

TInt CTunnelAgentConnectionProvider::VendorId(TVendorId& /*aResult*/) const
	{
	return KErrNotSupported;
	}

TBool CTunnelAgentConnectionProvider::HasCapability(const TCapability /*aCapability*/) const
	{
	return KErrNotSupported;
	}

TInt CTunnelAgentConnectionProvider::CheckPolicy(const TSecurityPolicy& /*aPolicy*/) const
	{
	// This is the whole reason that we need to implement MPlatSecApiExt in this node at all.  When TunnelAgentCpr issues
	// a TStop towards IpCpr, IpCpr requires the sending node (normally ESockSvr but TunnelAgentCpr in this case)
	// to implement MPlatSecApiExt through which IpCpr can check platsec capabilities (IpCprStates::TCheckStopCapabilities).
	// If TunnelAgentCpr does not impement MPlatSecApiExt, then IpCpr will error the stop request with KErrInterfaceNotSupported.
	return KErrNone;
	}
