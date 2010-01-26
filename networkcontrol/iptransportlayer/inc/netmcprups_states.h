/**
* Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
 @file netmcprups_states.h
 @internalTechnology
 @prototype
*/

#ifndef SYMBIAN_NETMCPRUPS_STATES_H
#define SYMBIAN_NETMCPRUPS_STATES_H

#define SYMBIAN_NETWORKING_UPS

#ifdef SYMBIAN_NETWORKING_UPS

#include <comms-infras/mobilitymcprstates.h>

class CUpsNetworkMetaConnectionProvider;

namespace NetMCprUpsStates
/**
Support for UPS

These states are dependent on CUpsNetworkMetaConnectionProvider.
*/
{
typedef ESock::TCFNodeContext<CUpsNetworkMetaConnectionProvider, MobilityMCprStates::TContext> TContext;

DECLARE_SMELEMENT_HEADER( TUpsProcessProviderStatusChange, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, TContext)
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TUpsProcessProviderStatusChange)

DECLARE_SMELEMENT_HEADER(TProcessUpsStatusChange, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, TContext)
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER(TProcessUpsStatusChange)

} //NetMCprUpsStates


#endif //SYMBIAN_NETWORKING_UPS

#endif //SYMBIAN_NETMCPRUPS_STATES_H
