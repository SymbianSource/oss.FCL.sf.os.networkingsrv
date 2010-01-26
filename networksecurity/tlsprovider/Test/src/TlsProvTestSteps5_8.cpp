
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

#include "TlsProvTestStep.h"
#include <test/testexecutelog.h>


	
TVerdict CTlsProvTestActive::doTest5_2L( CTlsProvStep* aStep )
	{
	CTlsTestRandom rnd;
	
	INFO_PRINTF1(_L("5.2"));
	iLogInfo.Copy( _L("	5.2 start") );
	SetThreadRandomL( &rnd );
	
	RArray<TTLSCipherSuite> userCipherSuiteList;
	CTLSProvider* tlsProvider = CTLSProvider::ConnectL();
	
	iStatus = KRequestPending;
	tlsProvider->CipherSuitesL( userCipherSuiteList ,iStatus);	
	SetActive();
	CActiveScheduler::Start();
	INFO_PRINTF1(_L("2"));
	
			
	CTLSSession* sessionObj;
	
	CTlsCryptoAttributes* tlsCryptoAttributes = tlsProvider->Attributes();
	
	HBufC8* encServerCert = NULL;
	
	TInt res = StandardTestInitL(aStep, tlsCryptoAttributes, encServerCert);
	if( res  < 0 )
		{
		iLogInfo.Copy( _L("	5.2 (&8.0):  problems with test data files") );
		return EFail;
		}
			
	tlsCryptoAttributes->isignatureAlgorithm = ERsaSigAlg;
	tlsCryptoAttributes->iDialogNonAttendedMode = ETrue;
	
	AddDNL();
	const TDesC8& issuerName = reinterpret_cast<const TDesC8&>(*iDNs[0]);
	tlsCryptoAttributes->iDistinguishedCANames.Append(&issuerName);
	iDNs.Close();
				
	CX509Certificate* serverCert = NULL;
	iStatus = KRequestPending;
	tlsProvider->VerifyServerCertificate( encServerCert->Des(), serverCert, iStatus);
	SetActive();
	CActiveScheduler::Start();
	INFO_PRINTF1(_L("3"));
			
	delete serverCert;
	
	tlsCryptoAttributes->iClientAuthenticate = ETrue; 
	
	tlsCryptoAttributes->iReqCertTypes.Append(ERsaSign);
	
	iStatus = KRequestPending;
	tlsProvider->CreateL( sessionObj,
						iStatus);	
	SetActive();
	CActiveScheduler::Start();
	INFO_PRINTF1(_L("4"));
	
		
	
	TBuf8<30> inputToSign;
	inputToSign.Append( _L8("Nice input to Hash and Sign") );	
	
		
	HBufC8* certVerifyOutput = NULL;
			
	CMessageDigest* md5Dig = NULL;
	CMessageDigest* shaDig = NULL;
	
	md5Dig = CMD5::NewL(); 
	shaDig = CSHA1::NewL(); 
						
	md5Dig->Reset();
	md5Dig->Update( inputToSign );
			
	shaDig->Reset();
	shaDig->Update( inputToSign );
	
	
	iStatus = KRequestPending;
	sessionObj->CertificateVerifySignatureL(
						md5Dig, shaDig, certVerifyOutput, iStatus); 
	SetActive();
	CActiveScheduler::Start();
	INFO_PRINTF1(_L("5"));
		
	if ( iStatus.Int() != KErrNone )
		{
		iLogInfo.Format( _L("	5.2 & 8.0):  CTLSSession::CertificateVerifySignatureLL failed %d"), iStatus.Int() );
		return EFail;
		}
	
	CX509Certificate* cert = NULL;

	HBufC8* certBuf = NULL;
	iStatus = KRequestPending;
	sessionObj->ClientCertificate( certBuf, iStatus);
		
	SetActive();
	CActiveScheduler::Start();	
	INFO_PRINTF1(_L("6"));
	
	if (!certBuf)
		{
		iLogInfo.Copy( _L("	5.2 & 8.0):  CTLSSession::No ClientCertificate") );
		return EFail;
		}

	cert = CX509Certificate::NewL( *certBuf );

	if ( (iStatus.Int() != KErrNone ) || (NULL == cert) )
		{
		iLogInfo.Copy( _L("	5.2 & 8.0):  CTLSSession::ClientCertificate failed") );
		return EInconclusive;
		}
		
			
	// check cert:
	if ( ERSA != cert->PublicKey().AlgorithmId() )
		{
		iLogInfo.Copy( _L("	5.2 & 8.0):  BAD certificate retrieved") );
		return EFail;
		}
	
		
	//get public key from cert
	CSubjectPublicKeyInfo* keyInfo = NULL;
	keyInfo = CSubjectPublicKeyInfo::NewLC( cert->PublicKey() );
	CleanupStack::Pop(1);
	if ( NULL == keyInfo )
		{
		iLogInfo.Copy( _L("	5.2 & 8.0):  cannot create CSubjectPublicKeyInfo object") );
		return EInconclusive;
		}
		
	// verify signature
	
	TBuf8<MD5_HASH + SHA1_HASH> input; 
	
	md5Dig->Reset();
	md5Dig->Update( inputToSign );
	input.Copy( md5Dig->Final() );
	
	shaDig->Reset();
	shaDig->Update( inputToSign );
	
	input.Append( shaDig->Final() );
	
	TBool result = tlsProvider->VerifySignatureL( *keyInfo, input, 
									certVerifyOutput->Des() ); 
	INFO_PRINTF1(_L("7"));
										
	if ( result == EFalse )
		{
		iLogInfo.Copy( _L("	5.2 & 8.0:  CTLSProvider::VerifySignatureL failed") );
		return EFail;
		}
	
		
	iLogInfo.Copy( _L("	5.2 & 8.0:  OK") );
	
	delete md5Dig;
	delete shaDig;
	delete tlsProvider;
	delete sessionObj;	
	
	return EPass;
	
	} 	
	

TVerdict CTlsProvTestActive::doTest5_3L( CTlsProvStep* aStep )
	{
	CTlsTestRandom rnd;
	
	SetThreadRandomL( &rnd );
	
	RArray<TTLSCipherSuite> userCipherSuiteList;
	CTLSProvider* tlsProvider = CTLSProvider::ConnectL();
	
	iStatus = KRequestPending;
	tlsProvider->CipherSuitesL( userCipherSuiteList ,iStatus);	
	SetActive();
	CActiveScheduler::Start();
	
			
	CTLSSession* sessionObj;
	
	CTlsCryptoAttributes* tlsCryptoAttributes = tlsProvider->Attributes();
	
	HBufC8* encServerCert = NULL;
	
	TInt res = StandardTestInitL(aStep, tlsCryptoAttributes, encServerCert);
	if( res  < 0 )
		{
		iLogInfo.Copy( _L("	5.3:  problems with test data files") );
		return EFail;
		}
			
		
	AddDNL();
	const TDesC8& issuerName = reinterpret_cast<const TDesC8&>(*iDNs[0]);
	tlsCryptoAttributes->iDistinguishedCANames.Append(&issuerName); 
	iDNs.Close();

	tlsCryptoAttributes->isignatureAlgorithm = ERsaSigAlg;
	tlsCryptoAttributes->iDialogNonAttendedMode = ETrue;
		
	CX509Certificate* serverCert = NULL;
	iStatus = KRequestPending;
	tlsProvider->VerifyServerCertificate( encServerCert->Des(), serverCert, iStatus);
	SetActive();
	CActiveScheduler::Start();
	
		
	delete serverCert;
	
	tlsCryptoAttributes->iClientAuthenticate = ETrue; 
	tlsCryptoAttributes->iDialogNonAttendedMode = ETrue; 
	
	tlsCryptoAttributes->iReqCertTypes.Append(ERsaSign);
	
	
	iStatus = KRequestPending;
	tlsProvider->CreateL( sessionObj,
						iStatus);	
	SetActive();
	CActiveScheduler::Start();
			
	
	TBuf8<30> inputToSign;
	inputToSign.Append( _L8("Nice input to Hash and Sign") );	
	
		
	HBufC8* certVerifyOutput = NULL;
			
	CMessageDigest* md5Dig = NULL;
	CMessageDigest* shaDig = NULL;
	
	md5Dig = CMD5::NewL(); 
	shaDig = CSHA1::NewL(); 
						
	md5Dig->Reset();
	md5Dig->Update( inputToSign );
			
	shaDig->Reset();
	shaDig->Update( inputToSign );
	
	iStatus = KRequestPending;
	sessionObj->CertificateVerifySignatureL(
						md5Dig, shaDig, certVerifyOutput, iStatus); 
	SetActive();
	CActiveScheduler::Start();
		
	if ( iStatus.Int() != KErrNone )
		{
		iLogInfo.Format( _L("	5.3:  CTLSSession::CertificateVerifySignatureLL failed %d"), iStatus.Int() );
		return EFail;
		}
		
	CX509Certificate* cert = NULL;
	iStatus = KRequestPending;
	sessionObj->ClientCertificate( cert, iStatus);
		
	SetActive();
	CActiveScheduler::Start();	
	
	if ( (iStatus.Int() != KErrNone ) || (NULL == cert) )
		{
		iLogInfo.Copy( _L("	5.3:  CTLSSession::ClientCertificate failed") );
		return EInconclusive;
		}
		
		
	// check cert:
	if ( ERSA != cert->PublicKey().AlgorithmId() )
		{
		iLogInfo.Copy( _L("	5.3:  BAD certificate retrieved") );
		return EFail;
		}
	
		
	//get public key from cert
	CSubjectPublicKeyInfo* keyInfo = NULL;
	
	keyInfo = CSubjectPublicKeyInfo::NewLC( cert->PublicKey()  );
	CleanupStack::Pop(1);
	if ( NULL ==  keyInfo )
		{
		iLogInfo.Copy( _L("	5.3:  cannot create CSubjectPublicKeyInfo object") );
		return EInconclusive;
		}
		
	// try to verify signature with bad digest
	
	TBuf8<MD5_HASH + SHA1_HASH> badInput; 
	
	badInput.Append(       _L8("Bad input of 36 characters lalalala.") );
			
	TBool result = tlsProvider->VerifySignatureL( *keyInfo, badInput, 
									certVerifyOutput->Des() ); 
										
	if ( result != EFalse )
		{
		iLogInfo.Copy( _L("	5.3:  CTLSProvider::VerifySignatureL verified POSITIVELY with BAD input") );
		return EFail;
		}
	
		
	iLogInfo.Copy( _L("	5.3:  OK") );
	
	delete md5Dig;
	delete shaDig;
	delete sessionObj;
	delete tlsProvider;
	
	return EPass;
	
	} 	
	
	
TVerdict CTlsProvTestActive::doTest5_4L( CTlsProvStep* aStep )
	{
	
	
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("ClientKeyExch"),aStep->iClientKeyExch);
		
	RArray<TTLSCipherSuite> userCipherSuiteList;
	CTLSProvider* tlsProvider = CTLSProvider::ConnectL();
	
	iStatus = KRequestPending;
	tlsProvider->CipherSuitesL( userCipherSuiteList ,iStatus);	
	SetActive();
	CActiveScheduler::Start();
	
			
	CTLSSession* sessionObj;
	
	CTlsCryptoAttributes* tlsCryptoAttributes = tlsProvider->Attributes();
		
	HBufC8* encServerCert = NULL;
	
	TInt res = StandardTestInitL(aStep, tlsCryptoAttributes, encServerCert);
	if( res  < 0 )
		{
		iLogInfo.Copy( _L("	5.4 (&8.2):  problems with test data files") );
		return EFail;
		}
		
	tlsCryptoAttributes->iCurrentCipherSuite.iLoByte = 0x13;
	tlsCryptoAttributes->iPublicKeyParams->iKeyType = EDHE;
	tlsCryptoAttributes->isignatureAlgorithm = EDsa;
	tlsCryptoAttributes->iDialogNonAttendedMode = ETrue;
	
	
		
	CX509Certificate* serverCert = NULL;
	iStatus = KRequestPending;
	tlsProvider->VerifyServerCertificate( encServerCert->Des(), serverCert, iStatus);
	SetActive();
	CActiveScheduler::Start();
	
		
	delete serverCert;
	
	tlsCryptoAttributes->iClientAuthenticate = ETrue; 
	tlsCryptoAttributes->iDialogNonAttendedMode = ETrue;
	
	tlsCryptoAttributes->iReqCertTypes.Append(EDssSign);
	
	
	iStatus = KRequestPending;
	tlsProvider->CreateL( sessionObj,
						iStatus);	
	SetActive();
	CActiveScheduler::Start();
	
		
	
	TBuf8<30> inputToSign;
	inputToSign.Append( _L8("Nice input to Hash and Sign") );	
	
		
	HBufC8* certVerifyOutput = NULL;
			
	CMessageDigest* md5Dig = NULL;
	CMessageDigest* shaDig = NULL;
	
	md5Dig = CMD5::NewL(); 
	shaDig = CSHA1::NewL(); 
						
	md5Dig->Reset();
	md5Dig->Update( inputToSign );
			
	shaDig->Reset();
	shaDig->Update( inputToSign );
	
	
	iStatus = KRequestPending;
	sessionObj->CertificateVerifySignatureL(
						md5Dig, shaDig, certVerifyOutput, iStatus); 
	SetActive();
	CActiveScheduler::Start();
		
		
	CX509Certificate* cert = NULL;
	iStatus = KRequestPending;
	sessionObj->ClientCertificate( cert, iStatus);
	SetActive();
	CActiveScheduler::Start();	
	
	if ( (iStatus.Int() != KErrNone ) || (NULL == cert) )
		{
		iLogInfo.Copy( _L("	5.4 & 8.2):  CTLSSession::ClientCertificate failed") );
		return EInconclusive;
		}
		
		
	// check cert:
	if ( EDSA != cert->PublicKey().AlgorithmId() )
		{
		iLogInfo.Copy( _L("	5.4 & 8.2):  BAD certificate retrieved") );
		return EFail;
		}
	
		
	//get public key from cert
	CSubjectPublicKeyInfo* keyInfo = NULL;
	
	keyInfo = CSubjectPublicKeyInfo::NewLC( cert->PublicKey()  );
	if ( NULL == keyInfo )
		{
		iLogInfo.Copy( _L("	5.4 & 8.2):  cannot create CSubjectPublicKeyInfo object") );
		return EInconclusive;
		}
		
	// verify signature
	
	TBuf8<SHA1_HASH> input; 
	
	shaDig->Reset();
	shaDig->Update( inputToSign );
	
	input.Append( shaDig->Final() );
	
	TBool result = tlsProvider->VerifySignatureL( *keyInfo, input, 
									certVerifyOutput->Des() ); 
										
	if ( result == EFalse )
		{
		iLogInfo.Copy( _L("	5.4 & 8.2):  CTLSProvider::VerifySignatureL failed") );
		return EFail;
		}
	
		
	iLogInfo.Copy( _L("	5.4 & 8.2):  OK") );
	
	delete md5Dig;
	delete shaDig;
	delete sessionObj;
	delete tlsProvider;
	
	return EPass;

	
	} 	
	
	
TVerdict CTlsProvTestActive::doTest5_5L( CTlsProvStep* aStep )
	{
	
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("ClientKeyExch"),aStep->iClientKeyExch);
		
	RArray<TTLSCipherSuite> userCipherSuiteList;
	CTLSProvider* tlsProvider = CTLSProvider::ConnectL();
	
	iStatus = KRequestPending;
	tlsProvider->CipherSuitesL( userCipherSuiteList ,iStatus);	
	SetActive();
	CActiveScheduler::Start();
	
			
	CTLSSession* sessionObj;
	
	CTlsCryptoAttributes* tlsCryptoAttributes = tlsProvider->Attributes();
		
	HBufC8* encServerCert = NULL;
	
	TInt res = StandardTestInitL(aStep, tlsCryptoAttributes, encServerCert);
	if( res  < 0 )
		{
		iLogInfo.Copy( _L("	5.5:  problems with test data files") );
		return EFail;
		}
		
	tlsCryptoAttributes->iCurrentCipherSuite.iLoByte = 0x13; 
	tlsCryptoAttributes->iPublicKeyParams->iKeyType = EDHE;
	tlsCryptoAttributes->isignatureAlgorithm = EDsa;
	tlsCryptoAttributes->iDialogNonAttendedMode = ETrue;
	
	CX509Certificate* serverCert = NULL;
	iStatus = KRequestPending;
	tlsProvider->VerifyServerCertificate( encServerCert->Des(), serverCert, iStatus);
	SetActive();
	CActiveScheduler::Start();
	
	delete serverCert;
	
	tlsCryptoAttributes->iClientAuthenticate = ETrue; 
	tlsCryptoAttributes->iDialogNonAttendedMode = ETrue;
	
	tlsCryptoAttributes->iReqCertTypes.Append(EDssSign);
	
	iStatus = KRequestPending;
	tlsProvider->CreateL( sessionObj,
						iStatus);	
	SetActive();
	CActiveScheduler::Start();
		
	TBuf8<30> inputToSign;
	inputToSign.Append( _L8("Nice input to Hash and Sign") );	
	
		
	HBufC8* certVerifyOutput = NULL;
			
	CMessageDigest* md5Dig = NULL;
	CMessageDigest* shaDig = NULL;
	
	md5Dig = CMD5::NewL(); 
	shaDig = CSHA1::NewL(); 
						
	md5Dig->Reset();
	md5Dig->Update( inputToSign );
			
	shaDig->Reset();
	shaDig->Update( inputToSign );
	
	
	iStatus = KRequestPending;
	sessionObj->CertificateVerifySignatureL(
						md5Dig, shaDig, certVerifyOutput, iStatus); 
	SetActive();
	CActiveScheduler::Start();
		
		
	CX509Certificate* cert = NULL;
	iStatus = KRequestPending;
	sessionObj->ClientCertificate( cert, iStatus );
	SetActive();
	CActiveScheduler::Start();	
	
	if ( (iStatus.Int() != KErrNone ) || (NULL == cert) )
		{
		iLogInfo.Copy( _L("	5.5:  CTLSSession::ClientCertificate failed") );
		return EInconclusive;
		}
		
	// check cert:
	if ( EDSA != cert->PublicKey().AlgorithmId() )
		{
		iLogInfo.Copy( _L("	5.5:  BAD certificate retrieved") );
		return EFail;
		}
	
		
	//get public key from cert
	CSubjectPublicKeyInfo* keyInfo = NULL;
	keyInfo = CSubjectPublicKeyInfo::NewLC( cert->PublicKey()  );
	CleanupStack::Pop(1);
	if ( NULL == keyInfo )
		{
		iLogInfo.Copy( _L("	5.5:  cannot create CSubjectPublicKeyInfo object") );
		return EInconclusive;
		}
		
	// try to verify signature with bad digest
	
	TBuf8<SHA1_HASH> badInput; 
	
	badInput.Append(       _L8("Bad input of 20chars") );
			
	TBool result = tlsProvider->VerifySignatureL( *keyInfo, badInput, 
									certVerifyOutput->Des() ); 
										
	if ( result != EFalse )
		{
		iLogInfo.Copy( _L("	5.5:  CTLSProvider::VerifySignatureL verified POSITIVELY with BAD input") );
		return EFail;
		}
	
		
	iLogInfo.Copy( _L("	5.5:  OK") );
	
	delete md5Dig;
	delete shaDig;
	delete sessionObj;
	delete tlsProvider;
	
	return EPass;
	
	} 	


TVerdict CTlsProvTestActive::doTest8_1L( CTlsProvStep* aStep ) 
	{
	
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("ClientKeyExch"),aStep->iClientKeyExch);
	
	
	RArray<TTLSCipherSuite> userCipherSuiteList;
	CTLSProvider* tlsProvider = CTLSProvider::ConnectL();
	
	iStatus = KRequestPending;
	tlsProvider->CipherSuitesL( userCipherSuiteList ,iStatus);	
	SetActive();
	CActiveScheduler::Start();
	
			
	CTLSSession* sessionObj;
	
	CTlsCryptoAttributes* tlsCryptoAttributes = tlsProvider->Attributes();
		
	HBufC8* encServerCert = NULL;
	
	TInt res = StandardTestInitL(aStep, tlsCryptoAttributes, encServerCert);
	if( res  < 0 )
		{
		iLogInfo.Copy( _L("	8.1:  problems with test data files") );
		return EFail;
		}

	tlsCryptoAttributes->isignatureAlgorithm = ERsaSigAlg;
	tlsCryptoAttributes->iDialogNonAttendedMode = ETrue;

	AddDNL();
	const TDesC8& issuerName = reinterpret_cast<const TDesC8&>(*iDNs[0]);
	tlsCryptoAttributes->iDistinguishedCANames.Append(&issuerName);
	iDNs.Close();
		
	CX509Certificate* serverCert = NULL;
	iStatus = KRequestPending;
	tlsProvider->VerifyServerCertificate( encServerCert->Des(), serverCert, iStatus);
	SetActive();
	CActiveScheduler::Start();
	
	delete serverCert;
	
	tlsCryptoAttributes->iClientAuthenticate = ETrue; 
	tlsCryptoAttributes->iDialogNonAttendedMode = ETrue;
	
	tlsCryptoAttributes->iReqCertTypes.Append(EDssSign);
	tlsCryptoAttributes->iReqCertTypes.Append(ERsaFixedDh);
	tlsCryptoAttributes->iReqCertTypes.Append(EDssFixedDh);
	
		
	iStatus = KRequestPending;
	tlsProvider->CreateL( sessionObj,
						iStatus);	
	SetActive();
	CActiveScheduler::Start();
	
	CX509Certificate* cert = NULL;
	iStatus = KRequestPending;
	sessionObj->ClientCertificate( cert, iStatus );
	SetActive();
	CActiveScheduler::Start();	
	// We need to set up the distinguish name!!
	if ( NULL == cert )
		{
		iLogInfo.Copy( _L("	8.1:  CTLSSession::ClientCertificate failed ") );
		return EFail;
		}
	
	
	iLogInfo.Copy( _L("	8.1:  OK") );		
	return EPass;
	
	}

void CTlsProvTestActive::AddDNL()
	{
	if (iIssuerName.Length())
		{
		RFs fs;
		User::LeaveIfError(fs.Connect());
		CleanupClosePushL(fs);
		
		RFile file;
		TInt ret = file.Open(fs, iIssuerName, EFileRead);
		CleanupClosePushL(file);
		
		TInt size;
		file.Size(size);
		
		HBufC8* cert = HBufC8::NewMaxLC(size);
		TPtr8 ptr(cert->Des());
		file.Read(ptr);
		
		CX509Certificate* parsedCert = CX509Certificate::NewLC(*cert);
		HBufC8* issuer = 
			parsedCert->DataElementEncoding(CX509Certificate::EIssuerName)->
			AllocLC();
		User::LeaveIfError(iDNs.Append(issuer));
		
		CleanupStack::Pop(issuer);
		CleanupStack::PopAndDestroy(4, &fs);
		}
	} 	
