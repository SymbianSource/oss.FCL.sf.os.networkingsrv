// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
 @file verifyservercertstep.cpp
 @internalTechnology
*/
#include "verifyservercertstep.h"

#include <tlsprovinterface.h>

CVerifyServerCertStep::CVerifyServerCertStep()
	{
	SetTestStepName(KServerCertStep);
	}

TVerdict CVerifyServerCertStep::doTestStepPreambleL()
	{
	ConstructL();
	
	CTlsCryptoAttributes* atts = Provider()->Attributes();
	
	// set the session ID and "server" name (localhost)
	atts->iSessionNameAndID.iSessionId = SessionId();
	atts->iSessionNameAndID.iServerName.iAddress = KLocalHost; 
	atts->iSessionNameAndID.iServerName.iPort = 443;
	atts->idomainName.Copy(DomainNameL());
	
	// No client authentication or dialogs for this test, please
	atts->iClientAuthenticate = EFalse;
	atts->iDialogNonAttendedMode = ETrue;
	
	return EPass;
	}
	
TVerdict CVerifyServerCertStep::doTestStepL()
	{
	INFO_PRINTF1(_L("Calling TLS Provider Verify Certificate."));
	
	CX509Certificate* cert = NULL;
	TInt err = VerifyServerCertificateL(cert);
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
