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
// IPSCPR States
// 
//

/**
 @file
 @internalComponent
*/

#ifndef SYMBIAN_IPSCPRSTATES_H
#define SYMBIAN_IPSCPRSTATES_H

#include <comms-infras/corescprstates.h>
#include <comms-infras/corescpractivities.h>
#include "IPSCPR.h"
#include "qos_msg.h"

//-=========================================================
//
// States
//
//-=========================================================

namespace QoSIpSCprActivities
{
enum QoSIpSCprActivityId
	{
	EQoSIpSCprOpenInternalSocket = ESock::ECFActivityCustom + 100 // + 100 to keep out of way of anything in base
	};
}

namespace QoSIpSCprStates
{
typedef MeshMachine::TNodeContext<CIpSubConnectionProvider, IPBaseSCprStates::TContext> TContext;

typedef MeshMachine::TAwaitingMessageState<TQoSIpSCprMessages::TOpenInternalSocket> TAwaitingOpenInternalSocket;
typedef MeshMachine::TAwaitingMessageState<TQoSIpSCprMessages::TInternalSocketOpened> TAwaitingInternalSocketOpened;

DECLARE_SERIALIZABLE_STATE(
	TNoTagBlockedByOpenInternalSocket,
	MeshMachine::TActivityIdMutex<QoSIpSCprActivities::EQoSIpSCprOpenInternalSocket>,
    MeshMachine::TNoTag
    )

DECLARE_SMELEMENT_HEADER( TNoTagOrSendApplyResponse, MeshMachine::TStateFork<TContext>, NetStateMachine::MStateFork, QoSIpSCprStates::TContext )
	virtual TInt TransitionTag();
DECLARE_SMELEMENT_FOOTER( TNoTagOrSendApplyResponse )

typedef MeshMachine::TActivitiesIdMutex<QoSIpSCprActivities::EQoSIpSCprOpenInternalSocket, IPDeftSCprBaseActivities::ECFActivityAddressUpdate> TOpenInternalSocketAndAddressUpdateActivityMutex;

DECLARE_SERIALIZABLE_STATE(
	TNoTagOrSendApplyResponseBlockedByOpenInternalSocketAndAddressUpdate,
	TOpenInternalSocketAndAddressUpdateActivityMutex,
    TNoTagOrSendApplyResponse
    )

DECLARE_SMELEMENT_HEADER( TAwaitingJoinComplete, MeshMachine::TState<TContext>, NetStateMachine::MState, QoSIpSCprStates::TContext )
	virtual TBool Accept();
DECLARE_SMELEMENT_FOOTER( TAwaitingJoinComplete )

DECLARE_SMELEMENT_HEADER( TAwaitingLeaveComplete, MeshMachine::TState<TContext>, NetStateMachine::MState, QoSIpSCprStates::TContext )
	virtual TBool Accept();
DECLARE_SMELEMENT_FOOTER( TAwaitingLeaveComplete )

DECLARE_SMELEMENT_HEADER( TAddClientToQoSChannel, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, QoSIpSCprStates::TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TAddClientToQoSChannel )

DECLARE_SMELEMENT_HEADER( TRemoveClientToQoSChannel, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, QoSIpSCprStates::TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TRemoveClientToQoSChannel )

DECLARE_SMELEMENT_HEADER( TOpenInternalSocket, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, QoSIpSCprStates::TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TOpenInternalSocket )

DECLARE_SMELEMENT_HEADER( TRemoveLeavingClientFromQoSChannel, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, QoSIpSCprStates::TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TRemoveLeavingClientFromQoSChannel )

DECLARE_SMELEMENT_HEADER( TStoreAddressUpdateAndAddClientToQoSChannel, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, QoSIpSCprStates::TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TStoreAddressUpdateAndAddClientToQoSChannel )

DECLARE_SMELEMENT_HEADER( TSetParameters, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, QoSIpSCprStates::TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TSetParameters )

DECLARE_SMELEMENT_HEADER( TBindSelfToDefaultServiceProvider, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, QoSIpSCprStates::TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TBindSelfToDefaultServiceProvider )

#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
DECLARE_AGGREGATED_TRANSITION2(
   TStoreAndSetParameters,
   PRStates::TStoreParams,
   QoSIpSCprStates::TSetParameters
   )
#else
DECLARE_AGGREGATED_TRANSITION2(
   TStoreAndSetParameters,
   SCprStates::TStoreParams,
   QoSIpSCprStates::TSetParameters
   )
#endif // SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
}

#endif // SYMBIAN_IPSCPRSTATES_H
