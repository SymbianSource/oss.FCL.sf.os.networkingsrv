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
#include "IPProtoSCPR.h"
#include "IPProtoSCPRStates.h"
#include "ItfInfoConfigExt.h"
#include <comms-infras/ss_datamonitoringprovider.h>

using namespace ESock;
using namespace NetStateMachine;
using namespace IPProtoSCpr;
using namespace Messages;

#ifndef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
DEFINE_SMELEMENT(TSendParamsToServiceProvider, NetStateMachine::MStateTransition, IPProtoSCpr::TContext)
void TSendParamsToServiceProvider::DoL()
	{
	ASSERT( ! iContext.Node().iParameterBundle.IsNull());
	iContext.iNodeActivity->PostRequestTo(*iContext.Node().ServiceProvider(),
	    TCFScpr::TParamsRequest(iContext.Node().iParameterBundle).CRef());
	}
#endif // SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW

DEFINE_SMELEMENT(TStoreProvision, NetStateMachine::MStateTransition, IPProtoSCpr::TContext)
void IPProtoSCpr::TStoreProvision::DoL()
	{
	PRStates::TStoreProvision storeProvision(iContext);
	storeProvision.DoL();			// set up iContext.Node().iProvisionConfig
	
	//[401TODO]: RZ Add datamonitoring ptrs. back.
    // Allocate and add the data monitoring shared memory pointers the provisioning info
	//CIPProtoSubConnectionProvider& node = iContext.Node();
	//TDataMonitoringSubConnProvisioningInfo* subConnProvisioningInfo = new (ELeave) TDataMonitoringSubConnProvisioningInfo(&node.iDataVolumes, &node.iThresholds);
	//CleanupStack::PushL(subConnProvisioningInfo);
	//node.AccessPointConfig().AppendExtensionL(subConnProvisioningInfo);
	//CleanupStack::Pop(subConnProvisioningInfo);
	}


//[401TODO]: DL Rename this to SendAddressInfoBundle
DEFINE_SMELEMENT(TSendDataClientRoutedToFlow, NetStateMachine::MStateTransition, IPProtoSCpr::TContext)
void IPProtoSCpr::TSendDataClientRoutedToFlow::DoL()
	{
	ASSERT(!iContext.Node().GetParameterBundle().IsNull());
	RParameterFamily dummy;
	(void)dummy;
	 RParameterFamily ipAddressInfoFamily = iContext.Node().GetParameterBundle().FindFamily(KSubConIPAddressInfoFamily);
	 if ( ipAddressInfoFamily.IsNull())
	     {
	     User::Leave(KErrArgument);
	     }
	RNodeInterface* dc = iContext.Node().GetFirstClient<TDefaultClientMatchPolicy>(TCFClientType::EData);	
	if (dc)
		{
#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
		dc->PostMessage(TNodeCtxId(iContext.ActivityId(), iContext.NodeId()), TCFScpr::TSetParamsRequest(iContext.Node().iParameterBundle).CRef());			
#else
		dc->PostMessage(TNodeCtxId(iContext.ActivityId(), iContext.NodeId()), TCFScpr::TParamsRequest(iContext.Node().iParameterBundle).CRef());
#endif // SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
		}
	}


//-=========================================================
// Data monitoring support
//-=========================================================
DEFINE_SMELEMENT(TAwaitingDataMonitoringNotification, NetStateMachine::MState, IPProtoSCpr::TContext)
TBool IPProtoSCpr::TAwaitingDataMonitoringNotification::Accept()
	{
	return iContext.iMessage.IsMessage<TCFDataMonitoringNotification::TDataMonitoringNotification>();
	}

DEFINE_SMELEMENT(TProcessDataMonitoringNotification, NetStateMachine::MStateTransition, IPProtoSCpr::TContext)
void IPProtoSCpr::TProcessDataMonitoringNotification::DoL()
	{
	TCFDataMonitoringNotification::TDataMonitoringNotification& msg =
		message_cast<TCFDataMonitoringNotification::TDataMonitoringNotification>(iContext.iMessage);

	iContext.Node().DataNotificationL(msg);
	}

//
// support for AgentEventNotification
//
DEFINE_SMELEMENT(TAwaitingAgentEventNotification, NetStateMachine::MState, IPProtoSCpr::TContext)
TBool IPProtoSCpr::TAwaitingAgentEventNotification::Accept()
	{
	return iContext.iMessage.IsMessage<TLinkMessage::TAgentEventNotification>();
	}

DEFINE_SMELEMENT(TProcessAgentEvent, NetStateMachine::MStateTransition, IPProtoSCpr::TContext)
void IPProtoSCpr::TProcessAgentEvent::DoL()
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

