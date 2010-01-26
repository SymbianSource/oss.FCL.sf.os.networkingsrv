/**
* Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* This contains CTestCase which is the base class for all the Testcases
* 
*
*/



/**
 @file TestStep.h
*/

#if (!defined __TESTSTEP_H__)
#define __TESTSTEP_H__

#include "cinidata.h"
#include "networking/log.h"

class CTestSuite;

/**
Maximum length for test step name
@internalComponent
*/
#define MAX_LEN_TEST_STEP_NAME 55

class CTestStep : public CBase
/**
CtestStep is an abstract base class for all test steps. Each test step 
consists of a class derived from CTestStep, which implements doTestStepL.  
Common test step functionality, which is required for a particular suite, 
can be implemented using an intermediate class.
@internalComponent
*/
{
public:

	// Constructor 
	IMPORT_C CTestStep(const TDesC &aName); 
	IMPORT_C CTestStep();

	/** destructor */
	IMPORT_C virtual ~CTestStep();

	/** 
	Every test step must implement one of these.
	This function contains the main code for a test step 
	and returns a test result.
	*/
	virtual enum TVerdict doTestStepL( void ) = 0;
	
	/** Tests may optionaly implement pre amble. */
	IMPORT_C virtual enum TVerdict doTestStepPreambleL( void );

	/** Tests may optionaly implement post amble. */
	IMPORT_C virtual enum TVerdict doTestStepPostambleL( void );

	/** the name of the test step */
	TBuf<MAX_LEN_TEST_STEP_NAME> iTestStepName;

	/** Pointer to test suite which owns this test. */
	CTestSuite * iSuite;

	/** The current test step verdict. */
	TVerdict iTestStepResult;

	/** initialise the config parameter system. */
	IMPORT_C void LoadConfig( TDesC &config );

	IMPORT_C void UnLoadConfig( void );


protected:

	/** read bool value from the Config file functions */
	IMPORT_C TBool GetBoolFromConfig(const TDesC &aSectName,const TDesC &aKeyName,TBool &aResult);

	/** read int value from the Config file functions */
	IMPORT_C TBool GetIntFromConfig(const TDesC &aSectName,const TDesC &aKeyName,TInt &aResult);

	/** read string value from the Config file functions */
	IMPORT_C TBool GetStringFromConfig(const TDesC &aSectName,const TDesC &aKeyName,TPtrC &aResult);

	/** test function */
	IMPORT_C void testBooleanTrue( TBool aCondition, char* aFile, TInt aLine );

	/** test function */
	IMPORT_C void testBooleanTrueL( TBool aCondition, char* aFile, TInt aLine );

	/** test function */
	IMPORT_C void testBooleanTrueWithErrorCode( TBool aCondition, TInt errorCode  , char* aFile, TInt aLine );

	/** test function */
	IMPORT_C void testBooleanTrueWithErrorCodeL( TBool aCondition, TInt errorCode  , char* aFile, TInt aLine );

	/** test function */
	IMPORT_C void teste( TBool aCondition, TInt value2 );

	/** test function */
	IMPORT_C void TestCheckPointCompareL(TInt aVal,TInt aExpectedVal, 
										 const TDesC& aText, char* aFile,TInt aLine);

	/** printf format log */
	IMPORT_C void Log( TRefByValue<const TDesC16> format, ... );

	/** printf format log */
	IMPORT_C void Log( TInt aSeverity, TRefByValue<const TDesC16> format, ... );

	/** printf format log */
	IMPORT_C void LogExtra(const TText8* aFile, TInt aLine, TInt aSeverity,
		TRefByValue<const TDesC> aFmt,...);

	IMPORT_C TPtrC EpocErrorToText(TInt aError);

	/** Config file data. */
	CIniData *	iConfigData;

	/** Print buffer used by ErrorToText. */
	TBuf <20> iPrnBuf;

private:
	/** True if ConfigData has been loaded. */
	TBool   iConfigDataAvailable;
};

class MControlNotify
/**
@internalComponent
*/
{
public:
	virtual TInt CallStateMachine() = 0;
};

class CActiveControl : public CActive
/**
@internalComponent
*/
{
public:
	static CActiveControl* NewL(MControlNotify* aControl);
	CActiveControl(MControlNotify* aControl);
	~CActiveControl();
	void ConstructL();
	virtual void RunL();
	virtual void DoCancel();
	IMPORT_C void ReStart();
	inline TRequestStatus* Status() {return &iStatus;}
private:
	MControlNotify* iControl;
};

class CTestActiveStep : public CTestStep, public MControlNotify
/**
@internalComponent
*/
{
public:
	// Constructor 
	IMPORT_C CTestActiveStep(const TDesC &aName); 
	IMPORT_C CTestActiveStep();

	/** destructor */
	IMPORT_C virtual ~CTestActiveStep();

	IMPORT_C virtual enum TVerdict doTestStepPreambleL( void );
	IMPORT_C virtual enum TVerdict doTestStepPostambleL( void );

protected:
	CActiveControl* iControl;
	TRequestStatus* iStatus;

private:
	CActiveScheduler* iScheduler;

};


/**
Checks the expression (a) is true, if not, 
then displays error information, records a test verdict of fail and leaves.
@internalAll
*/
#define TESTL(a) testBooleanTrueL((a), __FILE__, __LINE__) 

/**
Checks the expression (a) is true, if not, 
then display error information and records a test verdict of fail.
@internalAll
*/
#define TEST(a)  testBooleanTrue((a), __FILE__, __LINE__) 

/**
Checks the expression (a) is true, if not, 
then displays error code b, and records a fail verdict.
@internalAll
*/
#define TESTE(a, b) testBooleanTrueWithErrorCode((a), (b), __FILE__, __LINE__) 

/**
Checks the expression (a) is true, if not, 
then displays error code b, and records a fail verdict and leaves.
@internalAll
*/
#define TESTEL(a, b) testBooleanTrueWithErrorCodeL((a), (b), __FILE__, __LINE__)  

/**
Check the value against an expected value.
If not record error and error code.
@internalComponent
*/
#define TEST_CHECKL(p1, p2, p3) TestCheckPointCompareL((p1), (p2), (p3), __FILE__, __LINE__)	

/**
leave error code
@internalComponent
*/
#define TEST_ERROR_CODE 84	


#endif /* __TESTSTEP_H__ */
