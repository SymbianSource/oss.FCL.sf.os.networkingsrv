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
// crypto.h - IPSEC API towards cryptographic libraries
//



/**
 @internalComponent
*/

#ifndef __CRYPTO_H__
#define __CRYPTO_H__
/**
// @file crypto.h
//
// The basic API for cryptographic algorithm library.
//
// IPSEC hook itself does not contain any cryptographic algorithms.
// The available algorithms are dynamically imported to the IPSEC
// by binding the IPSEC (pfkey) to one or more cryptographic libraries,
// each of which are implemented as a protocol..
//
// This header describes the basic API and base classes for building
// such cryptographic algorithm libraries as protocol modules for IPSEC.
//
// In the TServerProtocolDesc of such protocol, the following fields
// are significant for the IPSEC:
//
// @li	TServerProtocolDesc::iName (protocol name) is the symbolic
//		name of the library. When multiple libraries are used,
//		they should have different names.
// @li	TServerProtocolDesc::iAddrFamily should be KAfCrypto
// @li	TServerProtocolDesc::iProtocol must be KProtocolCrypto
//
// The remaining fields can be freely initialized to any values
// that satisfy the SocketServer requirements. The chosen values
// will have no effect on the IPSEC functionality.
//
// The cryptographic library protocol can be implemented in any
// protocol module (PRT file) along with other protocols. The
// value KProtocolCrypto alone in iProtocol tells that the protocol
// supports the API defined by this definition.
//
// An example of possible Ipsec configuration (ESK file)
// @verbatim

[sockman]
protocols= secpol,pfkey,lib1,lib2

[pfkey]
filename= ipsec6.prt
index= 2
bindto= lib1,lib2

[secpol]
filename= ipsec6.prt
index= 1
bindto= pfkey,ip6

[lib1]
filename= eaysymb.prt
index= 1

[lib2]
filename= newcrypto.prt
index= 1
@endverbatim

@internalTechnology
@released
*/
#include <e32base.h>
#include <es_prot.h>

#include "cryptospidef.h"
#include "cryptosymmetriccipherapi.h"
#include "cryptomacapi.h"

/**
// The protocol number for a library.
*/
const TUint KProtocolCrypto =	0x104;
/**
// The protocol family for the library.
*/
const TUint KAfCrypto = 0x0803;


typedef TBuf<0x20> TAlgorithmName;

/**
* @name	Well Known Algorithm Names
* The cryptographic libary can choose the names for it's algorithms freely,
* but it will cause less confusion, if the well known standard algoriths
* are named uniformly.
*/
//@{
/**
*	Single DES in CBC-Mode.
*	- key: 8
*	- block: 8
*	- IV: 8
*/
_LIT(KIpsecName_DES_CBC,		"descbc");
/**
*	Triple DES in CBC-Mode.
*	- key: 24
*	- block: 8
*	- IV: 8
*/
_LIT(KIpsecName_3DES_CBC,		"3descbc");
/**
*	Blowfish in CBC-Mode.
*	- key: variable in 8..72
*	- block: 8
*	- IV: 8
*/
_LIT(KIpsecName_BLOWFISH_CBC,	"blowfish");
/**
*	IDEA in CBC-Mode.
*	- key: 16
*	- block: 8
*	- IV: 8
*/
_LIT(KIpsecName_IDEA_CBC,		"idea");
/**
*	AES in CBC-Mode.
*	- key: 16, 24 or 32
*	- block: 16
*	- IV: 16
*/
_LIT(KIpsecName_AES_CBC,		"aescbc");
/**
*	AES in CTR-Mode.
*	- key 16, 24 or 32 (+ append 4 octets for the NONCE)
*	- block: 16
*	- IV: 8
*/
_LIT(KIpsecName_AES_CTR,		"aesctr");
/**
*	RC5 in CBC-Mode.
*	- key: variable 5..255
*	- block: 8
*	- IV: 8
*/
_LIT(KIpsecName_RC5,			"rc5");
/**
*	SHA1 Digest.
*	- digest length: 20
*	- block: 64
*/
_LIT(KIpsecName_SHA1,			"sha1");
/**
*	SHA2-256 Digest.
*	- digest length: 32
*	- block: 64
*/
_LIT(KIpsecName_SHA2_256,		"sha2-256");
/**
*	MD5 Digest.
*	- digest length: 16
*	- block: 64
*/
_LIT(KIpsecName_MD5,			"md5");
/**
*
* 
*/
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
_LIT(KIpsecName_AES_XCBC_MAC, "aesxcbcmac") ;
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT
//@}

typedef enum
	{
	EAlgorithmClass_Digest,	//< Message Digest algorithm
	EAlgorithmClass_Cipher,	//< Symmetric Cipher algorithm
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
	EAlgorithmClass_Mac ,
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT
	//
	// New types are possible by adding the symbol here
	// and defining the corresponding abstract class
	// (similar to CMessageDigestCrypto and CSymmetricCipher)
	//
	} TAlgorithmClass;

//	TAlgorithmDesc (and related types)
/**
// A description of available algorithm.
//
// Similar to ProtocolList, a protocol supporting this API must
// return a description of each implemented algorithm as an
// array of TAlgorithmDesc objects as a result of AlgorithmList
// call.
*/
class TAlgorithmDesc
	{
public:
	TAlgorithmName iName;		//< Name of the algorithm
	TAlgorithmClass iAlgType;	//< Class of the algorithm (cipher/digest)
	TUint iMinBits;				//< Min Length of the key in bits (all keys total)
	TUint iMaxBits;				//< Max Length of the key in bits (all keys total)
	TUint iBlock;				//< Length of the block in bytes
	TUint iVector;				//< Initialization Vector length (bytes)
	};

// Each of the following includes virtual destructor
// just in case there is a need for a cleanup code
// when the object is deleted using a pointer to
// the base virtual class.

// CMessageDigestCrypto
// ********************
/**
// Base Message Digest (abstract) class.
//
// All message digest algorithms must be derived from this
// base class, which defines the IPSEC required API for
// message digests (used by AH and ESP with authentication
// implementations). 
//
// Because IPSEC needs to run digest for each packet
// independently, it is important that the implementation
// can reset the computation by Init() without needing
// to do any additional allocations.
*/
class CMessageDigestCrypto : public CBase
	{
	// NOTE: This class was originally designed based on assumption
	// that the derived class implementing the digest includes all
	// state in member variables and does not need to allocate
	// additional space -- thus no ConstructL method.
public:
	/**
	// Set digest into initial state.
	//
	// IPSEC calls this method to start a new digest
	// computation for each IP packet that needs
	// digest computation.
	*/
	virtual void Init()=0;
	/**
	// Add segment of data to the digest.
	//
	// The octets in aMessage must be added to the digest
	// value. The length of the aMessage can be anything
	// from 0 or more octets. If the digest algorithm has
	// any inherent block requirements, then this method
	// must handle it (specifically, the digest must work
	// correctly, even if the data is fed to it one byte
	// at time).
	//
	// @param aMessage
	//	describe the segment of octets to be added into
	//	the digest (length >= 0).
	*/
	virtual void Update(const TDesC8& aMessage)=0;
	/**
	// Wrap up the digest and return the result.
	//
	// @param aDigest
	//	a buffer to return the final computed digest value.
	*/
	virtual void Final(TDes8& aDigest)=0;
	virtual ~CMessageDigestCrypto() {}
	};
	
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
class CMacCrypto : public CBase
	{
	public:
	/**
	// Set digest into initial state.
	//
	// IPSEC calls this method to start a new digest
	// computation for each IP packet that needs
	// digest computation.
	*/
	virtual void Init()=0;
	/**
	// Add segment of data to the digest.
	//
	// The octets in aMessage must be added to the digest
	// value. The length of the aMessage can be anything
	// from 0 or more octets. If the digest algorithm has
	// any inherent block requirements, then this method
	// must handle it (specifically, the digest must work
	// correctly, even if the data is fed to it one byte
	// at time).
	//
	// @param aMessage
	//	describe the segment of octets to be added into
	//	the digest (length >= 0).
	*/
	virtual void Update(const TDesC8& aMessage)=0;
	/**
	// Wrap up the digest and return the result.
	//
	// @param aDigest
	//	a buffer to return the final computed digest value.
	*/
	virtual void Final(TDes8& aDigest)=0;
	virtual ~CMacCrypto() {}	
	};
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT

// CSymmetricCipher
// ****************
/**
// Base Symmetric Cipher (abstract) class.
//
// All cipher algorithms must be derived from this
// base class, which defines the IPSEC required API for
// cipher algorithms (used by ESP implementation). 
//
// Because IPSEC needs to run cipher for each packet
// independently, it is important that the implementation
// can reset the computation by InitL() without needing
// to do any additional allocations [which means that
// it being a leaving function is a bad sign!]
*/
class CSymmetricCipher : public CBase
	{
public:
	enum TAction { EEncrypt, EDecrypt };
	/**
	// Define the cipher key.
	//
	// Because setting the key can be time consuming,
	// this is only called once after instantiation of the
	// class. Then, each packet is started with a call
	// to InitL.
	//
	// @param aKey
	//	the cipher key. The length of the key is
	//	defined by the length of this descriptor,
	//	and is always multiple of 8 bits.
	// @return
	// @li	> 0, the key is weak (but set anyway)
	// @li	= 0, all ok
	// @li	< 0, the key not usable (not set)
	*/
	virtual TInt Setkey(const TDesC8& aKey)=0;
	/**
	// Reset the cipher engine to initial state.
	//
	// As this method is called for each packet, it
	// should not do any memory allocation or heavy
	// computations.
	//
	// @param aIV initial vector.
	// @param aMode tells whether initialize is for decrypt or encrypt.
	*/
	virtual void Init(const TDesC8 &aIV, TAction aMode)=0;
	/**
	// Perform encryption or decryption.
	//
	// Because algorithms are expected to work blocks, the
	// caller will guarantee that ALL Outbuf's given to Update
	// will exist up to Finish call (or at least as long as at
	// least blocksize octets have been given to Update after it).
	// The implementation of the algorithm can store pointer(s) to
	// aOutbuf described memory area, and return data to such
	// memory area on some later Update or Finish call.
	//
	// The lengths of buffers are always equal, e.g. aInbuf.Length()
	// octets will always fit into aOutbuf. This length can be anything
	// from zero upwards. The cipher must work even if octets were
	// fed to it one by one.
	//
	// @param aOutbuf	result of the decrypt/encrypt
	// @param aInbuf	input to decrypt/encrypt
	*/
	virtual void Update(TDes8& aOutbuf,const TDesC8& aInbuf)=0;
	/**
	// Finish encryption or decryption.
	//
	// Calling Finish is optional, it is needed if the total
	// bytes is not multiple of the blocksize, or if one wants
	// to get the final IV.
	//
	// IPSEC does use the final IV.
	//
	// @param	aIV	the place to return the final IV.
	*/
	virtual void Finish(TDes8& aIV)=0;
	virtual ~CSymmetricCipher() {}
	};

// CProtocolCrypto
// ***************
/**
// Base class of the protocol implementing an algorithm library as a protocol
//
// All algorithm libraries must be derived from this base class.
*/
class CProtocolCrypto : public CProtocolBase
	{
public:
	/**
	// Return the list of supported algorithms.
	//
	// IPSEC calls this method once during the binding
	// process to find out the algorithms that are supported
	// by this library.
	//
	// @retval	aList
	//	a pointer to a new allocated array of TAlgorithmDesc.
	//	This array contains the descriptions of the supported
	//	algorithms. Can also return NULL, if not algorithms
	//	are supported at this point. The calling IPSEC will
	//	release this array, when it is not needed.
	//
	// @returns
	//	the length of the the array. May also return <= 0,
	//	in which case IPSEC will not be using any algorithms
	//	from this library.
	*/
	virtual TUint AlgorithmList(TAlgorithmDesc *&aList) = 0;
	/**
	// Create an instance of cipher algorithm
	//
	// When IPSEC requires a use of specific algorithm, it
	// asks a new instance of the algorithm by calling this
	// method.
	//
	// @param aAlg
	//	index of the algorithm in the array of descriptions
	//	that was returned by the AlgorithmList().
	//
	// @param aKey - The key value to be used in the encryption/decryption operation    
	// @return
	// @li	NULL, if algorithm could not be instantiated
	// @li	non-NULL (= new algorithm engine instance), if algorithm instantiated
	*/
	virtual CryptoSpi::CSymmetricCipher* SymmetricCipherL(TUint aAlg, const TDesC8 &aKey)=0;
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT	
	virtual CryptoSpi::CMac*  GetMacImplementationL(const TDesC8& aKey)=0;
#endif	
	/**
	// Create an instance of digest algorithm
	//
	// When IPSEC requires a use of specific algorithm, it
	// asks a new instance of the algorithm by calling this
	// method.
	//
	// @param aAlg
	//	index of the algorithm in the array of descriptions
	//	that was returned by the AlgorithmList().
	//
	// @return
	// @li	NULL, if algorithm could not be instantiated
	// @li	non-NULL (= new algorithm engine instance), if algorithm instantiated
	*/
	virtual CMessageDigestCrypto* MessageDigest(TUint aAlg)=0;
protected:
	virtual ~CProtocolCrypto() {}
	};

#endif
