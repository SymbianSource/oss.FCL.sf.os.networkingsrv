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
// IPDeftProto SubConnection Provider implementation
// 
//

/**
 @file
 @internalComponent
*/


#include <comms-infras/corescprstates.h>
#include <comms-infras/corescpractivities.h>
#include "ipprotocprstates.h"

#include "ipprotodeftscpr.h"
#include "ipprotodeftscprstates.h"
#include <comms-infras/linkmessages.h>		// for TLinkMessage
#include <comms-infras/ss_msgintercept.h>

using namespace Messages;
using namespace ESock;
using namespace IPProtoDeftSCpr;
using namespace MeshMachine;

//-=========================================================
//
// Activities
//
//-=========================================================


namespace IPProtoDeftSCprProvisionActivity
{
DECLARE_DEFINE_NODEACTIVITY(ECFActivityStoreProvision, IPProtoDeftSCprProvision, TCFDataClient::TProvisionConfig)
	FIRST_NODEACTIVITY_ENTRY(CoreNetStates::TAwaitingProvision, MeshMachine::TNoTag)
	LAST_NODEACTIVITY_ENTRY(KNoTag, IPProtoDeftSCpr::TStoreProvision)
NODEACTIVITY_END()
}

namespace IPProtoDeftSCprDataClientStopActivity
{
DECLARE_DEFINE_CUSTOM_NODEACTIVITY(ECFActivityStopDataClient, IPProtoDeftSCprStop, TCFDataClient::TStop, MeshMachine::CPreallocatedNodeRetryActivity::New)
    FIRST_NODEACTIVITY_ENTRY(CoreNetStates::TAwaitingDataClientStop, TNoTagOrProviderStoppedOrDaemonReleased)
    NODEACTIVITY_ENTRY(KNoTag, TStopNetCfgExt, TAwaitingStateChange, TDaemonReleasedStateChangedOrNoTag)
    NODEACTIVITY_ENTRY(KNoTag, TForwardToControlProviderAndResetSentTo, TAwaitingStateChange, TDaemonReleasedStateChangedOrNoTagBackward)
    THROUGH_NODEACTIVITY_ENTRY(KDaemonReleasedStateChanged, TForwardToControlProviderAndResetSentTo,  TTag<KDaemonReleased>)
    NODEACTIVITY_ENTRY(KDaemonReleased, SCprStates::TStopYourFlows, CoreNetStates::TAwaitingDataClientStopped,  MeshMachine::TTag<CoreNetStates::KProviderStopped>)
   

    THROUGH_NODEACTIVITY_ENTRY(CoreNetStates::KProviderStopped, TStopNetCfgExtDelete, CoreNetStates::TNoTagOrUnbindOnStop)
    NODEACTIVITY_ENTRY(CoreNetStates::KUnbind, CoreNetStates::TSendClientLeavingRequestToServiceProvider, MeshMachine::TAwaitingLeaveComplete, MeshMachine::TNoTag)
    THROUGH_NODEACTIVITY_ENTRY(KNoTag, PRStates::TDestroyOrphanedDataClients, MeshMachine::TNoTag)
    LAST_NODEACTIVITY_ENTRY(KNoTag,  PRStates::TSendDataClientStopped)
NODEACTIVITY_END()
}

//Activity to handle the Ioctl.
//Ioctls are often handled by layer below ipproto. In ordered to maintain the genericity of ipproto we send these requests to the layer
//below us. If the layer below us services the request then we pass on the response to the originators else if the layer below us doesn't handle 
//the Ioctl, we are errored. Once errored we clear the error and try netcfgextn to complete the Ioctls.
namespace IPProtoDeftSCprIoctlActivity
{
DECLARE_DEFINE_CUSTOM_NODEACTIVITY(ECFActivityIoctl, IPProtoDeftSCprIoctl, TNodeSignal::TNullMessageId, MeshMachine::CNodeParallelMessageStoreActivityBase::NewL)
    FIRST_NODEACTIVITY_ENTRY(IpProtoCpr::TAwaitingIoctlMessage, IPProtoDeftSCpr::TTryServiceProviderOrTryNetCfgExt)
    NODEACTIVITY_ENTRY(KTryServiceProvider, IPProtoDeftSCpr::TForwardToServiceProvider, TAcceptErrorState<CoreNetStates::TAwaitingRMessage2Processed>, MeshMachine::TNoTagOrErrorTag)
    THROUGH_NODEACTIVITY_ENTRY(KErrorTag, MeshMachine::TClearError, MeshMachine::TTag<KTryNetCfgExt>)
    NODEACTIVITY_ENTRY(KTryNetCfgExt, IPProtoDeftSCpr::THandoffToNetCfgExt, IPProtoDeftSCpr::TAwaitingIoctlProcessed, MeshMachine::TNoTag)
    LAST_NODEACTIVITY_ENTRY(KNoTag, CoreStates::TPostToOriginators)
NODEACTIVITY_END()
}

namespace IPProtoDeftSCprAgentEventActivity
{
DECLARE_DEFINE_NODEACTIVITY(IPProtoDeftSCpr::EActivityAgentEvent, AgentEvent, TLinkMessage::TAgentEventNotification)
	NODEACTIVITY_ENTRY(KNoTag, IPProtoDeftSCpr::TProcessAgentEvent, IPProtoDeftSCpr::TAwaitingAgentEventNotification, MeshMachine::TNoTag)
NODEACTIVITY_END()
}

//-=========================================================
// Data Monitoring Activities
//-=========================================================
namespace IPProtoDeftSCprDataMonitoringActivity
{
DECLARE_DEFINE_NODEACTIVITY(IPProtoDeftSCpr::EActivityDataMonitoring, IPProtoDeftSCprDataMonitoring, TCFDataMonitoringNotification::TDataMonitoringNotification)
	FIRST_NODEACTIVITY_ENTRY(IPProtoDeftSCpr::TAwaitingDataMonitoringNotification, MeshMachine::TNoTag)
	LAST_NODEACTIVITY_ENTRY(KNoTag, IPProtoDeftSCpr::TProcessDataMonitoringNotification)
NODEACTIVITY_END()
}

//Activity to forward legacy progresses to IpProtoCpr.
//It overrides the default behaviour of forwarding legacy progresses up to the Control Clients.
//In the case of there being a config daemon, KLinkLayerOpen is swallowed. The config daemon will
//send it's own when configuration is done. IK
namespace IPProtoDeftSCprForwardStateChangeActivity
{
DECLARE_DEFINE_NODEACTIVITY(ECFActivityForwardStateChange, IPProtoDeftSCprForwardStateChange, TCFMessage::TStateChange)
	FIRST_NODEACTIVITY_ENTRY(MeshMachine::TAwaitingMessageState<TCFMessage::TStateChange>, IPProtoDeftSCpr::TNoTagOrSwallowMessage)
	LAST_NODEACTIVITY_ENTRY(KNoTag, CoreNetStates::TForwardToControlProvider)
	LAST_NODEACTIVITY_ENTRY(KSwallowMessage, MeshMachine::TDoNothing)
NODEACTIVITY_END()
}

namespace IPProtoDeftSCprConfigureNetworkActivity
{
DECLARE_DEFINE_NODEACTIVITY(ECFActivityConfigureNetwork, IPProtoDeftSCprConfigureNetwork, TCFIPProtoMessage::TConfigureNetwork)
	FIRST_NODEACTIVITY_ENTRY(IPProtoDeftSCpr::TAwaitingConfigureNetwork, MeshMachine::TNoTag)
	NODEACTIVITY_ENTRY(KNoTag, TStartNetCfgExtAndResetSentTo, TAwaitingStateChangeOrCancel, TNetworkConfiguredOrErrorTagOrCancelTagOrNoTag)
	NODEACTIVITY_ENTRY(KNoTag, TForwardToControlProviderAndResetSentTo, TAwaitingStateChangeOrCancel, TNetworkConfiguredOrErrorTagOrCancelTagOrNoTagBackward)
	THROUGH_NODEACTIVITY_ENTRY(KErrorTag, TForwardToControlProviderAndStopNetCfgExt, MeshMachine::TTag<KNetworkConfigured>)
	LAST_NODEACTIVITY_ENTRY(KNetworkConfigured, TSendNetworkConfigured)
    LAST_NODEACTIVITY_ENTRY(KCancelTag, TStopNetCfgExtAndRaiseActivityError)
NODEACTIVITY_END()
}

namespace IPProtoDeftSCprDataClientStartActivity
{
DECLARE_DEFINE_NODEACTIVITY(ECFActivityStartDataClient, IPProtoDeftSCprDataClientStart, TCFDataClient::TStart)
    FIRST_NODEACTIVITY_ENTRY(CoreNetStates::TAwaitingDataClientStart, MeshMachine::TNoTag)
    NODEACTIVITY_ENTRY(KNoTag, PRStates::TStartDataClients, MeshMachine::TAcceptErrorState<CoreNetStates::TAwaitingDataClientsStarted>, MeshMachine::TErrorTagOr<IPProtoDeftSCpr::TNoTagOrConfigureNetwork>)
	NODEACTIVITY_ENTRY(KConfigureNetwork, IPProtoDeftSCpr::TConfigureNetwork, IPProtoDeftSCpr::TAwaitingNetworkConfiguredOrError, IPProtoDeftSCpr::TErrorTagOrNoTag)
	LAST_NODEACTIVITY_ENTRY(KNoTag, PRStates::TSendDataClientStarted)

	NODEACTIVITY_ENTRY(KErrorTag, CoreNetStates::TStopSelf, CoreNetStates::TAwaitingDataClientStopped, MeshMachine::TTag<KErrorTag>)
	LAST_NODEACTIVITY_ENTRY(KErrorTag, MeshMachine::TRaiseAndClearActivityError)
NODEACTIVITY_END()
}

namespace IPProtoDeftSCpr
{
DECLARE_DEFINE_ACTIVITY_MAP(stateMap)
	ACTIVITY_MAP_ENTRY(IPProtoDeftSCprProvisionActivity, IPProtoDeftSCprProvision)
	ACTIVITY_MAP_ENTRY(IPProtoDeftSCprDataClientStopActivity, IPProtoDeftSCprStop)
	ACTIVITY_MAP_ENTRY(IPProtoDeftSCprDataMonitoringActivity, IPProtoDeftSCprDataMonitoring)
	ACTIVITY_MAP_ENTRY(IPProtoDeftSCprForwardStateChangeActivity, IPProtoDeftSCprForwardStateChange) //Forward progress horizontally
	ACTIVITY_MAP_ENTRY(IPProtoDeftSCprConfigureNetworkActivity, IPProtoDeftSCprConfigureNetwork)
	ACTIVITY_MAP_ENTRY(IPProtoDeftSCprDataClientStartActivity, IPProtoDeftSCprDataClientStart)
	ACTIVITY_MAP_ENTRY(IPProtoDeftSCprAgentEventActivity, AgentEvent)
    ACTIVITY_MAP_ENTRY(IPProtoDeftSCprIoctlActivity, IPProtoDeftSCprIoctl)
ACTIVITY_MAP_END_BASE(SCprActivities, coreSCprActivities)
}


CIPProtoDeftSubConnectionProvider::CIPProtoDeftSubConnectionProvider(ESock::CSubConnectionProviderFactoryBase& aFactory)
	:CCoreSubConnectionProvider(aFactory, IPProtoDeftSCpr::stateMap::Self()),
	 ALegacySubConnectionActiveApiExt(this),
	 TIfStaticFetcherNearestInHierarchy(this),
	 iNotify(NULL),
	 iControl(NULL),
	 iNetworkConfigurationState(EFalse)
    {
    LOG_NODE_CREATE(KIPProtoDeftScprTag, CIPProtoDeftSubConnectionProvider);
    }

void CIPProtoDeftSubConnectionProvider::ConstructL()
    {
    ADataMonitoringProvider::ConstructL();
    CCoreSubConnectionProvider::ConstructL();
    }


void CIPProtoDeftSubConnectionProvider::ReceivedL(const TRuntimeCtxId& aSender, const TNodeId& aRecipient, TSignatureBase& aMessage)
	{
    TNodeContext<CIPProtoDeftSubConnectionProvider> ctx(*this, aMessage, aSender, aRecipient);
	ESOCK_DEBUG_MESSAGE_INTERCEPT(aSender, aMessage, aRecipient);
	CCoreSubConnectionProvider::ReceivedL(aSender, aRecipient, aMessage);
	User::LeaveIfError(ctx.iReturn);
	}

CIPProtoDeftSubConnectionProvider* CIPProtoDeftSubConnectionProvider::NewL(ESock::CSubConnectionProviderFactoryBase& aFactory)
    {
    CIPProtoDeftSubConnectionProvider* prov = new (ELeave) CIPProtoDeftSubConnectionProvider(aFactory);
    CleanupStack::PushL(prov);
    prov->ConstructL();
    CleanupStack::Pop(prov);
    return prov;
    }

CIPProtoDeftSubConnectionProvider::~CIPProtoDeftSubConnectionProvider()
    {
    // In case network is not configured i.e. AP might get close in case for WIFi for an example, DHCP registration
    //will get failed. There is not point listening to such Progresses. So can notification and delete
    //delete CNetCfgExtNotify pointer).
   if(iNetworkConfigurationState == EFalse)
       {
       if(iNotify)
           {
           delete iNotify;
           iNotify = NULL;
           }
       }
	if (iControl)
		iControl->AsyncDelete();
	//incase registration is successful and Network is configured. 
	if (iNotify)
	    {
		delete iNotify;
		iNotify = NULL;
		}

    LOG_NODE_DESTROY(KIPProtoDeftScprTag, CIPProtoDeftSubConnectionProvider);
    }

void CIPProtoDeftSubConnectionProvider::ReturnInterfacePtrL(ADataMonitoringProtocolReq*& aInterface)
	{
	aInterface = this;
	}

void CIPProtoDeftSubConnectionProvider::ReturnInterfacePtrL(ESock::ALegacySubConnectionActiveApiExt*& aInterface)
	{
	aInterface = this;
	}

