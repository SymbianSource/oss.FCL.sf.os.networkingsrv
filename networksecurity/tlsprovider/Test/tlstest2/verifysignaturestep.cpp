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
 @file verifysignaturestep.cpp
 @internalTechnology
*/
#include "verifysignaturestep.h"

#include <tlsprovinterface.h>
#include <asymmetric.h>
#include <asnpkcs.h>
#include <asn1enc.h>

CVerifySignatureStep::CVerifySignatureStep()
	{
	SetTestStepName(KVerifySignatureStep);
	}
	
TVerdict CVerifySignatureStep::doTestStepPreambleL()
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
	
	// No client authentication or dialogs for this test, please
	atts->iClientAuthenticate = EFalse;
	atts->iDialogNonAttendedMode = ETrue;
	
	return EPass;
	}
	
TVerdict CVerifySignatureStep::doTestStepL()
	{
	INFO_PRINTF1(_L("Calling TLS Provider to fetch cipher suites."));
	// first we have to retrieve the available cipher suites
	TInt err = GetCipherSuitesL();
	
	if (err != KErrNone)
		{
		SetTestStepResult(EFail);
		return TestStepResult();
		}
	
	INFO_PRINTF1(_L("Calling TLS Provider to verify server certificate."));
	CX509Certificate* cert = NULL;
	err = VerifyServerCertificateL(cert);
	CleanupStack::PushL(cert);

	if (err != KErrNone)
		{
		SetTestStepResult(EFail);
		return TestStepResult();
		}
		
	// sign some random data, and call it a signature....
	// doesn't have to be an actual digest, since we don't pass the data buffer.
	
	TBuf8<36> digest;
	CTlsCryptoAttributes* atts = Provider()->Attributes();

	TInt index = KErrNotFound;
	for(TInt j=0;j<CipherSuites().Count();j++)
		{
		if(CipherSuites()[j] == atts->iCurrentCipherSuite)
			{
			index = j;
			break;
			}
		}
	User::LeaveIfError(index);

	TTLSCipherSuite suite = CipherSuites()[index];
	// ajust the pseudo-digest size for sig algorithm
	switch(suite.CipherDetails()->iSigAlg)
		{
	case EDsa:
		atts->isignatureAlgorithm = EDsa;
		digest.SetLength(20); // only one SHA-1 in size
		break;
	
	case ERsaSigAlg:
		atts->isignatureAlgorithm = ERsaSigAlg;
		digest.SetLength(36); // MD5 + SHA-1 in size
		break;
	
	default:
		User::Leave(KErrUnknown);
		break;
		}
	
	TRandom::RandomL(digest);
	
	// get the server private key data
	CDecPKCS8Data* keyData = ServerPrivateKeyL();
	CleanupStack::PushL(keyData);
	
	// sign the "digest" with the appropriate signature algorithm
	HBufC8* signature = NULL;
	switch(suite.CipherDetails()->iSigAlg)
		{
	case EDsa:
		{
		// we don't own this pointer
		CPKCS8KeyPairDSA* key = static_cast<CPKCS8KeyPairDSA*>(keyData->KeyPairData());
		
		CDSASigner* signer = CDSASigner::NewLC(key->PrivateKey());
		const CDSASignature* sig = signer->SignL(digest);
		CleanupStack::PushL(const_cast<CDSASignature*>(sig));
		
		// DSA sig is an ASN.1 sequence of R followed by S
		CASN1EncSequence* sigSeq = CASN1EncSequence::NewLC();
		
		CASN1EncBigInt* rAsn = CASN1EncBigInt::NewLC(sig->R());
		sigSeq->AddAndPopChildL(rAsn);
		
		CASN1EncBigInt* sAsn = CASN1EncBigInt::NewLC(sig->S());
		sigSeq->AddAndPopChildL(sAsn);
		
		signature = HBufC8::NewLC(sigSeq->LengthDER());
		TPtr8 sigPtr = signature->Des();
		sigPtr.SetLength(sigSeq->LengthDER());
		TUint pos = 0;
		sigSeq->WriteDERL(sigPtr, pos);
		CleanupStack::Pop(signature);
		
		CleanupStack::PopAndDestroy(3, signer); //sig, sigSeq
		}
		break;

	case ERsaSigAlg:
		{
		// we don't own this pointer
		CPKCS8KeyPairRSA* key = static_cast<CPKCS8KeyPairRSA*>(keyData->KeyPairData());
		
		CRSAPKCS1v15Signer* signer = CRSAPKCS1v15Signer::NewLC(key->PrivateKey());
		const CRSASignature* sig = signer->SignL(digest);
		CleanupStack::PushL(const_cast<CRSASignature*>(sig));
		
		signature = sig->S().BufferLC();
		CleanupStack::Pop(signature);
	
		CleanupStack::PopAndDestroy(2, signer); // sig
		}
		break;
		
	default:
		User::Leave(KErrUnknown);
		break;
		}
	
	CleanupStack::PushL(signature);
	
	//Code to check VerifySignatureL()
	TBool expectedResult = ETrue;
	
	// security test - we may wish to munge the digest we pass to the method
	TBool tamperDigest(EFalse);
	GetBoolFromConfig(ConfigSection(), KTamperedDigest, tamperDigest); // ignore error
	
	if (tamperDigest)
		{
		expectedResult = EFalse;
		TRandom::RandomL(digest);
		}
	// now, invoke the verify signature method.
	TBool result = Provider()->VerifySignatureL(cert->PublicKey(), digest, *signature);
	
	if (result == expectedResult)
		{
		SetTestStepResult(EPass);
		}
	else
		{
		SetTestStepResult(EFail);
		}
		
	//now invoke the verify signature method with invalid params and ensure that it fails accordingly
		TRandom::RandomL(digest);
	result = Provider()->VerifySignatureL(cert->PublicKey(), digest, *signature);
			expectedResult = EFalse;
	if (result == expectedResult)
		{
		SetTestStepResult(EPass);
		}
	else
		{
		SetTestStepResult(EFail);
		}
	
	CleanupStack::PopAndDestroy(3, cert); // keyData, signature
	return TestStepResult();
	}
