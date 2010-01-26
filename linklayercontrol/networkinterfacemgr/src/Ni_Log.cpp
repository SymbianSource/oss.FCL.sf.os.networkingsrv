// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
 @file NI_LOG.CPP
 @internalComponent
*/

#include <comms-infras/commsdebugutility.h>

#if defined(__FLOG_ACTIVE)

#include "Ni_Log.h"

void NifmanLog::Printf(TRefByValue<const TDesC> aFmt,...)
/**
Write a mulitple argument list to the log, trapping and ignoring any leave

@internalComponent
*/
	{

	VA_LIST list;
	VA_START(list,aFmt);
	RFileLogger::WriteFormat(KNifmanLogFolder(),KNifmanLogFile(),EFileLoggingModeAppend,aFmt,list);
	}

void NifmanLog::Printf(TRefByValue<const TDesC8> aFmt,...)
/**
Write a mulitple argument list to the log, trapping and ignoring any leave

@internalComponent
*/
	{
	VA_LIST list;
	VA_START(list,aFmt);
	RFileLogger::WriteFormat(KNifmanLogFolder(),KNifmanLogFile(),EFileLoggingModeAppend,aFmt,list);
	}

#endif


