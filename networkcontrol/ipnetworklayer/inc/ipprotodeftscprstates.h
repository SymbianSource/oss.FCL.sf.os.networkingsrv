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
// ipprotodeftscpr.h
// 
//

#if !defined(IPPROTODEFTSCPRSTATES_H_DEFINED)
#define IPPROTODEFTSCPRSTATES_H_DEFINED

#include <comms-infras/corescprstates.h>
#include "ipprotomessages.h"

class CIPProtoSubConnectionProviderBase;

//-=========================================================
//
//Generic States & Transitions
//
//-=========================================================

namespace IPProtoDeftSCpr
{
typedef MeshMachine::TNodeContext<CIPProtoDeftSubConnectionProvider, SCprStates::TContext> TContext;

const TInt KStartNetCfgExt     = 1;
const TInt KStopNetCfgExt      = 2;
const TInt KTryServiceProvider = 3;
const TInt KTryNetCfgExt       = 4;
const TInt KConfigureNetwork   = 5;
const TInt KNetworkConfigured  = 6;
const TInt KSwallowMessage     = 7;
const TInt KDaemonReleased    = 8;
const TInt KDaemonReleasedStateChanged    = 9;


enum TCFNodeActivityId
	{
	EActivityDataMonitoring,
	EActivityAgentEvent
	};

typedef MeshMachine::TAwaitingMessageState<ESock::TCFServiceProvider::TStarted> TAwaitingStarted;


//-=========================================================
//	Provisioning support
//-=========================================================
DECLARE_SMELEMENT_HEADER(TStoreProvision, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, IPProtoDeftSCpr::TContext)
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER(TStoreProvision)

//-=========================================================
//	Data monitoring support
//-=========================================================
DECLARE_SMELEMENT_HEADER(TAwaitingDataMonitoringNotification, MeshMachine::TState<TContext>, NetStateMachine::MState, IPProtoDeftSCpr::TContext)
	virtual TBool Accept();
DECLARE_SMELEMENT_FOOTER(TAwaitingDataMonitoringNotification)

DECLARE_SMELEMENT_HEADER(TProcessDataMonitoringNotification, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, IPProtoDeftSCpr::TContext)
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER(TProcessDataMonitoringNotification)







DECLARE_SMELEMENT_HEADER(TNoTagOrProviderStopped, MeshMachine::TStateFork<TContext>, NetStateMachine::MStateFork,IPProtoDeftSCpr::TContext)
    virtual TInt TransitionTag();
DECLARE_SMELEMENT_FOOTER(TNoTagOrProviderStopped)
   
DECLARE_SMELEMENT_HEADER(TNoTagOrProviderStoppedOrDaemonReleased, MeshMachine::TStateFork<TContext>, NetStateMachine::MStateFork, IPProtoDeftSCpr::TContext)
    virtual TInt TransitionTag();
DECLARE_SMELEMENT_FOOTER(TNoTagOrProviderStoppedOrDaemonReleased)

DECLARE_SMELEMENT_HEADER( TNoTagBackwardsOrProviderStopped, MeshMachine::TStateFork<TContext>, NetStateMachine::MStateFork, IPProtoDeftSCpr::TContext )
	virtual TInt TransitionTag();
DECLARE_SMELEMENT_FOOTER( TNoTagBackwardsOrProviderStopped )


//-=========================================================
// Ioctl Support
//-=========================================================

DECLARE_SMELEMENT_HEADER( TAwaitingIoctlProcessed, MeshMachine::TState<TContext>, NetStateMachine::MState, IPProtoDeftSCpr::TContext )
	virtual TBool Accept();
	virtual void Cancel();
DECLARE_SMELEMENT_FOOTER( TAwaitingIoctlProcessed )

DECLARE_SMELEMENT_HEADER( TNoTagOrTryNetCfgExt, MeshMachine::TStateFork<TContext>, NetStateMachine::MStateFork, TContext)
	virtual TInt TransitionTag();
DECLARE_SMELEMENT_FOOTER( TNoTagOrTryNetCfgExt )

DECLARE_SMELEMENT_HEADER( TTryServiceProviderOrTryNetCfgExt, MeshMachine::TStateFork<TContext>, NetStateMachine::MStateFork, TContext )
	virtual TInt TransitionTag();
DECLARE_SMELEMENT_FOOTER( TTryServiceProviderOrTryNetCfgExt )

DECLARE_SMELEMENT_HEADER( TForwardToServiceProvider, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, IPProtoDeftSCpr::TContext)
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TForwardToServiceProvider )

DECLARE_SMELEMENT_HEADER( THandoffToNetCfgExt, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, IPProtoDeftSCpr::TContext)
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( THandoffToNetCfgExt )

//-=========================================================
// NetCfgExtension Support
//-=========================================================
DECLARE_SMELEMENT_HEADER( TAwaitingStateChangeOrCancel, MeshMachine::TState<TContext>, NetStateMachine::MState, IPProtoDeftSCpr::TContext )
	virtual TBool Accept();
DECLARE_SMELEMENT_FOOTER( TAwaitingStateChangeOrCancel )

DECLARE_SMELEMENT_HEADER( TAwaitingStateChange, MeshMachine::TState<TContext>, NetStateMachine::MState, IPProtoDeftSCpr::TContext )
    virtual TBool Accept();
DECLARE_SMELEMENT_FOOTER( TAwaitingStateChange )

DECLARE_SMELEMENT_HEADER( TAwaitingConfigureNetwork, MeshMachine::TState<TContext>, NetStateMachine::MState, IPProtoDeftSCpr::TContext )
	virtual TBool Accept();
DECLARE_SMELEMENT_FOOTER( TAwaitingConfigureNetwork )

DECLARE_SMELEMENT_HEADER( TAwaitingNetworkConfiguredOrError, MeshMachine::TState<TContext>, NetStateMachine::MState, IPProtoDeftSCpr::TContext )
	virtual TBool Accept();
DECLARE_SMELEMENT_FOOTER( TAwaitingNetworkConfiguredOrError )

DECLARE_SMELEMENT_HEADER(TNoTagOrSwallowMessage, MeshMachine::TStateFork<TContext>, NetStateMachine::MStateFork, TContext)
	virtual TInt TransitionTag();
DECLARE_SMELEMENT_FOOTER(TNoTagOrSwallowMessage)

DECLARE_SMELEMENT_HEADER( TStopNetCfgExtOrNoTag, MeshMachine::TStateFork<TContext>, NetStateMachine::MStateFork, TContext)
	virtual TInt TransitionTag();
DECLARE_SMELEMENT_FOOTER( TStopNetCfgExtOrNoTag )

DECLARE_SMELEMENT_HEADER( TNetworkConfiguredOrErrorTagOrCancelTagOrNoTag, MeshMachine::TStateFork<TContext>, NetStateMachine::MStateFork, TContext)
	virtual TInt TransitionTag();
DECLARE_SMELEMENT_FOOTER( TNetworkConfiguredOrErrorTagOrCancelTagNoTag )

DECLARE_SMELEMENT_HEADER( TNetworkConfiguredOrErrorTagOrCancelTagOrNoTagBackward, IPProtoDeftSCpr::TNetworkConfiguredOrErrorTagOrCancelTagOrNoTag, NetStateMachine::MStateFork, TContext)
	virtual TInt TransitionTag();
DECLARE_SMELEMENT_FOOTER( TNetworkConfiguredOrErrorTagOrCancelTagOrNoTagBackward )

DECLARE_SMELEMENT_HEADER( TErrorTagOrNoTag, MeshMachine::TStateFork<TContext>, NetStateMachine::MStateFork, TContext)
	virtual TInt TransitionTag();
DECLARE_SMELEMENT_FOOTER( TErrorTagOrNoTag )

DECLARE_SMELEMENT_HEADER( TNoTagOrConfigureNetwork, MeshMachine::TStateFork<TContext>, NetStateMachine::MStateFork, TContext)
	virtual TInt TransitionTag();
DECLARE_SMELEMENT_FOOTER( TNoTagOrConfigureNetwork )

DECLARE_SMELEMENT_HEADER( TConfigureNetwork, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, IPProtoDeftSCpr::TContext)
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TConfigureNetwork )

DECLARE_SMELEMENT_HEADER( TSendNetworkConfigured, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, IPProtoDeftSCpr::TContext)
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TSendNetworkConfigured )

DECLARE_SMELEMENT_HEADER(TStartNetCfgExt, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, IPProtoDeftSCpr::TContext)
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER(TStartNetCfgExt)

DECLARE_SMELEMENT_HEADER(TStopNetCfgExt, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, IPProtoDeftSCpr::TContext)
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER(TStopNetCfgExt)

DECLARE_SMELEMENT_HEADER(TResetSentTo, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, IPProtoDeftSCpr::TContext)
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER(TResetSentTo)

DECLARE_SMELEMENT_HEADER( TDaemonReleasedStateChangedOrNoTag, MeshMachine::TStateFork<TContext>, NetStateMachine::MStateFork, TContext)
    virtual TInt TransitionTag();
DECLARE_SMELEMENT_FOOTER( TDaemonReleasedStateChangedOrNoTag )

DECLARE_SMELEMENT_HEADER( TDaemonReleasedStateChangedOrNoTagBackward, IPProtoDeftSCpr::TDaemonReleasedStateChangedOrNoTag, NetStateMachine::MStateFork, TContext)
    virtual TInt TransitionTag();
DECLARE_SMELEMENT_FOOTER( TDaemonReleasedStateChnagedOrNoTagBackward )
 
DECLARE_SMELEMENT_HEADER(TStopNetCfgExtDelete, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, IPProtoDeftSCpr::TContext)
    virtual void DoL();
DECLARE_SMELEMENT_FOOTER(TStopNetCfgExtDelete)

DECLARE_AGGREGATED_TRANSITION2(
	TStartNetCfgExtAndResetSentTo,
	IPProtoDeftSCpr::TStartNetCfgExt,
	IPProtoDeftSCpr::TResetSentTo
	)

DECLARE_AGGREGATED_TRANSITION2(
	TForwardToControlProviderAndResetSentTo,
	CoreNetStates::TForwardToControlProvider,
	IPProtoDeftSCpr::TResetSentTo
	)

DECLARE_AGGREGATED_TRANSITION2(
	TForwardToControlProviderAndStopNetCfgExt,
	CoreNetStates::TForwardToControlProvider,
	IPProtoDeftSCpr::TStopNetCfgExt
	)




DECLARE_AGGREGATED_TRANSITION2(
	TStopNetCfgExtAndRaiseActivityError,
	IPProtoDeftSCpr::TStopNetCfgExt,
	MeshMachine::TRaiseActivityError
	)

// Support for Agent events received from AgentSCPr to be forwarded to NetCfgExt
DECLARE_SMELEMENT_HEADER( TAwaitingAgentEventNotification, MeshMachine::TState<TContext>, NetStateMachine::MState, IPProtoDeftSCpr::TContext )
	virtual TBool Accept();
DECLARE_SMELEMENT_FOOTER( TAwaitingAgentEventNotification )

DECLARE_SMELEMENT_HEADER( TProcessAgentEvent, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, IPProtoDeftSCpr::TContext)
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TProcessAgentEvent )


//-=========================================================
DECLARE_SMELEMENT_HEADER( TNoTagOrParamsPresent, MeshMachine::TStateFork<TContext>, NetStateMachine::MStateFork, TContext)
	virtual TInt TransitionTag();
DECLARE_SMELEMENT_FOOTER( TNoTagOrParamsPresent )

DECLARE_SMELEMENT_HEADER( TSetParams, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, IPProtoDeftSCpr::TContext )
	virtual void DoL();
    RParameterFamilyBundle& GetBundleL(TContextId aContextId);
DECLARE_SMELEMENT_FOOTER( TSetParams )




class CSipAddressActivity : public MeshMachine::CNodeActivityBase
/**
Custom activity to store the sip ioctl when it is received,
to be used by later transitions.
@internal
@prototype
*/
	{
public:
    static MeshMachine::CNodeActivityBase* NewL(const MeshMachine::TNodeActivity& aActivitySig, MeshMachine::AMMNodeBase& aNode)
    	{
		return new (ELeave) CSipAddressActivity(aActivitySig,aNode);
    	}

	void SetIoctlMessage(const RMessage2& aIoctlMsg) { iIoctlMsg = aIoctlMsg; }
	const RMessage2& IoctlMessage() const { return iIoctlMsg; }
	void CompleteIoctlMessage(const TInt err)
			{
			if(!iIoctlMsg.IsNull())
				iIoctlMsg.Complete(err);
			SetError(KErrNone);
			}

protected:
	CSipAddressActivity(const MeshMachine::TNodeActivity& aActivitySig, MeshMachine::AMMNodeBase& aNode)
		: MeshMachine::CNodeActivityBase(aActivitySig, aNode)
		{
		}

	virtual ~CSipAddressActivity() {}

private:
	RMessage2 iIoctlMsg;
	};

} // namespace IPProtoDeftSCpr

#endif // IPPROTODEFTSCPRSTATES_H_DEFINED
