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
 @file multicancelstep.cpp
 @internalTechnology
*/
#include "multicancelstep.h"
#include <tlsprovinterface.h>


CMultiCancelStep::CMultiCancelStep()
	{
	SetTestStepName(KMultiCancelStep);
	}
	
TVerdict CMultiCancelStep::doTestStepL()
	{
	INFO_PRINTF1(_L("Calling TLS Provider to fetch cipher suites."));
	
	// Gets cipher suite with cancellation to increase test code coverage.
	TInt err = GetCipherSuitesWithCancelL();
	if (err != KErrNone && err != KErrCancel)
		{
		INFO_PRINTF2(_L("Unnexpected error when retrieving supported cipher suites! (with cancellation) (Error %d)"),			err);
		SetTestStepResult(EFail);
		return TestStepResult();
		}
	
	// first we have to retrieve the available cipher suites
	err = 0;
	err = GetCipherSuitesL();
	
	if (err != KErrNone)
		{
		INFO_PRINTF2(_L("Failed! Cannot retrieve supported cipher suites! (Error %d)"),
			err);
		SetTestStepResult(EFail);
		return TestStepResult();
		}
		
	// verifies certificate if is not a PSK cipher suite
  	if( !UsePsk() )
		{
			// we have to verify the server certificate, to supply the certificate
		// and its parameters to the TLS provider.

		INFO_PRINTF1(_L("Calling TLS Provider to verify server certificate, followed of cancellation of request."));

		CX509Certificate* cert = NULL;
		
		// Important part of the test: cancellation
 		err = VerifyServerCertificateWithCancelL(cert);
		delete cert;
		
			// make sure it returned was cancelled.
		if (err != KErrCancel)
			{
			INFO_PRINTF2(_L("Failed! Server Certificate cancelled returned incorrect Error: %d"),
				err);
			SetTestStepResult(EFail);
			return TestStepResult();
			}
				
		// Reissue same request but this time it does not cancel request.
		err = VerifyServerCertificateL(cert);
		delete cert;
				
		// make sure it completed sucessfully.
		if (err != KErrNone)
			{
			INFO_PRINTF2(_L("Failed! Server Certificate did not verify correctly! (Error %d)"),
					err);
			SetTestStepResult(EFail);
			return TestStepResult();
			}
		}   
	
  	INFO_PRINTF1(_L("Creating TLS Session."));	
  	// now, create a session with followed of a cancel of request
  	err = CreateSessionWithCancelL();
  	if (err != KErrCancel)
		{
		INFO_PRINTF2(_L("Failed! CreateSession cancelled returned incorrect Error: %d"),
			err);
		SetTestStepResult(EFail);
		return TestStepResult();
		}
  	
	// now, create a session with the parameters set in the preamble
	err = CreateSessionL();
	
	// Ensure we succeeded
	if (err != KErrNone)
		{
		INFO_PRINTF2(_L("Failed! Create Session failed! (Error %d)"), err);
		SetTestStepResult(EFail);
		return TestStepResult();
		}
	
	INFO_PRINTF1(_L("Calling TLS session key exchange with cancellation."));
	HBufC8* keyExMessage = NULL;
	
	// Important part of the test: cancellation
	err = ClientKeyExchangeWithCancel(keyExMessage);
	
	if (err != KErrCancel)
		{
		INFO_PRINTF2(_L("Client Key Exchange cancellation returned incorrect Error %d)"), err);
		delete keyExMessage;
		SetTestStepResult(EFail);
		return TestStepResult();
		}
	
	// Makes same request but this time does not cancel it
	err = ClientKeyExchange(keyExMessage);
		
	if (err != KErrNone)
		{
		INFO_PRINTF2(_L("Failed! Key exchange failed! (Error %d)"), err);
		delete keyExMessage;
		SetTestStepResult(EFail);
		return TestStepResult();
		}
	
	INFO_PRINTF1(_L("Deriving premaster secret."));
			
	// derive the premaster secret from the key exchange method	
	CleanupStack::PushL(keyExMessage);
	HBufC8* premaster = DerivePreMasterSecretL(*keyExMessage);
	CleanupStack::PopAndDestroy(keyExMessage);
	
	INFO_PRINTF1(_L("Deriving master secret."));
	
	// compute the master secret from the premaster.
	CleanupStack::PushL(premaster);
	HBufC8* master = ComputeMasterSecretL(*premaster);
	CleanupStack::PopAndDestroy(premaster);
	CleanupStack::PushL(master);
	
	ValidateServerFinishedL(*master);
		
	CleanupStack::PopAndDestroy(master);
			
	return TestStepResult();
		
	}

