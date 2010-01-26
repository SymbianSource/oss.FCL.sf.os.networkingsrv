/**
* Copyright (c) 2001-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* Agent internal errors and panic codes
* 
*
*/



/**
 @file AGENTPANIC.H
 @internalComponent
*/

#ifndef __AGENTPANIC_H__
#define __AGENTPANIC_H__

#include <e32def.h>

namespace Agent
	{

/**
panic codes for a CAgentBase
@internalComponent
*/
	enum TAgentPanic
		{
		EAgentExtAlreadyActive,
		EDialOutConnectionAlreadyStarted,
		EDialInConnectionAlreadyStarted,
		ENotDoingDialOutBeforeCallBack,
		ECallBackNotPending,
		EControllerUnknownStartType,
		EAgentExtUnknownStartType,
		EDestroyNotificationNotCancelled,
		EIllegalDbRequestForDialIn,
		EUnknownTableName,
		EUnknownDatabaseType,
		EObserverNotNull,
		EObserverNull,
		ENullWarnParameter,
		EIllegalActionType,
		EDialogProcessorSelectObserverNotImplemented,
		EDialogProcessorSelectModemAndLocationObserverNotImplemented,
		EDialogProcessorWarnObserverNotImplemented,
		EDialogProcessorLoginObserverNotImplemented,
		EDialogProcessorAuthObserverNotImplemented,
		EDialogProcessorReconnectObserverNotImplemented,
		EDialogProcessorReadPctObserverNotImplemented,
		EDialogProcessorDestroyPctObserverNotImplemented,
		EDialogProcessorWarnQoSObserverNotImplemented,
		EUnknownDatabaseDeviceType,
		EDbSettingsNotRead,
		ENotModemOrLocationTable,
		ENullDatabase,
		ENullDialogProcessor,
		ENullNifmanNotifyPointer,
		EEventLoggerMoreThanOneListenerForNotifyLastUpdate,
		ENullCLogEventPointerPresentInLogEventQueue
		};

/**
panic codes for a CStateMachineAgentBase
@internalComponent
*/
	enum TStateMachineAgentPanic
		{
		ENullStateMachineOnAuthentication,
		ENullStateMachineOnCancelAuthentication,
		ENullStateMachineOnReconnect,
		ENullStateMachineOnCallBack,
		ENullStateMachineOnGetExcessData,
		ENullStateMachineOnDb,
		ENonNullStateMachineOnOutgoing,
		ENonNullStateMachineOnIncoming,
		ENullStateOnProcessState,
		EUnknownStartType,
		ENullStateMachineOnConnect,
		ENullStateMachineOnCancelConnect,
		ENullStateMachineOnDisconnect,
		ENullStateMachineOnNotification,
		ENullStateMachineOnGetLastError,
		ENullStateMachineOnIsReconnect,
		EUndefinedNotifyOperation,
		ENotifyCallbackAlreadyPending,
		ENonNullStateMachineOnCreate
		};
	}


/**
@internalComponent
*/
GLREF_C void AgentPanic(Agent::TAgentPanic aPanic);

/**
@internalComponent
*/
GLREF_C void StateMachineAgentPanic(Agent::TStateMachineAgentPanic aPanic);

#endif

