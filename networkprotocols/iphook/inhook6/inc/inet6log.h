// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// LOG.H
// 
//

/**
 @internalComponent
*/

//Logging is now using CDU instead of flogger.
#ifndef INET6LOG_H
#define INET6LOG_H

#include <comms-infras/commsdebugutility.h>

#ifdef __FLOG_ACTIVE
#include <cflog.h>

#  define LOG(a) a 
#  define _LOG

//subsystem name for logging component use this file for logging
_LIT8(KTcpip6Subsystem,"inet6");

//defining component name
#ifndef TCPIP6_COMPNAME
#define TCPIP6_COMPNAME "tcpip6"
#endif

_LIT8(KTcpip6Component,TCPIP6_COMPNAME);

namespace Log
{
	static void Write(const TDesC& aDes)
	{
	CCFLogIf::Write(KTcpip6Subsystem, KTcpip6Component, aDes);
	}

	static void Printf(TRefByValue<const TDesC> aFmt, ...)
	{
	VA_LIST list;
	VA_START(list,aFmt);
	CCFLogIf::WriteFormat(KTcpip6Subsystem, KTcpip6Component, aFmt, list);
	VA_END(list);
	}
	
	static void Printf(TRefByValue<const TDesC8> aFmt, ...)
	{
	VA_LIST list;
	VA_START(list,aFmt);
	CCFLogIf::WriteFormat(KTcpip6Subsystem, KTcpip6Component, aFmt, list);
	VA_END(list);
	}

	static void HexDump(const TText* aHeader, const TText* aMargin, const TUint8* aPtr, TInt aLen)
	{
	CCFLogIf::HexDump(KTcpip6Subsystem, KTcpip6Component, aHeader, aMargin, aPtr, aLen);
	}
}//end of namespace.

#else //!flog active

#define LOG(a)
#endif //flog active

#endif //INET6LOG_H
