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
// IP Connection Provider state declarations for User Prompt Service (UPS).
// 
//

/**
 @file
 @internalComponent
*/

#ifndef IPCPRUPS_STATES_H_INCLUDED
#define IPCPRUPS_STATES_H_INCLUDED

#define SYMBIAN_NETWORKING_UPS

#ifdef SYMBIAN_NETWORKING_UPS

#include <comms-infras/corecpr.h>

class CIPConnectionProvider;

namespace IpCprStates
{
typedef MeshMachine::TNodeContext<CIPConnectionProvider, CprStates::TContext> TContext;

// -------- Capability Checks for Starting/Stopping a connection --------

// @TODO PREQ1116 - should we move TCheckCapabilitiesForUpscan into CoreCpr ?
DECLARE_SMELEMENT_HEADER(TCheckCapabilitiesForUps,  MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, TContext)
	virtual void DoL();
	virtual void GetPlatSecResultAndDestinationL(ESock::MPlatsecApiExt& aPlatSecApi, TInt& aPlatSecResult, TUpsDestinationAddrType& aDestinationType, TDes8& aDestination) = 0;
DECLARE_SMELEMENT_FOOTER(TCheckCapabilitiesForUps)

DECLARE_SMELEMENT_HEADER(TCheckStartCapabilities, IpCprStates::TCheckCapabilitiesForUps, NetStateMachine::MStateTransition, TContext)
	void GetPlatSecResultAndDestinationL(ESock::MPlatsecApiExt& aPlatSecApi, TInt& aPlatSecResult, TUpsDestinationAddrType& aDestinationType, TDes8& aDestination);
DECLARE_SMELEMENT_FOOTER(TCheckStartCapabilities)

TInt CheckAttachPolicy(const ESock::MPlatsecApiExt& aPlatSecApi);		// utility for checking attach policy

DECLARE_SMELEMENT_HEADER(TCheckAttachCapabilities, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, TContext)
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER(TCheckAttachCapabilities)



const TInt KUpsQuery = 10000;
const TInt KAttachCapabilityQuery = 10001;

DECLARE_SMELEMENT_HEADER(TNoTagOrUpsQueryOrAttachCapabilityQuery, MeshMachine::TStateFork<TContext>, NetStateMachine::MStateFork, TContext)
	virtual TInt TransitionTag();
DECLARE_SMELEMENT_FOOTER(TNoTagOrUpsQueryOrAttachCapabilityQuery)

const TInt KUpsDisabled = 10012;
const TInt KUpsEnabled  = 10013;

DECLARE_SMELEMENT_HEADER(TNoTagOrUpsDisabledOrUpsEnabled, MeshMachine::TStateFork<TContext>, NetStateMachine::MStateFork, TContext)
	virtual TInt TransitionTag();
DECLARE_SMELEMENT_FOOTER(TNoTagOrUpsDisabledOrUpsEnabled)

DECLARE_SMELEMENT_HEADER(TPostDisabledPolicyCheckResponseToOriginators, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, TContext)
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER(TPostDisabledPolicyCheckResponseToOriginators)

DECLARE_SMELEMENT_HEADER(TPopulatePolicyCheckRequest, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, TContext)
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER(TPopulatePolicyCheckRequest)

DECLARE_SMELEMENT_HEADER( TSendUpsStatusChange, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TSendUpsStatusChange )

// Utility

TBool UpsDisabled(TContext& aContext);
ESock::MPlatsecApiExt* GetPlatsecApiExt(TContext& aContext);
TInt GetPlatsecResult(TContext& aContext);
TInt GetPlatsecResultL(TContext& aContext);
} // IpCprStates

#endif //SYMBIAN_NETWORKING_UPS

#endif //IPCPRUPS_STATES_H_INCLUDED
