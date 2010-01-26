// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// This file forms the interface that other modules use to access the tunnel nif - so definitions of addressing 
// used by the nif etc
// 
//

/**
 @file tunnelnifvar.h
 @publishedPartner
 @released
*/

#ifndef __TUNNELNIFVAR_H__
#define __TUNNELNIFVAR_H__

#include <comms-infras/nifif.h>

enum TTunnelNifToAgentEventType
	{
	ENifToAgentEventTypeSetIfName = KVendorSpecificNotificationStart + 2
	};

enum TAgentToTunnelNifEventType
	{
	EAgentToNifEventTypeSetAddress = KVendorSpecificNotificationStart + 3,
	EAgentToNifEventTypeUpdateAddress
	};

#endif
