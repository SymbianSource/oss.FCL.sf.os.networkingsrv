// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// TeListen.cpp
// 
//

// includes
#include <test/testexecuteclient.h>
#include <e32base.h>
#include <c32root.h>
#include "TeInit.h"
#include "TeSocketListener.h"


_LIT(KPhbkSyncCMI,			"phbsync.cmi");

#if defined (__WINS__)
#define PDD_NAME _L("ECDRV")
#define LDD_NAME _L("ECOMM")
#else
#define PDD_NAME _L("EUART1")
#define LDD_NAME _L("ECOMM")
#endif

CTestStepInit::CTestStepInit(CTestListenerMgr* aListenerMgr) 
: CTestStepBase(aListenerMgr), iListenerMgr(aListenerMgr)
	{
	SetTestStepName(KInit);
	}

CTestStepInit::~CTestStepInit()
	{
	}

TVerdict CTestStepInit::doTestStepL()
	{

	TInt err;

	INFO_PRINTF1(_L("Load PDD"));	
	err = User::LoadPhysicalDevice(PDD_NAME);
	if (err != KErrNone && err != KErrAlreadyExists)
		{
		INFO_PRINTF2(_L("Could not load PDD! Leaving with error %d"), err);
		SetTestStepResult(EFail);
		User::Leave(err);
		}

	INFO_PRINTF1(_L("Load LDD"));	
	err = User::LoadLogicalDevice(LDD_NAME);
	if (err != KErrNone && err != KErrAlreadyExists)
		{
		INFO_PRINTF2(_L("Could not load LDD! Leaving with error %d"), err);
		SetTestStepResult(EFail);
		User::Leave(err);
		}

	// When bootstrapping C32 we have to avoid the PhBkSyncServer being started, since
 	// it needs a different CommDB
	TInt ret = StartC32WithCMISuppressions(KPhbkSyncCMI);
		
	if ((ret != KErrNone) && (ret != KErrAlreadyExists))
		INFO_PRINTF2(_L("error is : %d \n"),ret);
	else	
		INFO_PRINTF1(_L("Started C32\n"));


	return TestStepResult();
	}
