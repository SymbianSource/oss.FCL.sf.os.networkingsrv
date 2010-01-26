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
#include "ss_glob.h"
#include "policyrequest.h"
#include "policyrequeststates.h"
#include <comms-infras/ss_mcprnodemessages.h>
#include <comms-infras/ss_coreprstates.h>

#include <comms-infras/ss_nodemessages_dataclient.h>
#include <comms-infras/ss_nodemessages_rejoiningprovider.h>
#include <comms-infras/ss_nodemessages_subconn.h>
#include <comms-infras/ss_nodemessages_scpr.h>

using namespace Messages;
using namespace MeshMachine;
using namespace ESock;
using namespace PolicyRequestStates;
using namespace CorePanics;

#ifdef _DEBUG
_LIT (KCPolicyRequestPanic,"PolicyRequestPanic");
#endif

//
// States
//
DEFINE_SMELEMENT( TAwaitingBinderResponse, NetStateMachine::MState, PolicyRequestStates::TContext)
TBool TAwaitingBinderResponse::Accept()
	{
	const TCFServiceProvider::TCommsBinderResponse* br = message_cast<TCFServiceProvider::TCommsBinderResponse>(&iContext.iMessage);
	if (br)
    	{
        iContext.Node().iNewSCprId = br->iNodeId;
        return ETrue;        
    	}
	return EFalse;
	}

DEFINE_SMELEMENT( TAwaitingConnPolicyRequest, NetStateMachine::MState, PolicyRequestStates::TContext)
TBool PolicyRequestStates::TAwaitingConnPolicyRequest::Accept()
	{
	TCFIPMessage::TPolicyRequest* PolicyReqMsg = message_cast<TCFIPMessage::TPolicyRequest>(&iContext.iMessage);
	return PolicyReqMsg && PolicyReqMsg->iValue == ESoCreateWithConnection ? ETrue : EFalse;
	}

//
// Activities
//

DEFINE_SMELEMENT( TSendQoSParamsToNewSCpr, NetStateMachine::MStateTransition, PolicyRequestStates::TContext)
void TSendQoSParamsToNewSCpr::DoL()
	{
#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	iContext.iNodeActivity->PostRequestTo(*iContext.Node().ServiceProvider(),
	    ESock::TCFScpr::TSetParamsRequest(iContext.Node().iParamBundle).CRef());
#else
	iContext.iNodeActivity->PostRequestTo(*iContext.Node().ServiceProvider(),
	    ESock::TCFScpr::TParamsRequest(iContext.Node().iParamBundle).CRef());
#endif // SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	}

DEFINE_SMELEMENT( TJoinReceivedSCpr, NetStateMachine::MStateTransition, PolicyRequestStates::TContext)
void PolicyRequestStates::TJoinReceivedSCpr::DoL()
	{
	__ASSERT_DEBUG(iContext.iNodeActivity, User::Panic(KCPolicyRequestPanic, KPanicNoActivity));
	__ASSERT_DEBUG(iContext.iPeer == iContext.Node().ServiceProvider(), User::Panic(KCPolicyRequestPanic, KPanicExpectedNoServiceProvider));

	TCFServiceProvider::TCommsBinderResponse& br = message_cast<TCFServiceProvider::TCommsBinderResponse>(iContext.iMessage);
    iContext.Node().AddClientL(br.iNodeId, TClientType(TCFClientType::EServProvider, TCFClientType::EActive));

    iContext.iNodeActivity->PostRequestTo(br.iNodeId,
    	TCFServiceProvider::TJoinRequest(iContext.NodeId(), TClientType(TCFClientType::ECtrl)).CRef());
	}

DEFINE_SMELEMENT( TRequestCommsBinderFromSCpr, NetStateMachine::MStateTransition, PolicyRequestStates::TContext)
void PolicyRequestStates::TRequestCommsBinderFromSCpr::DoL()
	{
    __ASSERT_DEBUG(iContext.iNodeActivity, User::Panic(KCPolicyRequestPanic, KPanicNoActivity));
    RNodeInterface* sp = iContext.Node().ServiceProvider();
    __ASSERT_DEBUG(sp, User::Panic(KCPolicyRequestPanic, KPanicNoServiceProvider));
    iContext.iNodeActivity->PostRequestTo(*sp,
    	TCFServiceProvider::TCommsBinderRequest(TSubConnOpen::ECreateNew).CRef());
	}

DEFINE_SMELEMENT( TAwaitingError, NetStateMachine::MState, PolicyRequestStates::TContext)
TBool PolicyRequestStates::TAwaitingError::Accept()
	{
	return iContext.iMessage.IsMessage<TEBase::TError>();
	}

DEFINE_SMELEMENT( TAwaitingCancelError, NetStateMachine::MState, PolicyRequestStates::TContext)
TBool PolicyRequestStates::TAwaitingCancelError::Accept()
	{
	TEBase::TError* msg = message_cast<TEBase::TError>(&iContext.iMessage);

	return msg && msg->iValue == KErrCancel;
	}


//
//Cpr specific
DEFINE_SMELEMENT( TJoinCpr, NetStateMachine::MStateTransition, PolicyRequestStates::TContext)
void PolicyRequestStates::TJoinCpr::DoL()
	{
	__ASSERT_DEBUG(iContext.iNodeActivity, User::Panic(KCPolicyRequestPanic, KPanicNoActivity));
	RNodeInterface* cpr = iContext.Node().GetFirstClient<TDefaultClientMatchPolicy>(TClientType(TCFClientType::EServProvider, TCFClientType::EActive));

	//The sc has been already added
	__ASSERT_DEBUG(cpr != NULL, User::Panic(KCPolicyRequestPanic, KPanicNoServiceProvider));

	//Join the service provider
	iContext.iNodeActivity->PostRequestTo(*cpr,
		TCFServiceProvider::TJoinRequest(iContext.NodeId(), TClientType(TCFClientType::ECtrl)).CRef());
	}

DEFINE_SMELEMENT( TLeaveCpr, NetStateMachine::MStateTransition, PolicyRequestStates::TContext)
void PolicyRequestStates::TLeaveCpr::DoL()
	{
	__ASSERT_DEBUG(iContext.iNodeActivity, User::Panic(KCPolicyRequestPanic, KPanicNoActivity));
	RNodeInterface* cpr = iContext.Node().GetFirstClient<TDefaultClientMatchPolicy>(TClientType(TCFClientType::EServProvider, TCFClientType::EActive));

	//The sc has been already added
	__ASSERT_DEBUG(cpr != NULL, User::Panic(KCPolicyRequestPanic, KPanicNoServiceProvider));

	//Leave the service provider
	iContext.iNodeActivity->PostRequestTo(*cpr,
		TEPeer::TLeaveRequest().CRef());
	cpr->SetFlags(TCFClientType::ELeaving);
	}


DEFINE_SMELEMENT( TSendBindToComplete, NetStateMachine::MStateTransition, PolicyRequestStates::TContext)
void PolicyRequestStates::TSendBindToComplete::DoL()
	{
	__ASSERT_DEBUG(iContext.iNodeActivity, User::Panic(KCPolicyRequestPanic, KPanicNoActivity));
	RNodeInterface* cpr = iContext.Node().GetFirstClient<TDefaultClientMatchPolicy>(TClientType(TCFClientType::EServProvider, TCFClientType::EActive));

	if(cpr)
		{	
		iContext.iNodeActivity->PostRequestTo(*cpr,
			TCFDataClient::TBindToComplete(iContext.iNodeActivity->Error()).CRef());
 		}
	}

//-=========================================================
//Rejoin
//-=========================================================

DEFINE_SMELEMENT( TJoinTheDeftSCPr, NetStateMachine::MStateTransition, PolicyRequestStates::TContext)
void PolicyRequestStates::TJoinTheDeftSCPr::DoL()
	{
	__ASSERT_DEBUG(iContext.iNodeActivity, User::Panic(KCPolicyRequestPanic,KPanicNoActivity));

	//MZTODO: activity using this transition - and possibly the whole approach - must be redesigned
	//so that all destinations are safe! (used to be: TCtrlClientJoinRequestUnsafeDst).
    iContext.Node().iDeftScprClient = iContext.Node().AddClientL(iContext.Node().iSenderSCPrNodeId, TClientType(TCFClientType::EServProvider, TCFClientType::EActive));

    iContext.iNodeActivity->PostRequestTo(iContext.Node().iSenderSCPrNodeId,
    	TCFServiceProvider::TJoinRequest(iContext.NodeId(), TClientType(TCFClientType::ECtrl)).CRef());
	}

DEFINE_SMELEMENT( TLeaveTheDeftSCPr, NetStateMachine::MStateTransition, PolicyRequestStates::TContext)
void PolicyRequestStates::TLeaveTheDeftSCPr::DoL()
	{
	__ASSERT_DEBUG(iContext.iNodeActivity, User::Panic(KCPolicyRequestPanic,KPanicNoActivity));
    RClientInterface::OpenPostMessageClose(TNodeCtxId(iContext.ActivityId(), iContext.NodeId()), iContext.Node().iSenderSCPrNodeId,
    	TEPeer::TLeaveRequest().CRef());
	iContext.Node().iDeftScprClient->SetFlags(TCFClientType::ELeaving);
	}

DEFINE_SMELEMENT( TSendRejoinDataClientRequestToDeftSCPr, NetStateMachine::MStateTransition, PolicyRequestStates::TContext)
void PolicyRequestStates::TSendRejoinDataClientRequestToDeftSCPr::DoL()
	{
	__ASSERT_DEBUG(iContext.iNodeActivity, User::Panic(KCPolicyRequestPanic,KPanicNoActivity));

    RClientInterface::OpenPostMessageClose(TNodeCtxId(iContext.ActivityId(), iContext.NodeId()), iContext.Node().iSenderSCPrNodeId,
    	TCFRejoiningProvider::TRejoinDataClientRequest(iContext.Node().iFlowNodeId, iContext.Node().iNewSCprId).CRef());
	}

DEFINE_SMELEMENT( TSendApplyToDeftSCPr, NetStateMachine::MStateTransition, PolicyRequestStates::TContext)
void PolicyRequestStates::TSendApplyToDeftSCPr::DoL()
	{
	__ASSERT_DEBUG(iContext.iNodeActivity, User::Panic(KCPolicyRequestPanic,KPanicNoActivity));
    iContext.iNodeActivity->PostRequestTo(iContext.Node().iSenderSCPrNodeId,
    	ESock::TCFScpr::TApplyRequest().CRef());
	}

DEFINE_SMELEMENT( TSendApplyToNewSCPr, NetStateMachine::MStateTransition, PolicyRequestStates::TContext)
void PolicyRequestStates::TSendApplyToNewSCPr::DoL()
	{
	__ASSERT_DEBUG(iContext.iNodeActivity, User::Panic(KCPolicyRequestPanic,KPanicNoActivity));
    iContext.iNodeActivity->PostRequestTo(iContext.Node().iNewSCprId,
    	ESock::TCFScpr::TApplyRequest().CRef());
	}

DEFINE_SMELEMENT( TSendCancelToDeftSCPr, NetStateMachine::MStateTransition, PolicyRequestStates::TContext)
void PolicyRequestStates::TSendCancelToDeftSCPr::DoL()
	{
	__ASSERT_DEBUG(iContext.iNodeActivity, User::Panic(KCPolicyRequestPanic,KPanicNoActivity));

    RClientInterface::OpenPostMessageClose(TNodeCtxId(iContext.ActivityId(), iContext.NodeId()), iContext.Node().iSenderSCPrNodeId,
    	TEBase::TCancel().CRef());

	iContext.iNodeActivity->ClearPostedTo(); // the error that comes in wont be a reply
	}

DEFINE_SMELEMENT( TIgnoreAndCloseSubConEvent, NetStateMachine::MStateTransition, PolicyRequestStates::TContext)
void PolicyRequestStates::TIgnoreAndCloseSubConEvent::DoL()
	{
	TCFSubConnControlClient::TSubConnNotification& event = message_cast<TCFSubConnControlClient::TSubConnNotification>(iContext.iMessage);

    event.iRefCountOwnedSubConNotification->Close();
	}


