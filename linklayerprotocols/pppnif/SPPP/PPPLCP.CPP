// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <e32hal.h>
#include <e32math.h>
#include <f32file.h>
#include <random.h>
#include "PPPLOG.H"								// must appear before ss_log.h
#include <comms-infras/ss_metaconnprov.h>
#include <comms-infras/ss_log.h>				// for LOG_NODE_CREATE etc.
#include "PPPLCP.H"
#include "PPPAUTH.H"
#include "PPPCCP.H"
#include "PPPLRD.H"
#include "MSCBCPC.H"
#include "PPP_VER.H"
#include "PppProg.h"
#include <commsdattypeinfov1_1.h>
#include <networking/ppplcp.h>
#include "ncpip.h"
#include "ncpip6.h"
#include "PPPConfig.h"
#include "pppmessages.h"
#include <elements/nm_messages_base.h>
#include <elements/nm_messages_child.h>
#include <comms-infras/ss_nodemessages_dataclient.h>
#include <comms-infras/ss_nodemessages_flow.h>
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <nifvar_internal.h>
#endif

using namespace Elements;
using namespace Messages;
using namespace MeshMachine;
using namespace ESock;

static const TInt KMinValueForMSCallBackCode=1000;

//
// PPP LCP
//

#ifdef __VC32__
// warning C4355: 'this' : used in base member initializer list
#pragma warning (disable:4355)
#endif

#if defined ESOCK_LOGGING_ACTIVE || defined SYMBIAN_TRACE_ENABLE
_LIT8(KNif,"Ppp");
#endif

EXPORT_C CPppLcp* CPppLcp::NewL(ESock::CSubConnectionFlowFactoryBase& aFactory, const Messages::TNodeId& aSubConnId, ESock::CProtocolIntfBase* aProtocolIntf)
	{
	CPppLcp* flow = new (ELeave) CPppLcp(aFactory, aSubConnId, aProtocolIntf);
	return flow;
	}

// Registers as unknown initially as lists won't be initialised
EXPORT_C CPppLcp::CPppLcp(ESock::CSubConnectionFlowFactoryBase& aFactory, const Messages::TNodeId& aSubConnId, ESock::CProtocolIntfBase* aProtocolIntf)
  : CSubConnectionFlowBase(aFactory, aSubConnId, aProtocolIntf), MPppFsm(this, EPppPhaseAll, KPppIdLcp), MPppOptionsExtender(),
    iInitialised(EFalse),
    iSavedError(KErrNone),
    iTerminateAction(MNifIfNotify::EDisconnect),
    iLastRequest(Messages::TNodeSignal::TMessageId()),
    iLastRequestErrored(EFalse)
	{
    __FLOG_OPEN(KCFNodeTag, KNif);
	LOG_NODE_CREATE1(KNif, CPppLcp, " [factory=%08x]", &aFactory);
    __DECLARE_FSM_NAME(_S("LCP"));
	iRecvrList.SetOffset(_FOFF(MPppRecvr, iPppRecvrListLink));
    }

#ifdef __VC32__
#pragma warning (default:4355)
#endif

EXPORT_C CPppLcp::~CPppLcp()
	{
	PppNcpMsCbcpFactory::Delete( iPppMsCbcp );
	iPppMsCbcp=NULL;
	delete iRecvTimeRemMessage;
	iRecvTimeRemMessage = NULL;
	delete iRecvIdentification;
	iRecvIdentification = NULL;

	delete iPppLink;
	iPppLink = NULL;

// N.B.: HDLC link relies on iLogger
#if defined (_DEBUG)
	delete iLogger;
#endif


	delete iPppAcp;
	iPppAcp = NULL;
    delete iCallbackInfo;
	iCallbackInfo = NULL;
	delete iCallbackIETFRequestPacket;
	iCallbackIETFRequestPacket = NULL;
	delete iPppCcp;
	iPppCcp = NULL;
	delete iPppLrd;
	iPppLrd = NULL;
	delete iContainerForDlls;
	iContainerForDlls = NULL;

	ASSERT(iBinder4 == NULL);
	ASSERT(iBinder6 == NULL);

	LOG_NODE_DESTROY(KNif, CPppLcp);

    __FLOG_CLOSE;
	}

//-=========================================================
// Messages::ANode methods
//-=========================================================

EXPORT_C void CPppLcp::ReceivedL(const TRuntimeCtxId& aSender, const TNodeId& aRecipient, TSignatureBase& aMessage)
    {
	/**
	   These messages may be received in the middle of another request, such as start.
	   Therefore they must be handled before the CSubConnectionFlowBase::ReceivedL so
	   that they do not override the last request originator for that
	*/	
   	if (TLinkMessage::ERealmId == aMessage.MessageId().Realm())
       	{
       	switch ((iLastRequest = aMessage.MessageId()).MessageId())
           	{
        case TLinkMessage::TAuthenticateResponse::EId:
			{
        	AuthenticateResponseMessage();
        	break;
			}
        case TLinkMessage::TAgentToFlowNotification::EId:
			{
			TLinkMessage::TAgentToFlowNotification& msg = static_cast<TLinkMessage::TAgentToFlowNotification&>(aMessage);
        	AgentToFlowNotification(msg.iValue);
        	break;
			}
        default:
	    	__FLOG_2(_L8("CPppLcp\tReceivedL() unexpected message received [%08x,%d]"), aMessage.MessageId().Realm(), aMessage.MessageId().MessageId());
			PppPanic(EPppPanic_UnexpectedSCPRMessage);
        	}
        return;
        }
	
	CSubConnectionFlowBase::ReceivedL(aSender, aRecipient, aMessage);
	if (TEBase::ERealmId == aMessage.MessageId().Realm())
    	{
    	switch ((iLastRequest = aMessage.MessageId()).MessageId())
        	{
        case TEBase::TError::EId :
            SubConnectionError(static_cast<TEBase::TError&>(aMessage).iValue);
            break;
        case TEBase::TCancel::EId :
        	if (iMMState != EStarting)
	        	{
	        	User::Leave(KErrNotReady);
	        	}
			DoStopFlow(KErrCancel, MNifIfNotify::EDisconnect);
			break;
        default:
		    __FLOG_2(_L8("CPppLcp\tReceivedL() unexpected message received [%08x,%d]"), aMessage.MessageId().Realm(), aMessage.MessageId().MessageId());
			PppPanic(EPppPanic_UnexpectedSCPRMessage);
			}
		return;
		}
    else if (TEChild::ERealmId == aMessage.MessageId().Realm())
    	{
    	switch ((iLastRequest = aMessage.MessageId()).MessageId())
        	{
        case TEChild::TDestroy::EId :
            Destroy();
            break;
        default:
		    __FLOG_2(_L8("CPppLcp\tReceivedL() unexpected message received [%08x,%d]"), aMessage.MessageId().Realm(), aMessage.MessageId().MessageId());
			PppPanic(EPppPanic_UnexpectedSCPRMessage);
			}
		return;
		}
    else if (TCFDataClient::ERealmId == aMessage.MessageId().Realm())
    	{
    	switch ((iLastRequest = aMessage.MessageId()).MessageId())
        	{
        case TCFDataClient::TStart::EId :
            StartFlowL();
            break;
        case TCFDataClient::TStop::EId :
            StopFlow(static_cast<TCFDataClient::TStop&>(aMessage).iValue);
            break;
        case TCFDataClient::TProvisionConfig::EId:
        	ProvisionConfig(static_cast<TCFDataClient::TProvisionConfig&>(aMessage).iConfig);
        	break;
	    case TCFDataClient::TBindTo::EId :
        	{
            TCFDataClient::TBindTo& bindToReq = message_cast<TCFDataClient::TBindTo>(aMessage);
            if (!bindToReq.iNodeId.IsNull())
             	{
             	User::Leave(KErrNotSupported);
             	}
            RClientInterface::OpenPostMessageClose(Id(), aSender, TCFDataClient::TBindToComplete().CRef());
			}
            break;
        default:
		    __FLOG_2(_L8("CPppLcp\tReceivedL() unexpected message received [%08x,%d]"), aMessage.MessageId().Realm(), aMessage.MessageId().MessageId());
			PppPanic(EPppPanic_UnexpectedSCPRMessage);
			}
		return;
		}
    else if (TCFFlow::ERealmId == aMessage.MessageId().Realm())
    	{
    	switch ((iLastRequest = aMessage.MessageId()).MessageId())
        	{
        case TCFFlow::TBlock::EId :
            if (iBinder4)
            	{
            	iBinder4->BlockFlow(MLowerControl::EDisableAllOtherBindersOnFlow);
            	}
            if (iBinder6)
            	{
            	iBinder6->BlockFlow(MLowerControl::EDisableAllOtherBindersOnFlow);
            	}
            break;
        case TCFFlow::TUnBlock::EId :
            if (iBinder4)
            	{
            	iBinder4->SendFlowOn();
            	}
            if (iBinder6)
            	{
            	iBinder6->SendFlowOn();
            	}
            break;
        default:
		    __FLOG_2(_L8("CPppLcp\tReceivedL() unexpected message received [%08x,%d]"), aMessage.MessageId().Realm(), aMessage.MessageId().MessageId());
			PppPanic(EPppPanic_UnexpectedSCPRMessage);
			}
		return;
		}
    else if (TCFMessage::ERealmId == aMessage.MessageId().Realm())
    	{
		__FLOG_2(_L8("CPppLcp\tReceivedL() unexpected message received [%08x,%d]"), aMessage.MessageId().Realm(), aMessage.MessageId().MessageId());
		PppPanic(EPppPanic_UnexpectedSCPRMessage);
		return;
		}
    else if (TCFMessage::ERealmId == aMessage.MessageId().Realm())
    	{
    	switch ((iLastRequest = aMessage.MessageId()).MessageId())
        	{
        case TCFMessage::TStateChange::EId :
            __FLOG(_L8("WARNING: CPppLcp::\tReceivedL() - I have received a TStateChange and I have ignored it"));
            break;
        default:
		    __FLOG_2(_L8("CPppLcp\tReceivedL() unexpected message received [%08x,%d]"), aMessage.MessageId().Realm(), aMessage.MessageId().MessageId());
			PppPanic(EPppPanic_UnexpectedSCPRMessage);
			}
		return;
		}
    }

//-=========================================================
// Methods for handling SCPR messages
//-=========================================================

EXPORT_C void CPppLcp::SubConnectionGoingDown()
    {
    }

EXPORT_C void CPppLcp::SubConnectionError(TInt /*aError*/)
    {
    }

EXPORT_C void CPppLcp::StartFlowL()
    {
    ASSERT(iMMState == 0);

	if (iSavedError)
		{
		// Provisioning error has already occurred
	    __FLOG_1(_L8("CPppLcp:\tStartFlowL() error %d"), iSavedError);
		User::Leave(iSavedError);
		}

    __FLOG(_L8("CPppLcp::\tStartFlowL()"));

	User::LeaveIfError(DoStartFlow());
    iMMState = EStarting;
    }

EXPORT_C void CPppLcp::StopFlow(TInt aError)
    {
    __FLOG_1(_L8("CPppLcp::\tStopFlow(%d)"), aError);
    iMMState = EStopping;
    DoStopFlow(aError, MNifIfNotify::EDisconnect);
    }

EXPORT_C void CPppLcp::ProvisionConfig(const ESock::RMetaExtensionContainerC& aConfigData)
    {
    iSavedError = KErrNone;

    TRAPD(err, ProvisionConfigL(aConfigData));
    if (err)
        {
        __FLOG_1(_L8("CPppLcp::\tProvisionConfig(): error %d"), err);
        iSavedError = err;		// report error later on TCFDataClient::TStart message - there no response to TProvisionConfig message
        }
    }

EXPORT_C void CPppLcp::Destroy()
    {
    // No-one should be bound to us from above if we are about to disappear.
    ASSERT(iBinder4 == NULL && iBinder6 == NULL);
	DeleteThisFlow();
    }

EXPORT_C void CPppLcp::AuthenticateResponseMessage()
/**
Process link specific messages.
*/
	{
	// Retrieve credentials from control side memory
	ASSERT(iAgentProvision); //should have been cheked before attempting
	                         //authentication.
	iCredentials = iAgentProvision->Credentials();
	ASSERT(iCredentials);
	TInt authErr = KErrNone;
	CPppAuthentication* p = iAuthenticate;
	iAuthenticate = NULL;
	if((authErr = iCredentials->GetResult())==KErrNone)
		{
		iAuthenticateDone = ETrue;
		}
	if (p)
    	{
    	//may have bailed out due to tear down in progress.
    	p->CallAuthenticateComplete(authErr);
    	}


	if (authErr != KErrNone)
    	{
    	PostErrorMessage(authErr);
    	}
	}

EXPORT_C void CPppLcp::AgentToFlowNotification(TInt aNotificationId)
    {
    TInt err;
    if ((err = NotificationMessage(TAgentToNifEventType(aNotificationId))) != KErrNone)
        {
		if (iMMState == EStarted)
			{
			PostFlowDownMessage(err);
			}
		else
			{
			PostErrorMessage(err);
			}
        }
    }

TInt CPppLcp::NotificationMessage(TAgentToNifEventType aNotificationId)
	{
	switch(aNotificationId)
		{
	case EAgentToNifEventTypeModifyInitialTimer:
		// Notification destined for binders
		if (iBinder4)
			{
			return iBinder4->Notification(aNotificationId);
			}
		if (iBinder6)
			{
			return iBinder6->Notification(aNotificationId);
			}
		break;

	case EAgentToNifEventTypeDisableTimers:
	case EAgentToNifEventTypeEnableTimers:
		return Notification(aNotificationId, NULL);

	case EAgentToNifEventTypeGetDataTransfer:
		break;

	default:
		PppPanic(EPppPanic_UnexpectedSCPRMessage);
		}
	return KErrNotSupported;		// keep compiler happy
	}


//
// Utility functions for sending messages to SCPR
//
void CPppLcp::PostProgressMessage(TInt aStage, TInt aError)
	{
	if (iLastStateChange.iStage != aStage || iLastStateChange.iError != aError)
		{
		//unsolicited send of TStateChange message
		iSubConnectionProvider.PostMessage(Id(), TCFMessage::TStateChange(TStateChange(aStage, aError)).CRef());
		iLastStateChange.iStage = aStage;
		iLastStateChange.iError = aError;
		}
	}

void CPppLcp::PostDataClientStartedMessage()
	{
    #ifdef __FLOG_ACTIVE
		TPtrC8 ptr = GetMMStateName();
	   	__FLOG_1(_L8("CPppLcp:\tPostDataClientStartedMessage() - State: %S."), &ptr);
 	#endif

	if (iMMState == EStarting)
    	{
    	iMMState = EStarted;
    	//respond to earlier start request
    	iLastRequestOriginator.ReplyTo(Id(), TCFDataClient::TStarted().CRef());
    	}
    
    #ifdef __FLOG_ACTIVE
    	if(iMMState != EStarted)
    		{
	 	   	__FLOG(_L8("ERROR: CPppLcp:\tPostDataClientStartedMessage() - LCP was unable to start"));
    		}
 	#endif

    __ASSERT_DEBUG(iMMState == EStarted, PppPanic(EPppPanic_LCPFailedToStart));
	}

void CPppLcp::PostFlowDownMessage(TInt aError)
	{
    #ifdef __FLOG_ACTIVE
		TPtrC8 ptr = GetMMStateName();
	   	__FLOG_2(_L8("CPppLcp:\tPostFlowDownMessage() - Error: %d, State: %S."), aError, &ptr);
 	#endif

	ASSERT(aError != KErrNone);
	if (iMMState == EStopping)
	    {
	    if (!iLastRequestErrored)
	    	{
			// Unexpected termination so send TError message
			// Flow has gone down for an expected reason
			iLastRequestOriginator.ReplyTo(Id(), TCFDataClient::TStopped(aError).CRef());
			// The SCPR activity will terminate so wait for the next request
			iLastRequestErrored = EFalse;
	    	}
	    else
	    	{
			// TError message has already been sent so ignore
	    	iLastRequestErrored = EFalse;
			}
	    }
    else if (iMMState == EStarted )
        {
        // Something has occured in the bearer causing the link to go down unexpectedly
        // We use the RNodeInterface version of PostMessage because
        // TDataClientGoneDown is always sent as an unsolicited message. We don't want the
        // activity Id to be overidden.
    	iSubConnectionProvider.PostMessage(Id(), TCFControlProvider::TDataClientGoneDown(aError, iTerminateAction).CRef());
	    }
	else if (iMMState == EStarting)
		{
	    if (!iLastRequestErrored)
	    	{
			// Unexpected termination so send TError message
			iLastRequestOriginator.ReplyTo(Id(), TEBase::TError(TCFDataClient::TStart::Id(), aError).CRef());
			// The SCPR activity will terminate so wait for the next request
			iLastRequestErrored = EFalse;
	    	}
	    else
	    	{
	    	// TError message has already been sent so ignore
	    	iLastRequestErrored = EFalse;
	    	}
		}
    iMMState = EStopped;
	}

void CPppLcp::PostErrorMessage(TInt aError)
	{
	ASSERT(aError != KErrNone);
	// Error is always a response to an earlier received request
	if (iLastRequestOriginator.IsOpen()) // Errors only relevant if responding to something
		{
		iLastRequestOriginator.ReplyTo(Id(), TEBase::TError(iLastRequest, aError).CRef());
		}
	}

void CPppLcp::MaybePostDataClientIdle()
    {
    // Can only send DataClientIdle when the upper layer has unbound and the flow is stopped
	if (iBinder4 == NULL && iBinder6 == NULL && !iSentIdle)
		{
		// TDataClientIdle is sent unsolicited to the control side
   		iSubConnectionProvider.PostMessage(Id(), TCFControlProvider::TIdle().CRef());
		iSentIdle = ETrue;
		}
    }

//-=========================================================
// MFlowBinderControl methods
//-=========================================================
EXPORT_C MLowerControl* CPppLcp::GetControlL(const TDesC8& aProtocol)
    {
    User::LeaveIfError(iSavedError);
    MLowerControl* lowerControl = NULL;
	if (aProtocol.CompareF(KProtocol4()) == 0  ||
		aProtocol.CompareF(KDescIcmp) == 0)
		{
        __FLOG(_L8("CPppLcp:\tGetLowerControlL(KProtocol4)"));
		iBinder4 = CPppBinderIp4::NewL(this);
        lowerControl = iBinder4;
		}
	else
	if (aProtocol.CompareF(KProtocol6()) == 0)
		{
        __FLOG(_L8("CPppLcp:\tGetLowerControlL(KProtocol6)"));
        iBinder6 = CPppBinderIp6::NewL(this);
        lowerControl = iBinder6;
		}
    return lowerControl;
    }

EXPORT_C MLowerDataSender* CPppLcp::BindL(const TDesC8& aProtocol, MUpperDataReceiver* aReceiver, MUpperControl* aControl)
    {
    __FLOG_1(_L8("CPppLcp:\tBindL(%S)"), &aProtocol);

	MLowerDataSender* lowerDataSender = NULL;

	if (aProtocol.CompareF(KProtocol4()) == 0)
		{
		lowerDataSender = iBinder4->BindL(*aReceiver, *aControl);
		}
	else
	if (aProtocol.CompareF(KProtocol6()) == 0)
		{
		lowerDataSender = iBinder6->BindL(*aReceiver, *aControl);
		}

	if (lowerDataSender)
    	{
    	// TDataClientActive is sent unsolicited to the control side
    	iSubConnectionProvider.PostMessage(Id(), TCFControlProvider::TActive().CRef());
    	}

	return lowerDataSender;
    }

EXPORT_C void CPppLcp::Unbind(MUpperDataReceiver* aReceiver, MUpperControl* aControl)
    {
    __FLOG(_L8("CPppLcp:\tUnbind()"));

	if (iBinder4 && iBinder4->MatchesUpperControl(aControl))
		{
        iBinder4->UnBind(*aReceiver, *aControl);
		delete iBinder4;
		iBinder4 = NULL;
		}
	else if (iBinder6 && iBinder6->MatchesUpperControl(aControl))
		{
        iBinder6->UnBind(*aReceiver, *aControl);
		delete iBinder6;
		iBinder6 = NULL;
		}

    MaybePostDataClientIdle();
    }

EXPORT_C CSubConnectionFlowBase* CPppLcp::Flow()
	{
	return this;
	}

//-=========================================================
// MUpperDataReceiver methods
//-=========================================================
EXPORT_C void CPppLcp::Process(RMBufChain& aData)
    {
    LinkRecv(aData);
    }

//-=========================================================
// MUpperControl methods
//-=========================================================
EXPORT_C void CPppLcp::StartSending()
    {
    if (iBinder4)
    	{
    	iBinder4->SendFlowOn();
    	}
    if (iBinder6)
    	{
    	iBinder6->SendFlowOn();
    	}
    }

EXPORT_C void CPppLcp::Error(TInt aError)
	{
	if (iLastRequestOriginator.IsOpen())
		{
		// There is an outstanding request to complete
		iLastRequestOriginator.ReplyTo(Id(),TEBase::TError(iLastRequest, aError).CRef());
		iLastRequestErrored = ETrue;
		}
	else
		{
		// There is no outstanding request to complete
		// This is a spurious error that will be ignored by
		// the SCPR
		__ASSERT_DEBUG(EFalse, PppPanic(EPppPanic_SpuriousError));
		}
    iLastRequest = Messages::TNodeSignal::TMessageId();
	}

//-=========================================================
// Custom methods
//-=========================================================
// provide compatability for classes using old CNifIfLink style API
void CPppLcp::ProvisionConfigL(const ESock::RMetaExtensionContainerC& aConfigData)
/**
Handle ProvisionConfig messages from SCPR.

Provisioning requirements:

- On receipt of a ProvisionConfig message, provisioning information in the SAccessPointConfig array
  is validated, and the following structures must be present:
  	- CBCAProvision, CIPConfig, CPppLcpConfig, CPppAuthConfig
  if any are missing, then an error is saved in iSavedError (there is no response to ProvisionConfig).

- on receipt of a StartFlow message:
  - if iSavedError is not KErrNone, send a Error message to SCPr
  - search SAccessPointConfig again (in RetrieveDynamicProvisionInfo()), for Agent provisioning
    information:
  	- CAgentProvisionInfo, CPppProvisionInfo
  	if any are missing, send a Error message to SCPr
*/
    {
	// Retrieve MetaConnectionProvider provisioned information
  	iBCAProvision = static_cast<const CBCAProvision*>(
  	      aConfigData.FindExtension(STypeId::CreateSTypeId(CBCAProvision::EUid, CBCAProvision::ETypeId)));
  	iPppNcpConfig = static_cast<const CIPConfig*>(
  	      aConfigData.FindExtension(STypeId::CreateSTypeId(CIPConfig::EUid, CIPConfig::ETypeId)));
  	iPppLcpConfig = static_cast<const CPppLcpConfig*>(
  	      aConfigData.FindExtension(STypeId::CreateSTypeId(CPppLcpConfig::EUid, CPppLcpConfig::ETypeId)));
  	iPppAuthConfig  = static_cast<const CPppAuthConfig*>(
  	      aConfigData.FindExtension(STypeId::CreateSTypeId(CPppAuthConfig::EUid, CPppAuthConfig::ETypeId)));

    if (iBCAProvision == NULL || iPppNcpConfig == NULL || iPppLcpConfig == NULL || iPppAuthConfig == NULL)
        {
        __FLOG_4(_L8("CPppLcp:\tError: ProvisionConfigL() - PPP config incomplete: iBCAProvision %08x, iPppNcpConfig %08x, iPppLcpConfig %08x, iPppAuthConfig %08x"),
        		 iBCAProvision, iPppNcpConfig, iPppLcpConfig, iPppAuthConfig);
        User::Leave(KErrCorrupt);
        }

    iAccessPointConfig.Close();
	iAccessPointConfig.Open(aConfigData);

	if (!iInitialised && GetLcpConfig() && GetNcpConfig() && GetAuthConfig() && GetBCAProvision())
		{
		InitL();
		iInitialised = ETrue;
		}
    }

TInt CPppLcp::RetrieveDynamicProvisionInfo()
/**
Retrieve Provisioning Information which becomes available at Flow start time.

@return KErrNone if success, KErrCorrupt if information not available.
*/
	{
	// Retrieve Agent provisioned information.
	// The only thing PPP needs from the Agent provisioned information are the credentials,
	// whilst PPP doesn't always pursue authentication. It's ok to continue for now.
	iAgentProvision = static_cast<const CAgentProvisionInfo*>(AccessPointConfig().FindExtension(STypeId::CreateSTypeId(CAgentProvisionInfo::EUid, CAgentProvisionInfo::ETypeId)));
#ifdef _DEBUG
	if (iAgentProvision == NULL)
    	{
        __FLOG_0(_L8("CPppLcp:\tRetrieveDynamicProvisionInfo() - Generic agent config not found, continuing"));
    	}
#endif

	// Retrieve PPP specific Agent provisioned information
	iPppAgentProvision = static_cast<const CPppProvisionInfo*>(
          AccessPointConfig().FindExtension(STypeId::CreateSTypeId(CPppProvisionInfo::EUid, CPppProvisionInfo::ETypeId)));

    if (iPppAgentProvision == NULL)
        {
        __FLOG_0(_L8("CPppLcp:\tRetrieveDynamicProvisionInfo() - PPP agent config not found"));
        return KErrCorrupt;
        }
	return KErrNone;
	}

void CPppLcp::Stop(TInt aReason, MNifIfNotify::TAction aAction)
    {
    DoStopFlow(aReason, aAction);
    }

MLowerDataSender::TSendResult CPppLcp::Send(RMBufChain& aPdu, TUint aProtocol)
    {
    return MLowerDataSender::TSendResult(iPppLink->Send(aPdu, aProtocol));
    }

//
// Upcalls from Binders
//

// BinderLinkUp() overriden by CFProtocols derived from CPppLcp
EXPORT_C void CPppLcp::BinderLinkUp(TPppProtocol aProtocol)
	{
	(void)aProtocol;
	ASSERT(aProtocol >= CPppLcp::EPppProtocolBegin && aProtocol <= CPppLcp::EPppProtocolEnd);
	FlowUp();
	}

void CPppLcp::Progress(TInt aStage, TInt aError)
/**
Upcall from binders to send progress

@param aProgress progress parameters
@param aProtocol protocol.  This field is primarily for the benefit of CFProtocols that
derive from CPppLcp, so that they can identify which binder type is issuing the progress.

*/
	{
    __FLOG_2(_L8("CPppLcp:\tProgress(%d, %d)"), aStage, aError);
	PostProgressMessage(aStage, aError);
	}

void CPppLcp::FlowUp()
    {
    __FLOG(_L8("CPppLcp:\tFlowUp()"));
	PostDataClientStartedMessage();
    }

void CPppLcp::FlowDown(TInt aError, MNifIfNotify::TAction aAction)
    {
    __FLOG_2(_L8("CPppLcp:\tFlowDown(%d, %d)"), aError, aAction);

	// If ENoAction is specified, this is just an
    // informational call indicating that the link has gone
    // down briefly and is expected to go back up shortly.
    if(aAction == MNifIfNotify::ENoAction)
    	{
    	return;
    	}
    	
    if (aError == KErrNone)
        {
        //It is quite common for the fsm to pass an errorless down signal.
        aError = KErrUnknown;
        }

    //awaiting FsmTermination phase complete to post flow down
    iTerminateAction = aAction;
    PostFlowDownMessage(aError);
    }


//-=========================================================
// For CPPPAuthenticate methods - might be worth moving them elsewhere.
//-=========================================================
void CPppLcp::Authenticate(CPppAuthentication* aAp)
	{
	if(iAuthenticateDone)
		{
	    __FLOG(_L8("CPppLcp:\tAuthenticate(): calling CallAuthenticateComplete()"));
		aAp->CallAuthenticateComplete(KErrNone);
		return;
		}

	if(iAuthenticate)
    	{
    	aAp->CallAuthenticateComplete(KErrAlreadyExists);
    	return;
    	}

    if (iAgentProvision == NULL)
        {
        //PPP decides to authenticate, but there is no
        //agent provision structure holding the credentials.
        aAp->CallAuthenticateComplete(KErrCorrupt);
        }

    __FLOG(_L8("CPppLcp:\tAuthenticate(): sending Authenticate message"));

	// Send Authenticate message to SCPR requesting credentials for authentication.
	// TAuthenticate is sent unsolicited to the control side
	iSubConnectionProvider.PostMessage(Id(), TLinkMessage::TAuthenticate().CRef());

    iAuthenticate = aAp;
	}

void CPppLcp::CancelAuthenticate(CPppAuthentication* aAp)
	{

	if(aAp==iAuthenticate)
		{
		iAuthenticate=0;
		iLastRequestOriginator.ReplyTo(Id(), TEBase::TCancel().CRef());
		}
	}

//-=========================================================
// CSubConnectionFlowBase methods
//-=========================================================
EXPORT_C MFlowBinderControl* CPppLcp::DoGetBinderControlL()
    {
    return this;
    }

// ---- PPP Specifics ----
void CPppLcp::InitL()
/**
Construct the Link Protocol Object

@exception leaves if could not allocate memory
@returns none
 */
	{
	ASSERT(!iInitialised && GetLcpConfig() && GetNcpConfig() && GetAuthConfig() && GetBCAProvision());

#ifdef ESOCK_LOGGING_ACTIVE
	// instantiate logging object

	ASSERT(iLogger == NULL);

	iLogger = CPppLog::NewL();
	iLogLevel = 1; //Set default log level

	LOG( iLogger->Printf(_L("-----------------------------------------------------------------------------------------")); )
	LOG( iLogger->Printf(_L("---- New PPP instance [0x%+08x] ------------------------------------------------------"), this); )
	LOG( iLogger->Printf(_L("-----------------------------------------------------------------------------------------")); )

  	// Determine the link mode early so everyone can read it
  	iLinkMode = GetLcpConfig()->GetIfServerMode();
  	LOG(
  		if (iLinkMode == CPppLcpConfig::EPppLinkIsServer)
  			{
 			iLogger->Printf(_L("PPP running in server mode"));
  			}
  		else
  			{
  			iLogger->Printf(_L("PPP running in client mode"));
  			}
  		)
#endif

	FsmConstructL();


	// Use HDLC as lower layer
	iPppLink = PppLink()->PppCreateLinkL(this, KNullDesC);

	if(iPppLink)
		{
		iContainerForDlls = CObjectConIx::NewL();

		//
		// Note: Authentication credentials (iCredentials) are initialised from SCPR
		// provisioning information.
		//

		// Create compression control protocol object
		iPppCcp = CPppCcp::NewL(this);

		// Create authentication protocol configuration object
		iPppAcp = CPppAcp::NewL(this);

		// Create link quality checking object
		iPppLrd = CPppLrd::NewL(this);
		Register();
		}
	}

void CPppLcp::CallbackGrantedAndAuthenticated()
/**
To be called when the authentication (if any) has been completed successfully.
If there's no authentication at all, this should be called at the
point where the authentication phase would otherwise be.

If the other end has agreed to our request for callback, then this causes NETDIAL
to be notified that it's "all systems go" for callback.

If the other end has not confirmed accepted our request for callback, or if we haven't asked,
then this does nothing.
*/
	{
	if (! iCallbackRequestGranted)
		return;

	iCallbackRequestGranted = EFalse;	// do this once only (cheat)

	//
	// The following used to be Notification(ENifToAgentEventTypePPPCallbackGranted), but
	// has been converted to a progress to avoid having to define a special Flow to SCPR
	// Notification() message.
	PostProgressMessage(EPppProgressCallbackGranted, KErrNone);
	}


static const TInt KMaxCallbackRequestInfo = 256;

class CallbackAction
	{
public:
	static INLINE TBool	IsInfoFieldRequired( TCallbackAction aCallbackAction );
	static INLINE TCallbackIETFRequestType  IETFRequestType( TCallbackAction aCallbackAction );
	static INLINE TBool IsMSCBCP( TCallbackAction aCallbackAction );
	static INLINE TMSCBCPAction MSCBCPAction( const TCallbackAction aAction );
	};


INLINE TMSCBCPAction CallbackAction::MSCBCPAction( const TCallbackAction aAction )
	{
	__ASSERT_DEBUG( aAction == ECallbackActionMSCBCPRequireClientSpecifiedNumber
						|| aAction == ECallbackActionMSCBCPAcceptServerSpecifiedNumber
						|| aAction == ECallbackActionMSCBCPOverrideServerSpecifiedNumber,
					PppPanic(EPppPanic_PPPInvalidCallback) );

	return (TMSCBCPAction) aAction;
	}


TBool CallbackAction::IsInfoFieldRequired( TCallbackAction aCallbackAction )
	{
	switch (aCallbackAction)
		{
	case ECallbackActionIETFType0:
		return EFalse;
	case ECallbackActionIETFType1:
		return ETrue;
	case ECallbackActionIETFType2:
		return ETrue;
	case ECallbackActionIETFType3:
		return ETrue;
	case ECallbackActionIETFType4:
		return ETrue;
	case ECallbackActionIETFType5:
		return ETrue;
	case ECallbackActionMSCBCPRequireClientSpecifiedNumber:
		return ETrue;
	case ECallbackActionMSCBCPAcceptServerSpecifiedNumber:
		return EFalse;
	case ECallbackActionMSCBCPOverrideServerSpecifiedNumber:
		return ETrue;
	default:
		__ASSERT_DEBUG(EFalse, PppPanic(EPppPanic_PPPInvalidCallback));
		return EFalse;
		}
	}


inline TBool CallbackAction::IsMSCBCP( TCallbackAction aCallbackAction )
	{
	return int(aCallbackAction) > KMinValueForMSCallBackCode;	// MS codes are >1000
	}


TCallbackIETFRequestType CallbackAction::IETFRequestType( TCallbackAction aCallbackAction )
	{
	return CallbackAction::IsMSCBCP( aCallbackAction )
				? ECallbackIETFRequestTypeMSCBCP
				: TCallbackIETFRequestType( aCallbackAction );
	}



TBool CPppLcp::QueryExternalIPConfiguration()
/**
Returns the external IP configuration flag.
@return	External IP configuration flag value. */
	{
	return iDoExternalIPConfiguration;
	}

void CPppLcp::InitCallbackInfoL()
	{
	if ( !GetLcpConfig()->GetIfCallbackEnabled() )
    	{
    	return;
    	}
	if ( GetPppAgentConfig()->IsDialIn() )
    	{
    	return;
    	}

	iCallbackEnabled = ETrue;
	iCallbackAction = TCallbackAction(GetLcpConfig()->GetIfCallbackType());
	iCallbackIETFRequestType = CallbackAction::IETFRequestType( iCallbackAction );
	NewCallbackRequestInfoL( iCallbackIETFRequestType );

	if ( CallbackAction::IsMSCBCP( iCallbackAction ) )
		{
		if ( CallbackAction::IsInfoFieldRequired( iCallbackAction ) )
			{
			// Get the info field from the NETDIAL database, for the benefit of MS-CBCP
			GetCallbackInfoL();	// sets iCallbackInfo
			}
		iPppMsCbcp = PppNcpMsCbcpFactory::NewL(	this,				// CPppLcp* aLcp,
												CallbackAction::MSCBCPAction( iCallbackAction ),	// TMSCBCPAction aAction,
												iCallbackInfo,		// TUint8*  aCallbackInfo,
												iCallbackInfoLen ); // TInt aCallbackInfoLen
		}
	}


void CPppLcp::GetCallbackInfoL()
/**
Get the CallbackInfo field from the NETDIAL database
Allocates a buffer for it, and (if successful)
sets iCallbackInfo to point at the buffer allocated.

@post iCallbackInfo, iCallbackInfoLen are set
*/
	{
	if ( iCallbackInfo )	// already done it
        {
        return;
        }
  	TUint8*  p = new (ELeave) TUint8[ KMaxCallbackRequestInfo ];
  	TPtr8 tptr( p, KMaxCallbackRequestInfo );
    tptr.Copy(GetLcpConfig()->GetIfCallbackInfo());
  	TInt len = tptr.Length();

  	iCallbackInfo = p;
  	iCallbackInfoLen = len;
	__ASSERT_DEBUG( iCallbackInfo, PppPanic(EPppPanic_PPPNoCallbackInfo) );
	__ASSERT_DEBUG( iCallbackInfoLen <=KMaxCallbackRequestInfo, PppPanic(EPppPanic_PPPInvalidCallback) );
	}

void CPppLcp::NewCallbackRequestInfoL( TCallbackIETFRequestType aRequestType )
/**
Allocates a buffer containing a PPP callback request packet a la draft RFC.

Format is: [ length ] [ type ] [ .. info ... ]

@post Sets iCallbackIETFRequestPacket to point to the buffer allocated

@param aRequestType Callback IETF request type code
*/
	{

	if ( iCallbackIETFRequestPacket )	// if buffer exists already, delete it
		{
		delete iCallbackIETFRequestPacket;
		iCallbackIETFRequestPacket = NULL;
		}
	TInt len;
	if ( aRequestType == ECallbackIETFRequestType0 || aRequestType == ECallbackIETFRequestTypeMSCBCP)
		{
		len = 0;
		}
	else
		{
		GetCallbackInfoL();	// Sets iCallbackInfo to be valid
		__ASSERT_DEBUG( iCallbackInfo, PppPanic(EPppPanic_PPPNoCallbackInfo) );
		len = iCallbackInfoLen;
		}
	__ASSERT_DEBUG( !iCallbackIETFRequestPacket, PppPanic(EPppPanic_PPPInvalidCallback) );

	// Allocate space for the Operation byte i.e. aRequestType ( 0 - 4 )
	iCallbackIETFRequestPacket = new (ELeave) TUint8[ len + 1 ];
	// Set the length to include the Operation byte
	iCallbackIETFRequestPacketLen = len+1;
	// Set up the Operation Byte
	iCallbackIETFRequestPacket[0] = TUint8( aRequestType );
	// Append the database retrieved information field (Could be a Phone Number)
	// Generic code will add Callback type code and length byte later on
	if(len)
		Mem::Copy( iCallbackIETFRequestPacket +1, iCallbackInfo, len);
	}


void CPppLcp::PhaseAdvancesBeyondAuthenticate()
	{
	CallbackGrantedAndAuthenticated();
	}


TBool CPppLcp::DoesDllExist(const TDesC& aFilename)
/**
Search to see if a loadable DLL does actually exist

@param aFilename DLL filename

@return ETrue if the file exists
*/
	{
	TBool RetCode = EFalse;


    RLibrary lib;
	_LIT(KSysLibs,"\\SYSTEM\\LIBS\\");
    if (lib.Load(aFilename, KSysLibs) == KErrNone)
        {
        RetCode = ETrue;
        lib.Close();
        }

	return RetCode;
	}

TInt CPppLcp::DoStartFlow()
/**
Start the PPP link.

@return Error code
*/
	{
	TInt err = KErrNone;
#if defined (_DEBUG)
	// Move logging to new log file
	TRAP(err, iLogger->SetLogFileNameL(GetNcpConfig()->GetPortName()));
	if (err!=KErrNone)
		{
		// ** DO NOT forget to enclose any LOG() macros after an 'if' within '{...}' or else
		// the first innocent source line after the LOG() statement will be swallowed
		// by the 'if' when LOG() is empty !!
		LOG( iLogger->Printf(_L("Could not change log file name for this instance of PPP [0x%+08x]"), this); )
		}

	if (GetLcpConfig()->GetISPName().Ptr())
		{
		_LIT(KLine64, "---------------------------------------------------------------");
		LOG( iLogger->Printf(KLine64); )  ;
		LOG( iLogger->Printf(_L("Calling ISP '%S' using modem on %S"), GetLcpConfig()->GetISPName().Ptr(), GetNcpConfig()->GetPortName().Ptr()); ) ;
		LOG( iLogger->Printf(KLine64); ) ;
		}
#endif

	err = RetrieveDynamicProvisionInfo();
	if (err != KErrNone)
		{
		return err;
		}

    iDoLcpExts=GetLcpConfig()->GetEnableLcpExtensions();

	// Are we doing external IP configuration (MobileIP)?
	TRAPD(res, PppLink()->StartL();)

	//Are we doing CCP ?
	if (!GetLcpConfig()->GetEnableSwComp())
		{
		//Deregister CCP
		if(iPppCcp)
			iPppCcp->RemoveRegistration();
		}

	// Configure the FSM to never time out while connecting if we're a server
	ConnectionPersist(PppLinkMode() == CPppLcpConfig::EPppLinkIsServer);

	return res;
	}

void CPppLcp::DoStopFlow(TInt aReason, MNifIfNotify::TAction aAction)
/**
Cleanly stop the LCP link and notify the NIF.
If aReason == KErrConnectionTerminated signal Authoritative stop: there is no graceful
termination: PPP signals CNifAgentRef::LinkLayerDown to Nifman, which results in PPP deletion.

@param aReason The reason the link is going down
@param aAction The action to take once the link is down
*/
	{
 	LOG( iLogger->Printf(_L("CPppLcp::Stop: Administrative Close of PPP. aReason [%d], aAction [%d]"),aReason, aAction); )
 	iFsmTerminationCauseError = aReason;

 	// If it's an Authoritative Stop (KErrConnectionTerminated),
 	// we shut down as quickly as possible, with no concern for
 	// graceful termination. The assumption is that other networking components do the same, i.e.
 	// they do not expect us to shut down gracefully.
 	// We do not go through RFC-compliant PPP Termination sequence, because shutting down and releasing resources
 	// has higher priority than negotiationg with the peer.
	if(KErrConnectionTerminated != aReason)  // Normal stop
		{
		if(iTerminateRequestEnabled) // Fully RFC1661 compliant shutdown, as opposed to "legacy" shutdown.
		                             // Once the "legacy" shutdown (provided for back-compatibility) is removed,
		                             // the check for TerminateRequestEnabled may be safely removed as well.
			{
			FsmLayerDown(aReason);
 			TerminateLink(aAction, aReason);
 			return;
			}
		else // "Legacy" shutdown sequence.
		     // PPP terminates immediately, after making a "best-effort" attempt to send 1 LCP TerminateRequest.
		     // Once this legacy behaviour is not required any more, this section of code may be safely removed.
			{
			TerminateLink(aAction, aReason); // "Best-effort" attempt to send TerminateRequest.
			// Proceed to "Authoritative" shutdown. This may result in TerminateRequest being cancelled.
			}
		}

	// This is the "Authoritative" shutdown sequence. If we are terminating gracefully, we will not get here.
	// Inform Nifman that we are ready to be cleaned-up. We do not do full clean-up here,
 	// because this is done anyway in destructors, which will be invoked once Nifman tells PPP to delete itself.
 	// At this point we want to unblock Nifman as quickly as possible, to allow it to proceed with the rest
 	// of the Authoritative shutdown sequence.
	PppLink()->Stop(aReason); // Make sure PPP does not use the Link after telling nifman that the NIF is finished.

 	// We need to inform Nifman from here only if it is authoritative shutdown.
 	// In other cases, FSM goes through state transitions on shutdown, which result
 	// in FsmTerminationPhaseComplete to be called, and notify the Nifman.
 	if(KErrConnectionTerminated == aReason)
 		{
 		// Tell Nifman that PPP is finished. After the Agent finishes (terminates the connection, air link, etc), Nifman
 		// closes the NIF, which results in PPP deletion. The call to delete PPP originates in Nifman, implemented as
 		// asynchronous callback. This call would invoke the destructors, which do the actual cleanup in the NIF.
 	 	//iNotify->LinkLayerDown(aReason, aAction); REMEK: DO SOMETHING HERE - TALK TO NADEEM,
 	 	                                          //the following line has been added as a replacement of this line.

 		PostProgressMessage(EPppProgressLinkDown, aReason);
 	 	PostFlowDownMessage(aReason);
 		}
	}

//
// Protocols Manager
//

TInt CPppLcp::PppOpen()
/**
Called by an upper layer when it is started.

@return Error code
*/
	{

	LOG( iLogger->Printf(_L("PPPOpen - iOpenCount=%d\n"),iOpenCount); )
	if (iOpenCount++>0)
		return KErrNone;

	return FsmOpen();
	}

void CPppLcp::PppClose(TInt aReason)
/**
Called by an upper layer when it is finished.

@param aReason The reason the protocol is finished
*/
	{
	LOG( iLogger->Printf(_L("PPPClose - iOpenCount=%d\r\n"),iOpenCount); )
	if (--iOpenCount>0)
		{
		if (AllNcpsUp()) //This will need to happen when one ncp goes up but the other does not go up
			{
			FlowUp();
			}
		return;
		}
	FsmClose(aReason);
	iNcpUpCount = 0;
	}

void CPppLcp::RegisterRecvr(MPppRecvr* aRecvr)
/**
Register an object that is interested in receiving certain packets.
The receiver object knows the protocol number it is interested in.

@param aRecvr Object to receive packets
*/
	{
	iRecvrList.AddLast(*aRecvr);
	if (aRecvr->iPppId==KPppIdLcp)
		return;

	if (iPhase>=aRecvr->iActivePhase)
			aRecvr->LowerLayerUp();

	else
		aRecvr->LowerLayerDown(KErrNone);
	}

void CPppLcp::ReregisterRecvr(MPppRecvr* aRecvr)
/**
Reregister an object that is receiving certain packets.
The receiver object knows the protocol number it is interested in.

@param aRecvr Object registered to receive packets
*/
	{
	aRecvr->Deque();
	RegisterRecvr(aRecvr);
	}

void CPppLcp::DeregisterRecvr(MPppRecvr* aRecvr)
/**
Unregister an object that is no longer interested in receiving certain
packets.

@param aRecvr Object registered to receive packets
*/
	{
	aRecvr->Deque();
	}

//
// PPP Upcalls from Link
//

void CPppLcp::LinkRecv(RMBufChain& aPacket)
/**
Called by the link layer for each new PPP packet received.
Search registered Recvr list and deliver to all recipients.

@param aPacket MBuf chain containing received packet
*/
	{
	MPppRecvr* rcvr;

	RMBufPktInfo* info = RMBufPacket::PeekInfoInChain(aPacket);
	TUint prot = TPppAddr::Cast(info->iDstAddr).GetProtocol();

   //
   // For Van Jacobson compression have to tell NCPIP if a CRC error
   // occurred, may as well tell all the protocols!!
   //
   const TBool CrcError = TPppAddr::Cast(info->iDstAddr).CRCError();

	TDblQueIter<MPppRecvr> iter(iRecvrList);
	while (rcvr = iter++, rcvr!=NULL)
		{
		if (CrcError)
			{
			rcvr->FrameError();
			continue;
			}

		if (prot==rcvr->iPppId)
			{
			if (iPhase>=rcvr->iActivePhase)
				{
				rcvr->RecvFrame(aPacket);
				return;
				}
			else
				{
				// Protocol is known, but not available so
				// silent discard instead of reject
				aPacket.Free();
				}
			}
		}

   if (CrcError)
      {
      //
      // Have told everyone now so throw away the frame
      //
		aPacket.Free();
      }


	if (!aPacket.IsEmpty())
		{
		// Unrecognised protocol
		// This line was commented out and consequently NBCF requests were not
		// being rejected.


 		// Cisco Router fix
 		// There is a defect in some Cisco routers where they will send
 		// an authentication request before they have sent an LCP Config-Ack
 		// So if we are in the Ack-Sent state and we get an authentication request
 		// - i.e. CHAP (0xC223) or PAP (0xC023)) - then silently discard the packet
 		// rather than sending a Protocol reject

 		if ((iPhase == EPppPhaseEstablish) &&
 			(FsmState() == EPppFsmAckSent) &&
 			((prot == KPppIdChap) || (prot == KPppIdPap)))
 			{
 			; // don't send a protocol reject
 			LOG( iLogger->Printf(_L("CHAP / PAP Authenticate request received before entering the AUTHENTICATE phase: silently discarding packet\n")); )
 			}
 		else
 			FsmRejectPacket(aPacket, KPppLcpProtocolReject);

		aPacket.Free();
		}
	}

void CPppLcp::LinkLayerUp()
/**
Called from the lower layer when it becomes ready.
*/
	{
	LOG( iLogger->Printf(_L("Link Layer Up")); )
	MPppRecvr* rcvr = this;
	rcvr->LowerLayerUp();
//	PppOpen();
	}

void CPppLcp::LinkLayerDown(TInt aStatus)
/**
Called from the lower layer when it finishes shutting down.

@param aStatus Error code indicating the reason the link is going down
*/
	{
	// Note on errors:
	// Link can be down due to 1 of 2 causes: Comms failure of some sort, OR closing the link
	// by LCP. In the first case, we should get the Comms error from the link in aStatus.
	// In the second case,we have our own error describing why LCP is shutting down the link.

	if(KErrNone == iError) // Link reported failure. We didn't ask it to be closed
		{
		if(KErrNone == aStatus) // We have no error to work with!
			{
			// We absolutely must signal an error, otherwise Nifman will
			// ignore NIF's notification in certain states (e.g. before KLinkLayerUp).
			// This results in the interface and its ESock clients being stuck.
			//
			// After the first (outgoing) stage of callback, however, signalling EPppProgressLinkDown
			// with an error (in FsmTerminationPhaseComplete()) incorrectly triggers Nifman to perform
			// a connection retry with the next preference.  Passing KErrNone in this particular
			// circumstance is okay.
			//
			if (iTerminateAction != MNifIfNotify::ECallBack)
				{
				iError = KErrUnknown;
				}
			}
		else
			{
			iError = aStatus; // Report link error as PPP error.
			}
		}
	// Else: we have an error code already. Most likely, as result of closing the link.

	LOG( iLogger->Printf(_L("Link Layer Down reason %d\n"), aStatus); )
	MPppRecvr* rcvr = this;
	rcvr->LowerLayerDown(aStatus);
	}


void CPppLcp::LinkFlowOn()
/**
Link advisory flow on signal.
*/
	{
	MPppRecvr* rcvr;
	TDblQueIter<MPppRecvr> iter(iRecvrList);
	while (rcvr = iter++, rcvr!=NULL)
		rcvr->FlowOn();
	}


void CPppLcp::PhaseAdvance(TPppPhase aNewPhase)
/**
Switch to the next PPP operation phase.
When the PPP phase is advanced, protocols may have reached
their active phase and so have to be bound in to the
main data path and signalled active.

@param aNewPhase Phase just entered
*/
	{
	TPppPhase oldphase = iPhase;
	iPhase = aNewPhase;
	LOG( iLogger->DumpPhase(oldphase, aNewPhase); )
	if (oldphase==aNewPhase)
		return;
	TBool skip = ETrue;

	if (!iRecvrList.IsEmpty())
		{
		MPppRecvr* rcvr;
		TDblQueIter<MPppRecvr> iter(iRecvrList);
		while (rcvr=iter++, rcvr!=NULL)
			{
			if (rcvr->iPppId==KPppIdLcp)
				continue;
			if (aNewPhase==rcvr->iActivePhase)
				{
				skip = EFalse;

				rcvr->LowerLayerUp();

				}
			else if (oldphase<rcvr->iActivePhase && aNewPhase>rcvr->iActivePhase)
				rcvr->LowerLayerUp();
			}
		}
	if (skip && aNewPhase>EPppPhaseEstablish && aNewPhase<EPppPhaseNetwork)
		PhaseComplete();
	}

void CPppLcp::TerminateLink(MNifIfNotify::TAction aAction, TInt aError)
/**
Cleanly take down the LCP (and therefore PPP) link.

@param aAction The action to take after the link terminates
@param aError The reason the link is going down
*/
{
	iError = aError;
	iTerminateAction = aAction;
	LOG( iLogger->Printf(_L("TerminateLink due to error %d"), aError); )
	MPppFsm::TerminateLink();
	iGotCallbackInfo = EFalse;
}

void CPppLcp::PhaseRetard(TPppPhase aNewPhase, TInt aReason)
/**
Return to a previous PPP operation phase.
When the PPP phase is retarded, active protocols may no
longer be in their active phase and so have to be unbound
from the main data path and signalled inactive.

@param aNewPhase Phase just entered
@param aReason Error code indicating the reason we are changing phases
*/
	{
	LOG( iLogger->Printf(_L("PhaseRetard, reason[%d]"), aReason); )
	if (aReason==KErrCommsLineFail && FsmIsThisLayerOpen())		// going to do reconnection
		iGotCallbackInfo=EFalse;

	TPppPhase oldphase = iPhase;
	iPhase = aNewPhase;
	LOG( iLogger->DumpPhase(oldphase, aNewPhase); )
	if (oldphase==aNewPhase)
		return;

	// Terminate the link from here if and only if it is NCP failure of some sort
	switch(aReason)
		{
		case KErrIfAuthenticationFailure:
		case KErrIfAuthNotSecure:
		case KErrIfAccountDisabled:
		case KErrIfRestrictedLogonHours:
		case KErrIfPasswdExpired:
		case KErrIfNoDialInPermission:
		case KErrIfChangingPassword:
		case KErrIfCallbackNotAcceptable:
		case KErrIfDNSNotFound:
			TerminateLink(MNifIfNotify::EDisconnect, aReason);
			Error(aReason);
		default:
			break;
		}


	if (!iRecvrList.IsEmpty())
		{
		MPppRecvr* rcvr;
		TDblQueIter<MPppRecvr> iter(iRecvrList);
		while (rcvr=iter++, rcvr!=NULL)
			{
			if (rcvr->iPppId==KPppIdLcp)
				continue;

			if (/*oldphase>=rcvr->iActivePhase &&*/ aNewPhase<rcvr->iActivePhase/* || aReason!=KErrNone*/)
				rcvr->LowerLayerDown(aReason);
			}
		}
   	}


void CPppLcp::PhaseComplete()
/**
Advance to the next PPP phase after successfully completing the current phase.
*/
	{
	switch (iPhase)
		{
	case EPppPhaseTerminate:
		break;
	case EPppPhaseEstablish:
		PhaseAdvance(EPppPhaseEarlyCallback);
		break;
	case EPppPhaseEarlyCallback:
		PhaseAdvance(EPppPhaseAuthenticate);
		break;
	case EPppPhaseAuthenticate:
		//PhaseAdvancesBeyondAuthenticate();
		PhaseAdvance(EPppPhaseLateCallback);
		break;
	case EPppPhaseLateCallback:
		if (!iCallbackEnabled)
			{
			PhaseAdvance(EPppPhaseNetwork);
			}
		break;
	case EPppPhaseNetwork:
		break;
	default:
		break;
		}
	}

void CPppLcp::PhaseAborted(TInt aStatus)
/**
Return to a previous PPP phase after ending the current phase.

@param aStatus Error code indicating the reason we are ending the phase
*/
	{
	switch (iPhase)
		{
	case EPppPhaseNetwork:
		{
		// NCP failer only translates to total failure
		// if there are no more live NCPs
		TInt nets=0;
		MPppRecvr* rcvr;
		TDblQueIter<MPppRecvr> iter(iRecvrList);
		while (rcvr=iter++, rcvr!=NULL)
			{
			if (rcvr->iActivePhase==EPppPhaseNetwork && rcvr->iPppAbortCode==KErrNone)
				++nets;
			}
		if (nets>0)
			break;
		// else fall through...
		}
	case EPppPhaseTerminate:
	case EPppPhaseEstablish:
	case EPppPhaseEarlyCallback:
	case EPppPhaseAuthenticate:
	case EPppPhaseLateCallback:
		PhaseRetard(EPppPhaseTerminate, aStatus);
		break;
	default:
		break;
		}
	}

//
// PPP Upcalls from FSM
//

EXPORT_C TInt CPppLcp::FsmLayerStarted()
/**
Open the layer below.
Upcall from the FSM.

@return Error code
*/
	{
	TRAPD(err, iPppLink->OpenL());
	if (err==KErrNone)
		PhaseAdvance(EPppPhaseEstablish);
	return err;
	}

EXPORT_C void CPppLcp::FsmLayerFinished(TInt aReason)
/**
Close the layer below.
Upcall from the FSM.

@param aReason Error code indicating the reason we are finished
*/
	{
	LOG( iLogger->Printf(_L("CPppLcp::FsmLayerFinished aReason %d"), aReason); )
	iError = aReason;
	iPppLink->Close();
	//PhaseAborted(aReason<KErrNone ? aReason : KErrTimedOut); PRR 15/4/98
	PhaseAborted(aReason); // PRR 15/4/98

	if (aReason == KErrCommsLineFail)
		{
		LinkLayerDown(aReason);
		}
	}

EXPORT_C void CPppLcp::FsmTerminationPhaseComplete()
/**
Called when the FSM has stopped.
Upcall from the FSM.
*/
	{
	LOG( iLogger->Printf(_L("CPppLcp::FsmTerminationPhaseComplete iError[%d]"), iError); )
 	PostProgressMessage(EPppProgressLinkDown, iError);

  	if (iTerminateAction == MNifIfNotify::EDisconnect)
  		{
  		// Tell SCPR that PPP is finished. After the Agent finishes (terminates the connection, air link, etc), Nifman
   		// closes the NIF, which results in PPP deletion. The call to delete PPP originates in Nifman, implemented as
   		// asynchronous callback. This call would invoke the destructors, which do the actual cleanup in the NIF.
		PostFlowDownMessage(iError);
  		}
  	else if (iTerminateAction == MNifIfNotify::ECallBack)
  		{
  		// Action is not EDisconnect: this may not necessarily trigger NIF deletion - Nifman may try to restart the link.
        iLastRequestOriginator.ReplyTo(Id(), TPPPMessage::TPppLinkExpectingCallback().CRef());

		// After the first (outgoing) stage of callback, iTerminateAction contains ECallback.  If the
		// connection comes up correctly and is subsequently terminated from "below" (i.e. from HDLC
		// calling CPppLcp::LinkLayerDown() rather than from "above" (via CPppLcp::Stop() and TerminateLink()),
		// then iTerminateAction remains set to ECallback.  As a result, at the end of the call we end
		// up incorrectly signalling Nifman with LinkLayerDown(ECallback) which causes Nifman to wrongly
		// perform the ECallback actions rather than the EDisconnect actions (which means issuing a
		// Connect() to the Agent which subsequently goes wrong).
		iTerminateAction = MNifIfNotify::EDisconnect;
		}
  	else
  	    {
  	    // It is believed that EReconnect does not get set into iTerminateStatus
  	    // and therefore iTerminateAction should only ever be the ENoAction value
  	    ASSERT(iTerminateAction == MNifIfNotify::ENoAction);
  	    PostFlowDownMessage(KErrNone);
  	    }

	iCallbackEnabled = EFalse;
	iGotCallbackInfo = EFalse;
	PhaseAdvance(EPppPhaseTerminate);
	PppNcpMsCbcpFactory::Delete( iPppMsCbcp );
	iPppMsCbcp = NULL;
	}

EXPORT_C void CPppLcp::FsmLayerUp()
/**
Signal up event to next layer above
Upcall from the FSM.
*/
	{
	ExtOptNegotiationComplete();
	PhaseComplete();
	}

EXPORT_C void CPppLcp::FsmLayerDown(TInt aReason)
/**
Signal down event to next layer above
Upcall from the FSM.

@param aReason Error code indicating the reason the layer is going down
*/
	{
	ExtOptNegotiationAborted();
	if(aReason!=KErrNone && iPhase==EPppPhaseEstablish)
		iPhase=EPppPhaseTerminate;
	PhaseRetard(EPppPhaseEstablish, aReason);
	}

EXPORT_C void CPppLcp::FsmFillinConfigRequestL(RPppOptionList& aRequestList)
/**
Fill in Config Request to be sent

@param aRequestList Receives LCP option list
*/
	{
	ExtOptNegotiationStarted();
	iMaxSendSize = 0;

// Removed September 1999 , both set to zero in GetSendRecvSize()
//	iMaxRecvSize = 0;
//	iPppLink->GetSendRecvSize(iMaxSendSize, iMaxRecvSize);
//	iMaxRecvSize could be read from ppp.ini file
	// Get desired Link options
	if (iMaxSendSize==0)
		iMaxSendSize = KPppDefaultFrameSize;
	if (iMaxRecvSize==0)
		iMaxRecvSize = KPppDefaultFrameSize;
	else if(iMaxRecvSize != KPppDefaultFrameSize)
		// Only add to request if not the default
		aRequestList.CreateAndAddL(KPppLcpOptMaxRecvUnit, (TUint16)iMaxRecvSize);

	// Create a new magic number
	iConsecMagic = 0;
	iRemMagicNumber = 0;
	if (iLocMagicNumber == 0)
		{
		NewMagicNumberL(iLocMagicNumber);
		}
	aRequestList.CreateAndAddL(KPppLcpOptMagicNumber, (TUint32)iLocMagicNumber);

	AppendCallbackRequestL( aRequestList );

	ExtOptFillinConfigRequestL(aRequestList);
	}

EXPORT_C void CPppLcp::AppendCallbackRequestL( RPppOptionList& aRequestList )
/**
Append an LCP OPT of type "callback-request" to the given list

@param aRequestList LCP option list to fill in
*/
	{
	if ( ! iGotCallbackInfo )
		{
		InitCallbackInfoL();
		iGotCallbackInfo = ETrue;
		}

	if ( ! iCallbackEnabled )
		return;
	__ASSERT_DEBUG( iCallbackIETFRequestPacket, PppPanic(EPppPanic_IETFNoCalback) );
	__ASSERT_DEBUG( iCallbackIETFRequestPacketLen!=0, PppPanic(EPppPanic_IETFCalbackInvalid) );
	aRequestList.CreateAndAddL( KPppLcpOptCallback, iCallbackIETFRequestPacket, iCallbackIETFRequestPacketLen);
	}

EXPORT_C void CPppLcp::FsmCheckConfigRequest(RPppOptionList& aRequestList, RPppOptionList& aAckList, RPppOptionList& aNakList, RPppOptionList& aRejList)
/**
Check options in a received config request.
Each option is added to the appropriate Ack, Nak or Rej list.
Upcall from the FSM.

@param aRequestList LCP options to check
@param aAckList Acked LCP options
@param aNakList Naked LCP options
@param aRejList Rejected LCP options
*/
	{
	const TInt KOptMaxRecvUnitValueLen = 2; // correct value
	const TInt KOptMagicNumberValueLen = 4;
	const TInt KOptMruMinSize = 4;

	RPppOption opt;
	TBool forceNak(EFalse);
	while (aRequestList.Remove(opt))
		{
		forceNak = EFalse; // Should the option be NAKed due to corruption?

		switch (opt.OptType())
			{
		case KPppLcpOptMaxRecvUnit:
			// Check for correct MRU option length and MRU value >= 4
			if(KOptMaxRecvUnitValueLen == opt.ValueLength() && BigEndian::Get16(opt.ValuePtr()) >= KOptMruMinSize ) // valid option: Ack
				{
				aAckList.Append(opt);
				}
			else // invalid: NAK: put desired option, correct length
				{
				opt.SetValueLength(KOptMaxRecvUnitValueLen);
				//N.B. We NAK with our MRU, read from the .ini file.
				BigEndian::Put16(opt.ValuePtr(), static_cast<TUint16>(iMaxRecvSize));
				aNakList.Append(opt);
				}
			break;

		case KPppLcpOptMagicNumber:
			if(KOptMagicNumberValueLen == opt.ValueLength()) // Correct length
				{
				iRemMagicNumber = BigEndian::Get32(opt.ValuePtr());
				// Magic number of zero is not allowed and results in a Nak.
				if (iRemMagicNumber == 0)
					{
					forceNak = ETrue;
					}
				}
			else // Invalid length: correct it and NAK.
				{
				opt.SetValueLength(KOptMagicNumberValueLen);
				forceNak = ETrue;
				}

			// NAK if local and remote magic numbers are equal, or if
			// the option is corrupt.
			if((iRemMagicNumber == iLocMagicNumber) || forceNak)
			  	{
				TRAPD(result, NewMagicNumberL(iRemMagicNumber));

				if(result == KErrNone)
					{
					BigEndian::Put32(opt.ValuePtr(), iRemMagicNumber);
					}
				else
					{
					LOG( iLogger->Printf(_L("CPppLcp::FsmCheckConfigRequest Error when using NewMagicNumberL iError[%d]"), result); )
					}
				aNakList.Append(opt);
				}
			else // Remote magic number option is OK and remote number != local number
				{
				aAckList.Append(opt);
				}
			break;
		case KPppLcpOptMultiLinkEndPointDescriminator:
	        aAckList.Append(opt);
			break;

		default:
    		ExtOptCheckConfigRequest(opt, aAckList, aNakList, aRejList);
			}
		}
	}

EXPORT_C void CPppLcp::FsmApplyConfigRequest(RPppOptionList& aRequestList)
/**
Apply options in a received config request (that was ACK'd).
Upcall from the FSM.

@param aRequestList LCP options to use
*/
	{
	TMBufPktQIter iter(aRequestList);
	RPppOption opt;

	while (opt = iter++, !opt.IsEmpty())
		{
		switch (opt.OptType())
			{
		case KPppLcpOptMaxRecvUnit:
			iMaxSendSize = BigEndian::Get16(opt.ValuePtr());
			break;
		case KPppLcpOptMagicNumber:
			iRemMagicNumber = BigEndian::Get32(opt.ValuePtr());
			break;
		default:
			ExtOptApplyConfigRequest(opt);
			break;
			}
		}
	}

EXPORT_C void CPppLcp::FsmRecvConfigAck(RPppOptionList& aReplyList)
/**
Received a Config Ack - apply the options
Upcall from the FSM.

@param aReplyList LCP options to use
*/
	{
	TMBufPktQIter iter(aReplyList);
	RPppOption opt;

	while (opt = iter++, !opt.IsEmpty())
		{
		switch (opt.OptType())
			{
		case KPppLcpOptMaxRecvUnit:
			iMaxRecvSize = BigEndian::Get16(opt.ValuePtr());
			break;
		case KPppLcpOptMagicNumber:
			// Magic number is OK
			SendIdentification();
			break;
		case KPppLcpOptCallback:
			CallbackRequestGranted();
			break;
		default:
			ExtOptRecvConfigAck(opt);
			break;
			}
		}
	}

EXPORT_C void CPppLcp::FsmRecvConfigNak(RPppOptionList& aReplyList, RPppOptionList& aReqList)
/**
Modify request after receiving a Config Nak
Upcall from the FSM.

@param aReplyList NAK'd LCP options
@param aReqList The associated original request to be modified
*/
	{
	TMBufPktQIter iter(aReplyList);
	RPppOption opt;

	while (opt = iter++, !opt.IsEmpty())
		{
		switch (opt.OptType())
			{
		case KPppLcpOptMaxRecvUnit:
			aReqList.ReplaceOption(opt);
			break;
		case KPppLcpOptMagicNumber:
			{
			iLocMagicNumber = BigEndian::Get32(opt.ValuePtr());
			if (iLocMagicNumber==iRemMagicNumber)
				{
				if (++iConsecMagic>KPppFsmNonConvergeLimit)
					{
					FsmAbort(KErrTimedOut);
					return;
					}
				}
			else
				iConsecMagic = 0;
			aReqList.ReplaceOption(opt);
			}
			break;
		default:
			ExtOptRecvConfigNak(opt, aReqList);
			break;
			}
		}
	}

EXPORT_C void CPppLcp::FsmRecvConfigReject(RPppOptionList& aReplyList, RPppOptionList& aReqList)
/**
Modify request after receiving a Config Reject
Upcall from the FSM.

@param aReplyList NAK'd LCP options
@param aReqList The associated original request to be modified
*/
	{
	TMBufPktQIter iter(aReplyList);
	RPppOption opt;

	while (opt = iter++, !opt.IsEmpty())
		{
		switch (opt.OptType())
			{
		case KPppLcpOptMaxRecvUnit:
		case KPppLcpOptMagicNumber:
			aReqList.RemoveOption(opt);
			break;
		//PG if call back is rejected take the link down
		case KPppLcpOptCallback:
			PhaseAborted(KErrIfCallbackNotAcceptable);
			break;
		default:
			ExtOptRecvConfigReject(opt, aReqList);
			break;
			}
		}
	}

EXPORT_C TBool CPppLcp::FsmRecvUnknownCode(TUint8 aCode, TUint8 aId, TInt aLength, RMBufChain& aPacket)
/**
Process a packet with an otherwise unrecognised LCP opcode.
If processing of extended codes is enabled or the code can otherwise be
handled here, process it.
Upcall from the FSM.

@param aCode Unrecognised opcode
@param aId Packet identifier
@param aLength Length of packet
@param aPacket MBuf chain containing received packet

@return ETrue if the packet was handled
*/
	{
	switch (aCode)
		{
	case KPppLcpIdentification:
		if(DoLcpExts())
			{
			if(aLength<=4)
				return ETrue;
			aPacket.TrimStart(4);
			TUint num = BigEndian::Get32(aPacket.First()->Ptr());
			if (num==iRemMagicNumber)
				{
				delete iRecvIdentification;
				iRecvIdentification = NULL;
				iRecvIdentification = HBufC8::New(aLength-8);
				if(!iRecvIdentification)
					return ETrue;
				TPtr8 des = iRecvIdentification->Des();
				aPacket.CopyOut(des, 4);
				}
			return ETrue;
			}
		else
			return EFalse;

	case KPppLcpTimeRemaining:
		if(DoLcpExts())
			{
			if(aLength<=4)
				return ETrue;
			aPacket.TrimStart(4);
			TUint num = BigEndian::Get32(aPacket.First()->Ptr());
			if (num==iRemMagicNumber)
				{
				aPacket.TrimStart(4);
				iRecvTimeNotification.UniversalTime();
				iRecvTimeRemaining = BigEndian::Get32(aPacket.First()->Ptr());
				delete iRecvTimeRemMessage;
				iRecvTimeRemMessage=NULL;
				if (aLength>12)
					{
					iRecvTimeRemMessage = HBufC8::New(aLength-12);
					if(!iRecvTimeRemMessage)
						return ETrue;
					TPtr8 des = iRecvTimeRemMessage->Des();
					aPacket.CopyOut(des, 4);
					}
				}
			return ETrue;
			}
		else
			return EFalse;

	case KPppLcpEchoRequest:
		{
		if(aLength < 8)  //Packet's length is less than the Min 8 bytes for a echo request.
			return ETrue;
		RMBufPacket pkt;
		RMBufPktInfo* info=NULL;
		TRAPD(ret, info = pkt.NewInfoL());
		if (ret!=KErrNone)
			return ETrue;
		// Reuse the existing packet buffer for our reply
		pkt.Assign(aPacket);
		TUint8* ptr = pkt.First()->Ptr();
		//check if the magic number matches the negotiated value.
		TUint magicNumber= BigEndian::Get32(ptr+4);
		if(magicNumber != iRemMagicNumber)
			{
			LOG( iPppLcp->iLogger->Printf(_L("PPP detected a wrong magic number in echo request.")); )
			return ETrue; //Just discard this echo request.
			}
		info->iLength = BigEndian::Get16(ptr+2);
		TPppAddr::Cast(info->iDstAddr).SetProtocol(iPppId);
		BigEndian::Put32(ptr+4, iLocMagicNumber);
		*ptr = KPppLcpEchoReply;
		// If the frame to be sent is too large, lop off the end to make it fit.
 		__ASSERT_DEBUG(iPppLcp->MaxTransferSize() > 0, PppPanic(EPppPanic_InvalidData));
 		if (info->iLength > iPppLcp->MaxTransferSize())
 			{
 			pkt.TrimEnd(iPppLcp->MaxTransferSize());
 			info->iLength = iPppLcp->MaxTransferSize();
  			}
		pkt.Pack();
		SendFrame(pkt);
		}
		return ETrue;
	case KPppLcpEchoReply:
		{
		// Call CPppLrd::RecvEchoReply
		iPppLrd->RecvEchoReply(aId);
		return ETrue;
		}
	case KPppLcpDiscardRequest:
		return ETrue;
	default:
		break;
		}
	return EFalse;
	}

//
// Other methods
//

void CPppLcp::NewMagicNumberL(TUint& aMagicNumber)
/**
Generate a new magic number, based upon existing number,
machine unique ID and tick count.

@param aMagicNumber Magic number to update
*/
	{

	TPckg<TUint> randDes(aMagicNumber);
	do
		{
		TRandom::RandomL(randDes);
		}
		while (aMagicNumber==0);
	}

void CPppLcp::CancelRead()
/**
Unused
*/
	{
	}

TUint8 CPppLcp::SendEchoRequest()
/**
Send an LCP Echo Request frame.
Called by CPppLrd::TimerComplete periodically

@return On success, the Identifier that was selected
so CPppLrd can match the answer to the request.
On failure, 0 (safe because the identifier can't be 0)
*/
	{
	TUint8	returnValue = 0;
	RMBufPacket pkt;
	const TUint pktLen = 4+4;
	TUint8* ptr = NewPacket(pkt, pktLen);
	if (ptr == NULL)
		{
		return returnValue;
		}
	*ptr++ = (TUint8)KPppLcpEchoRequest;
	*ptr++ = (returnValue = FsmNewId());
	BigEndian::Put16(ptr, (TUint16)pktLen);
	BigEndian::Put32(ptr+2, (TUint32)iLocMagicNumber);
	pkt.Pack();
	SendFrame(pkt);
	return returnValue;
	}

void CPppLcp::SendIdentification()
/**
Send an LCP Identification frame (RFC 1570).
*/
	{
	if(!DoLcpExts())
		return;
	TPtrC8 id = _L8("Symbian Epoc");
	RMBufPacket pkt;
	const TUint pktLen = 4+4+id.Length();
	TUint8* ptr = NewPacket(pkt, pktLen);
	if (ptr == NULL)
		{
		return;
		}
	*ptr++ = (TUint8)KPppLcpIdentification;
	*ptr++ = FsmNewId();
	BigEndian::Put16(ptr, (TUint16)pktLen);
	BigEndian::Put32(ptr+2, (TUint32)iLocMagicNumber);

	pkt.CopyIn(id, 4+4);
	pkt.Pack();
	SendFrame(pkt);
	}

void CPppLcp::StopProtocol(TUint aProtocol)
/**
One of the NCP protocols has been shut down, so kill it.
Searches for the registered protocol handler based on the protocol ID.

@param aProtocol PPP protocol number
*/
	{
	MPppRecvr* rcvr;


	TDblQueIter<MPppRecvr> iter(iRecvrList);
	while (rcvr = iter++, rcvr!=NULL)
		{
		if (aProtocol == rcvr->iPppId)
			{
			/*
			*	Kill Me
			*/
			rcvr->KillProtocol();

			}
		}
	}

void CPppLcp::PppNewCompressor(const CPppCompressor* aCompressor)
/**
Indicate that the compression protocol handler is ready.
Called from CPppCcp.

@param aCompressor Compressor object, or NULL to indicate object is
no longer available
*/
	{
	if (iPppLink != NULL)
		{
		iPppLink->NewCompressor(aCompressor);
		}
	}

void CPppLcp::PppUnloadCompressor()
/**
Unload the compression module after a compressed packet is rejected.
Upcall from the FSM.
*/
	{
	if (iPppCcp != NULL)
		{
		iPppCcp->UnloadCompressor();
		}
	}

void CPppLcp::PppNewDeCompressor(const CPppDeCompressor* aDeCompressor)
/**
Indicate that the decompression protocol handler is ready.
Called from CPppCcp.

@param aDeCompressor Decompressor object, or NULL to indicate object is no longer available
*/
	{
	if (iPppLink != NULL)
		{
		iPppLink->NewDeCompressor(aDeCompressor);
		}
	}

TInt CPppLcp::Notification(TAgentToNifEventType aEvent, void * aInfo)
/**
Notification from Agent

@param aEvent Event type
@param aInfo Data relating to event

@return Error code
*/
	{
	switch (aEvent)
		{
	case EAgentToNifEventTypeModifyInitialTimer:

		return KErrNone;
	case EAgentToNifEventTypeDisableTimers:
		iPppLrd->EnableOrDisableTimer(EFalse);
		return KErrNone;
	case EAgentToNifEventTypeEnableTimers:
		iPppLrd->EnableOrDisableTimer(ETrue);
		return KErrNone;
	case EAgentToNifEventTypeGetDataTransfer:
		{
		//===========================================
		// JGG PPP CHANGE
		TPckg<RPacketContext::TDataVolume>* dataPackage = (TPckg<RPacketContext::TDataVolume>*) aInfo;
		RPacketContext::TDataVolume& data = (*dataPackage)();
		//TPckg<RGprsContext::TDataVolume>* dataPackage = (TPckg<RGprsContext::TDataVolume>*) aInfo;
		//RGprsContext::TDataVolume& data = (*dataPackage)();
		//===========================================
		iPppLink->GetDataTransfer(data);
		return KErrNone;
		}
	default:
		break;
		}
		return KErrNotSupported;
	}

EXPORT_C TBool CPppLcp::FsmAckOptionsValid(RPppOptionList& aList, RPppOptionList& aRequestList)
/**
Check if the option list specified matches the original Config-Request option list.

Used for conformance to RFC1661 sections 5.2 and 5.4 (the option list in a Config-Ack or
Config-Reject must exactly match the option list in the original Config-Request - same
option values in the same order with no new options appended).

@param aList option list to check against (e.g. from a Config-Ack or Config-Reject etc).
*/
	{
	return aList.EqualTo(aRequestList);
	}

EXPORT_C TBool CPppLcp::FsmRejectOptionsValid(RPppOptionList& aList, RPppOptionList& aRequestList)
/**
Check if the option list specified matches the original Config-Request option list.

Used for conformance to RFC1661 sections 5.2 and 5.4 (the option list in a Config-Ack or
Config-Reject must exactly match the option list in the original Config-Request - same
option values in the same order with no new options appended).

@param aList option list to check against (e.g. from a Config-Ack or Config-Reject etc).
*/
	{
	return aList.IsSubsetOf(aRequestList);
	}

EXPORT_C TBool CPppLcp::FsmConfigRequestOptionsValid(RPppOptionList& aList)
/**
Check if any RFC1661 options are duplicated in the specified option list.

Used for conformance to RFC1661 section 6:

	"... (None of the Configuration Options in this specification can be listed more than once.)..."

Note that this only checks for options in RFC1661 specification and not *all* PPP options
(hence the "RFC1661" prefix to the method name).

@param aList option list to check .
*/
	{
	const TInt KMinOption = KPppLcpOptMaxRecvUnit;
	const TInt KMaxOption = KPppLcpOptAddrCtrlCompress;

	ASSERT(KMinOption >= 0 && KMaxOption >= 0 && KMinOption <= KMaxOption);	// sanity

	TUint8 optionCount[KMaxOption + 1] = { 0 };

	TMBufPktQIter iter(aList);
	RPppOption opt;

	opt = iter++;
	while (!opt.IsEmpty())
		{
		TUint8 optType = opt.OptType();
		if (optType >= KMinOption && optType <= KMaxOption)
			{
			if (++optionCount[optType] == 2)		// duplicated option...
				{
				switch (optType)					// ...is it one of the RFC1661 options ?
					{
				case KPppLcpOptMaxRecvUnit:
				case KPppLcpOptAuthenticationProtocol:
				case KPppLcpOptQualityProtocol:
				case KPppLcpOptMagicNumber:
				case KPppLcpOptProtocolCompress:
				case KPppLcpOptAddrCtrlCompress:
					return EFalse;
				default:
					break;
					}
				}
			}
		opt = iter++;
		}
	return ETrue;
	}

void CPppLcp::NcpUp()
	{
	++iNcpUpCount;
	if (AllNcpsUp())
		{
		FlowUp();
		}
	}

TBool CPppLcp::AllNcpsUp()
	{
	return (iNcpUpCount == iOpenCount);
	}

#ifdef __FLOG_ACTIVE
const TText8* CPppLcp::GetMMStateName() const
	{
		switch(iMMState)
			{
			case EStopped:
				return _S8("Stopped");
	      	case EStarting:
				return _S8("Starting");
	      	case EStarted:
				return _S8("Started");
	      	case EStopping:
				return _S8("Stopping");
	      	default:
				return _S8("<Unknown State>");
			}
	}
#endif
