/**
* Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* Header file for the IP CPR Logger
* 
*
*/



/**
 @file ipcprlog.h
*/

#ifndef __IPCPRLOG_H__
#define __IPCPRLOG_H__

#include <e32std.h>

#ifdef _DEBUG

/**
@internalComponent
*/
#define __IPCPRLOG(x) x


class IpCprLog
/**
Provides a wrapper around use of Comms Debug Utility for the the
IP Connection Provider.  Calls are usuallly enclosed with the use
of the LOG macro.  This will thus remove calls to logging in
release builds.

@internalComponent

@released Since v9.0
*/
	{
public:
	static void Printf(TRefByValue<const TDesC8> aFmt, ...);
	static void Printf(TRefByValue<const TDesC> aFmt, ...);
	};

#else

#define __IPCPRLOG(x)

#endif

#endif // __IPCPRLOG_H__
