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
 @file hooklog.cpp
 @internalComponent
*/

#if defined (_DEBUG) && !defined (__WINC__)

#include <e32std.h>
#include "HookLog.h"
#include "comms-infras/commsdebugutility.h"

#ifndef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
_LIT(KHookLogFolder, "ipevent");

_LIT(KHookLogFile, "ipeventlog.txt");
#endif

void HookLog::Printf(TRefByValue<const TDesC> aFmt,...)
/**
 * Write a mulitple argument list to the log, trapping and ignoring any leave
*/
	{

	VA_LIST list;
	VA_START(list,aFmt);
#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
	CCFLogIf::WriteFormat(KHookLogFolder, KHookLogFile(), aFmt, list);
#else
	RFileLogger::WriteFormat(KHookLogFolder(), KHookLogFile(), EFileLoggingModeAppend, aFmt, list);
#endif
	}

void HookLog::Printf(TRefByValue<const TDesC8> aFmt,...)
/**
 * Write a mulitple argument list to the log, trapping and ignoring any leave
 */
	{

	VA_LIST list;
	VA_START(list,aFmt);
#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
	CCFLogIf::WriteFormat(KHookLogFolder, KHookLogFile(), aFmt, list);
#else
	RFileLogger::WriteFormat(KHookLogFolder(), KHookLogFile(), EFileLoggingModeAppend, aFmt, list);
#endif
	}


#endif

