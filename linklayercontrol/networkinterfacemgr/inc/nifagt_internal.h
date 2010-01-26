// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Agent (agt) APIs.
// This file contains the APIs required to implement a basic agent for Symbian OS.  APIs for more advanced agents can be found in the header files referenced below.
// 
//

/**
 @file nifagt_internal.h
 @internalTechnology
 @released
*/

#ifndef __NIFAGT_INTERNAL_H__
#define __NIFAGT_INTERNAL_H__

#include <e32base.h>


/**
@defgroup Constant definitions for use with CNifAgentBase::Control() calls
*/

/**
Notify the Agent that a new client has attached to it.

@internalTechnology
@capability NetworkControl These control options affect configuration at the designated level.
*/
const TUint KCOAgentNotifyClientAttach = 0x01 | KConnInternalOptionBit;

enum TAgentConnectionCommand
/**
Constant definitions for use with MNifAgentNotify::AgentEvent() calls.
These use an event type of EAgentOriginatedConnectionCommand.
@internalTechnology
@released
@since v8.1
*/
	{
	/**
	Perform a Stop() operation on the connection
	@note This allows an agent to stop a connection (analogous to a nif calling LinkLayerDown()
	*/
	EAgentConnectionCommandStop,
	};



#endif //__NIFAGT_INTERNAL_H__
