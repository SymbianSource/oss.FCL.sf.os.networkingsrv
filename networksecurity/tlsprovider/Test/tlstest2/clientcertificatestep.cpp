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
 @file clientcertificatestep.cpp
 @internalTechnology
*/
#include "clientcertificatestep.h"

#include <tlsprovinterface.h>
#include <x509cert.h>
#include <asnpkcs.h>

//#include <ssl.h>
//#include "pkixcertchain.h"

CClientCertificateStep::CClientCertificateStep()
	{
	SetTestStepName(KClientCertificateStep);
	}
	
TVerdict CClientCertificateStep::doTestStepPreambleL()
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
	
	// We do indeed want client authentication for this test
	atts->iClientAuthenticate = ETrue;
	atts->iDialogNonAttendedMode = ETrue;
	
	// we want either RSA or DSA signing certificates
	atts->iReqCertTypes.Append(ERsaSign);
	atts->iReqCertTypes.Append(EDssSign);
	
	return EPass;
	}

void FreeCertArray(TAny* aArray)
	{
	 RPointerArray<HBufC8>* certArray = (RPointerArray<HBufC8>*)aArray;
	 for ( TInt n = 0; n < certArray->Count(); n++ )
	      	{
	      	HBufC8* buf = (HBufC8*)((*certArray)[n]);
	      	delete buf;
	      	}
	 certArray->Close();
	}


TVerdict CClientCertificateStep::doTestStepL()
	{
	// first we have to retrieve the available cipher suites
	TInt err = GetCipherSuitesL();
	
	if (err != KErrNone)
		{
		ERR_PRINTF2(_L("Failed! GetCipherSuitesL returned %d"), err);
		SetTestStepResult(EFail);
		SetTestStepError(err);
		return TestStepResult();
		}
	
	// we have to verify the server certificate, to supply the certificate
	// and its parameters to the TLS provider.
	
	CX509Certificate* cert = NULL;
	err = VerifyServerCertificateL(cert);
	delete cert;
	
	// make sure it completed sucessfully.
	if (err != KErrNone)
		{
		ERR_PRINTF2(_L("Failed! VerifyServerCertificateL returned %d"), err);
		SetTestStepResult(EFail);
		SetTestStepError(err);
		return TestStepResult();
		}
		
	// Set the certificate names we want to use for client authentication
	
	CTlsCryptoAttributes* atts = Provider()->Attributes();
	
	HBufC8* cert2 = ServerCertificateL();
	CleanupStack::PushL(cert2);
	
	atts->isignatureAlgorithm = ERsaSigAlg;
	CX509Certificate* sCert = CX509Certificate::NewLC(*cert2);
	
	HBufC *pIssuer = sCert->IssuerL();
	CleanupStack::PushL(pIssuer);
	INFO_PRINTF2(_L("Requested Issuer: %S"), pIssuer);
	CleanupStack::PopAndDestroy(pIssuer);

	TBool hasServerDN;
	if ( !GetBoolFromConfig(ConfigSection(), KServerDNAvailable, hasServerDN) )
		{
		hasServerDN = ETrue;
		}
	
	if(hasServerDN)
	{
		HBufC8* dName = sCert->DataElementEncoding(CX509Certificate::EIssuerName)->AllocLC();
		atts->iDistinguishedCANames.AppendL(dName);
		CleanupStack::Pop(dName);
	}
	
	CleanupStack::PopAndDestroy(2, cert2); // sCert;
	
	// now, create a session with the parameters set in the preamble
	err = CreateSessionL();
	
	// ensure we succeeded
	if (err != KErrNone)
		{
		ERR_PRINTF2(_L("Failed! CreateSessionL returned %d"), err);
		SetTestStepResult(EFail);
		SetTestStepError(err);
		return TestStepResult();
		}

	// retrieve a client certificates chain
	// put it in both single HBufC and RPointerArray
	HBufC8* certBuf = NULL;
	err = ClientCertificate(certBuf);
	CleanupStack::PushL(certBuf);
	RPointerArray<HBufC8> certArray;
	err = ClientCertificate(&certArray);
	CleanupStack::PushL(TCleanupItem(&FreeCertArray, &certArray));

	// get expected number of certs
	TInt expectedCertCount(0);
	GetIntFromConfig(ConfigSection(), KExpectedCertCount, expectedCertCount);

	// check the result
	TInt expectedResult;
	if ( !GetIntFromConfig(ConfigSection(), KExpectedResult, expectedResult) )
		{
		// failed to get expected result from config file... using KErrNone.
		expectedResult = KErrNone;
		}

	if (err != expectedResult)
		{
		ERR_PRINTF3(_L("Failed! TLS Provider returned error code %d, expecting %d."),
			err, expectedResult);
		SetTestStepResult(EFail);
		CleanupStack::PopAndDestroy(2, certBuf); //TCleanupItem, certBuf
		return TestStepResult();
		}

	INFO_PRINTF2(_L("ClientCertificate returned %d as expected"), err);
	// if we got expected result
	// it may be an error or KErrNone with no client certificate was found
	// certBuf has to be NULL in both cases
	if ( !certBuf )
		{
		CleanupStack::PopAndDestroy(2, certBuf);//TCleanupItem, certBuf
		if ( KErrNone != err )
			{
			INFO_PRINTF1(_L("Test passed."));
			return TestStepResult();
			}
		else
			{
			ERR_PRINTF1(_L("Test failed. No Client Certificate found."));
			SetTestStepResult(EFail);
			return TestStepResult();
			}
		}

	// at this point no error should occur and certBuf is not NULL
	if ( KErrNone != err || !certBuf )
		{
		ERR_PRINTF2(_L("Failed! ClientCertificate returned %d"), err);
		SetTestStepResult(EFail);
		SetTestStepError(err);
		CleanupStack::PopAndDestroy(2, certBuf); //TCleanupItem, certBuf
		return TestStepResult();
		}

	// List certificates
	TInt certBufLen = certBuf->Length();
	TInt pos = 0;
	TInt intCount = 0;
	INFO_PRINTF1(_L("Listing returned chain:"));
	while (pos < certBufLen)
		{
		CX509Certificate *certChainCert = CX509Certificate::NewL(*certBuf,pos);
		CleanupStack::PushL(certChainCert);
		HBufC *pSubject = certChainCert->SubjectL();
		CleanupStack::PushL(pSubject);
		HBufC *pIssuer = certChainCert->IssuerL();
		CleanupStack::PushL(pIssuer);
		
		INFO_PRINTF2(_L("Returned Cert [%d]:"), intCount) ;
		INFO_PRINTF3(_L("Subject: %S *** Issuer: %S"), pSubject, pIssuer);
		
		CX509Certificate *certArrayCert = CX509Certificate::NewLC(*certArray[intCount]);
		if(certArrayCert->IsEqualL(*certChainCert))
			{
			INFO_PRINTF1(_L("certChain is equal to certArray"));
			}
		else
			{
			ERR_PRINTF2(_L("certChain is not equal to certArray at index %d"), intCount);
			SetTestStepResult(EFail);
			CleanupStack::PopAndDestroy(4);
			CleanupStack::PopAndDestroy(2, certBuf);//TCleanupItem, certBuf
			return TestStepResult();
			}
		CleanupStack::PopAndDestroy(4);
		
		intCount++;
		}

	CleanupStack::PopAndDestroy(2, certBuf);//TCleanupItem, certBuf
	
	// make sure expected number of certificates matches actual one
	if (intCount != expectedCertCount)
		{
		ERR_PRINTF3(_L("Failed! expected %d certs, got %d"), expectedCertCount, intCount);
		SetTestStepResult(EFail);
		return TestStepResult();
		}

	INFO_PRINTF1(_L("Test passed."));
	SetTestStepResult(EPass);

	return TestStepResult();
	}
