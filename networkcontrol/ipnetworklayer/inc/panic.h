/**
* Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* Support for TCP/IP calls directly to MNifIfNotify methods
* 
*
*/



/**
 @file notify.h
*/


#ifndef IPSHIMFLOW_PANIC_H_INCLUDED_
#define IPSHIMFLOW_PANIC_H_INCLUDED_

#include <e32def.h>

enum TIPShimPanic
	{
	EBadNotifyCall				= 1,
	EBadInfoControlOption		= 2,
	EBadConfigControlFamily		= 3,
	EBadBinderReadyFamily		= 4,
	EUnexpectedSubConnectionMsg = 5,
	EBinderNotFound				= 6,
	EBinderConfigNotSupported	= 7
	};

GLREF_C void Panic(TIPShimPanic);

#endif
