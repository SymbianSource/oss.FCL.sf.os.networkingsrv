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
//

/**
 @file consoleMain.cpp
 @internalComponent
*/

// system includes
#include <e32base.h>
#include <e32cons.h>
#include "f32file.h"

// test system includes
#include "../inc/Log.h"
#include "../inc/TestSuite.h"
#include "../inc/TestStep.h"
#include "../inc/TestUtils.h"
#include "script.h"
#include "parseline.h"

/** 
Holds EPOC32EX string constant 
*/
_LIT(KTxtEPOC32EX,"EPOC32EX");

/** 
Holds Example Code string constant 
*/
_LIT(KTxtExampleCode,"Integration Test Harness v 1.012");

/** 
Failed format string constant 
*/
_LIT(KFormatFailed,"failed: leave code=%d");

/** 
Holds OK string constant 
*/
_LIT(KTxtOK,"script completed \n");

/** 
Holds Example Usage constant 
*/
_LIT(KTxtUseExample,"Usage:\nSCHEDULETEST [-a] [-Sn] <file.script>\nSCHEDULETEST [-a] [-Sn] -u <test_suite.DLL> [file.ini]");

/** 
Holds DLL fullpath 
*/
_LIT(KTxtDLLpath,"c:\\;c:\\system\\libs");

/** 
Constant that holds press any key string 
*/
_LIT(KTxtPressAnyKey,"[press any key to continue]\n");

#ifdef _WIN32
	#ifdef __CW32__
	/** WINSCW target constant */
	_LIT(KTxtTarget,"WINSCW");
	#else
	/** WINS target constant */
	_LIT(KTxtTarget,"WINS");
	#endif
#else 
	#ifdef __MARM_THUMB__ 
	/** THUMB target constant */
		#ifdef __EABI__
		_LIT(KTxtTarget,"ARMV5");
		#else
		_LIT(KTxtTarget,"THUMB");
		#endif
	#else
	/** ARM4 target constant */
	_LIT(KTxtTarget,"ARM4");
	#endif
#endif

#ifdef _DEBUG
/** 
debug build string constant 
*/
_LIT(KTxtBuild,"udeb");
#else
/** 
release build string constant 
*/
_LIT(KTxtBuild,"urel");
#endif

/** 
Platform String Constant for EKA2
*/
_LIT(KTxtPlatform,"EKA2");

/** 
maximum length of command line 
*/
#define MAX_LEN_CMD_LINE	0x100

/** 
Global data: Asynchronous timer service 
*/
GLDEF_D static RTimer TheTimer;

// private

/** 
initialize with cleanup stack 
*/
LOCAL_C void callConsoleMainL(void);

/** the main function */ 
LOCAL_C void doConsoleMainL(void);	

LOCAL_C void doUnitTestL(TInt aSeverity,const TFileName& aSuitDLL,const TFileName& aIniFile);
LOCAL_C void Usage(void);

/**
The main function called by E32. 
@returns 0
*/
GLDEF_C TInt E32Main()  
    {
    //__UHEAP_MARK;
	// get clean-up stack
	CTrapCleanup* cleanup=CTrapCleanup::New(); 
	

	// more initialization, then do console main
	TRAPD(error,callConsoleMainL()); 
	__ASSERT_ALWAYS(!error,User::Panic(KTxtEPOC32EX,error));
	
	// destroy clean-up stack
	delete cleanup; 
	//__UHEAP_MARKEND;

	return 0; 
    }

/**
The main entry point for console applications. 
Initialize and call doConsoleMainL code under cleanup stack.
*/
LOCAL_C void callConsoleMainL() 
    {
	console=Console::NewL(KTxtExampleCode,TSize(KConsFullScreen,KConsFullScreen));
	CleanupStack::PushL(console);
	TRAPD(error,doConsoleMainL()); // perform tests
	if (error)
		console->Printf(KFormatFailed, error);
	else
		console->Printf(KTxtOK);

	CleanupStack::PopAndDestroy(); // close console
    }


/**
The start of code. 
*/
LOCAL_C void doConsoleMainL()
    {
	// console is initialised
	// now start the Log system
	TInt severity = ESevrAll;
	pLogSystem = CLog::NewL( console );
	CleanupStack::PushL(pLogSystem);

	// initialise the CTestUtils
	pTestUtils = CTestUtils::NewL( pLogSystem );
	CleanupStack::PushL(pTestUtils);

	// read the command line into cmd
	TBuf<MAX_LEN_CMD_LINE> cmd;
	User::CommandLine(cmd);
	cmd.UpperCase();

	// use Tlex to decode the cmd line
	TLex lex(cmd);
	TPtrC token=lex.NextToken();

	// if there is no input filename on the cmd line Panic!
	if (token.Length()==0) 
		Usage();
	else
		{
		// Process any options
		TBool unitTest = EFalse;
		while(token.Length() > 1 && token[0] == '-')
			{
			switch(token[1])
				{
				case 'U':
				case 'u':
					unitTest = ETrue;
					break;
				case 'S':
				case 's':
					{
					if( token.Length() == 3 )
						{
						TLex Severity(token);
						Severity.Inc(2);
						if((Severity.Peek()).IsDigit())
							{
							Severity.Val(severity);
							// wrong severity level
							if( (severity < 0) || (severity > 7 )) 
								severity = 7;
							}
						}
					break;
					}
				case 'A':
				case 'a':
					automatedMode = ETrue;
					break;
				default:
					Usage();
					return;
				}

			token.Set(lex.NextToken());
			}

		if(unitTest)
			{
			// get suite name
			TFileName suitFileName;

			if (token.Length()!=0) 
				suitFileName = token;
			else
				{
				Usage();
				User::Leave(KErrArgument);
				}

			pLogSystem->OpenLogFileL(suitFileName);
			
			pLogSystem->LogExtra(((TText8*)(__FILE__)), (__LINE__), ESevrErr,
				_L("%S %S %S %S starting...."), &KTxtExampleCode(), &KTxtTarget(), &KTxtPlatform(), &KTxtBuild() );

			// get ini file
			token.Set(lex.NextToken());

			TFileName configFileName;
			if (token.Length()!=0) 
				{
				if(token.Find(_L("-S")) == KErrNotFound )
				configFileName = token;
				}
			
			// do unit test
			doUnitTestL(severity, suitFileName, configFileName);
			
			if(!automatedMode)
				{
				console->Printf(KTxtPressAnyKey);
				console->Getch(); // get and ignore character
				}
			}
		else
			{			
			// there is a script file so lets do it!
			// save the input filename
			TFileName scriptFileName=token;

			// make the log file name from the script file name
			TFileName LogFileName = token;

			// open the log file
			pLogSystem->OpenLogFileL( LogFileName);
			
			pLogSystem->LogExtra(((TText8*)(__FILE__)), (__LINE__), ESevrErr,
					_L("%S %S %S %S starting...."), 
					&KTxtExampleCode(), &KTxtTarget(), &KTxtPlatform(), &KTxtBuild() );

			// create a ParseScript object
			CScript* parseScript=CScript::NewL();
			CleanupStack::PushL(parseScript);

			// parse all scripts
			do
				{
				// get the next file
				scriptFileName=token;

				// read in the script file
				if ( parseScript->OpenScriptFile( scriptFileName ))
					{
					// process it
					parseScript->ExecuteScriptL( );

					// display results summary
					parseScript->DisplayResults( );

					}
				// get the next
				token.Set(lex.NextToken());
				} while ( token.Length()!=0 );			

			CleanupStack::PopAndDestroy(parseScript);

			// close the logging system
			pLogSystem->CloseLogFile();
			}
		}

	// delete the test utils object
	CleanupStack::PopAndDestroy(pTestUtils);
//	delete pTestUtils;

	// close the log file
	CleanupStack::PopAndDestroy(pLogSystem);
//	delete pLogSystem;

}

/**
Performs unit test. 

@param aSeverity The current logging severity level.
@param aSuitDLL The test suite DLL which contains the unit test.
@param aIniFile The ini file name.
*/
LOCAL_C void doUnitTestL(TInt aSeverity, const TFileName& aSuitDLL,const TFileName& aIniFile)
{	
	// check the dll can be found before trying to load
    RLibrary lib;
	TInt err = lib.Load(aSuitDLL, KTxtDLLpath);
    if (err == KErrNone)
        {
        lib.Close();
        }
	    
	if ( err==KErrNotFound ) 
	{	
		// this is not going to load !
 		//pLogSystem->Log(_L("Test suite %S could not be found"), &aSuitDLL );
		//return;
	}

	// create a new suitedll object to store info on loaded DLL
	CSuiteDll * newRef = CSuiteDll::NewL( aSuitDLL );

	// set severity level and logging system
	newRef->iTestSuite->SetSeverity(aSeverity);
	newRef->iTestSuite->SetLogSystem(pLogSystem);

	// do unit test
	newRef->iTestSuite->DoTestUnit(const_cast<TFileName&>(aIniFile));

	delete newRef;
}

/**
Display command line format.
*/
LOCAL_C void Usage()
{
	console->Printf(_L("%S command line error...\n"), &KTxtExampleCode());
	console->Printf(_L("%S"), &KTxtUseExample() );
	console->Getch(); 
}
