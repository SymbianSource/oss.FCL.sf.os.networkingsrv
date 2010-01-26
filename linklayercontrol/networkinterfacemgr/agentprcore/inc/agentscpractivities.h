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
// THIS API IS INTERNAL TO NETWORKING AND IS SUBJECT TO CHANGE AND IS NOT FOR EXTERNAL USE
// Agent SCpr Activities
// 
//

/**
 @file
 @internalTechnology
 @prototype
*/

#ifndef SYMBIAN_AGENTSCPRACTIVITIES_H
#define SYMBIAN_AGENTSCPRACTIVITIES_H

#include <elements/mm_activities.h>


enum TAgentSCprActivities
    {
    ECFAgentSCprActivityBase = MeshMachine::KActivityCustom + 1, // Range is 0x0080-0x00bf
    ECFAgentSCprCustomActivityBase = ECFAgentSCprActivityBase + 0x0040, // Range is 0x00c0-0x00ff

    // Activities
    ECFAgentSCprNotify 			= ECFAgentSCprActivityBase,
	ECFActivityAuthentication	= ECFAgentSCprActivityBase + 1,
    };


namespace AgentSCprActivities
{
	DECLARE_EXPORT_ACTIVITY_MAP(agentSCprActivities)
}


#endif
// SYMBIAN_AGENTSCPRACTIVITIES_H

