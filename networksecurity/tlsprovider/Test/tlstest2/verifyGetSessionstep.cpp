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
 @file verifyGetSessionStep.cpp
 @internalTechnology
*/
#include "verifyGetSessionstep.h"

#include <tlsprovinterface.h>

CVerifyGetSessionStep::CVerifyGetSessionStep()
	{
	SetTestStepName(KGetSessionStep);
	}

TVerdict CVerifyGetSessionStep::doTestStepPreambleL()
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
	
	// No client authentication or dialogs for this test, please
	atts->iClientAuthenticate = EFalse;
	atts->iDialogNonAttendedMode = ETrue;
	
	return EPass;
	}
	
TVerdict CVerifyGetSessionStep::doTestStepL()
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
	
	INFO_PRINTF1(_L("Calling TLS Provider Verify Certificate."));
	
	CX509Certificate* cert = NULL;
	err = VerifyServerCertificateL(cert);
	delete cert; // don't really need the cert
	
	TInt expectedResult;
	
	if (!GetIntFromConfig(ConfigSection(), KExpectedValue, expectedResult))
		{
		// failed to get expected result from config file... using KErrNone.
		expectedResult = KErrNone;
		}
	
	if (err != expectedResult)
		{
		INFO_PRINTF3(_L("Failed! TLS Provider returned error code %d, expecting %d."),
			err, expectedResult);
		SetTestStepResult(EFail);
		}
	else
		{
		INFO_PRINTF1(_L("Test passed."));
		SetTestStepResult(EPass);
		}
		
	err = CreateSessionL();
	
	// ensure we succeeded
	if (err != KErrNone)
		{
		INFO_PRINTF2(_L("Failed! Create Session failed! (Error %d)"), err);
		SetTestStepResult(EFail);
		}
	
	INFO_PRINTF1(_L("Calling TLS session key exchange."));
	
	HBufC8* keyExMessage = NULL;
	err = ClientKeyExchange(keyExMessage);
	
	if (err != KErrNone)
		{
		INFO_PRINTF2(_L("Failed! Key exchange failed! (Error %d)"), err);
		SetTestStepResult(EFail);
		}
	//Check for GetSessionL before caching	
	
	CTlsCryptoAttributes* tlsCryptoAttributes = Provider()->Attributes();
	TPtrC server2;
	_LIT(KServer2,"server2");
	GetStringFromConfig(KServerSection,KServer2,server2);
	tlsCryptoAttributes->iSessionNameAndID.iServerName.iAddress.Copy( server2 );

	TInt sessionIdLength(0) ;
	CleanupStack::PushL(keyExMessage);
	err = VerifyGetSessionL(tlsCryptoAttributes->iSessionNameAndID.iServerName,sessionIdLength);
	
	if (err != KErrNone)
		{
		INFO_PRINTF2(_L("Failed! GetSession failed! Before Caching (Error %d)"), err);
		SetTestStepResult(EFail);
		}
		
	// Call ServerFinishedStep
	INFO_PRINTF1(_L("Deriving premaster secret."));
			
	// derive the premaster secret from the key exchange method	
	HBufC8* premaster = DerivePreMasterSecretL(*keyExMessage);
	CleanupStack::PopAndDestroy(keyExMessage);
	
	INFO_PRINTF1(_L("Deriving master secret."));
	
	// compute the master secret from the premaster.
	CleanupStack::PushL(premaster);
	HBufC8* master = ComputeMasterSecretL(*premaster);
	CleanupStack::PopAndDestroy(premaster);
	CleanupStack::PushL(master);
	
	// do the caching 
	ValidateServerFinishL(*master);
	
	CleanupStack::PopAndDestroy(master);
	
	//TO check for GetSessionL
		
	err = VerifyGetSessionL(tlsCryptoAttributes->iSessionNameAndID.iServerName,sessionIdLength);
	if (err != KErrNone)
		{
		INFO_PRINTF2(_L("Failed! GetSession failed! After Caching (Error %d)"), err);
		SetTestStepResult(EFail);
		}
	
	//Verify GetSessionL for Non-cached Session  ( The sessionIdLength has to be of 0 length)
	TPtrC server3;
	_LIT(KServer3,"server3");
	GetStringFromConfig(KServerSection,KServer3,server3);
	
	CTlsCryptoAttributes* tlsCryptAttribs = Provider()->Attributes();
	tlsCryptAttribs->iSessionNameAndID.iServerName.iAddress.Copy( server3 );
	tlsCryptAttribs->iNegotiatedProtocol.iMajor = 0;
	tlsCryptAttribs->iNegotiatedProtocol.iMinor = 3; 
	
	tlsCryptAttribs->iProposedProtocol.iMajor = 0;
	tlsCryptAttribs->iProposedProtocol.iMinor = 3; 
		
	err = VerifyGetSessionL(tlsCryptAttribs->iSessionNameAndID.iServerName,sessionIdLength);
	if (err != KErrNone)
		{
		INFO_PRINTF2(_L("Failed! GetSession failed! For Non-cached session(Error %d)"), err);
		SetTestStepResult(EFail);
		}
	else if(sessionIdLength != 0) 
		{
		INFO_PRINTF2(_L("Failed! CTLSProvider::GetSession - wrong error code returned for non-cached session  %d"), err);
		SetTestStepResult(EFail);
		
		}	
	
	//
	TTLSSessionNameAndID sessionNameAndId;
	
	sessionNameAndId.iServerName.iAddress.Copy( server2 );
	sessionNameAndId.iServerName.iPort = 10;

	// Increases test code coverage (by using cancellation)
	err = ClearSessionCacheWithCancelL(sessionNameAndId);
	if (err != KErrCancel)
		{
		INFO_PRINTF2(_L("Failed! ClearSessionCacheL cancelled returned incorrect Error: %d"),
			err);
		SetTestStepResult(EFail);
		return TestStepResult();
		}
	
	err = ClearSessionCacheL(sessionNameAndId);
	if (err != KErrNone)
		{
		INFO_PRINTF2(_L("Failed! Clear Session Failed! (Error %d)"), err);
		SetTestStepResult(EFail);
		}
	
	sessionIdLength = 0;
	err = VerifyGetSessionL(tlsCryptoAttributes->iSessionNameAndID.iServerName,sessionIdLength);
	if (err != KErrNone)
		{
		INFO_PRINTF2(_L("Failed! GetSession failed! After Cache cleaned (Error %d)"), err);
		SetTestStepResult(EFail);
		}
	else if(sessionIdLength != 0) 
		{
		INFO_PRINTF2(_L("Failed! CTLSProvider::GetSession - wrong error code returned  %d"), err);
		SetTestStepResult(EFail);
		
		}
	
	CTLSSession* checkSession = Provider()->TlsSessionPtr();
	if(checkSession == NULL)
		{
		SetTestStepResult(EFail);
		}
 	
	return TestStepResult();
	}


void CVerifyGetSessionStep::ValidateServerFinishL(const TDesC8& aMasterSecret)
{
	// create a block of random data to represent our handshake messages,
	// and create hash objects from it.
	
	HBufC8* handshake = HBufC8::NewLC(1024); // totally arbitary length...
	TPtr8 handshakeBuf = handshake->Des();
	handshakeBuf.SetLength(1024);
	TRandom::RandomL(handshakeBuf);
	
	CMessageDigest* handshakeSha = CMessageDigestFactory::NewDigestLC(CMessageDigest::ESHA1);
	CMessageDigest* handshakeMd = CMessageDigestFactory::NewDigestLC(CMessageDigest::EMD5);
	
	handshakeSha->Update(handshakeBuf);
	handshakeMd->Update(handshakeBuf);
	
	INFO_PRINTF1(_L("Computing our test finished message."));
	
	// now, calculate our idea of what the finished message should be.
	HBufC8* ourFinished = ComputeFinishedMessageL(handshakeSha, handshakeMd, aMasterSecret, EFalse);
	CleanupStack::PushL(ourFinished);
	
	TInt expectedResult = KErrNone;
	TBool tamper = EFalse;
	if (GetBoolFromConfig(ConfigSection(), KTamperHandshakeMessage, tamper) && tamper)
		{
		INFO_PRINTF1(_L("Simulating man in the middle handshake tampering."));
		
		// we want to simulate a third party tampering with our handshake
		expectedResult = KErrBadServerFinishedMsg;
		TRandom::RandomL(handshakeBuf);
		
		handshakeSha->Reset();
		handshakeMd->Reset();;
		handshakeSha->Update(handshakeBuf);
		handshakeMd->Update(handshakeBuf);
		}
	
	INFO_PRINTF1(_L("Calling TLS Session to verify server finished message."));
	
	// ask TLS provider to verify our finished message
	TInt err = VerifyServerFinishedL(handshakeSha, handshakeMd, *ourFinished);
	if (err != expectedResult)
		{
		INFO_PRINTF3(_L("Failed! Expecting code %d, actual code %d."), expectedResult, err);
		SetTestStepResult(EFail);
		}
	else
		{
		INFO_PRINTF1(_L("Test passed."));
		SetTestStepResult(EPass);
		}
	CleanupStack::PopAndDestroy(4, handshake);	// handshakeSha, handshakeMd, ourFinished
	
}
