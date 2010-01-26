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


#include "hwtlstokentypeplugin.h"

#include <signed.h>
#include <asymmetric.h>
#include <x509cert.h>
#include <x509keys.h>
#include <asn1enc.h>
#include <f32file.h>

#ifdef __MWERKS__
#ifdef _BullseyeCoverage
// mwccsym2.exe 3.2.5 build 465 crashes when trying to inline bullsEye 7.5.69 instrumentation 
// for this file, so disable inlining for instrumented code.
#pragma dont_inline on
#endif
#endif

const TInt KKeyingMaterialLen1 = 128;
const TInt KKeyingMaterialLen2 = 64;

MCTToken& CHwTLSSession::Token()
	{
	return iToken;
	}
	
const TDesC& CHwTLSSession::Label()
	{
	return iLabel;
	}
	

void CHwTLSSession::InitL(
			const TTLSSessionNameAndID& aSessionNameAndID, 
			const TTLSCipherSuite& aCipherSuite, 
			const TTLSCompressionMethod& aCompressionMethod,
			const TTLSProtocolVersion& aVersion,
			TBool aResume) 
	{
	
	HWTLSTOKEN_LOG(  _L("CHwTLSSession::InitL") )
	
	// Create an empty output file if we're called as a token for a cipher
	// This allows testexecute to check that the software/hardware selection
	// algorithm is working, since it checks if this file exists after trying
	// to encrypt data with different ciphers.
	RFs fs;
	fs.Connect();
	RFile tempfile;
	
	TDriveUnit sysDrive (fs.GetSystemDrive());
	TBuf<128> hwTLSTestLog (sysDrive.Name());
	hwTLSTestLog.Append(_L("\\tlstest2\\hwtest.log"));

	TInt err = tempfile.Replace(fs, hwTLSTestLog, EFileWrite|EFileShareAny);
	if (err != KErrNone)
		{
		HWTLSTOKEN_LOG2(  _L("cannot create test log file, %d"), err )
		}

	tempfile.Close();
	fs.Close();

	if( iInitialised )
		{
		HWTLSTOKEN_LOG(  _L("	interface was initialized before - reseting") ) 
		Reset();
		}
	
	iCertSelected = EFalse;
	iResumed = EFalse;
 
	
	TInt i;
		
	if( (KTLS1_0 != aVersion) && (KSSL3_0 != aVersion) )
		{
		HWTLSTOKEN_LOG(  _L("	protocol version not supported") )
		User::Leave( KTLSErrBadProtocolVersion );
		}

	iCompressionMethod = new(ELeave) TTLSCompressionMethod( aCompressionMethod );
	iSessionNameAndID = new(ELeave) TTLSSessionNameAndID( aSessionNameAndID );
	iProtocolVersion = new(ELeave) TTLSProtocolVersion( aVersion );
	
	

	TBool found = EFalse;
	HWTLSTOKEN_LOG(  _L("	about to check cipher suite") )
	TTLSCipherSuiteMapping selectedCipher = KSetOfTLSCipherSuites[0];
	// now check step by step starting from the end
	for( i=TLS_CIPHER_SUITES_NUMBER-1; i>0; i--)
		{
		const TTLSCipherSuite suite2(KSetOfTLSCipherSuites[i].iCipherSuite);
		if(  suite2.iLoByte == aCipherSuite.iLoByte &&
			  suite2.iHiByte == aCipherSuite.iHiByte)
			{
			selectedCipher =  KSetOfTLSCipherSuites[i];
			found = ETrue;
			break;
			}
		}
	
	if( found == EFalse )
		{
		HWTLSTOKEN_LOG(  _L("	cipher suite not supported") )
		User::Leave( KTLSErrBadCipherSuite );
		}
	
	iCipherSuite = new(ELeave) TTLSCipherSuiteMapping( selectedCipher );
	
	iMasterSecret = new(ELeave) TBuf8<KTLSMasterSecretLen>;
	iMasterSecret->SetLength( iMasterSecret->MaxLength() );
	iMasterSecret->FillZ();
		
	if( aResume ) 
		{
		HWTLSTOKEN_LOG(  _L("	resuming session from cache") )
		iResumed = ETrue;
		// retrieve data from a cache
		CHwTLSSessionCache* cacheEntry = iToken.SessionCache( iSessionNameAndID->iServerName,
															iSessionNameAndID->iSessionId );
		if ( NULL == cacheEntry )
			{
			HWTLSTOKEN_LOG(  _L("	session with given id not found in cache") )
			User::Leave( KTLSErrNotCached );
			}

		const TTLSSessionData sessionData = cacheEntry->ReadData();

		if( cacheEntry->IsResumable() == EFalse )
			User::Leave( KTLSErrCacheEntryInUse );

		const TTLSServerAddr servAddr = cacheEntry->ServerAddr();

		if ( !(sessionData.iCipherSuite == aCipherSuite ) ||
			(sessionData.iProtocolVersion != aVersion ) ||
			(sessionData.iCompressionMethod != aCompressionMethod ) ||
			(servAddr.iAddress != iSessionNameAndID->iServerName.iAddress) )
			{
			HWTLSTOKEN_LOG(  _L("	session with satisfying conditions not found in cache") )
			User::Leave( KErrNotFound );
			}
		
		iMasterSecret->Copy( cacheEntry->MasterSecret() );

		}
	
	if( EFalse == IsAdded() )
		CActiveScheduler::Add( this );
		

	iInitialised = ETrue;
	HWTLSTOKEN_LOG(  _L("	interface initialised") ) 
	HWTLSTOKEN_LOG2(  _L("	protocol version: 3.%d"), iProtocolVersion->iMinor )
	HWTLSTOKEN_LOG2(  _L("	cipher suite: %d"), iCipherSuite->iCipherSuite.iLoByte )
	HWTLSTOKEN_LOG(  _L("	iMasterSecret set initially to:") )
	HWTLSTOKEN_LOG_HEX( (const TUint8*)(iMasterSecret->Ptr()), iMasterSecret->Size() )
			

	return;

	}



void CHwTLSSession::ServerCertificate(
						HBufC8*& aEncodedServerCert,
						TRequestStatus& aStatus )
	{
	HWTLSTOKEN_LOG(  _L("CHwTLSSession::ServerCertificate") )
	
	aStatus = KRequestPending;
	iOriginalRequestStatus = &aStatus;
	
	CHwTLSSessionCache* entry = iToken.SessionCache( iSessionNameAndID->iServerName,
														iSessionNameAndID->iSessionId );
	if( NULL != entry )
		aEncodedServerCert = entry->ServerCertificate();
	else 
		aEncodedServerCert = NULL;
		
	User::RequestComplete(iOriginalRequestStatus, KErrNone); 
	return;
	}

void CHwTLSSession::ClientCertificate(
						CCTCertInfo*& aClientCertInfo,
						TRequestStatus& aStatus )
	{
	HWTLSTOKEN_LOG(  _L("CHwTLSSession::ClientCertificate") )
	
	aStatus = KRequestPending;
	iOriginalRequestStatus = &aStatus;
	
	CHwTLSSessionCache* entry = iToken.SessionCache( iSessionNameAndID->iServerName,
													iSessionNameAndID->iSessionId );
													
	if( NULL != entry )													
		aClientCertInfo = entry->ClientCertificate();
	else
		aClientCertInfo = NULL;
		
	User::RequestComplete(iOriginalRequestStatus, KErrNone); 
	return;
	}


void CHwTLSSession::ClientKeyExchange(
						const TTLSMasterSecretInput& aMasterSecretInput,
						const TTLSProtocolVersion& aClientHelloVersion,
						const TDes8& aEncodedServerCert,
						const CTLSPublicKeyParams* aKeyParams, 
						HBufC8*& aClientKeyExch, 
						TRequestStatus& aStatus )
	{
	HWTLSTOKEN_LOG(  _L("CHwTLSSession::ClientKeyExchange") )
	
	aStatus = KRequestPending;
	iOriginalRequestStatus = &aStatus;
	TInt err = KErrNone;
	
	TRAP( err, ClientKeyExchangeL( aMasterSecretInput, 
						aClientHelloVersion,
						aEncodedServerCert, 
						aKeyParams, 
						aClientKeyExch) )
	
	if( KErrNone == err)
		{					
		HWTLSTOKEN_LOG(  _L("	output from ClientKeyExchange: aClientKeyExch:") )
		HWTLSTOKEN_LOG_HEX( (const TUint8*)(aClientKeyExch->Des().Ptr()), 
						aClientKeyExch->Size() )
		}
	else
      {
		HWTLSTOKEN_LOG2(  _L("	ClientKeyExchange returned with error: %d"), err )
      }
	
	User::RequestComplete( iOriginalRequestStatus, err );
	return;
	
	}


void CHwTLSSession::RSAClientKeyExchL( const CTLSPublicKeyParams* aKeyParams, 
							const TDes8& aEncodedServerCert,
							const TTLSProtocolVersion& aClientHelloVersion,
							HBufC8*& aPremaster,
							HBufC8*& aClientKeyExch )
	{
	HBufC8 *premaster = HBufC8::NewLC( KTLSPreMasterLenInRsa );
		
	__UHEAP_MARK; 
		
	TBuf8<KTLSPreMasterLenInRsa> buf;
	buf.SetLength(KTLSPreMasterLenInRsa);
		
	GenerateRandomBytesL( buf );
	premaster->Des().Append(buf);
				
	TBuf8<2> protver; 
		
	// protocol version sent by the client in the ClientHello msg
	protver.Append( aClientHelloVersion.iMajor ); 
	protver.Append( aClientHelloVersion.iMinor ); 
			
	// add protocol version 
	premaster->Des().Replace( 0, 2, protver );
		
	HWTLSTOKEN_LOG2(  _L("	premaster, RSA case (size %d): "), premaster->Size() )
	HWTLSTOKEN_LOG_HEX( (const TUint8*)(premaster->Des().Ptr()), 
						premaster->Size() )
	__UHEAP_MARKEND;
						
	
	// generate client key exchange - that is (RSA case), encrypt 
	// pre-master with server public key

	
	HWTLSTOKEN_LOG(  _L("	generating RSA key exchange") )
		
	CRSAPKCS1v15Encryptor* rsaEncryptor = NULL;

	if( (NULL != aKeyParams) && (NULL != aKeyParams->iValue1) && 
				(NULL != aKeyParams->iValue2) ) 
		{
		if (aKeyParams->iKeyType != ERsa )
			User::Leave( KTLSErrBadSignAlg );
			
						
		HWTLSTOKEN_LOG(  _L("	keyParams not NULL:") )
		HWTLSTOKEN_LOG(  _L("	modulus:") )
		HWTLSTOKEN_LOG_HEX( (const TUint8*)(aKeyParams->iValue1->Des().Ptr()), 
					aKeyParams->iValue1->Size() )
		HWTLSTOKEN_LOG(  _L("	exponent:") )
		HWTLSTOKEN_LOG_HEX( (const TUint8*)(aKeyParams->iValue2->Des().Ptr()), 
					aKeyParams->iValue2->Size() )
				
		RInteger valN;
		valN = RInteger::NewL( aKeyParams->iValue1->Des() ); 
		CleanupStack::PushL( valN );
				
		RInteger valE;
		valE = RInteger::NewL( aKeyParams->iValue2->Des() );
		CleanupStack::PushL( valE ); 
								
		CRSAPublicKey* serverRsaPubKey = NULL; 
		serverRsaPubKey =  CRSAPublicKey::NewL( valN, valE );
			
		CleanupStack::Pop( 2, &valN ); // serverRsaKey is on cleanup stack
		CleanupStack::PushL( serverRsaPubKey ); 
						
		rsaEncryptor = CRSAPKCS1v15Encryptor::NewLC( *serverRsaPubKey);
						
		HWTLSTOKEN_LOG(  _L("	CRSAPKCS1v15Encryptor::NewLC( *serverRsaPubKey) called") )
			
		HBufC8* tmp = HBufC8::NewLC( rsaEncryptor->MaxOutputLength() );
		// debug
		HWTLSTOKEN_LOG2(  _L("	rsaEncryptor->MaxInputLength %d"), rsaEncryptor->MaxInputLength() )
		HWTLSTOKEN_LOG2(  _L("	rsaEncryptor->MaxOutputLength %d"), rsaEncryptor->MaxOutputLength() )
		HWTLSTOKEN_LOG2(  _L("	size of input to rsaEncryptor->EncryptL %d"), premaster->Size() )
			
		TPtr8 tmpTPtr = tmp->Des();
		TPtr8* tmpPtr = &tmpTPtr;
		rsaEncryptor->EncryptL( premaster->Des(), *tmpPtr );
						
				
		HWTLSTOKEN_LOG(  _L("	result of encryption:") )
		HWTLSTOKEN_LOG2(  _L("	%d bytes"), tmp->Size() )
		HWTLSTOKEN_LOG_HEX( (const TUint8*)(tmp->Des().Ptr()), tmp->Size() ) 
		
		aClientKeyExch = tmp->AllocL();
			
		CleanupStack::PopAndDestroy( 3, serverRsaPubKey );
										
		}
	else // RSA key must be retrieved from server cert
		{
		HWTLSTOKEN_LOG(  _L("	using server cert for encryption:") )
		
		CX509Certificate* serverCert = CX509Certificate::NewLC( aEncodedServerCert );
		
		CSubjectPublicKeyInfo* serverKeyInfo = NULL;
		serverKeyInfo = CSubjectPublicKeyInfo::NewLC(
						serverCert->PublicKey() );  
			
		CX509RSAPublicKey* serverRsaKey = NULL;
		serverRsaKey = CX509RSAPublicKey::NewLC( serverKeyInfo->KeyData() ); 
		HWTLSTOKEN_LOG(  _L("	serverRsaKey  created (CX509RSAPublicKey::NewLC())") )
		
		rsaEncryptor = CRSAPKCS1v15Encryptor::NewLC( *serverRsaKey ); 
		HWTLSTOKEN_LOG(  _L("	rsaEncryptor  created (CRSAPKCS1v15Encryptor::NewLC())") )
		
		if (premaster->Length() > rsaEncryptor->MaxInputLength())
			{
			User::Leave(KTLSErrBadArgument);
			}
						
		HBufC8* tmp1 = HBufC8::NewLC( rsaEncryptor->MaxOutputLength() );
		
		TPtr8 tmpTPtr1 = tmp1->Des();
		TPtr8* tmpPtr1 = &tmpTPtr1;
		rsaEncryptor->EncryptL( premaster->Des(), *tmpPtr1 );
					
		HWTLSTOKEN_LOG(  _L("	result of encryption:") )
		HWTLSTOKEN_LOG2(  _L("	(%d bytes)" ), tmp1->Size() )
		HWTLSTOKEN_LOG_HEX( (const TUint8*)(tmp1->Des().Ptr()), tmp1->Size() )
			
		aClientKeyExch = tmp1->AllocL();
			
		CleanupStack::PopAndDestroy( 5, serverCert );
						
		}
	
	CleanupStack::Pop( 1, premaster );
	aPremaster = premaster;

   return;
			
	}
								

void CHwTLSSession::DHEClientKeyExchL( const CTLSPublicKeyParams* aKeyParams, 
								HBufC8*& aPremaster,
								HBufC8*& aClientKeyExch )
	{
	
	CDHPublicKey* serverKey = NULL;
	
	if ( (NULL == aKeyParams) || (NULL == aKeyParams->iValue1) ||
			 (NULL == aKeyParams->iValue2) ||  (NULL == aKeyParams->iValue3)  )
		User::Leave( KErrArgument );
			
	if (aKeyParams->iKeyType != EDHE )
		User::Leave( KTLSErrBadSignAlg );
							
	HWTLSTOKEN_LOG(  _L("	DHE case, keyParams:") )
	HWTLSTOKEN_LOG(  _L("	modulus:") )
	HWTLSTOKEN_LOG_HEX( (const TUint8*)(aKeyParams->iValue1->Des().Ptr()), 
					aKeyParams->iValue1->Size() )
	HWTLSTOKEN_LOG(  _L("	generator:") )
	HWTLSTOKEN_LOG_HEX( (const TUint8*)(aKeyParams->iValue2->Des().Ptr()), 
					aKeyParams->iValue2->Size() )
	HWTLSTOKEN_LOG(  _L("	server public value:") )
	HWTLSTOKEN_LOG_HEX( (const TUint8*)(aKeyParams->iValue3->Des().Ptr()), 
						aKeyParams->iValue3->Size() )
			
	RInteger valN;
	valN = RInteger::NewL( aKeyParams->iValue1->Des() ); 
	CleanupStack::PushL( valN );
						
	RInteger valG;
	valG = RInteger::NewL( aKeyParams->iValue2->Des() ); 
	CleanupStack::PushL( valG );
						
	RInteger valX;
	valX = RInteger::NewL( aKeyParams->iValue3->Des() ); 
	CleanupStack::PushL( valX );
									
	serverKey = CDHPublicKey::NewL( valN, valG, valX ); 
			
	CleanupStack::Pop( 3, &valN ); 
	CleanupStack::PushL( serverKey );
									
		
	RInteger valN1;
	valN1 = RInteger::NewL( serverKey->N() ); 
	CleanupStack::PushL( valN1 );
		
	RInteger valG1;
	valG1 = RInteger::NewL( serverKey->G() ); 
	CleanupStack::PushL( valG1 ); 
	
	CDHKeyPair* dhKeyPair = NULL;
				
	dhKeyPair = CDHKeyPair::NewL( valN1, valG1 ); 
				
	CleanupStack::Pop( 2, &valN1 ); // dhKeyPair is on cleanup stack
	CleanupStack::PushL( dhKeyPair );
	
				
	// return client key exchange msg:
	
	CDH* keyAgreeObj = NULL;
	
	keyAgreeObj = CDH::NewLC( dhKeyPair->PrivateKey() );
		
	HBufC8 *premaster = const_cast<HBufC8*>(keyAgreeObj->AgreeL( *serverKey ));
	CleanupStack::PushL( premaster );
				
	HWTLSTOKEN_LOG2(  _L("	premaster, DHE case (size %d): "), premaster->Size() )
	HWTLSTOKEN_LOG_HEX( (const TUint8*)(premaster->Des().Ptr()), 
				premaster->Size() )
				 
	aClientKeyExch = dhKeyPair->PublicKey().X().BufferLC(); 
	CleanupStack::Pop(2, premaster); // don't want to have aClientKeyExch on cstack
	aPremaster = premaster;

	CleanupStack::PopAndDestroy( 3, serverKey );
	
	return;
	}
					
							
void CHwTLSSession::SSLMasterSecretComputationsL( 
										const TTLSMasterSecretInput& aMasterSecretInput,
										const TDesC8& aPremaster)
	{
	
	__UHEAP_MARK; 
		
	HWTLSTOKEN_LOG(  _L("	master secret generated using SSL algorithm") )
		
	CMessageDigest* shaDig = NULL;
	CMessageDigest* md5Dig = NULL;
		
	shaDig = CSHA1::NewL(); 
	CleanupStack::PushL( shaDig ); 
		
	md5Dig = CMD5::NewL(); 
	CleanupStack::PushL( md5Dig ); 
		
	iMasterSecret->Zero();
	
	TInt i;
	for( i=0; i<3; i++ ) // 3 iterations produce 48 bytes of master secret
		{
		shaDig->Reset();

		TPtrC8 saltChars(KSSLSaltSource);
		TBuf8<KSSLSaltStringsMaxNo> salt;
		salt.AppendFill( saltChars.Ptr()[i], i+1);
		salt.SetLength(i+1);
			
		shaDig->Update( salt );
						
		shaDig->Update( aPremaster );
		shaDig->Update( aMasterSecretInput.iClientRandom );
		shaDig->Update( aMasterSecretInput.iServerRandom );

		TPtrC8 tmpHash = shaDig->Final(); 

		TBuf8<SHA1_HASH> tmp; 

		tmp.Copy( tmpHash );

		md5Dig->Reset();
		md5Dig->Update( aPremaster );
		md5Dig->Update( tmp );

		iMasterSecret->Append( md5Dig->Final() );

		}
		
	CleanupStack::PopAndDestroy( 2, shaDig );
		
	__UHEAP_MARKEND;
		
	return;
	
	}


void CHwTLSSession::ClientKeyExchangeL(
						const TTLSMasterSecretInput& aMasterSecretInput,
						const TTLSProtocolVersion& aClientHelloVersion,
						const TDes8& aEncodedServerCert,
						const CTLSPublicKeyParams* aKeyParams, 
						HBufC8*& aClientKeyExch	)
	{
		
	HWTLSTOKEN_LOG(  _L("CHwTLSSession::ClientKeyExchangeL") )
	
	if( EDiffieHellman == iCipherSuite->iKeyExAlg ) 
		{
		HWTLSTOKEN_LOG(  _L("	DH fixed not supported") )
		User::Leave( KTLSErrBadKeyExchAlg );
		}
	
		
	HWTLSTOKEN_LOG(  _L("	server random:") )
	HWTLSTOKEN_LOG_HEX( (const TUint8*)(aMasterSecretInput.iServerRandom.Ptr()), 
						aMasterSecretInput.iServerRandom.Size() )
						
	HWTLSTOKEN_LOG(  _L("	client random:") )
	HWTLSTOKEN_LOG_HEX( (const TUint8*)(aMasterSecretInput.iClientRandom.Ptr()), 
						aMasterSecretInput.iClientRandom.Size() )
			
	if ( EFalse == iInitialised )
		{
		HWTLSTOKEN_LOG(  _L("	interface not initialised") )
		User::Leave( KTLSErrNotInitialized );
		}
	
		
	// 1. generate pre-master secret and client key exchange msg
	
	HBufC8* premaster = NULL;
	HBufC8* clientKeyExch = NULL;
	
	if( ERsa == iCipherSuite->iKeyExAlg) 
		RSAClientKeyExchL( aKeyParams, aEncodedServerCert, aClientHelloVersion,
							premaster,
							clientKeyExch);
	else if( EDHE == iCipherSuite->iKeyExAlg ) 
		DHEClientKeyExchL( aKeyParams, premaster, clientKeyExch );
	else // neither RSA nor DHE - we do not support such option
		User::Leave( KTLSErrBadKeyExchAlg );

	  CleanupStack::PushL( clientKeyExch );
	  CleanupStack::PushL( premaster );
			
	// 2. generate master secret

	if ( KTLS1_0 == *iProtocolVersion )
		{
		//from rfc2246:
		//	master_secret = PRF(pre_master_secret, "master secret",
		//                         ClientHello.random + ServerHello.random)
	    //   [0..47];
		__UHEAP_MARK;
		TBuf8<KTLSMasterSecretLen + 2*KTLSServerClientRandomLen> seed;
		seed.Append( KTLSMasterSecretLabel );
		seed.Append( aMasterSecretInput.iClientRandom );
		seed.Append( aMasterSecretInput.iServerRandom );
		
		TLSPRFL( premaster->Des(), seed, KTLSMasterSecretLen, *iMasterSecret );
		__UHEAP_MARKEND;
		}

	if ( KSSL3_0 == *iProtocolVersion )
		SSLMasterSecretComputationsL( aMasterSecretInput,
									premaster->Des() );
		
		
	HWTLSTOKEN_LOG(  _L("	iMasterSecret generated:") )
	HWTLSTOKEN_LOG_HEX( (const TUint8*)(iMasterSecret->Ptr()), iMasterSecret->Size() )

	
   CleanupStack::PopAndDestroy( 1, premaster ); //	delete premaster;
		
	// 3. store session in cache 

	HWTLSTOKEN_LOG(  _L("	about to store data in a cache") )
	TTLSSessionData sessionData;
	sessionData.iSessionId.Copy( iSessionNameAndID->iSessionId );
	sessionData.iCipherSuite = iCipherSuite->iCipherSuite;
	sessionData.iProtocolVersion = *iProtocolVersion;
	sessionData.iCompressionMethod		=	*iCompressionMethod;
	sessionData.iServerAuthenticated	=	EFalse; //	Server Authenticated - don't know yet
	sessionData.iClientAuthenticated	=	EFalse; //	Client Authenticated - don't know yet
		
		

	iToken.AddToCacheL( sessionData, 
		*iSessionNameAndID,  
		*iMasterSecret, 
		 aEncodedServerCert );

	// Pass ownership of client key exchange body to caller
    CleanupStack::Pop( clientKeyExch );
    aClientKeyExch = clientKeyExch;
			
	return;
	
	} 


		
void CHwTLSSession::ConnectionEstablished( TBool aSuccessful,
										TBool aServerAuthenticated,
										TBool aClientAuthenticated,
										TRequestStatus& aStatus )
	{
	
	HWTLSTOKEN_LOG(  _L("CHwTLSSession::ConnectionEstablished") )
	
	__UHEAP_MARK;
	
	aStatus = KRequestPending;
	iOriginalRequestStatus = &aStatus;
	
	if ( 0 == iSessionNameAndID->iSessionId.Length() )
		{
		HWTLSTOKEN_LOG(  _L("	session id empty - not cached") )
		User::RequestComplete(iOriginalRequestStatus, KErrArgument );		
		return;
		}
	
	// retrieve entry from a cache
	CHwTLSSessionCache* cacheEntry = iToken.SessionCache( iSessionNameAndID->iServerName,
															iSessionNameAndID->iSessionId );
	if ( NULL == cacheEntry )
		{
		User::RequestComplete(iOriginalRequestStatus, KTLSErrNotCached );
		return;
		}

	if( aSuccessful )
		{
		cacheEntry->SetResumable( ETrue, aServerAuthenticated, aClientAuthenticated );
		if ( (iResumed == EFalse) && (iCertSelected) )
			{
			cacheEntry->AddKeyInfo( iKeyObjectHandle, iClientCertInfo );
			iClientCertInfo = NULL; // don't delete - ownership passed to cache
			}
		}
	else
		{
		iToken.RemoveFromCache( iSessionNameAndID->iServerName,
								iSessionNameAndID->iSessionId );
		if( NULL != iClientCertInfo )
			iClientCertInfo->Release();
		iClientCertInfo = NULL;
		}

	User::RequestComplete(iOriginalRequestStatus, KErrNone );
	
	__UHEAP_MARKEND;		
	
	return;
	
	} 


void CHwTLSSession::ComputeDigitalSignature(
								const TDesC8& aInput, 
								// this parameter should be sha-1 hash in dss case (in rsa case anything but
								// it is expected to be concatenation of md5 and sha-1 hashes)
								HBufC8*& aSignature, 
								CCTCertInfo& aCertInfo,
								CCTKeyInfo& aKeyInfo,
								TRequestStatus& aStatus )
	{
	HWTLSTOKEN_LOG(  _L("	CHwTLSSession::ComputeDigitalSignature") )
	
	Cancel();
	
	aStatus = KRequestPending;
	iOriginalRequestStatus = &aStatus; 

	iSignatureOutput = &aSignature; 
	iInputToSign.Copy( aInput );

	if ( EFalse == iInitialised ) 
		{
		User::RequestComplete( iOriginalRequestStatus, KTLSErrNotInitialized);
		return;
		}
		
	iKeyObjectHandle = aKeyInfo.Handle();
	TInt err = KErrNone;
	TRAP( err, ( iClientCertInfo = CCTCertInfo::NewL(aCertInfo) ) )
	if ( KErrNone != err ) 
		{
		User::RequestComplete( iOriginalRequestStatus, KErrNoMemory );
		return;
		}
	iCertSelected = ETrue;
	
		
	ResetWorkingVars();
	
	TRAP( err, ( iUnifiedKeyStore =  CUnifiedKeyStore::NewL( iFs ) ) )
	if ( KErrNone != err ) 
		{
		User::RequestComplete( iOriginalRequestStatus, err );
		return;
		}
		
	iState = EInitKeyStoreForSigning;
	iStatus = KRequestPending;
	iUnifiedKeyStore->Initialize( iStatus );
	SetActive();
			
	return;
	
	}


void CHwTLSSession::PHash(
			const TDesC8& aInputData, 
			HBufC8*& aOutput, 
			const TPHashOp& aOperation,
			TRequestStatus& aStatus)
	{
	HWTLSTOKEN_LOG(  _L("CHwTLSSession::PHash 1") )
	
	aStatus = KRequestPending;
	iOriginalRequestStatus = &aStatus;

	if ( EFalse == iInitialised )
		{
		User::RequestComplete( iOriginalRequestStatus, KTLSErrNotInitialized);
		return;
		}

	TInt err = KErrNone;
	if ( KSSL3_0 == *iProtocolVersion )
		{
		if( EKeyBlockOp == aOperation )
			TRAP( err, SSLKeyBlockComputationsL ( aInputData, aOutput) )
		else
			{
			CMessageDigest* md5Dig = NULL;
			CMessageDigest* shaDig = NULL;
		
			TRAP( err, md5Dig = CMD5::NewL(); CleanupStack::PushL( md5Dig );
					   shaDig = CSHA1::NewL(); CleanupStack::PushL( shaDig ); ) 
			if( KErrNone != err )
				{
				User::RequestComplete( iOriginalRequestStatus, err );
				return;
				}
								
			md5Dig->Reset();
			md5Dig->Update( aInputData );
			
			shaDig->Reset();
			shaDig->Update( aInputData );
			
			TRAP( err, SSLPHashComputationsL ( md5Dig, shaDig, aOutput, aOperation ) )
			
			CleanupStack::PopAndDestroy( 2, md5Dig );
			}
		}
	
	if ( KTLS1_0 == *iProtocolVersion )
		TRAP( err, TLSPHashComputationsL ( aInputData, aOutput, aOperation ) )
		
	
	HWTLSTOKEN_LOG(  _L("	aOutput from PHash:") )
	if( NULL != aOutput )
		{
		HWTLSTOKEN_LOG_HEX( (const TUint8*)(aOutput->Des().Ptr()), 
						aOutput->Size() )
		}
						
	User::RequestComplete( iOriginalRequestStatus, err );
	return;

	} 
	
	
void CHwTLSSession::PHash(
		CMessageDigest* aMd5Digest, 
		CMessageDigest* aShaDigest,
		HBufC8*& aOutput, 
		const TPHashOp& aOperation,
		TRequestStatus& aStatus )
	{
	HWTLSTOKEN_LOG(  _L("CHwTLSSession::PHash for SSL") )
	
	aStatus = KRequestPending;
	iOriginalRequestStatus = &aStatus;

	if ( EFalse == iInitialised )
		{
		User::RequestComplete( iOriginalRequestStatus, KTLSErrNotInitialized);
		return;
		}
		
	if ( KTLS1_0 == *iProtocolVersion )
		{
		User::RequestComplete( iOriginalRequestStatus, KTLSErrBadProtocolVersion);
		return;
		}
		
	if( EKeyBlockOp == aOperation )
		{
		User::RequestComplete( iOriginalRequestStatus, KTLSErrBadArgument);
		return;
		}

	TInt err = KErrNone;
	if ( KSSL3_0 == *iProtocolVersion )
		TRAP( err, SSLPHashComputationsL ( aMd5Digest, aShaDigest, aOutput, aOperation ) )
	
		
		
	HWTLSTOKEN_LOG(  _L("	aOutput from PHash:") )
	HWTLSTOKEN_LOG_HEX( (const TUint8*)(aOutput->Des().Ptr()), 
						aOutput->Size() )
						
	User::RequestComplete( iOriginalRequestStatus, err );
	return;
	
	} 
	
	
//	from rfc2246: 5. HMAC and the pseudorandom function
//	...
//S1 and S2 are the two halves of the secret and each is the same
//   length. S1 is taken from the first half of the secret, S2 from the
//   second half. Their length is created by rounding up the length of the
//   overall secret divided by two; thus, if the original secret is an odd
//   number of bytes long, the last byte of S1 will be the same as the
//  first byte of S2.

//       L_S = length in bytes of secret;
//       L_S1 = L_S2 = ceil(L_S / 2);

//	   The secret is partitioned into two halves (with the possibility of
//   one shared byte) as described above, S1 taking the first L_S1 bytes
//   and S2 the last L_S2 bytes.

//   The PRF is then defined as the result of mixing the two pseudorandom
//   streams by exclusive-or'ing them together.

//      PRF(secret, label, seed) = P_MD5(S1, label + seed) XOR
//                                  P_SHA-1(S2, label + seed);

//   The label is an ASCII string. It should be included in the exact form
//   it is given without a length byte or trailing null character.  For
//   example, the label "slithy toves" would be processed by hashing the
//   following bytes:

//       73 6C 69 74 68 79 20 74 6F 76 65 73

//   Note that because MD5 produces 16 byte outputs and SHA-1 produces 20
//   byte outputs, the boundaries of their internal iterations will not be
//   aligned; to generate a 80 byte output will involve P_MD5 being
//   iterated through A(5), while P_SHA-1 will only iterate through A(4).
void CHwTLSSession::TLSPRFL( const TDesC8& aSecret,
				 const TDesC8& aLabelAndSeed,
				 const TInt aLen, 
				 TDes8& aOut)
	{
	
	__UHEAP_MARK;
	
	HWTLSTOKEN_LOG2( 
		 _L("CHwTLSSession::TLSPRFL called to generate %d length output"), aLen )
	HWTLSTOKEN_LOG(  _L("	aSecret:") )
	HWTLSTOKEN_LOG_HEX( (const TUint8*)(aSecret.Ptr()), 
						aSecret.Size() )
	HWTLSTOKEN_LOG(  _L("	aLabelAndSeed:") )
	HWTLSTOKEN_LOG_HEX( (const TUint8*)(aLabelAndSeed.Ptr()), 
						aLabelAndSeed.Size() )
	

	if ( aLen > aOut.MaxLength() )
		{
		HWTLSTOKEN_LOG(  _L("	not enough space in output buffer") )
		User::Leave( KTLSErrBadArgument );
		}

	aOut.SetLength( aLen );

	TInt half = (aSecret.Length() / 2) + (aSecret.Length() % 2);

	HBufC8* output2 = HBufC8::NewLC( aLen );
		
	TPtr8 tmpTPtr = output2->Des();
	TPtr8* tmpPtr = &tmpTPtr;
		
	TLSPRFComputationsL( aSecret.Left( half ), aLabelAndSeed, EMd5, aLen, aOut );
	TLSPRFComputationsL( aSecret.Right( half ) , aLabelAndSeed, ESha, aLen, *tmpPtr );
	
	// Xor:
	TInt i = 0;
	for( i=0; i < aLen; i++)
		aOut[i]^= output2->operator[](i);
		
	HWTLSTOKEN_LOG(  _L("	output from PRF:") )
	HWTLSTOKEN_LOG_HEX( (const TUint8*)(aOut.Ptr()), 
						aOut.Size() )
						
	CleanupStack::PopAndDestroy( 1, output2 );
	
	__UHEAP_MARKEND;	
	
	return;

	} 


	
void CHwTLSSession::TLSPHashComputationsL(
			const TDesC8& aInputData, 
			HBufC8*& aOutput, 
			const TPHashOp& aOperation
			)
	{
	
	HWTLSTOKEN_LOG(  _L("CHwTLSSession::TLSPHashComputationsL") )
	
	
	switch( aOperation )
	{
	case EKeyBlockOp:
		{
		HWTLSTOKEN_LOG(  _L("	EKeyBlockOp case") )
		//rfc2246, 6.3:
		// To generate the key material, compute

       //key_block = PRF(SecurityParameters.master_secret,
       //                   "key expansion",
       //                   SecurityParameters.server_random +
       //                   SecurityParameters.client_random);

		//until enough output has been generated. Then the key_block is
		//partitioned as follows:

		//client_write_MAC_secret[SecurityParameters.hash_size]
		//server_write_MAC_secret[SecurityParameters.hash_size]
		//client_write_key[SecurityParameters.key_material_length]
		//server_write_key[SecurityParameters.key_material_length]
		//client_write_IV[SecurityParameters.IV_size]
		//server_write_IV[SecurityParameters.IV_size]
		
		TInt keyMaterialSize;
		
		if( iCipherSuite->iIsExportable )
			{
			keyMaterialSize = 2 * ( iCipherSuite->iHashSize +
											iCipherSuite->iKeyMaterial + 
												iCipherSuite->iIVSize );
			
			}
		else
			{
			keyMaterialSize = 2 * ( iCipherSuite->iHashSize +
											iCipherSuite->iKeyMaterial + 
												iCipherSuite->iIVSize );
			}
		
		aOutput = HBufC8::NewL( keyMaterialSize );
		
		TPtr8 tmpTPtr1 = aOutput->Des();
		TPtr8* tmpPtr1 = &tmpTPtr1;
		TLSPRFL( *iMasterSecret, aInputData, keyMaterialSize, *tmpPtr1 );
		}				
		break;
	
	case EServerFinishedOp:
	case EClientFinishedOp:
		HWTLSTOKEN_LOG(  _L("	EServerFinishedOp/EClientFinishedOp case") )
		//	rfc2246, 7.4.9. Finished
		//	..
		//	struct {
        //  opaque verify_data[12];
       	//} Finished;

       	//verify_data
        //   PRF(master_secret, finished_label, MD5(handshake_messages) +
        //   SHA-1(handshake_messages)) [0..11];

       	//finished_label
        //   For Finished messages sent by the client, the string "client
        //   finished". For Finished messages sent by the server, the
        //   string "server finished". 
		{
		aOutput = HBufC8::NewL( KTLSFinishedMsgLen );
		
		TPtr8 tmpTPtr2 = aOutput->Des();
		TPtr8* tmpPtr2 = &tmpTPtr2;
		TLSPRFL( *iMasterSecret, aInputData, KTLSFinishedMsgLen, *tmpPtr2 );
		
		}
		break;

	case ECertificateVerifyOp:
		HWTLSTOKEN_LOG(  _L("	ECertificateVerifyOp case - NOT SUPPORTED") )
		// in TLS case, PHash should not be called for CertificateVerify calculations
		User::Leave( KTLSErrBadArgument );
		break;

	default:
		HWTLSTOKEN_LOG(  _L("	default case - NOTHING like this is SUPPORTED") )
		User::Leave( KTLSErrBadArgument );
		break;
	}; // end of switch()
	
	
	return;
	
	} 
	
	

//from rfc2246: 5. HMAC and the pseudorandom function
//	...
//	P_hash(secret, seed) = HMAC_hash(secret, A(1) + seed) +
//                              HMAC_hash(secret, A(2) + seed) +
//                              HMAC_hash(secret, A(3) + seed) + ...
//
//   Where + indicates concatenation.

//   A() is defined as:
//       A(0) = seed
//       A(i) = HMAC_hash(secret, A(i-1))

//  P_hash can be iterated as many times as is necessary to produce the
//   required quantity of data. For example, if P_SHA-1 was being used to
//   create 64 bytes of data, it would have to be iterated 4 times
//   (through A(4)), creating 80 bytes of output data; the last 16 bytes
//   of the final iteration would then be discarded, leaving 64 bytes of
//   output data.
void CHwTLSSession::TLSPRFComputationsL(
		   const TDesC8& aSecret,
		   const TDesC8& aSeed,
		   const TTLSMACAlgorithm& aMacAlg, //sha-1 is default
		   const TInt aLen, //no of bytes to produce
		   TDes8& aOut) // output buffer
	{
	__UHEAP_MARK; 

	HWTLSTOKEN_LOG2(  _L("CHwTLSSession::TLSPRFComputationsL called to produce %d bytes"), aLen )
	HWTLSTOKEN_LOG(  _L("	aSecret:") )
	HWTLSTOKEN_LOG_HEX( (const TUint8*)(aSecret.Ptr()), aSecret.Size() )
	
	if ( aLen > aOut.MaxLength() )
		{
		HWTLSTOKEN_LOG(  _L("	too small output buffer") )
		User::Leave( KTLSErrBadArgument );
		}
	
	CMessageDigest* digest1 = NULL;
	CMessageDigest* digest2 = NULL;
	
	switch ( aMacAlg )
	
	{
	case EMd5:
		digest2 = CMD5::NewL();
		CleanupStack::PushL( digest2 );
		digest1 = CMD5::NewL();
		break;
	case ESha:
		digest2 = CSHA1::NewL();
		CleanupStack::PushL( digest2 );
		digest1 = CSHA1::NewL();
		break;
	default:
		digest2 = CSHA1::NewL();
		CleanupStack::PushL( digest2 );
		digest1 = CSHA1::NewL();
		break;
	};
	CleanupStack::PushL( digest1 );
	
	HWTLSTOKEN_LOG2(  _L("	hash alg: %d"), aMacAlg )
	HWTLSTOKEN_LOG2(  _L("	size of MD5 object: %d"), sizeof(CMD5) )
	HWTLSTOKEN_LOG2(  _L("	size of SHA object: %d"), sizeof(CSHA1) )
	

		
	CHMAC* hmac1 = CHMAC::NewL( aSecret, digest1 );
	
	CleanupStack::Pop( digest1 );
	CleanupStack::PushL( hmac1 );
		
	CHMAC* hmac2 = CHMAC::NewL( aSecret, digest2 );
	
	CleanupStack::Pop( hmac1 );
	CleanupStack::Pop( digest2 );
	CleanupStack::PushL( hmac1 );
	CleanupStack::PushL( hmac2 );
	
	HWTLSTOKEN_LOG(  _L("	HMAC objects created") )
	
	aOut.Zero();
				
	hmac2->Update( aSeed );
	TBuf8<SHA1_HASH> A_i (hmac2->Final()); 
	HWTLSTOKEN_LOG(  _L("	A(1):") )
	HWTLSTOKEN_LOG_HEX( (const TUint8*)(A_i.Ptr()), A_i.Size() )
	
	
	do
	{
		hmac1->Reset();

		hmac1->Update( A_i );
		hmac1->Update( aSeed );
		TPtrC8 tmpHash = hmac1->Final(); 
		
		hmac2->Reset();
		hmac2->Update( A_i );
		A_i.Copy( hmac2->Final() );
		
		if(  aLen - aOut.Length() > tmpHash.Length() )
			aOut.Append( tmpHash );
		else 
			aOut.Append( tmpHash.Ptr() , aLen - aOut.Length() );
		
	}
	while( aOut.Length() < aLen );
	
	HWTLSTOKEN_LOG2(  _L("	aLen: %d"), aLen )
	HWTLSTOKEN_LOG2(  _L("	aOut length: %d"), aOut.Length() )
			
	aOut.SetLength( aLen );
	
	HWTLSTOKEN_LOG(  _L("	output from TLSPRFComputationsL:") )
	HWTLSTOKEN_LOG_HEX( (const TUint8*)(aOut.Ptr()), 
						aOut.Size() )

	CleanupStack::PopAndDestroy(hmac2);
	CleanupStack::PopAndDestroy(hmac1);
	
	__UHEAP_MARKEND;
	
	} 
			
						
void CHwTLSSession::SSLPHashComputationsL(
			CMessageDigest* aMd5Digest, 
			CMessageDigest* aShaDigest,
			HBufC8*& aOutput, 
			const TPHashOp& aOperation)
	{
	HWTLSTOKEN_LOG(  _L("CHwTLSSession::SSLPHashComputationsL") )	

		
	switch( aOperation )
	{
	case EKeyBlockOp:
		HWTLSTOKEN_LOG(  _L("	EKeyBlockOp case - BAD PHash version called") )
		User::Leave( KTLSErrBadArgument );
		break;
	
	case EServerFinishedOp:
	case EClientFinishedOp:
		HWTLSTOKEN_LOG(  _L("	EServerFinishedOp/EClientFinishedOp case") )
		SSLFinishedCheckComputationsL( aMd5Digest, aShaDigest, aOutput );	
		break;

	case ECertificateVerifyOp:
		HWTLSTOKEN_LOG(  _L("	ECertificateVerifyOp case") )
		SSLCertVerifyComputationsL( aMd5Digest, aShaDigest, aOutput );
		break;
				
	default:
		HWTLSTOKEN_LOG(  _L("	default case - NOT SUPPORTED") )
		User::Leave( KTLSErrBadArgument );
		break;
	}; // end of switch()
	
		
	return;
	
	} 



void CHwTLSSession::SSLKeyBlockComputationsL(
			const TDesC8& aInputData, 
			HBufC8*& aOutput)
	{
	
	HWTLSTOKEN_LOG(  _L("CHwTLSSession::SSLKeyBlockComputationsL") )	

	
		//key_block =
	    //MD5(master_secret + SHA(`A' + master_secret +
        //                   ServerHello.random +
        //                   ClientHello.random)) +
       	//MD5(master_secret + SHA(`BB' + master_secret +
            //                   ServerHello.random +
            //                  ClientHello.random)) +
	        //MD5(master_secret + SHA(`CCC' + master_secret +
            //                   ServerHello.random +
            //                   ClientHello.random)) + [...]; //understand that [...] means 'DDDD', 'EEEEE' etc

   			//until enough output has been generated.  Then the key_block is
   			//partitioned as follows.

     		//client_write_MAC_secret[CipherSpec.hash_size]
			//server_write_MAC_secret[CipherSpec.hash_size]
     		//client_write_key[CipherSpec.key_material]
     		//server_write_key[CipherSpec.key_material]
	
    	 //client_write_IV[CipherSpec.IV_size]  non-export ciphers 
	     //server_write_IV[CipherSpec.IV_size]  non-export ciphers 
		
		 //Any extra key_block material is discarded.

	  	 //Exportable encryption algorithms (for which
		 //CipherSpec.is_exportable is true) require additional processing as
		 //follows to derive their final write keys:

		 //    final_client_write_key = MD5(client_write_key +
         //                         ClientHello.random +
         //                         ServerHello.random);
		 //     final_server_write_key = MD5(server_write_key +
         //                        ServerHello.random +
         //                        ClientHello.random);

		 //Exportable encryption algorithms derive their IVs from the random
   		 //messages:

         //client_write_IV = MD5(ClientHello.random + ServerHello.random);
         //server_write_IV = MD5(ServerHello.random + ClientHello.random);

		 //Freier, Karlton, Kocher                                        [Page 34]
		 //	.

 		 //INTERNET-DRAFT                  SSL 3.0                November 18, 1996

		 //MD5 outputs are trimmed to the appropriate size by discarding the
   		 //least-significant bytes.

   		 //6.2.2.1 Export key generation example

   		 //SSL_RSA_EXPORT_WITH_RC2_CBC_40_MD5 requires five random bytes for
   		 //each of the two encryption keys and 16 bytes for each of the MAC
   		 //keys, for a total of 42 bytes of key material.  MD5 produces 16
   		 //bytes of output per call, so three calls to MD5 are required.  The
   		 //MD5 outputs are concatenated into a 48-byte key_block with the
   		 //first MD5 call providing bytes zero through 15, the second
   		 //providing bytes 16 through 31, etc.  The key_block is partitioned,
   		 //and the write keys are salted because this is an exportable
   		 //encryption algorithm.

     	 //client_write_MAC_secret = key_block[0..15]
     	 //server_write_MAC_secret = key_block[16..31]
     	 //client_write_key      = key_block[32..36]
     	 //server_write_key      = key_block[37..41]
     	 //final_client_write_key = MD5(client_write_key +
         //                         ClientHello.random +
         //                         ServerHello.random)[0..15];
     	 //final_server_write_key = MD5(server_write_key +
         //                         ServerHello.random +
         //                         ClientHello.random)[0..15];
     	 //client_write_IV = MD5(ClientHello.random +
         //                  ServerHello.random)[0..7];
     	 //server_write_IV = MD5(ServerHello.random +
         //                  ClientHello.random)[0..7];*/
	 
	const TUint keyMaterialSize = 2 * ( iCipherSuite->iHashSize +
						iCipherSuite->iKeyMaterial + 
						iCipherSuite->iIVSize );
	HWTLSTOKEN_LOG2(  _L("	key block of size %d is being generated"), keyMaterialSize )

	CMessageDigest* shaDig = NULL;
	CMessageDigest* md5Dig = NULL;
		
	shaDig = CSHA1::NewL(); 
	CleanupStack::PushL( shaDig );
					
	md5Dig = CMD5::NewL(); 
	CleanupStack::PushL( md5Dig );
		
				
	TInt i;
	const TInt maxCounter = (keyMaterialSize / MD5_HASH) + 1;
	
	aOutput = HBufC8::NewL( maxCounter*MD5_HASH );	

	if( maxCounter > KSSLSaltStringsMaxNo )
		{
		HWTLSTOKEN_LOG(  _L("	key block of size requested too big") )
		User::Leave( KTLSErrBadArgument );
		}

	for( i=0; i<maxCounter; i++ )
		{
		shaDig->Reset();

		TPtrC8 saltChars(KSSLSaltSource);
		TBuf8<KSSLSaltStringsMaxNo> salt;
		salt.AppendFill( saltChars.Ptr()[i], i+1);
		salt.SetLength(i+1);
				
		shaDig->Update( salt );
		shaDig->Update( *iMasterSecret );
		shaDig->Update( aInputData );

		TPtrC8 tmpHash = shaDig->Final(); 

		TBuf8<SHA1_HASH> tmp;  

		tmp.Copy( tmpHash );

		md5Dig->Reset();
		md5Dig->Update( *iMasterSecret );
		md5Dig->Update( tmp );

		aOutput->Des().Append( md5Dig->Final() );
		}
		
	CleanupStack::PopAndDestroy( 2, shaDig );
	
			
	return;	
		
	} 
			
			
			
void CHwTLSSession::SSLFinishedCheckComputationsL(
			CMessageDigest* aMd5Digest, 
			CMessageDigest* aShaDigest,
			HBufC8*& aOutput)
	{
	__UHEAP_MARK;
	
	HWTLSTOKEN_LOG(  _L("CHwTLSSession::SSLFinishedCheckComputationsL") )
	
			
		// SSL spec, 5.6.9 Finished:
		//	enum { client(0x434C4E54), server(0x53525652) } Sender;
		//   struct {
  		//     opaque md5_hash[16];
  		//   opaque sha_hash[20];
  	  	//} Finished;
  	    
  	    //md5_hash       MD5(master_secret + pad2 +
	    //                 MD5(handshake_messages + Sender +
	    //                   master_secret + pad1));
        //sha_hash        SHA(master_secret + pad2 +
   		//                 SHA(handshake_messages + Sender +
     	//                   master_secret + pad1));
		
	aMd5Digest->Update( *iMasterSecret ); 
	// assumed that padding should follow the pattern defined for MAC and
	// Certificate Verify calculations (SSL spec is unclear but this is the most 
	// obvious guess that all this calculations should look the same)
	TBuf8<KSSLNoOfPadsMd5> pad1md5;
	pad1md5.Fill( TUint8(KSSLPad1), KSSLNoOfPadsMd5 );
	aMd5Digest->Update( pad1md5 ); 

	TBuf8<MD5_HASH> tmp1; 

	tmp1.Copy( aMd5Digest->Final() );
		
	HWTLSTOKEN_LOG(  _L("	1st MD5 hash:") )
	HWTLSTOKEN_LOG_HEX( (const TUint8*)(tmp1.Ptr()), 
					tmp1.Size() )
	
	aMd5Digest->Reset();

	aMd5Digest->Update( *iMasterSecret ); 
	TBuf8<KSSLNoOfPadsMd5> pad2md5;
	pad2md5.Fill( TUint8(KSSLPad2), KSSLNoOfPadsMd5 );
	aMd5Digest->Update( pad2md5  ); 
	aMd5Digest->Update( tmp1 ); 

	tmp1.Copy( aMd5Digest->Final() );
		
				
	HWTLSTOKEN_LOG(  _L("	2nd MD5 hash:") )
	HWTLSTOKEN_LOG_HEX( (const TUint8*)(tmp1.Ptr()), 
					tmp1.Size() )

	TBuf8<SHA1_HASH + MD5_HASH> output; 
	
	output.Append( tmp1 );
	
	
	aShaDigest->Update( *iMasterSecret ); 

	TBuf8<KSSLNoOfPadsSha> pad1sha;
	pad1sha.Fill( TUint8(KSSLPad1), KSSLNoOfPadsSha );
	aShaDigest->Update( pad1sha  ); 

			
	TBuf8<SHA1_HASH> tmp2; 

	tmp2.Copy(  aShaDigest->Final() );
		
				
	HWTLSTOKEN_LOG(  _L("	1st SHA hash:") )
	HWTLSTOKEN_LOG_HEX( (const TUint8*)(tmp2.Ptr()), 
					tmp2.Size() )
	
	aShaDigest->Reset();

	aShaDigest->Update( *iMasterSecret ); 

	TBuf8<KSSLNoOfPadsSha> pad2sha;
	pad2sha.Fill( TUint8(KSSLPad2), KSSLNoOfPadsSha );
	aShaDigest->Update( pad2sha  ); 
	aShaDigest->Update( tmp2 ); 

	tmp2.Copy(  aShaDigest->Final() );
		
	HWTLSTOKEN_LOG(  _L("	2nd SHA hash:") )
	HWTLSTOKEN_LOG_HEX( (const TUint8*)(tmp2.Ptr()), 
					tmp2.Size() )

	output.Append( tmp2 );

		
	__UHEAP_MARKEND;
	
	aOutput = output.AllocL();
		
	return;
	
	}				   
				   
void CHwTLSSession::SSLCertVerifyComputationsL(
			CMessageDigest* aMd5Digest, 
			CMessageDigest* aShaDigest,
			HBufC8*& aOutput)
	{
	HWTLSTOKEN_LOG(  _L("CHwTLSSession::SSLCertVerifyComputationsL") )	

		// SSL: calculations for Finished messages and CertificateVerify are the same 
		// when sign alg is rsa
		// but output excludes md5 hash when signing alg. is dss

		
		//CertVerify:
		
		//	SSL, 5.6.8 Certificate verify

		//   This message is used to provide explicit verification of a client
		//   certificate.  This message is only sent following any client
		//   certificate that has signing capability (i.e. all certificates
		//   except those containing fixed Diffie-Hellman parameters).
		//
		//	   struct {
		//           Signature signature;
		//       } CertificateVerify;
		//
		// 	 CertificateVerify.signature.md5_hash
		//               MD5(master_secret + pad_2 +
		//        	        MD5(handshake_messages + master_secret + pad_1));
		//   Certificate.signature.sha_hash
		// 	            SHA(master_secret + pad_2 +
		//     	            SHA(handshake_messages + master_secret + pad_1));
		//
		//	Freier, Karlton, Kocher                                        [Page 30]
		//	.
		//
		//	INTERNET-DRAFT                  SSL 3.0                November 18, 1996
		//
		// 	 pad_1      This is identical to the pad_1 defined in
		//	                section 5.2.3.1.
		//     pad_2      This is identical to the pad_2 defined in
		//	                section 5.2.3.1.
			
				//that is:
				
		//	pad_1             The character 0x36 repeated 48 times for MD5
	    //  	                 or 40 times for SHA.
		//   pad_2             The character 0x5c repeated 48 times for MD5
      	//	                 or 40 times for SHA.
		
	
	TBuf8<KSSLMaxInputToSignLen> output;
				
	if( ERsaSigAlg == iCipherSuite->iSigAlg )
      {
	   CMessageDigest* pMd5 = aMd5Digest->CopyL();
	   pMd5->Update( *iMasterSecret ); 
	   // assumed that padding should follow the pattern defined for MAC and
	   // Certificate Verify calculations (SSL spec is unclear but this is the most 
	   // obvious guess that all this calculations should look the same)
	   TBuf8<KSSLNoOfPadsMd5> pad1md5;
	   pad1md5.Fill( TUint8(KSSLPad1), KSSLNoOfPadsMd5 );
	   pMd5->Update( pad1md5 ); 

	   TBuf8<MD5_HASH> tmp1; 

	   tmp1.Copy( pMd5->Final() );
		   
	   HWTLSTOKEN_LOG(  _L("	1st MD5 hash:") )
	   HWTLSTOKEN_LOG_HEX( (const TUint8*)(tmp1.Ptr()), 
					   tmp1.Size() )
	   
	   pMd5->Update( *iMasterSecret ); 
	   TBuf8<KSSLNoOfPadsMd5> pad2md5;
	   pad2md5.Fill( TUint8(KSSLPad2), KSSLNoOfPadsMd5 );
	   pMd5->Update( pad2md5  ); 
	   pMd5->Update( tmp1 ); 
		output.Append( pMd5->Final() );
      delete pMd5;
	   HWTLSTOKEN_LOG(  _L("	2nd MD5 hash:") )
	   HWTLSTOKEN_LOG_HEX( (const TUint8*)(output.Ptr()), 
					   output.Size() )
      }
		

	CMessageDigest* pSHA1 = aShaDigest->CopyL();
	pSHA1->Update( *iMasterSecret ); 

	TBuf8<KSSLNoOfPadsSha> pad1sha;
	pad1sha.Fill( TUint8(KSSLPad1), KSSLNoOfPadsSha );
	pSHA1->Update( pad1sha  ); 

	TBuf8<SHA1_HASH> tmp2; 

	tmp2.Copy( pSHA1->Final() );
		
	HWTLSTOKEN_LOG(  _L("	1st SHA hash:") )
	HWTLSTOKEN_LOG_HEX( (const TUint8*)(tmp2.Ptr()), 
					tmp2.Size() )
	
	pSHA1->Update( *iMasterSecret ); 
	TBuf8<KSSLNoOfPadsSha> pad2sha;
	pad2sha.Fill( TUint8(KSSLPad2), KSSLNoOfPadsSha );
	pSHA1->Update( pad2sha  ); 
	pSHA1->Update( tmp2 ); 
	tmp2.Copy( pSHA1->Final() );
   delete pSHA1;
		
	HWTLSTOKEN_LOG(  _L("	2nd SHA hash:") )
	HWTLSTOKEN_LOG_HEX( (const TUint8*)(tmp2.Ptr()), 
					tmp2.Size() )

	output.Append( tmp2 );

			
	aOutput = output.AllocL();
	
	return;
	
	} 
	
	
TInt CHwTLSSession::RunError(TInt aError)
	{
	HWTLSTOKEN_LOG2(  _L("CHwTLSSession::RunError called with error: %d"), aError )	
	
	iState = EIdle;
	ResetWorkingVars();
	User::RequestComplete( iOriginalRequestStatus, aError );
	return KErrNone;
	}
	

void CHwTLSSession::RunL()
	{
	HWTLSTOKEN_LOG(  _L("CHwTLSSession::RunL") )	
	
	if ( KErrNone != iStatus.Int() )
		{
		HWTLSTOKEN_LOG2(  _L(" asynchronous request completed with error: %d"), iStatus.Int() )
		iState = EIdle;
		User::Leave( iStatus.Int() );
		};

	switch( iState )
		{
		case EInitKeyStoreForSigning:
			HWTLSTOKEN_LOG(  _L("	case EInitKeyStoreForSigning") )	
			if ( ERsaSigAlg == iCipherSuite->iSigAlg )
				{
				iState = EComputeRsaSignature;
				iUnifiedKeyStore->Open(  
							iKeyObjectHandle, 
							iRsaSigner,
							iStatus );
				SetActive();
				}
			
			if ( EDsa == iCipherSuite->iSigAlg )
				{
				iState = EComputeDsaSignature;
				iUnifiedKeyStore->Open(  
							iKeyObjectHandle, 
							iDsaSigner,
							iStatus );
				SetActive();
				}
				
			break;
			
			
		case EComputeRsaSignature:
			HWTLSTOKEN_LOG(  _L("	case EComputeRsaSignature") )	
			iRsaSignature = NULL;
			
			iState = ESignWithRsa;
			iStatus = KRequestPending;
			iRsaSigner->Sign( iInputToSign, 
								iRsaSignature, 
								iStatus);
			SetActive();
			break;
			
		case ESignWithRsa:
			HWTLSTOKEN_LOG(  _L("	case ESignWithRsa") )	
			{	
			// from rfc2246, 4.7:
			//In RSA signing, a 36-byte structure of two hashes (one SHA and one
			//MD5) is signed (encrypted with the private key). It is encoded with
			//PKCS #1 block type 0 or type 1 as described in [PKCS1].
		
			const TInteger& sigVal = iRsaSignature->S(); 
		
			*iSignatureOutput = sigVal.BufferLC(); // returns  string in big endian order
			CleanupStack::Pop(1);  
			
			iState = EIdle;
			ResetWorkingVars();
			
			User::RequestComplete( iOriginalRequestStatus, KErrNone );
					
			}
			break;
			
		case EComputeDsaSignature:
			HWTLSTOKEN_LOG(  _L("	case EComputeDsaSignature") )	
			delete iDsaSignature;
			iDsaSignature = NULL;
			iState = ESignWithDsa;
			iStatus = KRequestPending;
			iDsaSigner->Sign( iInputToSign, 
								iDsaSignature, 
								iStatus);
			SetActive();
			
			break;
			
		case ESignWithDsa:
			HWTLSTOKEN_LOG(  _L("	case ESignWithDsa") )	
			{	
			// from rfc2246, 4.7:
			//In DSS, the 20 bytes of the SHA hash are run directly through the
			//Digital Signing Algorithm with no additional hashing. This produces
			//two values, r and s. The DSS signature is an opaque vector, as above,
			//the contents of which are the DER encoding of:

			//Dss-Sig-Value  ::=  SEQUENCE  {
			//			r       INTEGER,
			//          s       INTEGER
			//          }
		
			
			FormatDsaSignatureL( iDsaSignature , *iSignatureOutput );
									
			iState = EIdle;
			ResetWorkingVars();
			
			User::RequestComplete( iOriginalRequestStatus, KErrNone );
			}
			break;
			
			
		default: // EIdle or other strange value
			HWTLSTOKEN_LOG(  _L("	case default") )
			iState = EIdle;
			User::RequestComplete( iOriginalRequestStatus, KTLSErrUnknownRequest );
			break;	
		
		};
	
	return; 
	
	} 

void CHwTLSSession::FormatDsaSignatureL( 
											CDSASignature* aDsaSignature,
											HBufC8*& aOutput )
	{
	CASN1EncSequence* sigSeq;
	sigSeq = CASN1EncSequence::NewLC(); 
			
	// Stuff two signature integers into the sequence.
	RInteger valR = RInteger::NewL( aDsaSignature->R() );
	CleanupStack::PushL( valR );
						  
	CASN1EncBigInt* r = CASN1EncBigInt::NewL( valR ); 
	CleanupStack::PopAndDestroy( 1 );//valR
	CleanupStack::PushL( r );
							
	sigSeq->AddAndPopChildL(r);
	RInteger valS = RInteger::NewL( aDsaSignature->S() ); 
	CleanupStack::PushL( valS );
	CASN1EncBigInt* s = CASN1EncBigInt::NewL( valS );
	CleanupStack::PopAndDestroy( 1 );//valS
	CleanupStack::PushL( s );
	sigSeq->AddAndPopChildL(s);

	aOutput = HBufC8::NewMaxL(sigSeq->LengthDER() ); //done because of  a strange assert in CASN1EncBase::WriteDERL fn
	TUint pos = 0;
			
	TPtr8 tmpTPtr = aOutput->Des();
	TPtr8* tmpPtr = &tmpTPtr;
	sigSeq->WriteDERL( *tmpPtr, pos );
		
	CleanupStack::PopAndDestroy( 1 );
			
	return;
	}

TInt CHwTLSSession::KeyDerivation(
		const TDesC8& aLabel, 
		const TTLSMasterSecretInput& aMasterSecretInput, 
		TDes8& aKeyingMaterial)
	{
	HWTLSTOKEN_LOG(  _L("CHwTLSSession::KeyDerivation") )
	TInt err = KErrNone;
	TRAP(err,KeyDerivationL(aLabel,aMasterSecretInput,aKeyingMaterial));
	
	HWTLSTOKEN_LOG(  _L("	output from KeyDerivation:") )
	HWTLSTOKEN_LOG_HEX( (const TUint8*)(aKeyingMaterial.Ptr()), 
						aKeyingMaterial.Size() )
	return err;
	}

void CHwTLSSession::KeyDerivationL(
		const TDesC8& aLabel, 
		const TTLSMasterSecretInput& aMasterSecretInput, 
		TDes8& aKeyingMaterial)
	{
	HWTLSTOKEN_LOG(  _L("CHwTLSSession::KeyDerivationL") )
	
	TBuf8<KTLSMasterSecretLen + 2*KTLSServerClientRandomLen> seed;
	if ((aLabel.Length()>KTLSMasterSecretLen)||(aKeyingMaterial.MaxLength()<(KKeyingMaterialLen1+KKeyingMaterialLen2)))
		{
		User::Leave(KErrArgument);
		}
	
	seed.Append( aLabel );		
	seed.Append( aMasterSecretInput.iClientRandom );
	seed.Append( aMasterSecretInput.iServerRandom );
	
	TPtr8 keyingMaterial1((TUint8*)(aKeyingMaterial.Ptr()),0,KKeyingMaterialLen1);
	TPtr8 keyingMaterial2((TUint8*)(aKeyingMaterial.Ptr()+KKeyingMaterialLen1),0,KKeyingMaterialLen2);
	TLSPRFL( *iMasterSecret, seed, KKeyingMaterialLen1, keyingMaterial1);
	
	HWTLSTOKEN_LOG(  _L(" keyingMaterial1 generated::") )
	HWTLSTOKEN_LOG_HEX( (const TUint8*)(keyingMaterial1.Ptr()), 
						keyingMaterial1.Size() )

	if (keyingMaterial1.Length()!=KKeyingMaterialLen1)
		{
		User::Leave(KErrGeneral);
		}
	
	TBuf8<1> nullStr(0);	
	TLSPRFL( nullStr, seed, KKeyingMaterialLen2, keyingMaterial2);
	
	HWTLSTOKEN_LOG(  _L(" keyingMaterial2 generated::") )
	HWTLSTOKEN_LOG_HEX( (const TUint8*)(keyingMaterial2.Ptr()), 
						keyingMaterial2.Size() )

	if (keyingMaterial2.Length()!=KKeyingMaterialLen2)
		{
		User::Leave(KErrGeneral);
		}
	
	aKeyingMaterial.SetLength(KKeyingMaterialLen1+KKeyingMaterialLen2);		
	}

//
// cancel and reset functions
	

void CHwTLSSession::CancelServerCertificate()
	{
	HWTLSTOKEN_LOG(  _L("CHwTLSSession::CancelServerCertificate") );
	Cancel();
	return;
	}
	
void CHwTLSSession::CancelClientCertificate()
	{
	HWTLSTOKEN_LOG(  _L("CHwTLSSession::CancelClientCertificate") );
	Cancel();
	return;
	}

void CHwTLSSession::CancelClientKeyExchange()
	{
	HWTLSTOKEN_LOG(  _L("CHwTLSSession::CancelClientKeyExchange") );
	Cancel();
	return;
	}

void CHwTLSSession::CancelPHash()
	{
	HWTLSTOKEN_LOG(  _L("CHwTLSSession::CancelPHash") );
	Cancel();
	return;
	}

void CHwTLSSession::CancelComputeDigitalSignature()
	{
	HWTLSTOKEN_LOG(  _L("CHwTLSSession::CancelComputeDigitalSignature") );
	Cancel();
	return; 
	}

void CHwTLSSession::CancelConnectionEstablished()
	{
	HWTLSTOKEN_LOG(  _L("CHwTLSSession:::CancelConnectionEstablished") );
	Cancel();
	return; 
	}


void CHwTLSSession::DoCancel()
	{
			
	if ( (NULL != iUnifiedKeyStore) && (EIdle != iState))
		{
		iUnifiedKeyStore->Cancel();	
		}
	iState = EIdle;	
	User::RequestComplete(iOriginalRequestStatus, KErrCancel);	
 	return; 
	}
	

void CHwTLSSession::ResetWorkingVars()
	{
	
	if ( NULL != iRsaSigner )
		{
		iRsaSigner->Release();
		iRsaSigner = NULL;
		}
		
	if ( NULL != iDsaSigner )
		{
		iDsaSigner->Release();	
		iDsaSigner = NULL;
		}
		
	if ( NULL != iUnifiedKeyStore )
		{
		delete iUnifiedKeyStore; 
		iUnifiedKeyStore = NULL;
		}
				
	delete iRsaSignature;
	iRsaSignature = NULL;
	delete iDsaSignature;
	iDsaSignature = NULL;
	
	} 
	
	
void CHwTLSSession::Reset()
	{
	
	Cancel();
		
	delete iSessionNameAndID;
	delete iCipherSuite; 
	delete iCompressionMethod;
	delete iProtocolVersion;
	iMasterSecret->FillZ();
	delete iMasterSecret;
				
	ResetWorkingVars();
	
	} 
	

CHwTLSSession::CHwTLSSession(const TDesC& aLabel, CHwTlsToken& aToken) 
	: CActive( EPriorityStandard ), iLabel(aLabel), iToken(aToken) {};


CHwTLSSession::~CHwTLSSession()
	{
	Cancel();
	if( IsAdded() )
		Deque();
	
	iFs.Close();
	
	delete iSessionNameAndID;
	delete iCipherSuite; 
	delete iCompressionMethod;
	delete iProtocolVersion;
	iMasterSecret->FillZ();
	delete iMasterSecret;
	
		
	// working variables
	if ( NULL != iRsaSigner )
		iRsaSigner->Release();
		
	if ( NULL != iDsaSigner )
		iDsaSigner->Release();	
		
	if ( NULL != iUnifiedKeyStore )
		delete iUnifiedKeyStore;	
	
	if ( NULL != iClientCertInfo )
		iClientCertInfo->Release();
				
	delete iRsaSignature;
	delete iDsaSignature;
	
	}
