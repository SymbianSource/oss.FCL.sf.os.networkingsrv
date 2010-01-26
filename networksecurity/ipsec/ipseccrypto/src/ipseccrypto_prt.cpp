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

#include <e32cmn.h>

#include "ipseccrypto.h"
#include <networking/ipsecerr.h>

#include <des.h>
#include <3des.h>
#include <rijndael.h>
#include <hash.h>

#include "keys.h"
#include "cryptospidef.h"
#include "cryptosymmetriccipherapi.h"
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
#include "ruleselector.h"
#include "plugincharacteristics.h"
#include "cryptospistateapi.h"
#include "cryptoparams.h"
#include "cryptomacapi.h"
using namespace CryptoSpi;
#endif // SYMBIAN_IPSEC_VOIP_SUPPORT

#define DES_CBLOCK_SIZE 8
#define AES_CBLOCK_SIZE 16


#define SHA_DIGEST_LENGTH 20
#define MD5_DIGEST_LENGTH 16
#define SHA_CBLOCK  64
#define MD5_CBLOCK  64

//

class CMessageDigestSymbian : public CMessageDigestCrypto
	/**
	* Generic Hash engine wrapper
	*/
	{
public:
	CMessageDigestSymbian(CMessageDigest *aDigest);
	virtual void Init();
	virtual void Update(const TDesC8& aMessage);
	virtual void Final(TDes8& aDigest);
	static void FillinInfoSha1(TAlgorithmDesc &anEntry);
	static void FillinInfoMd5(TAlgorithmDesc &anEntry);
	~CMessageDigestSymbian();
private:
	CMessageDigest *iDigest;
	};

CMessageDigestSymbian::CMessageDigestSymbian(CMessageDigest *aDigest) : iDigest(aDigest)
	{
	ASSERT(aDigest);
	}

void CMessageDigestSymbian::Init()
	{
	ASSERT(iDigest);
	iDigest->Reset();
	}
	
void CMessageDigestSymbian::Update(const TDesC8& aMessage)
	{
	ASSERT(iDigest);
	iDigest->Update(aMessage);
	}
	
void CMessageDigestSymbian::Final(TDes8& aDigest)
	{
	ASSERT(iDigest);
	aDigest.Copy(iDigest->Final());
	}
	
CMessageDigestSymbian::~CMessageDigestSymbian()
	{
	delete iDigest;
	}


//

//  CProtocolEay::AlgorithmList + algorithm specific FillinInfo's
//
//      return descriptions of the algorithms supported by this module
//
static void FillinInfoDescbc(TAlgorithmDesc &anEntry)
	{
	anEntry.iName = KIpsecName_DES_CBC;
	anEntry.iAlgType = EAlgorithmClass_Cipher;
	anEntry.iMinBits = 64;
	anEntry.iMaxBits = 64;
	anEntry.iBlock = DES_CBLOCK_SIZE;
	anEntry.iVector = 8;
	}

static void FillinInfo3Descbc(TAlgorithmDesc &anEntry)
	{
	anEntry.iName = KIpsecName_3DES_CBC;
	anEntry.iAlgType = EAlgorithmClass_Cipher;
	anEntry.iMinBits = 3*64;
	anEntry.iMaxBits = 3*64;
	anEntry.iBlock = DES_CBLOCK_SIZE;
	anEntry.iVector = 8;
	}


static void FillinInfoAescbc(TAlgorithmDesc &anEntry)
	{
	anEntry.iName = KIpsecName_AES_CBC;
	anEntry.iAlgType = EAlgorithmClass_Cipher;
	anEntry.iMinBits = 128;
	anEntry.iMaxBits = 256;
	anEntry.iBlock = AES_CBLOCK_SIZE;
	anEntry.iVector = 16;
	}

static void FillinInfoAesctr(TAlgorithmDesc &anEntry)
	{
	anEntry.iName = KIpsecName_AES_CTR;
	anEntry.iAlgType = EAlgorithmClass_Cipher;
	anEntry.iMinBits = 128;
	anEntry.iMaxBits = 256;
	anEntry.iBlock = AES_CBLOCK_SIZE;
	anEntry.iVector = 16; // NOTE : Value set to 16 to be compliant to the current crypto implementation 
					      // which expects the IVSize to be 16. Will have to be updated (to 8) once the fix is 
					      // completed at the crypto layer
	}
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
static void FillinInfoAesXcbcMac96(TAlgorithmDesc &anEntry)
	{
	anEntry.iName = KIpsecName_AES_XCBC_MAC;
	anEntry.iAlgType = EAlgorithmClass_Mac;
	anEntry.iMinBits = 96;
	anEntry.iMaxBits = 128;
	anEntry.iBlock = AES_CBLOCK_SIZE;
	anEntry.iVector = 16; 
	}
#endif

void CMessageDigestSymbian::FillinInfoSha1(TAlgorithmDesc &anEntry)
	{
	anEntry.iName = KIpsecName_SHA1;
	anEntry.iAlgType = EAlgorithmClass_Digest;
	anEntry.iMinBits = SHA_DIGEST_LENGTH * 8;
	anEntry.iMaxBits = SHA_DIGEST_LENGTH * 8;
	anEntry.iBlock = SHA_CBLOCK;
	anEntry.iVector = SHA_DIGEST_LENGTH;
	}

void CMessageDigestSymbian::FillinInfoMd5(TAlgorithmDesc &anEntry)
	{
	anEntry.iName = KIpsecName_MD5;
	anEntry.iAlgType = EAlgorithmClass_Digest;
	anEntry.iMinBits = MD5_DIGEST_LENGTH * 8;
	anEntry.iMaxBits = MD5_DIGEST_LENGTH * 8;
	anEntry.iBlock = MD5_CBLOCK;
	anEntry.iVector = MD5_DIGEST_LENGTH;
	}


TUint CProtocolEay::AlgorithmList(TAlgorithmDesc *&aList)
	{
	aList = new TAlgorithmDesc[EAlgorithm_Max];
	if (aList == NULL)
		{
		return EAlgorithm_Max;
		}

	CMessageDigestSymbian::FillinInfoSha1(aList[EAlgorithm_Sha1]);
	CMessageDigestSymbian::FillinInfoMd5(aList[EAlgorithm_Md5]);
	FillinInfoDescbc(aList[EAlgorithm_Descbc]);
	FillinInfo3Descbc(aList[EAlgorithm_3Descbc]);
	FillinInfoAescbc(aList[EAlgorithm_Aescbc]);
	FillinInfoAesctr(aList[EAlgorithm_Aesctr]);
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
	FillinInfoAesXcbcMac96(aList[EAlgorithm_AesXcbcMac96]);
#endif
	return EAlgorithm_Max;
	}

//
//  Instantiate a Cipher algorithm
//
CryptoSpi::CSymmetricCipher* CProtocolEay::SymmetricCipherL(TUint aAlg, const TDesC8 &aKey)
	{
	CryptoSpi::CSymmetricCipher *enc = NULL;
	TUid operModeUid = CryptoSpi::KOperationModeCBCUid;
	TUid algUid = CryptoSpi::KAesUid;
	TUid cryptoModeUid = CryptoSpi::KCryptoModeEncryptUid;
	TUid paddingModeUid = CryptoSpi::KPaddingModeNoneUid;
	switch(aAlg)
	{
		case EAlgorithm_Descbc:
			algUid = CryptoSpi::KDesUid;
			break;
		case EAlgorithm_3Descbc:
			algUid = CryptoSpi::K3DesUid;
			break;
		case EAlgorithm_Aescbc:
			algUid = CryptoSpi::KAesUid;
			break;
		case EAlgorithm_Aesctr:
			algUid = CryptoSpi::KAesUid;
			operModeUid = CryptoSpi::KOperationModeCTRUid;;
			break;
		default:
			User::Leave(EIpsec_UnknownCipherNumber);	
	}
	// Initialize the Encryptor
	CryptoSpi::TKeyProperty keyProperty = {algUid, KNullUid, CryptoSpi::KSymmetricKeyUid, CryptoSpi::KNonEmbeddedKeyUid};
	CryptoSpi::CCryptoParams* keyParam =CryptoSpi::CCryptoParams::NewLC();
	keyParam->AddL(aKey, CryptoSpi::KSymmetricKeyParameterUid);
	CryptoSpi::CKey *key=CryptoSpi::CKey::NewLC(keyProperty, *keyParam);
	TRAPD(res, CryptoSpi::CSymmetricCipherFactory::CreateSymmetricCipherL
				(
				enc, 
				algUid, 
				*key, 
				cryptoModeUid, 
				operModeUid, 
				paddingModeUid, 
				NULL));	
	CleanupStack::PopAndDestroy(key);
	CleanupStack::PopAndDestroy(keyParam);
	if (res != KErrNone) 
		{
		// SetKey takes place implicit to the CreateSymmetricCipherL() factory call. If there is any issue with the key
		// we would get KErrNotSupported which we need to map to EIpsec_BadCipherKey
		if (res == KErrNotSupported)
			{
			User::Leave(EIpsec_BadCipherKey);
			}
		User::Leave(res);
		}		
	return enc;
	}

//
//  CProtocolEay::MessageDigest
//      Instantiate a Message Digest algorithm
//
CMessageDigestCrypto* CProtocolEay::MessageDigest(TUint aAlg)
	{
	CMessageDigest *digest = NULL;
	// Instead of CSHA1 and CMD5, we should have TSHA1 and TMD5
	// to be used as member classes, and the TRAP below would
	// not be needed -- or at least, should have non-leaving
	// New available!
	TRAP_IGNORE(
		switch (aAlg)
			{
		case EAlgorithm_Sha1:
			digest = CSHA1::NewL();
			break;
		case EAlgorithm_Md5:
			digest = CMD5::NewL();
			break;
		default:
			break;
			}
		);
	if (digest == NULL)
		return NULL;	// No Engine available: out of memory or bad call.
	CMessageDigestSymbian *wrapper = new CMessageDigestSymbian(digest);
	if (wrapper == NULL)
		delete digest;	// Out of memory
	return wrapper;
	}

#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
CMac* CProtocolEay::GetMacImplementationL(const TDesC8& aKey)
	{
	//create rule based selector for filtering the desired MAC interface plug-ins.
	CSelectionRules* rules = CSelectionRules::NewL();
	CleanupStack::PushL(rules);
	CSelectionRuleContent* rule = NULL;
	const long i=0;
	CCryptoParam* ruleValueParam =  CCryptoIntParam::NewL(0, KMacModeTypeUid);
	
	TUid id={KAlgorithmCipherAesXcbcMac96};
	 rule = CSelectionRuleContent::NewL(KMacInterfaceUid,id, ruleValueParam,EOpEqual,ETrue);
	rules->AddSelectionRuleL(rule);
	CRuleSelector* ruleSelector = CRuleSelector::NewL(rules);
	CleanupStack::Pop(rules);
	CleanupStack::PushL(ruleSelector);
	CCryptoSpiStateApi::SetSelector(ruleSelector);
	// Create & Set the key
	TKeyProperty keyProperty = {KAesUid, KNullUid, KSymmetricKey,KNonEmbeddedKeyUid};
	CCryptoParams* keyParam =CCryptoParams::NewLC();
	keyParam->AddL(aKey, KSymmetricKeyParameterUid);
	CKey* uniKey=CKey::NewLC(keyProperty, *keyParam);
	//Retrieve a Synchronous MAC Factory Object and use AES-XCBC-MAC-96 or any other MACalgorithm.
	CMac* macImpl =NULL;
	TRAPD(err,CMacFactory::CreateMacL(macImpl, KAesXcbcMac96Uid,*uniKey, NULL));
	//cleanup uniKey and keyParam.
	CleanupStack::PopAndDestroy(2,keyParam);
	CCryptoSpiStateApi::UnsetSelector();
	CleanupStack::PopAndDestroy(ruleSelector);
	return macImpl;
	}

HBufC8* CProtocolEay::RetrieveMacValueL(CMac *aMac, const TDesC8& aSourceData )
	{

	//Retrieve the 8bit mac value
	HBufC8* macValue = HBufC8::NewLC(16);
	TPtr8 macPtr = macValue->Des();
	//Copy the mac content into the heap based descriptor
	macPtr.Copy(aMac->MacL(aSourceData));
	CleanupStack::PopAndDestroy(macValue);
	return  macValue;
	}
#endif // SYMBIAN_IPSEC_VOIP_SUPPORT
