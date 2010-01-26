// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

/**
 @file hooklog.h
 @internalComponent
*/

#ifndef __HOOK_LOG_H__
#define __HOOK_LOG_H__

#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY

#include <comms-infras/ss_log.h>

#if defined (__CFLOG_ACTIVE)

_LIT8(KHookLogFolder, "ipevent");
_LIT8(KHookLogFile, "ipeventlog.txt");

class HookLog
	{
public:
	static void Printf(TRefByValue<const TDesC8> aFmt, ...);
	static void Printf(TRefByValue<const TDesC> aFmt, ...);
	};

#endif

#else 	// SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY

#if defined (_DEBUG) && !defined (__WINC__)

const TInt KHexDumpWidth = 16;

#define LOG(x) x

class HookLog
	{
public:
	static void Printf(TRefByValue<const TDesC8> aFmt, ...);
	static void Printf(TRefByValue<const TDesC> aFmt, ...);
	};

#else
#define LOG(x)
#endif

#endif	// SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY

#endif // __HOOK_LOG_H__




