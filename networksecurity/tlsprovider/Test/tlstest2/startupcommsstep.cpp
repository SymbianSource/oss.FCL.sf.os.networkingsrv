// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <c32root.h>
#include "startupcommsstep.h"

// PDD names for the physical device drivers that are loaded in wins or arm
#if defined (__WINS__)
#define PDD_NAME		_L("ECDRV")
#else
#define PDD_NAME		_L("EUART1")
#define PDD2_NAME		_L("EUART2")
#define PDD3_NAME		_L("EUART3")
#define PDD4_NAME		_L("EUART4")
#endif

#define LDD_NAME		_L("ECOMM")

CStartupCommsStep::CStartupCommsStep()
	{
	SetTestStepName(KStartupCommsStep);
	}

TVerdict CStartupCommsStep::doTestStepL()
	{
	SetTestStepResult(EFail);
	
	TInt ret = User::LoadPhysicalDevice(PDD_NAME);
	User::LeaveIfError(ret == KErrAlreadyExists?KErrNone:ret);

#ifndef __WINS__
	ret = User::LoadPhysicalDevice(PDD2_NAME);
	ret = User::LoadPhysicalDevice(PDD3_NAME);
	ret = User::LoadPhysicalDevice(PDD4_NAME);
#endif

	ret = User::LoadLogicalDevice(LDD_NAME);
	User::LeaveIfError(ret == KErrAlreadyExists?KErrNone:ret);
	ret = StartC32();
	User::LeaveIfError(ret == KErrAlreadyExists?KErrNone:ret);
	SetTestStepResult(EPass);

	return TestStepResult();
	}
