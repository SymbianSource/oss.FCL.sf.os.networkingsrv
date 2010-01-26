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
#include "ipscpr.h"


//-=========================================================
//
// States
//
//-=========================================================
namespace IpSCprStates
{
typedef MeshMachine::TNodeContext<CIpSubConnectionProvider, IPBaseSCprStates::TContext> TContext;

#ifndef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
DECLARE_SMELEMENT_HEADER( TSendParamsToServiceProvider, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, IpSCprStates::TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TSendParamsToServiceProvider )
#endif // SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW

DECLARE_SMELEMENT_HEADER( TSendSelfStart, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, IpSCprStates::TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TSendSelfStart )

DECLARE_SMELEMENT_HEADER( TPrepareToAddClientToQosChannel, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, IpSCprStates::TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TPrepareToAddClientToQosChannel )

DECLARE_SMELEMENT_HEADER( TAddClientToQosChannel, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, IpSCprStates::TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TAddClientToQosChannel )

DECLARE_SMELEMENT_HEADER( TSetJoiningFlag, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, IpSCprStates::TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TSetJoiningFlag )

DECLARE_SMELEMENT_HEADER( TClearJoiningFlag, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, IpSCprStates::TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TClearJoiningFlag )

DECLARE_SMELEMENT_HEADER( TRemoveClientFromQoSChannel, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, IpSCprStates::TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TRemoveClientFromQoSChannel )

DECLARE_SMELEMENT_HEADER( TCreateAddressInfoBundle, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, IpSCprStates::TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TCreateAddressInfoBundle )

DECLARE_SMELEMENT_HEADER( TCreateAddressInfoBundleFromJoiningClient, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, IpSCprStates::TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TCreateAddressInfoBundleFromJoiningClient )


// States

DECLARE_SMELEMENT_HEADER( TAwaitingJoinComplete, MeshMachine::TState<TContext>, NetStateMachine::MState, IpSCprStates::TContext )
	virtual TBool Accept();
DECLARE_SMELEMENT_FOOTER( TAwaitingJoinComplete )

// Forks

const TInt KDoNothingTag = 2;

DECLARE_SMELEMENT_HEADER( TNoTagOrSendApplyResponse, MeshMachine::TStateFork<TContext>, NetStateMachine::MStateFork, IpSCprStates::TContext )
	virtual TInt TransitionTag();
DECLARE_SMELEMENT_FOOTER( TNoTagOrSendApplyResponse )

DECLARE_SMELEMENT_HEADER( TNoTagOrSendApplyResponseOrErrorTag, MeshMachine::TStateFork<TContext>, NetStateMachine::MStateFork, IpSCprStates::TContext )
	virtual TInt TransitionTag();
DECLARE_SMELEMENT_FOOTER( TNoTagOrSendApplyResponseOrErrorTag )

DECLARE_SMELEMENT_HEADER( TNoTagOrDoNothingTag, MeshMachine::TStateFork<TContext>, NetStateMachine::MStateFork, TContext )
	virtual TInt TransitionTag();
DECLARE_SMELEMENT_FOOTER( TNoTagOrDoNothingTag )

DECLARE_SMELEMENT_HEADER( TDoNothingTag, MeshMachine::TStateFork<TContext>, NetStateMachine::MStateFork, TContext )
	virtual TInt TransitionTag();
DECLARE_SMELEMENT_FOOTER( TDoNothingTag )

DECLARE_SMELEMENT_HEADER( TNoTagOrAlreadyStarted, MeshMachine::TStateFork<TContext>, NetStateMachine::MStateFork, TContext )
	virtual TInt TransitionTag();
DECLARE_SMELEMENT_FOOTER( TNoTagOrAlreadyStarted )
// aggregation

DECLARE_AGGREGATED_TRANSITION2(
	TCreateAddressInfoBundleFromJoiningClientAndSetJoiningFlag,
	TCreateAddressInfoBundleFromJoiningClient,
	TSetJoiningFlag
	)

DECLARE_AGGREGATED_TRANSITION2(
	TCreateAddressInfoBundleAndSetJoiningFlag,
	TCreateAddressInfoBundle,
	TSetJoiningFlag
	)

// serialisation
	
DECLARE_SERIALIZABLE_STATE(
	TNoTagOrBearerPresentBlockedByStopOrBindTo,
	MeshMachine::TActivityIdMutex<ESock::ECFActivityBindTo>,
	CoreNetStates::TNoTagOrBearerPresentBlockedByStop
	)
}

#endif // SYMBIAN_IPSCPRSTATES_H
