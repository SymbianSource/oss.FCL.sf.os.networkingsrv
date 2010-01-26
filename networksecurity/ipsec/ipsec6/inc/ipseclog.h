// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
 @internalComponent
*/
#ifndef __IPSECLOG_H__
#define __IPSECLOG_H__

#include <comms-infras/commsdebugutility.h>

#ifdef __FLOG_ACTIVE
#include <cflog.h>

#  define LOG(a) a 
#  define _LOG

//subsystem name for logging component use this file for logging
_LIT8(KSubsystem,"Ipsec");

_LIT8(KSubcomponent,"ipsec6");


namespace Log
	{
static void Write(const TDesC& aDes)
	{
	CCFLogIf::Write(KSubsystem, KSubcomponent, aDes);
	}

static void Printf(TRefByValue<const TDesC> aFmt, ...)
	{
	VA_LIST list;
	VA_START(list,aFmt);
	CCFLogIf::WriteFormat(KSubsystem, KSubcomponent, aFmt, list);
	}

static void Printf(TRefByValue<const TDesC8> aFmt, ...)
	{
	VA_LIST list;
	VA_START(list,aFmt);
	CCFLogIf::WriteFormat(KSubsystem, KSubcomponent, aFmt, list);
	}

static void HexDump(const TText* aHeader, const TText* aMargin, const TUint8* aPtr, TInt aLen)
	{
	CCFLogIf::HexDump(KSubsystem, KSubcomponent, aHeader, aMargin, aPtr, aLen);
	}
}

#else //!flog active

#define LOG(a)
#endif //flog active

#endif //__IPSECLOG_H__
