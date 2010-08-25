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
#include <comms-infras/linkmessages.h>
#include <elements/nm_messages_base.h>

#include <comms-infras/ss_nodemessages_dataclient.h>

#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <comms-infras/nifagt_internal.h>
#endif

#include "agentscpr.h"
#include "agentscprstates.h"
#include "agentscpractivities.h"
#include "CAgentAdapter.h"
#include "agentmessages.h"

#include <comms-infras/ss_msgintercept.h>


#ifdef _DEBUG
// Panic category for "absolutely impossible!" vanilla ASSERT()-type panics from this module
// (if it could happen through user error then you should give it an explicit, documented, category + code)
_LIT(KSpecAssert_NifManAgtPrCgnts, "NifManAgtPrCgnts");
#endif

#if defined __CFLOG_ACTIVE || defined SYMBIAN_TRACE_ENABLE
#define KAgentSCprTag KESockSubConnectionTag
_LIT8(KAgentSCprSubTag, "agentscpr");
#endif


using namespace Messages;
using namespace MeshMachine;
using namespace ESock;
using namespace NetStateMachine;

/**
Creates an Agent SubConnection Provider
@param aFactory The parent factory which has created the SCPr
@return Pointer to the newly created SCPr
*/
EXPORT_C CAgentSubConnectionProvider* CAgentSubConnectionProvider::NewL(ESock::CSubConnectionProviderFactoryBase& aFactory)
	{
	CAgentSubConnectionProvider* self = new (ELeave) CAgentSubConnectionProvider(aFactory, AgentSCprActivities::agentSCprActivities::Self());
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}


/**
Constructor for the Agent SubConnection Provider
@param aFactory The parent factory which created this SCPr
*/
EXPORT_C CAgentSubConnectionProvider::CAgentSubConnectionProvider(CSubConnectionProviderFactoryBase& aFactory,
    const MeshMachine::TNodeActivityMap& aActivityMap)
   : CCoreSubConnectionProvider(aFactory, aActivityMap)
	{
	LOG_NODE_CREATE(KAgentSCprTag, CAgentSubConnectionProvider);
	}


/**
D'tor
*/
EXPORT_C CAgentSubConnectionProvider::~CAgentSubConnectionProvider()
    {
	CleanupProvisioningInfo();
    LOG_NODE_DESTROY(KAgentSCprTag, CAgentSubConnectionProvider);
	}


EXPORT_C void CAgentSubConnectionProvider::CleanupProvisioningInfo()
    {
    if (iAuthenticateInProgress && AgentProvisionInfo()->AgentAdapter())
	   {
	   iAuthenticateInProgress = EFalse;
	   AgentProvisionInfo()->AgentAdapter()->CancelAuthenticate();
	   }

	CAgentAdapter* agentAdapter(AgentProvisionInfo()->AgentAdapter());
	const_cast<CAgentProvisionInfo*>(AgentProvisionInfo())->SetAgentAdapter(NULL);
	delete agentAdapter;

	// Remove ourselves from the notification handler, or delete it entirely if we own it
	CAgentNotificationHandler* handler = AgentProvisionInfo()->AgentNotificationHandler();
	if (handler)
	    {
	    if (iScprOwnedNotificationHandler)
	        {
	        const_cast<CAgentProvisionInfo*>(AgentProvisionInfo())->SetAgentNotificationHandler(NULL);
	        delete handler;
	        }
        else
            {
            handler->Initialise(NULL);
            }
	    }
    }

/**
Mesh machine message entry point
*/
EXPORT_C void CAgentSubConnectionProvider::ReceivedL(const TRuntimeCtxId& aSender, const TNodeId& aRecipient, TSignatureBase& aMessage)
	{
	ESOCK_DEBUG_MESSAGE_INTERCEPT(aSender, aMessage, aRecipient);

	TNodeContext<CAgentSubConnectionProvider> ctx(*this, aMessage, aSender, aRecipient);
	CCoreSubConnectionProvider::Received(ctx);
	User::LeaveIfError(ctx.iReturn);
	}


TInt CAgentSubConnectionProvider::PostMessageToFlow(const TRuntimeCtxId& aSender, const TSignatureBase& aMessage)
   {
   // There can only ever be one flow for an AgentSCpr
   RNodeInterface* dataClient = this->GetFirstClient<TDefaultClientMatchPolicy>(TClientType(TCFClientType::EData));
   if (!dataClient)
      {
      return KErrNotFound;
      }
   dataClient->PostMessage(aSender, aMessage);
   return KErrNone;
   }


/**
Retrieves the Agent Provider specific provisioning information as given by the MCPr
transition.

@internalTechnology
*/
EXPORT_C const CAgentProvisionInfo* CAgentSubConnectionProvider::AgentProvisionInfo() const
    {
    const CAgentProvisionInfo* agentProvisionInfo = static_cast<const CAgentProvisionInfo*>(AccessPointConfig().FindExtension(CAgentProvisionInfo::TypeId()));
	__ASSERT_DEBUG(agentProvisionInfo, User::Panic(KSpecAssert_NifManAgtPrCgnts, 1));
    return agentProvisionInfo;
    }


/**
Starts the agent connection process
*/
EXPORT_C void CAgentSubConnectionProvider::ConnectAgent(TAgentConnectType aConnectType)
    {
    AgentProvisionInfo()->AgentAdapter()->ConnectAgent(aConnectType);
    }


/**
Notifies an event to the AgentSCpr's Agent via the AgentAdapter
*/
EXPORT_C TInt CAgentSubConnectionProvider::NotificationToAgent(TFlowToAgentEventType aEvent, TAny* aInfo)
    {
    return AgentProvisionInfo()->AgentAdapter()->NotificationToAgent(aEvent, aInfo);
    }


/**
Called from the StartAgent state transition
*/
void CAgentSubConnectionProvider::StartAgentL()
    {
    SetActivityIdForAdapter(ECFActivityStartDataClient);

    if (!AgentProvisionInfo()->AgentAdapter())
        {
        CAgentNotificationHandler* handler = AgentProvisionInfo()->AgentNotificationHandler();
        if (!handler)
            {
            // Ensure something is present to forward msgs to the flow
            // Normally the MCPr will own the handler, we must only delete if we created it
            handler = CAgentNotificationHandler::NewL();
            iScprOwnedNotificationHandler = ETrue;
            const_cast<CAgentProvisionInfo*>(AgentProvisionInfo())->SetAgentNotificationHandler(handler);
            }
        handler->Initialise (this);

        const_cast<CAgentProvisionInfo*>(AgentProvisionInfo())->SetAgentAdapter (CAgentAdapter::NewL(*this, AgentProvisionInfo()->AgentName()));
        }

	CAgentAdapter::TAgentState agtState = AgentProvisionInfo()->AgentAdapter()->AgentState();

    // Should never get a TStart whilst we are disconnecting
    __ASSERT_DEBUG(agtState != CAgentAdapter::EDisconnecting, User::Panic(KSpecAssert_NifManAgtPrCgnts, 2));

	if (agtState == CAgentAdapter::EDisconnected)
        {
        ConnectAgent(EAgentStartDialOut);
        }
    else if (agtState == CAgentAdapter::EConnected)
        {
        RClientInterface::OpenPostMessageClose(Id(), TNodeCtxId(ECFActivityStartDataClient, Id()), TCFServiceProvider::TStarted().CRef());
        iActivityIdForAdapter = KActivityNull;
        }
    // else if AgentState() is EConnecting or EReconnecting we wait for the TStarted
    //      from that operation
   }

/**
Called from the StopAgent state transition
*/
void CAgentSubConnectionProvider::StopAgent(TInt aReason)
    {
    // Agent should have been started, so the agent adapter should exist
    __ASSERT_DEBUG(AgentProvisionInfo()->AgentAdapter(), User::Panic(KSpecAssert_NifManAgtPrCgnts, 3));

    if (AgentProvisionInfo()->AgentAdapter()->AgentState() == CAgentAdapter::EDisconnected)
        {
        RClientInterface::OpenPostMessageClose(Id(), TNodeCtxId(iActivityIdForAdapter, Id()), TCFDataClient::TStopped(KErrDisconnected).CRef());
        iActivityIdForAdapter = KActivityNull;
        }
    else
        {
        iStopRequested = ETrue;
        iStoppingReason = aReason;
        AgentProvisionInfo()->AgentAdapter()->DisconnectAgent(aReason);
        }
    //ProgressL(KConnectionUninitialised);
    }

/**
Upcall from the agent
*/
void CAgentSubConnectionProvider::ServiceStarted()
   {
   CAgentNotificationHandler* handler = AgentProvisionInfo()->AgentNotificationHandler();
   if (handler)
      {
      handler->ServiceStarted ();
      }
   }


/**
Called from the CAgentAdapter. Gets information required by flow into a TCommsBinder
and posts it into this node's mesh machine with a TStarted message
*/
void CAgentSubConnectionProvider::ConnectionUpL()
	{
	__CFLOG_VAR((KAgentSCprTag, KAgentSCprSubTag, _L8("CAgentSubConnectionProvider::ConnectionUpL() - Agent has started")));

    CAgentNotificationHandler* handler = AgentProvisionInfo()->AgentNotificationHandler();
    if (handler)
        {
        handler->ConnectCompleteL();
        }

    // Send connection up message to the SCPr
    RClientInterface::OpenPostMessageClose(Id(), TNodeCtxId(ECFActivityStartDataClient, Id()), TCFServiceProvider::TStarted().CRef());
    iActivityIdForAdapter = KActivityNull;
	}



/**
Called from the CAgentAdapter. Indicates that the agent is now disconnected.
*/
void CAgentSubConnectionProvider::ConnectionDownL()
	{
	// Only send ourselves a DataClientStopped message if we really want to forward
	// it up. That translates to: have we been asked to StopConnection and are
	// we expecting the agent to disconnect, if so then kick the mesh machine
	// with a TStopped message
	if (iStopRequested)
	    {
    	__CFLOG_VAR((KAgentSCprTag, KAgentSCprSubTag, _L8("CAgentSubConnectionProvider::ConnectionDownL() - Agent has stopped by request")));
    	// Send connection down message to self (this is really like the TStopped message coming up from a lower layer)

    	RClientInterface::OpenPostMessageClose(Id(), TNodeCtxId(iActivityIdForAdapter, Id()), TCFServiceProvider::TStopped(iStoppingReason).CRef());
    	iActivityIdForAdapter = KActivityNull;
    	iStopRequested = EFalse;
	    }
	else
	    {
	    // Agent has stopped and disconnected without this node telling it to
	    // Could be an error in the agent initiating a disconnect
    	__CFLOG_VAR((KAgentSCprTag, KAgentSCprSubTag, _L8("CAgentSubConnectionProvider::ConnectionDownL() - Agent has stopped unexpectedly")));
	    ControlProvider()->PostMessage(Id(), TCFControlProvider::TDataClientGoneDown(KErrDisconnected).CRef());
	    }
	}


/**
Called from the CAgentAdapter. Indicates that the agent has finished getting
authentication data by whatever means it chooses.
*/
void CAgentSubConnectionProvider::AuthenticateCompleteL(TInt aStatus)
   {
   __ASSERT_DEBUG(iActivityIdForAdapter == ECFActivityAuthentication, User::Panic(KSpecAssert_NifManAgtPrCgnts, 4));

   CCredentialsConfig* credentials = AgentProvisionInfo()->Credentials();
   credentials->SetResult (aStatus);

   if (aStatus == KErrNone)
      {
      credentials->SetUserName (iUsername);
      credentials->SetPassword (iPassword);
      }
      else
      {
      credentials->SetUserName (_L(""));
      credentials->SetPassword (_L(""));
      }

   RClientInterface::OpenPostMessageClose(Id(), TNodeCtxId(ECFActivityAuthentication, Id()), TLinkMessage::TAuthenticateResponse().CRef());
   iAuthenticateInProgress = EFalse;
   iActivityIdForAdapter = KActivityNull;
   }


/**
Called from the CAgentAdapter. Indicates that the agent has had a response from
the user deciding whether or not to reconnect
*/
void CAgentSubConnectionProvider::PromptForReconnectComplete(TInt aStatus)
    {
    if (aStatus == KErrNone)
        {
        // Response was to reconnect
        ConnectAgent(EAgentReconnect);
        }
    else
        {
        AgentProvisionInfo()->AgentAdapter()->DisconnectAgent(aStatus);
        }
    }


/**
Posts the progress to the Control Client (SCPr above). There should only ever be
one Control Client for this SCPr.
*/
void CAgentSubConnectionProvider::ProgressL(TInt aStage)
    {
    iLastProgress.iStage = aStage;
    iLastProgress.iError = KErrNone;
    RClientInterface::OpenPostMessageClose(Id(), Id(),
    	TCFMessage::TStateChange(iLastProgress).CRef());
    }



/**
Posts the error to the control clients (Up)
*/
void CAgentSubConnectionProvider::Error(const Elements::TStateChange& aProgress)
    {
    __ASSERT_DEBUG(aProgress.iError, User::Panic(KSpecAssert_NifManAgtPrCgnts, 5));

    iLastProgress = aProgress;
    if (iActivityIdForAdapter != KActivityNull)
        {
        RClientInterface::OpenPostMessageClose(Id(), TNodeCtxId(iActivityIdForAdapter, Id()),
        	TEBase::TError(TCFDataClient::TStart::Id(), iLastProgress.iError).CRef());
        iActivityIdForAdapter = KActivityNull;
        }

    if (CountActivities(ECFActivityStartDataClient))
        {
        __ASSERT_DEBUG(CountActivities(ECFActivityStartDataClient) == 1, User::Panic(KSpecAssert_NifManAgtPrCgnts, 6));
        MeshMachine::CNodeActivityBase* startDataClientActivty = FindActivityById(ECFActivityStartDataClient);
        startDataClientActivty->SetError(iLastProgress.iError);
        TEBase::TCancel cancelMsg;
		TNodeCtxId sender(startDataClientActivty->ActivityId(), Id());
		TNodeCtxId recipient(0, Id());
	    TNodeContext<CAgentSubConnectionProvider> ctx(*this, cancelMsg, sender, recipient);
        startDataClientActivty->Cancel(ctx);
        }
    }


void CAgentSubConnectionProvider::NetworkAdaptorEvent(TNetworkAdaptorEventType aEventType, TUint aEvent, const TDesC8& aEventData, TAny* /*aSource*/)
    {
	if(aEventType == EAgentOriginatedConnectionCommand)
	// added to allow agent to generate connection control commands, such as Stop()
		{
		if(aEvent == EAgentConnectionCommandStop)
			{
			__ASSERT_DEBUG(aEventData.Length()==sizeof(TInt), User::Panic(KSpecAssert_NifManAgtPrCgnts, 7));
			TInt errorCode = *(reinterpret_cast<const TInt*>(aEventData.Ptr()));
			CancelStartOrSendStopToSelf(errorCode);
			}
		}
	else if(aEventType == EEtelEvent)
		{
		if (aEvent == ECurrentNetworkChangeEvent)
			{
			// This is a very specific event for CDMA2000 which is essentially for backwards
			// compatibility with old architecture.  The PSD Agent generates this event to indicate
			// a zone change.  This event needs to be propagated to the Network Config Daemon (NetCfgExt),
			// which is done here.  Technically, the PSD Agent should not be listening for this
			// specific ETel event - the NetCfgExt should be listening directly for the event and handling it
			// itself.  However, this change isn't currently the case hence this forwarding code is present.
			// This should be removed if zone change events are (ever) moved from PSD Agent to NetCfg Daemon.
			TLinkMessage::TAgentEventNotification msg(aEventType, aEvent);
	        PostToClients<TDefaultClientMatchPolicy>(Id(), msg, TClientType(TCFClientType::ECtrl));
			}
		}
    }

void CAgentSubConnectionProvider:: CancelStartOrSendStopToSelf(TInt aError)
/**
Issue a TCFDataClient::TStop message to ourselves.

Used to initiate the stop activity internally when requested from an Agent.

@param aError error code in TCFDataClient::TStop message.
*/
	{
#ifdef SYMBIAN_NETWORKING_CSDAGENT_BCA_SUPPORT	
	if(CountActivities(ECFActivityStartDataClient))
		{
		Elements::TStateChange aProgress;
		aProgress.iError=aError;
		Error(aProgress);
		}
	else
		{
		RClientInterface::OpenPostMessageClose(Id(), Id(), TCFDataClient::TStop(aError).CRef());
		}
#else
		RClientInterface::OpenPostMessageClose(Id(), Id(), TCFDataClient::TStop(aError).CRef());
#endif	
	}

TInt CAgentSubConnectionProvider::NotificationFromAgent(TAgentToFlowEventType aEvent, TAny* aInfo)
    {
    if (aEvent == EAgentToNifEventTypeDisableConnection)
    	{
#ifdef SYMBIAN_NETWORKING_CSDAGENT_BCA_SUPPORT		
		TInt errorCode = KErrCancel;
		if (NULL != aInfo)
			{			
			errorCode = reinterpret_cast<const TInt>(aInfo);
			__ASSERT_DEBUG(errorCode < KErrNone, User::Panic(KSpecAssert_NifManAgtPrCgnts, 8));
			}
		// Request from agent to disconnect using error code specified from agent
		CancelStartOrSendStopToSelf(errorCode);
#else
		CancelStartOrSendStopToSelf(KErrCancel);
#endif
    	}
    else
    	{
	    CAgentNotificationHandler* handler = AgentProvisionInfo()->AgentNotificationHandler();
	    if (handler)
	        {
	        return handler->NotificationFromAgent(aEvent, aInfo);
	        }
    	}

    return KErrNotSupported;
    }


void CAgentSubConnectionProvider::NotificationFromFlow(TFlowToAgentEventType aEvent)
    {
   	CAgentNotificationHandler* handler = AgentProvisionInfo()->AgentNotificationHandler();
   	if (handler)
       	{
       	handler->NotificationFromFlow(aEvent);
       	}
    }

