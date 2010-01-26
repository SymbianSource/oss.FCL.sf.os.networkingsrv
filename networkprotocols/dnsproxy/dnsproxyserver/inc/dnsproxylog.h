/**
* Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* Declaration of Logger macros.
*
*/



/**
 @file
 @internalComponent
*/

#ifndef __DNSPROXYLOG_H__
#define __DNSPROXYLOG_H__

// INCLUDES
#include <e32base.h>

// Constants
_LIT(KLogDirectory, "DNSPROXY");
_LIT(KLogFileName, "dnsproxy.txt");

class DNSProxyLog
    {
public:
    static void LogRaw(const TDesC& aLogfile, TDes& aData);
    static void LogFmt(const TDesC& aLogfile,TRefByValue<const TDesC> aFmt,...);
    };

//#define DNSPROXY_LOGGING_METHOD_RDEBUG
#define DNSPROXY_LOGGING_METHOD_FLOGGER

    #define __LOG(TXT)                  { _LIT(__KText,TXT);  DNSProxyLog::LogFmt(KLogFileName,__KText); }
    #define __LOG1(TXT,A)               { _LIT(__KText,TXT);  DNSProxyLog::LogFmt(KLogFileName,__KText,A); }
    #define __LOG2(TXT,A,B)             { _LIT(__KText,TXT);  DNSProxyLog::LogFmt(KLogFileName,__KText,A,B); }
    #define __LOG3(TXT,A,B,C)           { _LIT(__KText,TXT);  DNSProxyLog::LogFmt(KLogFileName,__KText,A,B,C); }


#endif
