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
 @file verifyCreateMethodStep.cpp
 @internalTechnology
*/
#include "verifyCreateMethodStep.h"

#include <tlsprovinterface.h>
#include <tlstypedef.h>


CCreateMethodStep::CCreateMethodStep()
	{
	SetTestStepName(KCreateMethodStep);
	}
	
TVerdict CCreateMethodStep::doTestStepPreambleL()
	{
	ConstructL();
	
	CTlsCryptoAttributes* atts = Provider()->Attributes();
	
	// read the "server" random
	HBufC8* random = ServerRandomL();
	atts->iMasterSecretInput.iServerRandom.Copy(*random);
	delete random;
	
	// and the client random
	random = ClientRandomL();
	atts->iMasterSecretInput.iClientRandom.Copy(*random);
	delete random;
	
	// we only support null compression...
	atts->iCompressionMethod = ENullCompression;
	
	// read the cipher suite for the test
	atts->iCurrentCipherSuite = CipherSuiteL();
	
	// read the protocol version
	TTLSProtocolVersion version = ProtocolVersionL();
	atts->iNegotiatedProtocol = version;
	atts->iProposedProtocol = version;
	
	// set the session ID and "server" name (localhost)
	atts->iSessionNameAndID.iSessionId = SessionId();
	atts->iSessionNameAndID.iServerName.iAddress = KLocalHost; 
	atts->iSessionNameAndID.iServerName.iPort = 443;
	atts->idomainName.Copy(DomainNameL());
	
	// try and read DH params, this section may not exist
	RInteger gen;
	CleanupClosePushL(gen);
	
	RInteger prime;
	CleanupClosePushL(prime);
	
	TRAPD(err, ReadDHParamsL());
	if (err == KErrNone)
		{
		atts->iPublicKeyParams->iKeyType = EDHE;
		
		// The params are:
		// 1 - Prime
		// 2 - Generator
		// 3 - generator ^ random mod prime
		
		atts->iPublicKeyParams->iValue1 = Prime().BufferLC();
		CleanupStack::Pop(atts->iPublicKeyParams->iValue1);
		
		atts->iPublicKeyParams->iValue2 = Generator().BufferLC();
		CleanupStack::Pop(atts->iPublicKeyParams->iValue2);
		
		atts->iPublicKeyParams->iValue3 = KeyPair()->PublicKey().X().BufferLC();
		CleanupStack::Pop(atts->iPublicKeyParams->iValue3);
		}
		
	CleanupStack::PopAndDestroy(2, &gen); // prime
	
	// No client auth, no dialogs
	atts->iClientAuthenticate = EFalse;
	atts->iDialogNonAttendedMode = ETrue;
	
	return EPass;
	}


TVerdict CCreateMethodStep::doTestStepL()
	{
	INFO_PRINTF1(_L("Calling TLS Provider to fetch cipher suites."));
	
	// first we have to retrieve the available cipher suites
	TInt err = GetCipherSuitesL();
	
	if (err != KErrNone)
		{
		INFO_PRINTF2(_L("Failed! Cannot retrieve supported cipher suites! (Error %d)"),
			err);
		SetTestStepResult(EFail);
		return TestStepResult();
		}
	
	// we have to verify the server certificate, to supply the certificate
	// and its parameters to the TLS provider.
	
	INFO_PRINTF1(_L("Calling TLS Provider to verify server certificate."));
	
	CX509Certificate* cert = NULL;
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
		
	
	INFO_PRINTF1(_L("Creating TLS Session."));	
	err = CreateSessionL();

	if(err != KErrNone)
		{
		INFO_PRINTF2(_L("Failed! Create Session failed! (Error %d)"),err);
		SetTestStepResult(EFail);
		return TestStepResult();
		}
	err = CreateSessionAddedL(0,-3);
	if(err == KErrNone)
		{
		INFO_PRINTF1(_L("Failed! Create Session Passed for invalid data)"));
		SetTestStepResult(EFail);
		return TestStepResult();
		}

	INFO_PRINTF1(_L("Create Session Test step Passed."));
	SetTestStepResult(EPass);
	return TestStepResult();
	}
	
