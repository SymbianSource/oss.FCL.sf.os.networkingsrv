/**
*   Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
*   All rights reserved.
*   This component and the accompanying materials are made available
*   under the terms of "Eclipse Public License v1.0"
*   which accompanies this distribution, and is available
*   at the URL "http://www.eclipse.org/legal/epl-v10.html".
*   
*   Initial Contributors:
*   Nokia Corporation - initial contribution.
*   
*   Contributors:
*
*   Description:
*   mcpr binding from agent mcpr for tunneldriver.
* 
*
*/

/**
 @file tundrivermcpr.cpp
 @internalComponent
*/

#include <comms-infras/ss_log.h>
#include <in_sock.h>
#include <comms-infras/metadata.h>
#include <comms-infras/coremcpractivities.h>
#include <comms-infras/agentmcpractivities.h>
#include <comms-infras/ss_tiermanagerutils.h>
#include "tundrivermcpr.h"
#include "tundriverprovision.h"
#include <comms-infras/ss_msgintercept.h>

using namespace Messages;
using namespace MeshMachine;
using namespace ESock;
using namespace MCprActivities;
using namespace TunDriverMCprStates;

// No Bearer Activity
namespace TunDriverMCPRNoBearerActivity
{
DECLARE_DEFINE_NODEACTIVITY(ECFActivityNoBearer, TunDriverMCPRNoBearer, TCFControlProvider::TNoBearer)
	NODEACTIVITY_ENTRY(KNoTag, CoreNetStates::TSendBearer, CoreNetStates::TAwaitingNoBearer, MeshMachine::TNoTag)	
NODEACTIVITY_END()
}


// Activity Map
namespace TunDriverMCprStates
{
DECLARE_DEFINE_ACTIVITY_MAP(stateMap)
   ACTIVITY_MAP_ENTRY(TunDriverMCPRNoBearerActivity, TunDriverMCPRNoBearer)
ACTIVITY_MAP_END_BASE(AgentMCprActivities, agentMCprActivities)
} // namespace TunDriverMCprStates


#if defined __CFLOG_ACTIVE || defined SYMBIAN_TRACE_ENABLE
    _LIT8(KTunDriverMCprSubTag, "tunmcpr");
#endif

CTunDriverMetaConnectionProvider* CTunDriverMetaConnectionProvider::NewL(ESock::CMetaConnectionProviderFactoryBase& aFactory, const ESock::TProviderInfo& aProviderInfo)
/**
* CTunDriverMetaConnectionProvider::NewL constructs a Default TunDriver MCPR
* @param aFactory
* @param aProviderInfo
* @returns pointer to a constructed mcpr object.
*/
    {
    CTunDriverMetaConnectionProvider* self = new (ELeave) CTunDriverMetaConnectionProvider(aFactory, aProviderInfo);
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);
    return self;
    }


CTunDriverMetaConnectionProvider::CTunDriverMetaConnectionProvider(CMetaConnectionProviderFactoryBase& aFactory, const ESock::TProviderInfo& aProviderInfo)
:	CAgentMetaConnectionProvider(aFactory, aProviderInfo, TunDriverMCprStates::stateMap::Self())
/**
* CTunDriverMetaConnectionProvider::CTunDriverMetaConnectionProvider is a default constructor
* @param aFactory
* @param aProviderInfo
* @returns pointer to a constructed mcpr object.
*/ 	
	{
	LOG_NODE_CREATE(KTunDriverMCprSubTag, CTunDriverMetaConnectionProvider);
	}

CTunDriverMetaConnectionProvider::~CTunDriverMetaConnectionProvider()
/**
* CTunDriverMetaConnectionProvider::CTunDriverMetaConnectionProvider is a default destructor
*/
	{
    SetAgentNotificationHandlerL(NULL);
	LOG_NODE_DESTROY(KTunDriverMCprSubTag, CTunDriverMetaConnectionProvider);
	}


void CTunDriverMetaConnectionProvider::ConstructL()
/**
* CTunDriverMetaConnectionProvider::ConstructL is a second-phase constructor
* Will set the accesspoint from Comms database.
* @param
* @returns 
*/
    {
    CAgentMetaConnectionProvider::ConstructL();
	SetAccessPointConfigFromDbL();
	}


void CTunDriverMetaConnectionProvider::ReceivedL(const TRuntimeCtxId& aSender, const TNodeId& aRecipient, TSignatureBase& aMessage)
/**
* CTunDriverMetaConnectionProvider::ReceivedL called on incoming MCPR Messages.
* @param
* @returns 
*/
    {
	__CFLOG_VAR((KTunDriverMCprTag, KTunDriverMCprSubTag, _L8("CTunDriverMetaConnectionProvider [this=%08x]::ReceivedL() aMessage=%d"),
	   this, aMessage.MessageId()));

	ESOCK_DEBUG_MESSAGE_INTERCEPT(aSender, aMessage, aRecipient);

	TNodeContext<CTunDriverMetaConnectionProvider> ctx(*this, aMessage, aSender, aRecipient);
    CCoreMetaConnectionProvider::Received(ctx);
    User::LeaveIfError(ctx.iReturn);
	}

void CTunDriverMetaConnectionProvider::SetAccessPointConfigFromDbL()
/**
* CTunDriverMetaConnectionProvider::SetAccessPointConfigFromDbL will initialize the tundriver with 
* the configuration set in comms database.
* @param
* @returns 
*/
    {
	RMetaExtensionContainer mec;
	mec.Open(AccessPointConfig());
	CleanupClosePushL(mec);

    // Add provisioning information.
   	CCommsDatIapView* iapView = OpenIapViewLC();
	CTunDriverProtoProvision* provision = new (ELeave) CTunDriverProtoProvision();
	CleanupStack::PushL(provision);
	provision->InitialiseConfigL(iapView);

	// Presumptions:
	// - none of the extensions can already exist in the AccessPointConfig array.  AppendExtensionL()
	//   is presumed to panic if adding the same extension a second time.
	// - if we have added several extensions to the AccessPointConfig array before getting a failure
	//   and leaving, it is presumed that the MCPr will be destroyed and AccessPointConfig destructor
	//   will clean up the extensions immediately afterwards.

	mec.AppendExtensionL(provision);
	CleanupStack::Pop(provision);          // ownership (including cleanup) transferred to AccessPointConfig()
	CleanupStack::PopAndDestroy();			// CloseIapView()
	
	AccessPointConfig().Close();
	AccessPointConfig().Open(mec);
	CleanupStack::PopAndDestroy(&mec);
    }
