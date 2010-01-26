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
// This module contains CScript class
// 
//

/**
 @file Script.cpp
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
Script files can reference other script files
MAX_DEPTH limits the number of references
This is to catch accidental circular references in script files
which would otherwise cause the system to continue until all
memory had be used making more CScript objects 
the maximum number of script files
@internalComponent
*/
#define MAX_DEPTH 100

/** 
Holds press any key string constant 
@internalComponent
*/
_LIT(KTxtPressAnyKey,"[press any key to continue]\n");

/** 
Holds string constant that displys error message 
@internalComponent
*/
_LIT(KTxtBreakOnError,"The test has failed, press X to terminate this test\n [press any other key to continue]\n");

/** 
Global data
count of how deep in script files parser is
this is to check against infinite recursion
@internalComponent
*/
GLDEF_D static TInt ScriptDepth = 0;			



CScript::CScript()
/**
constructor
*/
{}

void CScript::ConstructL( void )
/**
second phase constructor
*/
	{
	iParse = CParseLine::NewL(this);
	
	iParseLineOwner = ETrue;
	iPauseAtEnd = EFalse;
	}

CScript* CScript::NewL( void )
/**
NewL constructor
*/
	{
	CScript * self = new(ELeave) CScript;
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
	}

void CScript::ConstructL( CParseLine * aParse )
/**
Standard Symbian format second phase constructor.
*/
	{
	iParse = aParse;

	iPauseAtEnd = EFalse;

	}

CScript* CScript::NewL( CParseLine * aParse )
/**
Standard Symbian format constructor.

@param aParse A parse line object to be used for decoding the script.
@returns A pointer to the new CScript object.
*/
	{
	CScript * self = new(ELeave) CScript;
	CleanupStack::PushL(self);
	self->ConstructL(aParse);
	CleanupStack::Pop();
	return self;
	}


CScript::~CScript( )
/**
destructor deletes the script buffer.
*/
	{
	// delete scriptbuffer
	delete ipScriptBuffer;
	
	if (iParseLineOwner) delete iParse;
	}


bool CScript::OpenScriptFile(const TFileName &aScriptFileName)
/**
Read in the test script file. 

@param aScriptFileName The script file name. If no extension is supplied .script will
be appended.
@return ETrue the script was read ok, EFalse the script file could not be found.
*/
	{
	// get the full pathname default drive name and extension
	_LIT(Kdefault,"C:\\xx.script"); 
	TParse ScriptFileName;
	TInt returnCode = ScriptFileName.Set( aScriptFileName, &Kdefault, NULL );
	if (returnCode!=KErrNone)
		{
		pLogSystem->LogExtra(((TText8*)(__FILE__)), (__LINE__), ESevrErr,
			_L("Failed to open script file: %S"),&ScriptFileName.FullName());
		Pause();
		return false;
		}

	TFileName scriptFileName = aScriptFileName;

	if ( ScriptDepth++ > MAX_DEPTH )
		{
		// prevent the parser from recursing forever
		pLogSystem->LogExtra(((TText8*)(__FILE__)), (__LINE__), ESevrErr,
			_L("script paser aborting depth:%d"), ScriptDepth );
		return false;	
		}

	// connect to the fileserver
	returnCode=iTheFs.Connect();
	if (returnCode!=KErrNone)
		{
		pLogSystem->LogExtra(((TText8*)(__FILE__)), (__LINE__), ESevrErr,
			_L("Error trying to connect to the file server") );	
		return false;
		}

	// open the script file 
	RFile listfile;
	returnCode=listfile.Open(iTheFs,ScriptFileName.FullName(),EFileRead|EFileShareAny);

	// check if open fails 
	if (returnCode!=KErrNone)
		{
		pLogSystem->LogExtra(((TText8*)(__FILE__)), (__LINE__), ESevrErr,
			_L("Failed to open script file: %S"),&ScriptFileName.FullName());
		Pause();
		return false;
		}

	// display the file being processed
	pLogSystem->LogExtra(((TText8*)(__FILE__)), (__LINE__), ESevrErr,
		_L("reading script %S"),&ScriptFileName.FullName());

	// get the script file size
	TInt listfilesize;
	returnCode=listfile.Size(listfilesize);
	if (returnCode!=KErrNone)
		{
		pLogSystem->LogExtra(((TText8*)(__FILE__)), (__LINE__), ESevrErr,
			_L("Failed read script file: %S size "),&scriptFileName);
		return false;
		}

	// get a buffer to read the file into
	ipScriptBuffer=HBufC8::New(listfilesize);
	if (!ipScriptBuffer)
		{
		pLogSystem->LogExtra(((TText8*)(__FILE__)), (__LINE__), ESevrErr,
			_L("Failed to allocate memory for script file %S "),&scriptFileName);
		return false;
		}

	// get a pointer to the buffer
	TPtr8 ptr=ipScriptBuffer->Des();

	// read the file into the buffer
	returnCode=listfile.Read(ptr);
	if (returnCode!=KErrNone)
		{
		pLogSystem->LogExtra(((TText8*)(__FILE__)), (__LINE__), ESevrErr,
			_L("Failed to read script file %S "),&scriptFileName);
		return false;
		}

	// close the file
	listfile.Close();

	// close the file system
	iTheFs.Close();

	return true;
	
}

enum TVerdict CScript::ExecuteScriptL()
/**
The script file has been read into pScriptBuffer.
Now parse it for commands and excute them.

@return TVerdict The current test result
*/
	{
	// use TLex to decode the script
	TLex8 llex( *ipScriptBuffer);

	// keep a count of the line number
	TInt8 lineNo = 1;

	// loop though processing the rest a line at a time
	while(!llex.Eos())
		{
		// skip any spaces
		while ( llex.Peek()==' ' )
			llex.Inc();

		// mark the start of the line
		llex.Mark();
		
		// move to the next
		while(!llex.Eos() && llex.Peek()!='\n')
			llex.Inc();

		// step over \n
		if ( llex.Peek()=='\n' )
			llex.Inc();
				
		// get the line 
		TPtrC8 pline=llex.MarkedToken();
		if (pline.Length()!=0)
			{
			// and then process
			ProcessLineL( pline, lineNo );
			}

		// on to the next line
		lineNo ++;
		}

	/* script processing complete, now return the script verdict */
	/* Note: the script verdicts are just for the log */
	/* if no tests failed then return pass for the script */
	/* this covers scripts which do not test anything */
	return (iFail == 0 ? EPass : EFail );
	}

void CScript::ProcessLineL(const TPtrC8 &narrowline, TInt8 lineNo)
/**
process a line from the script file

@param narrowline The line of script file to be processed.
@param lineNo The current line number.
*/
	{
	// call parse to process line
	iParse->ProcessLineL(narrowline, lineNo);
	}


void CScript::DisplayResults(void)
/**
Display the accumulated  results
*/
	{
	pLogSystem->LogBlankLine();

	pLogSystem->LogExtra(((TText8*)(__FILE__)), (__LINE__), ESevrErr,_L("Test Results Summary ") );
	pLogSystem->LogExtra(((TText8*)(__FILE__)), (__LINE__), ESevrErr,_L("-------------------- ") );
	pLogSystem->LogExtra(((TText8*)(__FILE__)), (__LINE__), ESevrErr,_L("Passed            :%d"),  iPass);
	pLogSystem->LogExtra(((TText8*)(__FILE__)), (__LINE__), ESevrErr,_L("Failed            :%d"),  iFail);
	pLogSystem->LogExtra(((TText8*)(__FILE__)), (__LINE__), ESevrErr,_L("Inconclusive      :%d"),  iInconclusive);
	pLogSystem->LogExtra(((TText8*)(__FILE__)), (__LINE__), ESevrErr,_L("Test suite errors :%d"),  iTestSuiteError);
	pLogSystem->LogExtra(((TText8*)(__FILE__)), (__LINE__), ESevrErr,_L("Aborted           :%d"),  iAbort);
	pLogSystem->LogExtra(((TText8*)(__FILE__)), (__LINE__), ESevrErr,_L("Total             :%d"),  iTotal);

	if( iPauseAtEnd )
		{
		// A pause at the end has been requested
		Pause();
		}

	}

void CScript::Pause( void )
/**
Implements the Pause command.
*/
	{
	if(!automatedMode)
		{
		console->Printf(KTxtPressAnyKey);
		console->Getch(); // get and ignore character
		}
	}

TBool CScript::BreakOnError( void )
/**
Implements the BreakOnError command.
@return ETrue if the user enters "x" or EFalse
*/
	{
	if(automatedMode)
		{
		pLogSystem->LogExtra(((TText8*)(__FILE__)), (__LINE__), ESevrErr,_L("BREAK_ON_ERROR suppressed; terminating current test") );
		return ETrue;
		}
	else
		{
		// display prompt
		console->Printf(KTxtBreakOnError);

		// get a character from the keyboard
		TChar ch = console->Getch(); 

		if ( ch == 'x' )
			return ETrue;
		else
			return EFalse;
		}
	}

void CScript::AddResult(enum TVerdict aTestVerdict )
/**
The end of the test has been reached.
Add the result to the current totals.

@param aTestVerdict The latest result.
*/
	{
	// another test complete, so increment total
	iTotal++;

	// add in the current result
	switch (aTestVerdict) 
		{
	case EPass:
		iPass++;
		break;
	case EFail:
		iFail++;
		break;
	case EInconclusive:
		iInconclusive++;
		break;
	case ETestSuiteError:
		iTestSuiteError++;
		break;
	case EAbort:
		iAbort++;
		break;
		}

	// display the result
	pLogSystem->LogResult(aTestVerdict, _L("Test Result for %S is %s "), 
		&(iParse->iCurrentStepName), 
		pLogSystem->TestResultText( aTestVerdict ) );
	
	// add a blank line
	pLogSystem->LogBlankLine();

	}

void CScript::AddResult(CScript * subScript )
/**
The end of a sub script has been reached.
Add the result to the totals

@param subScript Pointer to the sript object containing the results
*/
	{

	iPass+= subScript->iPass;
	iFail+= subScript->iFail;
	iInconclusive += subScript->iInconclusive;
	iTestSuiteError+= subScript->iTestSuiteError;
	iAbort+= subScript->iAbort;
	iTotal+=subScript->iTotal;
	}
