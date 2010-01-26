// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// SSL DLL entry code 
// 
//

#if !defined(__SSLCONST_H__)
#define __SSLCONST_H__

#define __USE_TLSV1__
#include <e32std.h>
#include <e32base.h>

const TInt KSSLActiveHighPriority=10;
const TInt KSSLActiveMedPriority=0;
const TInt KSSLActiveLowPriority=-10;

// this constants are arbitrary and define high limits for the length of some tmp descriptors
const TUint KHSMessMax=0x100000; //TODO isnt it too big or too small?
const TInt KMaxBlkSize=32; 
const TInt KMaxMac=32; //
const TInt KMaxPad=8; //
const TInt KOutRecBufLen=1024;
const TUint KCiphTextLengLimit=0x8000; //TODO ssl defines 
// this value as 2^14+2048 but it seems that some servers exeed this limit 
// so I put greater value. Need to check this. (Artem)

// $$VR 21/09/2001
const TUint	KHSversionOffset=4;
const TUint	KHSrandomOffset=6;
const TUint KHSrandomLength=32;
const TUint KHSsessionLenOffset=38;
const TUint KHSciperOffset=39;

// CT 18/10/00 
// Maximum size of buffers used to hold keys during exchange. 512 will allow a 4096bit key.
const TInt KMaxKeyBufferSize=512;

//error const
const TInt KErrRecProt=1;
//const KErr

/** States for the record protocol parser. 
* @since v6.0 */
enum TRecordProtocol
	{
	/** Changing ciphers. */
	EChangeCipherSpec=20,
	/** Alerting. */
	EAlert=21,
	/** Handshake. */
  	EHandshake=22,
	/** Application data. */
  	EApplicationData=23
  	};

const TUint8 KCHCiphSpecByte=1;

/** Hanshake protocol state.
* @since v6.0 */
enum THandshakeProtocol
	{
	/** Hello request. */
	EHelloRequest,
	/** Client hello. */
	EClientHello,
	/** Server hello. */
	EServerHello,
	/** Certificate. */
	ECert,
	/** Certificate request. */
	ECertRequest,
	/** Certificate verify. */
	ECertVerify,
	/** Server hello done. */
	EServerHelloDone,
	/** Server key exchange. */
	EServKeyExchange,
	/** Client key exchange. */
	ECliKeyExchange,
	/** Finished. */
	EFinished
	};

class THandshakeProtocolTag
/** @since v6.0 */
	{
public:
	THandshakeProtocol iHSMess;
	TUint8 iValue;
	};

/** @since v6.0 */
const THandshakeProtocolTag THSMessTypes[10] =
	{
		{EHelloRequest,0},
		{EClientHello,1},
		{EServerHello,2},
		{ECert,11},
		{EServKeyExchange,12},
		{ECertRequest,13},
		{EServerHelloDone,14},
		{ECertVerify,15},
		{ECliKeyExchange,16},
		{EFinished,20}
	};

/** Handshake message item. */
enum THSMessItemType 
	{
	/** Version. */
	EVersion,
	/** Random. */
	ERandom,
	/** Session ID. */
	ESessionID,
	/** Cipher suite. */
	ECipherSuite,
	/** Compression method. */
	ECompressionMethod,
	/** Parameter. */
	EParameter,
	/** Signature. */
	ESignature,
	/** Raw data. */
	ERawData,
	/** ASN1 certificate. */
	EASN1Cert,
	/** Select. */
	ESelect,
	/** End. */
	EEnd
	};

/** Handshake message item quantity. */
enum THSMessItemQuantity 
	{
	/** Single item. */
	ESingle, 
	/** Item list. */
	EList
	};
	
/** Handshake vector length. */
enum THSVectorLengthType 
	{
	/** Length fixed. */
	EFixed,
	/** Length variable. */
	EVariable
	};
	
class THSMessItem
	{
public:
	THSMessItemType iItemType;
	TInt iValue;
	THSVectorLengthType iLenType;
	THSMessItemQuantity iQuantity;
	};

/** Protocol alerts. */
enum TAlertProtocol
	{
	/** Close notification. */
	ECloseNotify,
	/** Unexpected message. */
	EUnexpectedMes,
	/** Bad MAC received. */
	EBadRecordMac,
	/** Decompression failure. */
	EDecompressionFailure,
	/** Handshake failure. */
	EHandshakeFailure,
	/** No certificate. */
	ENoCertificate,
	/** Bad certificate. */
	EBadCertificate,
	/** Unsupported certificate. */
	EUnsupportedCertificate,
	/** Certificate is revoked. */
	ETLSCertificateRevoked,
	/** Certificate is expired. */
	ECertificateExpired,
	/** Certificate unknown. */
	ECertificateUnknown,
	/** Illegal parameter. */
	EIllegalParameter
	};

/** SSL Alert levels */
enum TAlertLevel
	{
	/** Warning. */
	EWarning=1,
	/** Fatal. */
	EFatal=2
	};


class TAlertLevelTag
/** @since v6.0 */
{
public:
	TAlertProtocol iAlert;
	TUint8 iValue;
	TAlertLevel iLevel;
};

/** @since v6.0 */
const TAlertLevelTag Alerts[12]=
{
	{ECloseNotify,0,EWarning},
	{EUnexpectedMes,10,EFatal},
	{EBadRecordMac,20,EFatal},
	{EDecompressionFailure,30,EFatal},
	{EHandshakeFailure,40,EFatal},
	{ENoCertificate,41,EWarning},
	{EBadCertificate,42,EWarning},
	{EUnsupportedCertificate,43,EWarning},
	{ETLSCertificateRevoked,44,EWarning},
	{ECertificateExpired,45,EWarning},
	{ECertificateUnknown,46,EWarning},
	{EIllegalParameter,47,EFatal}
};

class TTLSVersion
/** @since v6.0 */
	{
public:
	TUint8 iMajor;
	TUint8 iMinor;
	};

const TTLSVersion SSLv3Version= {3,0};
const TTLSVersion TLSv1Version= {3,1};
//const TTLSVersion DefaultVersion= TLSv1Version;

//#define RSA_PKCS1_PADDING	11
//#define RSA_SSLV23_PADDING	12

// Client/Server Hello frame constants,
const TUint KServerHelloMajorOffset = 0;
const TUint KServerHelloMinorOffset = 1;
const TUint KHelloRandomLength = 32;
//const TUint KServerHelloRandomOffset = 2;
//const TUint KServerHelloSessionIDLengthOffset = 34;
//const TUint KServerHelloSessionIDOffset = 35;
//const TUint KServerHelloCipherSuiteLengthOffset = 35;

// Server cert frame constants
const TInt KServerCertOffset= 0x00;  

//Look RFC 2104 for explanation of ipad & opad
const TUint8 KIpad=0x36;
const TUint8 KOpad=0x5C;
//pg Note
//These following definitions used to be part of the previous crypto lib 
/** Symmetric cipher types 
*
* @since v6.0 */
enum TSymmetricCipherType
	{
	/** default cipher. */
	EDefaultCipher,
	/** null cipher. */
	ENullCipher,
	/** DES in ecb mode, iv=0, block=8, key= 8. */
	EDes_ecb,
	/** DES in ecb ede mode, iv=0, block=8, key=16. */
	EDes_ede,
	/** DES in ecb ede mode, iv=0, block=8, key=24. */
	EDes_ede3,
	/** DES in cfb mode, iv=8, block=1, key= 8. */
	EDes_cfb,
	/** DES in ede cfb mode, iv=8, block=1, key=16. */
	EDes_ede_cfb,
	/** DES in ede cfb mode, iv=8, block=1, key=24. */
	EDes_ede3_cfb,
	/** DES in ofb mode, iv=8, block=1, key= 8. */
	EDes_ofb,
	/** DES in ede ofb mode, iv=8, block=1, key=16. */
	EDes_ede_ofb,
	/** DES in ede ofb mode, iv=8, block=1, key=24. */
	EDes_ede3_ofb,
	/** DES in cbc mode, iv=8, block=8, key= 8. */
	EDes_cbc,
	/** DES in cbc ede mode, iv=8, block=8, key=16. */
	EDes_ede_cbc,
	/** DES in cbc ede mode, iv=8, block=8, key=24. */
	EDes_ede3_cbc,
	/** DES in desx cbc mode,iv=8, block=8, key=24 */
	EDesx_cbc,
	/** ARC4, iv=0, block=1, key=16. */
	ERc4,
	/** RC5 in ecb mode. */
	ERc5_ecb,          // RC5, 
	/** RC5 in cbc mode. */
	ERc5_cbc,
	/** RC5 in cfb mode. */
	ERc5_cfb,
	/** RC5 in ofb mode. */
	ERc5_ofb,
	/** IDEA in ecb mode, iv=0, block=8, key=16. */
	EIdea_ecb,
	/** IDEA in cfb mode, iv=8, block=1, key=16. */
	EIdea_cfb,
	/** IDEA in ofb mode, iv=8, block=1, key=16. */
	EIdea_ofb,
	/** IDEA in cbc mode, iv=8, block=8, key=16. */
	EIdea_cbc,
	/** RC2 in ecb mode. */
	ERc2_ecb,
	/** RC2 in cbc mode. */
	ERc2_cbc
//	ERc2_cfb,
//	ERc2_ofb
	};

/** Message digest types. */
enum TMessageDigestType
	{
	/** Default. */
	EDefaultDigest,
	/** None. */
	EMdNull,
	/** RSA sign/verify. */
	EMd5,
	/** RSA sign/verify. */
	ESha,
	/** RSA sign/verify. */
	ESha1,
	/** DSA sign/verify. */
	EDss,
	/** DSA sign/verify. */
	EDss1,
	/** MAC sign/verify. */
	EHMac
	};

/** Public key algorithm. */
enum TPublicKeyAlg 
	{
	/** RSA encryption. */
	ERsa,
	/** DH encryption. */
	EDh,
	/** DSA encryption. */
	EDsa
	};

class TAlgorithmInfo
	{
public:
	TInt iType;//TODO enum
	TInt iSize;
	};


#endif

