// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
 @file servcertwithdialogstep.cpp
 @internalTechnology
*/
#include "servcertwithdialogstep.h"
#include <tlsprovinterface.h>

CServCertWithDialogStep::CServCertWithDialogStep()
	{
	SetTestStepName(KServCertWithDialogStep);
	}

TVerdict CServCertWithDialogStep::doTestStepPreambleL()
	{
	ConstructL();
	// reads value for secure dialog (Yes or Not)
	if (EFalse == GetStringFromConfig(ConfigSection(),KDialogOption,iDialogOption))
		{	
		INFO_PRINTF1(_L("Expected DialogOption tag is required to continue the test"));
		User::Leave(KErrNotFound);
		}	
	return EPass;
	}
	
TVerdict CServCertWithDialogStep::doTestStepL()
	{
	// Deletes input and output files for dialog file.
	DeleteSecureDialogFilesL();
	
	// Sets secure dialog used by this test framework to act with a "Yes" when necessary. 
 	RFs fs;
	User::LeaveIfError(fs.Connect());
	CleanupClosePushL(fs);
	
	RFileWriteStream stream;
	TDriveUnit sysDrive (RFs::GetSystemDrive());
	TBuf<128> inputFile (sysDrive.Name());
	inputFile.Append(KInputFile);
	
	TInt err = stream.Open(fs, inputFile, EFileWrite | EFileShareExclusive);
	if (err == KErrNotFound)
		{
		err = stream.Create(fs, inputFile, EFileWrite | EFileShareExclusive);
		}
	User::LeaveIfError(err);
	stream.PushL();
		
 	SetDialogRecordL(stream, EServerAuthenticationFailure, _L("Passphrase of the imported key file"), iDialogOption, KNullDesC);
	stream.CommitL();
	CleanupStack::PopAndDestroy(2, &fs); // and stream      
		
	CTlsCryptoAttributes* atts = Provider()->Attributes();
	atts->iDialogNonAttendedMode = EFalse;  
	
	INFO_PRINTF1(_L("Calling TLS Provider Verify Certificate."));
	
	CX509Certificate* cert = NULL;
	err = VerifyServerCertificateL(cert);
	delete cert; // don't really need the cert
	
	TInt expectedResult;
	
	if (!GetIntFromConfig(ConfigSection(), KExpectedResult, expectedResult))
		{
		// failed to get expected result from config file... using KErrNone.
		expectedResult = KErrNone;
		}
	
	if (err != expectedResult)
		{
		INFO_PRINTF3(_L("Failed! TLS Provider returned error code %d, expecting %d."),
			err, expectedResult);
		SetTestStepResult(EFail);
		return EFail;
		}
	else
		{
		INFO_PRINTF1(_L("Test passed."));
		SetTestStepResult(EPass);
		}
	return TestStepResult();
	}
