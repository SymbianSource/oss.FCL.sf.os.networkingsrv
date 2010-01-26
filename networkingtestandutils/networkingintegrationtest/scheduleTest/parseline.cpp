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
// This module contains CParseLine and CSuiteDll classes
// CParseLine contains the functions required to execute
// a line of test script file.
// CSuiteDll objects contains information about test suite
// dlls that have been loaded.
// 
//

/**
 @file parseLine.cpp
*/

// system includes
#include <e32base.h>
#include <e32cons.h>
#include "f32file.h"

// test system includes
#include "../inc/Log.h"
#include "../inc/TestUtils.h"
#include "../inc/TestStep.h"
#include "../inc/TestSuite.h"
#include "script.h"
#include "parseline.h"

/** 
DLL fullpath 
@internalComponent
*/
_LIT(KTxtDLLpath,"c:\\;c:\\system\\libs\\;d:\\;d:\\system\\libs\\");

#ifdef __WINS__
/** 
Holds ced_exe DLL string constant 
@internalComponent
*/
_LIT(KTxtcedDLL,"ced_exe.dll");
#endif

/** 
Thread name format 
@internalComponent
*/
_LIT(KThreadNameFmt, "DoTestThread_%d");

/** 
Maximum test thread heap size 
@internalComponent
*/
const TInt KMaxTestThreadHeapSize = 0x10000;

/**
extern global data - pointer to test utils
@internalComponent
*/
GLDEF_D	CTestUtils *		pTestUtils;

/**
extern global data - pointer to Log system
@internalComponent
*/
GLDEF_D CLog *				pLogSystem;

/**
extern global data - running in automated, non-interactive, stop-for-nothing mode
@internalComponent
*/
GLDEF_D TBool				automatedMode = EFalse;

/**
extern global data - pointer to console
@internalComponent
*/
GLDEF_D CConsoleBase *		console;


CParseLine::CParseLine():iTestVerdict(EPass)
/**
constructor
*/
	{}
void CParseLine::ConstructL(CScript * ptr)
/**
create a new Array to store the test steps in
*/
	{
	// create a new Array to store the test steps in
	iArrayLoadedSuiteDll = new(ELeave) CArrayPtrFlat<CSuiteDll>(1);
	
	iScript = ptr;
	iSeverity = ESevrAll;
	iBreakOnError = EFalse;
	iThreadNameSuffix = User::TickCount();
	}

CParseLine* CParseLine::NewL( CScript * ptr)
/**
NewL static constructor
*/
	{
	CParseLine * self = new(ELeave) CParseLine;
	CleanupStack::PushL(self);
	self->ConstructL(ptr);
	CleanupStack::Pop();
	return self;
	}

CParseLine::~CParseLine()
/**
Delete all objects in iArrayLoadedSuiteDll.
This will unload any loaded test suite DLLS.
*/
	{
	
	// unload DLLs and their records
	if (iArrayLoadedSuiteDll)
		{
		// delete all objects in iArrayLoadedSuiteDll
		// the destructors will unload any loaded DLLS
		iArrayLoadedSuiteDll->ResetAndDestroy();
		delete iArrayLoadedSuiteDll;
		}
	
	}

void CParseLine::ProcessLineL(const TPtrC8 &aNrrowline, TInt8 lineNo)
/**
Process a line from the script file

@param aNrrowline The line to be parsed
@param lineNo The current line number
*/
	{
	// make a local unicode buffer
	TBuf<MAX_SCRIPT_LINE_LENGTH> LineBuf;
	//If the lenght of the command line is more than MAX_SCRIPT_LINE_LENGTH=200
	//Igore the line with Warning and the test result would be Inconclusive	
	if (aNrrowline.Length() > LineBuf.MaxLength())
	{
		TLex8 lex(aNrrowline);
		// start at the begining
		TPtrC8 token=lex.NextToken();
		TInt firstChar = aNrrowline[0];
		if(firstChar == '\r' || firstChar == '\n' || firstChar == '#' || firstChar == '/' || token.CompareF((TPtrC8((const TText8 *)("PRINT")))) == 0 || token.CompareF((TPtrC8((const TText8 *)("PAUSE_AT_END")))) == 0 || token.CompareF((TPtrC8((const TText8 *)("PAUSE")))) == 0)
		{
			// ignore command line	with comments	
		}
		else
		{
			pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("WARNING : Command line too Long (Exceeds length of 200)  Line:%d"), lineNo);
			iTestVerdict = EInconclusive;
		}
		
		return;
	}

	LineBuf.Fill( '\0',MAX_SCRIPT_LINE_LENGTH);
	
	// convert the narrow script file to Unicode
	TFileName testnameU;
	testnameU.Copy(aNrrowline);
	
	// find the end of the line
	TInt end= testnameU.Locate('\n');
	
	// copy the line into LineBuf
	if ((end != -1) && (end < MAX_SCRIPT_LINE_LENGTH))
		LineBuf = testnameU.Left(end-1);
	else
		LineBuf = testnameU;
	
	// the parser relies on spaces between tokens. Commas are
	// allowed but are just replaced with spaces
	while ( LineBuf.Locate(TChar(',')) != KErrNotFound )
		{
		// found a comma so replave with space
		LineBuf.Replace(LineBuf.Locate(TChar(',')),1,_L(" "));
		}
	
	// for debugging display the line with a line no
#ifdef SCRIPT_DEBUG
	pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("Line:%d %s "), lineNo, LineBuf.Ptr()  );
#endif
	
	// if there has been a failure and the user has selected
	// x then the next commands in the script are skipped until
	// a test complete statement is found
	if ( iBreakOnError )
		{
		if (LineBuf.Find(_L("TEST_COMPLETE"))==0)
			{
			TestComplete( LineBuf );
			
			// reset flag now test complete found
			iBreakOnError = EFalse;
			}
		
		// do not process the rest of the line
		return;
		}
	
	// check the line for command keywords
	if ((LineBuf.Find(_L("//"))==0) || (LineBuf.Find(_L("#"))==0))
		{
		// ignore comments
		}
	else
		{
		// use Tlex to decode the cmd
		TLex lex(LineBuf);
		// start at the begining
		TPtrC token=lex.NextToken();
		
		if (token.CompareF(_L("LOAD_SUITE"))==0)
			{
			LoadSuiteL( LineBuf );
			}
		else if (token.CompareF(_L("RUN_SCRIPT"))==0)
			{
			RunScriptL( LineBuf );
			}
		else if (token.CompareF(_L("RUN_TEST_STEP"))==0)
			{
			RunTestStep( LineBuf );
			}
		else if (token.CompareF(_L("RUN_PANIC_STEP"))==0)
			{
			RunPanicTestStep( LineBuf );
			}
		else if (token.CompareF(_L("RUN_UTILS"))==0)
			{
			RunUtil( LineBuf );
			}
		else if (token.CompareF(_L("CED"))==0)
			{
			RunCed( LineBuf );
			}
		else if (token.CompareF(_L("RUN_PROGRAM"))==0)
			{
			RunProgram( LineBuf );
			}
		else if (token.CompareF(_L("UNLOAD"))==0)
			{
			Unload(LineBuf);
			}
		else if (token.CompareF(_L("HEAP_MARK"))==0)
			{
			HeapMark();
			}
		else if (token.CompareF(_L("HEAP_MARKEND"))==0)
			{
			HeapCheck();
			}
		else if (token.CompareF(_L("REQUEST_MARK"))==0)
			{
			RequestMark();
			}
		else if (token.CompareF(_L("REQUEST_CHECK"))==0)
			{
			RequestCheck();
			}
		else if (token.CompareF(_L("HANDLES_MARK"))==0)
			{
			HandlesMark();
			}
		else if (token.CompareF(_L("HANDLES_CHECK"))==0)
			{
			HandlesCheck();
			}
		else if (token.CompareF(_L("PRINT"))==0)
			{
			scriptPrint( LineBuf );
			}
		else if (token.CompareF(_L("DELAY"))==0)
			{
			Delay( LineBuf );
			}
		else if (token.CompareF(_L("SEVERITY"))==0)
			{
			SetSeverity( LineBuf );
			}
		else if (token.CompareF(_L("PAUSE_AT_END"))==0)
			{
			pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("Pause at end enabled "));
			iScript->iPauseAtEnd = ETrue;
			}
		else if (token.CompareF(_L("MULTITHREAD"))==0)
			{
			pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("Multithread operation enabled "));
			iScript->iMultThread = ETrue;
			}
		else if (token.CompareF(_L("SINGLETHREAD"))==0)
			{
			pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("Single thread operation enabled "));
			iScript->iMultThread = EFalse;
			}
		else if (token.CompareF(_L("PAUSE"))==0)
			{
			iScript->Pause();
			}
		else if (token.CompareF(_L("BREAK_ON_ERROR"))==0)
			{	// if the current test verdict is not PASS
			// give the user the chance to quit
			if ( iTestVerdict != EPass )
				iBreakOnError = iScript->BreakOnError();
			}
		else if (token.CompareF(_L("TEST_COMPLETE"))==0)
			{
			// use Tlex to decode the cmd line
			TestComplete( LineBuf );
			}
		else if (token.CompareF(_L("LOG_SETTINGS"))==0)
			{
			// use Tlex to decode the cmd line
			LogSettings( LineBuf );
			}
		else if (LineBuf.Length()==0)
			{
			// ignore blank lines
			}
		else
			{
			// failed to decode line
			pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("Error in script line:%d - %s "), lineNo, LineBuf.Ptr()  );
			}
		}
	
}

void CParseLine::TestComplete( const TDesC& Text )
/**
This function implements the script test complete keyword

@param The contents of Text are added to the log file with the result
*/
	{
	// use Tlex to decode the cmd line
	TLex lex(Text);
	
	// start at the begining
	TPtrC token=lex.NextToken();
	
	// step over the keyword
	token.Set(lex.NextToken());
	
	if (token.Length() !=0 )
		iCurrentStepName = token;
	
	// Test cases must be careful to clean up after themselves - any stray signals generated in a test case 
	// lead here to automatic failure of the step. They are caught here to allow further cases to run 
	// without crashing.
	while (RThread().RequestCount()!=0)
	{
		pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("Warning! Test case generated stray signal! (cleaning up)") );
		User::WaitForAnyRequest();
		iTestVerdict = EFail;
	}

	// add the current result to the script
	iScript->AddResult( iTestVerdict );
	
	// reset for next test
	iTestVerdict = EPass;
	}

void CParseLine::scriptPrint(  const TDesC& Text )
/**
This function implements the script PRINT Keyword

@param Text The text to be added to the log file 
*/
	{
	// display the text after the PRINT and 1 space = 6
	pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("%s "), (Text.Ptr()+6) );
	}

void CParseLine::Delay(  const TDesC& Text )
/**
This function implements the script Delay Keyword

@param Text contains the time to delay in milliseconds
*/
	{
	// if the test has already failed skip the delay
	if ( iTestVerdict != EPass )
		{
		pLogSystem->Log(_L("skipped delay as test has already failed") );
		return;
		}
	
	// get the required time for the delay
	// first get the value as a string
	TLex TimeOut(Text);
	TimeOut.NextToken();
	TPtrC token=TimeOut.NextToken();
	
	// convert the value into a int
	TLex timeoutLex(token);
	TInt GuardTimerValue =2500;
	if (timeoutLex.Val(GuardTimerValue) != KErrNone  )
		{
		pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr,
			_L("error in guard timer value could not decode >%S< as value"),
			&token,
			GuardTimerValue );
		return;
		}
	
	// display the text after the PRINT and 1 space = 6
	pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr,
		_L("Delay for %d mS"), GuardTimerValue );
	
	// wait for the required delay
	RTimer GuardTimer;
	GuardTimer.CreateLocal();			// create for this thread
	TRequestStatus TimerStatus;
	GuardTimer.After(TimerStatus,GuardTimerValue * 1000);
	User::WaitForRequest(TimerStatus);
	GuardTimer.Cancel();
	
	}

void CParseLine::SetSeverity(  const TDesC& Text )
/**
This function implements the script SetSeverity Keyword

@param Text contains the severity level
*/
	{
	// get the required time for the delay
	// first get the value as a string
	TLex SeverityOut(Text);
	SeverityOut.NextToken();
	TPtrC token=SeverityOut.NextToken();
	
	// convert the value into a int
	TLex Severity(token);
	TInt SeverityValue = 7;
	if (Severity.Val(SeverityValue) != KErrNone  )
		{
		pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr,
			_L("Error in severity level value could not decode >%S< as value"),
			&token,
			SeverityValue );
		return;
		}
	
	if(SeverityValue & ~7)
		{
		pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__,
			ESevrErr, _L("Error in severity value."));
		SeverityValue = 7;
		return;
		}
	else
		{
		iSeverity = SeverityValue;
		
		TInt NoOfDlls = iArrayLoadedSuiteDll->Count();
		for ( TInt i=0; i < NoOfDlls; i++ )
			{
			CSuiteDll * ptrSuite = iArrayLoadedSuiteDll->At(i);
			ptrSuite->iTestSuite->SetSeverity(iSeverity);
			}
		}
	
	pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("Severity is set to %d"), SeverityValue );
	}

void CParseLine::RunScriptL( const TDesC& Text )
/**
This function implements the script RUN_SCRIPT Keyword

@param Text contains the script file name
*/
	{
	// use Tlex to decode the cmd line
	TLex lex(Text);
	
	// start at the begining
	TPtrC token=lex.NextToken();
	
	// step over the keyword
	token.Set(lex.NextToken());
	
	// format for printing
	pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("RUN_SCRIPT %S"),&token );
	
	// create a new Script object (but use the current parse
	// as it has the dll loaded record
	CScript* newScript=CScript::NewL(this);
	CleanupStack::PushL(newScript);
	
	// read in the script file
	TFileName scriptFileName=token;
	if ( newScript->OpenScriptFile( scriptFileName ))
		{
		// process it
		iTestVerdict = newScript->ExecuteScriptL();
		
		/* verdicts for scripts are not really useful so forget! */
		/* pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("The final result for test script %S was %s"),
		&token,
		pLogSystem->TestResultText( iTestVerdict ) ); */
		
		// add results from the new script to the owner script
		iScript->AddResult( newScript );
		}
	else
		{
		// failed to find script so verdict incloncusive
		iTestVerdict = EInconclusive;
		}
	
	CleanupStack::PopAndDestroy(newScript);
	
	}

void CParseLine::RunTestStep( const TDesC& Text )
/**
RunTestStep

@param Text contains the test step name
@return KErrNone or an error code
*/
	{
	TPtrC suite, step, config;
	
	// use Tlex to decode the cmd line
	TLex lex(Text);
	
	// start at the begining
	TPtrC timeout=lex.NextToken();
	
	// step over the keyword
	timeout.Set(lex.NextToken());
	
	// get the other parameters
	suite.Set(lex.NextToken());
	step.Set(lex.NextToken());
	config.Set(lex.NextToken());
	
	// save the name of the current test step
	iCurrentStepName = step;
	
	// conert the guard timer value to a TInt
	TLex lexTimeOut(timeout);
	TInt GuardTimerValue;
	if (lexTimeOut.Val(GuardTimerValue) != KErrNone  )
		{
		pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("error in guard timer value:%S using default 100mS"), &timeout);
		GuardTimerValue = 100;
		}
	
	// log the start of a test step
	pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("RUN_TEST_STEP:%S suite:%S timeout:%dmS config:%S"),
		&step,
		&suite,
		GuardTimerValue,
		&config );
	
	// run the test step
	enum TVerdict CurrentTestVerdict;
	
	// check which thread mode selected!
	if ( iScript->iMultThread )
		CurrentTestVerdict = DoTestNewThread( suite, step, GuardTimerValue, config );
	else
		CurrentTestVerdict = DoTestCurrentThread( suite, step, config );
	
	pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("TEST_STEP:%S returned:%s "), &step, pLogSystem->TestResultText(CurrentTestVerdict));
	
	// this result is only significant if every thing else has passed
	if ( iTestVerdict == EPass )
		iTestVerdict = CurrentTestVerdict;
	
	}

void CParseLine::RunPanicTestStep( const TDesC& Text )
/**
This function implements the script RUN_PANIC_STEP Keyword.  
This step is expected to panic and gives a pass result if it panics
and a fail if it does not.

@param Text contains the test step name
*/
	{
	TPtrC suite, step, config;
	
	// use Tlex to decode the cmd line
	TLex lex(Text);
	
	// start at the begining
	TPtrC timeout=lex.NextToken();
	
	// step over the keyword
	timeout.Set(lex.NextToken());
	
	// get the other parameters
	suite.Set(lex.NextToken());
	step.Set(lex.NextToken());
	config.Set(lex.NextToken());
	
	// save the name of the current test step
	iCurrentStepName = step;
	
	// conert the guard timer value to a TInt
	TLex lexTimeOut(timeout);
	TInt GuardTimerValue;
	if (lexTimeOut.Val(GuardTimerValue) != KErrNone  )
		{
		pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("error in guard timer value:%S using default 10000"), &timeout);
		GuardTimerValue = 1000000;
		}
	
	// log the start of a test step
	pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("RUN_PANIC_STEP:%S suite:%S timeout:%dmS config:%S"),
		&step,
		&suite,
		GuardTimerValue,
		&config );
	
	// run the test step
	enum TVerdict CurrentTestVerdict;
	
	// check which thread mode selected!
	CurrentTestVerdict = DoPanicTest( suite, step, GuardTimerValue, config );
	
	pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("TEST_STEP:%S returned:%s "), &step, pLogSystem->TestResultText(CurrentTestVerdict));
	
	// this result is only significant if every thing else has passed
	if ( iTestVerdict == EPass )
		iTestVerdict = CurrentTestVerdict;
	
	}


void CParseLine::RunUtil( const TDesC& Text )
/**
This function implements the script RUN_UTILS Keyword

@param Text contains the util name
*/
	{
	// Call the utils
	pTestUtils->RunUtils( Text );
	}

void CParseLine::Reboot(void)
/**
This function implements the script REBOOT Keyword
*/
	{
	pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("REBOOT ") );
	}

TInt CParseLine::threadfn ( TAny * ptr )
/**
Static function to call DoTestStep() which is run
in a separate thread

@param ptr Contains the test step name, and configuration parameters passed to the new thread. 
@return The test result as a TVerdict.
*/
	{
	// get clean-up stack
	CTrapCleanup* Cleanup=CTrapCleanup::New();
	
	// get the data for the test
	CStepData * data = (CStepData *)ptr;
	
	// do the test step
	TVerdict result =  data->iSuite->iTestSuite->DoTestStep( data->step, data->config);
	
	// done with the clean up stack
	delete Cleanup;
	
	// return the test result
	return result;
	}

enum TVerdict CParseLine::DoTestNewThread(TPtrC suite, TPtrC step, TInt GuardTimerValue, const TPtrC &config)
/**
Call the test step in the correct suite

@param suite The test suite to use.
@param step The test step name
@param the guard timer in milliseconds
@param reference to the configuration file
@return The test result as a TVerdict
*/
	{
	//	get the number of suites loaded
	TInt NoOfDlls = iArrayLoadedSuiteDll->Count();
	
	TVerdict result = ETestSuiteError;
	
	// search the list of loaded test suite DLLs for the required one
	for ( TInt i=0; i < NoOfDlls; i++ )
		{
		CSuiteDll * ptrSuite = iArrayLoadedSuiteDll->At(i);
		
		if(ptrSuite->SuiteNameMatch(suite))
			{
			
			// do a test in a new thread
			RThread NewThread;
			
			CStepData data;
			data.step = step;
			data.config = config;
			data.iSuite = ptrSuite;
			
			// run in a new thread, with a new heap
			TBuf<32> threadName;
			threadName.Format(KThreadNameFmt, iThreadNameSuffix++);
			TInt res=NewThread.Create(threadName,
				threadfn,
				KDefaultStackSize,
				KMinHeapSize,
				KMaxTestThreadHeapSize,
				&data);
			
			if (res != KErrNone)
				{
				pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("unable to create test thread ") );
				return EFail;
				}
			
			// start the thread and request the status
			TRequestStatus ThreadStatus;
			NewThread.Logon(ThreadStatus);
			
			// if the guard timer value is -1 don't time at all
			if ( GuardTimerValue == -1 )
				{
				// no guard timer
				NewThread.Resume();
				User::WaitForRequest( ThreadStatus );
				}
			else
				{
				// wait for either test thread or timer to end
				RTimer GuardTimer;
				GuardTimer.CreateLocal();			// create for this thread
				TRequestStatus TimerStatus;
				NewThread.Resume();
				GuardTimer.After(TimerStatus,GuardTimerValue * 1000);
				User::WaitForRequest(ThreadStatus, TimerStatus);
				if (TimerStatus==KRequestPending)
					{
					GuardTimer.Cancel();
					User::WaitForRequest(TimerStatus);
					}
				GuardTimer.Close();
				}
			
			// get the test result
			result =  (TVerdict)ThreadStatus.Int();
			
			// check terminated ok
			switch(NewThread.ExitType() )
				{
				case EExitTerminate:
				case EExitKill:
					break;
				case EExitPanic:
					pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("thread had a panic") );
					result = EFail;
					break;
				case EExitPending:
					// if the thread is still pending then the guard timer must have expired
					pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("thread timed out") );
					// kill the test step thread
					NewThread.Kill(1);
					User::WaitForRequest(ThreadStatus);
					result = EFail;
					break;
				default:
					break;
				}
			
			// done with the test thread
			NewThread.Close();
			
			// send the log data a line at a time
			// find the end of the first line
			TInt nl= ptrSuite->iTestSuite->iLogData.Locate(TChar('\n'));
			
			// get each  line in turn
			while ( nl != KErrNotFound )
				{
				// display line
				TPtrC16 line = ptrSuite->iTestSuite->iLogData.Left(nl);
				pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("%S"), &line );
				
				// remove the line displayed
				ptrSuite->iTestSuite->iLogData.Replace(0,nl+1,_L(""));
				
				// find the next newline
				nl= ptrSuite->iTestSuite->iLogData.Locate(TChar('\n'));
				}
			
			// return the test verdict
			return result;
			}
		}
		
		// the required suite has not been found
		pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("Error in test step:%S cannot find suite:%S" ),
			&step,
			&suite );
		
		return ETestSuiteError;
}

// DoTest
enum TVerdict CParseLine::DoPanicTest(TPtrC suite, TPtrC step, TInt GuardTimerValue, const TPtrC &config)
/**
Call the test step which is expected to Panic

@param suite The test suite to use.
@param step The test step name
@param the guard timer in milliseconds
@param reference to the configuration file
@return The test result as a TVerdict
*/
	{
	//	get the number of suites loaded
	TInt NoOfDlls = iArrayLoadedSuiteDll->Count();
	
	TVerdict result;
	
	// search the list of loaded test suite DLLs for the required one
	for ( TInt i=0; i < NoOfDlls; i++ )
		{
		CSuiteDll * ptrSuite = iArrayLoadedSuiteDll->At(i);
		
		if(ptrSuite->SuiteNameMatch(suite))
			{
			
			// do a test in a new thread
			RThread t;
			
			CStepData data;
			data.step = step;
			data.config = config;
			data.iSuite = ptrSuite;
			
			// run in a new thread, with a new heap
			TBuf<32> threadName;
			threadName.Format(KThreadNameFmt, iThreadNameSuffix++);
			TInt res=t.Create(threadName,
				threadfn,
				KDefaultStackSize,
				KMinHeapSize,
				KMaxTestThreadHeapSize,
				&data);
			
			if (res != KErrNone)
				{
				pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("unable tp create test thread ") );
				return EFail;
				}
			
			// start the thread and request the status
			TRequestStatus ThreadStatus;
			t.Logon(ThreadStatus);
			
			// if the guard timer value is -1 don't time at all
			if ( GuardTimerValue == -1 )
				{
				// no guard timer
				t.Resume();
				User::WaitForRequest( ThreadStatus );
				}
			else
				{
				// wait for either test thread or timer to end
				RTimer GuardTimer;
				GuardTimer.CreateLocal();			// create for this thread
				TRequestStatus TimerStatus;
				t.Resume();
				GuardTimer.After(TimerStatus,GuardTimerValue * 1000);
				User::WaitForRequest(ThreadStatus, TimerStatus);
				GuardTimer.Cancel();
				User::WaitForAnyRequest();
				}
			
			// get the test result
			result =  (TVerdict)ThreadStatus.Int();
			
			TExitCategoryName category(t.ExitCategory());
			// check terminated ok
			switch(t.ExitType() )
				{
				case EExitPanic:
					pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("Panic occurred of category %S and reason %d"), &category, t.ExitReason());
					result =
						category.Left(4)==_L("KERN") || category.Left(3)==_L("E32") ? EFail : EPass;
					break;
				case EExitPending:
					// if the thread is still pending then the guard timer must have expired
					pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("thread timed out") );
					// kill the test step thread
					t.Kill(1);
					result = EFail;
					break;
				case EExitTerminate:
				case EExitKill:
				default:
					pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("Test did not panic so fail") );
					result = EFail;
					break;
				}
			
			// done with the test thread
			t.Close();
			
			// send the log data a line at a time
			// find the end of the first line
			TInt nl= ptrSuite->iTestSuite->iLogData.Locate(TChar('\n'));
			
			// get each  line in turn
			while ( nl != KErrNotFound )
				{
				// display line
				TPtrC16 line = ptrSuite->iTestSuite->iLogData.Left(nl);
				pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("%S"), &line );
				
				// remove the line displayed
				ptrSuite->iTestSuite->iLogData.Replace(0,nl+1,_L(""));
				
				// find the next newline
				nl= ptrSuite->iTestSuite->iLogData.Locate(TChar('\n'));
				}
			
			// return the test verdict
			return result;
			}
		}
		
		// the required suite has not been found
		pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("Error in test step:%S cannot find suite:%S" ),
			&step,
			&suite );
		
		return ETestSuiteError;
}

// DoTestCurrentThread
enum TVerdict CParseLine::DoTestCurrentThread(TPtrC suite, TPtrC step, TPtrC config)
/**
Call the test step in the current Thread

@param suite The test suite to use.
@param step The test step name
@param reference to the configuration file
@return The test result as a TVerdict
*/
	{
	//	get the number of suites loaded
	TInt NoOfDlls = iArrayLoadedSuiteDll->Count();
	
	TVerdict    result;

	// search the list of loaded test suite DLLs for the required one
	for ( TInt i=0; i < NoOfDlls; i++ )
		{

        CSuiteDll * ptrSuite = iArrayLoadedSuiteDll->At(i);

        if(ptrSuite->SuiteNameMatch(suite))
			{
			//  execute in the current thread
			result =  ptrSuite->iTestSuite->DoTestStep(step, config);
			
			// send the log data a line at a time
			// find the end of the first line
			TInt nl= ptrSuite->iTestSuite->iLogData.Locate(TChar('\n'));
			
			// get each  line in turn
			while ( nl != KErrNotFound )
				{
				// display line
				TPtrC16 line = ptrSuite->iTestSuite->iLogData.Left(nl);
				pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("%S"), &line );
				
				// remove the line displayed
				ptrSuite->iTestSuite->iLogData.Replace(0,nl+1,_L(""));
				
				// find the next newline
				nl= ptrSuite->iTestSuite->iLogData.Locate(TChar('\n'));
				}
			
			// return the test verdict
			return result;
			}
		}
	
	// the required suite has not been found
	pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("Error in test step:%S cannot find suite:%S" ),
		&step,
		&suite );
	
	return ETestSuiteError;
	}



// RunCed
void CParseLine::RunCed( const TDesC& Text )
/**
This function runs CED the commdb tool....

@param text The name of tge ced.cfg file to use
*/
	{
	// use Tlex to decode the cmd line
	TLex lex(Text);
	
	// start at the begining
	TPtrC token=lex.NextToken();
	
	// step over the keyword "ced" and get the rest
	token.Set(lex.Remainder());
	
	pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("Run Ced parameters %S"), &token);
	
	// In the ARM and EKA2 emulator builds run ced as a new process
	RProcess ced;
	TBuf<100> CmdLine;
	CmdLine.Format(_L("-i %S"),&token);
	TInt ret = ced.Create( _L("z:\\system\\libs\\ced.exe"), CmdLine );
	
	if ( ret != KErrNone )
		{
		TPtrC Errortxt = CLog::EpocErrorToText(ret);
		pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("Failed to start ced process %S"),&Errortxt );
		return;
		}
	else
		{
		pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("ced.exe started %S"), &CmdLine);
		
		// create and start a guard timer
		RTimer GuardTimer;
		TRequestStatus TimerStatus(KRequestPending);
		GuardTimer.CreateLocal();			// create for this thread
		GuardTimer.After(TimerStatus,120 * 1000 *1000);
		
		// start ced
		TRequestStatus ThreadStatus;
		ced.Logon(ThreadStatus);
		ced.Resume();
		
		// wait for guard timer or ced
		User::WaitForRequest(ThreadStatus, TimerStatus);
		
		// cancel the guard timer now a request has happened
		GuardTimer.Cancel();
		
		if(ThreadStatus==KRequestPending)
			{
			pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("<font size=5 color=FF0000>CED TIMED OUT</font>\n") );
			}

		User::WaitForAnyRequest();	//Need this to balance up the requests/completions
		//as User::WaitForRequest(ThreadStatus, TimerStatus) only makes one request
		
		// check return type
		TVerdict cedVerdict = EInconclusive;
		TExitType cedExitType = ced.ExitType();
		if ( cedExitType == EExitTerminate || cedExitType == EExitKill )
			{
			TInt exitReason = ced.ExitReason();
			if(exitReason == 0)
				{
				pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("ced exited with success") );
				cedVerdict = EPass;
				}
			else
				pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("ced exited with failure %d"), exitReason );
			}
		else
			{
			// Describe what specifically happened
			if ( cedExitType == EExitPanic)
				pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("ced exited with EExitPanic") );
			else if ( cedExitType == EExitPending)
				pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("ced still running (!)") );
			else
				pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("ced exited with unknown type %d"), cedExitType );
			}
		// this result is only significant if every thing else has passed
		if( iTestVerdict == EPass )
			{
			iTestVerdict = cedVerdict;
			}
		}
	
	
	
}

void CParseLine::RunProgram( const TDesC& Text )
/**
This function implements the script RunProgram command

@param Text The name of the program to run
*/
	{
	TPtrC Param;
	
	// use Tlex to decode the cmd line
	TLex lex(Text);
	
	// start at the begining
	TPtrC token=lex.NextToken();
	
	// step over the keyword
	token.Set(lex.NextToken());
	
	// get the parameters
	Param.Set(lex.NextToken());
	
	pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("Run Program "));
	
#ifdef __WINS__
	// this is not supportted in WINS builds
	pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("Warning RUN_PROGRAM is not supported on WINS") );
#endif
	
	// In the ARM build run program as a new process
	// use the rest of the text as parameters
	RProcess program;
	TInt ret = program.Create( token, lex.Remainder() );
	
	if ( ret != KErrNone )
		{
		TPtrC Errortxt = CLog::EpocErrorToText(ret);
		pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("Failed to start process %S"),&Errortxt );
		return;
		}
	else
		{
		pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("Program started") );
		
		// start program
		TRequestStatus ThreadStatus;
		program.Logon(ThreadStatus);
		program.Resume();
		
		// wait for guard timer or ced
		User::WaitForRequest(ThreadStatus);
		
		// check return type
		if ( program.ExitType() == EExitPanic)
			pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("program returned EExitPanic") );
		else if ( program.ExitType() == EExitPending)
			pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("program returned EExitPending") );
		else
			pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("program returned EExitTerminate") );
		}
	}

void CParseLine::LogSettings(  const TDesC& Text )
/**
This function parses "LOG_SETTNGS" command
Command format is LOG_SETTINGS "put src." (1/0),
"HTML format" (1/0)

@param string to parse
*/
	{
	// use Tlex to decode the cmd line
	TLex lex(Text);
	
	// start at the begining
	TPtrC token=lex.NextToken();
	
	// step over the keyword
	//Get information about source
	token.Set(lex.NextToken());
	
	TLex oSrcLex(token);
	TInt isSrc = ETrue; //Shall we put src information?
	if (oSrcLex.Val(isSrc) != KErrNone  )
		{
		pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr,
			_L("Error in LOG_SETTINGS  value could not decode >%S< as value(0/1)"),
			&token);
		}
	else
		{
		pLogSystem->SetPutSrcInfo(isSrc) ;
		}
	//Get information about format
	//TInt isHtml = ETrue; //Shall we use HTML log format?
	token.Set(lex.NextToken());
	TLex oHtmlLex(token);
	
	if (oHtmlLex.Val(isSrc) != KErrNone  )
		{
		pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr,
			_L("Error in LOG_SETTINGS  value could not decode >%S< as value(0/1)"),
			&token);
		}
	else
		{
		pLogSystem->SetHtmlLogMode(isSrc) ;
		}
	}


void CParseLine::LoadSuiteL( const TDesC& Text )
/** 
LoadSuite
This function loads a required test suite DLL
It also creates a CtestSuite object as a record
of the loaded dll

@param suite dll name
*/
	{
	// use Tlex to decode the cmd line
 	TLex lex(Text);
	
	// start at the begining
	TPtrC token=lex.NextToken();
	
	// step over the keyword
	token.Set(lex.NextToken());
	
	
    RLibrary lib;
	TInt err = lib.Load(token, KTxtDLLpath);
    if (err == KErrNone)
        {
        lib.Close();
        }
	
	if ( err==KErrNotFound )
		{
		// this is not going to load !
		// 		pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("Test suite %S could not be found"),&token );
		//return;
		}
	
	// check not already loaded
	// by searching the list of loaded test suite DLLs for the required one
	// start with the number of suites loaded
	TInt NoOfDlls = iArrayLoadedSuiteDll->Count();
	for ( TInt i=0; i < NoOfDlls; i++ )
		{
		CSuiteDll * ptrSuite = iArrayLoadedSuiteDll->At(i);
		
		// check the names
		if ( ptrSuite->iName.CompareF( token ) == 0 )
			{
			// this suite DDL is already loaded
			pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("warning Test suite %S already loaded not re-loaded"),&token );
			return;
			}
		}
	
	// create a new suitedll object to store info on loaded DLL
	CSuiteDll * newRef = CSuiteDll::NewL( token );
	CleanupStack::PushL(newRef);
	
	// set default severity and logging system
	newRef->iTestSuite->SetSeverity(iSeverity);
	newRef->iTestSuite->SetLogSystem(pLogSystem);
	
	// add to data
	iArrayLoadedSuiteDll->AppendL( newRef );
	CleanupStack::Pop(newRef);
	}

// unload all the loaded DLLS
void CParseLine::Unload(const TDesC& Text)
/**
This function implements the script Unload command
*/
	{
	// use Tlex to decode the cmd line
	TLex lex(Text);
	
	// start at the begining
	TPtrC token=lex.NextToken();
	

	if (!lex.Eos())
		{
		// step over the keyword
		token.Set(lex.NextToken());
		TInt NoOfDlls = iArrayLoadedSuiteDll->Count();
		for ( TInt i=0; i < NoOfDlls; i++ )
			{
			CSuiteDll * ptrSuite = iArrayLoadedSuiteDll->At(i);
		
			// check the names
			if ( ptrSuite->iName.CompareF( token )==0 )
				{
				if (NoOfDlls==1)
					{
					iArrayLoadedSuiteDll->ResetAndDestroy();
					}
				else
					{
					// this suite DDL is to be unloaded.
					iArrayLoadedSuiteDll->Delete(i);
					iArrayLoadedSuiteDll->Compress();
					delete ptrSuite;
					}
				return;
				}
			}
		}
	else
		{
		if (iArrayLoadedSuiteDll)
			{
			// unload all the loaded DLLS and their records
			iArrayLoadedSuiteDll->ResetAndDestroy();
			}
		}
	}

void CParseLine::HeapMark(void)
/**
This function implements the script HeapMark command
*/
	{
	__UHEAP_MARK;
	}


void CParseLine::HeapCheck(void)
/**
This function implements the script HeapCheck command
*/
	{
	__UHEAP_MARKEND;
	}

void CParseLine::RequestMark(void)
/**
This function implements the script RequestMark command
*/
	{
	// get number of outstanding requetsts on thread before we run the test
	iReqsAtStart = RThread().RequestCount();
	pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("Requests at the start %d "),iReqsAtStart);
	}


void CParseLine::RequestCheck(void)
/**
This function implements the script RequestCheck command
*/
	{
	// check the number of outstanding requests against recorded value
	pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("Requests at the start %d now %d"),
		iReqsAtStart,
		RThread().RequestCount() );
	
	if ( iReqsAtStart != RThread().RequestCount())
		{
		pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("Test failed on requests count"));
		
		// this result is only significant if every thing else has passed
		if ( iTestVerdict == EPass )
			iTestVerdict = EFail;
		
		}
	}


void CParseLine::HandlesMark(void)
/**
This function implements the script HandlesMark command
*/
	{
	// get number of Handles *before* we start the program
	RThread().HandleCount(iProcessHandleCountBefore, iThreadHandleCountBefore);
	
	pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("Process handles count %d thread handle count %d"),
		iProcessHandleCountBefore,
		iThreadHandleCountBefore );
	}

void CParseLine::HandlesCheck(void)
/**
This function implements the script HandlesCheck command
*/
	{
	TInt processHandleCountAfter;
	TInt threadHandleCountAfter;
	RThread().HandleCount(processHandleCountAfter, threadHandleCountAfter);
	
	pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("Process handles count %d thread handle count %d"),
		processHandleCountAfter,
		threadHandleCountAfter );
	
	// check that we are closing all the threads
	if(iThreadHandleCountBefore != threadHandleCountAfter)
		{
		pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("Test failed on thread handle count"));
		
		// this result is only significant if every thing else has passed
		if ( iTestVerdict == EPass )
			iTestVerdict = EFail;
		}
	
	// check that we are closing all the handles
	if(iProcessHandleCountBefore != processHandleCountAfter)
		{
		pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("Test failed on Process handle count"));
		
		// this result is only significant if every thing else has passed
		if ( iTestVerdict == EPass )
			iTestVerdict = EFail;
		}
	}

CSuiteDll* CSuiteDll::NewL( const TDesC& aName )
	{
	CSuiteDll* self = new(ELeave) CSuiteDll(aName);
	CleanupStack::PushL( self );
	self->ConstructL();
	CleanupStack::Pop();
	return self;
	}

CSuiteDll::CSuiteDll( const TDesC& aName )
/**
Constructor.
@param aName The name of the test suite DLL to be loaded (can contain file path)
*/
	{
	// save the name
	iName.Copy( aName );
	}


/**
Load a test suite dll and save the name and test
test suite pointers
*/
void CSuiteDll::ConstructL()
	{
	// load DLL by name
	TInt ret = iLibrary.Load(iName, KTxtDLLpath);
	if ( ret != KErrNone )
		{
		pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("Test suite %S found but would not load"),&iName );
		pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("check any other Dlls required by %S "),&iName );
		User::Leave(ret);
		}

	// save the suite name dll without file path
	TParse parse;
    parse.Set(iName, NULL, NULL);
    iName.Copy(parse.NameAndExt());

	// get the interface pointer at ordinal 1
	TLibraryFunction  entry=iLibrary.Lookup(1);

    // Call this interface pointer to create new CTestSuite
	// If this call goes to the wrong function then the test
	// suite does not have the correct function at ordinal 1
	// This is usually caused by an error in the def file
    iTestSuite = (CTestSuite*) entry();

    // Second-phase constructor for CTestSuite
    TRAPD(error,iTestSuite->ConstructL() );

    //-- set the suite name (internal variable) the same as suite name dll file name (without extention)
    iTestSuite->OverrideSuiteName(parse.NameAndExt());

	if (error)
		{
		pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("Failed to contruct test suite"));
		User::Leave(error);
		}

	// set suite severity level
	iTestSuite->SetSeverity(pLogSystem->Severity());

	// get the version information
	TPtrC Versiontxt = iTestSuite->GetVersion();

	// add to log
	pLogSystem->LogExtra(((TText8*)__FILE__), __LINE__, ESevrErr, _L("LOAD_SUITE %S version %S loaded ok"),&iName, &Versiontxt );
	}


CSuiteDll::~CSuiteDll()
/**
destructor Delete the TestSuiteObject in the loaded DLL and close
and unload the library
*/
	{
	// delete the TestSuiteObject in the loaded DLL
	if (iTestSuite)
		{
		delete iTestSuite;
		}

	// close and unload the library
	iLibrary.Close();
	}

/**
*   Find out if the suite name matches the suite dll name.
*
*   @param  aSuiteName suite name (without file extention)
*   @return ETrue if the given suite name matches suite name dll file name.
*/
TBool CSuiteDll::SuiteNameMatch(const TDesC& aSuiteName) const
{
    TParse parse;

    //-- iName contains suite dll name with file extention, strip the extention
    parse.Set(iName, NULL, NULL);

    //-- using CompareF instead of FindF to avoid situation when "AAA" suite name can match "AAA123"
    TBool bFound =  (aSuiteName.CompareF(parse.Name()) ==0 ) ? ETrue : EFalse;
    
    return bFound;
}

















