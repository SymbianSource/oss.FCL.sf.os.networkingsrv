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
//

/**
 @file Log.cpp
*/

// EPOC includes
#include <e32base.h>
#include <e32cons.h>

// Test system includes
#include "../inc/Log.h"

EXPORT_C CLog * CLog::NewL( CConsoleBase * console )
/**
Epoc style constuctor

@param console A pointer to the current console object.
@return A new CLog object.
*/

	{
	CLog * self = new(ELeave) CLog;
	CleanupStack::PushL(self);
	self->Construct( console );
	CleanupStack::Pop();
	return self;
	}

EXPORT_C void CLog::Construct( CConsoleBase * console )
/**
Epoc style 2nd phase constructor. 
Initialise the Utils.

@param console A pointer to the current console object.
@return A new CLog object.
*/
	{
	// the log system needs the current console
	// for the console log display
	iConsole = console; 
	iSeverity = ESevrAll;
	//Do we need to put information about source file & #line?
	//Default is yes.
	iPutSrcInfo = ETrue;
	iHtmlLogMode = ETrue ;
	}

EXPORT_C CLog::~CLog()
/**
destructor
*/
	{
	}

EXPORT_C void CLog::OpenLogFileL(const TFileName &aScriptFileName)
/**
Open the test log file.

@param aScriptFileName The script file name which is used to generate the log file name.
*/
	{

	// get the full pathname default drive name and extension
	_LIT(Kdefault,"c:\\xx.htm"); 
	TParse ScriptFileName;

	// combine C:\\xx.htm and the whole parameter supplied
	TInt returnCode = ScriptFileName.Set( aScriptFileName, &Kdefault, NULL );

	// overright extension if supplied with .htm
	returnCode = ScriptFileName.Set( ScriptFileName.Name(), &Kdefault, NULL );
	if (returnCode != KErrNone )
		return;

	// connect to the server
	TInt ret = iFileLogger.Connect();
	if (ret == KErrNone )
		{
		// create the log file in fixed directory c:\logs\Testresults
		iFileLogger.CreateLog(_L("C:\\Logs\\TestResults"), ScriptFileName.NameAndExt());
		}
	}

EXPORT_C void CLog::Log( TRefByValue<const TDesC16> format, ... )
/**
Display the log data on the console. 
If there is a log file open write to it.

@param format "printf" format string
*/
	{
    VA_LIST aList;
	VA_START( aList, format );

	TIntegrationTestLog16Overflow iOverflow16;

	// decode formated data for display on console
	TBuf <MAX_LOG_LINE_LENGTH> LineBuf;
	LineBuf.AppendFormatList( format, aList, &iOverflow16 );

	// write to the console
	iConsole->Printf(_L("%S\n"),&LineBuf );

	// write to log file
	iFileLogger.WriteFormat(_L("%S\n"),&LineBuf);

	VA_END( aList ); 
	}

EXPORT_C void CLog::SetPutSrcInfo( TBool aPutSrcInfo )
/**
Enable or disable souve information in the log file.

@param aPutSrcInfo The error code.
*/
	{
	iPutSrcInfo=aPutSrcInfo;
	}
	
EXPORT_C void CLog::Log( TInt aSeverity, TRefByValue<const TDesC16> format, ... )
/**
Display the log data on the console. 
If there is a log file open write to it.

@param aSeverity severity level
@param format "printf" format string
*/
{
    VA_LIST aList;
	VA_START( aList, format );

	if( aSeverity & Severity()) 
		{
		Log( format, aList);
		}

	VA_END( aList ); 
}

EXPORT_C void CLog::Log( TRefByValue<const TDesC16> format, VA_LIST aList )
/**
Display the log data on the console 
If there is a log file open write to it.

@param format "printf" format string.
@param aList Variable length argument list.
*/
	{
	// decode formated data for display on console
	TBuf <MAX_LOG_LINE_LENGTH> LineBuf;
	TIntegrationTestLog16Overflow iOverflow16;

	LineBuf.AppendFormatList( format, aList, &iOverflow16 );


	// write to log file
	iFileLogger.WriteFormat(_L("%S\n"),&LineBuf);

	// write to the console
	iConsole->Printf(_L("%S\n"),&LineBuf );
	}



EXPORT_C void CLog::LogExtra(const TText8* aFile, TInt aLine, TInt aSeverity,
		TRefByValue<const TDesC> aFmt, VA_LIST aList)
/**
Display the log data on the console 
if there is a log file open write to it

@param aFile source file name.
@param aLine line number.
@param aSeverity severity level.
@param aFmt "printf" format string.
@param aList List of printf params.
@note should be used for macros only
*/
{
	if( aSeverity & Severity()) 
	{

		TIntegrationTestLog16Overflow iOverflow16;

		// decode formated data for display on console
		TBuf <MAX_LOG_LINE_LENGTH> LineBuf;

		LineBuf.AppendFormatList( aFmt, aList, &iOverflow16 );
		// write to the console
		iConsole->Printf(_L("%S\n"),&LineBuf );
		//Do we need to put information about source file & #line?
		if(iPutSrcInfo)
		{		// Braces used to scope lifetime of TBuf objects
			TPtrC8 fileName8(aFile);
			TBuf<128> fileName;
			TParse printFileName ;
			fileName.Copy(fileName8);  //TText8->TBuf16
			//We don't need full file name.
			printFileName.Set(fileName, NULL, NULL) ;
			fileName.Copy(printFileName.NameAndExt()) ;
			// write to log file
			iFileLogger.WriteFormat(_L("%S\t%d\t%S\n"),&fileName, aLine, &LineBuf);
		}
		else
		{
	// write to log file
	iFileLogger.WriteFormat(_L("%S\n"),&LineBuf);
		}



	}

}

EXPORT_C void CLog::LogExtra(const TText8* aFile, TInt aLine, TInt aSeverity,
		TRefByValue<const TDesC> aFmt,...)
/**
Display the log data on the console 
if there is a log file open write to it.

@param aFile source file name
@param aLine line number
@param aSeverity severity level
@param aFmt "printf" format string
@note should be used for macros only
*/
	{
	VA_LIST aList;
	VA_START( aList, aFmt );
	LogExtra(aFile, aLine, aSeverity, aFmt, aList);
	VA_END( aList ); 
	}


EXPORT_C void CLog::LogResult( TVerdict ver, TRefByValue<const TDesC16> format, ... )
/**
Display highlighted results on the console. 
If there is a log file open write to it.

@param ver The test result.
@param format A printf style format string.
*/
	{
    VA_LIST aList;
	VA_START( aList, format );

	TIntegrationTestLog16Overflow iOverflow16;

	// decode formated data for display on console
	TBuf <MAX_LOG_LINE_LENGTH> LineBuf;
	LineBuf.AppendFormatList( format, aList, &iOverflow16 );

	// write to the console
	iConsole->Printf(_L("%S\n"),&LineBuf );

	// write to log file
	if( iHtmlLogMode)
		{ 
	switch( ver)
		{
	case EPass:
			ERR_PRINTF2(_L("<font size=4 color=00AF00>%S</font>\n"),&LineBuf );
		break;
	case EFail:
			ERR_PRINTF2(_L("<font size=4 color=FF0000>%S</font>\n"),&LineBuf );
		break;
	case EInconclusive:
			ERR_PRINTF2(_L("<font size=4 color=0000FF>%S</font>\n"),&LineBuf );
		break;
	case ETestSuiteError:
			ERR_PRINTF2(_L("<font size=4 color=0000FF>%S</font>\n"),&LineBuf );
		break;
	case EAbort:
			ERR_PRINTF2(_L("<font size=4 color=0000FF>%S</font>\n"),&LineBuf );
		}
	}
	else
	{
			ERR_PRINTF2(_L("%S\n"),&LineBuf );
		}	
	VA_END( aList ); 
	}

EXPORT_C TPtrC CLog::EpocErrorToText(TInt aError)
/**
Convert a SymbianOS error code to text.

@param aError The error code.
@return Text describing the error code.
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
		return _L("Unknown");
		}
	}


EXPORT_C const TText* CLog::TestResultText( enum TVerdict aTestVerdict )
/**
Turn a test verdict into text.

@param aTestVerdict The test result.
@return Text describing the current verdict.
*/
	{
	switch ( aTestVerdict )
		{
	case EPass:
		return _S("PASS");
	case EFail:
		return _S("FAIL");
	case EInconclusive:
		return _S("INCONCLUSIVE");
	case ETestSuiteError:
		return _S("TEST_SUITE_ERROR");
	case EAbort:
		return _S("ABORT");
	default:
		return _S("What");

		}
	}

EXPORT_C void CLog::LogBlankLine( TInt number )
/**
Add a number of blank lines.

@param number The number of blank lines to add to the Log.
*/
	{
	for (TInt i =0; i< number; i++)
		Log(_L(" "));
	}

EXPORT_C void CLog::CloseLogFile()
/**
Close the log file.
*/
	{
	// add the htm end
	iFileLogger.WriteFormat(_L("</end>"));
	
	// close flogger
	iFileLogger.CloseLog();
	}

void TIntegrationTestLog16Overflow::Overflow(TDes16& /*aDes*/)
/**
This function is called if the format text overflows the internal buffer.
*/
	{

	User::Panic(_L("Log output buffer overflow"),1);

	}

EXPORT_C void CLog::SetSeverity( TInt aSeverity )
/**
Set the current severity level.

@param aSeverity The new severity level.
*/
{
	iSeverity = aSeverity;
}

EXPORT_C TInt CLog::Severity()
/**
Get the current severity level.

@return The current severity level.
*/
{
	return iSeverity;
}


