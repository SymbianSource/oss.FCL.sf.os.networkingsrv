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
// Implementation file for the IP CPR Logger
// 
//

/**
 @file ipcprlog.cpp
*/

#ifdef _DEBUG

#include <e32std.h>
#include "ipscprlog.h"
#include "comms-infras/commsdebugutility.h"

/**
Comms Debug Utility Folder Name
@internalComponent
*/
_LIT(KQFrameLogFolder, "SubConn");

/**
Comms Debug Utilitiy File Name
@internalComponent
*/
_LIT(KQFrameLogFile, "ipscpr");

void IpCprLog::Printf(TRefByValue<const TDesC> aFmt,...)
/**
Write arguments in supplied format to log file
*/
	{
#if defined __FLOG_ACTIVE
	VA_LIST list;
	VA_START(list,aFmt);
	RFileLogger::WriteFormat(KQFrameLogFolder(), KQFrameLogFile(), EFileLoggingModeAppend, aFmt, list);
#else // stop variable not used message
	(void)aFmt;
	(void)KQFrameLogFolder();
	(void)KQFrameLogFile();
#endif
	}

void IpCprLog::Printf(TRefByValue<const TDesC8> aFmt,...)
/**
Write arguments in supplied format to log file
*/
	{
#if defined __FLOG_ACTIVE
	VA_LIST list;
	VA_START(list,aFmt);
	RFileLogger::WriteFormat(KQFrameLogFolder(), KQFrameLogFile(), EFileLoggingModeAppend, aFmt, list);
#else
	(void)aFmt;
	(void)KQFrameLogFolder();
	(void)KQFrameLogFile();
#endif
	}

#endif // _DEBUG

