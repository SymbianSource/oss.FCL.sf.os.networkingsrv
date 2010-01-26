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
 @file TestUtils.cpp
*/

// EPOC includes
#include <e32base.h>

#include <e32err.h>

// Test system includes
#include "../inc/Log.h"
#include "../inc/TestUtils.h"

#include <f32file.h>

EXPORT_C CTestUtils * CTestUtils::NewL( CLog * aLogSystem)
/**
Static constructor for CTestUtils.

@param aLogSystem A pointer to the test framework logging system.
*/
{
	CTestUtils * self = new(ELeave) CTestUtils;
	CleanupStack::PushL(self);
	self->Construct( aLogSystem);
	CleanupStack::Pop();
	return self;
}

void CTestUtils::Construct( CLog * aLogSystem)
/**
Second phase constructor for CTestUtils.

@param aLogSystem A pointer to the test framework logging system.
*/
{
	iLogSystem = aLogSystem;
}

EXPORT_C void CTestUtils::RunUtils( const TDesC& aText  )
/**
External interface for run utils.

@param aText Text specifying the utility required and any parameters.
@note This traps any leaves that may occur in the Test utilities.
*/
	{
	// now execute util inside a trap 
	TRAPD( r, RunUtilsL( aText ) )

	if (r!= KErrNone)
		{
		iLogSystem->Log(_L("Warning test utils :%S left!"), &aText );
		}
	}

void CTestUtils::RunUtilsL( const TDesC& aText  )
/**
Run a test utility.

@param aText Text specifying the utility required and any parameters.
@note This method can leave.
*/
	{
	// use Tlex to decode the cmd line
	TLex lex(aText);

	// start at the begining
	TPtrC token=lex.NextToken();

	// step over the keyword
	token.Set(lex.NextToken());

	// check which util required, then get any required parmeters
	if ( token.FindF( _L("ChangeDir")) != KErrNotFound )
		{
		// get the parameter
		token.Set(lex.NextToken());

		ChangeDirL(token);
		}
	else if ( token.FindF( _L("CopyFile")) != KErrNotFound )
		{
		// get the parameter
		TPtrC file1=lex.NextToken();
		TPtrC file2=lex.NextToken();

		CopyFileL(file1, file2);
		}
	else if ( token.FindF( _L("MkDir")) != KErrNotFound )
		{
		// get the parameter
		token.Set(lex.NextToken());

		MakedirL(token);
		}
	else if ( token.FindF( _L("delete")) != KErrNotFound )
		{
		// get the parameter
		token.Set(lex.NextToken());

		DeleteFileL(token);
		}
	else if ( token.FindF( _L("makereadwrite")) != KErrNotFound )
		{
		// get the parameter
		token.Set(lex.NextToken());

		MakeReadWriteL(token);
		}
	else
		{
		iLogSystem->Log( _L("Failed to decode run_utils command %S"), &aText );
		}
	}


void CTestUtils::ChangeDirL (const TDesC& aDirname) 
/**
Change current working directory.
This method can leave.

@param aDirname The name of the target directory.
@note This defaults to the C: drive.
*/
	{
	iLogSystem->Log( _L("\'ChangeDir %S\' failed, functionality not supported in EKA2"), &aDirname);
	_LIT(KChangeDirPanic, "ChangeDir");
	User::Panic(KChangeDirPanic, KErrNotSupported);
	}

void CTestUtils::MakedirL (const TDesC& aDirname) 
/**
Make new directory.
This method can leave.

@param aDirname The name of the new directory to be created.
@note This defaults to the C: drive.
*/
	{
	// parse the filenames
	_LIT(KDefault,"C:\\"); 
	TParse FullFileName;
	TInt returnCode = FullFileName.Set( aDirname, &KDefault, NULL );
	if ( returnCode != KErrNone )
		{
		TPtrC Errortxt = CLog::EpocErrorToText(returnCode);
		iLogSystem->Log( _L("Failed decode full path name %S - %S"), 
			&FullFileName.FullName(), 
			&Errortxt );
		}

	// create a fileserver
	RFs  FileSystem;

	// connect to file server
	returnCode=FileSystem.Connect();
	if ( returnCode != KErrNone )
		return;

	// now create the new directory
	returnCode = FileSystem.MkDir( FullFileName.DriveAndPath() ); 

	// check for errors
	if (returnCode == KErrNone )
		{
		// display full (including path) file name
		iLogSystem->Log( _L("made directory %S"), &FullFileName.FullName()  );	
		}
	else
		{		
		TPtrC Errortxt = CLog::EpocErrorToText(returnCode);

		iLogSystem->Log( _L("error %S making dir %S"), 
			&Errortxt, 
			&FullFileName.FullName()  );
		}

	}

void CTestUtils::CopyFileL (const TDesC& anOld,const TDesC& aNew) 
/**
Copy a file
This method can leave.

@param anOld The source name of file to be copied.
@param aNew The target name of file to be copied.
@note This defaults to the C: drive.
*/
	{
	iLogSystem->Log( _L("copy file from %S to %S"), &anOld, &aNew  );	

	// create a fileserver
	RFs  FileSystem;

	// connect to file server
	TInt returnCode=FileSystem.Connect();

	// create a file manager
	CFileMan * FileMan = CFileMan::NewL( FileSystem );
	if (returnCode != KErrNone )
		{
		// error opening FileManager
		iLogSystem->Log( _L("error opening file manager")); 
		}

	// parse the filenames
	_LIT(KRelated,"C:\\"); 
	TParse Source;
	returnCode = Source.Set( anOld, &KRelated, NULL );
	if ( returnCode != KErrNone )
		{
		TPtrC Errortxt = CLog::EpocErrorToText(returnCode);
		iLogSystem->Log( _L("Failed to parse %S - %S"), 
			&anOld, 
			&Errortxt );
		}
 
	// parse the filenames
	TParse Target;
	returnCode = Target.Set( aNew, &KRelated, NULL );
	if ( returnCode != KErrNone )
		{
		TPtrC Errortxt = CLog::EpocErrorToText(returnCode);
		iLogSystem->Log( _L("Failed to parse %S - %S"), 
			&aNew, 
			&Errortxt );
		}

	// do the copy
	returnCode=FileMan->Copy(Source.FullName(), 
		Target.FullName(), 
		CFileMan::EOverWrite );

	if ( returnCode != KErrNone )
		{
		TPtrC CopyErrortxt = CLog::EpocErrorToText(returnCode);

		iLogSystem->Log( _L("Failed to copy %S to %S - %S"), 
			&Source.FullName(), 
			&Target.FullName(),
			&CopyErrortxt );
		}
	else
		{
		iLogSystem->Log( _L("Copied file from %S to %S"), 
			&Source.FullName(), 
			&Target.FullName() );

		}

	// close the file system
	FileSystem.Close();

	delete FileMan;
	}

void CTestUtils::DeleteFileL (const TDesC& aFile) 
/**
Delete a file
This method can leave.

@param aFile Name of file to be deleted.
@note This defaults to the C: drive.
*/
	{
	// parse the filenames
	_LIT(KDefault,"C:\\"); 
	TParse FullFileName;
	TInt returnCode = FullFileName.Set( aFile, &KDefault, NULL );
	if ( returnCode != KErrNone )
		{
		TPtrC Errortxt = CLog::EpocErrorToText(returnCode);
		iLogSystem->Log( _L("Failed decode full path name %S - %S"), 
			&FullFileName.FullName(), 
			&Errortxt );
		}
	

	// create a fileserver
	RFs  FileSystem;

	// connect to file server
	returnCode=FileSystem.Connect();
	if ( returnCode != KErrNone )
		{
		iLogSystem->Log( _L("error connecting to FileSystem")  );	
		return;
		}

	// now do the delete
	returnCode = FileSystem.Delete(FullFileName.FullName()); 

	// check for errors
	if (returnCode == KErrNone )
		{
		// display full (including path) file name
		iLogSystem->Log( _L("deleted %S"), &FullFileName.FullName()  );	
		}
	else
		{		
		TPtrC Errortxt = CLog::EpocErrorToText(returnCode);

		iLogSystem->Log( _L("error %S deleting %S"), 
			&Errortxt, 
			&FullFileName.FullName()  );
		}

	}

void CTestUtils::MakeReadWriteL (const TDesC& aFile) 
/**
Enable a file for read and write.
This method can leave.

@param aFile Name of file.
@note This defaults to the C: drive.
*/
	{
	// parse the filenames
	_LIT(KDefault,"C:\\"); 
	TParse FullFileName;
	TInt returnCode = FullFileName.Set( aFile, &KDefault, NULL );
	if ( returnCode != KErrNone )
		{
		TPtrC Errortxt = CLog::EpocErrorToText(returnCode);
		iLogSystem->Log( _L("Failed decode full path name %S - %S"), 
			&FullFileName.FullName(), 
			&Errortxt );
		}
	

	// create a fileserver
	RFs  FileSystem;

	// connect to file server
	returnCode=FileSystem.Connect();
	if ( returnCode != KErrNone )
		{
		iLogSystem->Log( _L("error connecting to FileSystem")  );	
		return;
		}

	// now do the delete
	returnCode = FileSystem.SetAtt(FullFileName.FullName(),0, KEntryAttReadOnly ); 

	// check for errors
	if (returnCode == KErrNone )
		{
		// display full (including path) file name
		iLogSystem->Log( _L("Made %S RW"), &FullFileName.FullName()  );	
		}
	else
		{		
		TPtrC Errortxt = CLog::EpocErrorToText(returnCode);

		iLogSystem->Log( _L("error %S making %S RW"), 
			&Errortxt, 
			&FullFileName.FullName()  );
		}

	}
