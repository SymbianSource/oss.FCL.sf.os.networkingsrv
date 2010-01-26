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
//

#include "NifmanBCTestSuite.h"
#include "BCTestSection1.h"
#include "BCTestSection2.h"
#include <c32comm.h>



EXPORT_C CNifmanBCTestSuite* NewNifmanBCTestSuiteL()
//
// NewTestSuiteNifmanL is exported at ordinal 1
// this provides the interface to allow schedule test
// to create instances of this test suite
//
	{

	return new(ELeave) CNifmanBCTestSuite();
    }


CNifmanBCTestSuite::~CNifmanBCTestSuite()
//
// D'tor
//
	{
	}


TPtrC CNifmanBCTestSuite::GetVersion()
//
// Make a version string available for test system
//	
	{
	
	_LIT(KTxtVersion,"1.0");
	return KTxtVersion();
	}


void CNifmanBCTestSuite::AddTestStepL(CNifmanBCTestStep* aTestStep)
//
// Add a test step into the suite
//
	{

	// test steps contain a pointer back to the suite which owns them
	aTestStep->iTestSuite = this; 

	// add the step using tyhe base class method
	CTestSuite::AddTestStepL(aTestStep);
	}


void CNifmanBCTestSuite::InitialiseL()
//
// Constructor for Nifman BC test suite
// this creates the Nifman BC test steps and
// stores them inside CNifmanBCTestSuite
//
	{

	TInt err = CommsInit();
	if(err!=KErrNone)
		{
		Log(_L("CommsInit() returned error %d"));
		User::Leave(err);
		}

	// add test steps
	AddTestStepL(new(ELeave) CTestStep1_1());
	AddTestStepL(new(ELeave) CTestStep1_2());
	AddTestStepL(new(ELeave) CTestStep1_3());
	AddTestStepL(new(ELeave) CTestStep1_4());
	AddTestStepL(new(ELeave) CTestStep1_5());
	AddTestStepL(new(ELeave) CTestStep1_6());
	AddTestStepL(new(ELeave) CTestStep1_7());
	AddTestStepL(new(ELeave) CTestStep2_1());
	AddTestStepL(new(ELeave) CTestStep2_2());
	AddTestStepL(new(ELeave) CTestStep2_3());
	AddTestStepL(new(ELeave) CTestStep2_4());
	}

TInt CNifmanBCTestSuite::CommsInit()
//
// Load the serial comms PDD and LDD.
//
// This would normally be done by the 
// TSY, but when we use NT RAS no TSY
// is used so it must be done here.
//
	{

	#if defined (__WINS__)
	#define PDD_NAME _L("ECDRV")
	#else
	#define PDD_NAME _L("EUART1")
	#endif
	#define LDD_NAME _L("ECOMM")

	TInt err = User::LoadPhysicalDevice(PDD_NAME);
	if (err!=KErrNone && err!=KErrAlreadyExists)
		{
		return err;
		}

	err = User::LoadLogicalDevice(LDD_NAME);
	if (err!=KErrNone && err!=KErrAlreadyExists)
		{
		return err;
		}

 	// When bootstrapping C32 we have to avoid the PhBkSyncServer being started, since
 	// it needs a different CommDB
 	_LIT(KPhbkSyncCMI, "phbsync.cmi");
    err = StartC32WithCMISuppressions(KPhbkSyncCMI);
	if (err!=KErrNone && err!=KErrAlreadyExists)
		{
		return err;
		}

	return KErrNone;
	}

