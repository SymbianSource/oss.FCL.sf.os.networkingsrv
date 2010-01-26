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
// This file specifies the various events which will cause a panic
// inside the NetUps
// @internalComponent
// @prototype
// 
//

#ifndef NETUPSASSERT_H
#define NETUPSASSERT_H

#include <e32cmn.h>				// defines LITC
#include <e32def.h>				// defines__ASSERT_DEBUG

namespace NetUps
{
_LIT (KNetUpsPanic,"NetUpsPanic");

enum
 	{
	KPanicStateMachineIndexOutOfRange 	= 0,
	KPanicInvalidStateMachineIndex		= 1,
	KPanicMethodNotSupported		  	= 2,
	KPanicThreadLogonCancelInvalid		= 3,
	KPanicProcessLogonCancelInvalid		= 4,
	KPanicOutStandingRequestRePosted	= 5,	
	KPanicNullPointer					= 6,
	KPanicUpsResponseOutOfRange			= 7,
	KPanicSessionTypeOutOfRange			= 8,
	KPanicAttemptToDecrementPastZero	= 9,
	KPanicInvalidEvent					= 10,
	KPanicNonNullPointerToBeOverwritten = 11,
	KPanicInvalidState					= 12,
	KPanicUnhandledState				= 14,
	KPanicInvalidLogic					= 15,
	KPanicNullPointer_NetUpsSimpl		= 100,
	KPanicNullPointer_NetUpsSimpl1		= 101,
	KPanicNullPointer_NetUpsSimpl2		= 102,
	KPanicNullPointer_NetUpsSimpl3		= 103,
	KPanicNullPointer_NetUpsSimpl4		= 104,
	KPanicNullPointer_NetUpsSimpl5		= 105,
	KPanicNullPointer_NetUpsSimpl6		= 106,
	KPanicNullPointer_NetUpsSimpl7		= 107,
	KPanicNullPointer_NetUpsSimpl8		= 108,
	KPanicNullPointer_NetUpsSimpl9		= 109,
	KPanicNullPointer_NetUpsSimpl10		= 110,
	KPanicNullPointer_NetUpsSimpl11		= 111,
	KPanicNullPointer_NetUpsSimpl12		= 112,
	KPanicNullPointer_NetUpsSimpl13		= 113,
	KPanicNullPointer_NetUpsSimpl14		= 114,
	KPanicNullPointer_NetUpsSimpl15		= 115,
	KPanicNullPointer_NetUpsSimpl16		= 116,
	KPanicNullPointer_NetUpsSimpl17		= 117,
	KPanicNullPointer_NetUpsSimpl18		= 118,
	KPanicNullPointer_NetUpsSimpl19		= 119,
	KPanicNullPointer_NetUpsSimpl20		= 120,	
	KPanicNullPointer_NetupsPolicyCheckRequestQueue  = 200,
	KPanicNullPointer_NetupsPolicyCheckRequestQueue1 = 201,
	KPanicNullPointer_NetupsPolicyCheckRequestQueue2 = 202,	
	KPanicNullPointer_NetUpsAction		= 300,
	KPanicNullPointer_NetUpsAction1		= 301,
	KPanicNullPointer_NetUpsAction2		= 302,
	KPanicNullPointer_NetUpsAction3		= 303,	
	KPanicNullPointer_NetUpsSubSession  = 400,
	KPanicNullPointer_NetUpsSubSession1 = 401	
	};

} // end of namespace NetUps

#endif // NETUPSASSERT_H
