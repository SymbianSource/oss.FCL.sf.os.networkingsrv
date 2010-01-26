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
// sa_crypt.h - IPv6/IPv4 IPSEC interface to crypto libraries
// IPsec interface to the cryptographic library manager
// and to the underlying cryptographic algorithms.
// IPsec creates one instance of CIpsecCryptoManager and expects it to
// provide the authentication and cipher engines through the NewAuthL
// and NewEncryptL methods.
// The engines are assumed to work according to the abtract interfaces
// CEncryptionBase and CAuthenticationBase
//



/**
 @file sa_crypt.h
 @internalComponent
*/

#ifndef __SA_CRYPT_H__
#define __SA_CRYPT_H__

#include <e32base.h>
#include <networking/pfkeyv2.h>	// for struct sadb_alg
#include "ipseclog.h"	// because _LOG is used in this header.



class CEncryptionBase : public CBase
	/**
	* Base IPsec encryption class.
	*
	* This class defines the abstract IPsec interface to the encryption algorithms.
	* It serves as an intermediate between the IPsec and the actual encryption algorithm
	* object instantiated from some installed cryptographic library. Most of the methods
	* are plain "pass-through" to the underlying cryptographic algorithm engine.
	* The main "service" of this layer is provided in the set-up phase.
	*
	* Setting up:
	*	@li Create new object by CIpsecCryptoManager::NewEncryptL()
	*
	*·Using the encryption engine (any number of times):
	*	@li Start new encryption (Encrypt()) or decryption (Decrypt),
	*	@li Process octets, one or more uses of Update(),
	*	@li Finish the encryption process with Finish()
	*
	* Housekeeping methods:
	*	@li Return the block size of the algorithm (if you want to align things) with BlockSize(),
	*	@li Return the set IV size of the algorithm with IVSize()
	*
	* Closing down:
	*	@li Delete the object
	*
	* Currently there are two derived classes from this base class:
	*	-	CEncryptionCipher class - interface to any of the supported cipher algorithms in the installed libraries
	*	-	CEncryptionNull - the NULL encryption algorithm (RFC-2410) 
	*/
	{
public:
	virtual ~CEncryptionBase() {}
	/**
	* Prepare engine for encryption of a packet.
	*
	* @param anIV The initial vector for the encryption.
	*/
	virtual void EncryptL(const TDesC8 &anIV)=0;
	/**
	* Prepare engine for decryption of a packet.
	*
	* @param anIV The initial vector for the decryption
	*/
	virtual void DecryptL(const TDesC8 &anIV)=0;
	/**
	 * Method to fetch the length of output for encryption/decryption operation
	 * given the input size.
	 *
	 * @param aInputLength Length of input data
	 *
	 */
	 virtual TInt GetOutputLength(TInt aInputLen) = 0;
	/**
	 * Method to fetch the length of output for encryption/decryption operation
	 * given the input size of final buffer.
	 *
	 * @param aInputLength Length of input data
	 *
	 */
	 virtual TInt GetFinalOutputLength(TInt aInputLen) = 0;
	/**
	* Encrypt or decrypt a buffer fragment.
	*
	* @param aBuf The data to process
	* @param aBufOut Buffer to hold the output of encryption/decryption operation
	*
	*/
	virtual void UpdateL(TDes8& aBuf, TDes8& aBufOut) = 0;
	/**
	* Encrypt or decrypt the last buffer fragment.
	*
	* @param aBuf The data to process
	* @param aBufOut Buffer to hold the output of encryption/decryption operation
	*/
	virtual void UpdateFinalL(TDes8& aBuf, TDes8& aBufOut) = 0;
	/**
	* Return the block size of the algorithm.
	*
	* @return Block size in bytes.
	*/
	virtual TInt BlockSize() const =0;
	/**
	* Return configured IV size.
	*
	* The configured IV size can be shorter than the actual
	* IV of the algorithm.
	*
	* @return The configured IV size.
	*/
	virtual TInt IVSize() const = 0;
	/**
	* Reset the encryption/decryption state as part of handling of internal failures.
	*
	* @param None
	*/
	virtual void Reset() = 0;
	};

class CAuthenticationBase : public CBase
	/**
	* Base IPsec authentication class.
	*
	* This class defines the abstract IPsec interface to the authentication algorithms.
	* It implements the HMAC Keyed-Hashing algorithm as defined in RFC-2104, using a raw
	* message digest algorithm from the underlying cryptographic library. (If the
	* cryptographic library interface is later expanded to include native authentication
	* algorithms, the HMAC phase can be skipped for such algorithms).
	*
	* Setting up:
	*	@li Create new object by CIpsecCryptoManager::NewAuthL()
	*
	* Using the object to generate authentication (any number of times):
	*	@li Start new authentication with Init(),	
	*	@li Process octets, one or more uses of Update(),
	*	@li Finish the digest process by Final() or Compare().
	*
	* Housekeeping methods:
	*	@li Query the block size of the algorithm (if you want to align things) with BlockSize(),
	*	@li Query the set digest size of the algorithm with DigestSize()
	*
	* Closing down:
	*	@li Delete the object
	*
	* Currently there is only one derived class:
	*	@li CAuthenticationHmac - implements the HMAC message authentication using a raw message
	*	cipher algorithm from installed libraries.
	*/
	{
public:
	virtual ~CAuthenticationBase() {}
	/**
	* Initialize digest.
	*
	* Do the HMAC set-up and initialize the message digest computing.
	*/
	virtual void Init()=0;
	/**
	* Feed a fragment of data to the digest.
	*
	* @param aMessage The data fragment.
	*/
	virtual void Update(const TDesC8& aMessage)=0;
	/**
	* Finish digest and compare ICV.
	*
	* This is used when computing the digest for incoming packets. Input
	* ICV is from the incoming packet and is compared with the computed
	* value.
	*
	* @param aDigest The digest to compare
	* @return
	*	@li == 0, digests match (ICV check passes).
	*	@li != 0, digests do not match. 
	*/	
	virtual TInt Compare(const TDesC8& aDigest)=0;// Finish & Compare ICV
	/**
	* Finish digest and return final ICV.
	*
	* This is used when computing the digest for outgoing packets. The returned
	* digest value is placed into the packet.
	*
	* @param aSize The number of bytes to use.
	* @return The digest.
	*/
	virtual const TDesC8 &Final(TInt aSize)=0;
	/**
	* Return block size of the digest algorithm.
	*
	* @return The block size.
	*/
	virtual TInt BlockSize() const = 0;
	/**
	* Return configured digest size.
	*
	* This tells how many bytes of the digest value (BlockSize()) is actually
	* used as ICV value.
	*
	* @return The digest size.
	*/
	virtual TInt DigestSize() const = 0;
	};


class CLibraryList;
class CAlgorithmList;
class CProtocolCrypto;

class CIpsecCryptoManager : public CBase
	/**
	* Manager of loadable encryption libraries.
	*
	* There is only one instance of cryptographic library manager within the IPsec
	* implementation and it is created when CProtocolKey is created. The address of
	* this instance is stored in the CProtocolKey iCrypto variable.
	*
	* Logically, the manager is internal part of the CProtocolKey implementation and
	* must not be used directly from outside CProtocolKey/CSecurityAssociation methods.
	* The services or methods can be divided into three groups:
	*
	* Setting up and maintaining the libraries:
	* - construct the manager object (NewL())
	* - add one or more libraries to the library list (AddLibraryL())
	* - maintain and update the algorithm list (SetAlgorithms())
	* - shutting down and release of all resources (~CIpsecCryptoManager())
	*
	* Using the algorithms (the manager only handles the creating of the engine
	* objects, after that the objects themselves provide the actual interface
	* to the algorithms):
	* - create new instances of authentication engines with specific key (NewAuthL())
	* - create new instances of encryption engines with a specific key (NewEncryptL())
	*
	* The support for the key management applications
	* - return currently supported algorithms (SupportedAlgorithms())
	*/
	{
public:
    static CIpsecCryptoManager* NewL();
    void ConstructL();
	~CIpsecCryptoManager();
	void AddLibraryL(CProtocolCrypto *aLibrary);
	void SetAlgorithms(CAlgorithmList*& aList);
	CAuthenticationBase *NewAuthL(TInt anAlg, const TDesC8 &aKey);
	CEncryptionBase *NewEncryptL(TInt anAlg, const TDesC8 &aKey);
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
	CAuthenticationBase *NewMacL(TInt anAlg, const TDesC8 &aKey);
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT
	CArrayFixFlat<struct sadb_alg> *SupportedAlgorithms(TInt &aNumAuth, TInt &aNumEncrypt);
#ifdef _LOG
	// Only in DEBUG variant for logging (make it safe even if this ==  NULL)
	inline const CAlgorithmList *Algorithms() const  { return this ? iAlgorithmList : NULL; }
#endif	
private:
	CIpsecCryptoManager();
	
private:
	// Dynamic Crypto Library Bindings
	CLibraryList *iLibraryList;		//< Known crypto libraries
	CAlgorithmList *iAlgorithmList;	//< IPsec to crypto algorithm mappings
	};

#endif
