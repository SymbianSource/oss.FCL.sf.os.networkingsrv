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
*
*/



/**
 @file parseLine.h
 @internalComponent
*/

#if (!defined __PARSELINE_H__)
#define __PARSELINE_H__


/**
extern global data - pointer to Log system
*/
GLREF_D	class CLog *		pLogSystem;		

/**
extern global data - pointer to test utils
*/
GLREF_D	class CTestUtils *	pTestUtils;		

/**
extern global data - running in automated, non-interactive, stop-for-nothing mode
*/
GLREF_D TBool				automatedMode;	

/**
extern global data - pointer to console
*/
GLREF_D CConsoleBase *		console;


class CSuiteDll : public CBase
/**
This class is sued for storing information on 
test suite DLLs currently loaded
@internalComponent
*/
{
public:
	static CSuiteDll* NewL( const TDesC& aName );
	~CSuiteDll();

    TBool SuiteNameMatch(const TDesC& aSuiteName) const;

public:
	
	RLibrary	                iLibrary;   ///< DLL Library
	CTestSuite*	                iTestSuite;	///< pointer to Test suite object
	TBuf<MAX_LEN_SUITE_NAME>	iName;		///< name of the test suite dll

	
private:
	CSuiteDll( const TDesC& aName );
	void ConstructL();
		
};

class CStepData
/**
@internalComponent
*/
{
public:
	TBuf<100> step; 
	TBuf<100> config;
	CSuiteDll * iSuite;
};

class CParseLine : public CBase
/**
@internalComponent
*/
{
public:
	static CParseLine* NewL(CScript * ptr);
	~CParseLine();	
	CParseLine();
	void ConstructL(CScript * ptr);

	static TInt threadfn ( TAny * ptr );

	/** process a line of script */
	void ProcessLineL(const TPtrC8 &narrowline, TInt8 lineNo);	

	/** record of the current test step name */
	TBuf<MAX_LEN_TEST_STEP_NAME> iCurrentStepName;

private:

	/** script keyword command */
	void scriptPrint( const TDesC& Text );

	/** script keyword command */
	void RunScriptL( const TDesC& Text );

	void RunTestStep( const TDesC& Text );
	void RunPanicTestStep( const TDesC& Text );
	void RunUtil( const TDesC& Text );
	void LoadSuiteL( const TDesC& Text );
	void Reboot(void);
	enum TVerdict DoTestNewThread(TPtrC suite, TPtrC step, TInt GuardTimerValue, const TPtrC &config);
	enum TVerdict DoTestCurrentThread(TPtrC suite, TPtrC step, TPtrC config);
	enum TVerdict DoPanicTest(TPtrC suite, TPtrC step, TInt GuardTimerValue, const TPtrC &config);
	void Unload(const TDesC& Text);
	void HeapMark(void);
	void HeapCheck(void);
	void RequestMark(void);
	void RequestCheck(void);
	void HandlesMark(void);
	void HandlesCheck(void);
	void RunCed( const TDesC& Text );
	void RunProgram( const TDesC& Text );
	void TestComplete( const TDesC& Text );
	void Delay(  const TDesC& Text );
	void SetSeverity(  const TDesC& Text );
	void LogSettings(  const TDesC& Text );

	// data members
	/** the file system */
	RFs		iTheFs;

	/** test path */
	TPath	iTheTestPath;

	TInt	iReqsAtStart;
	TInt	iProcessHandleCountBefore;
	TInt	iThreadHandleCountBefore;

	/** to minimise risk of thread name clashes on EKA2 */
	TUint	iThreadNameSuffix;		

	/** the current test result */
	enum TVerdict iTestVerdict;

	/** owner script */
	CScript * iScript;

	/** severity level */
	TInt iSeverity;

	/** 
	array of CSuiteDll objects which contain 
	information on the current loaded test suite DDLs 
	*/
	CArrayPtrFlat<CSuiteDll> *	iArrayLoadedSuiteDll;

	/**
	Flag to indicate that there has been an error
	and the user has selected to skip the rest of teh script until a
	test result statement is found in the log
	*/
	TBool   iBreakOnError;

};

#endif /* __PARSELINE_H__ */
