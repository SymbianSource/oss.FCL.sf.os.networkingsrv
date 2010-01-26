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
// Interface Manager Standard Variable Names
// 
//

/**
 @file nifprvar_internal.h
*/

#ifndef __NIFPRVAR_INTERNAL_H__
#define __NIFPRVAR_INTERNAL_H__
#include <comms-infras/nifprvar.h>
/**
@internalComponent
*/
#define NIF_IFSERVERMODE            _L("Service\\IfServerMode")
#define NIF_CLIENTTIMEOUT			_L("Timeout\\Client")
#define NIF_ROUTETIMEOUT			_L("Timeout\\Route")
#define NIF_STOPIFNOCLIENT			_L("Timeout\\StopIfNoClient")
#define NIF_RECONNECTIFNOCLIENT     _L("Timeout\\ReconnectIfNoClient")

/**
Query is dial in responses from NetDial
@internalComponent
*/
enum TNDDialType
	{
	ENDDialTypeDialOut,
	ENDDialTypeDialIn,
	ENDDialTypeCallBackDialIn
	};

/**
Identical to TAgentToNifEventType, but defined to move the mindset away from NIF
terminology and towards the new architecture
*/
enum TAgentToFlowEventType
	{
	EAgentToFlowEventTypeModifyInitialTimer,		//< for GPRS context activation
	EAgentToFlowEventTypeDisableTimers,			//< for GPRS suspension
	EAgentToFlowEventTypeEnableTimers,			//< for GPRS resumption
	EAgentToFlowEventTypeGetDataTransfer,
	EAgentToFlowEventTypeDisableConnection,
	EAgentToFlowEventTsyConfig,
	EAgentToFlowEventTsyConnectionSpeed,
	EAgentToFlowEventVendorSpecific = KVendorSpecificNotificationStart
	};

/**
Identical to TNifToAgentEventType, but defined to move the mindset away from NIF
terminology and towards the new architecture
*/
enum TFlowToAgentEventType
	{
	EFlowToAgentEventTypePPPCallbackGranted	= 1,
	EFlowToAgentEventTypeQueryIsDialIn	= 2,
	EFlowToAgentEventTypeLinkLayerDown	= 3,
	EFlowToAgentEventTsyConfig           = 4,
	EFlowToAgentEventTsyConnectionSpeed  = 5,	
	
	EFlowToAgentEventVendorSpecific = KVendorSpecificNotificationStart
	};
	
#endif

