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

#define SYMBIAN_NETWORKING_UPS

#include <comms-infras/corecprstates.h>
#include <comms-infras/corecpractivities.h>
#include <comms-infras/ss_log.h>
#include <comms-infras/ss_legacyinterfaces.h>
#include <comms-infras/ss_datamon_apiext.h>
#include <es_prot.h> // ESocketTimerPriority/KConnProfile(None/Long/Medium)
#include <e32def.h>
#include <es_prot_internal.h>


#include "IPProtoCprStates.h"
#include "IPProtoCPR.h"
#include "IPProtoMCpr.h"
#include "IPProtoMessages.h"
#include "linkcprextensionapi.h"

#include <comms-infras/ss_nodemessages_factory.h>
#include <comms-infras/ss_msgintercept.h>
#include <comms-infras/ss_nodemessages_internal.h>

#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
#include <comms-infras/ss_nodemessages_subconn.h>
#endif //SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW

#ifdef _DEBUG
   #include <networking/idletimertest.h>
#endif

using namespace Messages;
using namespace MeshMachine;
using namespace IpProtoCpr;
using namespace ESock;
using namespace NetStateMachine;
using namespace PRActivities;



//We reserve space for two preallocated activities that may start concurrently on the CPR
//node: destroy and data client stop.
static const TUint KDefaultMaxPreallocatedActivityCount = 2;
static const TUint KMaxPreallocatedActivitySize = sizeof(MeshMachine::CNodeRetryParallelActivity) + sizeof(MeshMachine::APreallocatedOriginators<4>);
static const TUint KIPProtoCPRPreallocatedActivityBufferSize = KDefaultMaxPreallocatedActivityCount * KMaxPreallocatedActivitySize;

//-=========================================================
//
// Activities
//
//-=========================================================

namespace IPProtoCprProvisionActivity
{
DECLARE_DEFINE_NODEACTIVITY(ECFActivityStoreProvision, IPProtoCprProvision, TCFDataClient::TProvisionConfig)
	FIRST_NODEACTIVITY_ENTRY(CoreNetStates::TAwaitingProvision, MeshMachine::TNoTag)
	THROUGH_NODEACTIVITY_ENTRY(KNoTag, IpProtoCpr::TProvisionActivation, MeshMachine::TNoTagOrErrorTag)
	LAST_NODEACTIVITY_ENTRY(KNoTag, IpProtoCpr::TStoreProvision)
	LAST_NODEACTIVITY_ENTRY(KErrorTag, IpProtoCpr::THandleProvisionError)
NODEACTIVITY_END()
}


namespace IPProtoCprConnectionDownActivity
{
// In the event that a StopConnection has been issued this activity will not receive
// a ConnectionDown message. It will be received instead by the running StopConnectionActivity.
//
// In the event that the Idle Timer has expired, this node will originate a StopConnection
// to itself, the StopConnectionActivity will post a ConnectionDown to the origator (this
// node) and the StopConnectionActivity will go idle. The ConnectionDown message will then
// be received by this activity.

DECLARE_DEFINE_NODEACTIVITY(ECFActivityGoneDown, IPProtoCprConnectionDown, TCFServiceProvider::TStopped)
	FIRST_NODEACTIVITY_ENTRY(CoreNetStates::TAwaitingStopped, MeshMachine::TNoTag)
	LAST_NODEACTIVITY_ENTRY(KNoTag, PRStates::TSendGoneDown)
NODEACTIVITY_END()
}

namespace IPProtoCprBinderRequestActivity
{
//The reason IPProtoCPR overrides this activity is that IPProto layer doesn't
//implement non-default SCPRs and although higher levels will ask for them
//(in QoS scenarios) IPProto will assume the higher levels will do fine
//with default SCPRs instead. The current QoS solution involves GuQoS and
//multiplexining channels at IPProto layer.
DECLARE_DEFINE_CUSTOM_NODEACTIVITY(ECFActivityBinderRequest, IPProtoCprBinderRequest, TCFServiceProvider::TCommsBinderRequest, PRActivities::CCommsBinderActivity::NewL)
//	FIRST_NODEACTIVITY_ENTRY(CoreNetStates::TAwaitingBinderRequest, CCommsBinderActivity::TNoTagOrWaitForIncomingOrUseExistingBlockedByBinderRequest)
	FIRST_NODEACTIVITY_ENTRY(CoreNetStates::TAwaitingBinderRequest, CCommsBinderActivity::TNoTagOrWaitForIncomingOrUseExistingDefaultBlockedByBinderRequest)
	NODEACTIVITY_ENTRY(KNoTag, PRStates::TCreateDataClient, CoreNetStates::TAwaitingDataClientJoin, MeshMachine::TNoTag)

	THROUGH_NODEACTIVITY_ENTRY(KNoTag, CCommsBinderActivity::TProcessDataClientCreation, TTag<CoreStates::KUseExisting>)

	NODEACTIVITY_ENTRY(CoreStates::KUseExisting, CCommsBinderActivity::TSendBinderResponse, CCommsBinderActivity::TAwaitingBindToComplete, MeshMachine::TNoTagOrErrorTag)
	LAST_NODEACTIVITY_ENTRY(KNoTag, MeshMachine::TDoNothing)

	LAST_NODEACTIVITY_ENTRY(KErrorTag, MeshMachine::TClearError)
	LAST_NODEACTIVITY_ENTRY(CoreNetStates::KWaitForIncoming, MeshMachine::TRaiseError<KErrNotSupported>)
NODEACTIVITY_END()
}



namespace IPProtoCprDataMonitoringActivity
{
DECLARE_DEFINE_NODEACTIVITY(ECFActivityDataMonitoring, IPProtoCprDataMonitoring, TCFDataMonitoringNotification::TDataMonitoringNotification)
	FIRST_NODEACTIVITY_ENTRY(IpProtoCpr::TAwaitingDataMonitoringNotification, MeshMachine::TNoTag)
	LAST_NODEACTIVITY_ENTRY(KNoTag, IpProtoCpr::TProcessDataMonitoringNotification)
NODEACTIVITY_END()
}

namespace IPProtoCprOpenCloseRouteActivity
{
DECLARE_DEFINE_NODEACTIVITY(ECFActivityOpenCloseRoute, IPProtoCprOpenCloseRoute, TCFIPProtoMessage::TOpenCloseRoute)
	FIRST_NODEACTIVITY_ENTRY(IpProtoCpr::TAwaitingOpenCloseRoute, MeshMachine::TNoTag)
	LAST_NODEACTIVITY_ENTRY(KNoTag, IpProtoCpr::TDoOpenCloseRoute)
NODEACTIVITY_END()
}

namespace IPProtoCprForwardStateChangeActivity
{
DECLARE_DEFINE_NODEACTIVITY(ECFActivityForwardStateChange, IPProtoCprForwardStateChange, TCFMessage::TStateChange)
	NODEACTIVITY_ENTRY(KNoTag, IpProtoCpr::TStoreAndFilterDeprecatedAndForwardStateChange, MeshMachine::TAwaitingMessageState<TCFMessage::TStateChange>, MeshMachine::TNoTag)
NODEACTIVITY_END()
}




namespace IPProtoCprLinkDown
{
	
	DECLARE_DEFINE_NODEACTIVITY(ECFActivityGoneDown, IPProtoCprLinkDownOnMesg, TCFControlClient::TGoneDown)
	// Our Service Provider has gone down unexpectedly (we haven't issued a TStop)
	NODEACTIVITY_ENTRY(KNoTag, MeshMachine::TDoNothing, TAwaitingGoneDown, MeshMachine::TNoTag)
	NODEACTIVITY_END()
}



namespace IPProtoCprStartActivity
{
typedef MeshMachine::TAcceptErrorState<CoreNetStates::TAwaitingDataClientStarted> TAwaitingDataClientStartedOrError;

DECLARE_DEFINE_CUSTOM_NODEACTIVITY(ECFActivityStart, IPProtoCprStart, TCFServiceProvider::TStart, PRActivities::CStartActivity::NewL)
    FIRST_NODEACTIVITY_ENTRY(IpProtoCpr::TAwaitingStart, CoreNetStates::TNoTagOrBearerPresentBlockedByStop)
	NODEACTIVITY_ENTRY(CoreNetStates::KBearerPresent, CoreNetStates::TBindSelfToPresentBearer, CoreNetStates::TAwaitingBindToComplete, TTag<CoreNetStates::KBearerPresent>)

	NODEACTIVITY_ENTRY(KNoTag, CoreNetStates::TSendNoBearer, MeshMachine::TAwaitingMessageState<TCFControlProvider::TBearer>, TErrorTagOr<TTag<CoreNetStates::KBearerPresent> >)

	//Start the service provider, use the default cancellation.
	//Forward TCancel to the service provider, wait for TStarted or TError (via the Error Activity)
	//When TStarted arrives after TCancel the activity will move to the nearest KErrorTag
	NODEACTIVITY_ENTRY(CoreNetStates::KBearerPresent, CoreNetStates::TStartServiceProviderRetry, CoreNetStates::TAwaitingStarted, MeshMachine::TNoTagOrErrorTag)
	LAST_NODEACTIVITY_ENTRY(KErrorTag, IpProtoCpr::TCleanupStart)

	//Start data clients, use the default cancellation.
	//Forward TCancel to the self, wait for TCFDataClient::TStarted or TError (via the Error Activity)
	//When TCFDataClient::TStarted arrives after TCancel the activity will move to the nearest KErrorTag
	NODEACTIVITY_ENTRY(KNoTag, TLinkUpAndTStartSelf, TAwaitingDataClientStartedOrError, MeshMachine::TNoTagOrErrorTag)
	LAST_NODEACTIVITY_ENTRY(KNoTag, IpProtoCpr::TSendStarted)

	//IPProto layer must stop the lower layer on failure to start as it would detach the lower layer from the idle timer impl.
	NODEACTIVITY_ENTRY(KErrorTag, IpProtoCpr::TSendStopToSelf, CoreNetStates::TAwaitingStopped, MeshMachine::TErrorTag)
	LAST_NODEACTIVITY_ENTRY(KErrorTag, IpProtoCpr::TCleanupStart)
NODEACTIVITY_END()
}

namespace IPProtoCprClientLeaveActivity
{ //This activity will wait for ECFActivityBinderRequest to complete
using namespace  CprClientLeaveActivity;
DECLARE_DEFINE_CUSTOM_NODEACTIVITY(ECFActivityClientLeave, IPProtoCprClientLeave, Messages::TNodeSignal::TNullMessageId, CClientLeaveActivity::NewL) //May be waiting for both messages
	FIRST_NODEACTIVITY_ENTRY(CoreStates::TAwaitingClientLeave, MeshMachine::TNoTag)
	THROUGH_NODEACTIVITY_ENTRY(KNoTag, CprClientLeaveActivity::CClientLeaveActivity::TRemoveClientAndDestroyOrphanedDataClients, CClientLeaveActivity::TNoTagOrSendPriorityToCtrlProvider)
	NODEACTIVITY_ENTRY(CprStates::KSendPriorityToCtrlProvider, CClientLeaveActivity::TUpdatePriorityForControlProvider, CoreStates::TAwaitingJoinComplete, CClientLeaveActivity::TNoTagOrSendPriorityToServProvider)
	NODEACTIVITY_ENTRY(CprStates::KSendPriorityToServProvider, CClientLeaveActivity::TUpdatePriorityForServiceProviders, CoreStates::TAwaitingJoinComplete, MeshMachine::TNoTag)
 	THROUGH_NODEACTIVITY_ENTRY(KNoTag, CprClientLeaveActivity::CClientLeaveActivity::TSendLeaveCompleteAndSendDataClientIdleIfNeeded, MeshMachine::TNoTag)
 	LAST_NODEACTIVITY_ENTRY(KNoTag, IpProtoCpr::TCheckIfLastControlClientLeaving)
NODEACTIVITY_END()
}

DECLARE_DEFINE_NODEACTIVITY(ECFIpProtoCprActivityDataClientStatusChange, IPProtoCprDataClientStatusChangeActivity, TCFControlProvider::TDataClientStatusChange)
	NODEACTIVITY_ENTRY(KNoTag, IpProtoCpr::TProcessDataClientStatusChange, CoreNetStates::TAwaitingDataClientStatusChange, MeshMachine::TNoTag)
NODEACTIVITY_END()

#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
namespace IPProtoCprNotificationActivity
{
DECLARE_DEFINE_NODEACTIVITY(ECFActivityNotification, IPProtoCprNotification, TCFSubConnControlClient::TPlaneNotification)
	NODEACTIVITY_ENTRY(KNoTag, CoreNetStates::TPassPlaneEventToControlClients, CoreNetStates::TAwaitingConEvent, MeshMachine::TNoTag)
NODEACTIVITY_END()
}
#endif // SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW

namespace IPProtoCprIoctlActivity
{
DECLARE_DEFINE_CUSTOM_NODEACTIVITY(ECFActivityIoctl, IPProtoCprIoctl, TNodeSignal::TNullMessageId, MeshMachine::CNodeParallelMessageStoreActivityBase::NewL)
    FIRST_NODEACTIVITY_ENTRY(IpProtoCpr::TAwaitingIoctlMessage, MeshMachine::TNoTag)
    NODEACTIVITY_ENTRY(KNoTag, IpProtoCpr::TForwardToDefaultDataClient, CoreNetStates::TAwaitingRMessage2Processed, MeshMachine::TNoTag)
    LAST_NODEACTIVITY_ENTRY(KNoTag, CoreStates::TPostToOriginators)
NODEACTIVITY_END()
}

namespace IPProtoCprStopActivity
{
DECLARE_DEFINE_CUSTOM_NODEACTIVITY(ECFActivityStop, IPProtoCprStop, TCFServiceProvider::TStop, MeshMachine::CNodeRetryActivity::NewL)
    FIRST_NODEACTIVITY_ENTRY(IpProtoCpr::TAwaitingStop, CoreNetStates::TActiveOrNoTagBlockedByBindTo)
	THROUGH_NODEACTIVITY_ENTRY(KActiveTag, CoreNetStates::TCancelDataClientStart, MeshMachine::TNoTag)
	NODEACTIVITY_ENTRY(KNoTag, CoreNetStates::TStopSelf, CoreNetStates::TAwaitingDataClientStopped, CoreNetStates::TNoTagOrNoBearer)
	NODEACTIVITY_ENTRY(KNoTag, CoreNetStates::TSendStop, CoreNetStates::TAwaitingStopped, MeshMachine::TNoTag)
	NODEACTIVITY_ENTRY(KNoTag, CoreNetStates::TSendClientLeavingRequestToServiceProvider, MeshMachine::TAwaitingLeaveComplete, TTag<CoreNetStates::KNoBearer>)	
	THROUGH_NODEACTIVITY_ENTRY(CoreNetStates::KNoBearer, IpProtoCpr::TSendStoppedAndGoneDown, MeshMachine::TNoTag)
	//Ensure that we reset the iLinkUp flag, otherwise a Start activity blocked against this Stop activity will
	//cause an assertion failure as iLinkUp will be set twice.
	LAST_NODEACTIVITY_ENTRY(KNoTag, IpProtoCpr::TLinkDown)
NODEACTIVITY_END()
}

namespace IPProtoCprActivities
{
DECLARE_DEFINE_ACTIVITY_MAP(activityMap)
	ACTIVITY_MAP_ENTRY(IPProtoCprForwardStateChangeActivity, IPProtoCprForwardStateChange)
	ACTIVITY_MAP_ENTRY(IPProtoCprLinkDown, IPProtoCprLinkDownOnMesg) 
	ACTIVITY_MAP_ENTRY(IPProtoCprProvisionActivity, IPProtoCprProvision)
	ACTIVITY_MAP_ENTRY(IPProtoCprBinderRequestActivity, IPProtoCprBinderRequest)
	ACTIVITY_MAP_ENTRY(IPProtoCprDataMonitoringActivity, IPProtoCprDataMonitoring)
	ACTIVITY_MAP_ENTRY(IPProtoCprConnectionDownActivity, IPProtoCprConnectionDown)
	ACTIVITY_MAP_ENTRY(IPProtoCprOpenCloseRouteActivity, IPProtoCprOpenCloseRoute)
	ACTIVITY_MAP_ENTRY(IPProtoCprStartActivity, IPProtoCprStart)
	ACTIVITY_MAP_ENTRY(IPProtoCprDataClientStatusChangeActivity, IPProtoCprDataClientStatusChangeActivity)
	ACTIVITY_MAP_ENTRY(PRDataClientIdleActivity, PRDataClientIdle)
	ACTIVITY_MAP_ENTRY(IPProtoCprClientLeaveActivity, IPProtoCprClientLeave)
	ACTIVITY_MAP_ENTRY(IPProtoCprIoctlActivity, IPProtoCprIoctl)
    ACTIVITY_MAP_ENTRY(IPProtoCprStopActivity, IPProtoCprStop)
#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	ACTIVITY_MAP_ENTRY(IPProtoCprNotificationActivity, IPProtoCprNotification)
#endif // SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
ACTIVITY_MAP_END_BASE(CprActivities, coreCprActivities)
}

//-=========================================================
//
// CIPProtoConnectionProvider methods
//
//-=========================================================
CIPProtoConnectionProvider* CIPProtoConnectionProvider::NewL(CConnectionProviderFactoryBase& aFactory)
    {
    CIPProtoConnectionProvider* prov = new (ELeave) CIPProtoConnectionProvider(aFactory,IPProtoCprActivities::activityMap::Self());
    CleanupStack::PushL(prov);
    prov->ConstructL();
    CleanupStack::Pop(prov);
    return prov;
    }

CIPProtoConnectionProvider::~CIPProtoConnectionProvider()
    {
    LOG_NODE_DESTROY(KIPProtoCprTag, CIPProtoConnectionProvider);

    CancelTimer();
    delete iTimer;
    
    iNodeLocalExtensions.Close();
    }

CIPProtoConnectionProvider::CIPProtoConnectionProvider(CConnectionProviderFactoryBase& aFactory, const MeshMachine::TNodeActivityMap& aActivityMap) :
	CCoreConnectionProvider(aFactory,aActivityMap),
	ALegacySubConnectionActiveApiExt(this),
	TIfStaticFetcherNearestInHierarchy(this),
    iDataMonitoringConnProvisioningInfo(&iDataVolumes, &iThresholds)
    {
    LOG_NODE_CREATE(KIPProtoCprTag, CIPProtoConnectionProvider);
    }

void CIPProtoConnectionProvider::ConstructL()
    {
    iTimer = COneShotTimer::NewL(ESocketTimerPriority, this);

    ADataMonitoringProvider::ConstructL();
    CCoreConnectionProvider::ConstructL(KIPProtoCPRPreallocatedActivityBufferSize);
    }

void CIPProtoConnectionProvider::ReturnInterfacePtrL(ADataMonitoringProtocolReq*& aInterface)
	{
	aInterface = this;
	}

void CIPProtoConnectionProvider::ReturnInterfacePtrL(MLinkCprApiExt*& aInterface) 	  	 
	{ 	  	 
	//Get the extension from the Access Point Config, it must be there by now (constructed on provision) 	  	 
	//We are the only node ever accessing the interface, this is why we can safely return it as non-const. 	  	 
	CLinkCprExtensionApi* ext = const_cast<CLinkCprExtensionApi*>(static_cast<const CLinkCprExtensionApi*>(AccessPointConfig().FindExtension(CLinkCprExtensionApi::TypeId()))); 	  	 
	ASSERT(ext); //Udeb 	  	 
	User::LeaveIfError(ext? KErrNone : KErrCorrupt); //Urel 	  	 
	aInterface = ext; 	  	 
	}


void CIPProtoConnectionProvider::ReturnInterfacePtrL(ESock::MLegacyControlApiExt*& aInterface)
	{
	aInterface = this;
	}

void CIPProtoConnectionProvider::ReturnInterfacePtrL(ESock::ALegacySubConnectionActiveApiExt*& aInterface)
	{
	aInterface = this;
	}

/**
Retrieves the ALegacyEnumerateSubConnectionsApiExt implementation
*/
void CIPProtoConnectionProvider::ReturnInterfacePtrL(ESock::ALegacyEnumerateSubConnectionsApiExt*& aInterface)
    {
    aInterface = this;
    }


void CIPProtoConnectionProvider::EnumerateSubConnections(CLegacyEnumerateSubConnectionsResponder*& aResponder)
	{
	TInt count = CountClients<TDefaultClientMatchPolicy>(TClientType(TCFClientType::EData, TCFClientType::EStarted));

	/*
	  Plus one for to match legacy behaviour. The extra subconnection is there to
	  represent the connectino and all its subconnections as a whole.

	  So subconnection array is accessed as:
	    [0] = Entire connection
		[1] = Default subconnection
		[2] = non-default subconnection ...
		...
	*/
	count += 1;
	CLegacyEnumerateSubConnectionsResponder::CompleteClient(aResponder, count);
	}

void CIPProtoConnectionProvider::ReceivedL(const TRuntimeCtxId& aSender, const TNodeId& aRecipient, TSignatureBase& aMessage)
    {
	ESOCK_DEBUG_MESSAGE_INTERCEPT(aSender, aMessage, aRecipient);
    TNodeContext<CIPProtoConnectionProvider> ctx(*this, aMessage, aSender, aRecipient);
   	Received(ctx);
    User::LeaveIfError(ctx.iReturn);
	}

void CIPProtoConnectionProvider::Received(MeshMachine::TNodeContextBase& aContext)
    {
    Messages::TNodeSignal::TMessageId noPeerIds[] = {
        TCFFactory::TPeerFoundOrCreated::Id(),
        TCFPeer::TJoinRequest::Id(),
        //TDataMonitoringInternal no-peer as Flow sending directly.
        TCFDataMonitoringNotification::TDataMonitoringNotification::Id(),
        TCFIPProtoMessage::TOpenCloseRoute::Id(),
        Messages::TNodeSignal::TMessageId()
        };

    MeshMachine::AMMNodeBase::Received(noPeerIds, aContext);
	MeshMachine::AMMNodeBase::PostReceived(aContext);
	}

void CIPProtoConnectionProvider::LinkUp()
	{
	ESOCK_EXTLOG_VAR( (KESockConnectionTag, KIPProtoCprSubTag, _L8("CIPProtoConnectionProvider %x:\tLinkUp()"), this) );
	ASSERT(!iLinkUp);
	iLinkUp = ETrue;
	iLastControlClientsCount = ControlClientsCount();

	TTime now;
	now.UniversalTime();
	iStartTime = now;
	}

void CIPProtoConnectionProvider::LinkDown()
	{
	ESOCK_EXTLOG_VAR( (KESockConnectionTag, KIPProtoCprSubTag, _L8("CIPProtoConnectionProvider %x:\tLinkDown()"), this) );

	iLinkUp = EFalse;
	CancelTimer();
	}

void CIPProtoConnectionProvider::OpenRoute()
	{
	if (iTimerExpired)
		{
		return;
		}
	iRouteCount++;
	ESOCK_EXTLOG_VAR( (KESockConnectionTag, KIPProtoCprSubTag, _L8("CIPProtoConnectionProvider %x:\tOpenRoute() count %d"), this, iRouteCount) );
	}

void CIPProtoConnectionProvider::CloseRoute()
	{
	ESOCK_EXTLOG_VAR( (KESockConnectionTag, KIPProtoCprSubTag, _L8("CIPProtoConnectionProvider %x:\tCloseRoute() count %d timer expired: %d"), this, iRouteCount-1, iTimerExpired) );
	if (iTimerExpired)
		{
		return;
		}
	ASSERT(iRouteCount > 0);
	if (--iRouteCount == 0 && !iTimer->IsActive())
		{
		// if the number of calls to CloseRoute() matches those to OpenRoute(), then ensure that
		// the idle timer is running.
		TTimerType newMode;

		if ( (iRouteCountStretchOne)
			&& (iTickThreshold[iTimerMode] != (TInt)KMaxTUint32)
			)
			{
			// Note that there is a slim possiblility that the OpenRoute / CloseRoute event pair
			// occured too quickly and that a TimerComplete event did not occur to check iRouteCount.
			// To account for the OpenRoute / CloseRoute event pair artificially lengthen the iRouteCount.
			// If the current timer is disabled then the OpenRoute / CloseRoute event pair
			// would never have been detected so then dont stretch the event.
			newMode = DecideTimerMode(1);
			}
		else
			{
			newMode = DecideTimerMode(iRouteCount);
			}

		if (newMode != ETimerUnknown)
			SetTimerMode(newMode);
		else
			ResetTimer();
		}

	}

void CIPProtoConnectionProvider::TimerComplete(TInt /*aError*/)
	{
	ESOCK_EXTLOG_VAR( (KESockConnectionTag, KIPProtoCprSubTag, _L8("CIPProtoConnectionProvider %x:\tTimer Complete %d/%d ticks, Mode %d"), this, iExpiredTicks+1, iTickThreshold[iTimerMode], iTimerMode) );

	if (iTimerMode == ETimerImmediate)
		{
		ESOCK_EXTLOG_VAR( (KESockConnectionTag, KIPProtoCprSubTag, _L8("CIPProtoConnectionProvider %x:\tIdle timeout completed - stopping interface"), this) );
		TimerExpired();
		return;
		}

	ASSERT(iLinkUp);

	// reset the iRouteCountStretchOne
	iRouteCountStretchOne = EFalse;

	// Determine if we need to alter the timer mode
	TTimerType newMode = DecideTimerMode(iRouteCount);
	iConnectionControlActivity = EFalse; // (do not reset before DecideTimerMode())

	if (0 == iRouteCount)
		{
		// Note that there is a slim possiblility that the OpenRoute / CloseRoute event pair
		// occured too quickly and that a TimerComplete event did not occur to check iRouteCount.
		// If this occurs then the newMode selected here will be incorrect.
		// Extend iRouteCount if iRouteCount > 0 -> iRouteCount = 0 is seen before the next timer event
		iRouteCountStretchOne = ETrue;
		}

	// Also a similar issue of connection Start/Attach type activity occuring too quickly to
	// be noticed by iConnectionControlActivity might be present.
	// TODO create a test and solution to prove the connection Start/Attach type activity.

	// set new timer mode if required
	if (newMode != ETimerUnknown)
		SetTimerMode(newMode);

	if (iPeriodActivity)
		{
		iPeriodActivity = EFalse;

		// Reset the timer on packet activity (if the timer mode hasn't just been changed).
		// (Should this reset only the Long timer, or should it reset the timer in all modes ?)

		if (newMode == ETimerUnknown)
			{
			ESOCK_EXTLOG_VAR( (KESockConnectionTag, KIPProtoCprSubTag, _L8("CIPProtoConnectionProvider %x:\tTimer reset due to packet activity"), this) );
			ResetTimer();
			}
		}
	else
		{
		if (newMode == ETimerUnknown)
			{
			// No Activity and no change in timer state, check if timer has expired
			// (checking first for a value of KMaxTUint32, which means the timer is disabled).

			if (iTickThreshold[iTimerMode] != (TInt)KMaxTUint32)
				{
				if (iTickThreshold[iTimerMode] <= ++iExpiredTicks)
					{
					ESOCK_EXTLOG_VAR( (KESockConnectionTag, KIPProtoCprSubTag, _L8("CIPProtoConnectionProvider %x:\tIdle timeout completed"), this) );
					TimerExpired();
					}
				else
					{
					StartNextTick();
					}
				}
			}
		}
	}

CIPProtoConnectionProvider::TTimerType CIPProtoConnectionProvider::DecideTimerMode(TInt aRouteCount)
	{
	TTimerType newMode = ETimerUnknown;
	TInt currentControlClientsCount = ControlClientsCount();
	if (currentControlClientsCount > iLastControlClientsCount)
		{
		iConnectionControlActivity = ETrue;
		}
	ESOCK_EXTLOG_VAR( (KESockConnectionTag, KIPProtoCprSubTag, _L8("CIPProtoConnectionProvider %x:\tDecideTimerMode() currentControlClientsCount %d iLastControlClientsCount %d iRouteCount %d"), this, currentControlClientsCount, iLastControlClientsCount, iRouteCount) );
	iLastControlClientsCount = currentControlClientsCount;

	switch (iTimerMode)
		{
	case ETimerShort:
		if (aRouteCount > 0) // any Flows or ESock Sessions?
			{
			ESOCK_EXTLOG_VAR( (KESockConnectionTag, KIPProtoCprSubTag, _L8("CIPProtoConnectionProvider %x:\tTimer mode changed from Short to Long due to presence of protocol flows"), this) );
			newMode = ETimerLong;
			}
		else if (iLastControlClientsCount > 0) // any new Control Clients attached?
			{
			ESOCK_EXTLOG_VAR( (KESockConnectionTag, KIPProtoCprSubTag, _L8("CIPProtoConnectionProvider %x:\tTimer mode changed from Short to Medium due to presence of control providers"), this) );
			newMode = ETimerMedium;
			}
		break;

	case ETimerMedium:
		if (aRouteCount > 0)
			{
			ESOCK_EXTLOG_VAR( (KESockConnectionTag, KIPProtoCprSubTag, _L8("CIPProtoConnectionProvider %x:\tTimer mode changed from Medium to Long due to presence of flows"), this) );
			newMode = ETimerLong;
			}
		else if (iLastControlClientsCount == 0)
			{
			ESOCK_EXTLOG_VAR( (KESockConnectionTag, KIPProtoCprSubTag, _L8("CIPProtoConnectionProvider %x:\tTimer mode changed from Medium to Short due to absence of flows and control providers"), this) );
			newMode = ETimerShort;
			}
		else if (iConnectionControlActivity && iTickThreshold[iTimerMode] != (TInt)KMaxTUint32)
			{
			// there has been connection Start/Attach type activity, so reset medium timer
			ESOCK_EXTLOG_VAR( (KESockConnectionTag, KIPProtoCprSubTag, _L8("CIPProtoConnectionProvider %x:\tMedium timer reset due to connection control activity"), this) );
			newMode = ETimerMedium;
			}
		else if (0 == aRouteCount && iTickThreshold[iTimerMode] == (TInt)KMaxTUint32 && iTickThreshold[ETimerShort] != (TInt)KMaxTUint32)
			{
			// There are no sockets and the current timer is disabled but the short timer is set
			ESOCK_EXTLOG_VAR( (KESockConnectionTag, KIPProtoCprSubTag, _L8("CIPProtoConnectionProvider %x:\tTimer set to Short because there are no Sockets and Long or Medium timer is disabled"), this) );
			newMode = ETimerShort;
			}
		break;

	case ETimerLong:
		if (0 == aRouteCount)
			{
			if (iLastControlClientsCount > 0)
				{
				ESOCK_EXTLOG_VAR( (KESockConnectionTag, KIPProtoCprSubTag, _L8("CIPProtoConnectionProvider %x:\tTimer mode changed from Long to Medium due to presence of control providers"), this) );
				newMode = ETimerMedium;
				}
			else
				{
				ESOCK_EXTLOG_VAR( (KESockConnectionTag, KIPProtoCprSubTag, _L8("CIPProtoConnectionProvider %x:\tTimer mode changed from Long to Short due to absence of flows and control providers"), this) );
				newMode = ETimerShort;
				}
			}
		break;

	default:
		break;
		}

	return (newMode);
	}

/**
 Start the next one second tick for the Idle Timer.
 As the Idle Timer is implemented as a repeated one second timeout rather than a single timeout period,
 this routine is necessary to ensure overall accuracy.  The period of each "one second" tick
 is adjusted slightly to compensate for any accumulated inaccuracies.
 */
void CIPProtoConnectionProvider::StartNextTick()
	{
	/*
	The inactivity timeout period is made up of a number of successive
	one second timer periods up to the desired inactivity timeout.
	This is done because certain operations are needed every second.
	However, this can result in cumulative errors in the final timeout
	period.  An attempt is made here to keep the final timeout period
	accurate by adjusting the duration of a timer tick every so often
	to compensate for any observed drift.  This is only best-effort
	synchronisation which ignores drift that seems way out - as could
	happen if user altered system time, for example,
	*/

	if (iExpiredTicks % KTimerCorrectionPeriod == 0)
		{
		TTime currentTime;
		currentTime.HomeTime();

		// Time interval for timer synch & high limit for validity of observed timer drift
		const TTimeIntervalMicroSeconds KTimeCheckInterval(KTimerCorrectionPeriod * KTimerTick);

		iDriftCheckTime += KTimeCheckInterval;

		// Only act on latest timer drift if it's within sensible limits.
		// Might not be if user has reset system time, for example.
		if ( currentTime > iDriftCheckTime )
			{
			TInt64 t = currentTime.MicroSecondsFrom(iDriftCheckTime).Int64();
			if ( t < KTimeCheckInterval.Int64())
				iTotalTimerDrift += I64LOW(t);
			}
		else
			{
			TInt64 t = iDriftCheckTime.MicroSecondsFrom(currentTime).Int64();
 			if (t < KTimerTick-KMinTimerTick)
				iTotalTimerDrift -= I64LOW(t);
			}

		iDriftCheckTime = currentTime;

		if (iTotalTimerDrift > KTimerTick - KMinTimerTick)
			TimerAfter(KMinTimerTick);
		else if (iTotalTimerDrift > 0)
			TimerAfter(KTimerTick - iTotalTimerDrift);
		else
			TimerAfter(KTimerTick);
		}
	else
		TimerAfter(KTimerTick);
	}

void CIPProtoConnectionProvider::SetTimers(TUint32 aShortTimer, TUint32 aMediumTimer, TUint32 aLongTimer)
	{
	ESOCK_EXTLOG_VAR( (KESockConnectionTag, KIPProtoCprSubTag, _L8("CIPProtoConnectionProvider %x:\tSetTimers(aShortTimer %ds, aMediumTimer %ds, aLongTimer %ds)"), this, aShortTimer, aMediumTimer, aLongTimer) );

	// Commsdat field: LAST_SESSION_CLOSED_TIMEOUT
	iTickThreshold[ETimerShort] = aShortTimer;
	// Commsdat field: LAST_SOCKET_CLOSED_TIMEOUT
	iTickThreshold[ETimerMedium] = aMediumTimer;
	// Commsdat field: LAST_SOCKET_ACTIVITY_TIMEOUT
	iTickThreshold[ETimerLong] = aLongTimer;
	iTickThreshold[ETimerImmediate] = 0;
	}

void CIPProtoConnectionProvider::ResetTimer()
    {
	/**
	Restart the Idle Timer.
	Used when switching the timer into a different mode of operation.
	*/

    // Initial the iRouteCountStretchOne boolean
    // Extend iRouteCount if a iRouteCount > 0 is seen before the next timer event
    iRouteCountStretchOne = ETrue;

#ifdef ESOCK_EXTLOG_ACTIVE
	TBuf8<9> mode;	// enough for "Immediate"
	TInt len(0);
	switch(iTimerMode)
		{
		case ETimerLong:
			mode = _L8("Long");
			len = iTickThreshold[ETimerLong];
			break;

		case ETimerMedium:
			mode = _L8("Medium");
			len = iTickThreshold[ETimerMedium];
			break;

		case ETimerShort:
			mode = _L8("Short");
			len = iTickThreshold[ETimerShort];
			break;

		case ETimerImmediate:
			mode = _L8("Immediate");
			break;

		default:
			mode = _L8("Unknown");
		}
	ESOCK_EXTLOG_VAR( (KESockConnectionTag, KIPProtoCprSubTag, _L8("CIPProtoConnectionProvider %x:\tTimer mode set to %S (%d ticks)"), this, &mode, len) );
#endif

	// if we are not in the packet activity monitoring mode, reset the activity flag.
	if (iTimerMode != ETimerLong)
		{
		iPeriodActivity = EFalse;
		}

	if ( !iLinkUp || iTimer->IsActive() )

		{
		return;
		}

	iExpiredTicks = 0;
	iTotalTimerDrift = 0;
	iDriftCheckTime.HomeTime();
	// Only start the timer if it is not disabled (i.e. KMaxTUint32)
	// Defensive check against ETimerUnknown.
	if ( iTimerMode != ETimerUnknown && iTickThreshold[iTimerMode] != (TInt)KMaxTUint32 )
		{
		TimerAfter(KTimerTick);
		}
	}

void CIPProtoConnectionProvider::DisableTimers()
	{
	if(0 == iTimerDisableCount)
		{
		CancelTimer();
		}
	iTimerDisableCount++;
	}

void CIPProtoConnectionProvider::EnableTimers()
	{
	--iTimerDisableCount;
	if(0 == iTimerDisableCount)
		{
		ResetTimer();
		}
	}

void CIPProtoConnectionProvider::CancelTimer()
    {
    if (iTimer)
    	{
    	iTimer->Cancel();
    	}
    }

void CIPProtoConnectionProvider::StopConnection()
	{
	if (!iTimerExpired)
		{
		iTimerExpired = ETrue;
		CancelTimer();
		if (CountActivities(ECFActivityStop) == 0 &&
+		    CountActivities(ECFActivityDestroy) == 0)
			{
			RClientInterface::OpenPostMessageClose(Id(), TNodeCtxId(ECFActivityStop, Id()), TCFServiceProvider::TStop(KErrTimedOut).CRef());
			}
		}
	}

void CIPProtoConnectionProvider::TimerExpired()
	{
	ESOCK_EXTLOG_VAR( (KESockConnectionTag, KIPProtoCprSubTag, _L8("CIPProtoConnectionProvider %x:\tTimerExpired()"), this) );
	StopConnection();
	}

TInt CIPProtoConnectionProvider::ControlClientsCount()
	{
	return CountClients<TDefaultClientMatchPolicy>(
		TClientType(TCFClientType::ECtrl),
		TClientType(TCFClientType::ECtrl, TCFClientType::EMonitor)
		);
	}

void CIPProtoConnectionProvider::SetUsageProfile(TInt aProfile)
	{
	ESOCK_EXTLOG_VAR( (KESockConnectionTag, KIPProtoCprSubTag, _L8("CIPProtoConnectionProvider %x:\tSetUsageProfile(%d)"), this, aProfile) );

	TInt currentControlClientsCount = ControlClientsCount();

	switch (aProfile)
		{
	case KConnProfileMedium:
		if (currentControlClientsCount == 0)
			{
			// Move from short to medium timer
			if (iTimerMode == ETimerShort)
				{
				ESOCK_EXTLOG_VAR( (KESockConnectionTag, KIPProtoCprSubTag, _L8("CIPProtoConnectionProvider %x:\tUsage profile %d - timer mode set to Medium"), this, aProfile) );
				SetTimerMode(ETimerMedium);
				}
			}
		break;

	case KConnProfileNone:
		if (currentControlClientsCount == 0 && iRouteCount == 0)
			{
			if (iTimerMode == ETimerMedium)
				{
				// Moving from medium to short timer
				ESOCK_EXTLOG_VAR( (KESockConnectionTag, KIPProtoCprSubTag, _L8("CIPProtoConnectionProvider %x:\tUsage profile %d - timer mode set to Short"), this, aProfile) );
				SetTimerMode(ETimerShort);
				}
			else if ((iTimerMode == ETimerUnknown) && !iLinkUp)
				{
				ESOCK_EXTLOG_VAR( (KESockConnectionTag, KIPProtoCprSubTag, _L8("CIPProtoConnectionProvider %x:\tUsage profile %d - stopping interface"), this, aProfile) );
				}
			}

		break;

	default:
		ASSERT(0);
		}
	}

void CIPProtoConnectionProvider::SetTimerMode(TTimerType aTimerMode)
	{
	iTimerMode = aTimerMode;
	ResetTimer();
	};

void CIPProtoConnectionProvider::TimerAfter(TInt aInterval)
	{
	ASSERT(iTimer);
	iTimer->After(aInterval);
	}

TInt CIPProtoConnectionProvider::ControlL(TUint aOptionLevel, TUint aOptionName, TDes8& aOption, MPlatsecApiExt* aPlatsecItf)
	{
	switch(aOptionLevel)
		{
#ifdef _DEBUG
		// We're only servicing 'testing' control options here.  They used
		// to be serviced by Dummy NIF but now the idle timers are
		// associated with the IPProto layer rather than the link layer.
	case KCOLInterface:
		if (aOption.Length() != sizeof(TInt))
			{
			return KErrArgument;
			}
		switch(aOptionName)
			{
		case KTestSoDummyNifSetLastSessionClosedTimeout:
			iTickThreshold[ETimerShort] = *(reinterpret_cast<const TInt*>(aOption.Ptr()));
			break;

		case KTestSoDummyNifSetLastSocketClosedTimeout:
			iTickThreshold[ETimerMedium] = *(reinterpret_cast<const TInt*>(aOption.Ptr()));
			break;

		case KTestSoDummyNifSetLastSocketActivityTimeout:
			iTickThreshold[ETimerLong] = *(reinterpret_cast<const TInt*>(aOption.Ptr()));
			break;

		default:
			return KErrNotSupported;
			}
		break;
#endif // _DEBUG

	case KCOLProvider:
		switch(aOptionName)
			{
		case KConnDisableTimers:
			{
			if(!aPlatsecItf->HasCapability(ECapabilityNetworkControl))
				{
				return KErrPermissionDenied;
				}

			if (aOption.Length() != sizeof(TBool))
				{
				return KErrArgument;
				}

			TBool disable = *reinterpret_cast<const TBool*>(aOption.Ptr());
			if(disable)
				{
				DisableTimers();
				}
			else
				{
				EnableTimers();
				}
			break;
			}
		case KConnGetInterfaceName:
			{
			if(aOption.Length() != sizeof(TConnInterfaceName))
				{
				return KErrArgument;
				}

 			TConnInterfaceName* connItfName;
			connItfName = reinterpret_cast<TConnInterfaceName*>(const_cast<TUint8*>(aOption.Ptr()));

			XInterfaceNames* itfNames = static_cast<XInterfaceNames*>(const_cast<Meta::SMetaData*>(AccessPointConfig().FindExtension(XInterfaceNames::TypeId())));

			if(!itfNames)
				{
				return KErrNotFound;
				}

			// Interface indices are 1-based we so perform subtract 1 to get the correct
			// name from the store.
			TUint ret = itfNames->InterfaceName(connItfName->iIndex - 1, connItfName->iName);

			return ret;
			}
		default:
			return KErrNotSupported;
			}
		break;

	default:
		return KErrNotSupported;
		}

	return KErrNone;
	}

void CIPProtoConnectionProvider::ForceCheckShortTimerMode()
/**
 * This method allow to force to check if it's possible to switch
 * 	to the TimerMode "Short" without waiting for the next tick.
 *
 * The "IdleTimer" inside IPProtoCpr can work in 3 different TimerMode:
 * 	1) Short 2) Medium 3) Long.
 * Depending on the Number of CtrlClients attached and on the
 * 	Activity in the lower planes, the Timer change is mode.
 * A problem poped-out when there are no more CtrlClient and there is no
 * 	Activity: the timer needs to switch to "Short" and, if nothing happens
 * 	in the meanwhile that the Short timeout finish, send a "StopSelf" message.
 * BUT this is based on the "count" of the number of CtrlClient and this
 * 	"count" is done ONLY every Tick. That, in this case, has 1 second freq.
 * This means that in many situation the Timer lose almost 1 second BEFORE
 * 	to recognize that it has to switch to Short mode.
 *
 * This method allow to "force" the check to see if it's the right moment
 * 	to switch to Short mode and, if it is the case, it does so.
 * We overriden the activity "ClientLeaveActivity" so this method is
 * 	called when a "ClientLeave" message is processed by this node.
 *
 * It's clear that this is just a patch: the whole timer needs a refactoring.
 */
	{
	// If the number of ControlClient goes to "0", it needs to
	// 	force to switch the TimerMode to "ShortTimeout".
	// This is to avoid to waste *1 Tick* (waiting for the next one)
	//	to recognize that the number of ControlClient is "0".
	//
	// We do exactly what it's done when a Tick is complete:
	//	this will switch the Mode to Short Timeout.
	if ( iLastControlClientsCount >= 1 &&	// - If there was at least 1 Control Clients
			iTimerMode == ETimerMedium &&	// - AND the TimerMode is on Medium
			iTimerExpired == EFalse &&		// - AND The time is not ALREADY Expired
			ControlClientsCount() == 0		// - AND There are no more Control Clients Attached
			)
		{
		ESOCK_EXTLOG_VAR( (KESockConnectionTag, KIPProtoCprSubTag, _L8("CIPProtoConnectionProvider %x:\tTimer mode FORCED to Short due to absence of flows and control providers"), this) );
		iLastControlClientsCount = 0;
		TTimerType newMode = ETimerShort;

		iConnectionControlActivity = EFalse;

		iTimerMode = newMode;
		iPeriodActivity = EFalse;
		iExpiredTicks = 0;
		iTotalTimerDrift = 0;
		iDriftCheckTime.HomeTime();

		// The timer may never have been started if no sockets were ever opened and the "medium"
		// and "long" timers were infinite. So we make sure that it is running.
		ResetTimer();
		}
	}

void CIPProtoConnectionProvider::GetSubConnectionInfo(TSubConnectionInfo &aInfo)
	{
	aInfo.iTimeStarted = iStartTime;
	}

//
// CIPProtoConnectionProvider::COneShotTimer
//
CIPProtoConnectionProvider::COneShotTimer* CIPProtoConnectionProvider::COneShotTimer::NewL(TInt aPriority, CIPProtoConnectionProvider* aOwner)
	{
	COneShotTimer* self = new (ELeave)COneShotTimer(aPriority, aOwner);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
CIPProtoConnectionProvider::COneShotTimer::COneShotTimer(TInt aPriority, CIPProtoConnectionProvider* aOwner)
	: CTimer(aPriority), iOwner(aOwner)
	{
	CActiveScheduler::Add(this);
	}

void CIPProtoConnectionProvider::COneShotTimer::RunL()
	{
	iOwner->TimerComplete(iStatus.Int());
	}

void CIPProtoConnectionProvider::COneShotTimer::ConstructL()
	{
	CTimer::ConstructL();
	}

