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

#include <e32std.h>
#include <cflog.h>
#include "ipscprlog.h"

#ifdef __FLOG_ACTIVE
/**
Comms Debug Utility Folder Name
@internalComponent
*/
_LIT8(KQFrameLogFolder, "SubConn");

/**
Comms Debug Utilitiy File Name
@internalComponent
*/
_LIT8(KQFrameLogFile, "ipscpr");

void IpCprLog::Printf(TRefByValue<const TDesC> aFmt,...)
/**
Write arguments in supplied format to log file
*/
	{
	VA_LIST list;
	VA_START(list,aFmt);
	__CFLOG_VA(KQFrameLogFolder(), KQFrameLogFile(), aFmt, list);
	}

void IpCprLog::Printf(TRefByValue<const TDesC8> aFmt,...)
/**
Write arguments in supplied format to log file
*/
	{
	VA_LIST list;
	VA_START(list,aFmt);
	__CFLOG_VA(KQFrameLogFolder(), KQFrameLogFile(), aFmt, list);
	}
#endif // __FLOG_ACTIVE

