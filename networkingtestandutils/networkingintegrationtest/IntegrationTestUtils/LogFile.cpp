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
 @file LogFile.cpp
*/

// EPOC includes
#include <e32base.h>
#include <e32cons.h>

// Test system includes
#include "../inc/Log.h"

EXPORT_C void CFileLogger::CreateLog(const TDesC& aDir, const TDesC& aName)
/**
Create a log file.

@param aDir Directory in which to create the log file.
@param aName Log file name.
@note If the directory does not exsist then logging is disabled.
*/
	{
	// only log if adir exists
	TUint aAttValue;
	if ( iFs.Att(aDir, aAttValue) ==KErrNotFound) 
		{
		iEnabled = false;
		return;
		}
	else
		iEnabled = true;

	// make the correct file name
	TFileName LogFile;
	LogFile.Format( _L("\\logs\\testresults\\%S"), &aName );

	// If the file does not exist - create it
	TInt returnCode=iLogfile.Replace(iFs,LogFile,EFileWrite | EFileStreamText); 

	// check if open fails 
	if (returnCode==KErrNone )
		{
		//file has opened ok
		// add the start to the htm
		iLogfile.Write( _L8("<html><body><pre>\n") );
		}
	else
		iEnabled = false;
	};

EXPORT_C void CFileLogger::CloseLog( void )
/**
Close log file.
*/
	{

	// if logging enabled flush buffers
	if ( iEnabled )
		{
		iLogfile.Flush();
		iLogfile.Close();
		}

	// disconnect from the file server
	iFs.Close();
	}

EXPORT_C void CFileLogger::WriteFormat( TRefByValue<const TDesC16> format, ...  )
/**
Write formatted output to the log file. 

@param format "printf" format string
*/
	{

	if ( !iEnabled ) return ;

    VA_LIST aList;
	VA_START( aList, format );

	TIntegrationTestLog16Overflow iOverflow16;

	// decode formated data for display on console
	TBuf <MAX_LOG_LINE_LENGTH> LineBuf;
	TBuf8 <MAX_LOG_LINE_LENGTH> LineBuf8;

	// get the current time and date
	TTime now;
	now.HomeTime();
	TDateTime dateTime = now.DateTime() ;

	// add the current time and date 
	LineBuf.AppendFormat(_L("%02d/%02d/%04d\t%02d:%02d:%02d:%03d\t"),
		dateTime.Day()+1,
		dateTime.Month()+1,
		dateTime.Year(),
		dateTime.Hour(),
		dateTime.Minute(),
		dateTime.Second(),
		(dateTime.MicroSecond()/1000) ); 

	// followed by the formatted data
	LineBuf.AppendFormatList( format, aList,  &iOverflow16 );
	VA_END( aList ); 

	// convert from unicode to 8 bit
	for (TInt i = 0; i< LineBuf.Length() ;i++)
		LineBuf8.Append( (char)LineBuf[i] );


	// write to log file
	iLogfile.Write( LineBuf8 );
	}

EXPORT_C TInt CFileLogger::Connect( void )
/**
Connect to the file server.
*/
	{
	return iFs.Connect();
	}
