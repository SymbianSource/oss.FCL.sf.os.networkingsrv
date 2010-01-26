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

#if defined (_DEBUG) && !defined (__WINC__)

const TInt KHexDumpWidth = 16;

#define LOG(x) x

class HookLog
	{
public:
	static void Printf(TRefByValue<const TDesC8> aFmt, ...);
	static void Printf(TRefByValue<const TDesC> aFmt, ...);
//	static void ConnectionInfoPrintf(const TDesC8& aConnectionInfo, TRefByValue<const TDesC> aFmt, ...);
	};

#else
#define LOG(x)
#endif

#endif // __HOOK_LOG_H__




