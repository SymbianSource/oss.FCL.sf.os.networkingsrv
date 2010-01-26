// Copyright (c) 2001-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// This main DLL entry point for the TS_SelfTest.dll
// 
//



// EPOC includes
#include <e32base.h>

// Test system includes
#include "networking/log.h"
#include "networking/teststep.h"
#include "TestStepSelfTest.h"
#include "ExampleTest.h"
#include "networking/testsuite.h"
#include "TestStepSelfTest.h"
#include "TestSuiteSelfTest.h"




// NewTestEtelPacket is exported at ordinal 1
// this provides the interface to allow schedule test
// to create instances of this test suite
EXPORT_C CTestSuiteSelfTest* NewTestSuiteSelfTestL() 
    { 
	return new (ELeave) CTestSuiteSelfTest;
    }

// destructor
CTestSuiteSelfTest::~CTestSuiteSelfTest()
	{
	}

// make a version string available for test system 
_LIT(KTxtVersion,"1.0");
TPtrC CTestSuiteSelfTest::GetVersion( void )
	{
	return KTxtVersion();
	}

// Add a test step into the suite
void CTestSuiteSelfTest::AddTestStepL( CTestStepSelfTest * aPtrTestStep )
	{
	// test steps contain a pointer back to the suite which owns them
	aPtrTestStep->iSelfTestSuite = this; 

	// add the step using tyhe base class method
	CTestSuite::AddTestStepL(aPtrTestStep);
	}


// constructor for ESOCK test suite
// this creates all the ESOCK test steps and
// stores them inside CTestSuiteEsock
void CTestSuiteSelfTest::InitialiseL( void )
	{

	// add test steps
	AddTestStepL( new(ELeave) CExampleTest );
	AddTestStepL( new(ELeave) CExampleTest2 );
	AddTestStepL( new(ELeave) CExampleTest3 );
	AddTestStepL( new(ELeave) CExampleTest4 );
	AddTestStepL( new(ELeave) CExampleTest5 );
	AddTestStepL( new(ELeave) CPanicTest );
	AddTestStepL( new(ELeave) CFlogTest );
	AddTestStepL( new(ELeave) CTimerTest );
	AddTestStepL( new(ELeave) CTimerTest );
	AddTestStepL( new(ELeave) CErrorCodeTest );
	AddTestStepL( new(ELeave) CSchedulerTest );
	}


