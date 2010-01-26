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
// This contains CTestSuite which is the base class for all the TestSuite DLLs
// 
//

/**
 @file TestSuite.cpp
*/

// EPOC includes
#include <e32base.h>

// Test system includes
#include "../inc/Log.h"
#include "../inc/TestStep.h"
#include "../inc/TestSuite.h"

EXPORT_C CTestSuite::~CTestSuite()
/**
Test suite destructor.
This destroys all the test steps contained in this class and
in classes drived from it.
*/
{
	// free all test steps
	if (iArrayTestSteps)
		iArrayTestSteps->ResetAndDestroy();

	// free the dynamic array used for test steps
	delete iArrayTestSteps;

}

EXPORT_C void CTestSuite::ConstructL( )
/**
Construct a test suite object and initialise any drived test suites

@note This class is used as a base class for all test suites.  
It contains all the functionality common to the test suites 
that is independent of any particular test.
*/
{
	// create a new Array to store the test steps in
	iArrayTestSteps = new(ELeave) CArrayPtrFlat<CTestStep>(1);

	// default severity
	SetSeverity(ESevrAll);
	iLogger = NULL;

    //-- initialize suite name buffer
    new((TAny*)&iSuiteName) TBuf<MAX_LEN_TEST_SUITE_NAME>;
    iSuiteName.Copy(_L("Uninitialized Suite name"));

	// initialise the derived test suites
	InitialiseL();
}

/**
version string constant
*/ 
_LIT(KTxtVersion,"?.?");
EXPORT_C TPtrC CTestSuite::GetVersion( void )
/**
Make a version string available for test system.

@return A string representation of the current version.
@note This method should be overridden in the test suites.
This is not a pure virtual so as to be backward compatible with some early test suites which did not implement this method.
*/
	{
	return KTxtVersion();
	}

EXPORT_C void CTestSuite::AddTestStepL( CTestStep * aTestStep )
/**
Add a test step into the suite.

@param aTestStep A pointer to a CTestStep which is added into the test suite.
*/
{
	__ASSERT_ALWAYS(aTestStep,User::Panic(_L("CTestSuite::AddTestStepL"),KErrArgument));
	// test steps contain a pointer back to the suite which owns them
	aTestStep->iSuite = this;
	// add the step, order is not important, so at position 1
	iArrayTestSteps->AppendL(aTestStep, 1);
	}

EXPORT_C enum TVerdict CTestSuite::DoTestStep( TDesC &step, TDesC &config )
/**
Run the test step specified.

@param step The name of the test step to be run.
@param config Configuration values file name (*.ini), as supplied in the test script.
@return The test result.
@note This function traps leaves in the test steps, so should not ever leave.
*/
{
	//	get the number of tests available in this suite
	TInt NoOfTests = iArrayTestSteps->Count();

	// search the available test steps for the required one
	for ( TInt i=0; i < NoOfTests; i++ )
	{
		if ( iArrayTestSteps->At(i)->iTestStepName.MatchF( step )!= KErrNotFound )
		{

			// reset the log data
			iLogData.Zero();

			// found required test so initialise to PASS
			iArrayTestSteps->At(i)->iTestStepResult = EPass;

			// pass the config file info into the test step
			iArrayTestSteps->At(i)->LoadConfig( config );

			// assume it's going to work
			enum TVerdict result = EPass;

			// And assume that it is going to leave
			TBool testLeft = ETrue;
			// execute test step preamble (if any) inside a trap
			TRAPD( rPreAmble, result = iArrayTestSteps->At(i)->doTestStepPreambleL( ) ; testLeft = EFalse)

			if (testLeft)
				{
				ERR_PRINTF4(_L("Warning Test step:%S PreAmble in suite:%S left with %d!"), &step, &iSuiteName, testLeft );
				result = EFail;
				}
			else if (rPreAmble!= KErrNone)
				{
				ERR_PRINTF4(_L("Warning Test step:%S PreAmble in suite:%S failed with %d!"), &step, &iSuiteName, rPreAmble );
				result = EFail;
				}
			
			// only continue if the preamble passed
			if (result == EPass)
			{
				testLeft = ETrue;
				// now execute test step inside a trap
				TRAPD( r, result = iArrayTestSteps->At(i)->doTestStepL( ); testLeft = EFalse)

				if (testLeft)
					{
					ERR_PRINTF4(_L("Warning Test step:%S in suite:%S left with %d!"), &step, &iSuiteName, r );
					result = EFail;
					}
				
				else if (r!= KErrNone)
					{
					ERR_PRINTF4(_L("Warning Test step:%S in suite:%S failed with %d!"), &step, &iSuiteName, result );
					result = EFail;
					}

				testLeft = ETrue;
				// execute test step postamble (if any) inside a trap
				enum TVerdict PostAmbleResult = EPass;
				TRAPD( rPostAmble, PostAmbleResult = iArrayTestSteps->At(i)->doTestStepPostambleL( ); testLeft = EFalse)

				// The postamble result is only significant if the test has passed
				if ( result == EPass ) result = PostAmbleResult;

				if (testLeft)
					{
					ERR_PRINTF4(_L("Warning Test step:%S PostAmble in suite:%S left with %d!"), &step, &iSuiteName, testLeft );
					result = EFail;
					}
				else if (rPostAmble!= KErrNone)
					{
					ERR_PRINTF4(_L("Warning Test step:%S PostAmble in suite:%S failed with %d!"), &step, &iSuiteName, rPostAmble );
					result = EFail;
					}
			}

			// clean up Config data object
			iArrayTestSteps->At(i)->UnLoadConfig();

			return result;

		}
	}

	// suite has been searched but test step not found so
 	ERR_PRINTF3(_L("Failed to find test step:%S in suite:%S"), &step, &iSuiteName );

	return ETestSuiteError;

}

EXPORT_C enum TVerdict CTestSuite::DoTestUnit( TDesC &config )
/**
Run the unit test

@param config Configuration values file name (*.ini), as supplied in the test script.
@return The test result.
*/
{
	TPtrC step,verdict;
	//TIntegrationTestLog16Overflow iOverflow16;
	TVerdict result = EPass;
	TInt	iPass(1);
	TInt	iFail(0);
	TInt	iInconclusive(0);
	TInt	iTestSuiteError(0);
	TInt	iAbort(0);
	TInt	iTotal(1);

	//	get the number of tests available in this suite
	TInt NoOfTests = iArrayTestSteps->Count();

	// search all available test steps
	for ( TInt i=0; i < NoOfTests; i++ )
	{
		step.Set(iArrayTestSteps->At(i)->iTestStepName);

		result = DoTestStep(step, config);

		verdict.Set(iLogger->TestResultText( result ));
		if(verdict.Ptr() == _S("FAIL"))
		{
			iPass =0;
			iFail=1;
		}


		iLogger->LogResult(result, _L("Test Result for %S is %s"), &step, verdict.Ptr());
		iLogger->LogBlankLine();
		iLogger->LogBlankLine();

		iLogger->LogExtra(((TText8*)(__FILE__)), (__LINE__), ESevrErr,_L("Test Results Summary ") );
		iLogger->LogExtra(((TText8*)(__FILE__)), (__LINE__), ESevrErr,_L("-------------------- ") );
		iLogger->LogExtra(((TText8*)(__FILE__)), (__LINE__), ESevrErr,_L("Passed            :%d"),  iPass);
		iLogger->LogExtra(((TText8*)(__FILE__)), (__LINE__), ESevrErr,_L("Failed            :%d"),  iFail);
		iLogger->LogExtra(((TText8*)(__FILE__)), (__LINE__), ESevrErr,_L("Inconclusive      :%d"),  iInconclusive);
		iLogger->LogExtra(((TText8*)(__FILE__)), (__LINE__), ESevrErr,_L("Test suite errors :%d"),  iTestSuiteError);
		iLogger->LogExtra(((TText8*)(__FILE__)), (__LINE__), ESevrErr,_L("Aborted           :%d"),  iAbort);
		iLogger->LogExtra(((TText8*)(__FILE__)), (__LINE__), ESevrErr,_L("Total             :%d"),  iTotal);

	}

	return result;
}

EXPORT_C void CTestSuite::Log( TRefByValue<const TDesC16> format, ... )
/**
General purpose formated logging output.

@param format Printf style formated output string.
*/
	{

	VA_LIST aList;
	VA_START( aList, format );

	if(iLogger) iLogger->Log(format, aList);

	VA_END( aList );
	}

EXPORT_C void CTestSuite::Log( TInt aSeverity, TRefByValue<const TDesC16> format, ... )
/**
General purpose formated logging output.

@param aSeverity The current log severity level.
@param format Printf style formated output string.
*/
{
	VA_LIST aList;
	VA_START( aList, format );

	if( aSeverity & Severity())
	{
		if(iLogger) iLogger->Log(format, aList);
	}

	VA_END( aList );
}

EXPORT_C void CTestSuite::LogExtra(const TText8* aFile, TInt aLine, TInt aSeverity,
		TRefByValue<const TDesC> aFmt,...)
/**
General purpose formated logging output.

@param aFile The current file name.
@param aLine The current line number.
@param aSeverity The current log severity level.
@param aFmt Printf style formated output string.
*/
	{
	VA_LIST aList;
	VA_START( aList, aFmt );

	if( aSeverity & Severity())
	{
		if(iLogger)
			{
			iLogger->LogExtra(aFile, aLine, aSeverity, aFmt, aList);
			}
	}

	VA_END( aList );
	}


EXPORT_C void CTestSuite::testBooleanTrueL( TBool aCondition, char* aFile, TInt aLine )
/**
Check the boolean expression is true,
if not record error and then leave.

@param aCondition The condition to test.
@param aFile The current file name.
@param aLine The current line number.
@note This fuunction can leave.
*/
	{

	// check condition
	if (aCondition)
		return;

	// convert filename for log
	TBuf<MAX_LOG_FILENAME_LENGTH> fileName;
	fileName.Copy(TPtrC8((TText8*)aFile));

	// display a log message
 	ERR_PRINTF3(_L("Test Failed in file:%S line:%d"), &fileName, aLine);

	// leave with error code
	User::Leave(TEST_ERROR_CODE);

	}

EXPORT_C void CTestSuite::SetSeverity( TInt aSeverity)
/**
Set the current log severity level.

@param aSeverity The new log level severity.
*/
{
	iSeverity = aSeverity;
}

EXPORT_C TInt CTestSuite::Severity()
/**
Get the current log severity level.
*/
{
	return iSeverity;
}

EXPORT_C void CTestSuite::SetLogSystem(CLog *aLogger)
/**
Set logging system.

@param aLogger Loads the test suite with a pointer to the log system provided by the test framework.
*/
	{
	iLogger = aLogger;
	}

/**
  Set a new suite name.
  Actually, there is no need for test suite derivatives to override existing suite name, because it is
  set automatically during loading test suite dll.

  @param    aNewName new name for this test suite
*/
EXPORT_C void CTestSuite::OverrideSuiteName(const TDesC& aNewName)
    {
     iSuiteName.Copy(aNewName);
    }
