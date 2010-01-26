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

_LIT(KHookLogFolder, "ramod");

_LIT(KHookLogFile, "ipeventlog.txt");

void HookLog::Printf(TRefByValue<const TDesC> aFmt,...)
/**
 * Write a mulitple argument list to the log, trapping and ignoring any leave
*/
	{

	VA_LIST list;
	VA_START(list,aFmt);
	RFileLogger::WriteFormat(KHookLogFolder(), KHookLogFile(), EFileLoggingModeAppend, aFmt, list);
	}

void HookLog::Printf(TRefByValue<const TDesC8> aFmt,...)
/**
 * Write a mulitple argument list to the log, trapping and ignoring any leave
 */
	{

	VA_LIST list;
	VA_START(list,aFmt);
	RFileLogger::WriteFormat(KHookLogFolder(), KHookLogFile(), EFileLoggingModeAppend, aFmt, list);
	}


#endif

