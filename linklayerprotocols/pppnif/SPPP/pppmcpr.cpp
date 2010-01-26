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
// PPP MCpr
// 
//

/**
 @file
 @internalComponent
*/


#include <comms-infras/ss_log.h>
#include <comms-infras/agentmcpractivities.h>
#include <comms-infras/coremcpractivities.h>
#include <comms-infras/ss_tiermanagerutils.h>

#include "pppmcpr.h"
#include "pppmcprstates.h"
#include "csdavailabilitylistener.h"

#include <comms-infras/ss_msgintercept.h>

using namespace Messages;
using namespace MeshMachine;
using namespace ESock;
using namespace NetStateMachine;
using namespace MCprActivities;

#if defined __CFLOG_ACTIVE || defined SYMBIAN_TRACE_ENABLE
#define KPppMCprTag KESockMetaConnectionTag
_LIT8(KPppMCprSubTag, "PppMCpr");
#endif

// No Bearer Activity
namespace PppMCprNoBearerActivity
{
DECLARE_DEFINE_NODEACTIVITY(ECFActivityNoBearer, PppMCprNoBearer, TCFControlProvider::TNoBearer)
	FIRST_NODEACTIVITY_ENTRY(CoreNetStates::TAwaitingNoBearer, MeshMachine::TNoTag)
	LAST_NODEACTIVITY_ENTRY(KNoTag, CoreNetStates::TSendBearer)
NODEACTIVITY_END()
}


// Activity Map
namespace PppMCprStates
{
DECLARE_DEFINE_ACTIVITY_MAP(stateMap)
	ACTIVITY_MAP_ENTRY(PppMCprNoBearerActivity, PppMCprNoBearer)
ACTIVITY_MAP_END_BASE(AgentMCprActivities, agentMCprActivities)
} // namespace PppMCprStates

//-=========================================================
//
//CPppMetaConnectionProvider implementation
//
//-=========================================================

CPppMetaConnectionProvider* CPppMetaConnectionProvider::NewL(ESock::CMetaConnectionProviderFactoryBase& aFactory,
                                                             const ESock::TProviderInfo& aProviderInfo)
    {
    // coverity[alloc_fn] coverity[alias] coverity[assign]
    CPppMetaConnectionProvider* self = new (ELeave) CPppMetaConnectionProvider(aFactory,aProviderInfo,PppMCprStates::stateMap::Self());
    // coverity[push]
    CleanupStack::PushL(self);
    // coverity[alias] coverity[double_push]
    self->ConstructL();
    CleanupStack::Pop(self);
    return self;
    }

void CPppMetaConnectionProvider::ConstructL()
    {
    CAgentMetaConnectionProvider::ConstructL();
    SetAccessPointConfigFromDbL();
    }

CPppMetaConnectionProvider::CPppMetaConnectionProvider(CMetaConnectionProviderFactoryBase& aFactory,
                                                       const ESock::TProviderInfo& aProviderInfo,
                                                       const MeshMachine::TNodeActivityMap& aActivityMap)
:	CAgentMetaConnectionProvider(aFactory,aProviderInfo,aActivityMap)
	{
	LOG_NODE_CREATE(KPppMCprSubTag, CPppMetaConnectionProvider);
	}

CPppMetaConnectionProvider::~CPppMetaConnectionProvider()
	{
	// Clean up the Agent Notification Handler
	SetAgentNotificationHandlerL(NULL);
	delete iAgentHandler;
	CPppMetaConnectionProvider::CancelAvailabilityMonitoring(); //Don't call virtual for obvious reasons!
	LOG_NODE_DESTROY(KPppMCprSubTag, CPppMetaConnectionProvider);
	}


void CPppMetaConnectionProvider::ReceivedL(const TRuntimeCtxId& aSender, const TNodeId& aRecipient, TSignatureBase& aMessage)
	{
	__CFLOG_VAR((KPppMCprTag, KPppMCprSubTag, _L8("CPppMetaConnectionProvider %08x:\tReceivedL() aCFMessage=%d"),
	  this, aMessage.MessageId().MessageId()));

	ESOCK_DEBUG_MESSAGE_INTERCEPT(aSender, aMessage, aRecipient);

	TNodeContext<CPppMetaConnectionProvider> ctx(*this, aMessage, aSender, aRecipient);
	CCoreMetaConnectionProvider::Received(ctx);
	User::LeaveIfError(ctx.iReturn);
	}

void CPppMetaConnectionProvider::CleanupCloseIapView(TAny* aThis)
	{
	reinterpret_cast<CPppMetaConnectionProvider*>(aThis)->CloseIapView();
	}

void CPppMetaConnectionProvider::SetAccessPointConfigFromDbL()
	{
	// Its necessary here to check whether the mcpr has already been provisioned, because it may be still
	// lingering because of a previous connection start
	if (AccessPointConfig().FindExtension(CBCAProvision::TypeId()))
		{
		ASSERT(AccessPointConfig().FindExtension(CIPConfig::TypeId()));
		ASSERT(AccessPointConfig().FindExtension(CPppLcpConfig::TypeId()));
		ASSERT(AccessPointConfig().FindExtension(CPppAuthConfig::TypeId()));
		ASSERT(AccessPointConfig().FindExtension(CPppTsyConfig::TypeId()));
		return;
		}

    RMetaExtensionContainer mec;
    mec.Open(AccessPointConfig());
    CleanupClosePushL(mec);
    
	CCommsDatIapView* iapView = OpenIapViewLC();

	// Presumptions:
	// - none of the extensions can already exist in the AccessPointConfig array.  AppendExtensionL()
	//   is presumed to panic if adding the same extension a second time.
	// - if we have added several extensions to the AccessPointConfig array before getting a failure
	//   and leaving, it is presumed that the MCPr will be destroyed and AccessPointConfig destructor
	//   will clean up the extensions immediately afterwards.
	mec.AppendExtensionL(CBCAProvision::NewLC(iapView));
	CleanupStack::Pop(); //Ownership with the list

	mec.AppendExtensionL(CIPConfig::NewLC(iapView));
	CleanupStack::Pop(); //Ownership with the list

	mec.AppendExtensionL(CPppLcpConfig::NewLC(iapView));
	CleanupStack::Pop(); //Ownership with the list

	mec.AppendExtensionL(CPppAuthConfig::NewLC(iapView));
	CleanupStack::Pop(); //Ownership with the list

	mec.AppendExtensionL(CPppTsyConfig::NewLC(iapView));
	CleanupStack::Pop(); //Ownership with the list

	CleanupStack::PopAndDestroy();			// CloseIapView();
	
	iAccessPointConfig.Close();
	iAccessPointConfig.Open(mec);
	CleanupStack::PopAndDestroy(&mec);

	// Add the Agent Notification Handler.
	ASSERT(iAgentHandler == NULL);
	iAgentHandler = CPppAgentNotificationHandler::NewL();				// ownership retained
	SetAgentNotificationHandlerL(iAgentHandler);
	}

void CPppMetaConnectionProvider::StartAvailabilityMonitoringL(const TNodeCtxId& aAvailabilityActivity)
	{
	ASSERT(iAvailabilityListener==NULL); //Only one start allowed from the av activity!
	const CPppTsyConfig& config = static_cast<const CPppTsyConfig&>(AccessPointConfig().FindExtensionL(STypeId::CreateSTypeId(CPppTsyConfig::EUid, CPppTsyConfig::ETypeId)));
	iAvailabilityListener = CCsdAvailabilityListener::NewL(aAvailabilityActivity, config, ProviderInfo().APId());
	AddClientL(iAvailabilityListener->Id(), TClientType(TCFClientType::ERegistrar, TCFClientType::EAvailabilityProvider));
	}

void CPppMetaConnectionProvider::CancelAvailabilityMonitoring()
	{
	if (iAvailabilityListener)
		{
		RemoveClient(iAvailabilityListener->Id());
		iAvailabilityListener = NULL; //iAvailabilityListener will delete itself when cancelled from the availability activity
		}
	}

CPppAgentNotificationHandler* CPppAgentNotificationHandler::NewL ()
   {
   return new (ELeave)CPppAgentNotificationHandler();
   }


CPppAgentNotificationHandler::CPppAgentNotificationHandler()
   {
   }


/**
Upcall from the Agent
*/
void CPppAgentNotificationHandler::ConnectCompleteL()
	{
	TBool pop = EFalse;
	CPppProvisionInfo* pppInfo = const_cast<CPppProvisionInfo*>(static_cast<const CPppProvisionInfo*>(GetExtension(STypeId::CreateSTypeId(CPppProvisionInfo::EUid, CPppProvisionInfo::ETypeId))));
	if (!pppInfo) // not provisioned yet
		{
		pppInfo = new (ELeave) CPppProvisionInfo;
		CleanupStack::PushL(pppInfo);
		pop = ETrue;
		}

	const TInt KMaxExcessData = 1503 * 2; // from PPP HDLC
	RBuf8 excessData;
	CleanupClosePushL(excessData);
	excessData.CreateL(KMaxExcessData);

	(void)ReadExcessData(excessData);
	User::LeaveIfError(pppInfo->SetExcessData(excessData));

	CleanupStack::PopAndDestroy(&excessData);

	pppInfo->SetIsDialIn(QueryIsDialIn());

	if (pop)
		{
		AppendExtensionL(pppInfo);
		CleanupStack::Pop(pppInfo);
		}
	}
