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
 
#ifndef __TNIFNOTIFY_H__
#define __TNIFNOTIFY_H__

#include <comms-infras/nifprvar.h>

class MDummyNifToAgtHarnessNotify 
/** @internalComponent */
	{
public:
	// Used by the test harness to intercept Agent to Nif Events
	virtual TInt Notification(TAgentToNifEventType aEvent, TAny* aInfo)=0;

	// Informs the test harness of the AgentProgress, used for some test cases
	virtual void AgentProgress(TInt aStage, TInt aError)=0;
	};

#endif
