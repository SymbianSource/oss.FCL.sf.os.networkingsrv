/**
* Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* GenConn Data and Event Logger header
* 
*
*/



/**
 @file SLOGGER.H
 @internalComponent
*/



#ifndef __SLOGGER_H__
#define __SLOGGER_H__

#include <comms-infras/commsdebugutility.h>

/**
@internalComponent
*/
_LIT(KGenConnLogFile,"GENCONN.TXT");
_LIT(KGenConnLogFolder,"GENCONN");

#ifdef	__FLOG_ACTIVE

/**
@internalComponent
*/
#define CREATELOGTEXT(AAA)			RFileLogger::Write(KGenConnLogFolder(),KGenConnLogFile(),EFileLoggingModeOverwrite,AAA)
#define LOGTEXT2(AAA,BBB)			RFileLogger::WriteFormat(KGenConnLogFolder(),KGenConnLogFile(),EFileLoggingModeAppend,TRefByValue<const TDesC>(AAA),BBB)
#define LOGSTRING(AAA)				{ _LIT(tempLogDes,AAA); RFileLogger::Write(KGenConnLogFolder(),KGenConnLogFile(),EFileLoggingModeAppend,tempLogDes()); }
#define LOGSTRING2(AAA,BBB)			{ _LIT(tempLogDes,AAA); RFileLogger::WriteFormat(KGenConnLogFolder(),KGenConnLogFile(),EFileLoggingModeAppend,TRefByValue<const TDesC>(tempLogDes()),BBB); }
#define LOGSTRING3(AAA,BBB,CCC)		{ _LIT(tempLogDes,AAA); RFileLogger::WriteFormat(KGenConnLogFolder(),KGenConnLogFile(),EFileLoggingModeAppend,TRefByValue<const TDesC>(tempLogDes()),BBB,CCC); }

#else	// Release builds

#define CREATELOGTEXT(AAA)			RFileLogger::Write(KGenConnLogFolder(),KGenConnLogFile(),EFileLoggingModeOverwrite,KNullDesC())
#define LOGTEXT2(AAA,BBB)
#define LOGSTRING(AAA)
#define LOGSTRING2(AAA,BBB)
#define LOGSTRING3(AAA,BBB,CCC)

#endif	// _DEBUG

#endif	// __SLOGGER_H__


