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
// This file contains an example Test step implementation 
// This demonstrates the various functions provided
// by the CTestStep base class which are available within
// a test step 
// 
//

// EPOC includes
#include <e32base.h>

// Test system includes
#include "networking/log.h"
#include "networking/teststep.h"
#include "TestStepSelfTest.h"
#include "TestSuiteSelfTest.h"
#include "ExampleTest.h"
#include "comms-infras/commsdebugutility.h"

// Each test step initialises it's own name
CExampleTest::CExampleTest()
{
	// store the name of this test case
	// this is the name that is used by the script file
	iTestStepName = _L("ExampleTest1");
}

// destructor
CExampleTest::~CExampleTest()
{
}

// Each test step must supply a implementation for doTestStepL
enum TVerdict CExampleTest::doTestStepL( void )
{
	// Printing to the console and log file
	Log(_L("Self Test 1"));
	Log(_L("this tests reading from a config file and should pass"));

	// read in a bool value from the config file
	TBool aBoolResult;
	TBool returnValue = GetBoolFromConfig(_L("SectionOne"),_L("Keybool"), aBoolResult);

	// check result
	TESTL(returnValue);
	if ( aBoolResult ) 
		Log(_L("Keybool =true"));
	else
		Log(_L("Keybool =false"));
	
	// check result
	returnValue = GetBoolFromConfig(_L("SectionOne"),_L("Keybool2"), aBoolResult);
	TESTL(returnValue);

	if ( aBoolResult ) 
		Log(_L("Keybool2 =true"));
	else
		Log(_L("Keybool2 =false"));

	// read in a TInt value from the config file
	TInt aIntResult;
	returnValue = GetIntFromConfig(_L("SectionOne"),_L("KeyTInt"),aIntResult);
	TESTL(returnValue);
	if (returnValue) Log(_L("KeyTInt = %d"), aIntResult );
	
	// read in a bool value from the config file
	TPtrC aPtrResult;
	TPtrC* res=&aPtrResult;
	returnValue =GetStringFromConfig(_L("SectionOne"),_L("KeyStr"), aPtrResult);
	TESTL(returnValue);
	if (returnValue) Log(_L("KeyStr  = %S"), res);

	// boolean test expression macros

	// TEST(a) check a boolean expression (a) is true, if not display error, & record fail
	TEST(1);

	// TESTL(a) check a boolean expression (a) is true, if not display error, record fail and LEAVE
	TESTL(1);

	// TESTE(a, b) check a boolean expression (a) is true, if not display error code b and record fail
	TESTE(1, 2);

	// TESTEL(a, b) check a boolean expression (a) is true, if not display error code b, record fail and LEAVE
	TESTEL(1, 2);

	// test steps return a result
	return iTestStepResult;
}

// Each test step initialises it's own name
CExampleTest2::CExampleTest2()
{
	// store the name of this test case
	// this is the name that is used by the script file
	iTestStepName = _L("ExampleTest2");
}

// destructor
CExampleTest2::~CExampleTest2()
{
}

// Each test step must supply a implementation for doTestStepL
enum TVerdict CExampleTest2::doTestStepL( void )
{
	// Printing to the console and log file
	Log(_L("log text from example Test 2"));
	Log(_L("This test should fail but not leave"));

	// display error, & record fail
	TEST(0);

	// test steps return a result
	return iTestStepResult;
}

// Each test step initialises it's own name
CExampleTest3::CExampleTest3()
{
	// store the name of this test case
	// this is the name that is used by the script file
	iTestStepName = _L("ExampleTest3");
}

// destructor
CExampleTest3::~CExampleTest3()
{
}

// Each test step must supply a implementation for doTestStepL
enum TVerdict CExampleTest3::doTestStepL( void )
{
	// Printing to the console and log file
	Log(_L("Self Test 3"));
	Log(_L("this tests leaves with error code 3 and should fail"));

	// this test should leave with error code 3
	TESTEL(0, 3);

	// test steps return a result
	return iTestStepResult;
}

// Each test step initialises it's own name
CExampleTest4::CExampleTest4()
{
	// store the name of this test case
	// this is the name that is used by the script file
	iTestStepName = _L("ExampleTest4");
}


// Each test step must supply a implementation for doTestStepL
enum TVerdict CExampleTest4::doTestStepL( void )
{
	// Printing to the console and log file
	Log(_L("Self Test 4"));
	Log(_L("this tests should pass"));

	// this test should pass
	TESTEL(1, 3);

	// test steps return a result
	return iTestStepResult;
}

// Each test step initialises it's own name
CExampleTest5::CExampleTest5()
{
	// store the name of this test case
	// this is the name that is used by the script file
	iTestStepName = _L("ExampleTest5");
}


// Each test step must supply a implementation for doTestStepL
enum TVerdict CExampleTest5::doTestStepL( void )
{
	Log(_L("Self Test 5"));
	Log(_L("this tests should fail (not leave)"));

	// test steps return a result
	return iTestStepResult = EFail;
}

// Each test step initialises it's own name
CPanicTest::CPanicTest()
{
	// store the name of this test case
	// this is the name that is used by the script file
	iTestStepName = _L("PanicTest");
}


// Each test step must supply a implementation for doTestStepL
enum TVerdict CPanicTest::doTestStepL( void )
{
	Log(_L("Panic Test "));
	Log(_L("this tests should panic (not leave)"));

	User::Panic(_L("this is the panic test"),1);

	// this test should fail regardless of verdict here !
	return iTestStepResult = EPass;
}

// Each test step initialises it's own name
CTimerTest::CTimerTest()
{
	// store the name of this test case
	// this is the name that is used by the script file
	iTestStepName = _L("TimerTest");
}


// Each test step must supply a implementation for doTestStepL
enum TVerdict CTimerTest::doTestStepL( void )
{
	Log(_L("this test runs forever (so should be caught by the guard timer)"));

	FOREVER
		{;}
}

// Each test step initialises it's own name
CFlogTest::CFlogTest()
{
	// store the name of this test case
	// this is the name that is used by the script file
	iTestStepName = _L("FlogTest");
}


// Each test step must supply a implementation for doTestStepL
enum TVerdict CFlogTest::doTestStepL( void )
{

	_LIT(KTestFloggerLogFile,"FLOGGERTEST.TXT");
	_LIT(KTestFloggerLogFolder,"FLOGGERTEST");

	Log(_L("FLog Test "));
	RFileLogger::Write(KTestFloggerLogFolder(),KTestFloggerLogFile(),EFileLoggingModeOverwrite,_L("TEST FLOGGER"));
	// this test should fail regardless of verdict here !
	return iTestStepResult = EPass;
}

// Each test step initialises it's own name
CErrorCodeTest::CErrorCodeTest()
{
	// store the name of this test case
	// this is the name that is used by the script file
	iTestStepName = _L("ErrorCodeTest");
}


// Each test step must supply a implementation for doTestStepL
enum TVerdict CErrorCodeTest::doTestStepL( void )
{

	Log(_L("Error code Test "));

	TESTE(0,103);
	TESTE(0,54);
	TESTE(0,-2);

	return iTestStepResult = EPass;
}

CSchedulerTest::CSchedulerTest()
{
	iTestStepName = _L("SchedulerTest");
}
enum TVerdict CSchedulerTest::doTestStepL( void )
{
	iTimer.CreateLocal();
	iControl->ReStart();
	iState=EStepOne;
	CActiveScheduler::Start();
	
	return EPass;
}
TInt CSchedulerTest::CallStateMachine()
{
	TInt activate(0);
	switch (iState)
		{
		case EStepOne:
			iTimer.After(*iStatus, 10000000);
			activate=1;
			iState=EStepTwo;
			break;
		case EStepTwo:
			iTimer.After(*iStatus, 10000000);
			activate=1;
			iState=EStepThree;
			break;
		case EStepThree:
			CActiveScheduler::Stop();
			break;
		}
	return activate;
}

