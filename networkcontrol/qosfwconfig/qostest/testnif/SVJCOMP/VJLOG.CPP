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
//

#include "vjlog.h"

#ifdef _DEBUG
_LIT(KPppLogFolder,"Ppp");
_LIT(KVjLogFile,"Vjcomp.txt");

//
// Log definitions
//

void Log::Write(const TDesC& aDes)
//
// Write aText to the log
//
	{

	RFileLogger::Write(KPppLogFolder(),KVjLogFile(),EFileLoggingModeAppend,aDes);
	}

void Log::Printf(TRefByValue<const TDesC> aFmt,...)
//
// Write a mulitple argument list to the log, trapping and ignoring any leave
//
	{

	VA_LIST list;
	VA_START(list,aFmt);
	RFileLogger::WriteFormat(KPppLogFolder(),KVjLogFile(),EFileLoggingModeAppend,aFmt,list);
	}

#endif
