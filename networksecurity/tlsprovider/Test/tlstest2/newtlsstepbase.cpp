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
 @file newtlsstepbase.cpp
 @internalTechnology
*/
#include "newtlsstepbase.h"
#include "psuedorandom.h"
#include <tlsprovinterface.h>

CNewTlsStepBase::~CNewTlsStepBase()
	{
	// values populated at preamble
 	delete iActive;
	delete iSched;
	delete iServerRandom;
	delete iClientRandom;
	delete iPskKey;
	delete iPskIdentity;
	
	// values populated at test time. need to add appropiate destruction for OOM testing
	delete iClientMacSecret;
	delete iServerMacSecret;

	delete iClientWriteSecret;
	delete iServerWriteSecret;

	delete iClientInitVector;
	delete iServerInitVector;
	}
	
TVerdict CNewTlsStepBase::doTestStepPreambleL()
	{
 	ConstructL();
	
	GetBoolFromConfig(ConfigSection(), _L("OOMCondition"),iOOMCondition);
	GetBoolFromConfig(ConfigSection(), _L("OOMAllowNonMemoryErrors"),iOOMAllowNonMemoryErrors);
		
	// Reads PSK values if included in INI file.
	ReadPskToBeUsedL();
	
	if(!UsePsk())
		{
		// Read Diffie Hellman parameters only if not intending to use PSKs.
	    TRAPD(err,ReadDHParamsL());
	    if(err == KErrNone)
		    {
		    iUseDHParams = ETrue; 	
		    }
		 else
			{
		    iUseDHParams = EFalse; 	
		    }
		// read server certificate form INI file.
		iServerCertificate = ServerCertificateL();
		
		// Reads server private key from INI file.
		iServerPrivateKey = ServerPrivateKeyL();
		}
	
	// Reads if NULL ciphers suites are to be allowed from INI file.
	ReadUseNullCipher();
	
	// Reads server random from INI file.
	iServerRandom = ServerRandomL();
	
	// Reads client random from INI file.
	iClientRandom = ClientRandomL();
	
	// Reads cipher Suites from INI file.
	iCipherSuite = CipherSuiteL();
	
	// Reads protocol version from INI file.
	iProtocolVersion = ProtocolVersionL(); 
	
	// Gets session ID from INI file.
	iSessionId = SessionId();
	
	// Gets domain name.
	iDomainName.Set(DomainNameL());
	
	// key material.
	iClientMacSecret = NULL;
	iServerMacSecret = NULL;
    iClientWriteSecret = NULL;
    iServerWriteSecret = NULL;
    iClientInitVector = NULL;
    iServerInitVector = NULL;
	
	return EPass;
	}
	
TVerdict CNewTlsStepBase::doTestStepL()
	{
 	if (!iOOMCondition)
		{
		 doTestL(); 
		}
	else
		{
 		return doOOMTestL();
	    }	
   	return TestStepResult();
	}

TVerdict CNewTlsStepBase::doOOMTestL()
	{
	TVerdict verdict = EFail;
	TInt countAfter = 0;
	TInt countBefore = 0;
	for (TInt oomCount = 0; ; oomCount++)
		{
		INFO_PRINTF2(_L("==== Number of memory allocations %d ===="), oomCount);

		verdict = EFail;
		__UHEAP_RESET;
		__UHEAP_SETFAIL(RHeap::EDeterministic, oomCount);
		countBefore = User::CountAllocCells();

//		__UHEAP_MARK; // debug.

		TRAPD(error, doTestL());// ----> This is the actual test that runs under OOM conditions.

		countAfter = User::CountAllocCells();

		if(countBefore != countAfter)
			{
			INFO_PRINTF3(_L("Heap alloc count: %d final vs %d initial"), countAfter,countBefore);
			}  

//		__UHEAP_MARKEND; // debug	
		__UHEAP_RESET;

		if(error == KErrNone)  // First posibility: Test sequence was able to run to completion
			{
			verdict = EPass;
			INFO_PRINTF1(_L("OOM Test sequence completed"));

			if(countBefore != countAfter)
				{
				// Memory has to balance.
				verdict = EFail;
				INFO_PRINTF1(_L("Memory did not balance. Test outcome : Fail"));
				break;
				}  
			break;
			} 
		// Second possibility: Test sequence was NOT completed and error other than KErrNoMemory was returned. 
		else if (error != KErrNoMemory)  
			{
			if(iOOMAllowNonMemoryErrors == EFalse)
				{
				INFO_PRINTF1(_L("Test outcome : Fail"));
				INFO_PRINTF2(_L("Non KErrNoMemory returned : %d"), error);
				verdict = EFail;
				break;	
				}

			if(countBefore != countAfter)
				{
				// For any error returned memory has to balance.
				verdict = EFail;
				INFO_PRINTF1(_L("Memory did not balance. Test outcome : Fail"));
				break;
				}  
			}
		// Third possibility: Test sequence was NOT completed and error KErrNoMemory was returned. 
		else   
			{
			if (countBefore != countAfter)
				{
				verdict = EFail;
				INFO_PRINTF2(_L("OOM Status %d"),error);
				INFO_PRINTF2(_L("MEMORY DID NOT BALANCE!!. OOM Failed at %d"), oomCount);
				break;  
				}   
			}  
		INFO_PRINTF2(_L("OOM Failed Point status %d"), error);
		}  // End of for loop.
	SetTestStepResult(verdict);
	return verdict;	
	}
	
void CNewTlsStepBase::doTestL()
	{
	ASSERT(EFail);	
	}
	
void CNewTlsStepBase::ConstructL()
	{
	iSched = new(ELeave) CActiveScheduler;
	CActiveScheduler::Install(iSched);
	iActive = new (ELeave) CGenericActive;
	}
	
TInt CNewTlsStepBase::LeanVerifyServerCertificate(CX509Certificate*& aCertOut, HBufC8* aCertIn)
	{
  	iProvider->VerifyServerCertificate(aCertIn->Des(), aCertOut, iActive->iStatus);
 	iActive->Start();
 	return iActive->iStatus.Int();   
	}
	

TInt CNewTlsStepBase::LeanCreateSession()
	{

	TRAPD(error,iProvider->CreateL(iSession, iActive->iStatus));
	if(error)
		{
		return error;	
		}
	iActive->Start();
	return iActive->iStatus.Int();
	}
	
TInt CNewTlsStepBase::LeanClientKeyExchange(HBufC8*& aMessageOut)
	{
	iSession->ClientKeyExchange(aMessageOut, iActive->iStatus);
	iActive->Start();
	return iActive->iStatus.Int();
	}
	
HBufC8* CNewTlsStepBase::LeanDerivePreMasterSecretL(const TDesC8& aClientKeyExMessage,CDecPKCS8Data* aServerKeyData )
	{
	// Look up the cipher suite we used for this test
	CTlsCryptoAttributes* atts = iProvider->Attributes();
	
	TInt index = LeanCipherSuiteIndex(atts->iCurrentCipherSuite);
	if (index < 0)
		{
		INFO_PRINTF3(_L("Failed! Could not find cipher 0x%02x.0x%02x, did the ECOM plugin load okay?"),
			  atts->iCurrentCipherSuite.iHiByte, atts->iCurrentCipherSuite.iLoByte);
		User::Leave(KErrNotFound);
		}

	HBufC8* ret = NULL;

	// currently, we only support three key exhange algorithms, so that is what we test here
	switch (iSuites[index].CipherDetails()->iKeyExAlg)
		{
	case ERsa:
		// decrypt the key exhange message with the "server" private key, and
		// verify the version and that the premaster key is the right size...
		{
		CDecPKCS8Data* keyData = aServerKeyData;
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
		
		RInteger prime = RInteger::NewL(Prime());
		CleanupClosePushL(prime);
		
		RInteger gen = RInteger::NewL(Generator());
		CleanupClosePushL(gen);
		
		CDHPublicKey* clientKey = CDHPublicKey::NewL(prime, gen, clientX);
		CleanupStack::Pop(3, &clientX); // prime, gen, adopted by clientKey
		CleanupStack::PushL(clientKey);
		
		CDH* dh = CDH::NewLC(KeyPair()->PrivateKey());
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
		
		ret = HBufC8::NewLC(PskKey()->Length()*2 + 4 );
		ret->Des().FillZ(PskKey()->Length()*2 + 4);
 		TPtr8 ptr = ret->Des();
		
		// Populates first two field bytes values. 
		ptr[0] = (PskKey()->Length() & 0xFF00 ) >> 8;
		ptr[1] = PskKey()->Length() & 0xFF; 
		
		// Populates second two field bytes values. 
		ptr[PskKey()->Length() + 2] = ptr[0];
		ptr[PskKey()->Length() + 3] = ptr[1];
		
		// Populates the actual key value.
		ptr.Replace(PskKey()->Length() + 4, PskKey()->Length(), PskKey()->Des() );
		
		CleanupStack::Pop(ret);
		
		}
		break;

	default:
		User::Leave(KErrUnknown);
		break;
		}
		
	return ret;
	}


TInt CNewTlsStepBase::LeanCipherSuiteIndex(const TTLSCipherSuite& aSuite)
	{
	for (TInt i = 0 ; i < iSuites.Count() ; ++i)
		{
		if (iSuites[i] == aSuite)
			{
			return i;
			};
		}
	return KErrNotFound;
	}
	

HBufC8* CNewTlsStepBase::LeanComputeMasterSecretL(const TDesC8& aPremasterSecret)
	{
	if (iProvider->Attributes()->iNegotiatedProtocol == KSSL3_0)
		{
		return LeanComputeSslMasterSecretL(aPremasterSecret);
		}
	else if (iProvider->Attributes()->iNegotiatedProtocol == KTLS1_0)
		{
		return LeanComputeTlsMasterSecretL(aPremasterSecret);
		}
	else
		{
		// currently unknown protocol!
		User::Leave(KErrUnknown);
		return NULL; // keep the compiler happy..
		}
	}
	
HBufC8* CNewTlsStepBase::LeanComputeTlsMasterSecretL(const TDesC8& aPremasterSecret)
	{
	HBufC8* random = HBufC8::NewLC(KTLSServerClientRandomLen * 2);
	
	TTLSMasterSecretInput params = iProvider->Attributes()->iMasterSecretInput;
	random->Des().Append(params.iClientRandom);
	random->Des().Append(params.iServerRandom);
	
	_LIT8(KMasterSecretLabel, "master secret");
	HBufC8* ret = CTls10PsuedoRandom::PseudoRandomL(aPremasterSecret, KMasterSecretLabel, 
		*random, KTLSMasterSecretLen);
	CleanupStack::PushL(ret);
	
	LeanComputeTlsCipherKeysL(*ret, *random);
	
	CleanupStack::Pop(ret);
	CleanupStack::PopAndDestroy(random);
	return ret;
	}
	
HBufC8* CNewTlsStepBase::LeanComputeSslMasterSecretL(const TDesC8& aPremasterSecret)
	{
	HBufC8* random = HBufC8::NewLC(KTLSServerClientRandomLen * 2);
	
	TTLSMasterSecretInput params = iProvider->Attributes()->iMasterSecretInput;
	random->Des().Append(params.iClientRandom);
	random->Des().Append(params.iServerRandom);
	
	HBufC8* ret = CSsl30PsuedoRandom::PseudoRandomL(aPremasterSecret, *random, KTLSMasterSecretLen);
	CleanupStack::PushL(ret);
	
	LeanComputeSslCipherKeysL(*ret, *random);
	
	CleanupStack::Pop(ret);
	CleanupStack::PopAndDestroy(random);
	return ret;
	}
	
void CNewTlsStepBase::LeanComputeTlsCipherKeysL(const TDesC8& aMasterSecret, const TDesC8& aRandom)
	{
	// Look up the cipher suite we used for this test
	CTlsCryptoAttributes* atts = iProvider->Attributes();
	
	TInt index = LeanCipherSuiteIndex(atts->iCurrentCipherSuite);
	User::LeaveIfError(index);
	
	TTLSCipherSuite suite = iSuites[index];
	
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
	
	
void CNewTlsStepBase::LeanComputeSslCipherKeysL(const TDesC8& aMasterSecret, const TDesC8& aRandom)
	{
	// Look up the cipher suite we used for this test
	CTlsCryptoAttributes* atts = iProvider->Attributes();
	
	TInt index = LeanCipherSuiteIndex(atts->iCurrentCipherSuite);
	User::LeaveIfError(index);
	
	TTLSCipherSuite suite = iSuites[index];
	
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


TInt CNewTlsStepBase::LeanGetCipherSuitesL()
	{
	iSuites.Reset();
	iProvider->CipherSuitesL(iSuites, iActive->iStatus);
	iActive->Start();
	return iActive->iStatus.Int();
	}



