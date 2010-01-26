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
 @file tlsstepbase.cpp
 @internalTechnology
*/
#include "tlsstepbase.h"

#include <f32file.h>
#include <tlsprovinterface.h>
#include <asymmetric.h>
#include <asymmetrickeys.h>
#include <symmetric.h>
#include <asnpkcs.h>
#include "tlsgenericactive.h"
#include "psuedorandom.h"
#include "dhparamreader.h"
#include "testpadding.h"

CTlsStepBase::~CTlsStepBase()
	{
	delete iActive;
	delete iProvider;
	delete iSched;
	iSuites.Close();
//	iGenerator.Close();
//	iPrime.Close();
	delete iKeyPair;
	delete iClientMacSecret;
	delete iServerMacSecret;
	delete iClientWriteSecret;
	delete iServerWriteSecret;
	delete iClientInitVector;
	delete iServerInitVector;
	
	}
	
void CTlsStepBase::ConstructL()
	{
	iSched=new(ELeave) CActiveScheduler;
	CActiveScheduler::Install(iSched);
	
	iProvider = CTLSProvider::ConnectL();
	iActive = new (ELeave) CGenericActive;
	}

// Test methods

TInt CTlsStepBase::GetCipherSuitesL()
	{
	iSuites.Reset();
	iProvider->CipherSuitesL(iSuites, iActive->iStatus);
	iActive->Start();
	return iActive->iStatus.Int();
	}

TInt CTlsStepBase::GetCipherSuitesWithCancelL()
	{
	iSuites.Reset();
	iProvider->CipherSuitesL(iSuites, iActive->iStatus);
	iProvider->CancelRequest();
	iActive->Start();
	return iActive->iStatus.Int();
	}


TInt CTlsStepBase::GetCipherSuitesL(CTLSProvider* & aTLSProviderInstance, RArray<TTLSCipherSuite> & aCipherSuites)
	{
	aTLSProviderInstance->CipherSuitesL(aCipherSuites, iActive->iStatus);
	iActive->Start();
	return iActive->iStatus.Int();
	}

TInt CTlsStepBase::VerifyServerCertificateL(CX509Certificate*& aCertOut)
	{
	HBufC8* cert = ServerCertificateL();
	iProvider->VerifyServerCertificate(*cert, aCertOut, iActive->iStatus);
	iActive->Start();
	delete cert;
	return iActive->iStatus.Int();
	}

TInt CTlsStepBase::VerifyServerCertificateL(CTLSProvider* & aTLSProviderInstance, CX509Certificate*& aCertOut)
	{
	HBufC8* cert = ServerCertificateL();
	aTLSProviderInstance->VerifyServerCertificate(*cert, aCertOut, iActive->iStatus);
	iActive->Start();
	delete cert;
	return iActive->iStatus.Int();
	}
	
TInt CTlsStepBase::VerifyServerCertificateWithCancelL(CX509Certificate*& aCertOut)
	{
	HBufC8* cert = ServerCertificateL();
	iProvider->VerifyServerCertificate(*cert, aCertOut, iActive->iStatus);
	iProvider->CancelRequest();
	iActive->Start();
	delete cert;
	return iActive->iStatus.Int();
	}

TInt CTlsStepBase::RetrieveServerCert(CX509Certificate*& aCertOut)
	{
	iSession->ServerCertificate(aCertOut,iActive->iStatus);		
	iActive->Start();
	return iActive->iStatus.Int();
	}

TInt CTlsStepBase::CreateSessionL()
	{
	iProvider->CreateL(iSession, iActive->iStatus);
	iActive->Start();
	return iActive->iStatus.Int();
	}

TInt CTlsStepBase::CreateSessionWithCancelL()
	{
	iProvider->CreateL(iSession, iActive->iStatus);
	iProvider->CancelRequest();
	iActive->Start();
	return iActive->iStatus.Int();
	}

TInt CTlsStepBase::CreateSessionL(CTLSProvider* & aTLSProviderInstance, CTLSSession* aCTLSSession)
	{
	aTLSProviderInstance->CreateL(aCTLSSession, iActive->iStatus);
	iActive->Start();
	return iActive->iStatus.Int();
	}


TInt CTlsStepBase ::VerifyGetSessionL(TTLSServerAddr& aServerName,TInt& aSessionIdLength)
	{
	TTLSSessionId sessionId;
	iProvider->GetSessionL(
		aServerName,
		sessionId,
		iActive->iStatus );
	iActive->Start();
	aSessionIdLength = sessionId.Length();
	return iActive->iStatus.Int();
	}

TInt CTlsStepBase ::VerifyGetSessionL(CTLSProvider* & aTLSProviderInstance , TTLSServerAddr& aServerName, TInt& aSessionIdLength)
	{
	TTLSSessionId sessionId;
	aTLSProviderInstance->GetSessionL(
		aServerName,
		sessionId,
		iActive->iStatus );
	iActive->Start();
	aSessionIdLength = sessionId.Length();
	return iActive->iStatus.Int();
	}


TInt CTlsStepBase ::CreateSessionAddedL(TInt aHiByte,TInt aLoByte)
	{
	
	CTlsCryptoAttributes* TlsCryptoAttributes =  iProvider->Attributes();
	TlsCryptoAttributes->iNegotiatedProtocol = KTLS1_0;
	TlsCryptoAttributes->iCurrentCipherSuite.iHiByte = aHiByte;
	TlsCryptoAttributes->iCurrentCipherSuite.iLoByte = aLoByte;
	iProvider->CreateL(iSession, iActive->iStatus);
	iActive->Start();
	return iActive->iStatus.Int();
	}
	
TInt CTlsStepBase::ClearSessionCacheL(TTLSSessionNameAndID &aSessionNameAndId)
	{
	iProvider->ClearSessionCacheL( 
			aSessionNameAndId, 
			iActive->iStatus);
	iActive->Start();
	return iActive->iStatus.Int();
	}

TInt CTlsStepBase::ClearSessionCacheWithCancelL(TTLSSessionNameAndID &aSessionNameAndId)
	{
	iProvider->ClearSessionCacheL( 
			aSessionNameAndId, 
			iActive->iStatus);
	iProvider->CancelRequest();
	iActive->Start();
	return iActive->iStatus.Int();
	}

TInt CTlsStepBase::ClearSessionCacheL(CTLSProvider* & aTLSProviderInstance ,TTLSSessionNameAndID &aSessionNameAndId)
	{
	aTLSProviderInstance->ClearSessionCacheL( 
			aSessionNameAndId, 
			iActive->iStatus);
	iActive->Start();
	return iActive->iStatus.Int();
	}

void CTlsStepBase::SessionCancelReq()
	{
	iSession->CancelRequest();
	}

void CTlsStepBase::ProviderCancelReq()
	{
	iProvider->CancelRequest();
	}

TInt CTlsStepBase::ClientKeyExchange(HBufC8*& aMessageOut)
	{
	iSession->ClientKeyExchange(aMessageOut, iActive->iStatus);
	iActive->Start();
	return iActive->iStatus.Int();
	}

TInt CTlsStepBase::ClientKeyExchange(CTLSSession* &aCTLSSession, HBufC8*& aMessageOut)
	{
	aCTLSSession->ClientKeyExchange(aMessageOut, iActive->iStatus);
	iActive->Start();
	return iActive->iStatus.Int();
	}


TInt CTlsStepBase::ClientKeyExchangeWithCancel(HBufC8*& aMessageOut)
	{
	iSession->ClientKeyExchange(aMessageOut, iActive->iStatus);
	iSession->CancelRequest();
	iActive->Start();
	return iActive->iStatus.Int();
	}


TInt CTlsStepBase::GenerateClientFinishedL(CMessageDigest* aShaDigest, CMessageDigest* aMd5Digest, HBufC8*& aMessageOut)
	{
	iSession->ClientFinishedMsgL(aMd5Digest, aShaDigest, aMessageOut, iActive->iStatus);
	iActive->Start();
	return iActive->iStatus.Int();
	}
	
TInt CTlsStepBase::VerifyServerFinishedL(CMessageDigest* aShaDigest, CMessageDigest* aMd5Digest, const TDesC8& aMessage)
	{
	iSession->VerifyServerFinishedMsgL(aMd5Digest, aShaDigest, aMessage, iActive->iStatus);
	iActive->Start();
	return iActive->iStatus.Int();
	}

TInt CTlsStepBase::CipherSuiteIndex(const TTLSCipherSuite& aSuite)
	{
	for (TInt i = 0 ; i < CipherSuites().Count() ; ++i)
		{
		if (CipherSuites()[i] == aSuite)
			{
			return i;
			};
		}
	return KErrNotFound;
	}

HBufC8* CTlsStepBase::DerivePreMasterSecretL(const TDesC8& aClientKeyExMessage)
	{
	return DerivePreMasterSecretL(iProvider, aClientKeyExMessage);
	}

HBufC8* CTlsStepBase::DerivePreMasterSecretL(CTLSProvider* & aTLSProviderInstance, const TDesC8& aClientKeyExMessage)
	{
	// Look up the cipher suite we used for this test
	CTlsCryptoAttributes* atts = aTLSProviderInstance->Attributes();
	
	TInt index = CipherSuiteIndex(atts->iCurrentCipherSuite);
	if (index < 0)
		{
		INFO_PRINTF3(_L("Failed! Could not find cipher 0x%02x.0x%02x, did the ECOM plugin load okay?"),
			  atts->iCurrentCipherSuite.iHiByte, atts->iCurrentCipherSuite.iLoByte);
		User::Leave(index);
		}

	HBufC8* ret = NULL;

	// currently, we only support three key exhange algorithms, so that is what we test here
	switch (CipherSuites()[index].CipherDetails()->iKeyExAlg)
		{
	case ERsa:
		// decrypt the key exhange message with the "server" private key, and
		// verify the version and that the premaster key is the right size...
		{
		CDecPKCS8Data* keyData = ServerPrivateKeyL();
		CleanupStack::PushL(keyData);
		
		// we don't own this pointer...
		CPKCS8KeyPairRSA* key = static_cast<CPKCS8KeyPairRSA*>(keyData->KeyPairData());
		
		CRSAPKCS1v15Decryptor* decryptor = CRSAPKCS1v15Decryptor::NewLC(key->PrivateKey());
		
		ret = HBufC8::NewLC(decryptor->MaxOutputLength());
		TPtr8 ptr = ret->Des();
		decryptor->DecryptL(aClientKeyExMessage, ptr);

		CleanupStack::Pop(ret);
		CleanupStack::PopAndDestroy(2, keyData); // decryptor
		}
		break;
		
	case EDHE:
		{
		RInteger clientX = RInteger::NewL(aClientKeyExMessage);
		CleanupClosePushL(clientX);
		
		RInteger prime = RInteger::NewL(iPrime);
		CleanupClosePushL(prime);
		
		RInteger gen = RInteger::NewL(iGenerator);
		CleanupClosePushL(gen);
		
		CDHPublicKey* clientKey = CDHPublicKey::NewL(prime, gen, clientX);
		CleanupStack::Pop(3, &clientX); // prime, gen, adopted by clientKey
		CleanupStack::PushL(clientKey);
		
		CDH* dh = CDH::NewLC(KeyPair()->PrivateKey());
		// this cast is evil, but hey! it's test code. And I'm not gonna revise the interface.
		ret = const_cast<HBufC8*>(dh->AgreeL(*clientKey));
		
		CleanupStack::PopAndDestroy(2, clientKey); // dh
		}
		break;
		
	case EPsk:
		{
		// For PSK cipher suites the premaster secret is formed as follows: 
		// if the PSK is N octets long, concatenate a uint16 with the value N, 
		// N zero octets, a second uint16 with the value N, and the PSK itself.
		// REF: RFC4279
		
		ret = HBufC8::NewLC(iPskKey->Length()*2 + 4 );
		ret->Des().FillZ(iPskKey->Length()*2 + 4);
 		TPtr8 ptr = ret->Des();
		
		// Populates first two field bytes values. 
		
		ptr[0] = (iPskKey->Length() & 0xFF00 ) >> 8;
		ptr[1] = iPskKey->Length() & 0xFF; 
		
		// Populates second two field bytes values. 
		ptr[iPskKey->Length() + 2] = ptr[0];
		ptr[iPskKey->Length() + 3] = ptr[1];
		
		// Populates the actual key value.
		ptr.Replace(iPskKey->Length() + 4, iPskKey->Length(), iPskKey->Des() );
		
		CleanupStack::Pop(ret);
		
		}
		break;

	default:
		User::Leave(KErrUnknown);
		break;
		}
	
	return ret;
	}

HBufC8* CTlsStepBase::ComputeMasterSecretL(const TDesC8& aPremasterSecret)
	{
	return CTlsStepBase::ComputeMasterSecretL(iProvider, aPremasterSecret);
	}
	
HBufC8* CTlsStepBase::ComputeMasterSecretL(CTLSProvider* & aTLSProviderInstance, const TDesC8& aPremasterSecret)
	{
	if (aTLSProviderInstance->Attributes()->iNegotiatedProtocol == KSSL3_0)
		{
		return ComputeSslMasterSecretL(aPremasterSecret);
		}
	else if (aTLSProviderInstance->Attributes()->iNegotiatedProtocol == KTLS1_0)
		{
		return ComputeTlsMasterSecretL(aPremasterSecret);
		}
	else
		{
		// currently unknown protocol!
		User::Leave(KErrUnknown);
		return NULL; // keep the compiler happy..
		}
	}

HBufC8* CTlsStepBase::ComputeMacL(const TDesC8& aData, TInt64 aSequenceNumber, TRecordProtocol& aType, TBool aIsServerMac)
	{
	if (Provider()->Attributes()->iNegotiatedProtocol == KSSL3_0)
		{
		return ComputeSslMacL(aData, aSequenceNumber, aType, aIsServerMac);
		}
	else if (Provider()->Attributes()->iNegotiatedProtocol == KTLS1_0)
		{
		return ComputeTlsMacL(aData, aSequenceNumber, aType, aIsServerMac);
		}
	else
		{
		// currently unknown protocol!
		User::Leave(KErrUnknown);
		return NULL; // keep the compiler happy..
		}
	}
	
HBufC8* CTlsStepBase::ComputeFinishedMessageL(CMessageDigest* aShaDigest, CMessageDigest* aMd5Digest,
	const TDesC8& aMasterSecret, TBool aClientFinished)
	{
	if (Provider()->Attributes()->iNegotiatedProtocol == KSSL3_0)
		{
		return ComputeSslFinishedL(aShaDigest, aMd5Digest, aMasterSecret, aClientFinished);
		}
	else if (Provider()->Attributes()->iNegotiatedProtocol == KTLS1_0)
		{
		return ComputeTlsFinishedL(aShaDigest, aMd5Digest, aMasterSecret, aClientFinished);
		}
	else
		{
		// currently unknown protocol!
		User::Leave(KErrUnknown);
		return NULL; // keep the compiler happy..
		}
	}
	
HBufC8* CTlsStepBase::EncryptRecordL(const TDesC8& aData, TInt64 aSequenceNumber, TRecordProtocol& aType, TBool aIsServerCrypt)
	{
	// Compute the mac for this record...
	HBufC8* mac = ComputeMacL(aData, aSequenceNumber, aType, aIsServerCrypt);
	CleanupStack::PushL(mac);
	
	// now, create an encryptor for this operation....
	CTlsCryptoAttributes* atts = Provider()->Attributes();
	
	TInt index = CipherSuiteIndex(atts->iCurrentCipherSuite);
	User::LeaveIfError(index);
	
	TTLSCipherSuite suite = CipherSuites()[index];
	CSymmetricCipher* encryptor = NULL;
	HBufC8* key = aIsServerCrypt ? ServerWriteSecret() : ClientWriteSecret();
	
	if (suite.CipherDetails()->iCipherType == EStream)
		{
		
		switch (suite.CipherDetails()->iBulkCiphAlg)
			{
		case ERc4:
			encryptor = CARC4::NewL(*key, 0);
			break;
		case ENullSymCiph:
			encryptor = CNullCipher::NewL();
			break;	
		default:
			User::Leave(KErrUnknown); // unknown cipher!
			break;
			}
		}
	else
		{
		
		CBlockTransformation* transform = NULL;
		switch (suite.CipherDetails()->iBulkCiphAlg)
			{
		case EDes:
		case EDes40:
			transform = CDESEncryptor::NewLC(*key, EFalse);
			break;
			
		case E3Des:
			transform = C3DESEncryptor::NewLC(*key);
			break;
			
		case EAes:
			transform = CAESEncryptor::NewLC(*key);
			break;
			
		default:
			User::Leave(KErrUnknown); // unknown cipher!
			break;
			}
			
		HBufC8* iv = aIsServerCrypt ? ServerInitVector() : ClientInitVector();
		CleanupStack::Pop(transform); // ownership transfered to cbc
		CModeCBCEncryptor* cbc = CModeCBCEncryptor::NewLC(transform, *iv);
		
		CPaddingSSLv3* padding = CPaddingSSLv3::NewLC(cbc->BlockSize());
		// Use next line instead later. Currently broken. Will be required for TLS 1.2
		// CTlsTestPadding* padding = CTlsTestPadding::NewLC(cbc->BlockSize(), 0);
		
		encryptor = CBufferedEncryptor::NewL(cbc, padding);
		CleanupStack::Pop(2, cbc); // padding - ownership transfered to encryptor
		}
	
	CleanupStack::PushL(encryptor);
	
	// encrypt the data, and the mac... padding will be added as appropriate.
	TInt outputMaxLen = encryptor->MaxFinalOutputLength(mac->Length() + aData.Length());
	HBufC8* ret = HBufC8::NewLC(outputMaxLen);
	TPtr8 des = ret->Des();
	
	encryptor->Process(aData, des);
	encryptor->ProcessFinalL(*mac, des);
	
	CleanupStack::Pop(ret);
	CleanupStack::PopAndDestroy(2, mac); // encryptor
	return ret;
	}

// INI Read methods

HBufC8* CTlsStepBase::ServerRandomL()
	{
	return ReadRandomL(KServerRandomFile);
	}
	
HBufC8* CTlsStepBase::ClientRandomL()
	{
	return ReadRandomL(KClientRandomFile);
	}
	
TTLSCipherSuite CTlsStepBase::CipherSuiteL()
	{
	TTLSCipherSuite ret;
	
	TInt highByte(0);
	if (!GetIntFromConfig(ConfigSection(), KCipherHighByte, highByte))
		{
		User::Leave(KErrNotFound);
		}
	
	TInt lowByte(0);
	if (!GetIntFromConfig(ConfigSection(), KCipherLowByte, lowByte))
		{
		User::Leave(KErrNotFound);
		}
		
	ret.iHiByte = highByte;
	ret.iLoByte = lowByte;
		
	return ret;
	}
	
TTLSProtocolVersion CTlsStepBase::ProtocolVersionL()
	{
	TTLSProtocolVersion ret;
	
	TInt majorVersion(0);
	if (!GetIntFromConfig(ConfigSection(), KProtocolMajorVersion, majorVersion))
		{
		User::Leave(KErrNotFound);
		}
		
	TInt minorVersion(0);
	if (!GetIntFromConfig(ConfigSection(), KProtocolMinorVersion, minorVersion))
		{
		User::Leave(KErrNotFound);
		}
		
	ret.iMajor = majorVersion;
	ret.iMinor = minorVersion;
	
	return ret;	
	}
	
TTLSSessionId CTlsStepBase::SessionId()
	{
	// Create a unique session ID, by filling with zeros, then setting the top
	// 8 bytes with the current time.
	TTLSSessionId ret;
	ret.SetMax();
	ret.Fill(0);
	
	TTime now;
	now.UniversalTime();
	
	TInt64 time = now.Int64();
	TInt byteNum(0);
	while (time)
		{
		ret[byteNum++] = time & 0xFF;
		time >>= 8;
		}
	
	return ret;
	}
	
HBufC8* CTlsStepBase::ServerCertificateL()
	{
	TPtrC serverCertName;
	if (!GetStringFromConfig(ConfigSection(), KServerCert, serverCertName))
		{
		User::Leave(KErrNotFound);
		}
	
	RFs fs;
	User::LeaveIfError(fs.Connect());
	CleanupClosePushL(fs);
	
	RFile certFile;
	User::LeaveIfError(certFile.Open(fs, serverCertName, EFileRead));
	CleanupClosePushL(certFile);
	
	TInt size(0);
	User::LeaveIfError(certFile.Size(size));
	HBufC8* certData = HBufC8::NewLC(size);
	TPtr8 ptr = certData->Des();
	
	User::LeaveIfError(certFile.Read(ptr, size));
	
	CleanupStack::Pop(certData);
	CleanupStack::PopAndDestroy(2, &fs); // certFile
	return certData;
	}
	
void CTlsStepBase::ReadDHParamsL()
	{
	TPtrC dhParamName;
	if (!GetStringFromConfig(ConfigSection(), KDhParamFile, dhParamName))
		{
		User::Leave(KErrNotFound);
		}

	CDHParamReader::DecodeDERL(dhParamName, iPrime, iGenerator);
	iKeyPair = CDHKeyPair::NewL(iPrime, iGenerator);
	}
	
	
TPtrC CTlsStepBase::DomainNameL()
	{
	TPtrC name;
	if (!GetStringFromConfig(ConfigSection(), KDomainName, name))
		{
		User::Leave(KErrNotFound);
		}
	return name;
	}
	
CDecPKCS8Data* CTlsStepBase::ServerPrivateKeyL()
	{
	TPtrC serverKeyName;
	if (!GetStringFromConfig(ConfigSection(), KServerKey, serverKeyName))
		{
		User::Leave(KErrNotFound);
		}
	
	RFs fs;
	User::LeaveIfError(fs.Connect());
	CleanupClosePushL(fs);
	
	RFile keyFile;
	User::LeaveIfError(keyFile.Open(fs, serverKeyName, EFileRead));
	CleanupClosePushL(keyFile);
	
	TInt size(0);
	User::LeaveIfError(keyFile.Size(size));
	HBufC8* keyData = HBufC8::NewLC(size);
	TPtr8 ptr = keyData->Des();
	
	User::LeaveIfError(keyFile.Read(ptr, size));
	
	CDecPKCS8Data* ret = TASN1DecPKCS8::DecodeDERL(*keyData);

	CleanupStack::PopAndDestroy(3, &fs); // keyFile, keyData
	return ret;
	}
	
HBufC8* CTlsStepBase::ReadRandomL(const TDesC& aTag)
	{
	TPtrC randomFile;
	if (!GetStringFromConfig(ConfigSection(), aTag, randomFile))
		{
		User::Leave(KErrNotFound);
		}
	
	RFs fs;
	User::LeaveIfError(fs.Connect());
	CleanupClosePushL(fs);
	
	RFile file;
	User::LeaveIfError(file.Open(fs, randomFile, EFileRead));
	CleanupClosePushL(file);
	
	TInt fileSize;
	User::LeaveIfError(file.Size(fileSize));
	
	HBufC8* random = HBufC8::NewLC(fileSize);
	TPtr8 randomDes = random->Des();
	User::LeaveIfError(file.Read(randomDes));
	
	CleanupStack::Pop(random);
	CleanupStack::PopAndDestroy(2, &fs); // file
	return random;
	}
	
HBufC8* CTlsStepBase::ComputeTlsMasterSecretL(const TDesC8& aPremasterSecret)
	{
	HBufC8* random = HBufC8::NewLC(KTLSServerClientRandomLen * 2);
	
	TTLSMasterSecretInput params = iProvider->Attributes()->iMasterSecretInput;
	random->Des().Append(params.iClientRandom);
	random->Des().Append(params.iServerRandom);
	
	_LIT8(KMasterSecretLabel, "master secret");
	HBufC8* ret = CTls10PsuedoRandom::PseudoRandomL(aPremasterSecret, KMasterSecretLabel, 
		*random, KTLSMasterSecretLen);
	CleanupStack::PushL(ret);
	
	ComputeTlsCipherKeysL(*ret, *random);
	
	CleanupStack::Pop(ret);
	CleanupStack::PopAndDestroy(random);
	return ret;
	}
	
HBufC8* CTlsStepBase::ComputeSslMasterSecretL(const TDesC8& aPremasterSecret)
	{
	HBufC8* random = HBufC8::NewLC(KTLSServerClientRandomLen * 2);
	
	TTLSMasterSecretInput params = iProvider->Attributes()->iMasterSecretInput;
	random->Des().Append(params.iClientRandom);
	random->Des().Append(params.iServerRandom);
	
	HBufC8* ret = CSsl30PsuedoRandom::PseudoRandomL(aPremasterSecret, *random, KTLSMasterSecretLen);
	CleanupStack::PushL(ret);
	
	ComputeSslCipherKeysL(*ret, *random);
	
	CleanupStack::Pop(ret);
	CleanupStack::PopAndDestroy(random);
	return ret;
	}
	
void CTlsStepBase::ComputeTlsCipherKeysL(const TDesC8& aMasterSecret, const TDesC8& aRandom)
	{
	// Look up the cipher suite we used for this test
	CTlsCryptoAttributes* atts = Provider()->Attributes();
	
	TInt index = CipherSuiteIndex(atts->iCurrentCipherSuite);
	User::LeaveIfError(index);
	
	TTLSCipherSuite suite = CipherSuites()[index];
	
	// Key material is:
	// - 2 mac secrets of size hash_size
	// - 2 bulk cipher secrets of size keymaterial_size
	// - 2 initialisation vectors of size iv_size
	
	TInt hashSize = suite.CipherDetails()->iHashSize;
	TInt keySize = suite.CipherDetails()->iKeyMaterial;
	TInt ivSize = suite.CipherDetails()->iIVSize;
	
	TInt keyMaterialSize = (2*hashSize) + (2*keySize) + (2*ivSize);
	
	// This calculation uses the random data in the opposite order to the master secret
	
	HBufC8* random = HBufC8::NewLC(KTLSServerClientRandomLen * 2);
	TTLSMasterSecretInput params = atts->iMasterSecretInput;
	random->Des().Append(params.iServerRandom);
	random->Des().Append(params.iClientRandom);
	
	_LIT8(KKeyExpansionLabel, "key expansion");
	HBufC8* keyMaterial = CTls10PsuedoRandom::PseudoRandomL(aMasterSecret, KKeyExpansionLabel,
	    *random, keyMaterialSize);		
	CleanupStack::PushL(keyMaterial);
		
	iClientMacSecret = keyMaterial->Left(hashSize).AllocL();
	iServerMacSecret = keyMaterial->Mid(hashSize, hashSize).AllocL();
		
	// if the cipher is exportable, we need to do further PRF calculations to get the keys
	if (suite.CipherDetails()->iIsExportable)
		{
		TInt expandedKeySize = 	suite.CipherDetails()->iExpKeySize;
		
		_LIT8(KClientWriteLabel, "client write key");
		iClientWriteSecret = CTls10PsuedoRandom::PseudoRandomL(keyMaterial->Mid(2*hashSize, keySize), 
		    KClientWriteLabel, aRandom, expandedKeySize);
			
		_LIT8(KServerWriteLabel, "server write key");
		iServerWriteSecret = CTls10PsuedoRandom::PseudoRandomL(keyMaterial->Mid((2*hashSize)+keySize, keySize),
		    KServerWriteLabel, aRandom, expandedKeySize);
			
		_LIT8(KIVBlockLabel, "IV block");
		HBufC8* ivMaterial = CTls10PsuedoRandom::PseudoRandomL(KNullDesC8, KIVBlockLabel, aRandom, 2*ivSize);
		CleanupStack::PushL(ivMaterial);
		
		iClientInitVector = ivMaterial->Left(ivSize).AllocL();
		iServerInitVector = ivMaterial->Right(ivSize).AllocL();
		
		CleanupStack::PopAndDestroy(ivMaterial);
		}
	else
		{
		// just devide the key material up in to its respective blocks
		iClientWriteSecret = keyMaterial->Mid(2*hashSize, keySize).AllocL();
		iServerWriteSecret = keyMaterial->Mid((2*hashSize)+keySize, keySize).AllocL();
		iClientInitVector = keyMaterial->Mid((2*hashSize)+(2*keySize), ivSize).AllocL();
		iServerInitVector = keyMaterial->Right(ivSize).AllocL();
		}
	
	CleanupStack::PopAndDestroy(2, random); // keyMaterial
	}
	
void CTlsStepBase::ComputeSslCipherKeysL(const TDesC8& aMasterSecret, const TDesC8& aRandom)
	{
	// Look up the cipher suite we used for this test
	CTlsCryptoAttributes* atts = Provider()->Attributes();
	
	TInt index = CipherSuiteIndex(atts->iCurrentCipherSuite);
	User::LeaveIfError(index);
	
	TTLSCipherSuite suite = CipherSuites()[index];
	
	// Key material is:
	// - 2 mac secrets of size hash_size
	// - 2 bulk cipher secrets of size keymaterial_size
	// - 2 initialisation vectors of size iv_size
	
	TInt hashSize = suite.CipherDetails()->iHashSize;
	TInt keySize = suite.CipherDetails()->iKeyMaterial;
	TInt ivSize = suite.CipherDetails()->iIVSize;
	
	TInt keyMaterialSize = (2*hashSize) + (2*keySize) + (2*ivSize);
	
	// This calculation uses the random data in the opposite order to the master secret
	
	HBufC8* random = HBufC8::NewLC(KTLSServerClientRandomLen * 2);
	TTLSMasterSecretInput params = atts->iMasterSecretInput;
	random->Des().Append(params.iServerRandom);
	random->Des().Append(params.iClientRandom);
	
	HBufC8* keyMaterial = CSsl30PsuedoRandom::PseudoRandomL(aMasterSecret, *random, keyMaterialSize);		
	CleanupStack::PushL(keyMaterial);
	
	iClientMacSecret = keyMaterial->Left(hashSize).AllocL();
	iServerMacSecret = keyMaterial->Mid(hashSize, hashSize).AllocL();
	
	// if the cipher is exportable, we need to do further MD5 calculations to get the keys
	if (suite.CipherDetails()->iIsExportable)
		{
		TInt expandedKeySize = 	suite.CipherDetails()->iExpKeySize;

		CMessageDigest* md5dig = CMessageDigestFactory::NewDigestLC(CMessageDigest::EMD5);
			
		md5dig->Update(keyMaterial->Mid((2*hashSize), keySize));
		iClientWriteSecret = md5dig->Hash(aRandom).Left(expandedKeySize).AllocL();
		md5dig->Reset();
			
		md5dig->Update(keyMaterial->Mid((2*hashSize)+keySize, keySize));
		iServerWriteSecret = md5dig->Hash(*random).Left(expandedKeySize).AllocL();
		md5dig->Reset();
		
		iClientInitVector = md5dig->Hash(aRandom).Left(ivSize).AllocL();
		md5dig->Reset();
		
		iServerInitVector = md5dig->Hash(*random).Left(ivSize).AllocL();
		
		CleanupStack::PopAndDestroy(md5dig);
		}
	else
		{
		// just devide the key material up in to its respective blocks
		iClientWriteSecret = keyMaterial->Mid(2*hashSize, keySize).AllocL();
		iServerWriteSecret = keyMaterial->Mid((2*hashSize)+keySize, keySize).AllocL();
		iClientInitVector = keyMaterial->Mid((2*hashSize)+(2*keySize), ivSize).AllocL();
		iServerInitVector = keyMaterial->Right(ivSize).AllocL();
		}
	
	CleanupStack::PopAndDestroy(2, random); // keyMaterial
	}
	
HBufC8* CTlsStepBase::ComputeTlsMacL(const TDesC8& aData, TInt64 aSequenceNumber, TRecordProtocol& aType, TBool aIsServerMac)
	{
	HBufC8* macSecret = aIsServerMac ? ServerMacSecret() : ClientMacSecret();
		
	// look up the cipher suite
	CTlsCryptoAttributes* atts = Provider()->Attributes();
	
	TInt index = CipherSuiteIndex(atts->iCurrentCipherSuite);
	User::LeaveIfError(index);
	
	TTLSCipherSuite suite = CipherSuites()[index];
	
	// use the hash algorithm associated with it for the mac. 
	CMessageDigest::THashId hashId = CMessageDigest::ESHA1;
	switch (suite.CipherDetails()->iMacAlg)
		{
	case EMd5:
		hashId = CMessageDigest::EMD5;
		break;
	case ESha:
		hashId = CMessageDigest::ESHA1;
		break;
	default:
		User::Leave(KErrUnknown); // unknown mac algorithm
		break;
		}
	
	CMessageDigest* digest = CMessageDigestFactory::NewHMACLC(hashId, *macSecret);
	
	// construct the mac header, which consists of:
	// - The 8 byte sequence number
	// - The one byte record type
	// - The two byte version
	// - The two byte record length
	// = 13 bytes
	
	TBuf8<13> macHeader;
	macHeader.SetLength(13);
	
	TUint len = aData.Length();
	TInt64 seq = aSequenceNumber;
	for (TInt i = 7; i >= 0; --i)
		{
		macHeader[i] = seq;
		seq >>= 8;
		}
		
	macHeader[8] = aType;
	macHeader[9] = atts->iNegotiatedProtocol.iMajor;
	macHeader[10] = atts->iNegotiatedProtocol.iMinor;
	macHeader[11] = (len >> 8);
	macHeader[12] = len;
	
	// now, hash the header and the data to get the final mac
	digest->Update(macHeader);
	HBufC8* ret = digest->Hash(aData).AllocL();
	
	CleanupStack::PopAndDestroy(digest);
	return ret;
	}


HBufC8* CTlsStepBase::ComputeSslMacL(const TDesC8& aData, TInt64 aSequenceNumber, TRecordProtocol& aType, TBool aIsServerMac)
	{
	HBufC8* macSecret = aIsServerMac ? ServerMacSecret() : ClientMacSecret();
		
	// look up the cipher suite
	CTlsCryptoAttributes* atts = Provider()->Attributes();
	
	TInt index = CipherSuiteIndex(atts->iCurrentCipherSuite);
	User::LeaveIfError(index);
	
	TTLSCipherSuite suite = CipherSuites()[index];
	
	// use the hash algorithm associated with it for the mac. 
	CMessageDigest::THashId hashId = CMessageDigest::ESHA1;
	TInt paddingLen = 0;
	switch (suite.CipherDetails()->iMacAlg)
		{
	case EMd5:
		hashId = CMessageDigest::EMD5;
		paddingLen = 48;
		break;
	case ESha:
		hashId = CMessageDigest::ESHA1;
		paddingLen = 40;
		break;
	default:
		User::Leave(KErrUnknown); // unknown mac algorithm
		break;
		}
	
	CMessageDigest* digest = CMessageDigestFactory::NewDigestLC(hashId);
	
	// construct the mac header, which consists of:
	// - The 8 byte sequence number
	// - The one byte record type
	// - The two byte record length
	// = 11 bytes
	
	TBuf8<11> macHeader;
	macHeader.SetLength(11);
	
	TUint len = aData.Length();
	TInt64 seq = aSequenceNumber;
	for (TInt i = 7; i >= 0; --i)
		{
		macHeader[i] = seq;
		seq >>= 8;
		}
		
	macHeader[8] = aType;
	macHeader[9] = (len >> 8);
	macHeader[10] = len;
	
	// construct the padding.
	HBufC8* padding = HBufC8::NewLC(paddingLen);
	TPtr8 paddingBuf = padding->Des();	
	paddingBuf.SetLength(paddingLen);
	
	// fill padding buffer for padding 1
	paddingBuf.Fill(0x36);
	
	// compute the inner hash
	digest->Update(*macSecret);
	digest->Update(*padding);
	digest->Update(macHeader);
	digest->Update(aData);
	HBufC8* inner = digest->Final().AllocL();
	
	digest->Reset();
	// fill padding buffer for padding 2
	paddingBuf.Fill(0x5c);
	
	// compute the outer hash
	digest->Update(*macSecret);
	digest->Update(*padding);
	digest->Update(*inner);
	delete inner;
	
	HBufC8* ret = digest->Final().AllocL();
	CleanupStack::PopAndDestroy(2, digest); // padding
	return ret;
	}

HBufC8* CTlsStepBase::ComputeTlsFinishedL(CMessageDigest* aShaDigest, CMessageDigest* aMd5Digest,
	const TDesC8& aMasterSecret, TBool aClientFinished)
	{
	CMessageDigest* ourSha = aShaDigest->CopyL();
	CleanupStack::PushL(ourSha);
	
	CMessageDigest* ourMd = aMd5Digest->CopyL();
	CleanupStack::PushL(ourMd);
	
	TInt len = ourSha->HashSize() + ourMd->HashSize();
	HBufC8* hashBuf = HBufC8::NewLC(len);
	
	hashBuf->Des().Append(ourMd->Final());
	hashBuf->Des().Append(ourSha->Final());
	
	_LIT8(KClientLabel, "client finished");
	_LIT8(KServerLabel, "server finished");
	
	TPtrC8 label;
	if (aClientFinished)
		{
		label.Set(KClientLabel);
		}
	else
		{
		label.Set(KServerLabel);
		}
	
	HBufC8* ret = CTls10PsuedoRandom::PseudoRandomL(aMasterSecret, label, *hashBuf, 12);
	CleanupStack::PopAndDestroy(3, ourSha);
	return ret;
	}
	
HBufC8* CTlsStepBase::ComputeSslFinishedL(CMessageDigest* aShaDigest, CMessageDigest* aMd5Digest,
	const TDesC8& aMasterSecret, TBool aClientFinished)
	{
	CMessageDigest* ourSha = aShaDigest->CopyL();
	CleanupStack::PushL(ourSha);
	
	CMessageDigest* ourMd = aMd5Digest->CopyL();
	CleanupStack::PushL(ourMd);
	
	_LIT8(KClientLabel, "CLNT");
	_LIT8(KServerLabel, "SRVR");
	
	TPtrC8 label;
	if (aClientFinished)
		{
		label.Set(KClientLabel);
		}
	else
		{
		label.Set(KServerLabel);
		}

	// hash the label and master secret
	ourSha->Update(label);
	ourMd->Update(label);
	
	ourSha->Update(aMasterSecret);
	ourMd->Update(aMasterSecret);

	// add the padding
	HBufC8* shaPadding = HBufC8::NewLC(40);
	TPtr8 shaPaddingBuf = shaPadding->Des();
	shaPaddingBuf.SetLength(40);
	
	HBufC8* mdPadding = HBufC8::NewLC(48);
	TPtr8 mdPaddingBuf = mdPadding->Des();
	mdPaddingBuf.SetLength(48);
	
	shaPaddingBuf.Fill(0x36);
	mdPaddingBuf.Fill(0x36);
	
	ourSha->Update(shaPaddingBuf);
	ourMd->Update(mdPaddingBuf);
	
	// finalise the inner hashes
	HBufC8* innerSha = ourSha->Final().AllocLC();
	HBufC8* innerMd = ourMd->Final().AllocLC();
	
	// reset for the outer hashes
	ourSha->Reset();
	ourMd->Reset();
	
	// hash master secret and padding for outer hashes
	ourSha->Update(aMasterSecret);
	ourMd->Update(aMasterSecret);
	
	shaPaddingBuf.Fill(0x5c);
	mdPaddingBuf.Fill(0x5c);
	
	ourSha->Update(shaPaddingBuf);
	ourMd->Update(mdPaddingBuf);
	
	// and finally, the inner hash.
	ourSha->Update(*innerSha);
	ourMd->Update(*innerMd);
	
	// create the final buffer
	HBufC8* ret = HBufC8::NewL(ourSha->HashSize() + ourMd->HashSize());
	ret->Des().Append(ourMd->Final());
	ret->Des().Append(ourSha->Final());

	CleanupStack::PopAndDestroy(6, ourSha); // ourMd, shaPadding, mdPadding, innerSha, innerMd
	return ret;		
	}
	
TInt CTlsStepBase::ClientCertificate(CX509Certificate* aCert)
	{
	iSession->ClientCertificate(aCert,iActive->iStatus);
	iActive->Start();
	return iActive->iStatus.Int();
	}

TInt CTlsStepBase::ClientCertificate(HBufC8*& aCertBuf)
	{
	iActive->iStatus = KRequestPending;
	iSession->ClientCertificate(aCertBuf, iActive->iStatus);
	iActive->Start();
	return iActive->iStatus.Int();
	}

TInt CTlsStepBase::ClientCertificate(RPointerArray<HBufC8>* aClientCertArray)
	{
	iActive->iStatus = KRequestPending;
	iSession->ClientCertificate(aClientCertArray, iActive->iStatus);
	iActive->Start();
	return iActive->iStatus.Int();
	}

TInt CTlsStepBase::CertificateVerifySignatureL(
		CMessageDigest* iMd5DigestInput,
		CMessageDigest* iShaDigestInput,
		HBufC8*& aOutput)
	{
	iActive->iStatus = KRequestPending;
	iSession->CertificateVerifySignatureL(
		iMd5DigestInput, 
		iShaDigestInput,
		aOutput,
		iActive->iStatus);
	iActive->Start();
	return iActive->iStatus.Int();
	}
	
TBool CTlsStepBase::ReadPskToBeUsedL() 
	{
	
	TBool theReturn = ETrue;
	
	
 	TBool usePsk;
	if(GetBoolFromConfig(ConfigSection(), KUsePsk, usePsk))
		{
	 	iUsePsk = usePsk;
	 	}
	else
		{
		iUsePsk = EFalse;
		theReturn = EFalse;
		}
	
 	// Reads key 1 value	
 	if (iUsePsk)
		{	
			TPtrC8  pskKey;
			if(GetKeyFromConfigL(ConfigSection(),KPskKey ,pskKey ) )
				{
				iPskKey = pskKey.AllocL();
				}
				else
				{
				ERR_PRINTF1(_L("Couldn't read PSK key") );
				User::Leave(KErrGeneral);
				}
		}  
		
 	// Reads PSK identity	
 	if (iUsePsk)
		{	
			TPtrC   pskIdentity;
			if(GetStringFromConfig(ConfigSection(), KPskIdentity, pskIdentity) )
				{
				iPskIdentity = HBufC8::New(pskIdentity.Length());
				iPskIdentity->Des().Copy(pskIdentity);
				}
			else
				{
				ERR_PRINTF1(_L("Couldn't read PSK identity") );
				User::Leave(KErrGeneral);
				}
		}  
		  
	return theReturn;
	}
	
void CTlsStepBase::ReadUseNullCipher() 
	{
	 	TBool useNullCipher;
	if(GetBoolFromConfig(ConfigSection(), KUseNullCipher, useNullCipher))
		{
	 	iUseNullCipher = useNullCipher;
	 	}
	else
		{
		iUseNullCipher = EFalse;
		}
	}
	
TInt CTlsStepBase::ReadGetSessionDelayL() 
	{
	TInt sessionDelay;
	
	if(GetIntFromConfig(ConfigSection(), KSessionDelay, sessionDelay))
		{
	 	return sessionDelay;
	 	}
	else
		{
		ERR_PRINTF1(_L("Couldn't read session delay INI value") );
		User::Leave(KErrNotFound);
		}
	 // Keeps compiler happy, will not hit.
	return 0;
	}

	HBufC8* CTlsStepBase::StringToHexLC(const TDes8 &aString)
	/**
	 * Function to convert the contents of a TDes8 into a Binary format
	 *
	 * @param  - cosnt TDes8 aString: String to convert into Hexadecimal
	 * @return - HBufC8*: Converted Binary string representation
	 **/
	{
 	HBufC8* parsedString = HBufC8::NewLC(aString.Length()/2);

    TBuf8<1> binChar;
	_LIT8(KFormatBinary,"%c"); 
	
 	TPtr8 ptr(parsedString->Des());
	
  	for(TInt i = 0; i<aString.Length()/2 ; i++)
    	{
    	TPtrC8 tempPtr(aString.Mid(i*2,2));
    	TLex8 lex(tempPtr);
    	TUint val=0;
    	lex.Val(val, EHex);
    	binChar.Format(KFormatBinary,val);
    	 ptr.Append(binChar);
             	
       	}   
      	
        
	return parsedString;
	}

	
	
	
    TBool CTlsStepBase::GetKeyFromConfigL(const TDesC& aSectName, const TDesC& aIniValueName, TPtrC8 & aResult)
	{
	
	
	TBool theReturn(ETrue);
	// The buffer with key read form ini file as a string converted to a buffer of hex.

	TPtrC iniString; 
	if(GetStringFromConfig(aSectName, aIniValueName, iniString))
		{
		
					
		TInt stringLength(iniString.Length());
		// Assumes that keys must be formed by Hex pairs (full bytes)
		// example:  ABC1  is acceptable.  ABC   is not acceptable
		if( stringLength % 2 > 0)
			{
			// not pairs.
			theReturn = EFalse;
			}
		// Pairs
		else
			{
		 		
			HBufC8* keyBuf = HBufC8::New(iniString.Length());
			TPtr8 ptrKey = keyBuf->Des();
			ptrKey.Copy(iniString);
			
		  	HBufC8* key = StringToHexLC(keyBuf->Des());
			delete keyBuf;
	 		aResult.Set(key->Des());
	 		CleanupStack::PopAndDestroy(key);
 			
			}
				
		}

 	return theReturn;		
	}
 
TInt CTlsStepBase::SessionServerCertificate(CX509Certificate*& aCertOut) 
	{
	iActive->iStatus = KRequestPending;
	iSession->ServerCertificate(aCertOut, iActive->iStatus );
	iActive->Start();
	return iActive->iStatus.Int();
		
	}

TInt CTlsStepBase::SessionServerCertificateWithCancel(CX509Certificate*& aCertOut) 
	{
	iActive->iStatus = KRequestPending;
	iSession->ServerCertificate(aCertOut, iActive->iStatus );
	iSession->CancelRequest();
 	iActive->Start();
	return iActive->iStatus.Int();
	}

void CTlsStepBase::StandardAttrInit( CTlsCryptoAttributes* tlsCryptoAttributes)
	{
	tlsCryptoAttributes->iClientAuthenticate = EFalse;
	tlsCryptoAttributes->iDialogNonAttendedMode = ETrue;
	
	tlsCryptoAttributes->iSessionNameAndID.iServerName.iAddress.Copy( KNServer1 );
	tlsCryptoAttributes->iSessionNameAndID.iServerName.iPort = 10;
	tlsCryptoAttributes->iSessionNameAndID.iSessionId.Append( KSessionId1 );
		
	tlsCryptoAttributes->iCompressionMethod = ENullCompression;
	tlsCryptoAttributes->iCurrentCipherSuite.iHiByte = 0;
	tlsCryptoAttributes->iCurrentCipherSuite.iLoByte = 3;
			
	tlsCryptoAttributes->iNegotiatedProtocol.iMajor = 3;
	tlsCryptoAttributes->iNegotiatedProtocol.iMinor = 1; 
	
	tlsCryptoAttributes->iProposedProtocol.iMajor = 3;
	tlsCryptoAttributes->iProposedProtocol.iMinor = 1; 
		
	tlsCryptoAttributes->iPublicKeyParams->iKeyType = ERsa;
	tlsCryptoAttributes->iClientAuthenticate = EFalse; 
	tlsCryptoAttributes->iDialogNonAttendedMode = ETrue;
	}

void CTlsStepBase::DeleteSecureDialogFilesL()
	{
	RFs fs;
	User::LeaveIfError(fs.Connect());
	CleanupClosePushL(fs);
	
	CFileMan* fileMan = CFileMan::NewL(fs);
	CleanupStack::PushL(fileMan);
	
	TDriveUnit sysDrive (RFs::GetSystemDrive());
	TDriveName sysDriveName (sysDrive.Name());
	
	TBuf<128> fileName (sysDriveName);
	fileName.Append(KInputFile);
	TInt err = fileMan->Delete(fileName);
	if ( err != KErrNotFound && err != KErrNone )
		{
		User::LeaveIfError(err);
		}
		
	fileName.Copy(sysDriveName);
	fileName.Append(KOutputFile);	
	err = fileMan->Delete(fileName);
	if (err != KErrNotFound && err != KErrNone )
		{
		User::LeaveIfError(err);
		}
	CleanupStack::PopAndDestroy(2, &fs);// and fileMan
	}

void CTlsStepBase::SetDialogRecordL(RFileWriteStream& aStream, TSecurityDialogOperation aOp, const TDesC& aLabelSpec,
											 const TDesC& aResponse1, const TDesC& aResponse2)
	{
	MStreamBuf* streamBuf = aStream.Sink();
	streamBuf->SeekL(MStreamBuf::EWrite, EStreamEnd);
	aStream.WriteInt32L(aOp);
	aStream.WriteInt32L(aLabelSpec.Length());
	aStream.WriteL(aLabelSpec);
	aStream.WriteInt32L(aResponse1.Length());
	aStream.WriteL(aResponse1);
	aStream.WriteInt32L(aResponse2.Length());
	aStream.WriteL(aResponse2);
	}



