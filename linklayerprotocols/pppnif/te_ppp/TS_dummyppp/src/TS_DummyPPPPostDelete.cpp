// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// PRECOPY.CPP
// @file
// @internalComponent
// Pre Copy file preparation
// 
//

// EPOC includes
#include <e32base.h>
#include <f32file.h>

// GenConn Test system includes
#include "TS_DummyPPPStep.h"

//
//
// Test Post Delete
/* This New Class is added to move deletefile command from script file
   This test does a delete operation 	
*/
CDummyPPPPostDelete::CDummyPPPPostDelete()
	{
	// store the name of this test case
	iTestStepName = _L("PostDeleteTest");
	}

CDummyPPPPostDelete::~CDummyPPPPostDelete()
	{
	}

// Delete Test Main Code
enum TVerdict CDummyPPPPostDelete::doTestStepL( void )
	{
	deleteFileL(KAgentDestPath);							// delete agentdialog.ini
	TRAPD (error, copyFileL(KPppTempPath, KPppDestPath));	// restore original ppp.ini if it was backed up
	if (error != KErrNone)
		{
		deleteFileL (KPppDestPath) ;						// else delete it if it didn't originally exist
		}
	return EPass;
	}


void CDummyPPPPostDelete::deleteFileL (const TDesC& aFileName)
	{
	// create a fileserver
	RFs  fileSystem;

	User::LeaveIfError(fileSystem.Connect());
	CleanupClosePushL(fileSystem);

	// Remove read only flag
	TInt ret = fileSystem.SetAtt(aFileName, 0, KEntryAttReadOnly);
	if (ret == KErrNotFound)
		{
		// If file already removed then no need to delete it
		Log(_L("File not found"));
		CleanupStack::PopAndDestroy(&fileSystem);
		return;
		}
	User::LeaveIfError(ret);

	Log(_L("Set file to read only"));

	// Delete file
	User::LeaveIfError(fileSystem.Delete(aFileName));
	Log(_L("deleted file"));

	// clean up
	CleanupStack::PopAndDestroy(&fileSystem);
	}
