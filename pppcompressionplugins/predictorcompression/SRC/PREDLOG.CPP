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

/**
 @file PREDLOG.CPP
*/

#ifdef _DEBUG

#include <comms-infras/commsdebugutility.h>
#include "PREDLOG.H"

/**
KPredLogFolder holds the Name of the PredLog Folder.

@internalComponent
*/
_LIT(KPredLogFolder,"PppComp");
 
/**
KPredLogFile holds the name of the PredLog File

@internalComponent
*/
_LIT(KPredLogFile,"Predictor.txt");


void PredLog::Write(const TDesC& aDes)
/**
Appending the descriptor aDes into the predLog file of predLog folder.
*/
	{

	RFileLogger::Write(KPredLogFolder(),KPredLogFile(),EFileLoggingModeAppend,aDes);
	}

void PredLog::Printf(TRefByValue<const TDesC> aFmt, ...)
	{

	VA_LIST list;
	VA_START(list,aFmt);
	RFileLogger::WriteFormat(KPredLogFolder(),KPredLogFile(),EFileLoggingModeAppend,aFmt,list);
	}

#endif
