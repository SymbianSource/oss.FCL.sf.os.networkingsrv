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
// Tunnel MCPR
// 
//

/**
 @file
 @internalComponent
*/

#include <comms-infras/ss_log.h>
#include <comms-infras/ss_tiermanagerutils.h>
#include <comms-infras/coremcpractivities.h>
#include <comms-infras/agentmcpractivities.h>
#include "tunnelProvision.h"
#include "TunnelAgentHandler.h"
#include "tunnelmcpr.h"
#include <comms-infras/ss_msgintercept.h>

#define KTunnelMCprTag KESockMetaConnectionTag

using namespace Messages;
using namespace MeshMachine;
using namespace ESock;
using namespace NetStateMachine;
using namespace MCprActivities;


// No Bearer Activity
namespace TunnelMCPRNoBearerActivity
{
DECLARE_DEFINE_NODEACTIVITY(ECFActivityNoBearer, activity, TCFControlProvider::TNoBearer)
	FIRST_NODEACTIVITY_ENTRY(CoreNetStates::TAwaitingNoBearer, MeshMachine::TNoTag)
	LAST_NODEACTIVITY_ENTRY(KNoTag, CoreNetStates::TSendBearer)
NODEACTIVITY_END()
}


// Activity Map
namespace TunnelMCprStates
{
DECLARE_DEFINE_ACTIVITY_MAP(stateMap)
   ACTIVITY_MAP_ENTRY(TunnelMCPRNoBearerActivity, activity)
ACTIVITY_MAP_END_BASE(AgentMCprActivities, agentMCprActivities)
} // namespace TunnelMCprStates

//-=========================================================
//
//CTunnelMetaConnectionProvider implementation
//
//-=========================================================

CTunnelMetaConnectionProvider* CTunnelMetaConnectionProvider::NewL(ESock::CMetaConnectionProviderFactoryBase& aFactory, const ESock::TProviderInfo& aProviderInfo)
    {
    // coverity[alloc_fn] coverity[alias] coverity[assign]
    CTunnelMetaConnectionProvider* tunnel = new (ELeave) CTunnelMetaConnectionProvider(aFactory, aProviderInfo);
    // coverity[push]
    CleanupStack::PushL(tunnel);
    // coverity[alias] coverity[double_push]
    tunnel->ConstructL();
    CleanupStack::Pop(tunnel);
    return tunnel;
    }


CTunnelMetaConnectionProvider::CTunnelMetaConnectionProvider(CMetaConnectionProviderFactoryBase& aFactory, const ESock::TProviderInfo& aProviderInfo)
:	CAgentMetaConnectionProvider(aFactory, aProviderInfo, TunnelMCprStates::stateMap::Self())
	{
	LOG_NODE_CREATE(KTunnelMCprTag, CTunnelMetaConnectionProvider);
	}

CTunnelMetaConnectionProvider::~CTunnelMetaConnectionProvider()
	{
	// Cleanup up Agent Notification Handler
	SetAgentNotificationHandlerL(NULL);
	delete iAgentHandler;

	LOG_NODE_DESTROY(KTunnelMCprTag, CTunnelMetaConnectionProvider);
	}

void CTunnelMetaConnectionProvider::ConstructL()
    {
    CAgentMetaConnectionProvider::ConstructL();
	SetAccessPointConfigFromDbL();
	}


void CTunnelMetaConnectionProvider::ReceivedL(const TRuntimeCtxId& aSender, const TNodeId& aRecipient, TSignatureBase& aMessage)
    {
	__CFLOG_VAR((KTunnelMCprTag, KTunnelMCprSubTag, _L8("CTunnelMetaConnectionProvider %08x:\tReceivedL() aCFMessage=%d"),
	   this, aMessage.MessageId()));

	ESOCK_DEBUG_MESSAGE_INTERCEPT(aSender, aMessage, aRecipient);

	TNodeContext<CTunnelMetaConnectionProvider> ctx(*this, aMessage, aSender, aRecipient);
    CCoreMetaConnectionProvider::Received(ctx);
    User::LeaveIfError(ctx.iReturn);
	}

void CTunnelMetaConnectionProvider::SetAccessPointConfigFromDbL()
/**
Setup the provisioning information required by the Tunnel CFProtocol.
*/
    {
    RMetaExtensionContainer mec;
    mec.Open(AccessPointConfig());
    CleanupClosePushL(mec);

	TTunnelProvision* tp = new (ELeave) TTunnelProvision;
    CleanupStack::PushL(tp);

    // Open an IAP specific view on CommsDat
	CCommsDatIapView* iapView = OpenIapViewLC();
	tp->InitialiseConfigL(iapView);
	CleanupStack::PopAndDestroy();			// CloseIapView();

	// Presumptions:
	// - none of the extensions can already exist in the AccessPointConfig array.  AppendExtensionL()
	//   is presumed to panic if adding the same extension a second time.
	// - if we have added several extensions to the AccessPointConfig array before getting a failure
	//   and leaving, it is presumed that the MCPr will be destroyed and AccessPointConfig destructor
	//   will clean up the extensions immediately afterwards.

	// Append the provisioning object to the CAccessPointConfig array
	mec.AppendExtensionL(tp);
	CleanupStack::Pop(tp);					// ownership transferred

	AccessPointConfig().Close();
	AccessPointConfig().Open(mec);
	CleanupStack::PopAndDestroy(&mec);
	
	// Setup Tunnel Agent Notification handler in order to retrieve TSY and connection speed
	// information from the Agent once ConnectComplete() occurs.
	ASSERT(iAgentHandler == NULL);
	iAgentHandler = new (ELeave) CTunnelAgentHandler(tp->iInfo);
	SetAgentNotificationHandlerL(iAgentHandler);	// ownership NOT transferred
    }
