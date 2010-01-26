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

#include "timeout.h"
#include "qos_prot.h"

// CModuleTimeout
CModuleTimeout::CModuleTimeout(TCallBack& aCallBack, TInt aPriority) : 
  CTimer(aPriority), iCallback(aCallBack.iFunction, aCallBack.iPtr)
	{
	}

CModuleTimeout* CModuleTimeout::NewL(TCallBack& aCallback, TInt aPriority)
	{
	CModuleTimeout* timeout = new (ELeave) CModuleTimeout(aCallback, aPriority);
	CleanupStack::PushL(timeout);
	timeout->InitL();
	CleanupStack::Pop();
	return timeout;
	}

void CModuleTimeout::Start(TUint aMicroSeconds)
	{
	if (!IsActive())
		After(aMicroSeconds);
	}

void CModuleTimeout::Restart(TUint aMicroSeconds)
	{
	if (IsActive())
		Cancel();
	After(aMicroSeconds);
	}

void CModuleTimeout::InitL()
	{
	ConstructL();
	CActiveScheduler::Add(this);
	}

