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

#include <comms-infras/ss_log.h>
#include <elements/sm_core.h>
#include <comms-infras/corecpractivities.h>
#include <comms-infras/ss_nodemessages_dataclient.h>

#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
#include <comms-infras/ss_nodemessages_scpr.h>
#endif //SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW

#include "agentcpr.h"
#include "agentcprstates.h"
#include "agentmessages.h"
#include "agentqueryconnsettingsimpl.h"

#include <comms-infras/ss_msgintercept.h>


#ifdef _DEBUG
// Panic category for "absolutely impossible!" vanilla ASSERT()-type panics from this module
// (if it could happen through user error then you should give it an explicit, documented, category + code)
_LIT(KSpecAssert_NifManAgtPrCgntc, "NifManAgtPrCgntc");
#endif

#if defined __CFLOG_ACTIVE || defined SYMBIAN_TRACE_ENABLE
#define KAgentCprTag KESockConnectionTag
_LIT8(KAgentCprSubTag, "agentcpr");
#endif


using namespace Messages;
using namespace MeshMachine;
using namespace ESock;
using namespace NetStateMachine;

//We reserve space for two preallocated activities that may start concurrently on the CPR
//node: destroy and data client stop.
static const TUint KDefaultMaxPreallocatedActivityCount = 2;
static const TUint KMaxPreallocatedActivitySize = sizeof(MeshMachine::CNodeRetryParallelActivity) + sizeof(MeshMachine::APreallocatedOriginators<4>);
static const TUint KAgentCPRPreallocatedActivityBufferSize = KDefaultMaxPreallocatedActivityCount * KMaxPreallocatedActivitySize;

// Agent SCPR Going Down Activity
namespace AgentDataClientGoneDownActivity
{
DECLARE_DEFINE_NODEACTIVITY(ECFActivityDataClientGoneDown, AgentDataClientGoneDown, TCFControlProvider::TDataClientGoneDown)
	// AwaitingDataClientGoneDown used rather than AlwaysAccept to mark data client EStopped
	FIRST_NODEACTIVITY_ENTRY(CoreNetStates::TAwaitingDataClientGoneDown, CoreNetStates::TNoTagOrNonDefault)
    LAST_NODEACTIVITY_ENTRY(KNoTag, PRStates::TSendGoneDown)
    LAST_NODEACTIVITY_ENTRY(CoreNetStates::KNonDefault, MeshMachine::TDoNothing)    
NODEACTIVITY_END()
}


// No Bearer Activity
namespace AgentCprNoBearerActivity
{
DECLARE_DEFINE_NODEACTIVITY(ECFActivityNoBearer, AgentCprNoBearer, TCFControlProvider::TNoBearer)
	FIRST_NODEACTIVITY_ENTRY(CoreNetStates::TAwaitingNoBearer, MeshMachine::TNoTag)
    NODEACTIVITY_ENTRY(KNoTag, CoreNetStates::TSendNoBearer, CoreNetStates::TAwaitingBindTo, MeshMachine::TNoTag)
	LAST_NODEACTIVITY_ENTRY(KNoTag, AgentCprStates::TSendBindTo)
NODEACTIVITY_END()
}

#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
/*
Receives the Request for bearer type, Updates the bundle with Bearer type and sends response
*/
namespace AgentCprLinkCharacteristicActivity
{
DECLARE_DEFINE_NODEACTIVITY(ECFActivityParamRequest, AgentCprLinkCharacteristic, TCFScpr::TGetParamsRequest)
	NODEACTIVITY_ENTRY(KNoTag, AgentCprStates::TUpdateBundleAndRespondWithRetrievedParams, PRStates::TAwaitingParamRequest,  MeshMachine::TNoTag)
NODEACTIVITY_END()
}
#endif //SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW

// Activity Map
namespace AgentCprStates
{
DEFINE_EXPORT_ACTIVITY_MAP(agentCprActivities)
    ACTIVITY_MAP_ENTRY(AgentCprNoBearerActivity, AgentCprNoBearer)
    ACTIVITY_MAP_ENTRY(AgentDataClientGoneDownActivity, AgentDataClientGoneDown)
    
#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	ACTIVITY_MAP_ENTRY(AgentCprLinkCharacteristicActivity, AgentCprLinkCharacteristic)
#endif //SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
    
ACTIVITY_MAP_END_BASE(CprActivities, coreCprActivities)
}

/**
Creates an Agent Connection Provider
@param aFactory The parent factory which has created the Cpr
@return Pointer to the newly created Cpr
*/
EXPORT_C CAgentConnectionProvider* CAgentConnectionProvider::NewL(CConnectionProviderFactoryBase& aFactory)
	{
	CAgentConnectionProvider* self = new (ELeave) CAgentConnectionProvider(aFactory);
    CleanupStack::PushL(self);
    self->ConstructL(KAgentCPRPreallocatedActivityBufferSize);
    CleanupStack::Pop(self);
	return self;
	}


CAgentConnectionProvider::CAgentConnectionProvider(CConnectionProviderFactoryBase& aFactory, const MeshMachine::TNodeActivityMap& aActivityMap)
   : CCoreConnectionProvider(aFactory, aActivityMap),
	 TIfStaticFetcherNearestInHierarchy(this)
	{
	LOG_NODE_CREATE(KAgentCprTag, CAgentConnectionProvider);
	iQueryConnSettingsImpl=NULL;
	}
/**
Constructor for the Agent Connection Provider
@param aFactory The parent factory which created this Cpr
*/
CAgentConnectionProvider::CAgentConnectionProvider(CConnectionProviderFactoryBase& aFactory)
   : CCoreConnectionProvider(aFactory, AgentCprStates::agentCprActivities::Self()),
	 TIfStaticFetcherNearestInHierarchy(this)
	{
	LOG_NODE_CREATE(KAgentCprTag, CAgentConnectionProvider);
	iQueryConnSettingsImpl=NULL;
	}


/**
D'tor
*/
EXPORT_C CAgentConnectionProvider::~CAgentConnectionProvider()
	{
	LOG_NODE_DESTROY(KAgentCprTag, CAgentConnectionProvider);
	delete iQueryConnSettingsImpl;
	}

/**
Retrieves the MQueryConnSettingsApiExt implementation
@param aInterface Pointer to the interface implementation.
*/
void CAgentConnectionProvider::ReturnInterfacePtrL(MQueryConnSettingsApiExt*& aInterface)
    {
    if (!iQueryConnSettingsImpl)
    	{
    	const CAgentProvisionInfo* provisionInfo = AgentProvisionInfo();
    	iQueryConnSettingsImpl = new (ELeave)CAgentQueryConnSettingsImpl(*provisionInfo, AccessPointConfig());
    	}
    aInterface = iQueryConnSettingsImpl;
    }

/**
Mesh machine message entry point
@param aCFMessage The Message
*/
void CAgentConnectionProvider::ReceivedL(const TRuntimeCtxId& aSender, const TNodeId& aRecipient, TSignatureBase& aMessage)
	{
	ESOCK_DEBUG_MESSAGE_INTERCEPT(aSender, aMessage, aRecipient);

	TNodeContext<CAgentConnectionProvider> ctx(*this, aMessage, aSender, aRecipient);
	CCoreConnectionProvider::Received(ctx);
	User::LeaveIfError(ctx.iReturn);
	}

/**
Retrieves the MLegacyControlApiExt implementation.
@param aInterface Pointer to the interface implementation.
*/
void CAgentConnectionProvider::ReturnInterfacePtrL(MLegacyControlApiExt*& aInterface)
    {
    aInterface = this;
    }


/**
Retrieves the MLinkCprServiceChangeNotificationApiExt implementation
@param aInterface Pointer to the interface implementation.
*/
void CAgentConnectionProvider::ReturnInterfacePtrL(MLinkCprServiceChangeNotificationApiExt*& aInterface)
	{
	// We return a Pointer to AgentAdapter that is implementing the
	// the requested interface.
	const CAgentProvisionInfo* provisionInfo = AgentProvisionInfo();
	if (!provisionInfo || !provisionInfo->AgentAdapter())
		{
		User::Leave (KErrNotReady);
		}

	aInterface = provisionInfo->AgentAdapter();
	}

/**
Retrieves the Agent Provider specific provisioning information as given by the MCpr
transition.

@internalTechnology
*/
const CAgentProvisionInfo* CAgentConnectionProvider::AgentProvisionInfo() const
    {
    const CAgentProvisionInfo* agentProvisionInfo = static_cast<const CAgentProvisionInfo*>(AccessPointConfig().FindExtension(CAgentProvisionInfo::TypeId()));
	__ASSERT_DEBUG(agentProvisionInfo, User::Panic(KSpecAssert_NifManAgtPrCgntc, 1));
    return agentProvisionInfo;
    }

TInt CAgentConnectionProvider::ControlL(TUint aOptionLevel, TUint aOptionName, TDes8& aOption, MPlatsecApiExt* aPlatsecItf)
	{
	__CFLOG_VAR((KAgentCprTag, KAgentCprSubTag, _L8("CAgentConnectionProvider [this=%08x]: MLegacyControlApiExt::ControlL()"),
		this));

	const CAgentProvisionInfo* provisionInfo = AgentProvisionInfo();

	if(!provisionInfo || !provisionInfo->AgentAdapter())
		{
		User::Leave(KErrNotReady);
		}

	return provisionInfo->AgentAdapter()->Control(aOptionLevel, aOptionName, aOption, aPlatsecItf);
	}

