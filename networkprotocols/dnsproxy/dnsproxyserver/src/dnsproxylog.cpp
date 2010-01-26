/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
*
*/
#include "dnsproxylog.h"

#include <flogger.h>
#include <e32debug.h>

const TInt KMaxLogData = 255;

#ifdef DNSPROXY_LOGGING_METHOD_RDEBUG
void DNSProxyLog::LogRaw(const TDesC&, TDes& aData)
#else
void DNSProxyLog::LogRaw(const TDesC& aLogfile, TDes& aData)
#endif
    {
    #ifdef DNSPROXY_LOGGING_METHOD_RDEBUG
        //RFileLogger::Write(KLogDirectory, aLogfile, EFileLoggingModeAppend, aData);
        RDebug::Print( _L("%S"), &aData );
    #endif

    #if defined( DNSPROXY_LOGGING_METHOD_FLOGGER )
        RFileLogger::Write(KLogDirectory, aLogfile, EFileLoggingModeAppend, aData);
    #endif
    }

void DNSProxyLog::LogFmt(const TDesC& aLogfile, TRefByValue<const TDesC> aFmt,...)
    {
    VA_LIST list;
    VA_START(list,aFmt);

    TBuf<KMaxLogData> buf;
    buf.FormatList(aFmt,list);

    DNSProxyLog::LogRaw(aLogfile, buf );
    }
