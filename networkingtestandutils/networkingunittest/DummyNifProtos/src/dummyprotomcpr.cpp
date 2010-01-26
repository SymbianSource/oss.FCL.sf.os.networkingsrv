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
// DummyProto MCPR
// 
//

/**
 @file
 @internalComponent
*/

#include <comms-infras/ss_log.h>
#include <in_sock.h>
#include <comms-infras/metadata.h>
#include <comms-infras/coremcpractivities.h>
#include <comms-infras/agentmcpractivities.h>
#include <comms-infras/ss_tiermanagerutils.h>
#include "dummyprotomcpr.h"
#include "DummyProvision.h"
#include "DummyAgentHandler.h"
#include <comms-infras/ss_msgintercept.h>

using namespace Messages;
using namespace MeshMachine;
using namespace ESock;
using namespace MCprActivities;
using namespace DummyProtoMCprStates;

// No Bearer Activity
namespace DummyProtoMCPRNoBearerActivity
{
DECLARE_DEFINE_NODEACTIVITY(ECFActivityNoBearer, DummyProtoMCPRNoBearer, TCFControlProvider::TNoBearer)
	NODEACTIVITY_ENTRY(KNoTag, CoreNetStates::TSendBearer, CoreNetStates::TAwaitingNoBearer, MeshMachine::TNoTag)	
NODEACTIVITY_END()
}


// Activity Map
namespace DummyProtoMCprStates
{
DECLARE_DEFINE_ACTIVITY_MAP(stateMap)
   ACTIVITY_MAP_ENTRY(DummyProtoMCPRNoBearerActivity, DummyProtoMCPRNoBearer)
ACTIVITY_MAP_END_BASE(AgentMCprActivities, agentMCprActivities)
} // namespace DummyProtoMCprStates


#if defined __CFLOG_ACTIVE || defined SYMBIAN_TRACE_ENABLE
    _LIT8(KDummyProtoMCprSubTag, "dummymcpr");
#endif

//-=========================================================
//
//CDummyProtoMetaConnectionProvider implementation
//
//-=========================================================

CDummyProtoMetaConnectionProvider* CDummyProtoMetaConnectionProvider::NewL(ESock::CMetaConnectionProviderFactoryBase& aFactory, const ESock::TProviderInfo& aProviderInfo)
    {
    CDummyProtoMetaConnectionProvider* self = new (ELeave) CDummyProtoMetaConnectionProvider(aFactory, aProviderInfo);
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);
    return self;
    }


CDummyProtoMetaConnectionProvider::CDummyProtoMetaConnectionProvider(CMetaConnectionProviderFactoryBase& aFactory, const ESock::TProviderInfo& aProviderInfo)
:	CAgentMetaConnectionProvider(aFactory, aProviderInfo, DummyProtoMCprStates::stateMap::Self())
	{
	LOG_NODE_CREATE(KDummyProtoMCprSubTag, CDummyProtoMetaConnectionProvider);
	}

CDummyProtoMetaConnectionProvider::~CDummyProtoMetaConnectionProvider()
	{
	// Assumption is that CDummyProtoProvision will be cleaned up by ~CAccessPointConfig

	// Clean up Agent Notification Handler
    SetAgentNotificationHandlerL(NULL);
    delete iAgentHandler;

	LOG_NODE_DESTROY(KDummyProtoMCprSubTag, CDummyProtoMetaConnectionProvider);
	}


void CDummyProtoMetaConnectionProvider::ConstructL()
    {
    CAgentMetaConnectionProvider::ConstructL();
	SetAccessPointConfigFromDbL();
	}


void CDummyProtoMetaConnectionProvider::ReceivedL(const TRuntimeCtxId& aSender, const TNodeId& aRecipient, TSignatureBase& aMessage)
    {
	__CFLOG_VAR((KDummyProtoMCprTag, KDummyProtoMCprSubTag, _L8("CDummyProtoMetaConnectionProvider [this=%08x]::ReceivedL() aMessage=%d"),
	   this, aMessage.MessageId()));

	ESOCK_DEBUG_MESSAGE_INTERCEPT(aSender, aMessage, aRecipient);

	TNodeContext<CDummyProtoMetaConnectionProvider> ctx(*this, aMessage, aSender, aRecipient);
    CCoreMetaConnectionProvider::Received(ctx);
    User::LeaveIfError(ctx.iReturn);
	}

void CDummyProtoMetaConnectionProvider::SetAccessPointConfigFromDbL()
    {
	RMetaExtensionContainer mec;
	mec.Open(AccessPointConfig());
	CleanupClosePushL(mec);

    // Add provisioning information.
   	CCommsDatIapView* iapView = OpenIapViewLC();
	CDummyProtoProvision* provision = new (ELeave) CDummyProtoProvision();
	CleanupStack::PushL(provision);
	provision->InitialiseConfigL(iapView);

	// Presumptions:
	// - none of the extensions can already exist in the AccessPointConfig array.  AppendExtensionL()
	//   is presumed to panic if adding the same extension a second time.
	// - if we have added several extensions to the AccessPointConfig array before getting a failure
	//   and leaving, it is presumed that the MCPr will be destroyed and AccessPointConfig destructor
	//   will clean up the extensions immediately afterwards.

	mec.AppendExtensionL(provision);
	CleanupStack::Pop(provision);			// ownership (including cleanup) transferred to AccessPointConfig()
	CleanupStack::PopAndDestroy();			// CloseIapView()
	
	AccessPointConfig().Close();
	AccessPointConfig().Open(mec);
	CleanupStack::PopAndDestroy(&mec);

	ASSERT(iAgentHandler == NULL);
	// Register the agent notification handler
    iAgentHandler = new (ELeave) CDummyProtoAgentHandler();
    SetAgentNotificationHandlerL(iAgentHandler);	// ownership NOT transferred
    }
