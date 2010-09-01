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
 @internalComponent
 @released
*/

#ifndef __TLSTYPEDEF_INTERNAL_H__
#define __TLSTYPEDEF_INTERNAL_H__

#include <e32std.h>
#include <e32base.h>


#include "pkixcertchain.h"
#include <sslerr.h>
#include <tlstypedef.h>

#ifndef BULLSEYE_OFF
#ifdef _BullseyeCoverage
#define BULLSEYE_OFF "BullseyeCoverage save off";
#define BULLSEYE_RESTORE "BullseyeCoverage restore";
#else
#define BULLSEYE_OFF 
#define BULLSEYE_RESTORE 
#endif
#endif

#define KTLSMasterSecretLen 48
#define KTLSPreMasterSecretLen 48
#define KTLSMaxSymmetricKeyLen 24 
#define KTLSMaxMacSecretLen 20
#define KTLSMaxIVLen 8 

#define KTLSServerFinishedLabel _L8("server finished")
#define KTLSClientFinishedLabel _L8("client finished")
#define KTLSKeyExpansionLabel _L8("key expansion")

const TUint KAESBlockBytes = 16;

//Do not uncomment KDESBlockBytes..security comps doesnt define this anymore..
const TUint KDESBlockBytes = 8;  
const TUint KRC2BlockBytes = 8;

const TUint8 KIpad=0x36;
const TUint8 KOpad=0x5C;

//Dont change the order
/**
 * @internalAll 
 */
enum TTLSClientCertType { ENullCertType, ERsaSign, EDssSign, EDssFixedDh,ERsaFixedDh, 
						  ERsaEDH, EDssEDH, EFortezza}; 

/**
High and low sequence number of TTLS
@internalComponent
@released
*/
class TTLSSequenceNumber
	{
public:
	TUint32 iSeqHi;
	TUint32 iSeqLo;
	};

/**
Holds the description of the message.
@internalComponent
@released
*/
class TTLSMessageDigest
	{
public:
	CMessageDigest* iMd5Digest;
	CMessageDigest* iShaDigest;
	};

const TInt KMaxMac=32;
const TInt KMaxPad=8; 

// Unfortunatelty can not include #include <ssl.h> as it causes compilation errors...
class MSoPskKeyHandler;

/**
This structure hold all the information required by the provider or the token obtained form 
the handshake. The structure is gradually filled in by the protocol.
@internalAll
@released optional The following structure is incomplete and will have to be decided with nicky
Also the enums and constants, may be referenced through ssl.h
*/
class CTlsCryptoAttributes : public CBase
	{
public:
	TTLSMasterSecretInput iMasterSecretInput;
	TTLSCompressionMethod iCompressionMethod;
	TTLSCipherSuite	 iCurrentCipherSuite;
	TTLSProtocolVersion iNegotiatedProtocol;
	TTLSProtocolVersion iProposedProtocol;
	TTLSSessionNameAndID iSessionNameAndID;	
	CTLSPublicKeyParams* iPublicKeyParams;
	RArray<TTLSClientCertType> iReqCertTypes;
	RPointerArray<const TDesC8> iDistinguishedCANames; //!!in fact array of HBufC8* because of the parameters of iPtrUnifiedCertStore->List(..)
                                                      //values are deleted in ~CTlsCryptoAttributes()

	TBuf8<60> iProposedCiphers; 
	TBool iClientAuthenticate;
	TBool iDialogNonAttendedMode;  
	TTLSSignatureAlgorithm isignatureAlgorithm; 
	TBuf8<256> idomainName;	
	TTLSDialogMode iDialogMode;
	TBool iAllowNullCipherSuites;

	TBool iPskConfigured;
	MSoPskKeyHandler *iPskKeyHandler;
	HBufC8* iPskIdentityHint;

	CDesC8Array *iServerNames; ///< Optional RFC3546 server name indication - see SetOpt KSoServerNameIndication

	HBufC8* iServerDNFromCertSubject; //if iDistingshuiedCANames is not set, use iServerDNFromCert* as a backup
	HBufC8* iServerDNFromCertIssuer;
public:	
	/**
	 * @internalAll 
	 */
	static CTlsCryptoAttributes* NewL();
	/**
	 * @internalAll
	 */
	virtual ~CTlsCryptoAttributes();
private:
	CTlsCryptoAttributes();

	//option flags
	};
/**
 * @internalAll 
 */
enum TRecordProtocol{EChangeCipherSpec=20,EAlert=21, EHandshake=22,EApplicationData=23};


/**
This class hold the information of the tokens
@internalComponent
@released
*/
class CTokenInfo : public CBase
	{
public:		
	RArray<TTLSKeyExchangeAlgorithm> aKeyExchAlgs;
	RArray<TTLSSignatureAlgorithm> aSignatureExchAlgs;
	RArray<TTLSCipherSuite> iCipherSuitesSupported;
	RArray<TTLSProtocolVersion> iSupportedProtocols;
	TBool iSupported;
	
public:
	void Close();
	~CTokenInfo();
	};

#endif //__TLSTYPEDEF_INTERNAL_H__
