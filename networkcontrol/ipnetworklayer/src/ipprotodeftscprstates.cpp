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
// IPProto SubConnection Provider states/transitions
// 
//

/**
 @file
 @internalComponent
*/

#include <comms-infras/netcfgextprov.h>
#include <etelqos.h>
#include <networking/qos3gpp_subconparams.h>
#include <cs_subconparams.h>
#include <cs_subconevents.h>
#include <comms-infras/corescpractivities.h>
#include <comms-infras/linkmessages.h>		// for TLinkMessage
#include "ipprotodeftscpr.h"
#include "ipprotodeftscprstates.h"
#include "ItfInfoConfigExt.h"
#include <comms-infras/es_config.h>
#include <es_panic.h>
#include <comms-infras/ss_datamonitoringprovider.h>
#include <comms-infras/ss_log.h>

using namespace Messages;
using namespace MeshMachine;
using namespace ESock;
using namespace NetStateMachine;
using namespace IPProtoDeftSCpr;


DEFINE_SMELEMENT(TStoreProvision, NetStateMachine::MStateTransition, IPProtoDeftSCpr::TContext)
void IPProtoDeftSCpr::TStoreProvision::DoL()
	{
	PRStates::TStoreProvision storeProvision(iContext);
	storeProvision.DoL();			// set up iContext.Node().iProvisionConfig

    // Allocate and add the data monitoring shared memory pointers the provisioning info
	CIPProtoDeftSubConnectionProvider& node = iContext.Node();
	TDataMonitoringSubConnProvisioningInfo* subConnProvisioningInfo = const_cast<TDataMonitoringSubConnProvisioningInfo*>(static_cast<const TDataMonitoringSubConnProvisioningInfo*>(node.AccessPointConfig().FindExtension(STypeId::CreateSTypeId(TDataMonitoringSubConnProvisioningInfo::iUid, TDataMonitoringSubConnProvisioningInfo::iId))));
	ASSERT(subConnProvisioningInfo);

#ifdef _DEBUG
	_LIT8(KNote, "Note");
	if (subConnProvisioningInfo->iDataVolumesPtr || subConnProvisioningInfo->iThresholdsPtr)
		{
		__CFLOG_VAR((KIPProtoDeftScprTag, KNote, _L8("IPProtoDeftSCpr %x:\tNOTE: data monitoring subconnection structure re-assigned"), &node));
		}
#endif
	
	// In the situation where there is an existing IPProtoSCPr in the ELeaving state (which has been
	// stopped and has had a TDestroy sent to it) we can safely re-assign the pointers in the (shared)
	// data monitoring subconnection structure from pointing to that IPProtoSCPr instance to this one.
	// This is because the IP Shim Flow corresponding to that IPProtoSCPr should not be performing any
	// data transfer, after having unbound from its lower flow and closed its binders in response to a
	// TDataClientStop.  The structure itself is allocated in the CPR.

	::new (subConnProvisioningInfo) TDataMonitoringSubConnProvisioningInfo(&node.iDataVolumes, &node.iThresholds);
	}

//-=========================================================
// Data monitoring support
//-=========================================================
DEFINE_SMELEMENT(TAwaitingDataMonitoringNotification, NetStateMachine::MState, IPProtoDeftSCpr::TContext)
TBool IPProtoDeftSCpr::TAwaitingDataMonitoringNotification::Accept()
	{
	return iContext.iMessage.IsMessage<TCFDataMonitoringNotification::TDataMonitoringNotification>();
	}

DEFINE_SMELEMENT(TProcessDataMonitoringNotification, NetStateMachine::MStateTransition, IPProtoDeftSCpr::TContext)
void IPProtoDeftSCpr::TProcessDataMonitoringNotification::DoL()
	{
	TCFDataMonitoringNotification::TDataMonitoringNotification& msg =
		message_cast<TCFDataMonitoringNotification::TDataMonitoringNotification>(iContext.iMessage);

	iContext.Node().DataNotificationL(msg);
	}

// NetCfgExtension Support
//-=========================================================
DEFINE_SMELEMENT(TAwaitingStateChangeOrCancel, NetStateMachine::MState, IPProtoDeftSCpr::TContext)
TBool IPProtoDeftSCpr::TAwaitingStateChangeOrCancel::Accept()
	{
	return (iContext.iMessage.IsMessage<TCFMessage::TStateChange>() || iContext.iMessage.IsMessage<TEBase::TCancel>());
	}

DEFINE_SMELEMENT(TAwaitingStateChange, NetStateMachine::MState, IPProtoDeftSCpr::TContext)
TBool IPProtoDeftSCpr::TAwaitingStateChange::Accept()
    {
    return (iContext.iMessage.IsMessage<TCFMessage::TStateChange>());
    }

DEFINE_SMELEMENT(TAwaitingConfigureNetwork, NetStateMachine::MState, IPProtoDeftSCpr::TContext)
TBool IPProtoDeftSCpr::TAwaitingConfigureNetwork::Accept()
	{
	return iContext.iMessage.IsMessage<TCFIPProtoMessage::TConfigureNetwork>();
	}

DEFINE_SMELEMENT(TAwaitingNetworkConfiguredOrError, NetStateMachine::MState, IPProtoDeftSCpr::TContext)
TBool IPProtoDeftSCpr::TAwaitingNetworkConfiguredOrError::Accept()
	{
	return (iContext.iMessage.IsMessage<TCFIPProtoMessage::TNetworkConfigured>()
			||  iContext.iMessage.IsMessage<TEBase::TError>());
	}

DEFINE_SMELEMENT(TNoTagOrSwallowMessage, NetStateMachine::MStateFork, IPProtoDeftSCpr::TContext)
TInt IPProtoDeftSCpr::TNoTagOrSwallowMessage::TransitionTag()
	{
	const RMetaExtensionContainerC& mec = iContext.Node().AccessPointConfig();
	const CNetCfgExtProvision* provision = static_cast<const CNetCfgExtProvision*>(mec.FindExtension(
	        STypeId::CreateSTypeId(CNetCfgExtProvision::EUid, CNetCfgExtProvision::ETypeId)));
	TCFMessage::TStateChange& msg = message_cast<TCFMessage::TStateChange>(iContext.iMessage);

	if (provision && KLinkLayerOpen == msg.iStateChange.iStage)
		{
		return KSwallowMessage;
		}
	else
		{
		return KNoTag;
		}
	}

DEFINE_SMELEMENT(TStopNetCfgExtOrNoTag, NetStateMachine::MStateFork, IPProtoDeftSCpr::TContext)
TInt IPProtoDeftSCpr::TStopNetCfgExtOrNoTag::TransitionTag()
	{
	if (iContext.Node().iControl)
		return KStopNetCfgExt;

	return KNoTag;
	}

DEFINE_SMELEMENT(TNetworkConfiguredOrErrorTagOrCancelTagOrNoTag, NetStateMachine::MStateFork, IPProtoDeftSCpr::TContext)
TInt IPProtoDeftSCpr::TNetworkConfiguredOrErrorTagOrCancelTagOrNoTag::TransitionTag()
	{
	if (iContext.iMessage.IsMessage<TEBase::TCancel>())
		{
		iContext.iNodeActivity->SetError(KErrCancel);
		return KCancelTag;
		}
					
	TCFMessage::TStateChange& msg = message_cast<TCFMessage::TStateChange>(iContext.iMessage);

	if (msg.iStateChange.iError != KErrNone)
		{
		iContext.iNodeActivity->SetError(msg.iStateChange.iError);
		return KErrorTag;
		}

	if (msg.iStateChange.iStage == KLinkLayerOpen)
		{
		return KNetworkConfigured;
		}

	return KNoTag;
	}

DEFINE_SMELEMENT(TNetworkConfiguredOrErrorTagOrCancelTagOrNoTagBackward, NetStateMachine::MStateFork, IPProtoDeftSCpr::TContext)
TInt IPProtoDeftSCpr::TNetworkConfiguredOrErrorTagOrCancelTagOrNoTagBackward::TransitionTag()
	{
	TInt tag = IPProtoDeftSCpr::TNetworkConfiguredOrErrorTagOrCancelTagOrNoTag::TransitionTag();
	if (tag == KNoTag)
		return tag | NetStateMachine::EBackward;
	return tag;
	}

DEFINE_SMELEMENT(TErrorTagOrNoTag, NetStateMachine::MStateFork, IPProtoDeftSCpr::TContext)
TInt IPProtoDeftSCpr::TErrorTagOrNoTag::TransitionTag()
	{
	TEBase::TError *error = message_cast<TEBase::TError>(&iContext.iMessage);
	if (error)
		{
		iContext.iNodeActivity->SetError(error->iValue);

		return KErrorTag;
		}
	TCFIPProtoMessage::TNetworkConfigured* msg = message_cast<TCFIPProtoMessage::TNetworkConfigured>(&iContext.iMessage);

	if (msg->iValue != KErrNone)
		{
		iContext.iNodeActivity->SetError(msg->iValue);

		return KErrorTag;
		}
	return KNoTag;
	}

DEFINE_SMELEMENT(TNoTagOrConfigureNetwork, NetStateMachine::MStateFork, IPProtoDeftSCpr::TContext)
TInt IPProtoDeftSCpr::TNoTagOrConfigureNetwork::TransitionTag()
	{
	const RMetaExtensionContainerC& mec = iContext.Node().AccessPointConfig();
	const CNetCfgExtProvision* provision = static_cast<const CNetCfgExtProvision*>(mec.FindExtension(
	        STypeId::CreateSTypeId(CNetCfgExtProvision::EUid, CNetCfgExtProvision::ETypeId)));

	if (provision)
		{
		return KConfigureNetwork;
		}
	else
		{
		return KNoTag;
		}
	}

DEFINE_SMELEMENT(TNoTagOrProviderStoppedOrDaemonReleased, NetStateMachine::MStateFork, IPProtoDeftSCpr::TContext)
TInt IPProtoDeftSCpr::TNoTagOrProviderStoppedOrDaemonReleased::TransitionTag()
    {
    iContext.iNodeActivity->SetError(message_cast<TCFDataClient::TStop>(iContext.iMessage).iValue);
    if (iContext.Node().GetFirstClient<TDefaultClientMatchPolicy>(TClientType(TCFClientType::EData, TCFClientType::EStarted)) != NULL)
        {
        // At least one data client started 
        if(iContext.Node().iControl)
            {
            return MeshMachine::KNoTag;
            }
        else
            {
            return KDaemonReleased;
            }
        }
    return CoreNetStates::KProviderStopped;
    }

DEFINE_SMELEMENT(TDaemonReleasedStateChangedOrNoTagBackward, NetStateMachine::MStateFork, IPProtoDeftSCpr::TContext)
TInt IPProtoDeftSCpr::TDaemonReleasedStateChangedOrNoTagBackward::TransitionTag()
    {
    TInt tag = IPProtoDeftSCpr::TDaemonReleasedStateChangedOrNoTag::TransitionTag();
    if (tag == KNoTag)
        return tag | NetStateMachine::EBackward;
    return tag;
    }

DEFINE_SMELEMENT(TDaemonReleasedStateChangedOrNoTag, NetStateMachine::MStateFork, IPProtoDeftSCpr::TContext)
TInt IPProtoDeftSCpr::TDaemonReleasedStateChangedOrNoTag::TransitionTag()
    {
    TCFMessage::TStateChange& msg = message_cast<TCFMessage::TStateChange>(iContext.iMessage);
 
    if (msg.iStateChange.iStage == KConfigDaemonFinishedDeregistrationStop)
        {
        return KDaemonReleasedStateChanged;
        }
    return KNoTag;
    }

DEFINE_SMELEMENT(TConfigureNetwork, NetStateMachine::MStateTransition, IPProtoDeftSCpr::TContext)
void IPProtoDeftSCpr::TConfigureNetwork::DoL()
	{
	iContext.iNodeActivity->PostRequestTo(iContext.NodeId(), TCFIPProtoMessage::TConfigureNetwork().CRef());
	}

DEFINE_SMELEMENT(TSendNetworkConfigured, NetStateMachine::MStateTransition, IPProtoDeftSCpr::TContext)
void IPProtoDeftSCpr::TSendNetworkConfigured::DoL()
	{
	TCFMessage::TStateChange& msg = message_cast<TCFMessage::TStateChange>(iContext.iMessage);
      if (msg.iStateChange.iStage == KLinkLayerOpen)
          {
          // After network get configured if the last state change massage is KLinklayerOpen fwd the 
          // massage to control provider.
          iContext.Node().PostToClients<TDefaultClientMatchPolicy>(TNodeCtxId(iContext.ActivityId(), iContext.NodeId()), iContext.iMessage, TClientType(TCFClientType::ECtrlProvider));
          }
	TCFIPProtoMessage::TNetworkConfigured resp(iContext.iNodeActivity->Error());
	iContext.iNodeActivity->SetError(KErrNone);
	iContext.iNodeActivity->PostToOriginators(resp);
	}

DEFINE_SMELEMENT(TStartNetCfgExt, NetStateMachine::MStateTransition, IPProtoDeftSCpr::TContext)
void IPProtoDeftSCpr::TStartNetCfgExt::DoL()
	{
	ASSERT(!iContext.Node().iNotify);
	ASSERT(!iContext.Node().iControl);

	iContext.Node().iNotify = CNetCfgExtNotify::NewL(&iContext.Node());
	iContext.Node().iControl = CNifConfigurationControl::NewL(*iContext.Node().iNotify);
	iContext.Node().iControl->ConfigureNetworkL();
	}


DEFINE_SMELEMENT(TStopNetCfgExt, NetStateMachine::MStateTransition, IPProtoDeftSCpr::TContext)
void IPProtoDeftSCpr::TStopNetCfgExt::DoL()
	{
	if(!iContext.Node().iControl) return;

	iContext.Node().iControl->Deregister(EConfigDaemonDeregisterCauseStop);
	}

DEFINE_SMELEMENT(TStopNetCfgExtDelete, NetStateMachine::MStateTransition, IPProtoDeftSCpr::TContext)
void IPProtoDeftSCpr::TStopNetCfgExtDelete::DoL()
    {
    if(!iContext.Node().iControl)
        {
        return;
        }
    iContext.Node().iControl->AsyncDelete();
    iContext.Node().iControl = NULL;
 
    delete iContext.Node().iNotify;
    iContext.Node().iNotify = NULL;
    }

DEFINE_SMELEMENT(TResetSentTo, NetStateMachine::MStateTransition, IPProtoDeftSCpr::TContext)
void IPProtoDeftSCpr::TResetSentTo::DoL()
	{
	iContext.iNodeActivity->ClearPostedTo();
	}

//-=========================================================
// Ioctl Support
//-=========================================================
DEFINE_SMELEMENT(TAwaitingIoctlProcessed, NetStateMachine::MState, IPProtoDeftSCpr::TContext)
TBool IPProtoDeftSCpr::TAwaitingIoctlProcessed::Accept()
	{
	return iContext.iMessage.IsMessage<TCFLegacyMessage::TLegacyRMessage2Processed>();
	}

void IPProtoDeftSCpr::TAwaitingIoctlProcessed::Cancel()
	{
	CIPProtoDeftSubConnectionProvider* node = static_cast<CIPProtoDeftSubConnectionProvider*>(&iContext.Node());
	if (node->iControl)
		{
		node->iControl->CancelControl();
		}

	iContext.Activity()->SetError(KErrCancel);
	}

DEFINE_SMELEMENT(TNoTagOrTryNetCfgExt, NetStateMachine::MStateFork, IPProtoDeftSCpr::TContext)
TInt IPProtoDeftSCpr::TNoTagOrTryNetCfgExt::TransitionTag()
	{
	TCFLegacyMessage::TLegacyRMessage2Processed& msg = message_cast<TCFLegacyMessage::TLegacyRMessage2Processed>(iContext.iMessage);
	
	if ((msg.iResponse.iType == ESock::TLegacyRMessage2Response::ENormal && msg.iResponse.iCode == KErrNone)
		|| msg.iResponse.iType == ESock::TLegacyRMessage2Response::EPanic)
		{
		return KNoTag;
		}
	else
		{
		return KTryNetCfgExt;
		}
	}

DEFINE_SMELEMENT(TTryServiceProviderOrTryNetCfgExt, NetStateMachine::MStateFork, IPProtoDeftSCpr::TContext)
TInt IPProtoDeftSCpr::TTryServiceProviderOrTryNetCfgExt::TransitionTag()
	{
	MeshMachine::CNodeParallelMessageStoreActivityBase* act = static_cast<MeshMachine::CNodeParallelMessageStoreActivityBase*>(iContext.Activity());

	ASSERT(act->Message().IsTypeOf(Meta::STypeId::CreateSTypeId(TCFSigLegacyRMessage2Ext::EUid, TCFSigLegacyRMessage2Ext::ETypeId)));
	if ((static_cast<TCFSigLegacyRMessage2Ext&>(act->Message()).iMessage.Int0() == KCOLConfiguration
			&& static_cast<TCFSigLegacyRMessage2Ext&>(act->Message()).iMessage.Int1() == KConnGetSipServerAddr))
		{
		return KTryServiceProvider;
		}
	else
		{
		return KTryNetCfgExt;
		}
	}

DEFINE_SMELEMENT(TForwardToServiceProvider, NetStateMachine::MStateTransition, IPProtoDeftSCpr::TContext)
void IPProtoDeftSCpr::TForwardToServiceProvider::DoL()
	{
	ASSERT(iContext.iMessage.IsTypeOf(Meta::STypeId::CreateSTypeId(TCFSigLegacyRMessage2Ext::EUid, TCFSigLegacyRMessage2Ext::ETypeId)));
	
	MeshMachine::CNodeParallelMessageStoreActivityBase* act = static_cast<MeshMachine::CNodeParallelMessageStoreActivityBase*>(iContext.Activity());

	TCFSigLegacyRMessage2Ext::RReadOnlyRMessage& ioctl(static_cast<TCFSigLegacyRMessage2Ext&>(act->Message()).iMessage);
	RLegacyResponseMsg r(iContext, static_cast<TCFSigLegacyRMessage2Ext&>(act->Message()).iMessage, static_cast<TCFSigLegacyRMessage2Ext&>(act->Message()).iMessage.Int0());
			
	// currently only supporting configuration ioctls
	if (ioctl.Int0() == KCOLConfiguration && ioctl.Int1() == KConnGetSipServerAddr)
		{
		TSipServerAddrBuf sipAddr;
		if (ioctl.GetDesLengthL(2) != sipAddr.Length())
			{
			r.Panic(KESockClientPanic, EBadDescriptor);
			act->ClearPostedTo();
			return; // the sip address handlers will complete the message
			}
		}
		
	RNodeInterface* dc = iContext.Node().GetFirstClient<TDefaultClientMatchPolicy>(TClientType(TCFClientType::EServProvider));
	
	iContext.Activity()->PostRequestTo(*dc, iContext.iMessage);
	}

DEFINE_SMELEMENT(THandoffToNetCfgExt, NetStateMachine::MStateTransition, IPProtoDeftSCpr::TContext)
void IPProtoDeftSCpr::THandoffToNetCfgExt::DoL()
	{
	ASSERT(iContext.Activity());
	MeshMachine::CNodeParallelMessageStoreActivityBase* act = static_cast<MeshMachine::CNodeParallelMessageStoreActivityBase*>(iContext.Activity());

	TCFSigLegacyRMessage2Ext::RReadOnlyRMessage& ioctl(static_cast<TCFSigLegacyRMessage2Ext&>(act->Message()).iMessage);
	RLegacyResponseMsg r(iContext, static_cast<TCFSigLegacyRMessage2Ext&>(act->Message()).iMessage, static_cast<TCFSigLegacyRMessage2Ext&>(act->Message()).iMessage.Int0());

	if (ioctl.Int0() == KCOLConfiguration && ioctl.Int1() == KConnGetSipServerAddr)
		{
		TCFSigLegacyRMessage2Ext::RReadOnlyRMessage& ioctl(static_cast<TCFSigLegacyRMessage2Ext&>(act->Message()).iMessage);
		TSipServerAddrBuf sipAddr;
		if (ioctl.GetDesLengthL(2) != sipAddr.Length())
			{
			r.Panic(KESockClientPanic, EBadDescriptor);
			act->ClearPostedTo();
			return; // the sip address handlers will complete the message
			}
		}
		
	if ((ioctl.Int0() == KCOLConfiguration && ioctl.Int1() == KConnGetSipServerAddr))
		{
		TSipServerAddrBuf sipBuf;
		ioctl.ReadL( 2, sipBuf );

		if( sipBuf().index != 0 )
			{
			r.Complete(KErrNotFound);
			act->ClearPostedTo();
			return;
			}
		}
		
	// IPProto returns KErrNotSupported if DHCP is not configured for the IAP
	// so we want to make sure DHCP fallback is only ever tried once for index
	// zero.  Otherwise, the last address index that the caller tries may return
	// KErrNotSupported instead of the expected KErrNotFound.
	if (iContext.Node().iControl)
		{
		iContext.Node().iControl->SendIoctlMessageL(r);
		act->ClearPostedTo();
		}
	else
		{
		const RMetaExtensionContainerC& mec = iContext.Node().AccessPointConfig();
		const CNetCfgExtProvision* provision = static_cast<const CNetCfgExtProvision*>(mec.FindExtension(
				STypeId::CreateSTypeId(CNetCfgExtProvision::EUid, CNetCfgExtProvision::ETypeId)));

		if (provision)
			{
			r.Complete(KErrNotReady);
			act->ClearPostedTo();
			}
		else
			{
			r.Complete(KErrNotSupported);
			act->ClearPostedTo();
			}
		}
	}

//
// support for AgentEventNotification
//
DEFINE_SMELEMENT(TAwaitingAgentEventNotification, NetStateMachine::MState, IPProtoDeftSCpr::TContext)
TBool IPProtoDeftSCpr::TAwaitingAgentEventNotification::Accept()
	{
	return iContext.iMessage.IsMessage<TLinkMessage::TAgentEventNotification>();
	}

DEFINE_SMELEMENT(TProcessAgentEvent, NetStateMachine::MStateTransition, IPProtoDeftSCpr::TContext)
void IPProtoDeftSCpr::TProcessAgentEvent::DoL()
	{
	if (iContext.Node().iControl)
		{
		// AgentEventNotification originates from the AgentSCPr as a result of an AgentEvent()
		// call from the Agent.  Although the message is generic, it was primarily created to pass the
		// event ECurrentNetworkChangeEvent from the Agent towards the NetCfgExt for CDMA2000 support.
		// The "aEventData" and "aSource" fields of the EventNotification() call are not used for this
		// event, and hence are passed as null values.
		TLinkMessage::TAgentEventNotification& msg = message_cast<TLinkMessage::TAgentEventNotification>(iContext.iMessage);
		iContext.Node().iControl->EventNotification(TNetworkAdaptorEventType(msg.iValue1), msg.iValue2, KNullDesC8, NULL);
		}
	}

