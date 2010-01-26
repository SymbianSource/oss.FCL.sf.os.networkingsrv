/**
* Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* THIS API IS INTERNAL TO NETWORKING AND IS SUBJECT TO CHANGE AND NOT FOR EXTERNAL USE
* 
*
*/



/**
 @file netmcprstates.h
 @internalTechnology
 @prototype
*/

#ifndef SYMBIAN_NETMCPRSTATES_H
#define SYMBIAN_NETMCPRSTATES_H

#define SYMBIAN_NETWORKING_UPS

#include <comms-infras/mobilitymcprstates.h>
#include "netmcpr.h"

class CNetworkMetaConnectionProvider;
#ifdef SYMBIAN_NETWORKING_UPS
#include <comms-infras/upsmessages.h>
class CUpsNetworkMetaConnectionProvider;
#endif

namespace NetMCprStates
{
typedef MeshMachine::TNodeContext<CNetworkMetaConnectionProvider, MobilityMCprStates::TContext> TContext;

DECLARE_SMELEMENT_HEADER( TAwaitingPolicyParams, MeshMachine::TState<TContext>, NetStateMachine::MState, TContext)
	virtual TBool Accept();
DECLARE_SMELEMENT_FOOTER( TAwaitingPolicyParams)

#ifdef SYMBIAN_NETWORKING_UPS
DECLARE_SMELEMENT_HEADER( TAwaitingUPSStatusChange, MeshMachine::TState<TContext>, NetStateMachine::MState, TContext )
	virtual TBool Accept();
DECLARE_SMELEMENT_FOOTER( TAwaitingUPSStatusChange )
#endif

#ifdef DUMMY_MOBILITY_MCPR

const TInt KMigrationRejected = 1;
const TInt KMigrationAccepted = 2;
const TInt KMigrationRequested = 3;

typedef CoreStates::TAwaitingMessageState<ESock::TCFMessage::TMigrationAvailable> TDummyAwaitingMigrationAvailable;

DECLARE_SMELEMENT_HEADER( TAwaitingMigrationRejectedOrMigrationAccepted, MeshMachine::TState<TContext>, NetStateMachine::MState, TContext)
	virtual TBool Accept();
DECLARE_SMELEMENT_FOOTER( TAwaitingMigrationRejectedOrMigrationAccepted)

DECLARE_SMELEMENT_HEADER( TAwaitingMigrationRequestedOrMigrationRejected, MeshMachine::TState<TContext>, NetStateMachine::MState, TContext)
	virtual TBool Accept();
DECLARE_SMELEMENT_FOOTER( TAwaitingMigrationRequestedOrMigrationRejected)

DECLARE_SMELEMENT_HEADER( TMigrationRequestedOrMigrationRejected, MeshMachine::TStateFork<TContext>, NetStateMachine::MStateFork, TContext)
	virtual TInt TransitionTag();
DECLARE_SMELEMENT_FOOTER( TMigrationRequestedOrMigrationRejected)

DECLARE_SMELEMENT_HEADER( TSendMigrationAvailable, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, TContext)
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TSendMigrationAvailable)

DECLARE_SMELEMENT_HEADER( TSendMigrateToAccessPoint, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, TContext)
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TSendMigrateToAccessPoint)

#endif

#ifdef SYMBIAN_NETWORKING_UPS

//
// Support for User Prompt Service
//
// These tuple classes were added for UPS support but are not in themselves UPS specific.

// States

DECLARE_SMELEMENT_HEADER( TAwaitingNoBearerNotFromSelf, MeshMachine::TState<TContext>, NetStateMachine::MState, TContext)
	virtual TBool Accept();
DECLARE_SMELEMENT_FOOTER( TAwaitingNoBearerNotFromSelf)

DECLARE_SMELEMENT_HEADER( TAwaitingBindToCompleteOrCancel, MeshMachine::TState<TContext>, NetStateMachine::MState, TContext)
	virtual TBool Accept();
DECLARE_SMELEMENT_FOOTER( TAwaitingBindToCompleteOrCancel)

// Transitions

DECLARE_SMELEMENT_HEADER( TSendNoBearerToSelf, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, TContext)
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TSendNoBearerToSelf)

#endif // SYMBIAN_NETWORKING_UPS

DECLARE_SMELEMENT_HEADER( TProcessPolicyParams, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, TContext)
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TProcessPolicyParams)
	
//-=========================================================
//
//Network Meta Connection Provider Transition Ids 11000..20000
//
//-=========================================================

//const TInt KAwaitAvailabilityNotification = 11000;
//const TInt KProcessAvailabilityNotification = 11001;

} // namespace NetMCprStates

#ifdef SYMBIAN_NETWORKING_UPS

namespace NetMCprUpsStates
/**
Support for UPS

These states are dependent on CUpsNetworkMetaConnectionProvider.
*/
{
typedef MeshMachine::TNodeContext<CUpsNetworkMetaConnectionProvider, MobilityMCprStates::TContext> TContext;

DECLARE_SMELEMENT_HEADER( TUpsProcessProviderStatusChange, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, TContext)
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TUpsProcessProviderStatusChange)

#ifdef SYMBIAN_NETWORKING_UPS
DECLARE_SMELEMENT_HEADER( TProcessUpsStatusChange, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, TContext)
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TProcessUpsStatusChange)
#endif

} //NetMCprUpsStates

#endif //SYMBIAN_NETWORKING_UPS

#endif //SYMBIAN_NETMCPRSTATES_H
