// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <comms-infras/ss_log.h>
#include <comms-infras/ss_nodemessages.h>
#include "ss_std.h"
#include "ss_glob.h"
#include <comms-infras/ss_subconnflow.h>
#include <comms-infras/ss_roles.h>
#include <comms-infras/ss_tiermanagerutils.h>
#include "policyrequest.h"
#include <comms-infras/ss_esockstates.h>
#include "policyrequeststates.h"
#include <comms-infras/ss_tiermanager.h>
#include <comms-infras/ss_corepractivities.h>

#include <comms-infras/ss_nodemessages_serviceprovider.h>
#include <comms-infras/ss_nodemessages_subconn.h>

using namespace Messages;
using namespace MeshMachine;
using namespace ESock;
using namespace NetStateMachine;
using namespace PolicyRequestStates;


namespace PolicyRequestErrorActivity
{
DECLARE_DEFINE_CUSTOM_NODEACTIVITY(ECFActivityError, PolicyRequestError, TEBase::TError, PolicyRequestActivities::CPolicyRequestActivity::NewL)
	FIRST_NODEACTIVITY_ENTRY(PolicyRequestStates::TAwaitingError, MeshMachine::TNoTag)
	THROUGH_NODEACTIVITY_ENTRY(KNoTag, CoreNetStates::TLeaveServiceProvidersOrSetIdle, MeshMachine::TNoTag)
	//TDestroyAwaitingLeaveCompleteLoop loops back to its own triple if more SPs
	NODEACTIVITY_ENTRY(KNoTag, CoreNetStates::TSetIdleIfNoServiceProviders, MeshMachine::TAwaitingLeaveComplete, CoreActivities::CDestroyActivity::TNoTagOrNoTagBackwards)
	LAST_NODEACTIVITY_ENTRY(KNoTag, CoreStates::TAbortAllActivities)
NODEACTIVITY_END()
}

namespace PolicyConnectionActivity
{
//TODO perhaps get this from a header file since its used in a number of places - see ss_subconn.cpp
typedef MeshMachine::TAcceptErrorState<CoreNetStates::TAwaitingApplyResponse> TAwaitingApplyResponseOrError;

DECLARE_DEFINE_CUSTOM_NODEACTIVITY(ECFPolicyConnectionActivity, PolicyConnection, TCFIPMessage::TPolicyRequest, PolicyRequestActivities::CPolicyRequestActivity::NewL)
	FIRST_NODEACTIVITY_ENTRY(PolicyRequestStates::TAwaitingConnPolicyRequest, MeshMachine::TNoTag)
	NODEACTIVITY_ENTRY(KNoTag, PolicyRequestStates::TJoinCpr, CoreStates::TAwaitingJoinComplete, MeshMachine::TNoTag)
	NODEACTIVITY_ENTRY(KNoTag, PolicyRequestStates::TRequestCommsBinderFromSCpr, PolicyRequestStates::TAwaitingBinderResponse, MeshMachine::TNoTag)
	NODEACTIVITY_ENTRY(KNoTag, PolicyRequestStates::TJoinReceivedSCpr, CoreStates::TAwaitingJoinComplete, MeshMachine::TNoTag)

	THROUGH_NODEACTIVITY_ENTRY(KNoTag, PolicyRequestStates::TSendBindToComplete, MeshMachine::TNoTag)
    NODEACTIVITY_ENTRY(KNoTag, PolicyRequestStates::TLeaveCpr, MeshMachine::TAwaitingLeaveComplete, MeshMachine::TNoTag)

	NODEACTIVITY_ENTRY(KNoTag, PolicyRequestStates::TJoinTheDeftSCPr, CoreStates::TAwaitingJoinComplete, MeshMachine::TNoTag)
	//NODEACTIVITY_ENTRY(KNoTag, PolicyRequestStates::TSendRejoinDataClientRequestToDeftSCPr, CoreNetStates::TAwaitingRejoinDataClientComplete, MeshMachine::TNoTag)
	NODEACTIVITY_ENTRY(KNoTag, PolicyRequestStates::TSendRejoinDataClientRequestToDeftSCPr, CoreNetStates::TAwaitingRejoinDataClientComplete, MeshMachine::TNoTagOrErrorTag)
	NODEACTIVITY_ENTRY(KErrorTag, PolicyRequestStates::TLeaveTheDeftSCPr, MeshMachine::TAwaitingLeaveComplete, MeshMachine::TTag<KErrorTag>)

	// send param bundle with qos params
	NODEACTIVITY_ENTRY(KNoTag, PolicyRequestStates::TSendQoSParamsToNewSCpr, CoreNetStates::TAwaitingParamResponse, MeshMachine::TNoTagOrErrorTag)

	//When adding socket, add first, remove later.
	NODEACTIVITY_ENTRY(KNoTag, PolicyRequestStates::TSendApplyToNewSCPr, TAwaitingApplyResponseOrError, MeshMachine::TNoTagOrErrorTag)
	NODEACTIVITY_ENTRY(KNoTag, PolicyRequestStates::TSendApplyToDeftSCPr, TAwaitingApplyResponseOrError, MeshMachine::TNoTagOrErrorTag)

    NODEACTIVITY_ENTRY(KErrorTag, PolicyRequestStates::TSendCancelToDeftSCPr, PolicyRequestStates::TAwaitingCancelError, MeshMachine::TNoTag)
	NODEACTIVITY_ENTRY(KNoTag,    PolicyRequestStates::TLeaveTheDeftSCPr, MeshMachine::TAwaitingLeaveComplete, MeshMachine::TNoTag)

	//Cleanup
	NODEACTIVITY_ENTRY(KNoTag, CoreNetStates::TLeaveServiceProvidersOrSetIdle, MeshMachine::TAwaitingLeaveComplete, MeshMachine::TNoTag)
	NODEACTIVITY_ENTRY(KNoTag, CoreNetStates::TSetIdleIfNoServiceProviders, MeshMachine::TAwaitingLeaveComplete, CoreActivities::CDestroyActivity::TNoTagOrNoTagBackwards)
	LAST_NODEACTIVITY_ENTRY(KNoTag, MeshMachine::TDoNothing)
NODEACTIVITY_END()
}

namespace PolicyRequestIgnoreSubConEvent
{
DECLARE_DEFINE_NODEACTIVITY(ECFActivityNotification, PolicyRequestSCprNotif, TCFSubConnControlClient::TSubConnNotification)
    NODEACTIVITY_ENTRY(KNoTag, PolicyRequestStates::TIgnoreAndCloseSubConEvent, CoreNetStates::TAwaitingSubConEvent, MeshMachine::TNoTag)
NODEACTIVITY_END()
}

namespace PolicyRequestActivities
{
DECLARE_DEFINE_ACTIVITY_MAP(policyRequestActivities)
	ACTIVITY_MAP_ENTRY(PolicyConnectionActivity, PolicyConnection)
	ACTIVITY_MAP_ENTRY(PolicyRequestErrorActivity, PolicyRequestError)
	ACTIVITY_MAP_ENTRY(PolicyRequestIgnoreSubConEvent, PolicyRequestSCprNotif)
ACTIVITY_MAP_END()
}

CPolicyRequest::CPolicyRequest(const TCFConnPolicyRequest& aMsg)
:   ACFMMNodeIdBase(PolicyRequestActivities::policyRequestActivities::Self())
	{
	iParamBundle.Open(aMsg.iParamBundle);
	iFlowNodeId = aMsg.iFlowNodeId;
	iSenderSCPrNodeId = aMsg.iSenderSCPrNodeId;
	iIPCPrNodeId = aMsg.iIPCPrNodeId;
	iNewSCprId.SetNull();

	//iMessage.Adopt(static_cast<RSafeMessage&>(const_cast<RMessage2&>(aMessage)));
	LOG_NODE_CREATE(KESockConnectionTag, CPolicyRequest);
	}

CPolicyRequest::~CPolicyRequest()
	{
	//ASSERT(!iMessage.IsNull());
	//iMessage.Complete(iError);
	iParamBundle.Close();
	LOG_NODE_DESTROY(KESockConnectionTag, CPolicyRequest);
	}

void CPolicyRequest::Received(TNodeContextBase& aContext)
    {
    Messages::TNodeSignal::TMessageId noPeerIds[] = {
    	TCFIPMessage::TPolicyRequest::Id(),
    	Messages::TNodeSignal::TMessageId()
    	};
    MeshMachine::AMMNodeBase::Received(noPeerIds, aContext);
	MeshMachine::AMMNodeBase::PostReceived(aContext);
	}

void CPolicyRequest::ReceivedL(const TRuntimeCtxId& aSender, const TNodeId& aRecipient, TSignatureBase& aMessage)
    {
    // TODO: TCFMessage::TConnectionGoingDown - If this message is received
    // a new request needs to be sent on behalf of the socket, and
    // this one needs to cleanup/destroy itself. This is for the race condition
    // where either the idle timer triggers and begin stopping the connection,
    // or the connection is lost (both resulting in TConnectionGoingDown) while
    // the request is in progress.
    if (aMessage.IsMessage<TCFMessage::TStateChange>() ||
        aMessage.IsMessage<TCFControlClient::TGoneDown>() )
        {
        // Note of caution: As a control client of providers the CPolicyRequest
        // could be bombarded with messages that we have no interest in.
        // TStateChange is one, but there could be others. Due to the nature
        // of the mesh machine if we don't handle them it will forward them
        // to the CSocket which only expects a limited set of messages from
        // the CPolicyRequest. The CSocket wont like the messages and its not
        // great for performance to forward them unnecessarily so try to kill
        // them off here.
        aMessage.ClearMessageId();
        return;
        }

	TNodeContext<CPolicyRequest> ctx(*this, aMessage, aSender, aRecipient);
    CPolicyRequest::Received(ctx);
    User::LeaveIfError(ctx.iReturn);
	}

//
//TCFConnPolicyRequest
void TCFConnPolicyRequest::StartL(const TRuntimeCtxId& aSender)
	{
	CPolicyRequest* req = new (ELeave) CPolicyRequest(*this);
	CleanupStack::PushL(req);

    TNodeId newServiceProvider = iIPCPrNodeId;
    // set service provider of the PolicyPequest node
	req->AddClientL(iIPCPrNodeId, TClientType(TCFClientType::EServProvider,TCFClientType::EActive));

	TCFIPMessage::TPolicyRequest msg(ESoCreateWithConnection); //Message to triger activity
	req->ReceivedL(aSender, TNodeCtxId(0, req->Id()), msg);
	CleanupStack::Pop(req);
	}

void TCFConnPolicyRequest::DispatchL(const TRuntimeCtxId& aSender, const TRuntimeCtxId& aRecipient)
	{
	const TNodeId& nodeId = address_cast<const TNodeId>(aRecipient);  //This message type operates on nodes
	ASSERT(nodeId==SockManGlobals::Get()->GetPlaneFC(TCFPlayerRole(TCFPlayerRole::ESubConnPlane))); //Should be dispatched on the Connection Plane container!
	
	CleanupClosePushL(iParamBundle);
	StartL(aSender);
	CleanupStack::PopAndDestroy(&iParamBundle);
	}

