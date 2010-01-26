/**
* Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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



/**
 @file NifmanIPC.h
 @internalComponent
*/

#ifndef __NIFMANIPC_H__
#define __NIFMANIPC_H__

/**
@internalComponent
*/
_LIT(KGenericAgentName, "GenConn");

/**
@internalComponent
*/
const TInt KGenericAgentMajorVersionNumber=1;
const TInt KGenericAgentMinorVersionNumber=0;
const TInt KGenericAgentBuildVersionNumber=102;

/**
@internalComponent
*/
enum TNifCompanionFunction{ ENifStartPos, ENifCompanionFirst=100 };

/**
@internalComponent
*/
enum TNifMessage 
	{
	ENifStart=ENifStartPos, ENifOpen, ENifStop, ENifProgress, 
	ENifProgressNotification, ENifCancelProgressNotification, ENifAgentInfo,
	ENifLastProgressError, ENifNetworkActive, ENifDisableTimers, ENifCheckIniConfig,
	ENifOpenMonitor,
	ENifCompanionStart=ENifCompanionFirst
	};

/**
@internalComponent
*/
enum TGenericAgentOperation
	{
	EGenericAgentSetOutgoingOverrides = ENifCompanionFirst + 1,
	EGenericAgentOutgoingErrorNotification,
	EGenericAgentCancelOutgoingErrorNotification,
	EGenericAgentSetIncomingOverrides,
	EGenericAgentAcceptIncomingRequests,
	EGenericAgentAcceptIncomingRequestsCancel,
	EGenericAgentServiceChangeNotification,
	EGenericAgentCancelServiceChangeNotification,
	EGenericAgentGetActiveIntSetting,
	EGenericAgentGetActiveBoolSetting,
	EGenericAgentGetActiveDes8Setting,
	EGenericAgentGetActiveDes16Setting,
	EGenericAgentGetActiveLongDesSetting,
	EGenericAgentDebugMarkStart,
	EGenericAgentDebugMarkEnd,
	EGenericAgentDebugSetAllocFail,
	EGenericAgentConnectionError
	};

#endif

