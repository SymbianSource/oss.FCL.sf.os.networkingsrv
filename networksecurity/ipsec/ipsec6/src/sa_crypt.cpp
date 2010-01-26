// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// sa_crypt.cpp - IPv6/IPv4 IPSEC interface to crypto libraries
// IPsec interface to cryptographic libraries -- the implementation.
//



/**
 @file sa_crypt.cpp
*/
#include "ext_hdr.h"
#include <networking/pfkeyv2.h>
#include "sa_spec.h"
#include "sa_crypt.h"
#include <networking/ipsecerr.h>
#include "ipseccrypto.h"
#include "keys.h"
#include "cryptospidef.h"
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
#include "cryptomacapi.h"
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT

class TLibraryPtr
	/**
	* Description of installed cryptographic library.
	*/
	{
public:
	TLibraryPtr() : iLibrary(NULL), iName(KNullDesC), iAlgs(NULL), iNum(0) {}
	TLibraryPtr(CProtocolCrypto* aLibrary, const TDesC &aName, TAlgorithmDesc *algs, TUint aNum)
		: iLibrary(aLibrary), iName(aName), iAlgs(algs), iNum(aNum) {}

	CProtocolCrypto* iLibrary;	//< Library (protocol) instance
	TProtocolName iName;		//< Symbolic name of the library
	TAlgorithmDesc *iAlgs;		//< Supported Algorithms descriptions
	TUint iNum;					//< Number of Algorithms
	};


class CLibraryList : public CArrayFixFlat<TLibraryPtr>
	/**
	* List of installed libraries.
	* Implemented as an array of TLibraryPtr, each of which
	* describes one cryptographi library.
	*/
	{
public:
	CLibraryList() : CArrayFixFlat<TLibraryPtr>(2) {}
	~CLibraryList();
	void AddL(CProtocolCrypto* aLibrary);
	TUint Lookup(const TAlgorithmMap &aMap, TLibraryPtr **aLib);
	};


void CLibraryList::AddL(CProtocolCrypto* aLibrary)
	/**
	* Add a cryptographic library to the list.
	*
	* Find out the list of algoritms supported by this library (AlgorithmList()), and
	* if there are some, then add this library to the list of available libaries.
	*
	* @param aLibrary The crypto protocol.
	* @leave error if library cannot be added.
	*/
	{
	TAlgorithmDesc *algs = NULL;
	TServerProtocolDesc desc;

	aLibrary->Identify(&desc);
	TUint num = aLibrary->AlgorithmList(algs);
	//coverity[leave_without_push]
	LOG(Log::Printf(_L("Registering crypto library: %S with %d algorithms"), &desc.iName, num));
	if (algs)
		{
		CleanupStack::PushL(algs);
		TLibraryPtr ptr(aLibrary, desc.iName, algs, num);
		AppendL(ptr);
		CleanupStack::Pop();
		aLibrary->Open();
#ifdef _LOG
		for (TInt i = 0; i < num; ++i)
			{
			const TAlgorithmDesc &a = algs[i];
			if (a.iAlgType == EAlgorithmClass_Digest)
				{
				Log::Printf(_L("\tDigest %S block=%d"), &a.iName, a.iBlock);
				}
			else if (a.iAlgType == EAlgorithmClass_Cipher)
				{
				Log::Printf(_L("\tCipher %S block=%d, IV=%d key min=%d max=%d"),
					&a.iName, a.iBlock, a.iVector, a.iMinBits, a.iMaxBits);
				}
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT				
			else if (a.iAlgType == EAlgorithmClass_Mac)
				{
				Log::Printf(_L("\tMac %S block=%d, IV=%d key min=%d max=%d"),
					&a.iName, a.iBlock, a.iVector, a.iMinBits, a.iMaxBits);
				
				}
#endif // SYMBIAN_IPSEC_VOIP_SUPPORT
		 	else 
		 		{
		 		Log::Printf(_L("\tUnknown %S block=%d, IV=%d key min=%d max=%d"),
					&a.iName, a.iBlock, a.iVector, a.iMinBits, a.iMaxBits);	
		 		}				
			}
#endif
		}
	}

CLibraryList::~CLibraryList()
	/**
	* Desctructor.
	*
	* Resease resources and Close() the crypto protocol modules.
	*/
	{
	TInt i, n;
	n = Count();
	TLibraryPtr *lib;

	for (i = 0; i < n; i++)
		{
		lib = &operator[](i);
		delete[] lib->iAlgs;
		if (lib->iLibrary)
			lib->iLibrary->Close();
		}
	}


TUint CLibraryList::Lookup(const TAlgorithmMap &aMap, TLibraryPtr **aLib)
	{
	/**
	* Lookup a library that implements the specified algorithm.
	*
	* If the map entry specifies a specific library, then only that
	* library is examined for a match of algorithm name.
	*
	* The algorithm search is based on algorithm name (not on number).
	*
	* @param aMap The algorithm to find.
	* @retval The library that implements it.
	*/
	TInt i, n;
	n = Count();
	TLibraryPtr *lib;

	for (i = 0; i < n; i++)
		{
		lib = &operator[](i);
		if (aMap.iLibrary.Length() == 0 ||
			aMap.iLibrary == lib->iName)
			{
			//
			// Search Algorithms within this library
			//
			for (TUint j = 0; j < lib->iNum; ++j)
				{
				if (lib->iAlgs[j].iName == aMap.iAlgorithm)
					{
					*aLib = lib;
					return j;
					}
				}
			}
		}
	// Requested algorithm is not available! This is configuration
	// error, because Security Policy should not request algorithms
	// that are not installed.
	*aLib = NULL;
	return 0;
	}

CIpsecCryptoManager::CIpsecCryptoManager()
	/**
	* Constructor.
	*
	* Only called from CIpsecCryptoManager::NewL().
	*/
	{
	}

CIpsecCryptoManager* CIpsecCryptoManager::NewL()
    /**
    * Return instance of crypto library manager.
    *
    * @return Cryptographic libarary manager.
    */
    {
    CIpsecCryptoManager *self = new (ELeave) CIpsecCryptoManager();
    CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
    return self;
    }
	
void CIpsecCryptoManager::ConstructL()
	/**
	* Construction.
	*
	* The construction creates initial library list (empty) and algorithm map
	* (initialised with the default mapping). The default mapping uses the standard
	*  algorithm number symbols defined in the standard <networking/pfkeyv2.h> as follows:
	* @code
	* // Algorithm			Bits Library	Name 
	*	SADB_AALG_MD5HMAC,	96, KNullDesC, KIpsecName_MD5
	*	SADB_AALG_SHA1HMAC,	96, KNullDesC, KIpsecName_SHA1
	*	SADB_EALG_DESCBC,	64, KNullDesC, KIpsecName_DES_CBC
	*	SADB_EALG_3DESCBC,	64, KNullDesC, KIpsecName_3DES_CBC
	*	SADB_EALG_NULL	,	0,  KNullDesC, KNullDesC
	*	SADB_X_AALG_AES_XCBC_MAC, 128, KNullDesC, KIpsecName_AES_XCBC_MAC
	* @endcode
	*
	* @leave error if not enough memory.
	*/
    {
	iLibraryList = new (ELeave) CLibraryList;
	iAlgorithmList = new (ELeave) CAlgorithmList;
	
	// Default mapping IPSEC algorithm numbers to engines:
	// Should read these mappings from some ini file. For test purposes,
	// the following mappings are now hard coded (library = "" => use
	// whichever defines the algorithm first).
	iAlgorithmList->AddL(EAlgorithmClass_Digest, SADB_AALG_MD5HMAC 	,	96, KNullDesC, KIpsecName_MD5);
	iAlgorithmList->AddL(EAlgorithmClass_Digest, SADB_AALG_SHA1HMAC	,	96, KNullDesC, KIpsecName_SHA1);
	iAlgorithmList->AddL(EAlgorithmClass_Cipher, SADB_EALG_DESCBC  	,	64, KNullDesC, KIpsecName_DES_CBC);
	iAlgorithmList->AddL(EAlgorithmClass_Cipher, SADB_EALG_3DESCBC 	,	64, KNullDesC, KIpsecName_3DES_CBC);
	iAlgorithmList->AddL(EAlgorithmClass_Cipher, SADB_EALG_AESCBC	,	64, KNullDesC, KIpsecName_AES_CBC);
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
	iAlgorithmList->AddL(EAlgorithmClass_Mac, 	 SADB_AALG_AES_XCBC_MAC , 128,KNullDesC,KIpsecName_AES_XCBC_MAC );
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT
#ifdef SYMBIAN_CRYPTOSPI	
	iAlgorithmList->AddL(EAlgorithmClass_Cipher, SADB_EALG_AESCTR	,	64, KNullDesC, KIpsecName_AES_CTR);
#endif	//SYMBIAN_CRYPTOSPI 
	iAlgorithmList->AddL(EAlgorithmClass_Cipher, SADB_EALG_NULL    	,	 0,	KNullDesC, KNullDesC);

	}

CIpsecCryptoManager::~CIpsecCryptoManager()
	/**
	* Descructor.
	*
	* Delete algorithm mapping and library list.
	*/
	{
	delete iLibraryList;
	delete iAlgorithmList;
	}

void CIpsecCryptoManager::SetAlgorithms(CAlgorithmList*& aList)
	/**
	* Update the algorithm map with a new mapping table.
	*
	* Replace the current algorithm list with a the new list, take ownership
	* of the structure and place NULL into aList.
	*
	* Algorithm mapping cannot be set to NULL using this method.
	*
	* @param aList	The algorithm list (must be non-NULL)
	*/
	{
	if (aList)
		{
		// Replace old with new
		delete iAlgorithmList;
		iAlgorithmList = aList;
		aList = NULL;
		}
#ifdef _LOG
	if (iAlgorithmList)
		{
		const TInt N = iAlgorithmList->Count();
		for (TInt i = 0; i < N; ++i)
			{
			const TAlgorithmMap &map = (*iAlgorithmList)[i];
			_LIT(KEncr, "ENCR");
			_LIT(KAuth, "AUTH");
			Log::Printf(_L("\tAlgorithmMap %S %d (bits=%d) maps to %S.%S"),
				map.iClass == EAlgorithmClass_Digest ? &KAuth() : &KEncr(),
				map.iId, map.iBits,
				&map.iLibrary, &map.iAlgorithm);
			}
		}
	else
		{
		Log::Printf(_L("\tAlgorithmMap does not exist"));
		}
#endif
	}

void CIpsecCryptoManager::AddLibraryL(CProtocolCrypto *aLibrary)
	/**
	* Add a cryptographic library to the list.
	*
	* Add a cryptographic library packaged as a protocol into the
	* list of installed libraries. The CLibraryList::AddL() does
	* the major work.
	*
	* @param aLibrary The crypto protocol.
	*/
	{
	iLibraryList->AddL(aLibrary);
	}


CArrayFixFlat<struct sadb_alg> *CIpsecCryptoManager::SupportedAlgorithms
	(TInt &aNumAuth, TInt &aNumEncrypt)
	/**
	* Ask the supported algorithms of a library.
	*
	* This function has a very specialized use: return supported
	* algorithms, when a PFKEY reply to the REGISTER message is generated
	* (CProtocolKey::ExecRegister). The format of the return value is tailored
	* for that purpose.
	*
	* Return a dynamically alloated (heap) array of 'sadb_alg'
	* descriptions. The first aNumAuth descriptors are authentication
	* algorithms, and the tail aNumEncrypt are encryption
	* algorithms.
	*
	* @retval aNumAuth Count of authentication algorithms
	* @retval aNumEncrypt Count of cipher algorithms
	* return Algorithm descriptions
	*
	* The return is guaranteed to be non-NULL, if aNumAuth + aNumEncrypt > 0. 
	*/
	{
	CArrayFixFlat<struct sadb_alg> *algs;
	TInt n;

	aNumAuth = 0;
	aNumEncrypt = 0;

	if (iAlgorithmList == NULL ||
		iLibraryList == NULL ||
		(n = iAlgorithmList->Count()) == 0)
		return NULL;		// No algorithms supported!
	//
	// Allocate all the space that is needed
	//
	// Sets 'aNumEncrypt' in case the heap allocations
	// fail (a NULL return with non-zero count is used
	// to indicate Memory Allocation error).
	aNumEncrypt = n;
	algs = new CArrayFixFlat<struct sadb_alg>(n);
	if (!algs)
		return NULL;
	TRAPD(left, algs->ResizeL(n));
	if (left)
		{
		delete algs;
		return NULL;
		}
	aNumEncrypt = 0;
	//
	// -- All requred heap space has now been allocated --
	// (The space is contigous in memory)
	struct sadb_alg *auth = algs->Back(0);
	struct sadb_alg *encrypt = algs->End(0);

	for (TInt i = 0; i < n; ++i)
		{
		struct sadb_alg *alg;
		TAlgorithmMap &p = iAlgorithmList->At(i);
		if (p.iAlgorithm.Length() == 0 && p.iClass == EAlgorithmClass_Cipher )
			{
			// The NULL encryption is indicated by empty string in the
			// algorithm name. NULL encryption is supported, if the
			// mapping has such entry configured...
			alg = encrypt - ++aNumEncrypt;
			alg->sadb_alg_id = (TUint8)p.iId;
			alg->sadb_alg_ivlen = 0;
			alg->sadb_alg_minbits = 0;
			alg->sadb_alg_maxbits = 0;
			alg->sadb_alg_reserved = 0;
			continue;
			}
		TLibraryPtr *lib;
		TInt j = iLibraryList->Lookup(p, &lib);
		if (lib != NULL)
			{
			// A matching library instance and algorithm
			// located, fill in the algorithm description.
			if (p.iClass == EAlgorithmClass_Digest)
				{
				alg = auth + aNumAuth++;
				alg->sadb_alg_ivlen = 0;
				}
			else if (p.iClass == EAlgorithmClass_Cipher)
				{
				alg = encrypt - ++aNumEncrypt;
				alg->sadb_alg_ivlen = (TUint8)lib->iAlgs[j].iVector;
				}
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
			else if (p.iClass == EAlgorithmClass_Mac )	
				{	
				alg = auth + aNumAuth++;
				alg->sadb_alg_ivlen = 0;
				}
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT
			else 
				continue;	// Some unknown algorithm class, that
							// cannot be used by this implementation.
			alg->sadb_alg_id = (TUint8)p.iId;
			alg->sadb_alg_minbits = (TUint16)lib->iAlgs[j].iMinBits;
			alg->sadb_alg_maxbits = (TUint16)lib->iAlgs[j].iMaxBits;
			alg->sadb_alg_reserved = 0;
			}
		}
	return algs;
	}

class CAuthenticationHmac : public CAuthenticationBase
	/**
	* HMAC authentication.
	*
	* Implement the (RFC-2104) HMAC based keyed authentication as used by IPsec.
	*/
	{
	friend class CIpsecCryptoManager;
public:
	virtual void Init();
	virtual void Update(const TDesC8& aMessage);
	virtual const TDesC8 &Final(TInt aSize);
	virtual TInt Compare(const TDesC8& aDigest);
	virtual TInt BlockSize() const {return iBlockSize;}
	//
	// Return the number of bytes defined by the algorithm
	// map, not the real digest length of the algorithm.
	virtual TInt DigestSize() const {return (iBits + 7) / 8;}
protected:
    static CAuthenticationHmac* NewL(const TLibraryPtr &lib, TUint anIndex, const TDesC8 &aKey, TInt aBits);
    void ConstructL(const TLibraryPtr &lib, TUint anIndex, const TDesC8 &aKey, TInt aBits);
	virtual ~CAuthenticationHmac();
private:
	CAuthenticationHmac();
protected:
	HBufC8 *iHmac_ipad;		//< Precomputed HMAC input pad
	HBufC8 *iHmac_opad;		//< Precomputed HMAC output pad
	HBufC8 *iTemp;			//< Working space (at least native digest size)
	TInt iBlockSize;		//< The blocksize in bytes
	TInt iDigestSize;		//< The digest size in bytes
	TInt iBits;				//< Number of bits used from the digest.
	CMessageDigestCrypto *iDigest;//< The raw message digest engine
	};

	
CAuthenticationHmac::CAuthenticationHmac()
	/**
	* Constructor.
	*/
	{
	}

CAuthenticationHmac* CAuthenticationHmac::NewL(const TLibraryPtr &aLib, TUint anIndex, const TDesC8 &aKey, TInt aBits)
	/**
	* Create HMAC authentication engine.
	*
	* @param aLib The library to use for the digest engine
	* @param anIndex The digest algorithm number in library
	* @param aKey The authentication key
	* @param aBits The number of bits used from the digets.
	*/
	{
	CAuthenticationHmac *self = new (ELeave) CAuthenticationHmac();
	CleanupStack::PushL(self);
	self->ConstructL(aLib, anIndex, aKey, aBits);
	CleanupStack::Pop();        // self
	return self;
	}

void CAuthenticationHmac::ConstructL(const TLibraryPtr &aLib, TUint anIndex, const TDesC8 &aKey, TInt aBits)
	{
	/**
	* Setup HMAC processing using the specified digest.
	*
	* @param aLib The library to use for the digest engine
	* @param anIndex The digest algorithm number in library
	* @param aKey The authentication key
	* @param aBits The number of bits used from the digets.
	*/
	iDigest = aLib.iLibrary->MessageDigest(anIndex);
	iBlockSize = aLib.iAlgs[anIndex].iBlock;
	iDigestSize = aLib.iAlgs[anIndex].iVector;
	iBits = aBits;

	iHmac_ipad = HBufC8::NewMaxL(iBlockSize);
	iHmac_opad = HBufC8::NewMaxL(iBlockSize);
	iTemp = HBufC8::NewMaxL(iDigestSize);
	TPtr8 ipad(iHmac_ipad->Des());
	TPtr8 opad(iHmac_opad->Des());
	if (aKey.Length() > iBlockSize)
		{
		iDigest->Init();
		iDigest->Update(aKey);
		iDigest->Final(ipad);
		ipad.SetLength(iDigestSize);
		}
	else
		ipad = aKey;

	opad.SetLength(iBlockSize);
	int i;
	for (i = 0; i < ipad.Length(); ++i)
		{
		opad[i] = (TUint8)(ipad[i] ^ 0x5c);
		ipad[i] ^= 0x36;
		}
	ipad.SetLength(iBlockSize);
	for ( ;i < iBlockSize; ++i)
		{
		ipad[i] = 0x36;
		opad[i] = 0x5c;
		}
    }

	
CAuthenticationHmac::~CAuthenticationHmac()
	/**
	* Destructor.
	*
	* Clear the memory areas containing the key information, before
	* releasing the memory.
	*/
	{
	if (iHmac_ipad)
		{
		iHmac_ipad->Des().FillZ();
		delete iHmac_ipad;
		}
	if (iHmac_opad)
		{
		iHmac_opad->Des().FillZ();
		delete iHmac_opad;
		}
	if (iTemp)
		{
		iTemp->Des().FillZ();
		delete iTemp;
		}
	delete iDigest;
	}

void CAuthenticationHmac::Init()
	/**
	* Initialize authentication for a packet.
	*
	* Reset digest enginge and feed the input (HMAC) pad to it.
	*/
	{
	iDigest->Init();				// Initialize Digest engine
	iDigest->Update(*iHmac_ipad);	// Feed in the precomputed input pad
	};

void CAuthenticationHmac::Update(const TDesC8& aMessage)
	/**
	* Feed data to the digest.
	*
	* @param aMessage The data.
	*/
	{
	iDigest->Update(aMessage);		// Feed actual payload material
	}

const TDesC8 &CAuthenticationHmac::Final(TInt aSize)
	/**
	* Finish the digest computation and return ICV.
	*
	* @param aSize The length of the ICV (bytes).
	* @return The computed ICV
	*/
	{
	TPtr8 ptr = iTemp->Des();
	ptr.SetLength(iDigestSize);		// Ensure correct length! [also in *iTemp!]
	iDigest->Final(ptr);			// Get Current Digest Value
	iDigest->Init();				// Initialize Digest Engine
	iDigest->Update(*iHmac_opad);	// Feed in the precomputed output pad
	iDigest->Update(ptr);			// Merge with digest from the first phase
	iDigest->Final(ptr);			// and produce the final digest value.

	if (ptr.Length() > aSize)		// The caller may want a trucated value
		ptr.SetLength(aSize);		// [This changes also *iTemp length!]
	return *iTemp;
	}

TInt CAuthenticationHmac::Compare(const TDesC8 &aDigest)
	/**
	* Finish the digets computation and compare with ICV.
	*
	* @param aDigest The ICV to match
	* @return comparison result (= 0, match, != 0, no match).
	*/
	{
	TPtr8 ptr = iTemp->Des();
	ptr.SetLength(iDigestSize);		// Ensure correct length!
	iDigest->Final(ptr);			// Get Current Digest Value
	iDigest->Init();				// Initialize Digest Engine
	iDigest->Update(*iHmac_opad);	// Feed in the precomputed output pad
	iDigest->Update(ptr);			// Merge with digest from the first phase
	iDigest->Final(ptr);			// and produce the final digest value.
									// The caller may want a tructated value
	ptr.SetLength(aDigest.Length());
	return aDigest.Compare(ptr);
	}


class CEncryptionCipher : public CEncryptionBase
	/**
	* IPsec Cipher class using externally given algorithm.
	*
	* Acts as an intermediate between the IPsec and the raw external
	* cryptographic algorithm.
	*/
	{
	friend class CIpsecCryptoManager;
public:
	virtual void EncryptL(const TDesC8& anIV);	// Reset encryption engine
	virtual void DecryptL(const TDesC8& anIV);	// Reset encryption engine
	virtual void UpdateL(TDes8& aBuf, TDes8& aBufOut);	// Encrypt/decrypt a block
	virtual void UpdateFinalL(TDes8& aBuf, TDes8& aBufOut); // Encrypt/decrypt the last block
	virtual TInt BlockSize() const {return iBlockSize; }   // Fetch the block size of the algorithm under consideration
	virtual TInt GetOutputLength(TInt aInputLen);   // Fetch the encrypt/decrypt output length of given block from crypto
	virtual TInt GetFinalOutputLength(TInt aInputLen); // Fetch the encrypt/decrypt output length of final block from crypto
	virtual TInt IVSize() const {return iIVSize;}   // Fetch the IVSize of the algorithm under consideration
	virtual void Reset();   // Reset the encryptor/decryptor state at the crypto layer
protected:
	static CEncryptionCipher* NewL(const TLibraryPtr &aLib, TUint anIndex, const TDesC8 &aKey, TInt aBits);
	void ConstructL(const TLibraryPtr &aLib, TUint anIndex, const TDesC8 &aKey, TInt aBits);
	virtual ~CEncryptionCipher();
	TInt iBlockSize;			//< The blocksize in bytes
	TInt iIVSize;				//< The IV size in bytes
	TInt iBits;					//< Number of bits used from the IV
	CryptoSpi::CSymmetricCipher *iEncrypt; 	//< The raw encryption engine
	};

CEncryptionCipher*
CEncryptionCipher::NewL(const TLibraryPtr &aLib, TUint anIndex, const TDesC8 &aKey, TInt aBits)
	/**
	 * Construct cipher engine.
	 *
	 * @param aLib The library to use.
	 * @param anIndex The algorithm number in library.
	 * @param aKey The encryption key to be used in the encryption/decryption operation
	 * @param aBits The IV length.
	 */
	{
	CEncryptionCipher *self = new (ELeave) CEncryptionCipher();
	CleanupStack::PushL(self);
	self->ConstructL(aLib, anIndex, aKey, aBits);
	CleanupStack::Pop();        // self
	return self;	
	}

void CEncryptionCipher::ConstructL(const TLibraryPtr &aLib, TUint anIndex, const TDesC8 &aKey, TInt aBits)
	{
	iEncrypt = aLib.iLibrary->SymmetricCipherL(anIndex, aKey);
	iBlockSize = aLib.iAlgs[anIndex].iBlock;
	iIVSize = aLib.iAlgs[anIndex].iVector;
	iBits = aBits;
	}
	
CEncryptionCipher::~CEncryptionCipher()
	/**
	* Destructor.
	*/
	{
	delete iEncrypt;
	}

void CEncryptionCipher::EncryptL(const TDesC8& anIV)
	/**
	* Start cipher in encryption mode.
	*
	* @param anIV The IV to start with.
	*/
	{
	iEncrypt->SetCryptoModeL(CryptoSpi::KCryptoModeEncryptUid);
	iEncrypt->SetIvL(anIV);
	}

void CEncryptionCipher::DecryptL(const TDesC8& anIV)
	/**
	* Start cipher in decryption mode.
	*
	* @param anIV The IV to start with.
	*/
	{
	iEncrypt->SetCryptoModeL(CryptoSpi::KCryptoModeDecryptUid);
	iEncrypt->SetIvL(anIV);
	}

TInt CEncryptionCipher::GetOutputLength(TInt aInputLen)
	/**
	* Determine output length for the given input length
	* 
	*@param aInputLen Length of the input data
	*/
	{
	return iEncrypt->MaxOutputLength(aInputLen);
	}
	
TInt CEncryptionCipher::GetFinalOutputLength(TInt aInputLen)
	/**
	* Determine output length for the given input length
	* 
	*@param aInputLen Length of the input data
	*/
	{
	return iEncrypt->MaxFinalOutputLength(aInputLen);
	}
	
void CEncryptionCipher::UpdateL(TDes8& aBuf, TDes8& aBufOut)
	/**
	* Feed data to cipher engine (decrypt or encrypt).
	*
	* @param aBuf The input data buffer for encryption/decryption operation
	* @param aBufOut The output  data buffer for encryption/decryption operation
	*
	*/
	{
	iEncrypt->ProcessL(aBuf, aBufOut);
	}

void CEncryptionCipher::UpdateFinalL(TDes8& aBuf, TDes8& aBufOut)
	/**
	* Feed data to cipher engine (decrypt or encrypt) to process the final block.
	*
	* @param aBuf The input data buffer for encryption/decryption operation
	* @param aBufOut The output  data buffer for encryption/decryption operation
	*/
	{
	iEncrypt->ProcessFinalL(aBuf, aBufOut);
	}

void CEncryptionCipher::Reset ()
	/**
	* Reset the state of encryption/decryption operation. Invoked as part of handling of internal errors.
	*
	* @param <None>
	*/
	{
	if (iEncrypt)
		{
		iEncrypt->Reset();
		}
	}
class CEncryptionNull : public CEncryptionBase
	/**
	* NULL Encryption Engine.
	*
	* Implements the NULL encryption for the RFC-2410 purposes.
	*/
	{
	friend class CIpsecCryptoManager;
public:
	virtual void EncryptL(const TDesC8& /*anIV*/) {}	// Reset encryption engine
	virtual void DecryptL(const TDesC8& /*anIV*/) {}	// Reset encryption engine
	virtual void UpdateL(TDes8& aBuf, TDes8& aBufOut) { aBufOut.Copy(aBuf);}   // Encrypt/decrypt a block
	virtual void UpdateFinalL(TDes8& aBuf, TDes8& aBufOut) { aBufOut.Copy(aBuf);} // Encrypt/decrypt last block
	virtual TInt GetOutputLength(TInt aInputLength) { return aInputLength;}
	virtual TInt GetFinalOutputLength(TInt aInputLength) { return aInputLength;}
	virtual TInt BlockSize() const {return 1; }
	virtual TInt IVSize() const {return 0;}
	virtual void Reset() {}
protected:
	CEncryptionNull() {}
	~CEncryptionNull() {}
	};

CEncryptionBase *CIpsecCryptoManager::NewEncryptL(TInt anAlg, const TDesC8 &aKey)
	/**
	* Create an instance of cipher engine.
	*
	* Create a new IPsec encryption engine (CEncryptionBase) using the key and
	* encryption algorithm (anAlg).
	*
	* @li First, use the algorithm number to locate the symbolic name
	*	name of the algorithm from the algorithm map
	* @li then use this name to search supported algorithm from the
	*	installed crypto libraries.
	* If found, ask the the library to create a symmetric cipher engine for
	* this algorithm.
	*
	* @param anAlg The algorithm number
	* @param aKey The key for the algorithm
	* @return The cipher engine.
	*/
	{
	TAlgorithmMap *map = iAlgorithmList->Lookup(EAlgorithmClass_Cipher, anAlg);

	if (!map)
		User::Leave(EIpsec_UnknownCipherNumber);

	if (map->iAlgorithm.Length() == 0)
		//
		// The NULL encryption is indicated by empty string in the
		// algorithm name. Instantiate a NULL encryption "cipher"
		return new CEncryptionNull();

	TLibraryPtr *lib;
	TUint index = iLibraryList->Lookup(*map, &lib);
	if (!lib)
		User::Leave(EIpsec_UnavailableCipher);
	LOG(Log::Printf(_L("\t* using cipher %S(%d) from library %S"), &map->iAlgorithm, anAlg, &lib->iName));
	CEncryptionCipher *enc = CEncryptionCipher::NewL(*lib, index, aKey, map->iBits);
	if (enc && enc->iEncrypt)
		{
		return enc;		// All OK.
		}
	delete enc;
	//
	// Getting here implies fatal error, something is not working
	//
	return NULL;	
	}


CAuthenticationBase *CIpsecCryptoManager::NewAuthL(TInt anAlg, const TDesC8 &aKey)
	/**
	* Create an instance of authentication engine.
	* Create a new IPsec authentication engine (CAuthenticationBase) using the key and
	* digest algorithm (anAlg). First use the algorithm number to find the symbolic
	* name of the algorithm from the algorithm map, then use this name to search
	* supported algorithm from the installed crypto libraries. If found,
	* ask the library to create a raw message digest engine for this algorithm.
	*
	* The current IPsec authentication uses the digest engine to implement HMAC
	* keyed-hash authentication as described in RFC-2104. The number of bits
	* actually used from the digest value is determined by the respective
	* configuration parameter in the algorithm map entry.
	*
	* @param anAlg The authentication algorithm number
	* @param aKey  The authentication key
	* @return The authentication engine.
	*
	* @leave EIpsec_UnknownDigestNumber if anAlg does not appear in algorithm list
	* @leave EIpsec_UnavailableDigest if no library implements the requested algorithm
	* @leave other on other errors
	*/
	{
	TAlgorithmMap *map = iAlgorithmList->Lookup(EAlgorithmClass_Digest, anAlg);

	if (!map)
		User::Leave(EIpsec_UnknownDigestNumber);

	if (map->iAlgorithm.Length() > 0)
		{
		TLibraryPtr *lib;
		TUint index = iLibraryList->Lookup(*map, &lib);
		if (!lib)
			User::Leave(EIpsec_UnavailableDigest);
		LOG(Log::Printf(_L("\t* using digest %S(%d) from library %S"), &map->iAlgorithm, anAlg, &lib->iName));
		return CAuthenticationHmac::NewL(*lib, index, aKey, map->iBits);
		}
	//
	// Getting here implies fatal error, something is not working
	//
	return NULL;
	}
	
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
class  CAuthenticationMac : public CAuthenticationBase
	{
	friend class CIpsecCryptoManager;
public:
	virtual void Init()	;
	virtual void Update(const TDesC8& aMessage);
	virtual const TDesC8 &Final(TInt aSize);
	virtual TInt BlockSize() const {return iBlockSize;}
	virtual TInt Compare(const TDesC8& aDigest);
	virtual TInt DigestSize() const {return (iBits + 7) / 8;}
	//
	// Return the number of bytes defined by the algorithm
	// map, not the real digest length of the algorithm.
	virtual TInt MacSize() const {return (iBits + 7) / 8;}
protected:
    static CAuthenticationMac* NewL(const TLibraryPtr &lib, TUint anIndex, const TDesC8 &aKey, TInt aBits);
    void ConstructL(const TLibraryPtr &lib, TUint anIndex, const TDesC8 &aKey, TInt aBits);
	virtual ~CAuthenticationMac();
private:
	CAuthenticationMac();
protected:
	TInt iBlockSize;		//< The blocksize in bytes
	TInt iMacOutputSize;		//< The digest size in bytes
	TInt iBits;				//< Number of bits used from the digest.
	TInt iIVSize;
	HBufC8* iTemp;
	CMacCrypto *iMacCrypto;
	CryptoSpi::CMac *iMacImpl;
	};

CAuthenticationMac::CAuthenticationMac()
	/**
	* Constructor.
	*/
	{
	}
	
CAuthenticationMac::~CAuthenticationMac()
	/**
	* Destructor.
	*
	* Clear the memory areas containing the key information, before
	* releasing the memory.
	*/
	{	
	}
void CAuthenticationMac::Init()
	/**
	* Initialize authentication for a packet.
	*
	* Reset digest enginge and feed the input (HMAC) pad to it.
	*/
	{
	};	
CAuthenticationMac* CAuthenticationMac::NewL(const TLibraryPtr &aLib, TUint anIndex, const TDesC8 &aKey, TInt aBits)
	/**
	* Create MAC authentication engine.
	*
	* @param aLib The library to use for the MacCrypto engine
	* @param anIndex The digest algorithm number in library
	* @param aKey The authentication key
	* @param aBits The number of bits used from the digets.
	*/
	{
	CAuthenticationMac *self = new (ELeave) CAuthenticationMac();
	CleanupStack::PushL(self);
	self->ConstructL(aLib, anIndex, aKey, aBits);
	CleanupStack::Pop();        // self
	return self;
	}
void CAuthenticationMac ::ConstructL(const TLibraryPtr &aLib, TUint anIndex, const TDesC8 &aKey, TInt aBits)
	{
	iMacImpl = aLib.iLibrary->GetMacImplementationL( aKey);
	iMacOutputSize = aLib.iAlgs[anIndex].iVector;
	iBlockSize = aLib.iAlgs[anIndex].iBlock;
	iIVSize = aLib.iAlgs[anIndex].iVector;
	iBits = aBits;
	iTemp = HBufC8::NewMaxL(iMacOutputSize);
	}	



void CAuthenticationMac::Update(const TDesC8& aMessage)
	/**
	* Feed data to the digest.
	*
	* @param aMessage The data.
	*/
	{
	iMacImpl->UpdateL(aMessage);		// Feed actual payload material
	}
	


const TDesC8 &CAuthenticationMac::Final(TInt /* aSize */)
	/**
	* Finish the digest computation
	*/
	{
	//presently return temp value	
	return *iTemp; 
	}
	
TInt CAuthenticationMac::Compare(const TDesC8 &aDigest)
	/**
	* Finish the digets computation and compare with ICV.
	*
	* @param aDigest The ICV to match
	* @return comparison result (= 0, match, != 0, no match).
	*/
	{
	return aDigest.Compare(iTemp->Des());
	}
	
CAuthenticationBase *CIpsecCryptoManager::NewMacL(TInt anAlg, const TDesC8 &aKey)
	{
	TAlgorithmMap *map = iAlgorithmList->Lookup(EAlgorithmClass_Mac, anAlg);
	if (!map)
		User::Leave(EIpsec_UnknownCipherNumber);
	
	if (map->iAlgorithm.Length() > 0)
		{
		TLibraryPtr *lib;
		TUint index = iLibraryList->Lookup(*map, &lib);
		if (!lib)
			User::Leave(EIpsec_UnavailableCipher);
		LOG(Log::Printf(_L("\t* using digest %S(%d) from library %S"), &map->iAlgorithm, anAlg, &lib->iName));
		return CAuthenticationMac::NewL(*lib, index, aKey, map->iBits);
		}
	//
	// Getting here implies fatal error, something is not working
	//
	return NULL;
	}
	

#endif //SYMBIAN_IPSEC_VOIP_SUPPORT

