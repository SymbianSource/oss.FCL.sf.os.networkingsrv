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
// Contains the implementation of the helper steps (initialisation and waiting
// for esock to die)
// 
//

/**
 @file TS_DummyPppHelperSteps.cpp
*/

#include "TS_DummyPPPHelperSteps.h"
#include <c32root.h>
//

#include <e32base.h>
#include <e32std.h>
#include <c32comm.h>
#include <e32hal.h>
#include <comms-infras/nifif.h>
#include <comms-infras/nifagt.h>
//

#if defined (__WINS__)
#define PDD_NAME _L("ECDRV")
#define LDD_NAME _L("ECOMM")
#else
#define PDD_NAME _L("EUART1")
#define LDD_NAME _L("ECOMM")
#endif

TS_DummyPPPCommInit::TS_DummyPPPCommInit(TPtrC aName) : TS_DummyPPPStep()
{
	iTestStepName=aName;
}

TS_DummyPPPCommInit::~TS_DummyPPPCommInit()
{
}

enum TVerdict TS_DummyPPPCommInit::doTestStepL(void)
{
	TInt err;

	err=User::LoadPhysicalDevice(PDD_NAME);
	if (err!=KErrNone && err!=KErrAlreadyExists)
		{
		Log(_L("Could not load PDD! Leaving with error %d"), err);
		User::Leave(err);
		}

	err=User::LoadLogicalDevice(LDD_NAME);
	if (err!=KErrNone && err!=KErrAlreadyExists)
		{
		Log(_L("Could not load LDD! Leaving with error %d"), err);
		User::Leave(err);
		}

 	_LIT(KPhbkSyncCMI, "phbsync.cmi");
    err = StartC32WithCMISuppressions(KPhbkSyncCMI);
	if (err!=KErrNone && err!=KErrAlreadyExists)
		{
		Log(_L("Could not start comm process! Leaving with error %d"), err);
		User::Leave(err);
		}

	return iTestStepResult;
}

TS_DummyPPPForceCCoverWrite::TS_DummyPPPForceCCoverWrite(TPtrC aName) : TS_DummyPPPStep()
{
	iTestStepName=aName;
}

TS_DummyPPPForceCCoverWrite::~TS_DummyPPPForceCCoverWrite()
{
}

enum TVerdict TS_DummyPPPForceCCoverWrite::doTestStepL(void)
/*
 * It appears that CCover only writes to its data file every now and again, so force it here 
 * to write its data just before we shutdown and lose anything that has changed
 */
{
	#ifdef CCOVER
		cov_write();
	#endif

	return iTestStepResult;
} // TS_DummyPPPForceCCoverWrite
