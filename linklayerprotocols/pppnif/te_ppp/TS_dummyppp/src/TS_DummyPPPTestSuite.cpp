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
// This defines the TS_DummyPPPSuite class which is the 
// container class for the TS_DummyPPPStep derived test steps
// 
//

/**
 @file TS_DummyPPPSuite.cpp
*/
#include "TS_DummyPPPSuite.h"
#include "TS_DummyPPPStep.h"
#include "TS_DummyPPPHelperSteps.h"
#include "TS_DummyPPPNifTestSteps.h"
 
					  

EXPORT_C TS_DummyPPPSuite* NewTS_DummyPPPSuite( void ) 
/** Polymorphic interface, exported ordinal 1.  Called by scheduletest
 * and used to instantiate the suite
 * @return a pointer to the created TS_DummyPPPSuite object
 */
{
	TS_DummyPPPSuite* ts = 0;
	TRAPD(err,ts = new (ELeave) TS_DummyPPPSuite);
	if (err == KErrNone)
		return ts;
	return 0;
}

TS_DummyPPPSuite::~TS_DummyPPPSuite()
/** 
 * the destructor has to clean up any TConnDetails and TSock Details left hanging around
 * 
 */
{
}

void TS_DummyPPPSuite::AddTestStepL(TS_DummyPPPStep* ptrTestStep )
/**
 * adds a test step to the test suite object, using the base class method
 * @param ptrTestStep - a pointer to the test step to be added
 * @exception can leave
 */
{
	CTestSuite::AddTestStepL(ptrTestStep);
}

void TS_DummyPPPSuite::InitialiseL( void )
/**
 * Effective second-phase constructor.  Creates all suite test steps
 * and associates them with the suite.
 */
    {
   	// start c32 process
 	// When bootstrapping C32 we have to avoid the PhBkSyncServer being started, since
 	// it needs a different CommDB
 	_LIT(KPhbkSyncCMI, "phbsync.cmi");
    TInt ret = StartC32WithCMISuppressions(KPhbkSyncCMI);
   	if (KErrNone!=ret && KErrAlreadyExists!=ret)
   		{
   		User::Leave( ret );
   		}

	
	// Added for copying agentdialog.ini file
	AddTestStepL( new(ELeave) CDummyPPPPreCopy());
	// Helper test steps
	AddTestStepL(new(ELeave) TS_DummyPPPCommInit(_L("CommInit")));
	AddTestStepL(new(ELeave) TS_DummyPPPForceCCoverWrite(_L("ForceCCoverWrite")));

	// Add the test steps for the tests which mainly use the dummy ppp nif (using NT RAS as the default interface)
	AddTestStepL(new(ELeave) TS_DummyPPPTest1 (_L("Test1")));
	AddTestStepL(new(ELeave) TS_DummyPPPTest2 (_L("Test2")));
	AddTestStepL(new(ELeave) TS_DummyPPPTest3 (_L("Test3")));
	AddTestStepL(new(ELeave) TS_DummyPPPTest4 (_L("Test4")));
	AddTestStepL(new(ELeave) TS_DummyPPPTest5 (_L("Test5")));
	AddTestStepL(new(ELeave) TS_DummyPPPTest6 (_L("Test6")));
	 
	AddTestStepL(new(ELeave) TS_DummyPPPTest7 (_L("Test7")));
	AddTestStepL(new(ELeave) TS_DummyPPPTest8 (_L("Test8")));
	AddTestStepL(new(ELeave) TS_DummyPPPTest9(_L("Test9")));
	AddTestStepL(new(ELeave) TS_DummyPPPTest10(_L("Test10")));
	AddTestStepL(new(ELeave) TS_DummyPPPTest11(_L("Test11")));
 
	AddTestStepL(new(ELeave) TS_DummyPPPTest12(_L("Test12")));
	AddTestStepL(new(ELeave) TS_DummyPPPTest13(_L("Test13")));
	AddTestStepL(new(ELeave) TS_DummyPPPTest14(_L("Test14")));
	AddTestStepL(new(ELeave) TS_DummyPPPTest15(_L("Test15")));
	AddTestStepL(new(ELeave) TS_DummyPPPTest16(_L("Test16")));
	AddTestStepL(new(ELeave) TS_DummyPPPTest17(_L("Test17")));
	AddTestStepL(new(ELeave) TS_DummyPPPTest18(_L("Test18")));
	 
	AddTestStepL(new(ELeave) TS_DummyPPPTest19(_L("Test19")));
	AddTestStepL(new(ELeave) TS_DummyPPPTest20(_L("Test20")));
	 
	 
	 
	AddTestStepL(new(ELeave) TS_DummyPPPTest21(_L("Test21")));
	 
	AddTestStepL(new(ELeave) TS_DummyPPPTest22(_L("Test22")));
	AddTestStepL(new(ELeave) TS_DummyPPPTest23(_L("Test23")));
	AddTestStepL(new(ELeave) TS_DummyPPPTest24(_L("Test24")));
	AddTestStepL(new(ELeave) TS_DummyPPPTest25(_L("Test25")));
	AddTestStepL(new(ELeave) TS_DummyPPPTest26(_L("Test26")));
	AddTestStepL(new(ELeave) TS_DummyPPPTest27(_L("Test27")));
	AddTestStepL(new(ELeave) TS_DummyPPPTest28(_L("Test28")));
	AddTestStepL(new(ELeave) TS_DummyPPPTest29(_L("Test29")));
	AddTestStepL(new(ELeave) TS_DummyPPPTest30(_L("Test30")));
	 
	AddTestStepL(new(ELeave) TS_DummyPPPTest31(_L("Test31")));
	 
	AddTestStepL(new(ELeave) TS_DummyPPPTest32(_L("Test32")));
	AddTestStepL(new(ELeave) TS_DummyPPPTest33(_L("Test33")));
	AddTestStepL(new(ELeave) TS_DummyPPPTest34(_L("Test34")));
	
	AddTestStepL(new(ELeave) TS_DummyPPPTest35(_L("Test35")));
	AddTestStepL(new(ELeave) TS_DummyPPPTest36(_L("Test36")));
	//Added extra for flowon test from dummy ppp nif
	AddTestStepL(new(ELeave) TS_DummyPPPTest37(_L("Test37")));
	// Added for deleting agentdialog.ini file
	AddTestStepL( new(ELeave) CDummyPPPPostDelete());
}


TPtrC TS_DummyPPPSuite::GetVersion( void )
/**
 * Give version information back to Schedultest
 * @return The descriptor of the version
 */
{
	return KTxtVersion();
}
