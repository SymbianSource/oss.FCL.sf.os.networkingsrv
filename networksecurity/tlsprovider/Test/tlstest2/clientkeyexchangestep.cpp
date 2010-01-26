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
 @file clientkeyexchangestep.cpp
 @internalTechnology
*/
#include "clientkeyexchangestep.h"

#include <tlsprovinterface.h>
#include <x509cert.h>
#include <asymmetric.h>
#include <asymmetrickeys.h>
#include <asnpkcs.h>

CClientKeyExchangeStep::CClientKeyExchangeStep()
	{
	SetTestStepName(KClientKeyExchangeStep);
	}

TVerdict CClientKeyExchangeStep::doTestStepPreambleL()
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
	

	if(UseNullCipher())
		{
		// Enables null cipher by setting appropiate parameter  
		atts->iAllowNullCipherSuites = ETrue;
 		}

	// No client authentication or dialogs for this test, please
	atts->iClientAuthenticate = EFalse;
	atts->iDialogNonAttendedMode = ETrue;    


	return EPass;
	}
	
TVerdict CClientKeyExchangeStep::doTestStepL()
	{
 	
	
  	INFO_PRINTF1(_L("Calling TLS Provider to fetch cipher suites."));
	
	// first we have to retrieve the available cipher suites
	TInt err = GetCipherSuitesL();
	
 	if (err != KErrNone)
		{
		INFO_PRINTF2(_L("Failed! Cannot retrieve supported cipher suites! (Error %d)"), err);
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
		
  	CleanupStack::PushL(keyExMessage);
	
	// examine the key exchange message, checking it is as expected.
 	ExamineKeyExchangeMessageL(*keyExMessage, CipherSuites()); 
	
 	CleanupStack::PopAndDestroy(keyExMessage);     

	return TestStepResult();
	}

/***
 *
 * Since we can't get the master secret out of the token from the test code,
 * verification that master secret is calculated correctly is done implicitly
 * in later steps, via the encrypt and decrypt operation steps.
 *
 */
	
void CClientKeyExchangeStep::ExamineKeyExchangeMessageL(const TDesC8& aMessage,
		const RArray<TTLSCipherSuite>& aCiphers)
	{
	// Look up the cipher suite we used for this test
	CTlsCryptoAttributes* atts = Provider()->Attributes();
	TInt index = KErrNotFound;
	for(TInt j=0;j<aCiphers.Count();j++)
		{
		if(aCiphers[j] == atts->iCurrentCipherSuite)
			{
			index = j;
			break;
			}
		}
	User::LeaveIfError(index);
	
	HBufC8* premaster = NULL;
  	
  	// Derive premaster secret only for non PSK cipher suits. 
  	if (!UsePsk())
		{
		INFO_PRINTF1(_L("Deriving Master Secret."));
 	 	premaster = DerivePreMasterSecretL(aMessage);
		}
	
	// currently, we only support two key exhange algorithms, so that is what we test here
	switch (aCiphers[index].CipherDetails()->iKeyExAlg)
		{
	case ERsa:
		{
		
		// first two bytes should be the version, and the whole message
		// should be 48 bytes long
		if ((*premaster)[0] != atts->iNegotiatedProtocol.iMajor ||
			(*premaster)[1] != atts->iNegotiatedProtocol.iMinor ||
			premaster->Length() != 48)
			{
			INFO_PRINTF1(_L("Failed! RSA key exchange message the wrong length/version!"));
			SetTestStepResult(EFail);
			}
		else
			{
			INFO_PRINTF1(_L("Test passed."));
			SetTestStepResult(EPass);
			}
		
		}
		break;
		
	case EDHE:
		// there isn't a whole lot we can do to verify the premaster secret
		// here... just make sure the buffer isn't empty
		if (aMessage.Length() != 0)
			{
			INFO_PRINTF1(_L("Test passed."));
			SetTestStepResult(EPass);
			}
		else
			{
			INFO_PRINTF1(_L("Failed! DHE key exchange message the wrong length!"));
			SetTestStepResult(EFail);
			}
		
		break;
		
	case EPsk:
	    
	    // for PSK cipher suites it checks content of message 
	    if (aMessage  != PskIdentity()->Des()  )
	    	{
	    	INFO_PRINTF1(_L("Failed! PSK key exchange message with wrong length/content"));
			SetTestStepResult(EFail);
	    	}
	   else 
	   		{
	   		INFO_PRINTF1(_L("Test passed."));
			SetTestStepResult(EPass);
	    	}
		break;
	
	default:
		User::Leave(KErrUnknown);
		break;
		}
	
	if(premaster)	
		delete premaster;
	
	}
