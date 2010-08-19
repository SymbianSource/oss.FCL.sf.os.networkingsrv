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
// IPProto SubConnection Provider implementation
// 
//

/**
 @file
 @internalComponent
*/

#include <comms-infras/corescprstates.h>
#include <comms-infras/corescpractivities.h>
#include "IPProtoSCPR.h"
#include "IPProtoSCPRStates.h"
#include <comms-infras/linkmessages.h>		// for TLinkMessage
#include <comms-infras/ss_msgintercept.h>


using namespace Messages;
using namespace ESock;
using namespace IPProtoSCpr;
using namespace MeshMachine;

//-=========================================================
//
// Activities
//
//-=========================================================


namespace IPProtoSCprProvisionActivity
{
DECLARE_DEFINE_NODEACTIVITY(ECFActivityStoreProvision, IPProtoSCprProvision, TCFDataClient::TProvisionConfig)
	FIRST_NODEACTIVITY_ENTRY(CoreNetStates::TAwaitingProvision, MeshMachine::TNoTag)
	LAST_NODEACTIVITY_ENTRY(KNoTag, IPProtoSCpr::TStoreProvision)
NODEACTIVITY_END()
}


namespace IPProtoSCprStartActivity
{
DECLARE_DEFINE_CUSTOM_NODEACTIVITY(ECFActivityStart, IPProtoSCprStart, TCFServiceProvider::TStart, PRActivities::CStartActivity::NewL)
    FIRST_NODEACTIVITY_ENTRY(CoreNetStates::TAwaitingStart, CoreNetStates::TNoTagOrBearerPresentBlockedByStop)
	NODEACTIVITY_ENTRY(CoreNetStates::KBearerPresent, CoreNetStates::TBindSelfToPresentBearer, CoreNetStates::TAwaitingBindToComplete, MeshMachine::TTag<CoreNetStates::KBearerPresent>)

	NODEACTIVITY_ENTRY(KNoTag, CoreNetStates::TSendNoBearer, MeshMachine::TAwaitingMessageState<TCFControlProvider::TBearer>, MeshMachine::TNoTagOrErrorTag)	

#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	NODEACTIVITY_ENTRY(KNoTag, PRStates::TSendParamsToServiceProvider, CoreNetStates::TAwaitingParamResponse, CoreNetStates::TNoTagOrBearerPresent)
#else
	NODEACTIVITY_ENTRY(KNoTag, IPProtoSCpr::TSendParamsToServiceProvider, CoreNetStates::TAwaitingParamResponse, CoreNetStates::TNoTagOrBearerPresent)
#endif // SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	NODEACTIVITY_ENTRY(CoreNetStates::KBearerPresent, CoreNetStates::TStartServiceProviderRetry, CoreNetStates::TAwaitingStarted, MeshMachine::TNoTag)
	NODEACTIVITY_ENTRY(KNoTag, CoreNetStates::TStartSelf, CoreNetStates::TAwaitingDataClientStarted, MeshMachine::TNoTag)
	THROUGH_NODEACTIVITY_ENTRY(KNoTag, IPProtoSCpr::TSendDataClientRoutedToFlow, MeshMachine::TNoTag)
	LAST_NODEACTIVITY_ENTRY(KNoTag, CoreNetStates::TSendStarted)
	LAST_NODEACTIVITY_ENTRY(KErrorTag, MeshMachine::TDoNothing)
NODEACTIVITY_END()
}

#ifndef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
namespace IPProtoSCprParamsRequest
{
DECLARE_DEFINE_NODEACTIVITY(ECFActivityParamRequest, IPProtoSCprSetParams, TCFScpr::TParamsRequest)
	FIRST_NODEACTIVITY_ENTRY(SCprStates::TAwaitingParamRequest, CoreNetStates::TNoTagOrBearerPresent)
	NODEACTIVITY_ENTRY(CoreNetStates::KBearerPresent, SCprStates::TPassToServiceProvider, CoreNetStates::TAwaitingParamResponse, CoreNetStates::TBearerPresent)
	LAST_NODEACTIVITY_ENTRY(CoreNetStates::KBearerPresent, SCprStates::TStoreParamsAndPostToOriginators)
	LAST_NODEACTIVITY_ENTRY(KNoTag, SCprStates::TStoreAndRespondWithCurrentParams)
NODEACTIVITY_END()
}
#endif // SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW

namespace IPProtoSCprAgentEventActivity
{
DECLARE_DEFINE_NODEACTIVITY(IPProtoSCpr::EActivityAgentEvent, AgentEvent, TLinkMessage::TAgentEventNotification)
	NODEACTIVITY_ENTRY(KNoTag, IPProtoSCpr::TProcessAgentEvent, IPProtoSCpr::TAwaitingAgentEventNotification, MeshMachine::TNoTag)
NODEACTIVITY_END()
}

//-=========================================================
// Data Monitoring Activities
//-=========================================================
namespace IPProtoSCprDataMonitoringActivity
{
DECLARE_DEFINE_NODEACTIVITY(IPProtoSCpr::EActivityDataMonitoring, IPProtoSCprDataMonitoring, TCFDataMonitoringNotification::TDataMonitoringNotification)
	FIRST_NODEACTIVITY_ENTRY(IPProtoSCpr::TAwaitingDataMonitoringNotification, MeshMachine::TNoTag)
	LAST_NODEACTIVITY_ENTRY(KNoTag, IPProtoSCpr::TProcessDataMonitoringNotification)
NODEACTIVITY_END()
}

namespace IPProtoSCprForwardStateChangeActivity
{
DECLARE_DEFINE_NODEACTIVITY(ECFActivityForwardStateChange, IPProtoSCprForwardStateChange, TCFMessage::TStateChange)
    FIRST_NODEACTIVITY_ENTRY(MeshMachine::TAwaitingMessageState<TCFMessage::TStateChange>, MeshMachine::TNoTag)
    LAST_NODEACTIVITY_ENTRY(KNoTag, CoreNetStates::TForwardToControlProvider)
NODEACTIVITY_END()
}

namespace IPProtoSCpr
{
DECLARE_DEFINE_ACTIVITY_MAP(stateMap)
	ACTIVITY_MAP_ENTRY(IPProtoSCprProvisionActivity, IPProtoSCprProvision)
	ACTIVITY_MAP_ENTRY(IPProtoSCprStartActivity, IPProtoSCprStart)
    ACTIVITY_MAP_ENTRY(IPProtoSCprDataMonitoringActivity, IPProtoSCprDataMonitoring)
	ACTIVITY_MAP_ENTRY(IPProtoSCprForwardStateChangeActivity, IPProtoSCprForwardStateChange) //Forward progress horizontally
	ACTIVITY_MAP_ENTRY(IPProtoSCprAgentEventActivity, AgentEvent)
#ifndef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	ACTIVITY_MAP_ENTRY(IPProtoSCprParamsRequest, IPProtoSCprSetParams)
#endif // SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
ACTIVITY_MAP_END_BASE(SCprActivities, coreSCprActivities)
}


CIPProtoSubConnectionProvider::CIPProtoSubConnectionProvider(ESock::CSubConnectionProviderFactoryBase& aFactory)
	:CCoreSubConnectionProvider(aFactory, IPProtoSCpr::stateMap::Self()),
	 ALegacySubConnectionActiveApiExt(this),
	 TIfStaticFetcherNearestInHierarchy(this),
	 iNotify(NULL),
	 iControl(NULL)
    {
    LOG_NODE_CREATE(KIPProtoSCPRTag, CIPProtoSubConnectionProvider);
    }

void CIPProtoSubConnectionProvider::ConstructL()
    {
    ADataMonitoringProvider::ConstructL();
    CCoreSubConnectionProvider::ConstructL();
    }


void CIPProtoSubConnectionProvider::ReceivedL(const TRuntimeCtxId& aSender, const TNodeId& aRecipient, TSignatureBase& aMessage)
	{
	TNodeContext<CIPProtoSubConnectionProvider> ctx(*this, aMessage, aSender, aRecipient);
	ESOCK_DEBUG_MESSAGE_INTERCEPT(aSender, aMessage, aRecipient);
	CCoreSubConnectionProvider::ReceivedL(aSender, aRecipient, aMessage);
	User::LeaveIfError(ctx.iReturn);
	}

CIPProtoSubConnectionProvider* CIPProtoSubConnectionProvider::NewL(ESock::CSubConnectionProviderFactoryBase& aFactory)
    {
    CIPProtoSubConnectionProvider* prov = new (ELeave) CIPProtoSubConnectionProvider(aFactory);
    CleanupStack::PushL(prov);
    prov->ConstructL();
    CleanupStack::Pop(prov);
    return prov;
    }

CIPProtoSubConnectionProvider::~CIPProtoSubConnectionProvider()
    {
	if (iControl)
		iControl->AsyncDelete();
	if (iNotify)
		delete iNotify;

    LOG_NODE_DESTROY(KIPProtoSCPRTag, CIPProtoSubConnectionProvider);
    }

void CIPProtoSubConnectionProvider::ReturnInterfacePtrL(ADataMonitoringProtocolReq*& aInterface)
	{
	aInterface = this;
	}

void CIPProtoSubConnectionProvider::ReturnInterfacePtrL(ESock::ALegacySubConnectionActiveApiExt*& aInterface)
	{
	aInterface = this;
	}

