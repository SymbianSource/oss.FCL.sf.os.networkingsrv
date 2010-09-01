/**
* Copyright (c) 2003-2010 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* This file contains shared types and data structures between TLS protocol,Provider and Token
* 
*
*/



/**
 @file
 @publishedPartner
 @released
*/

#ifndef __TLSTYPEDEF_H__
#define __TLSTYPEDEF_H__

#include <e32std.h>
#include <e32base.h>


#include "pkixcertchain.h"
#include <sslerr.h>

#ifndef BULLSEYE_OFF
#ifdef _BullseyeCoverage
#define BULLSEYE_OFF "BullseyeCoverage save off";
#define BULLSEYE_RESTORE "BullseyeCoverage restore";
#else
#define BULLSEYE_OFF 
#define BULLSEYE_RESTORE 
#endif
#endif

#define KTLSServerClientRandomLen 32
#define KTLSMaxSessionIdLen 32


/*ERROR CODES: The final list had to be drafted 
along with Protocol and token
*/
#define KTLSProviderErrBase		-11100
#define KErrYetToEnumerate		KTLSProviderErrBase - 1
#define KErrCannotObtainList	KTLSProviderErrBase - 2
#define KErrNoTokenTypes		KTLSProviderErrBase - 3
#define KErrNoTokensPresent		KTLSProviderErrBase - 4
#define KErrKeyGenerationFailed	KTLSProviderErrBase - 5
#define KErrCipherNotSupported	KTLSProviderErrBase - 6
#define KErrCannotOpenToken		KTLSProviderErrBase - 7
#define KErrNoCertsAvailable	KTLSProviderErrBase - 8
#define KErrCertValidationFailed	KTLSProviderErrBase - 9
#define KErrBadServerFinishedMsg	KTLSProviderErrBase - 10
#define KErrBadMAC					KTLSProviderErrBase - 11
   
//token error codes
const TInt KTLSErrUnknownRequest = KTLSProviderErrBase - 12;
const TInt KTLSErrBadSignAlg = KTLSProviderErrBase - 13;
const TInt KTLSErrBadKeyExchAlg = KTLSProviderErrBase - 14;
const TInt KTLSErrBadProtocolVersion = KTLSProviderErrBase - 15;
const TInt KTLSErrBadCipherSuite = KTLSProviderErrBase - 16;
const TInt KTLSErrCacheEntryInUse = KTLSProviderErrBase - 17;
const TInt KTLSErrNotCached = KTLSProviderErrBase - 18;
const TInt KTLSErrNotInitialized = KTLSProviderErrBase - 19;
const TInt KTLSErrBadArgument = KTLSProviderErrBase - 20;


/** 
 * @publishedPartner 
 * @released
 */
typedef TBuf8<KTLSMaxSessionIdLen> TTLSSessionId;


/**
 * @publishedPartner
 * @released
 */
enum TTLSKeyExchangeAlgorithm { ENullKeyEx, ERsa, EDiffieHellman, EDHE, EDHanon, ECDH, EPsk, EDhePsk, ERsaPsk };
/**
 * @publishedPartner
 * @released
 */
enum TTLSSignatureAlgorithm { EAnonymous, ERsaSigAlg, EDsa, ECDsa, EPskSigAlg };
/**
 * @publishedPartner
 * @released
 */
enum TTLSBulkCipherAlgorithm { ENullSymCiph, ERc4, ERc2, EDes, E3Des, EDes40, EIdea, EAes }; 
/**
 * @publishedPartner
 * @released
 */
enum TTLSMACAlgorithm { ENullMac, EMd5, ESha };
/**
 * @publishedPartner
 * @released
 */
enum TTLSCipherType { EStream, EBlock };
/**
 * @publishedPartner
 * @released
 */
enum TTLSCompressionMethod { ENullCompression  };

/**
 * @internalAll 
 */
enum TTLSDialogMode { ETTLSDialogModeAttended, ETTLSDialogModeUnattended, ETTLSDialogModeAllowAutomatic };

class CSubjectPublicKeyInfo;
class CMessageDigest;
class TTLSCipherSuiteMapping;

/**
This class belongs to Cipher Suite Types.This follows the convention of cipher suites identification from RFC2246 - compare A.5
@publishedPartner
@released
*/
class TTLSCipherSuite
	{
public: 
	TBool operator==(TTLSCipherSuite aInput) const
		{
		return((iHiByte == aInput.iHiByte)  && (iLoByte == aInput.iLoByte) );
		}
		
	inline const TTLSCipherSuiteMapping* CipherDetails() const;

public:
	TUint8 iHiByte;
	TUint8 iLoByte; 

// This makes this class big enough to be in a RArray, note that 
// adding 2 TUint8 does not make the class big enough yet!	
	TInt iAlign;
	};

/**
The TMasterSecretInput class is used for passing information necessary for the 
generation of a master secret
@publishedPartner
@released
*/
class TTLSMasterSecretInput
	{
public:
	TBuf8<KTLSServerClientRandomLen>  iServerRandom;
	TBuf8<KTLSServerClientRandomLen>  iClientRandom;
	};

/**
This component provides with the data which are used to create the private keys.new type with 
flag: ERsa or EDHE (or EDiffieHellman) then value1, value2, value3 of which 1,2 are rsa_modulus,
rsa_exponent in RSA case,or 1,2,3 are dh_p, dh_g, dh_Ys in DH case.
If using PSK key exchange, args 4 and 5 are the psk identity and  psk value
@publishedPartner
@released
*/
class CTLSPublicKeyParams : public CBase
	{
public:
	TTLSKeyExchangeAlgorithm iKeyType; 
	HBufC8* iValue1; /// rsa_modulus 	or dh_p
	HBufC8* iValue2; /// rsa_exponent 	or dh_g
	HBufC8* iValue3; /// - 			or dh_Ys
	HBufC8* iValue4; /// 0 or PSK identity
	HBufC8* iValue5; /// 0 or PSK key
public:
	virtual ~CTLSPublicKeyParams();

	};
/**
This class is used for server identification. Its definition conforms with the definition of the 'Peers' Data Object structure
@publishedPartner
@released
*/
class TTLSServerAddr
	{
public:
	TUint16 iPort;
	TBuf8<18> iAddress;
	};

/**
Provide a description of session and ID what this does (and where it's used).
@publishedPartner
@released
@see Rename TTLSSessionNameAndID to TTlsServerNameAndID 
*/
class TTLSSessionNameAndID 
	{
public:
	TTLSSessionId iSessionId;
	TTLSServerAddr iServerName; 
	};

/**
This class specifies the SSL or TLS protocol version.
@publishedPartner
@released
*/
class TTLSProtocolVersion
	{
public:

	TBool operator==(const TTLSProtocolVersion aInput) const
		{
		return((iMajor == aInput.iMajor)  && (iMinor == aInput.iMinor) );
		}

	TBool operator!=( const TTLSProtocolVersion aInput) const
		{
		return((iMajor != aInput.iMajor)  && (iMinor != aInput.iMinor) );
		}
		
public:
	TUint8 iMajor;
	TUint8 iMinor;

// This makes this class big enough to be in a RArray, note that 
// adding 2 TUint8 does not make the class big enough yet!
	TInt 	iAlign;  
	};

/**
The TSessionData class is used for passing session specific data between cryptographic tokens and the TLS provider interface.
@publishedPartner
@released
*/
class TTLSSessionData 
	{
public:
	TTLSSessionId		iSessionId;
	TTLSCipherSuite		iCipherSuite;
	TTLSProtocolVersion iProtocolVersion;
	TTLSCompressionMethod	iCompressionMethod;
	TBool			iServerAuthenticated;
	TBool			iClientAuthenticated;
	
	TTLSMACAlgorithm		iMacAlgorithm;
	TTLSBulkCipherAlgorithm	iBulkCipherAlgorithm;	
	TBool iResumable;
	};

const TTLSProtocolVersion KTLS1_0 = {3,1};
const TTLSProtocolVersion KSSL3_0 = {3,0}; 

/**
used re-map cipher suite identifiers specified by the TTLSCipherSuite type object into
concrete cryptographic algorithms and associated parameters. In order to define this 
re-mapping the following class is specified:
@publishedPartner
@released
*/
class TTLSCipherSuiteMapping 
	{
public:
	TTLSCipherSuite iCipherSuite;
	TTLSKeyExchangeAlgorithm iKeyExAlg;
	TTLSSignatureAlgorithm iSigAlg;
	TTLSBulkCipherAlgorithm iBulkCiphAlg;
	TTLSMACAlgorithm iMacAlg;
	TTLSCipherType iCipherType;
	TBool iIsExportable;
	TUint iHashSize;
	TUint iKeyMaterial;
	TUint iIVSize;	
	TUint iPriority;
	TBool iSupported;
	TUint iExpKeySize;
	};

// Ciphersuites 0,0 to 0,0x1b + 0,0x2f + 0,0x35 + 0,0x8a to 0,0x95
#define TLS_CIPHER_SUITES_NUMBER (0x1c + 2 + 12)

/**
	Cipher priority.  This determines the order in which
	the ciphers are sent.  Numerically lower priorities
	are listed first.
 */
#define pri2(hi,lo)		((hi << 16) | lo)
/**
	Cipher priority.  This only takes the high
	halfword, leaving the low halfword available
	for inserting new ciphers later.
 */
#define pri(hi)			pri2(hi,0x0000)
/**
	Unsupported cipher priority.  This is numerically greater
	than any supported priority, although that is academic because
	it will not be listed by CTlsProviderImpl::ReturnCipherListL.
	
	@see CTlsProviderImpl::ReturnCipherListL
 */
#define pri_unsupp		pri(0x2000)
/**
	Supported priority.  This should numerically greater
	than any priority directly specified with the pri macro,
	but less than any unsupported priority.  The effect is
	to list supported but unprioritised ciphers last.

	To simplify checking the returned cipher suites, these
	ciphers must also be ordered.
 */
#define pri_supp_last(lo)	pri2(0x1000,lo)

// If CSecureSocket::SetAvailableCipherSuites is used to specify an ordered list
// of ciphersuites to offer to the server in the ClientHelllo, then that list will be used
// (after filtering out any which are not supported by a tls token).
//
// If CSecureSocket::SetAvailableCipherSuites is not called, the following table will be
// used to construct an ordered list to use.
//
// Only ciphersuties marked as supported and allowed by the security setting (weak/strong) 
// will be used.
//
// The ciphersuites will be sorted by the pri column (lower value means higher priority).
// The order is intended to be most secure first. Ciphersuites which use PSK are
// considered to be slightly more secure (because they also offer client authentication)
// than the similar non-PSK ciphersuites.
const TTLSCipherSuiteMapping KSetOfTLSCipherSuites[TLS_CIPHER_SUITES_NUMBER] =
/**
	Available cipher suites.  View this table with TAB=4.
 */
	{
	// id,	iKeyExAlg	iSigAlg,		iBulkCiphAlg		iMacAlg		iCipherType,	iIsExp,	hshsz,	keymat,	iVSz,	pri,			iSupported,  	iEKeySize
	//TLS_NULL_WITH_NULL_NULL
	{{0,0},	ENullKeyEx,	EAnonymous, ENullSymCiph,	ENullMac,	EStream,	EFalse,	0,		0,		0,		pri_unsupp,	EFalse,		0},
	//TLS_RSA_WITH_NULL_MD5
	{{0,1},	ERsa,		ERsaSigAlg,	ENullSymCiph,	EMd5,		EStream,	ETrue,	16,		0,		0,		pri(210),	ETrue,		0},
	//TLS_RSA_WITH_NULL_SHA
	{{0,2},	ERsa,		ERsaSigAlg,	ENullSymCiph,	ESha,		EStream,	ETrue,	20,		0,		0,		pri(200),	ETrue,		0},
	//TLS_RSA_EXPORT_WITH_RC4_40_MD5
	{{0,3},	ERsa,		ERsaSigAlg,	ERc4,			EMd5,		EStream,	ETrue,	16,		5,		0,		pri(110),	ETrue,		16},

	// id,	iKeyExAlg	iSigAlg,	iBulkCiphAlg	iMacAlg		iCipherType,iIsExp,	hshsz,	keymat,	iVSz,	pri,		iSupported
	//TLS_RSA_WITH_RC4_128_MD5
	{{0,4},	ERsa,		ERsaSigAlg,	ERc4,			EMd5,		EStream,	EFalse,	16,		16,		0,		pri(70),	ETrue,		16},
	//TLS_RSA_WITH_RC4_128_SHA
	{{0,5},	ERsa,		ERsaSigAlg,	ERc4,			ESha,		EStream,	EFalse,	20,		16,		0,		pri(60),	ETrue,		16},
	//TLS_RSA_EXPORT_WITH_RC2_CBC_40_MD5
	{{0,6},	ERsa,		ERsaSigAlg,	ERc2,			EMd5,		EBlock,		ETrue,	16,		5,		8,		pri_unsupp,	EFalse,		16},
	//TLS_RSA_WITH_IDEA_CBC_SHA
	{{0,7},	ERsa,		ERsaSigAlg,	EIdea,			ESha,		EBlock,		EFalse,	20,		16,		8,		pri_unsupp,	EFalse,		16},

	// id,	iKeyExAlg	iSigAlg,	iBulkCiphAlg	iMacAlg		iCipherType,iIsExp,	hshsz,	keymat,	iVSz,	pri,		iSupported
	//TLS_RSA_EXPORT_WITH_DES40_CBC_SHA
	{{0,8},	ERsa,		ERsaSigAlg,	EDes40,			ESha,		EBlock,		ETrue,	20,		5,		8,		pri(100),	ETrue,		8},
	//TLS_RSA_WITH_DES_CBC_SHA
	{{0,9},	ERsa,		ERsaSigAlg,	EDes,			ESha,		EBlock,		EFalse,	20,		8,		8,		pri(80),	ETrue,		8},
	//TLS_RSA_WITH_3DES_EDE_CBC_SHA
	{{0,0xA},ERsa,		ERsaSigAlg,	E3Des,			ESha,		EBlock,		EFalse,	20,		24,		8,		pri(30),	ETrue,		24},
	//TLS_DH_DSS_EXPORT_WITH_DES40_CBC_SHA
	{{0,0xB}, EDiffieHellman, EDsa,	EDes40,			ESha,		EBlock,		ETrue,	20,		5,		8,		pri_unsupp,	EFalse,		8},

	// id,	iKeyExAlg	iSigAlg,	iBulkCiphAlg	iMacAlg		iCipherType,iIsExp,	hshsz,	keymat,	iVSz,	pri,		iSupported
	//TLS_DH_DSS_WITH_DES_CBC_SHA
	{{0,0xC}, EDiffieHellman, EDsa,	EDes,			ESha,		EBlock,		EFalse,	20,		8,		8,		pri_unsupp,	EFalse,		8},
	//TLS_DH_DSS_WITH_3DES_EDE_CBC_SHA
	{{0,0xD}, EDiffieHellman, EDsa,	E3Des,			ESha,		EBlock,		EFalse,	20,		24,		8,		pri_unsupp,	EFalse,		24},
	//TLS_DH_RSA_EXPORT_WITH_DES40_CBC_SHA
	{{0,0xE}, EDiffieHellman, ERsaSigAlg, EDes40,	ESha,		EBlock,		ETrue,	20,		5,		8,		pri_unsupp,	EFalse,		8},
	//TLS_DH_RSA_WITH_DES_CBC_SHA
	{{0,0xF}, EDiffieHellman, ERsaSigAlg, EDes,		ESha,		EBlock,		EFalse,	20,		8,		8,		pri_unsupp,	EFalse,		8},

	// id,	iKeyExAlg	iSigAlg,	iBulkCiphAlg	iMacAlg		iCipherType,iIsExp,	hshsz,	keymat,	iVSz,	pri,		iSupported
	//TLS_DH_RSA_WITH_3DES_EDE_CBC_SHA
	{{0,0x10}, EDiffieHellman, ERsaSigAlg, E3Des,	ESha,		EBlock,		EFalse,	20,		24,		8,		pri_unsupp,	EFalse,		24},
	//TLS_DHE_DSS_EXPORT_WITH_DES40_CBC_SHA
	{{0,0x11}, EDHE,	EDsa,		EDes40,			ESha,		EBlock,		ETrue,	20,		5,		8,		pri(120),	ETrue,		8},
	//TLS_DHE_DSS_WITH_DES_CBC_SHA
	{{0,0x12}, EDHE,	EDsa,		EDes,			ESha,		EBlock,		EFalse,	20,		8,		8,		pri(90),	ETrue,		8},
	//TLS_DHE_DSS_WITH_3DES_EDE_CBC_SHA
	{{0,0x13}, EDHE,	EDsa,		E3Des,			ESha,		EBlock,		EFalse,	20,		24,		8,		pri(50),	ETrue,		24},

	// id,	iKeyExAlg	iSigAlg,	iBulkCiphAlg	iMacAlg		iCipherType,iIsExp,	hshsz,	keymat,	iVSz,	pri,		iSupported
	//TLS_DHE_RSA_EXPORT_WITH_DES40_CBC_SHA
	{{0,0x14}, EDHE,	ERsaSigAlg,	EDes40,			ESha,		EBlock,		ETrue,	20,		5,		8,		pri(130),	ETrue,		8},
	//TLS_DHE_RSA_WITH_DES_CBC_SHA
	{{0,0x15}, EDHE,	ERsaSigAlg,	EDes,			ESha,		EBlock,		EFalse,	20,		8,		8,		pri_unsupp,	EFalse,		8},
	//TLS_DHE_RSA_WITH_3DES_EDE_CBC_SHA
	{{0,0x16}, EDHE,	ERsaSigAlg,	E3Des,			ESha,		EBlock,		EFalse,	20,		24,		8,		pri(40),	ETrue,		24},
	//TLS_DH_anon_EXPORT_WITH_RC4_40_MD5
	{{0,0x17}, EDHanon,	EAnonymous,	ERc4,			EMd5,		EStream,	ETrue,	16,		5,		0,		pri_unsupp,	EFalse,		16},

	// id,	iKeyExAlg	iSigAlg,	iBulkCiphAlg	iMacAlg		iCipherType,iIsExp,	hshsz,	keymat,	iVSz,	pri,		iSupported
	//TLS_DH_anon_WITH_RC4_128_MD5
	{{0,0x18}, EDHanon,	EAnonymous,	ERc4,			EMd5,		EStream,	EFalse,	16,		16,		0,		pri_unsupp,	EFalse,		16},
	//TLS_DH_anon_EXPORT_WITH_DES40_CBC_SHA
	{{0,0x19}, EDHanon,	EAnonymous,	EDes40,			ESha,		EBlock,		EFalse,	20,		5,		8,		pri(140), 	ETrue,		8},
	//TLS_DH_anon_WITH_DES_CBC_SHA
	{{0,0x1A}, EDHanon,	EAnonymous,	EDes,			ESha,		EBlock,		EFalse,	20,		8,		8,		pri_unsupp,	EFalse,		8},
	//TLS_DH_anon_WITH_3DES_EDE_CBC_SHA
	{{0,0x1B}, EDHanon, EAnonymous, E3Des,			ESha,		EBlock,		EFalse,	20,		24,		8,		pri_unsupp,	EFalse,		24},

	// id,	iKeyExAlg	iSigAlg,	iBulkCiphAlg	iMacAlg		iCipherType,iIsExp,	hshsz,	keymat,	iVSz,	pri,		iSupported
	//TLS_RSA_WITH_AES_128_CBC_SHA
	{{0,0x2F}, ERsa,	ERsaSigAlg,	EAes,			ESha,		EBlock,		EFalse,	20,		16,		16,		pri(20), 	ETrue,		16},
	//TLS_RSA_WITH_AES_256_CBC_SHA
	{{0,0x35}, ERsa,	ERsaSigAlg,	EAes,			ESha,		EBlock,		EFalse,	20,		32,		16,		pri(10), 	ETrue,		32},

	// id,	iKeyExAlg	iSigAlg,		iBulkCiphAlg		iMacAlg		iCipherType,	iIsExp,	hshsz,	keymat,	iVSz,	pri,		iSupported
	// TLS_PSK_WITH_RC4_128_SHA
	{{0,0x8a}, EPsk,	EPskSigAlg,	ERc4,			ESha,		EStream,	EFalse,	20,		16,		0,		pri(59), 	ETrue,		16},
	// TLS_PSK_WITH_3DES_EDE_CBC_SHA
	{{0,0x8b}, EPsk,	EPskSigAlg,	E3Des,			ESha,		EBlock,		EFalse,	20,		24,		8,		pri(29), 	ETrue,		24},
	// TLS_PSK_WITH_AES_128_CBC_SHA
	{{0,0x8c}, EPsk,	EPskSigAlg,	EAes,			ESha,		EBlock,		EFalse,	20,		16,		16,		pri(19), 	ETrue,		16},
	// TLS_PSK_WITH_AES_256_CBC_SHA
	{{0,0x8d}, EPsk,	EPskSigAlg,	EAes,			ESha,		EBlock,		EFalse,	20,		32,		16,		pri(9), 	ETrue,		32},

	// id,	iKeyExAlg	iSigAlg,		iBulkCiphAlg		iMacAlg		iCipherType,	iIsExp,	hshsz,	keymat,	iVSz,	pri,		iSupported
	// TLS_DHE_PSK_WITH_RC4_128_SHA
	{{0,0x8e}, EDhePsk,	EPskSigAlg,	ERc4,			ESha,		EStream,	EFalse,	20,		16,		0,		pri(58), 	EFalse,		16},
	// TLS_DHE_PSK_WITH_3DES_EDE_CBC_SHA
	{{0,0x8f}, EDhePsk,	EPskSigAlg,	E3Des,			ESha,		EBlock,		EFalse,	20,		24,		8,		pri(28), 	EFalse,		24},
	// TLS_DHE_PSK_WITH_AES_128_CBC_SHA
	{{0,0x90}, EDhePsk,	EPskSigAlg,	EAes,			ESha,		EBlock,		EFalse,	20,		16,		16,		pri(18), 	EFalse,		16},
	// TLS_DHE_PSK_WITH_AES_256_CBC_SHA
	{{0,0x91}, EDhePsk,	EPskSigAlg,	EAes,			ESha,		EBlock,		EFalse,	20,		32,		16,		pri(8), 	EFalse,		32},

	// id,	iKeyExAlg	iSigAlg,		iBulkCiphAlg		iMacAlg		iCipherType,	iIsExp,	hshsz,	keymat,	iVSz,	pri,		iSupported
	// TLS_RSA_PSK_WITH_RC4_128_SHA
	{{0,0x92}, ERsaPsk,	EPskSigAlg,	ERc4,			ESha,		EStream,	EFalse,	20,		16,		0,		pri(57), 	EFalse,		16},
	// TLS_RSA_PSK_WITH_3DES_EDE_CBC_SHA
	{{0,0x93}, ERsaPsk,	EPskSigAlg,	E3Des,			ESha,		EBlock,		EFalse,	20,		24,		8,		pri(27), 	EFalse,		24},
	// TLS_RSA_PSK_WITH_AES_128_CBC_SHA
	{{0,0x94}, ERsaPsk,	EPskSigAlg,	EAes,			ESha,		EBlock,		EFalse,	20,		16,		16,		pri(17), 	EFalse,		16},
	// TLS_RSA_PSK_WITH_AES_256_CBC_SHA
	{{0,0x95}, ERsaPsk,	EPskSigAlg,	EAes,			ESha,		EBlock,		EFalse,	20,		32,		16,		pri(7), 	EFalse,		32}
	};




inline const TTLSCipherSuiteMapping* TTLSCipherSuite::CipherDetails() const
	{
	TInt cipherIndex = 0;
	
	while(cipherIndex < TLS_CIPHER_SUITES_NUMBER)  //For every cipher defined
		{
		if((KSetOfTLSCipherSuites[cipherIndex].iCipherSuite) == *this) 
			{
			return(&KSetOfTLSCipherSuites[cipherIndex]);
			}
		cipherIndex++;
		}
	return NULL;
	}

#ifndef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <tlstypedef_internal.h>
#endif

#endif //__TLSTYPEDEF_H__
