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
// NETUTIL Definitions
// 
//

/**
 @file
 @internalComponent
*/

#if !defined(__NI_STD_H__)
#define __NI_STD_H__

#include <comms-infras/nif_ver.h>

/**
@internalComponent
*/
enum TMBufExtPanic
	{
	EMbExtPanic_MBufAllocTooBig,
	EMbExtPanic_HeaderTooBig,
	EMbExtPanic_PacketHeader
	};
/**
@internalComponent
*/
enum TNifManPanic
	{
	ENifManPanic_Unknown,
	ENifManPanic_LinkLayerAlreadyExists,
	ENifManPanic_InterfaceAlreadyExists,
	ENifManPanic_InterfaceDoesNotExist,
	ENifManPanic_LinkLayerNotEstablished,
	ENifManPanic_BadLinkDownState,
	ENifManPanic_NotAuthenticate,
	ENifManPanic_BadState,
	ENifManPanic_HandleShouldBeNull,
	ENifManPanic_LibraryNotClosed,
	ENifManPanic_BindWithoutId,
	ENifManPanic_NegativeCloseCount,
	ENifManPanic_BadLinkUpState,
	ENifManPanic_NotIdle,
	ENifManPanic_BadTimerCalculation,
	ENifManPanic_NifProgressOutOfRange,
	ENifManPanic_AgtProgressOutOfRange,
	ENifManPanic_BadAgentPointer,
	ENifManPanic_UsageProfileOutOfRange,
	ENifManPanic_BadNetworkControllerCallback,
	ENifManPanic_NonNullAgentPointer,
	ENifManPanic_IncorrectNifOperation,
	ENifManPanic_UnSupportedCall,
	ENifManPanic_BadTimerModeTimeout
	};
/**
@internalComponent
*/
enum TNetUtilPanic
	{
	ENuPanic_NotConstructed
	};

/**
@internalComponent
*/
void NetUtilPanic(TNetUtilPanic aPanic);
void Panic(TNifManPanic aPanic);

#endif
