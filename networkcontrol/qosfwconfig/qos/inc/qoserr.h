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
//



/**
 @internalComponent
*/

#ifndef __QOSERR_H__
#define __QOSERR_H__

/**
* @internalTechnology
*/

enum TQoSReasonCode
    {
    EQoSOk,
    EQoSPolicyExists = -5119,       //< -5119 Policy exists in database
    EQoSNoModules,					//< -5118 No QoS modules available
    EQoSInterface,			        //< -5117 Flows are using different interfaces
	EQoSModules,					//< -5116 Flows use different QoS modules
	EQoSModuleLoadFailed,			//< -5115 Loading of QoS module failed
	EQoSMessageCorrupt,				//< -5114 Pfqos message corrupted
	EQoSJoinFailure,				//< -5113 Join to QoS channel failed
	EQoSLeaveFailure,				//< -5112 Leave from QoS channel failed
	EQoSNoInterface,				//< -5111 Network interface deleted
	EQoSChannelDeleted,				//< -5110 QoS channel deleted
	EQoSDowngradeForced				//< -5109 QoS parameters downgraded by administrative policy
    };

#endif
