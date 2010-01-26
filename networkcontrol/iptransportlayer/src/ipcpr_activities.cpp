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
// IP Connection Provider activity definitions.
// 
//

/**
 @file
 @internalComponent
*/

#define SYMBIAN_NETWORKING_UPS

#include "IPCpr.h" 
#include "ipcpr_activities.h"
#include "ipcpr_states.h"
#include <comms-infras/ss_nodemessages.h>
#include <comms-infras/corecpractivities.h>
#include <comms-infras/ss_corepractivities.h>
#include <comms-infras/ss_nodemessages_dataclient.h>
#include <comms-infras/ss_nodemessages_internal.h>
#include "IPMessages.h"

#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
#include <comms-infras/ss_nodemessages_scpr.h>
#endif // SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW


#ifdef SYMBIAN_NETWORKING_UPS
#include <comms-infras/upspractivities.h>
#include "ipcprups_states.h"
#endif

using namespace ESock;
using namespace NetStateMachine;
using namespace IpCprActivities;
using namespace IpCprStates;
using namespace MeshMachine;

namespace IpCprActivities
{

DECLARE_DEFINE_CUSTOM_NODEACTIVITY(ECFActivityStart, IpCprStart, TCFServiceProvider::TStart, PRActivities::CStartActivity::NewL)
    NODEACTIVITY_ENTRY(KNoTag, IpCprStates::TCheckStartCapabilities, CoreNetStates::TAwaitingStart, CoreNetStates::TNoTagOrBearerPresentBlockedByStop)
	
	NODEACTIVITY_ENTRY(CoreNetStates::KBearerPresent, CoreNetStates::TBindSelfToPresentBearer, CoreNetStates::TAwaitingBindToComplete, MeshMachine::TTag<CoreNetStates::KBearerPresent>)
	// Note for UPS, we must allow TCancel messages to propagate through to NetMCpr, so that long running
	// UPS requests occurring as part of the NoBearer activity can be cancelled.
	NODEACTIVITY_ENTRY(KNoTag, CoreNetStates::TSendNoBearer, MeshMachine::TAwaitingMessageState<TCFControlProvider::TBearer>, CoreNetStates::TNoTagOrBearerPresentOrErrorTag)
	
	//Start the service provider, use the default cancellation.
	//Forward TCancel to the service provider, wait for TStarted or TError (via the Error Activity)
	//When TStarted arrives after TCancel the activity will move to the nearest KErrorTag
	NODEACTIVITY_ENTRY(CoreNetStates::KBearerPresent, CoreNetStates::TStartServiceProviderRetry, CoreNetStates::TAwaitingStarted, MeshMachine::TNoTagOrErrorTag)
	LAST_NODEACTIVITY_ENTRY(KErrorTag, MeshMachine::TDoNothing)

	#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	//This entry will initialise the parameter bundle and send to self. SendToSelf will trigger a request to the lower layers.
	//The lower layer (Agent/PDP CPR) will store the bearer type in the parameter set and send the response.
	NODEACTIVITY_ENTRY(KNoTag, IpCprStates::TInitialiseParamsAndSendToSelf, CoreNetStates::TAwaitingParamResponse, MeshMachine::TNoTag)
	//MCPR is updated with the bearer type
	THROUGH_NODEACTIVITY_ENTRY(KNoTag, IpCprStates::TUpdateProvisionConfigAtStartup, TNoTag)
	//Throughnode activity which will send the TCP receive window size to the data clients
	THROUGH_NODEACTIVITY_ENTRY(KNoTag, IpCprStates::TSendTransportNotificationToDataClients, MeshMachine::TNoTag)
	#endif //SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW

	//Start data clients, use the default cancellation.
	//Forward TCancel to the self, wait for TCFDataClient::TStarted or TError (via the Error Activity)
	//When TCFDataClient::TStarted arrives after TCancel the activity will move to the nearest KErrorTag
	NODEACTIVITY_ENTRY(KNoTag, CoreNetStates::TStartSelf, CoreNetStates::TAwaitingDataClientStarted, MeshMachine::TNoTagOrErrorTag)
	NODEACTIVITY_ENTRY(KErrorTag, CoreNetStates::TStopSelf, CoreNetStates::TAwaitingDataClientStopped, MeshMachine::TErrorTag)
	LAST_NODEACTIVITY_ENTRY(KErrorTag, MeshMachine::TDoNothing)

	THROUGH_NODEACTIVITY_ENTRY(KNoTag, IpCprStates::TSendInitialSubConnectionOpenedEvent, MeshMachine::TNoTag)
	THROUGH_NODEACTIVITY_ENTRY(KNoTag, IpCprStates::TSendSubsequentSubConnectionOpenedEvent, MeshMachine::TNoTag)
	LAST_NODEACTIVITY_ENTRY(KNoTag, CoreNetStates::TSendStarted)
NODEACTIVITY_END()


DECLARE_DEFINE_CUSTOM_NODEACTIVITY(ECFActivityStop, IpCprStop, TCFServiceProvider::TStop, MeshMachine::CNodeRetryActivity::NewL)
	NODEACTIVITY_ENTRY(KNoTag, IpCprStates::TCheckStopCapabilities, CoreNetStates::TAwaitingStop, CoreNetStates::TNoTagBlockedByStart)
	THROUGH_NODEACTIVITY_ENTRY(KNoTag, MeshMachine::TDoNothing, MeshMachine::TActiveOrNoTag<ECFActivityStartDataClient>)
	THROUGH_NODEACTIVITY_ENTRY(KActiveTag, CoreNetStates::TCancelDataClientStart, MeshMachine::TNoTag) //MZTODO: This triple should wait for TError sent as a response to TCancel

	NODEACTIVITY_ENTRY(KNoTag, CoreNetStates::TStopSelf, CoreNetStates::TAwaitingDataClientStopped, CoreNetStates::TNoTagOrNoBearer)
	NODEACTIVITY_ENTRY(KNoTag, CoreNetStates::TSendStop, CoreNetStates::TAwaitingStopped, MeshMachine::TNoTag)
	NODEACTIVITY_ENTRY(KNoTag, CoreNetStates::TSendClientLeavingRequestToServiceProvider, MeshMachine::TAwaitingLeaveComplete, MeshMachine::TTag<CoreNetStates::KNoBearer>)

	LAST_NODEACTIVITY_ENTRY(CoreNetStates::KNoBearer, PRStates::TSendStoppedAndGoneDown)
NODEACTIVITY_END()

#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
/*
This activity is modified to retrieve the bearer type from lower layer for the use case of 
implicit socket.
*/
#endif //SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
DECLARE_DEFINE_CUSTOM_NODEACTIVITY(ECFActivityNoBearer, IpCprNoBearer, TCFControlProvider::TNoBearer, PRActivities::CNoBearer::NewL)
	FIRST_NODEACTIVITY_ENTRY(CoreNetStates::TAwaitingNoBearer, PRActivities::CNoBearer::TNoTagOrBearerPresentBlockedByNoBearer)
	NODEACTIVITY_ENTRY(KNoTag, CoreNetStates::TSendNoBearer, MeshMachine::TAwaitingMessageState<TCFControlProvider::TBearer>, MeshMachine::TNoTagOrErrorTag)

	NODEACTIVITY_ENTRY(CoreNetStates::KBearerPresent, PRActivities::CNoBearer::TRequestCommsBinderRetry, CoreNetStates::TAwaitingBinderResponse, MeshMachine::TTag<CoreNetStates::KBearerPresent>)
	NODEACTIVITY_ENTRY(CoreNetStates::KBearerPresent, CoreNetStates::TSendBindTo, CoreNetStates::TAwaitingBindToComplete, MeshMachine::TTag<CoreNetStates::KBearerPresent>)
	THROUGH_NODEACTIVITY_ENTRY(CoreNetStates::KBearerPresent,CoreActivities::ABindingActivity::TSendBindToComplete, MeshMachine::TNoTag) 

	NODEACTIVITY_ENTRY(KNoTag, CoreNetStates::TStartServiceProviderRetry, CoreNetStates::TAwaitingStarted, MeshMachine::TNoTag)
	
#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	//Request for bearer type from lower layer
	NODEACTIVITY_ENTRY(KNoTag, IpCprStates::TInitialiseParamsAndSendToSelf, CoreNetStates::TAwaitingParamResponse, MeshMachine::TNoTag)
	//MCPR is updated with the bearer type
	THROUGH_NODEACTIVITY_ENTRY(KNoTag, IpCprStates::TUpdateProvisionConfigAtStartup, TNoTag)
	//Throughnode activity which will send the TCP receive window size to the data clients
	THROUGH_NODEACTIVITY_ENTRY(KNoTag, IpCprStates::TSendTransportNotificationToDataClients, MeshMachine::TNoTag)
#endif //SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW

	THROUGH_NODEACTIVITY_ENTRY(KNoTag, IpCprStates::TSendSubsequentSubConnectionOpenedEvent, MeshMachine::TNoTag)
	
	LAST_NODEACTIVITY_ENTRY(KNoTag, CoreNetStates::TSendBearer)
	LAST_NODEACTIVITY_ENTRY(KErrorTag, MeshMachine::TDoNothing)
NODEACTIVITY_END()

DECLARE_DEFINE_NODEACTIVITY(ECFIpCprActivitySubConnEvents, IpCprSubConnEvents, TCFControlProvider::TDataClientStatusChange)
	NODEACTIVITY_ENTRY(KNoTag, PRStates::THandleDataClientStatusChangeAndDestroyOrphans, CoreNetStates::TAwaitingDataClientStatusChange, MeshMachine::TNoTag)
NODEACTIVITY_END()

DECLARE_DEFINE_NODEACTIVITY(ECFIpCprActivitySubConnDataTransferred, IpCprSubConnDataTransferred, TCFMessage::TSubConnDataTransferred)
	NODEACTIVITY_ENTRY(KNoTag, IpCprStates::TProcessSubConnDataTransferred, IpCprStates::TAwaitingSubConnDataTransferred, MeshMachine::TNoTag)
NODEACTIVITY_END()

DECLARE_DEFINE_NODEACTIVITY(ECFActivityCustom, IPCprSendPolicyParams, TCFIPMessage::TPolicyParams)
	NODEACTIVITY_ENTRY(KNoTag, IpCprStates::TSendPolicyParams, IpCprStates::TAwaitingPolicyParams, MeshMachine::TNoTag)
NODEACTIVITY_END()

#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
/*
This activity awaits for a notification in case of Modulation change for the same link layer.
It receives the bearer type after modulation change in TPlaneNotification message.
The bearer info is updated to MCPR and the TCP receive window size is sent to all data clients.
*/
DECLARE_DEFINE_NODEACTIVITY(ECFActivityNotification, IPCprEventNotification, TCFSubConnControlClient::TPlaneNotification)
	FIRST_NODEACTIVITY_ENTRY(CoreNetStates::TAwaitingConEvent, MeshMachine::TNoTag)
	THROUGH_NODEACTIVITY_ENTRY(KNoTag, IpCprStates::TUpdateProvisionConfigAtModulation, MeshMachine::TNoTag)
	LAST_NODEACTIVITY_ENTRY(KNoTag, IpCprStates::TSendTransportNotificationToDataClients)
NODEACTIVITY_END()
#endif // SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW

DEFINE_ACTIVITY_MAP(ipCprActivities)
	ACTIVITY_MAP_ENTRY(IpCprActivities, IpCprStart)
	ACTIVITY_MAP_ENTRY(IpCprActivities, IpCprStop)
	ACTIVITY_MAP_ENTRY(IpCprActivities, IpCprSubConnEvents)
	ACTIVITY_MAP_ENTRY(IpCprActivities, IpCprSubConnDataTransferred)
	ACTIVITY_MAP_ENTRY(IpCprActivities, IpCprNoBearer)
	ACTIVITY_MAP_ENTRY(IpCprActivities, IPCprSendPolicyParams)
#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	ACTIVITY_MAP_ENTRY(IpCprActivities, IPCprEventNotification)
#endif //SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW

#ifdef SYMBIAN_NETWORKING_UPS
	ACTIVITY_MAP_ENTRY(IpCprActivities, IpCprControlClientJoin)
	ACTIVITY_MAP_ENTRY(IpCprActivities, IpCprPolicyCheckRequest)
	ACTIVITY_MAP_ENTRY(IpCprActivities, IpCprClientLeave)	
ACTIVITY_MAP_END_BASE(UpsActivities, upsActivitiesCpr)
#else
ACTIVITY_MAP_END_BASE(MobilityCprActivities, mobilityCprActivities)
#endif //SYMBIAN_NETWORKING_UPS

}
