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
 
#ifndef IPPROTOCPRSTATES_H_
#define IPPROTOCPRSTATES_H_

#include <comms-infras/corecprstates.h>

#include "IPProtoCPR.h"

namespace IpProtoCpr
{
typedef MeshMachine::TNodeContext<CIPProtoConnectionProvider, CprStates::TContext> TContext;
//-=========================================================
//
// States
//
//-=========================================================
typedef MeshMachine::TAcceptErrorState<CoreNetStates::TAwaitingDataClientStarted> TAwaitingDataClientStartedOrError;

DECLARE_SMELEMENT_HEADER( TStoreProvision, MeshMachine::TStateTransition<IpProtoCpr::TContext>, NetStateMachine::MStateTransition, IpProtoCpr::TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TStoreProvision )

DECLARE_SMELEMENT_HEADER( TSendStoppedAndGoneDown, MeshMachine::TStateTransition<IpProtoCpr::TContext>, NetStateMachine::MStateTransition, IpProtoCpr::TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TSendStoppedAndGoneDown )

DECLARE_SMELEMENT_HEADER( TProvisionActivation, MeshMachine::TStateTransition<IpProtoCpr::TContext>, NetStateMachine::MStateTransition, IpProtoCpr::TContext )
    virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TProvisionActivation )

DECLARE_SMELEMENT_HEADER( THandleProvisionError, MeshMachine::TStateTransition<IpProtoCpr::TContext>, NetStateMachine::MStateTransition, IpProtoCpr::TContext )
    virtual void DoL();
DECLARE_SMELEMENT_FOOTER( THandleProvisionError )


DECLARE_SMELEMENT_HEADER( TStoreAndFilterDeprecatedAndForwardStateChange, MeshMachine::TStateTransition<IpProtoCpr::TContext>, NetStateMachine::MStateTransition, IpProtoCpr::TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TStoreAndFilterDeprecatedAndForwardStateChange )

DECLARE_SMELEMENT_HEADER( TLinkUp, MeshMachine::TStateTransition<IpProtoCpr::TContext>, NetStateMachine::MStateTransition, IpProtoCpr::TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TLinkUp )

DECLARE_SMELEMENT_HEADER( TLinkDown, MeshMachine::TStateTransition<IpProtoCpr::TContext>, NetStateMachine::MStateTransition, IpProtoCpr::TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TLinkDown )
	
DECLARE_AGGREGATED_TRANSITION2(
	TLinkUpAndTStartSelf,
	CoreNetStates::TStartSelf,
	IpProtoCpr::TLinkUp
	)
	




DECLARE_SMELEMENT_HEADER( TDataClientIsIdle, MeshMachine::TStateTransition<IpProtoCpr::TContext>, NetStateMachine::MStateTransition, IpProtoCpr::TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TDataClientIsIdle )

DECLARE_SMELEMENT_HEADER( TSendStarted, MeshMachine::TStateTransition<IpProtoCpr::TContext>, NetStateMachine::MStateTransition, IpProtoCpr::TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TSendStarted )

DECLARE_SMELEMENT_HEADER( TAwaitingStart, MeshMachine::TState<IpProtoCpr::TContext>, NetStateMachine::MState, IpProtoCpr::TContext)
	virtual TBool Accept();
DECLARE_SMELEMENT_FOOTER( TAwaitingStart )

DECLARE_SMELEMENT_HEADER( TCleanupStart, MeshMachine::TStateTransition<IpProtoCpr::TContext>, NetStateMachine::MStateTransition, IpProtoCpr::TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TCleanupStart )

DECLARE_SMELEMENT_HEADER( TCheckIfLastControlClientLeaving, MeshMachine::TStateTransition<IpProtoCpr::TContext>, NetStateMachine::MStateTransition, IpProtoCpr::TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TCheckIfLastControlClientLeaving )

DECLARE_SMELEMENT_HEADER(TAwaitingGoneDown, MeshMachine::TState<IpProtoCpr::TContext>, NetStateMachine::MState, IpProtoCpr::TContext)
	virtual TBool Accept();
DECLARE_SMELEMENT_FOOTER(TAwaitingGoneDown)


//-=========================================================
//	Data monitoring states
//-=========================================================
DECLARE_SMELEMENT_HEADER(TAwaitingDataMonitoringNotification, MeshMachine::TState<IpProtoCpr::TContext>, NetStateMachine::MState, IpProtoCpr::TContext)
	virtual TBool Accept();
DECLARE_SMELEMENT_FOOTER(TAwaitingDataMonitoringNotification)

DECLARE_SMELEMENT_HEADER(TProcessDataMonitoringNotification, MeshMachine::TStateTransition<IpProtoCpr::TContext>, NetStateMachine::MStateTransition, IpProtoCpr::TContext)
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER(TProcessDataMonitoringNotification)

//-=========================================================
// Open/close route states
//-=========================================================
DECLARE_SMELEMENT_HEADER(TAwaitingOpenCloseRoute, MeshMachine::TState<IpProtoCpr::TContext>, NetStateMachine::MState, IpProtoCpr::TContext)
	virtual TBool Accept();
DECLARE_SMELEMENT_FOOTER(TAwaitingOpenCloseRoute)

DECLARE_SMELEMENT_HEADER(TDoOpenCloseRoute, MeshMachine::TStateTransition<IpProtoCpr::TContext>, NetStateMachine::MStateTransition, IpProtoCpr::TContext)
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER(TDoOpenCloseRoute)
   
//-=========================================================
// DataClientStart states
//-=========================================================
DECLARE_SMELEMENT_HEADER( TSendStopToSelf, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TSendStopToSelf )

//-=========================================================
// DataClientStatusChange states
//-=========================================================
DECLARE_SMELEMENT_HEADER(TProcessDataClientStatusChange, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, TContext)
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER(TProcessDataClientStatusChange)

//-=========================================================
// ioctl states
//-=========================================================
DECLARE_SMELEMENT_HEADER(TAwaitingIoctlMessage, MeshMachine::TState<IpProtoCpr::TContext>, NetStateMachine::MState, IpProtoCpr::TContext)
	virtual TBool Accept();
DECLARE_SMELEMENT_FOOTER(TAwaitingIoctlMessage)

DECLARE_SMELEMENT_HEADER(TForwardToDefaultDataClient, MeshMachine::TStateTransition<IpProtoCpr::TContext>, NetStateMachine::MStateTransition, IpProtoCpr::TContext)
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER(TForwardToDefaultDataClient)

}

#endif //IPPROTOCPRMESSAGES_H_

