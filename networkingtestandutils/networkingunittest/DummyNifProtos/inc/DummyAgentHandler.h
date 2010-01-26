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
*/


#ifndef DUMMYAGENTHANDLER_H_
#define DUMMYAGENTHANDLER_H_

#include <comms-infras/agentmessages.h>

/**
Dummy Proto Agent Handler
*/
NONSHARABLE_CLASS(CDummyProtoAgentHandler) : public CAgentNotificationHandler
	{
public:
	virtual void ConnectCompleteL();
	};

#endif
// DUMMYAGENTHANDLER_H_
