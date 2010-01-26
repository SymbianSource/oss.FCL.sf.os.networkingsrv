// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// IP Connection Provider activity declarations.
// 
//

/**
 @file
 @internalComponent
*/
 
#ifndef IPCPR_ACTIVITIES_H_INCLUDED
#define IPCPR_ACTIVITIES_H_INCLUDED

#define SYMBIAN_NETWORKING_UPS

#include <comms-infras/ss_mobility_apiext.h>
#include <comms-infras/corecprstates.h>

enum TIpCprActivities
    {
    ECFIpCprActivityBase = ESock::ECFActivityCustom + 1,
    ECFIpCprActivitySubConnEvents,
    ECFIpCprActivitySubConnDataTransferred,
    };

class CIPConnectionProvider;

namespace IpCprActivities
{
DECLARE_ACTIVITY_MAP(ipCprActivities)

#ifdef SYMBIAN_NETWORKING_UPS
	DECLARE_NODEACTIVITY(IpCprControlClientJoin)
	DECLARE_NODEACTIVITY(IpCprPolicyCheckRequest)	
	DECLARE_NODEACTIVITY(IpCprClientLeave)
#endif
}

#endif // IPCPR_ACTIVITIES_H_INCLUDED
