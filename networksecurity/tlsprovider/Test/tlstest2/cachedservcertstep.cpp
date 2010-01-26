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
 @file encodedcertstep.cpp
 @internalTechnology
*/
#include "cachedservcertstep.h"

#include <tlsprovinterface.h>

CCachedServCertStep::CCachedServCertStep()
	{
	SetTestStepName(KCachedServCertStep);
	}

// Note this test step is not suitable to use with PSK cipher suites.	

TVerdict CCachedServCertStep::doTestStepL()
	{
	
	TRequestStatus status;
	
  	INFO_PRINTF1(_L("Calling TLS Provider to fetch cipher suites."));
	
	// first we have to retrieve the available cipher suites
	TInt err = GetCipherSuitesL();
	
 	if (err != KErrNone)
		{
		INFO_PRINTF2(_L("Failed! Cannot retrieve supported cipher suites! (Error %d)"), err);
		SetTestStepResult(EFail);
		return TestStepResult();
		}
	
		// we have to verify the server certificate, to supply the certificate
		// and its parameters to the TLS provider.

	INFO_PRINTF1(_L("Calling TLS Provider to verify server certificate."));

	CX509Certificate* cert = NULL;

	err = VerifyServerCertificateL(cert);
	
	CleanupStack::PushL(cert);
		
	// make sure it completed sucessfully.
	if (err != KErrNone)
		{
		INFO_PRINTF2(_L("Failed! Server Certificate did not verify correctly! (Error %d)"),
			err);
		SetTestStepResult(EFail);
		return TestStepResult();
		}		
			
  	INFO_PRINTF1(_L("Creating TLS Session."));	
	
	// now, create a session with the parameters set in the preamble
 	err = CreateSessionL();
	
	// ensure we succeeded
	if (err != KErrNone)
		{
		INFO_PRINTF2(_L("Failed! Create Session failed! (Error %d)"), err);
		SetTestStepResult(EFail);
		CleanupStack::PopAndDestroy(cert);
		return TestStepResult();
		}
	
 	INFO_PRINTF1(_L("Calling TLS session key exchange."));    
	
 	HBufC8* keyExMessage = NULL;
  	err = ClientKeyExchange(keyExMessage);
	
	if (err != KErrNone)
		{
		INFO_PRINTF2(_L("Failed! Key exchange failed! (Error %d)"), err);
		delete keyExMessage;
		SetTestStepResult(EFail);
		CleanupStack::PopAndDestroy(cert);
		return TestStepResult();
		}     
		
  	CleanupStack::PushL(keyExMessage);
	
	// examine the key exchange message, checking it is as expected.
 	ExamineKeyExchangeMessageL(*keyExMessage, CipherSuites()); 
 	
 	CX509Certificate* cachedCert = NULL;
 	
 	// Increases test code coverage. (Cancellation).
 	// Actually, this behaves as a syncronous method, so cancellation will have no effect
 	SessionServerCertificateWithCancel(cachedCert);
 	 	
	CleanupStack::PushL(cachedCert);
	//compare retrieved cert with original one:
	if ( cachedCert->IsEqualL( *cert ) == EFalse  ) 
		{
		INFO_PRINTF1(_L("Failed! Original server cert differs from cached one"));
		SetTestStepResult(EFail);   
		}
	else
		{
		INFO_PRINTF1(_L("Original server cert is equal to cached one"));
		}
	
	CleanupStack::PopAndDestroy(cachedCert);     
	CleanupStack::PopAndDestroy(keyExMessage);     
    CleanupStack::PopAndDestroy(cert);
	
	return TestStepResult();
	}


