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
//

/**
 @file
 @internalTechnology
 @prototype
*/


#ifndef SYMBIAN_AGENTPRCONST_H
#define SYMBIAN_AGENTPRCONST_H

#include <comms-infras/ss_activities.h>

/**
@internalTechnology
@prototype

Agent Provider Activities
ECFAgentProviderActivityBase - This base activity Id is for Agent Provider Core internal use
ECFAgentProviderCustomActivityBase - This base activity Id is for Agent Provider Core derived providers to use
*/

enum TAgentProviderActivity
    {
    ECFAgentProviderActivityBase = ESock::ECFActivityCustom + 1, // Range is 0x0080-0x00bf
    ECFAgentProviderCustomActivityBase = ECFAgentProviderActivityBase + 0x0040, // Range is 0x00c0-0x00ff

    // Agent Provider Core internal Activities
    ECFSelfStopDataClientStoppedActivity = ECFAgentProviderActivityBase,
    ECFNotificationFromFlowActivity
    };

#endif
// SYMBIAN_AGENTPRCONST_H

