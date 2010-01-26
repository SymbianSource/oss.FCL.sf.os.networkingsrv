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
// This contains CTestCase which is the base class for all the TestCase DLLs
// 
//

/**
 @file TestStep.cpp
*/

// EPOC includes
#include <e32base.h>

// Test system includes
#include "../inc/Log.h"
#include "../inc/TestStep.h"
#include "../inc/TestSuite.h"

EXPORT_C CTestStep::CTestStep(const TDesC &aName):iTestStepName(aName),iTestStepResult(EPass)
/**
constructor
*/
{}

EXPORT_C CTestStep::CTestStep():iTestStepResult(EPass)
/**
constructor
*/
{}

EXPORT_C CTestStep::~CTestStep()
/**
destructor
*/
	{
	}

EXPORT_C void CTestStep::Log( TRefByValue<const TDesC16> format, ... )
/**
general purpose log interface for TestSteps

@param format "printf" format string
*/
	{
    
	VA_LIST aList;
	VA_START( aList, format );

	TIntegrationTestLog16Overflow iOverflow16;

	// decode formated data for display on console
	TBuf <MAX_LOG_LINE_LENGTH> LineBuf;
	LineBuf.AppendFormatList( format, aList, &iOverflow16 );

	// send the data to the log system via the suite
	iSuite->Log( _L("%S"),&LineBuf );

	VA_END( aList ); 

	}

EXPORT_C void CTestStep::Log( TInt aSeverity, TRefByValue<const TDesC16> format, ... )
/**
Display the log data on the console. 
If there is a log file open write to it.

@param aSeverity severity level
@param format "printf" format string
*/
{
	VA_LIST aList;
	VA_START( aList, format );

	TIntegrationTestLog16Overflow iOverflow16;

	// decode formated data for display on console
	TBuf <MAX_LOG_LINE_LENGTH> LineBuf;
	LineBuf.AppendFormatList( format, aList, &iOverflow16 );

	// send the data to the log system via the suite
	if( aSeverity & iSuite->Severity()) iSuite->Log( aSeverity, _L("%S"),&LineBuf );

	VA_END( aList ); 
}

EXPORT_C void CTestStep::LogExtra(const TText8* aFile, TInt aLine, TInt aSeverity,
		TRefByValue<const TDesC> aFmt,...)
/**
Display the log data on the console 
if there is a log file open write to it.

@param aFile source file name
@param aLine line number
@param aSeverity severity level
@param aFmt "printf" format string
*/
	{
	VA_LIST aList;
	VA_START( aList, aFmt );

	TIntegrationTestLog16Overflow iOverflow16;

	// decode formated data for display on console
	TBuf <MAX_LOG_LINE_LENGTH> LineBuf;
	LineBuf.AppendFormatList( aFmt, aList, &iOverflow16 );

	// send the data to the log system via the suite
	if( aSeverity & iSuite->Severity()) 
		{
		iSuite->LogExtra(aFile, aLine, aSeverity, LineBuf );
		}

	VA_END( aList ); 
	}

EXPORT_C void CTestStep::LoadConfig( TDesC &config )
/**
If a config file is supplied, then create and load CIniData object

@param config config file parameter
*/
	{

	// if a config file supplied then use
	if ( config.Length() != 0)
		{

		// get the full pathname default drive name and extension
		_LIT(KRelated,"C:\\config.ini"); 
		TParse ConfigFileName;
		TInt returnCode = ConfigFileName.Set( config, &KRelated, NULL );

		if (returnCode != KErrNone )
			{
			// error opening FileManager
			ERR_PRINTF2( _L("Error opening config file %S"), &ConfigFileName.FullName()); 
			}

		// create and load the CIniData object
		TRAPD(r,iConfigData=CIniData::NewL(ConfigFileName.FullName() ));
		
		// check if loaded ok
		if ( r==KErrNone )
			{
			// loaded ok
			iConfigDataAvailable = ETrue;
			}
		else
			{
			// failed to load
			iConfigDataAvailable = EFalse;
			iConfigData = NULL;

			// report error 
			TPtrC Errortxt = CLog::EpocErrorToText(r);
			ERR_PRINTF2(_L("Failed to load config data file error= %S"), &Errortxt );
			}
		}
	}

EXPORT_C void CTestStep::UnLoadConfig( void )
/**
If a config file is supplied, then clean up Config data object
*/
	{
	iConfigDataAvailable = EFalse;

	// clean up Config data object
	if (iConfigData) delete iConfigData;
	}

EXPORT_C TBool CTestStep::GetBoolFromConfig(const TDesC &aSectName,const TDesC &aKeyName,TBool &aResult)
/**
Read a boolean value from the configuration file.

@param aSectName SectionName in the config file
@param aKeyName textname for bool value in the config file
@param aResult textname for bool value in the config file
@return ETrue or EFalse
*/
	{
	// check file available
	if ( !iConfigDataAvailable )
		{
		ERR_PRINTF1(_L("No config file available"));
		return EFalse;
		}

	TBool ret=EFalse;
	TPtrC result;

	// get the value 
	ret=iConfigData->FindVar( aSectName,aKeyName, result);

	// if failed to decode display error
	if (!ret) 
		{
		// display error message
		ERR_PRINTF3(_L("Failed to read section:%S key:%S "),
				&aSectName, &aKeyName );

		// return fail
		return EFalse;
		}

	// return result as a TBool
	( aResult =(result.FindF( _L("true")) ==  KErrNotFound )?  EFalse : ETrue  );	
	return ETrue;
	}

EXPORT_C TBool CTestStep::GetIntFromConfig(const TDesC &aSectName,const TDesC &aKeyName,TInt &aResult)
/**
Read a integer value from the configuration file.

@param aSectName SectionName in the config file
@param aKeyName textname for integer value in the config file
@param aResult textname for integer value in the config file
@return ETrue or EFalse
*/
	{
	// check file available
	if ( !iConfigDataAvailable )
		{
		ERR_PRINTF1(_L("No config file available"));
		return EFalse;
		}	
	TBool ret=EFalse;
	TPtrC result;

	// get the value 
	ret=iConfigData->FindVar( aSectName,aKeyName, result);

	// if failed to decode display error
	if (!ret) 
		{
		// display error message
		ERR_PRINTF3(_L("Failed to read section:%S key:%S "),
				&aSectName, &aKeyName );

		// return fail
		return EFalse;
		}

	// use Tlex to convert to a TInt
	TLex lex(result);
	if (lex.Val(aResult) == KErrNone  )
		return ETrue;
	else
		return EFalse;
}

EXPORT_C TBool CTestStep::GetStringFromConfig(const TDesC &aSectName,const TDesC &aKeyName,TPtrC &aResult)
/**
Read a string from the configuration file.

@param aSectName SectionName in the config file
@param aKeyName textname for string value in the config file
@param aResult textname for string value in the config file
@return ETrue or EFalse
*/
	{
	// check file available
	if ( !iConfigDataAvailable )
		{
		ERR_PRINTF1(_L("No config file available"));
		return EFalse;
		}	

	TBool ret=EFalse;

	// get the value 
	//TPtrC result;
	ret=iConfigData->FindVar( aSectName,aKeyName, aResult);

	// if failed to decode display error
	if (!ret) 
		{
		// display error message
		ERR_PRINTF3(_L("Failed to read section:%S key:%S "),
				&aSectName, &aKeyName );

		// return fail
		return EFalse;
		}

	return ETrue;
}

EXPORT_C enum TVerdict CTestStep::doTestStepPreambleL( void )
/**
Default empty implementation of doTestStepPreambleL().
Test steps can overide this to implement required code.
This can be used to provide common initialisation for the test steps
Preamble should only return EPass or EInconculsive.

@return EPass if the step completed successfully 
or EInconculsive, if the step did not attempt the run.
*/
	{
	return EPass;
	}

EXPORT_C enum TVerdict CTestStep::doTestStepPostambleL( void )
/**
Default empty implementation of doTestStepPostambleL().
Test steps can overide this to implement required code.
This can be used to provide common initialisation for the test steps
Postamble should only return EPass or EInconculsive.

@return EPass if the step completed successfully 
or EInconculsive, if the step did not attempt the run.
*/
	{
	return EPass;
	}

EXPORT_C void CTestStep::testBooleanTrue( TBool aCondition, char* aFile, TInt aLine )
/**
Check the boolean expression is true.
If not record error.

@param aCondition Expression to check.
@param aFile Current script file name.
@param aLine Current line number.
 */
	{

	// check condition
	if (aCondition)
		return;

	// this is only relevant if the current result is pass
	if ( iTestStepResult == EPass )
		iTestStepResult = EFail;

	// convert filename for log
	TBuf<MAX_LOG_FILENAME_LENGTH> fileName;
	fileName.Copy(TPtrC8((TText8*)aFile));

	// display a log message
 	ERR_PRINTF3(_L("Test Failed in file:%S line:%d"), &fileName, aLine);

	}

EXPORT_C void CTestStep::testBooleanTrueL( TBool aCondition, char* aFile, TInt aLine )
/**
Check the boolean expression is true.
If not record error and then leave.

@param aCondition Expression to check.
@param aFile Current script file name.
@param aLine Current line number.
*/
	{

	// check condition
	if (aCondition)
		return;

	// this is only relevant if the current result is pass
	if ( iTestStepResult == EPass )
		iTestStepResult = EFail;

	// convert filename for log
	TBuf<MAX_LOG_FILENAME_LENGTH> fileName;
	fileName.Copy(TPtrC8((TText8*)aFile));

	// display a log message
 	ERR_PRINTF3(_L("Test Failed in file:%S line:%d"), &fileName, aLine);

	// leave with error code
	User::Leave(TEST_ERROR_CODE);

	}

EXPORT_C void CTestStep::testBooleanTrueWithErrorCodeL( TBool aCondition, TInt errorCode  , char* aFile, TInt aLine )
/**
Check the boolean expression is true.
If not record error, error code and then leave.

@param aCondition Expression to check.
@param errorCode Error code.
@param aFile Current script file name.
@param aLine Current line number.
*/
	{
	// check condition
	if (aCondition)
		return;

	// this is only relevant if the current result is pass
	if ( iTestStepResult == EPass )
		iTestStepResult = EFail;

	// convert filename for log
	TBuf<MAX_LOG_FILENAME_LENGTH> fileName;
	fileName.Copy(TPtrC8((TText8*)aFile));

	// get the error text
	TPtrC Error = EpocErrorToText(errorCode);

	// display a log message
	ERR_PRINTF4(_L("Test Failed with error:%S in file:%S line:%d"),
			&Error, &fileName, aLine);

	// leave with error code
	User::Leave( errorCode );
	
	}

EXPORT_C void CTestStep::testBooleanTrueWithErrorCode( TBool aCondition, TInt errorCode  , char* aFile, TInt aLine )
/**
Check the boolean expression is true.
If not record error and error code.

@param aCondition Expression to check.
@param errorCode Error code.
@param aFile Current script file name.
@param aLine Current line number.
*/
	{
	// check condition
	if (aCondition)
		return;

	// this is only relevant if the current result is pass
	if ( iTestStepResult == EPass )
		iTestStepResult = EFail;

	// convert filename for log
	TBuf<MAX_LOG_FILENAME_LENGTH> fileName;
	fileName.Copy(TPtrC8((TText8*)aFile));

	// get the error text
	TPtrC Error = EpocErrorToText(errorCode);

	// display a log message
	ERR_PRINTF4(_L("Test Failed with error:%S in file:%S line:%d"),
			&Error, &fileName, aLine);
	}

EXPORT_C void CTestStep::TestCheckPointCompareL(TInt aVal,TInt aExpectedVal, 
												  const TDesC& aText, char* aFile,TInt aLine)
/**
Check the value against an expected value.
If not record error and error code.

@param aVal Value to check.
@param aExpectedVal Expected value.
@param aText Text for log file.
@param aFile Current script file name.
@param aLine Current line number.
*/
	{
	if(aVal != aExpectedVal)
		{

		// decode formated data for display on console
		TBuf <MAX_LOG_LINE_LENGTH> LineBuf;
		LineBuf.AppendFormat( _L("FAILED test:  Val = %d Exp Val = %d %S"), 
			aVal, aExpectedVal, &aText);

		// send the data to the log system via the suite
		iSuite->LogExtra((TText8*)aFile, aLine, ESevrErr, LineBuf );
		User::Leave(aVal);
		}
	}

EXPORT_C TPtrC CTestStep::EpocErrorToText(TInt aError)
/**
Convert a Error code to text.

@param aError error code to display as text.
@return Text describing the error.
*/
	{
	switch (aError)
		{
	case KErrNone:
		return _L("KErrNone");
	case KErrNotFound:
		return _L("KErrNotFound");
	case KErrGeneral:
		return _L("KErrGeneral");
	case KErrCancel:
		return _L("KErrCancel");
	case KErrNoMemory:
		return _L("KErrNoMemory");
	case KErrNotSupported:
		return _L("KErrNotSupported");
	case KErrArgument:
		return _L("KErrArgument");
	case KErrTotalLossOfPrecision:
		return _L("KErrTotalLossOfPrecision");
	case KErrBadHandle:
		return _L("KErrBadHandle");
	case KErrOverflow:
		return _L("KErrOverflow");
	case KErrUnderflow:
		return _L("KErrUnderflow");
	case KErrAlreadyExists:
		return _L("KErrAlreadyExists");
	case KErrPathNotFound:
		return _L("KErrPathNotFound");
	case KErrDied:
		return _L("KErrDied");
	case KErrInUse:
		return _L("KErrInUse");
	case KErrServerTerminated:
		return _L("KErrServerTerminated");
	case KErrServerBusy:
		return _L("KErrServerBusy");
	case KErrCompletion:
		return _L("KErrCompletion");
	case KErrNotReady:
		return _L("KErrNotReady");
	case KErrUnknown:
		return _L("KErrUnknown");
	case KErrCorrupt:
		return _L("KErrCorrupt");
	case KErrAccessDenied:
		return _L("KErrAccessDenied");
	case KErrLocked:
		return _L("KErrLocked");
	case KErrWrite:
		return _L("KErrWrite");
	case KErrDisMounted:
		return _L("KErrDisMounted");
	case KErrEof:
		return _L("KErrEof");
	case KErrDiskFull:
		return _L("KErrDiskFull");
	case KErrBadDriver:
		return _L("KErrBadDriver");
	case KErrBadName:
		return _L("KErrBadName");
	case KErrCommsLineFail:
		return _L("KErrCommsLineFail");
	case KErrCommsFrame:
		return _L("KErrCommsFrame");
	case KErrCommsOverrun:
		return _L("KErrCommsOverrun");
	case KErrCommsParity:
		return _L("KErrCommsParity");
	case KErrTimedOut:
		return _L("KErrTimedOut");
	case KErrCouldNotConnect:
		return _L("KErrCouldNotConnect");
	case KErrCouldNotDisconnect:
		return _L("KErrCouldNotDisconnect");
	case KErrDisconnected:
		return _L("KErrDisconnected");
	case KErrBadLibraryEntryPoint:
		return _L("KErrBadLibraryEntryPoint");
	case KErrBadDescriptor:
		return _L("KErrBadDescriptor");
	case KErrAbort:
		return _L("KErrAbort");
	case KErrTooBig:
		return _L("KErrTooBig");
	default:
		iPrnBuf.Format(_L(" %d"),aError);
		return iPrnBuf;
		}
	}


/**
	Active controller
*/
CActiveControl* CActiveControl::NewL(MControlNotify* aControl)
	{
	CActiveControl* self = new(ELeave) CActiveControl(aControl);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
	}

void CActiveControl::ConstructL()
	{
	}

CActiveControl::CActiveControl(MControlNotify* aControl)
: CActive(EPriorityStandard)
/**
constructor
*/
	{
	iControl = aControl;
	}

CActiveControl::~CActiveControl()
	{
	}

EXPORT_C void CActiveControl::ReStart()
	{
	TRequestStatus* status = &iStatus;
	SetActive();
	User::RequestComplete(status, KErrNone);
	}

void CActiveControl::RunL()
	{
	if(iControl->CallStateMachine())
		{
		SetActive();
		}
	}

void CActiveControl::DoCancel()
	{
	}
	
EXPORT_C CTestActiveStep::CTestActiveStep(const TDesC &aName):CTestStep(aName)
/**
constructor
*/
	{
	
	}
EXPORT_C CTestActiveStep::CTestActiveStep()
/**
constructor
*/
	{
	}
EXPORT_C CTestActiveStep::~CTestActiveStep()
/**
destructor
*/
	{
	}
EXPORT_C enum TVerdict CTestActiveStep::doTestStepPreambleL( void )
	{
	TVerdict retVal(EPass);
	if ((retVal=CTestStep::doTestStepPreambleL())!=EPass)
		{
		return retVal;
		}	
	iScheduler = new(ELeave) CActiveScheduler;
	CActiveScheduler::Install(iScheduler);
	iControl = CActiveControl::NewL(this);
	iScheduler->Add(iControl);
	iStatus = iControl->Status();	
	return retVal;		
	}
EXPORT_C enum TVerdict CTestActiveStep::doTestStepPostambleL( void )
	{
	TVerdict retVal(EPass);
	if ((retVal=CTestStep::doTestStepPostambleL())!=EPass)
		{
		return retVal;
		}
	iStatus=NULL;
	if (iControl)
		{
		delete iControl;
		iControl = NULL;
		}
	if (iScheduler)
		{
		delete iScheduler;
		iScheduler=NULL;
		}
	return retVal;	
	}	
