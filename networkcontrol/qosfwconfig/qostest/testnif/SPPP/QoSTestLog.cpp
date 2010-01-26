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
// QosTest factory class and DLL entry point
// 
//

/**
 @file
 @internalComponent
*/

#include "QoSTestLog.h"
#ifdef _DEBUG

_LIT(KPdpLogFolder,"QoSPpp");
_LIT(KPdpLogFile,"Context");
 
 //
 // Log definitions
 //
 
 void PdpLog::Write(const TDesC& aDes)
 //
 // Write aText to the log
 //
 	{
 
 	RFileLogger::Write(KPdpLogFolder(),KPdpLogFile(),EFileLoggingModeAppend,aDes);
 	}
 
void PdpLog::Printf(TRefByValue<const TDesC> aFmt,...)
 //
 // Write a mulitple argument list to the log, trapping and ignoring any leave
 //
 	{
 
 	VA_LIST list;
 	VA_START(list,aFmt);
 	RFileLogger::WriteFormat(KPdpLogFolder(),KPdpLogFile(),EFileLoggingModeAppend,aFmt,list);
 	}
#endif //_DEBUG
