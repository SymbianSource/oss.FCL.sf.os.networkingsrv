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
// ipprotoscpr.h
// 
//

#if !defined(IPPROTOSCPRSTATES_H_DEFINED)
#define IPPROTOSCPRSTATES_H_DEFINED

#include <comms-infras/corescprstates.h>
#include "ipprotomessages.h"

class CIPQoSProtoSubConnectionProviderBase;

//-=========================================================
//
//Generic States & Transitions
//
//-=========================================================

namespace IPProtoSCpr
{
typedef MeshMachine::TNodeContext<CIPProtoSubConnectionProvider, SCprStates::TContext> TContext;

const TInt KStartNetCfgExt     = 1;
const TInt KStopNetCfgExt      = 2;
//const TInt KTryServiceProvider = 3;
const TInt KTryNetCfgExt       = 4;
const TInt KConfigureNetwork   = 5;
const TInt KNetworkConfigured  = 6;
const TInt KSwallowMessage     = 7;
const TInt KCancelIoctl        = 8;

enum TCFNodeActivityId
	{
	EActivityDataMonitoring,
	EActivityAgentEvent
	};

typedef MeshMachine::TAwaitingMessageState<ESock::TCFServiceProvider::TStarted> TAwaitingStarted;

#ifndef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
DECLARE_SMELEMENT_HEADER( TSendParamsToServiceProvider, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, IPProtoSCpr::TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TSendParamsToServiceProvider )
#endif // SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW

//-=========================================================
//	Provisioning support
//-=========================================================
DECLARE_SMELEMENT_HEADER(TStoreProvision, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, IPProtoSCpr::TContext)
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER(TStoreProvision)

//-=========================================================
//	Data monitoring support
//-=========================================================
DECLARE_SMELEMENT_HEADER(TAwaitingDataMonitoringNotification, MeshMachine::TState<TContext>, NetStateMachine::MState, IPProtoSCpr::TContext)
	virtual TBool Accept();
DECLARE_SMELEMENT_FOOTER(TAwaitingDataMonitoringNotification)

DECLARE_SMELEMENT_HEADER(TProcessDataMonitoringNotification, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, IPProtoSCpr::TContext)
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER(TProcessDataMonitoringNotification)


//-=========================================================
//	AddressUpdate
//-=========================================================
DECLARE_SMELEMENT_HEADER( TAwaitingAddressUpdate, MeshMachine::TState<TContext>, NetStateMachine::MState, TContext )
	virtual TBool Accept();
DECLARE_SMELEMENT_FOOTER( TAwaitingAddressUpdate )

DECLARE_SMELEMENT_HEADER( TAddIpAddressInfo, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TAddIpAddressInfo )

DECLARE_SMELEMENT_HEADER( TSendDataClientRoutedToFlow, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TSendDataClientRoutedToFlow )


//-=========================================================

DECLARE_SMELEMENT_HEADER(TNoTagOrProviderStopped, MeshMachine::TStateFork<TContext>, NetStateMachine::MStateFork, TContext)
	virtual TInt TransitionTag();
DECLARE_SMELEMENT_FOOTER(TNoTagOrProviderStopped)

DECLARE_SMELEMENT_HEADER( TNoTagBackwardsOrProviderStopped, MeshMachine::TStateFork<TContext>, NetStateMachine::MStateFork, IPProtoSCpr::TContext )
	virtual TInt TransitionTag();
DECLARE_SMELEMENT_FOOTER( TNoTagBackwardsOrProviderStopped )

// Support for Agent events received from AgentSCPr to be forwarded to NetCfgExt
DECLARE_SMELEMENT_HEADER( TAwaitingAgentEventNotification, MeshMachine::TState<TContext>, NetStateMachine::MState, IPProtoSCpr::TContext )
	virtual TBool Accept();
DECLARE_SMELEMENT_FOOTER( TAwaitingAgentEventNotification )

DECLARE_SMELEMENT_HEADER( TProcessAgentEvent, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, IPProtoSCpr::TContext)
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TProcessAgentEvent )

DECLARE_SMELEMENT_HEADER(TDeleteAllContexts, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, IPProtoSCpr::TContext)
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER(TDeleteAllContexts)


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

} // namespace IPProtoSCpr

#endif // IPPROTOSCPRSTATES_H_DEFINED
