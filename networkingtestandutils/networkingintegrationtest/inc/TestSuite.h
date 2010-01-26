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
* This contains CTestSuite which is the Abstract base class
* for all the TestSuite DLLs
* 
*
*/



/**
 @file TestSuite.h
*/

#if (!defined __TESTSUITE_H__)
#define __TESTSUITE_H__


#include "networking/log.h"

/**
Maximum length for test suite name
@internalComponent
*/
#define MAX_LEN_TEST_SUITE_NAME 55


class CTestStep;

class CTestSuite : public CBase
/**
Abstract base class for all test suites
@internalAll
*/
{
public:
	/** second phase constructor */
	IMPORT_C void ConstructL( void );

	/** destructor */
	IMPORT_C virtual ~CTestSuite();

    IMPORT_C void OverrideSuiteName(const TDesC& aNewName);

	/** add a test step to the suite */
	IMPORT_C void AddTestStepL( CTestStep * ptrTestStep );

	/** public interface to run test steps */
	IMPORT_C enum TVerdict DoTestStep( TDesC &step, TDesC &config );

	/** public interface to run unit test */
	IMPORT_C virtual enum TVerdict DoTestUnit( TDesC &config );

	/** 
	Test suite second phase initialisation
	pure-virtual, all test suites must implement this
	this is called from CTestSuite::ConstructL 
	*/
	virtual void InitialiseL( void ) = 0;

	IMPORT_C void Log( TRefByValue<const TDesC16> format, ... );
	IMPORT_C void Log( TInt aSeverity, TRefByValue<const TDesC16> format, ... );

	IMPORT_C void LogExtra(const TText8* aFile, TInt aLine, TInt aSeverity,
		TRefByValue<const TDesC> aFmt,...);

	/**
	this should be a pure virtual so every test ddl
	has to provide a version but for now defaults to ?.?
	*/
	IMPORT_C virtual TPtrC GetVersion( void );

	/** set severity */
	IMPORT_C void SetSeverity( TInt aSeverity);

	/** get severity level */
	IMPORT_C TInt Severity();

	/** set */
	IMPORT_C void SetLogSystem(CLog *aLogger);

	/** log data buffer */
	TBuf<32384>		iLogData;
protected:
	/** test functions */
	IMPORT_C void testBooleanTrueL( TBool aCondition, char* aFile, TInt aLine );

private:

    /** 
        the name of this suite, is used for reporting only.
        Is set automatically during loading test suite dll.
        Can be overriden, but there is no need.
    */
	TBuf<MAX_LEN_TEST_SUITE_NAME> iSuiteName;

	/** severity level */
	TInt iSeverity;

	/** File logging system */
	CLog *iLogger;

	/** array of pointers to the test steps in this suite */
	CArrayPtrFlat<CTestStep> *	iArrayTestSteps;
};

#endif /* __TESTSUITE_H__ */
