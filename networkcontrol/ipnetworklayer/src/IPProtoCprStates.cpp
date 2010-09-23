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
// IPProto Connection Provider implementation
// 
//

/**
 @file
 @internalComponent
*/

#include <comms-infras/ss_nodemessages.h>
#include <comms-infras/corecpractivities.h>

#include <comms-infras/ss_metaconnprov.h>
#include <comms-infras/sockmes.h> // for ioctl ipc

#include "IPProtoCprStates.h"
#include "IPProtoMessages.h"
#include "linkcprextensionapi.h"
#include <comms-infras/ss_datamonitoringprovider.h>
#include <comms-infras/ss_connlegacy.h>
#include <nifvar.h> // KLinkLayerOpen
#include <comms-infras/ss_nodemessages_internal.h>
#include <comms-infras/ss_legacyinterfaces.h>
#include <comms-infras/simpleselectorbase.h>
#include <commsdattypesv1_1_partner.h>
#include <es_prot_internal.h>
#include <elements/nm_messages_errorrecovery.h>

using namespace Messages;
using namespace MeshMachine;
using namespace IpProtoCpr;
using namespace ESock;

DEFINE_SMELEMENT(TProvisionActivation, NetStateMachine::MStateTransition, IpProtoCpr::TContext)
void TProvisionActivation::DoL()
    {
    CIPProtoConnectionProvider& node = iContext.Node();
    
    //Trap if memory allocation fails
    TRAP( node.iProvisionError, node.iSubConnProvisioningInfo = new (ELeave) TDataMonitoringSubConnProvisioningInfo(NULL, NULL));
    }

DEFINE_SMELEMENT(THandleProvisionError, NetStateMachine::MStateTransition, IpProtoCpr::TContext)
void THandleProvisionError::DoL()
    {
    //Set node error
    iContext.iNodeActivity->SetError(iContext.Node().iProvisionError);
    }

DEFINE_SMELEMENT(TStoreProvision, NetStateMachine::MStateTransition, IpProtoCpr::TContext)
void TStoreProvision::DoL()
	{
	PRStates::TStoreProvision storeProvision(iContext);
	storeProvision.DoL();

	CIPProtoConnectionProvider& node = iContext.Node();

	// Retrieve the idle timer values from the provisioning information bucket
	const Meta::SMetaData* extension =
		node.AccessPointConfig().FindExtension(STypeId::CreateSTypeId(TIdleTimerValues::iUid, TIdleTimerValues::iId));

    if (extension)
        {
        const TIdleTimerValues* vals = static_cast<const TIdleTimerValues*>(extension);
        node.SetTimers(vals->iShortTimer, vals->iMediumTimer, vals->iLongTimer);
        }

    if (!node.iNodeLocalExtensionsCreated)
    	{
    	node.iNodeLocalExtensions.Open();

    	TPacketActivity* packetActivity = new(ELeave)TPacketActivity(&iContext.Node().iPeriodActivity);
        CleanupStack::PushL(packetActivity);
        node.iNodeLocalExtensions.AppendExtensionL(packetActivity);
        CleanupStack::Pop(packetActivity);
        
        // Allocate and add the data monitoring shared memory pointers to the provisioning info
    	TDataMonitoringConnProvisioningInfo* connProvisioningInfo = new (ELeave) TDataMonitoringConnProvisioningInfo(&node.iDataVolumes, &node.iThresholds);
    	CleanupStack::PushL(connProvisioningInfo);
    	node.iNodeLocalExtensions.AppendExtensionL(connProvisioningInfo);
    	CleanupStack::Pop(connProvisioningInfo);

    	// Allocate the data monitoring subconnection data structure.
    	//
    	// This used to be allocated in the SCPR, but it has been moved here to the CPR.  This is because there can be
    	// temporary circumstances where there is more than one SCPR present - one or more of these in a leaving
    	// state and one in an active state.  The access point config cannot presently hold provisioning information
    	// specific to an SCPR instance, so it is allocated and managed here in the CPR as a shared entity.  Only
    	// one SCPR instance should be using this at a time, however.
    	
    	//this allocation taken care in previous activity entry
    	//just append here
    	//TDataMonitoringSubConnProvisioningInfo* subConnProvisioningInfo = new (ELeave) TDataMonitoringSubConnProvisioningInfo(NULL, NULL);
    	CleanupStack::PushL(node.iSubConnProvisioningInfo);
    	node.iNodeLocalExtensions.AppendExtensionL(node.iSubConnProvisioningInfo);
    	CleanupStack::Pop(node.iSubConnProvisioningInfo);
    	
       	// The CLinkCprExtensionApi may have been added previously if this is a reconnect scenario
    	// We add it again in this new override of the extensions, if the old container is fully superceded
    	// it will be closed and destroyed.
       	extension = CLinkCprExtensionApi::NewLC(iContext.Node());
       	node.iNodeLocalExtensions.AppendExtensionL(extension);
    	CleanupStack::Pop(); // CLinkCprExtensionApi
    	
    	node.iNodeLocalExtensionsCreated = ETrue;
    	}
    
    RMetaExtensionContainer mec;
    mec.Open(iContext.Node().AccessPointConfig());
    CleanupClosePushL(mec);
    mec.AppendContainerL(node.iNodeLocalExtensions);
    
	iContext.Node().iAccessPointConfig.Close();
    iContext.Node().iAccessPointConfig.Open(mec);
	CleanupStack::PopAndDestroy(&mec);
	
	// For the case that this is an update of the provisioned info we forward it to non-leaving data clients
	TClientIter<TDefaultClientMatchPolicy> iter = iContext.Node().GetClientIter<TDefaultClientMatchPolicy>(
		TClientType(TCFClientType::EData),
		TClientType(0, TClientType::ELeaving)
		);

	for (TInt i = 0; iter[i]; i++)
		{
		iter[i]->PostMessage(
			iContext.NodeId(),
			TCFDataClient::TProvisionConfig(iContext.Node().iAccessPointConfig).CRef()
			);
		}
	}

DEFINE_SMELEMENT(TSendStoppedAndGoneDown, NetStateMachine::MStateTransition, IpProtoCpr::TContext)
void TSendStoppedAndGoneDown::DoL()
	{
	ASSERT(iContext.iNodeActivity);

	// stop has been caused by timer expiry, remove self from originators list, because we
	// are not waiting for TStopped and in certain situations it would arrive after the node has been
	// destroyed
	if (iContext.Node().iTimerStopped)
		{
		TInt selfidx = iContext.iNodeActivity->FindOriginator(iContext.Node().SelfInterface());
		ASSERT(selfidx != KErrNotFound);
		iContext.iNodeActivity->RemoveOriginator(selfidx);
		}
		
	TInt stopCode = KErrCancel;
    MeshMachine::CNodeActivityBase* activity = iContext.iNodeActivity;
    
    if (activity && activity->Error() != KErrNone)
        {
        stopCode = activity->Error();
        activity->SetError(KErrNone);
        }

	// Could be TStop, TStopped or TDataClientStopped (usually TStopped)
	if  ( (iContext.iMessage.IsMessage<TCFServiceProvider::TStopped>()) ||
		(iContext.iMessage.IsMessage<TCFServiceProvider::TStop>()) ||
		(iContext.iMessage.IsMessage<TCFDataClient::TStopped>()) ||
		(iContext.iMessage.IsMessage<TCFDataClient::TStop>()) )
		{
		stopCode = static_cast<const Messages::TSigNumber&>(iContext.iMessage).iValue;
		}
	else if ( (iContext.iMessage.IsMessage<TCFControlClient::TGoneDown>()) ||
		(iContext.iMessage.IsMessage<TCFControlProvider::TDataClientGoneDown>()) )
		{
		stopCode = static_cast<const Messages::TSigNumberNumber&>(iContext.iMessage).iValue1;
		}
	else if ( iContext.iMessage.IsMessage<TEErrorRecovery::TErrorRecoveryResponse>() )
		{
		// Action must be propagate or there is no error code (your activity flow is faulty)!
		const Messages::TSigErrResponse& sig = static_cast<const Messages::TSigErrResponse&>(iContext.iMessage);
		__ASSERT_DEBUG(sig.iErrResponse.iAction == Messages::TErrResponse::EPropagate, User::Invariant());
   		stopCode = sig.iErrResponse.iError;
		}

	TCFServiceProvider::TStopped msg(stopCode);
	iContext.iNodeActivity->PostToOriginators(msg);

    const TProviderInfo& providerInfo = static_cast<const TProviderInfoExt&>(iContext.Node().AccessPointConfig().FindExtensionL(
            STypeId::CreateSTypeId(TProviderInfoExt::EUid, TProviderInfoExt::ETypeId))).iProviderInfo;

	TCFControlClient::TGoneDown goneDown(stopCode, providerInfo.APId());
	TClientIter<TDefaultClientMatchPolicy> iter = iContext.Node().GetClientIter<TDefaultClientMatchPolicy>(TClientType(TCFClientType::ECtrl));
    CNodeActivityBase* startActivity = iContext.Node().FindActivityById(ECFActivityStart);

    for (TInt i = 0; iter[i]; i++)
		{
		//Send TGoneDown to every Ctrl client except
		// * the originator (who would be recieving TStopped)
		// * originators of the start activity (these will be errored separately)
        if (iContext.iNodeActivity && iContext.iNodeActivity->FindOriginator(*iter[i]) >= 0)
            {
            continue; // ControlClient is a Stop originator
            }
		
        // So far the control client is not a Stop originator
        if (startActivity == NULL || startActivity->FindOriginator(*iter[i]) == KErrNotFound)
			{
            // ControlClient is not a Start originator
			iter[i]->PostMessage(TNodeCtxId(iContext.ActivityId(), iContext.NodeId()), goneDown);
			}
		}

	if (iContext.iNodeActivity)
    	{
        iContext.iNodeActivity->SetError(KErrNone);
    	}
	}


DEFINE_SMELEMENT(TAwaitingStop, NetStateMachine::MState, IpProtoCpr::TContext)
TBool TAwaitingStop::Accept()
    {
	/*
	Please note, in order for this to work, ESock needs to export the Accept function
	of the CoreNetStates::TAwaitingStop.
	*/
	CoreNetStates::TAwaitingStop state(iContext);
    if (state.Accept())
        {
		if (iContext.Node().CountActivities(ECFActivityDestroy) == 0)
            {
            return ETrue;
            }
		else
            {
            //The messageId is cleared so the CoreNetStates::TAwaitingStop is not picking up this TStop message after this Accept has rejected it.
            //If this activity is kicked off while there is a Destroy activity, most probably the phone will crash, as there will be a SelfStop
            //which will arrive after the node has destructed.
            iContext.iMessage.ClearMessageId();
            }
        }
    return EFalse;
    }


DEFINE_SMELEMENT(TAwaitingDataMonitoringNotification, NetStateMachine::MState, IpProtoCpr::TContext)
TBool TAwaitingDataMonitoringNotification::Accept()
	{
	return iContext.iMessage.IsMessage<TCFDataMonitoringNotification::TDataMonitoringNotification>();
	}


DEFINE_SMELEMENT( TAwaitingGoneDown , NetStateMachine::MState, IpProtoCpr::TContext)
TBool TAwaitingGoneDown::Accept()
	{
	if  (iContext.iMessage.IsMessage<TCFControlClient::TGoneDown>())
		{
		iContext.Node().LinkDown();
		}
	// return EFalse to allow further PRActivities processing
	return EFalse;
	}


DEFINE_SMELEMENT(TProcessDataMonitoringNotification, NetStateMachine::MStateTransition, IpProtoCpr::TContext)
void TProcessDataMonitoringNotification::DoL()
	{
	TCFDataMonitoringNotification::TDataMonitoringNotification& msg =
		message_cast<TCFDataMonitoringNotification::TDataMonitoringNotification>(iContext.iMessage);

	iContext.Node().DataNotificationL(msg);
	}

DEFINE_SMELEMENT(TAwaitingStart, NetStateMachine::MState, IpProtoCpr::TContext)
TBool TAwaitingStart::Accept()
	{
	CoreNetStates::TAwaitingStart state(iContext);
	if (state.Accept())
		{
		iContext.Node().DisableTimers();
		return ETrue;
		}
	return EFalse;
	}

DEFINE_SMELEMENT(TCleanupStart, NetStateMachine::MStateTransition, IpProtoCpr::TContext)
void TCleanupStart::DoL()
 	{
 	//Re-enable idle timers disabled by IpProtoCpr::TAwaitingStart
	iContext.Node().EnableTimers();
	}

DEFINE_SMELEMENT(TCheckIfLastControlClientLeaving, NetStateMachine::MStateTransition, IpProtoCpr::TContext)
void TCheckIfLastControlClientLeaving::DoL()
	{
	CIPProtoConnectionProvider& node = iContext.Node();
	node.ForceCheckShortTimerMode();
	}

DEFINE_SMELEMENT(TAwaitingOpenCloseRoute, NetStateMachine::MState, IpProtoCpr::TContext)
TBool TAwaitingOpenCloseRoute::Accept()
	{
	return (iContext.iMessage.IsMessage<TCFIPProtoMessage::TOpenCloseRoute>());
	}

DEFINE_SMELEMENT(TDoOpenCloseRoute, NetStateMachine::MStateTransition, IpProtoCpr::TContext)
void TDoOpenCloseRoute::DoL()
	{
	TCFIPProtoMessage::TOpenCloseRoute& msg = message_cast<TCFIPProtoMessage::TOpenCloseRoute>(iContext.iMessage);
	if (msg.iValue)
		iContext.Node().OpenRoute();
	else
		iContext.Node().CloseRoute();
	}





DEFINE_SMELEMENT(TLinkUp, NetStateMachine::MStateTransition, IpProtoCpr::TContext)
void TLinkUp::DoL()
	{
	iContext.Node().LinkUp();
	}

DEFINE_SMELEMENT(TLinkDown, NetStateMachine::MStateTransition, IpProtoCpr::TContext)
void TLinkDown::DoL()
	{
	iContext.Node().LinkDown();
	}

DEFINE_SMELEMENT(TStoreAndFilterDeprecatedAndForwardStateChange, NetStateMachine::MStateTransition, IpProtoCpr::TContext)
void TStoreAndFilterDeprecatedAndForwardStateChange::DoL()
	{
	TCFMessage::TStateChange& msg = message_cast<TCFMessage::TStateChange>(iContext.iMessage);
	//Forward to control clients if there are any

    ESOCK_EXTLOG_VAR( (KESockConnectionTag, KIPProtoCprSubTag, _L8("CIPProtoConnectionProvider %x TStateChange\tProgress: %d, Error: %d"), &iContext.Node(), msg.iStateChange.iStage, msg.iStateChange.iError) );

	//We are the only node accessing this CLinkCprExtensionApi interface so we can do it safely here.
	//This is rather an exceptional situation, please keep the const_cast.
	const CLinkCprExtensionApi* linkCPR = static_cast<const CLinkCprExtensionApi*>(iContext.Node().AccessPointConfig().FindExtension(CLinkCprExtensionApi::TypeId()));
	const_cast<CLinkCprExtensionApi*>(linkCPR)->SetLastProgress(msg.iStateChange);

	if ((KLinkLayerOpen != msg.iStateChange.iStage) && (KLinkLayerClosed != msg.iStateChange.iStage) )
		{
		//KLinkLayerOpen is now redundant with TCFControlClient::TGoneUp & TCFServiceProvider::TStarted 
		//KLinkLayerClosed is now redundant with TCFControlClient::TGoneDown & TCFServiceProvider::TStopped
		//They have therefore been deprecated. Stack nodes are requested not to send the deprecated
		//signals anymore. If stack nodes do send the deprecated signals, the signals will be eaten here 
		//because they confuse convergence and mobility layers, which may live just above. 
		TInt ctrlClientCount = iContext.Node().PostToClients<TDefaultClientMatchPolicy>(TNodeCtxId(iContext.ActivityId(), iContext.NodeId()), iContext.iMessage, TClientType(TCFClientType::ECtrl));
		if (0==ctrlClientCount)
			{ //If there are no control clients any more, forward to the control provider
			iContext.Node().PostToClients<TDefaultClientMatchPolicy>(TNodeCtxId(iContext.ActivityId(), iContext.NodeId()), iContext.iMessage, TClientType(TCFClientType::ECtrlProvider));
			}
		}
	}

DEFINE_SMELEMENT(TSendStopToSelf, NetStateMachine::MStateTransition, TContext)
void TSendStopToSelf::DoL()
 	{
	iContext.iNodeActivity->PostRequestTo(iContext.NodeId(),
		TCFServiceProvider::TStop(iContext.iNodeActivity->Error()).CRef());
	}


DEFINE_SMELEMENT(TSendStarted, NetStateMachine::MStateTransition, TContext)
void TSendStarted::DoL()
 	{
 	//Set the idle timers
	iContext.Node().EnableTimers();
	iContext.Node().SetUsageProfile(KConnProfileMedium);
	iContext.Node().SetTimerMode(CIPProtoConnectionProvider::ETimerMedium);

    iContext.Node().iTimerStopped = EFalse;
	CoreNetStates::TSendStarted transition(iContext);
	transition.DoL();
	}

DEFINE_SMELEMENT(IpProtoCpr::TProcessDataClientStatusChange, NetStateMachine::MStateTransition, IpProtoCpr::TContext)
void IpProtoCpr::TProcessDataClientStatusChange::DoL()
    {
    TCFControlProvider::TDataClientStatusChange& msg = message_cast<TCFControlProvider::TDataClientStatusChange>(iContext.iMessage);

   	if(msg.iValue == TCFControlProvider::TDataClientStatusChange::EStarted)
		{
		iContext.iPeer->SetFlags(TCFClientType::EStarted);
		}
	else
		{
		iContext.iPeer->ClearFlags(TCFClientType::EStarted);
		}

    if(!msg.iValue && !iContext.Node().iSubConnEventDataSent)
    	{ // We're only interested in the provider being stopped.
    	ADataMonitoringProtocolReq* dmInterface = NULL;
		iContext.Node().ReturnInterfacePtrL(dmInterface);

		ADataMonitoringProvider* dmProvider = static_cast<ADataMonitoringProvider*>(dmInterface);

    	ASSERT(dmProvider); // Must always support this interface

    	TCFMessage::TSubConnDataTransferred wholeConnMsg(KNifEMCompatibilityLayerEntireSubConnectionUid, dmProvider->DataVolumesPtr()->iSentBytes, dmProvider->DataVolumesPtr()->iReceivedBytes);
    	TCFMessage::TSubConnDataTransferred defaultSubConnMsg(KNifEMCompatibilityLayerFakeSubConnectionId, dmProvider->DataVolumesPtr()->iSentBytes, dmProvider->DataVolumesPtr()->iReceivedBytes);

      // Sending data clent status change message to all the control clients
      TClientIter<TDefaultClientMatchPolicy> ccIter = iContext.Node().GetClientIter<TDefaultClientMatchPolicy>(TClientType(TCFClientType::ECtrl), TClientType(0, TCFClientType::ELeaving));        
	RNodeInterface* ctrlClient; 
	TBool ctrlClientPresent = false;
	while ((ctrlClient = ccIter++) != NULL)            
	    {
          //If any cntl clinet is present setting the variable ctrlClientPresent as true.
	    ctrlClientPresent = true;
	    ctrlClient->PostMessage(iContext.NodeId(), wholeConnMsg);    
	    ctrlClient->PostMessage(iContext.NodeId(), defaultSubConnMsg); 
	    }
	if(ctrlClientPresent)
	    {
	    iContext.Node().iSubConnEventDataSent = ETrue;
	    }
    	}
    }

DEFINE_SMELEMENT(TAwaitingIoctlMessage, NetStateMachine::MState, IpProtoCpr::TContext)
TBool TAwaitingIoctlMessage::Accept()
	{
	if (iContext.iMessage.IsTypeOf(Meta::STypeId::CreateSTypeId(TCFSigLegacyRMessage2Ext::EUid, TCFSigLegacyRMessage2Ext::ETypeId)))
		{
		TCFSigLegacyRMessage2Ext& msg = static_cast<TCFSigLegacyRMessage2Ext&>(iContext.iMessage);
		if (msg.iMessage.Function() == ECNIoctl)
			{
			return ETrue;
			}
		}
	return EFalse;
	}

DEFINE_SMELEMENT(TForwardToDefaultDataClient, NetStateMachine::MStateTransition, IpProtoCpr::TContext)
void TForwardToDefaultDataClient::DoL()
	{
	ASSERT(iContext.iMessage.IsTypeOf(Meta::STypeId::CreateSTypeId(TCFSigLegacyRMessage2Ext::EUid, TCFSigLegacyRMessage2Ext::ETypeId)));

	RNodeInterface* dc = iContext.Node().GetFirstClient<TDefaultClientMatchPolicy>(TClientType(TCFClientType::EData));	
	iContext.Activity()->PostRequestTo(*dc, iContext.iMessage);
	}
