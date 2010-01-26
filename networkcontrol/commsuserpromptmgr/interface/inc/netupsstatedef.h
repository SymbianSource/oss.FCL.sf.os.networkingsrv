// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// netupstatedef.h
// This file provides the internal definitions for the IP Transport State
// Machine and the events which cause the state machine to transition
// into the next state.
// @internalComponent
// @released
// 
//


#ifndef NETUPSSTATEDEF_H
#define NETUPSSTATEDEF_H

namespace NetUps
{

enum TNetUpsState
	{
	// Numbers need to be unique as they are used in switch statements.

	// Note a transit state is entered when the UPS Server responds with
	// either SessionYes or SessionNo and there are 1 or more UPS requests
	// outstanding to other subsessions which are associated with the same process.			


	// Definitions common to all state machines
	ENull 												= 0,

	// Definitions common to IP Transport Process Life Time
	EProcLife_NonSession 								= 10,

	EProcLife_Transit_SessionYes 						= 11,
	EProcLife_SessionYes 								= 12,

	EProcLife_Transit_SessionNo 						= 13,
	EProcLife_SessionNo 								= 14,

	// Definitions common to IP Transport Network Life Time
	ENetLife_NonSession									= 20,

	ENetLife_SessionNo_Transit_WithoutConnections 		= 21,
	ENetLife_SessionNo_WithOutConnections 				= 22,

	ENetLife_SessionNo_Transit_WithConnections 			= 23,
	ENetLife_SessionNo_WithConnections 					= 24,

	ENetLife_Transit_SessionYes 						= 25,		
	ENetLife_SessionYes 								= 26

	// Subsequent NetUps Clients should add their state definitions
	// in their own unique decade.
	};
	
enum  TEvent
	{
	EPolicyCheckRequest,
	EResponseYes,
	EResponseNo,
	EResponseSessionYes,
	EResponseSessionNo,
	EResponseSessionNo_WithConnections,
	EResponseSessionNo_WithoutConnections,		
	ETransitionForward	
	};

}
#endif // NETUPSSTATEDEF_H
