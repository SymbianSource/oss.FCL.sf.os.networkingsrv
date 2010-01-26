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
#include <comms-infras/linkmessages.h>
#include <comms-infras/nifif.h>
#include <elements/nm_messages_base.h>
#include <comms-infras/ss_nodemessages_dataclient.h>

#include "agentscpr.h"
#include "agentscprstates.h"
#include "CAgentAdapter.h"
#include "agentmessages.h"


#ifdef _DEBUG
// Panic category for "absolutely impossible!" vanilla ASSERT()-type panics from this module
// (if it could happen through user error then you should give it an explicit, documented, category + code)
_LIT(KSpecAssert_NifManAgtPrCSSta, "NifManAgtPrCSSta");
#endif


using namespace Messages;
using namespace ESock;


#ifdef __CFLOG_ACTIVE
#define KAgentSCprTag KESockSubConnectionTag
_LIT8(KAgentSCprSubTag, "agentscprstates");
#endif


namespace AgentSCprStates
{

// ---------------- Start Connection Activity Overrides ----------------
EXPORT_DEFINE_SMELEMENT(TStartAgent, NetStateMachine::MStateTransition, AgentSCprStates::TContext)
EXPORT_C void TStartAgent::DoL()
   {
   __CFLOG_VAR((KAgentSCprTag, KAgentSCprSubTag, _L8("TStartAgent::DoL()")));

   iContext.Node().SetActivityIdForAdapter(iContext.iNodeActivity->ActivityId());
   iContext.Node().StartAgentL();
   }


// ---------------- Notifications From Flow Activity ----------------
EXPORT_DEFINE_SMELEMENT(TAwaitingNotificationFromFlow, NetStateMachine::MState, AgentSCprStates::TContext)
EXPORT_C TBool TAwaitingNotificationFromFlow::Accept()
    {
    return iContext.iMessage.IsMessage<TLinkMessage::TFlowToAgentNotification>();
    }

EXPORT_DEFINE_SMELEMENT(TNotifyAgent, NetStateMachine::MStateTransition, AgentSCprStates::TContext)
EXPORT_C void TNotifyAgent::DoL()
    {
    TLinkMessage::TAgentToFlowNotification& msg = message_cast<TLinkMessage::TAgentToFlowNotification>(iContext.iMessage);
    iContext.Node().NotificationFromFlow(static_cast<TFlowToAgentEventType>(msg.iValue));
    }


// ---------------- Unsolicited Flow Down Activity ----------------
EXPORT_DEFINE_SMELEMENT(TProcessDataClientGoneDown, NetStateMachine::MStateTransition, AgentSCprStates::TContext)
EXPORT_C void TProcessDataClientGoneDown::DoL()
    {
    TCFControlProvider::TDataClientGoneDown& msg = message_cast<TCFControlProvider::TDataClientGoneDown>(iContext.iMessage);
    TInt error = msg.iValue1;
    MNifIfNotify::TAction action = static_cast<MNifIfNotify::TAction>(msg.iValue2);

    RNodeInterface* dataClient = iContext.Node().FindClient(iContext.iSender);
    dataClient->ClearFlags(TCFClientType::EStarted);

    iContext.Node().NotificationToAgent (EFlowToAgentEventTypeLinkLayerDown, &error);

    if (action == MNifIfNotify::EReconnect)
        {
        CAgentProvisionInfo::TAgentReconnectOption option = iContext.Node().AgentProvisionInfo()->ReconnectOption();
    	TUint32 attempts = iContext.Node().AgentProvisionInfo()->ReconnectAttempts();

		if (option == CAgentProvisionInfo::EDoNotAttemptReconnect
			|| attempts == 0)
			{
            iContext.Node().AgentProvisionInfo()->AgentAdapter()->DisconnectAgent(error);
			}
		else
			{
			const_cast<CAgentProvisionInfo*>(iContext.Node().AgentProvisionInfo())->SetReconnectAttempts(attempts - 1);

	        switch (option)
	            {
	            case CAgentProvisionInfo::EAttemptReconnect:
            		iContext.Node().ConnectAgent(EAgentReconnect);
	                break;

	            case CAgentProvisionInfo::EPromptForReconnect:
	                // This causes the agent Reconnect() to begin
	                // If the CAgentBase::Reconnect() hasn't been overridden the
	                // user will be prompted
	                iContext.Node().AgentProvisionInfo()->AgentAdapter()->PromptForReconnect();
	                break;

	            default:
	                // No other options are available
	                __ASSERT_DEBUG(EFalse, User::Panic(KSpecAssert_NifManAgtPrCSSta, 1));
	                break;
	            }
			}
        }
    else
        {
        iContext.Node().AgentProvisionInfo()->AgentAdapter()->DisconnectAgent(error);
        }
    }


// ---------------- Authentication Activity ----------------

EXPORT_DEFINE_SMELEMENT(TAwaitingAuthenticate, NetStateMachine::MState, CoreStates::TContext)
EXPORT_C TBool TAwaitingAuthenticate::Accept()
	{
	return iContext.iMessage.IsMessage<TLinkMessage::TAuthenticate>();
	}

EXPORT_DEFINE_SMELEMENT(TAwaitingAuthenticateComplete, NetStateMachine::MState, AgentSCprStates::TContext)
EXPORT_C TBool TAwaitingAuthenticateComplete::Accept()
	{
	return iContext.iMessage.IsMessage<TLinkMessage::TAuthenticateResponse>();
	}

EXPORT_C void TAwaitingAuthenticateComplete::Cancel()
    {
    __CFLOG_VAR((KAgentSCprTag, KAgentSCprSubTag, _L8("AAwaitingAuthenticateComplete::Cancel()")));

    CAgentSubConnectionProvider& provider = iContext.Node();
    provider.SetActivityIdForAdapter(iContext.iNodeActivity->ActivityId());
    __ASSERT_DEBUG(provider.AgentProvisionInfo()->AgentAdapter(), User::Panic(KSpecAssert_NifManAgtPrCSSta, 2));

    if (provider.iAuthenticateInProgress)
        {
        provider.iAuthenticateInProgress = EFalse;
        provider.AgentProvisionInfo()->AgentAdapter()->CancelAuthenticate();
        }
    }

EXPORT_DEFINE_SMELEMENT(TSendAuthenticate, NetStateMachine::MStateTransition, AgentSCprStates::TContext)
EXPORT_C void TSendAuthenticate::DoL()
    {
    __CFLOG_VAR((KAgentSCprTag, KAgentSCprSubTag, _L8("ASendAuthenticate::DoL()")));

    CAgentSubConnectionProvider& provider = iContext.Node();
    __ASSERT_DEBUG(provider.AgentProvisionInfo()->AgentAdapter(), User::Panic(KSpecAssert_NifManAgtPrCSSta, 3));

    provider.iAuthenticateInProgress = ETrue;
    provider.AgentProvisionInfo()->AgentAdapter()->Authenticate(provider.iUsername, provider.iPassword);
    provider.SetActivityIdForAdapter(iContext.iNodeActivity->ActivityId());
    iContext.iNodeActivity->SetPostedTo(provider.Id());
    }

EXPORT_DEFINE_SMELEMENT(TSendAuthenticateComplete, NetStateMachine::MStateTransition, CoreStates::TContext)
EXPORT_C void TSendAuthenticateComplete::DoL()
    {
    __CFLOG_VAR((KAgentSCprTag, KAgentSCprSubTag, _L8("ASendAuthenticateComplete::DoL()")));
    TLinkMessage::TAuthenticateResponse msg;
    if (iContext.iNodeActivity)
        {

        iContext.iNodeActivity->PostToOriginators(msg);
        }
    else
        {
        iContext.PostToSender(msg);
        }
    }


// ---------------- Stop Connection Activity Overrides ----------------

EXPORT_DEFINE_SMELEMENT(TNoTagOrProviderStopped, NetStateMachine::MStateFork, AgentSCprStates::TContext)
EXPORT_C TInt TNoTagOrProviderStopped::TransitionTag()
	{
	if (iContext.Node().AgentProvisionInfo()->AgentAdapter() == NULL ||
        iContext.Node().AgentProvisionInfo()->AgentAdapter()->AgentState() == CAgentAdapter::EDisconnected)
        {
        return CoreNetStates::KProviderStopped;
        }
    return MeshMachine::KNoTag;
    }

EXPORT_DEFINE_SMELEMENT(TNoTagOrProviderStarted, NetStateMachine::MStateFork, AgentSCprStates::TContext)
EXPORT_C TInt TNoTagOrProviderStarted::TransitionTag()
	{
	RNodeInterface* flow = iContext.Node().GetFirstClient<TDefaultClientMatchPolicy>(TClientType(TCFClientType::EData), TClientType(TCFClientType::EStarted));
	return flow ? CoreNetStates::KProviderStarted : MeshMachine::KNoTag;
    }

EXPORT_DEFINE_SMELEMENT(TStopAgent, NetStateMachine::MStateTransition, AgentSCprStates::TContext)
EXPORT_C void TStopAgent::DoL()
    {
    __CFLOG_VAR((KAgentSCprTag, KAgentSCprSubTag, _L8("AStopAgent::DoL()")));

    TInt reason = KErrCancel;
    if (iContext.iMessage.IsMessage<TCFDataClient::TStopped>())
        {
        // This supports the logic from the legacy CNifAgentRef::LinkLayerDown()
        // During the processing of LinkLayerDown the Agent would be notified.
        TCFDataClient::TStopped& msg = message_cast<TCFDataClient::TStopped>(iContext.iMessage);
        iContext.Node().AgentProvisionInfo()->AgentAdapter()->NotificationToAgent (EFlowToAgentEventTypeLinkLayerDown, &msg.iValue);
        reason = msg.iValue;
        }
    else if (iContext.iMessage.IsMessage<TCFDataClient::TStop>())
        {
        // Receiving TCFDataClient::TStop when we don't have a flow will get us here
        // Not sure that should ever happen though
        TCFDataClient::TStop& msg = message_cast<TCFDataClient::TStop>(iContext.iMessage);
        reason = msg.iValue;
        }
    else
        {
        // TODO: SCpr Panic - unexpected message
        }

    iContext.Node().SetActivityIdForAdapter(iContext.iNodeActivity->ActivityId());
    iContext.Node().StopAgent(reason);
    iContext.iNodeActivity->SetPostedTo(iContext.NodeId());
    }

EXPORT_DEFINE_SMELEMENT(TAwaitingAgentDown, NetStateMachine::MState, AgentSCprStates::TContext)
EXPORT_C TBool TAwaitingAgentDown::Accept()
	{
	return iContext.iMessage.IsMessage<TCFServiceProvider::TStopped>();
	}

EXPORT_DEFINE_SMELEMENT(TSendDataClientGoneDown, NetStateMachine::MStateTransition, AgentSCprStates::TContext)
EXPORT_C void TSendDataClientGoneDown::DoL()
	{
	RNodeInterface *cp = iContext.Node().ControlProvider();
	cp->PostMessage(TNodeCtxId(iContext.ActivityId(), iContext.NodeId()),
		TCFControlProvider::TDataClientGoneDown(iContext.Node().iStoppingReason).CRef());
	}

EXPORT_DEFINE_SMELEMENT(TSendError, NetStateMachine::MStateTransition, AgentSCprStates::TContext)
EXPORT_C void TSendError::DoL()
	{
	RNodeInterface *cp = iContext.Node().ControlProvider();
	cp->PostMessage(TNodeCtxId(iContext.ActivityId(), iContext.NodeId()),
		TEBase::TError(TCFDataClient::TStart::Id(), iContext.Node().iStoppingReason).CRef());
	}

} // namespace AgentSCprStates

