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


#include "tlstest2servercertstep.h"

#include <test/testexecutelogger.h>
#include <f32file.h>

CServerCertStep::CServerCertStep()
	{
	SetTestStepName(KServerCertStep);
	}

CServerCertStep::~CServerCertStep()
	{
	delete iProvider;
	}
	
TVerdict CServerCertStep::doTestStepL()
	{
	// Load the server certificate to use for this test
	TPtrC serverCertName;
	if (!GetStringFromConfig(ConfigSection(), KServerCert, serverCertName))
		{
		Logger().Write(_L("Failed to read server certificate file from ini."));
		SetTestStepResult(EFail);
		return TestStepResult();
		}

	Logger().WriteFormat(_L("Using server certificate file '%S'."), &serverCertName);
	
	RFs fs;
	User::LeaveIfError(fs.Connect());
	CleanupClosePushL(fs);
	
	RFile certFile;
	User::LeaveIfError(certFile.Open(fs, serverCertName, EFileRead));
	CleanupClosePushL(certFile);
	
	TInt size(0);
	User::LeaveIfError(certFile.Size(size));
	HBufC8* certData = HBufC8::NewLC(size);
	TPtr8 ptr = certData->Des();
	
	User::LeaveIfError(certFile.Read(ptr, size));
	
	TPtrC domainName;
	if (!GetStringFromConfig(ConfigSection(), KDomainName, domainName))
		{
		Logger().Write(_L("Failed to read domain name from ini."));
		SetTestStepResult(EFail);
		return TestStepResult();
		}
	
	Logger().WriteFormat(_L("Using domain name '%S'."), &domainName);
	Logger().Write(_L("Connecting to TLS provider."));
	
	CActiveScheduler* sched=NULL;
	sched=new(ELeave) CActiveScheduler;
	CleanupStack::PushL(sched);
	CActiveScheduler::Install(sched);
	
	iProvider = CTLSProvider::ConnectL();
	
	Logger().Write(_L("Setting TLS Parameters."));
	
	CTlsCryptoAttributes* atts = iProvider->Attributes();
	// set non-attended mode.
	atts->iDialogNonAttendedMode = ETrue;
	// set the domain name
	atts->idomainName.Copy(domainName);
	
	Logger().Write(_L("Running verify step."));
	CX509Certificate* cert = CX509Certificate::NewLC(*certData);
	
	CGenericActive* active = new (ELeave) CGenericActive;
	
	iProvider->VerifyServerCertificate(*certData, cert, active->iStatus);
	active->Start();
	CActiveScheduler::Start();
	
	TInt result = active->iStatus.Int();
	delete active;
	
	Logger().WriteFormat(_L("Certificate Validation Result Was: %d."), result);
	
	TInt expectedResult;
	
	if (!GetIntFromConfig(ConfigSection(), KExpectedResult, expectedResult))
		{
		Logger().Write(_L("Failed to get expected result from config, assuming KErrNone."));
		expectedResult = KErrNone;
		}
	else
		{
		Logger().WriteFormat(_L("Expected Validation Result Was: %d."), expectedResult);
		}
		
	if (result == expectedResult)
		{
		Logger().Write(_L("Test step passed."));
		SetTestStepResult(EPass);
		}
	else
		{
		Logger().Write(_L("Test step failed."));
		SetTestStepResult(EFail);
		}
	
	CActiveScheduler::Install(NULL);
	CleanupStack::PopAndDestroy(5, &fs); // certFile, certData, cert, sched
	return TestStepResult();
	}
