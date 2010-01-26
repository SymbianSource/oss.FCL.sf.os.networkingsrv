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
 @file keyderivationstep.cpp
 @internalTechnology
*/
#include "keyderivationstep.h"
#include "psuedorandom.h"

#include <tlsprovinterface.h>

CKeyDerivationStep::CKeyDerivationStep()
	{
	SetTestStepName(KKeyDerivationStep);
	}
	
TVerdict CKeyDerivationStep::doTestStepPreambleL()
	{
	ConstructL();
	
	CTlsCryptoAttributes* atts = Provider()->Attributes();
	
	// Reads PSK values if included in INI file.
	ReadPskToBeUsedL();
	
	// Reads if NULL ciphers suites are to be allowed from INI file.
	ReadUseNullCipher();
	
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
	
	// If cipher suite under test is uses PSK (Pre Shared Key)
	if(UsePsk())
		{
		// Populates values for PSK 
		atts->iPskConfigured = true;
		atts->iPublicKeyParams->iKeyType = EPsk;
		atts->iPublicKeyParams->iValue4 = PskIdentity();
		atts->iPublicKeyParams->iValue5 = PskKey();
		}
	else 
		{
		// If cipher suite under test is NOT PSK 
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
		}
		
	CleanupStack::PopAndDestroy(2, &gen); // prime
	
	// No client authentication or dialogs for this test, please
	atts->iClientAuthenticate = EFalse;
	atts->iDialogNonAttendedMode = ETrue;
	
	if(UseNullCipher())
		{
		// Enables null cipher by setting appropiate parameter  
		atts->iAllowNullCipherSuites = ETrue;
 		}
	
	return EPass;
	}
	
TVerdict CKeyDerivationStep::doTestStepL()
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
	
	// verifies certificate if is not a PSK cipher suite
  	if( !UsePsk() )
		{
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
		
		}   
	
	INFO_PRINTF1(_L("Creating TLS Session."));	
	
	// now, create a session with the parameters set in the preamble
	err = CreateSessionL();
	
	// ensure we succeeded
	if (err != KErrNone)
		{
		INFO_PRINTF2(_L("Failed! Create Session failed! (Error %d)"), err);
		SetTestStepResult(EFail);
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

	// now generate what we think the derived EAP key block should look like.
	TBuf8<192> ourEAP;
	
	CTlsCryptoAttributes* atts = Provider()->Attributes();
	TBuf8<64> random;
	random.Append(atts->iMasterSecretInput.iClientRandom);
	random.Append(atts->iMasterSecretInput.iServerRandom);
	
	// make sure we're using TLS. This step makes no sense for SSL 3.0
	if (atts->iNegotiatedProtocol.iMajor == 3 && atts->iNegotiatedProtocol.iMinor == 0)
		{
		INFO_PRINTF1(_L("Error! Cannot use this test step with SSLv3!"));
		User::Leave(KErrNotSupported);
		}
	
	INFO_PRINTF1(_L("Computing our derived EAP-TLS key."));
	
	// compute the 128 byte block that uses the master secret as key.
	_LIT8(KEAPEncryptionLabel, "client EAP encryption");
	HBufC8* block1 = CTls10PsuedoRandom::PseudoRandomL(*master, KEAPEncryptionLabel, random, 128);
	ourEAP.Append(*block1);
	delete block1;
	
	// compute the 64 byte IV block
	HBufC8* block2 = CTls10PsuedoRandom::PseudoRandomL(KNullDesC8, KEAPEncryptionLabel, random, 64);
	ourEAP.Append(*block2);
	delete block2;
	
	INFO_PRINTF1(_L("Calling TLS Session key derivation."));
	
	// get the TLS provider's idea of what the EAP keyblock should be, and check they match.
	TBuf8<192> theirEAP;
	User::LeaveIfError(Session()->KeyDerivation(KEAPEncryptionLabel, atts->iMasterSecretInput, theirEAP));
	
	if (ourEAP == theirEAP)
		{
		INFO_PRINTF1(_L("Test passed."));
		SetTestStepResult(EPass);
		}
	else
		{
		INFO_PRINTF1(_L("Failed! EAP-TLS is corrupt!"));	
		SetTestStepResult(EFail);
		}
	
	CleanupStack::PopAndDestroy(master);
	return TestStepResult();
	}
