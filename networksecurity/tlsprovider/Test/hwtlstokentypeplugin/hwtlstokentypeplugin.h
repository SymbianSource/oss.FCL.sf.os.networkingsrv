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


#ifndef __HWTLSTOKENTYPEPLUGIN_H__
#define __HWTLSTOKENTYPEPLUGIN_H__


#include "tlsprovtokeninterfaces.h"

#include <e32base.h>
#include <ecom/ecom.h>
#include <implementationproxy.h>
#include <ct.h>

#include <hash.h>
#include <x500dn.h>
#include <mctcertstore.h>
#include <mctkeystore.h>
#include <unifiedkeystore.h>
#include <e32def.h>

#include "hwtlstoken_log.h"

#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <tlstypedef_internal.h>
#include <tlsprovtokeninterfaces_internal.h>
#endif


_LIT(KHwTlsTokenInfo, "Hardware TLS Token");


const TInt KHwTlsTokenTypeUid = 0x102827D1;
					
const TInt KTLSCacheSize = 3 ;

const TInt KTLSCachingTimeout = 180; //for test set the timeout as 3mnts									 

_LIT8(KTLSMasterSecretLabel, "master secret");

// the constants below denote sizes of different parameters in bytes
const TInt KTLSPreMasterLenInRsa = 48;

const TInt KTLSFinishedMsgLen = 12;

//the pad_1 defined in section 5.2.3.1. in SSL 3.0 spec
const TUint8 KSSLPad1 =	0x36; 
//the pad_2 defined in section 5.2.3.1. in SSL 3.0 spec
const TUint8 KSSLPad2 =	0x5c;

//	pad_1           The character 0x36 repeated 48 times for MD5
//                  or 40 times for SHA.
//  pad_2           The character 0x5c repeated 48 times for MD5
//					or 40 times for SHA.
//#define KSSLNoOfPadsMd5 48
const TInt  KSSLNoOfPadsMd5 = 48;
//#define KSSLNoOfPadsSha	40
const TInt  KSSLNoOfPadsSha = 40;


//#define KSSLSaltStringsMaxNo 20 // max conceivable size of salt used 
						// in number of SSL calculations of master secret
const TInt KSSLSaltStringsMaxNo = 20;
_LIT8( KSSLSaltSource, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");

#define KSSLMaxInputToSignLen (SHA1_HASH + MD5_HASH)


// forward declaration
class CHwTlsToken;

class CHwTlsTokenProvider : public CBase, public MTLSTokenProvider
	{
public:
	CHwTlsTokenProvider(const TDesC& aLabel, CHwTlsToken& aToken )
			: iLabel(aLabel), iToken(aToken) {};

	// virtual methods inherited from MCTTokenInterface
	virtual MCTToken& Token();

	virtual const TDesC& Label();
	
	// virtual methods inherited from MTLSTokenProvider
	virtual void GetSession(
		const TTLSServerAddr& aServerName,		
		RArray<TTLSProtocolVersion>& aAcceptableProtVersions,
		TTLSSessionData& aOutputSessionData, 
		TRequestStatus& aStatus);
	
	virtual void ClearSessionCache(
		const TTLSServerAddr& aServerName, 
		TTLSSessionId& aSession, 
		TBool& aResult, 
		TRequestStatus& aStatus);
		
	virtual void CryptoCapabilities( 
		RArray<TTLSProtocolVersion>& aProtocols,
		RArray<TTLSKeyExchangeAlgorithm>& aKeyExchAlgs,
		RArray<TTLSSignatureAlgorithm>& aSigAlgs,
		TRequestStatus& aStatus);

	virtual void CancelGetSession();
	
	virtual void CancelCryptoCapabilities();

	virtual void CancelClearSessionCache();
	
private:
	const TDesC& iLabel;

public:
	CHwTlsToken& iToken;
		
	};
	

class CHwTLSSession : public CActive, public MTLSSession
	{

public:

	CHwTLSSession(const TDesC& aLabel, CHwTlsToken& aToken);
			
	~CHwTLSSession();

	// virtual methods inherited from MCTTokenInterface
	virtual MCTToken& Token();

	virtual const TDesC& Label();
	
	
	// virtual methods inherited from MTLSSession
	virtual void InitL(
		const TTLSSessionNameAndID& aSessionNameAndID, 
		const TTLSCipherSuite& aCipherSuite,
		const TTLSCompressionMethod& aCompressionMethod,
		const TTLSProtocolVersion& aVersion,
		TBool aResume );
		
			
	virtual void ClientCertificate(
				CCTCertInfo*& aCertInfo,
				TRequestStatus& aStatus);
				
	virtual void ServerCertificate(
					HBufC8*& aEncodedServerCert,
					TRequestStatus& aStatus );
					
	virtual void ClientKeyExchange(
						const TTLSMasterSecretInput& aMasterSecretInput,
						const TTLSProtocolVersion& aClientHelloVersion,
						const TDes8& aEncodedServerCert, 
						const CTLSPublicKeyParams* keyParams, // pointer - because can be NULL,
						//TTLSServerPublicKeyParams - the type with flag: RSA or DH then value1, value2, value3
						// of which 1,2 are rsa_modulus, rsa_exponent in RSA case, or 1,2,3 are dh_p, dh_g, dh_Ys 
						// in DH case
						HBufC8*& aClientKeyExch, 
						TRequestStatus& aStatus );

	virtual void PHash(
		const TDesC8& aInputData, 
		HBufC8*& aOutput, 
		const TPHashOp& aOperation, 
		TRequestStatus& aStatus);
		
	virtual void PHash(
		CMessageDigest* aMd5Digest, 
		CMessageDigest* aShaDigest,
		HBufC8*& aOutput, 
		const TPHashOp& aOperation,
		TRequestStatus& aStatus );
		
	virtual void ComputeDigitalSignature(
		const TDesC8& aInput, 
		HBufC8*& aSignature, 
		CCTCertInfo& aCertInfo,
		CCTKeyInfo& aKeyInfo,
		TRequestStatus& aStatus);
		
	virtual void ConnectionEstablished( TBool aSuccessful,
										TBool aServerAuthenticated,
										TBool aClientAuthenticated,
										TRequestStatus& aStatus );

	virtual void CancelClientCertificate();
	
	virtual void CancelServerCertificate();

	virtual void CancelClientKeyExchange();

	virtual void CancelPHash();

	virtual void CancelComputeDigitalSignature();
	
	virtual void CancelConnectionEstablished();
	
	virtual TInt KeyDerivation(
		const TDesC8& aLabel, 
		const TTLSMasterSecretInput& aMasterSecretInput, 
		TDes8& aKeyingMaterial);
		
	// virtual methods inherited from CActive
	virtual void DoCancel();
	virtual void RunL();
	virtual TInt RunError(TInt aError);
	
private: 
	void ClientKeyExchangeL(
						const TTLSMasterSecretInput& aMasterSecretInput,
						const TTLSProtocolVersion& aClientHelloVersion,
						const TDes8& aEncodedServerCert, 
						const CTLSPublicKeyParams* keyParams, 
						HBufC8*& aClientKeyExch );
						
	void DHEClientKeyExchL( const CTLSPublicKeyParams* keyParams, 
								HBufC8*& aPremaster,
								HBufC8*& aClientKeyExch );
								
	void RSAClientKeyExchL( const CTLSPublicKeyParams* keyParams, 
							const TDes8& aEncodedServerCert,
							const TTLSProtocolVersion& aClientHelloVersion,
							HBufC8*& aPremaster,
							HBufC8*& aClientKeyExch );	
							
	void SSLMasterSecretComputationsL( const TTLSMasterSecretInput& aMasterSecretInput,
										const TDesC8& aPremaster);				
							
	void TLSPHashComputationsL(
			const TDesC8& aInputData, 
			HBufC8*& aOutput, 
			const TPHashOp& aOperation);
			
	void SSLPHashComputationsL(
			CMessageDigest* aMd5Digest, 
			CMessageDigest* aShaDigest,
			HBufC8*& aOutput, 
			const TPHashOp& aOperation);
			
	void SSLKeyBlockComputationsL(
			const TDesC8& aInputData, 
			HBufC8*& aOutput);
			
	void SSLFinishedCheckComputationsL(
			CMessageDigest* aMd5Digest, 
			CMessageDigest* aShaDigest,
			HBufC8*& aOutput);
				   
	void SSLCertVerifyComputationsL(
			CMessageDigest* aMd5Digest, 
			CMessageDigest* aShaDigest,
			HBufC8*& aOutput);
			
	void TLSPRFL( const TDesC8& aSecret,
				 const TDesC8& aLabelAndSeed,
				 const TInt aLen, 
				 TDes8& aOut);
				 
	void TLSPRFComputationsL(
		   const TDesC8& aSecret,
		   const TDesC8& aSeed,
		   const TTLSMACAlgorithm& aMacAlg, //sha-1 is default
		   const TInt aLen, // number of bytes to produce
		   TDes8& aOut);
		   
	void FormatDsaSignatureL( CDSASignature* aDsaSignature,
							  HBufC8*& aOutput );
	
	void KeyDerivationL(
		const TDesC8& aLabel, 
		const TTLSMasterSecretInput& aMasterSecretInput, 
		TDes8& aKeyingMaterial);
	
	void ResetWorkingVars();
	void Reset();
private:
	// token data			
	const TDesC& iLabel;
public:
	CHwTlsToken& iToken;
private:

	enum TRequestIssued 
		{
		EIdle,
					
		EInitKeyStoreForSigning,
		
		EComputeRsaSignature,
		ESignWithRsa,
		EComputeDsaSignature,
		ESignWithDsa
		
		};

			
	
	// session data:
	TTLSSessionNameAndID* iSessionNameAndID;
	TTLSCipherSuiteMapping* iCipherSuite; 
	TTLSCompressionMethod* iCompressionMethod;
	TTLSProtocolVersion* iProtocolVersion;
	TBuf8<KTLSMasterSecretLen>* iMasterSecret;
	
	CCTCertInfo* iClientCertInfo;
	TCTTokenObjectHandle iKeyObjectHandle; 
	TBool iResumed;
	
	
	// members connected with processing of async events
	TBool iInitialised;
	TRequestIssued iState;
	TRequestStatus* iOriginalRequestStatus;
	TBool iCertSelected; 
	
	RFs iFs;
		
	// working variables:
	CUnifiedKeyStore* iUnifiedKeyStore; 
		
	MRSASigner* iRsaSigner;
	MDSASigner* iDsaSigner;
		
	// variables for computation of digital signature:
	HBufC8** iSignatureOutput;
	TBuf8<KSSLMaxInputToSignLen> iInputToSign;
	
	CRSASignature* iRsaSignature;
	CDSASignature* iDsaSignature;
	
	};


// forward reference
class CHwTLSSessionCache;

class CHwTlsToken : public CBase, public MCTToken
	{
public:

	class CHwTLSCache
		{
		public:
		TInt iCounter;
		RPointerArray<CHwTLSSessionCache>* iCacheArray;
		};

	CHwTlsToken(const TDesC& aLabel, CCTTokenType& aTokenType);
	~CHwTlsToken();
	
	static CHwTlsToken* NewL(const TDesC& aLabel, CCTTokenType& aTokenType);

	// virtual methods inherited from MCTToken
	virtual void DoGetInterface(TUid aRequiredInterface,
							  MCTTokenInterface*& aReturnedInterface, 
							  TRequestStatus& aStatus);
	virtual TBool DoCancelGetInterface();

	virtual const TDesC& Label();

	virtual MCTTokenType& TokenType();

	virtual TCTTokenHandle Handle();
	virtual const TDesC& Information(TTokenInformation aRequiredInformation);
	virtual void DoRelease();
	
	// SSL/TLS specific:
	TBool AddToCacheL (
		TTLSSessionData& aData, 
		TTLSSessionNameAndID& aSessNam, 
		TDes8& aMasterSecret,
		const TDes8& aEncodedServerCert // can be empty
		); 

	TBool RemoveFromCache( 
					 const TTLSServerAddr& aServerName,
					 const TDes8& aSessionId );

	void GetCacheData( 
		const TTLSServerAddr& aServerName,		
		RArray<TTLSProtocolVersion>& aAcceptableProtVersions,
		TTLSSessionData& aOutputSessionData	);

	CHwTLSSessionCache* SessionCache(
								const TTLSServerAddr& aServerName,
								TDes8&	aSessionId);
	
protected:
	// virtual method from MCTToken
	virtual TInt& ReferenceCount();
	
private: 
	void InitThreadLocStorageL(); 
	void ReadResourceFileL( );
	
public:
	CCTTokenType& iTokenType;	
	TInt iCount;
private:
	const TDesC& iLabel;
		
	RCPointerArray<HBufC>* iStrings;
	
	// session cache stored in thread local storage area
	CHwTLSCache* iCache; 
	
	};
	
	
class CHwTlsTokenType : public CCTTokenType
	{
public:
	
	static CHwTlsTokenType* InitL();

	// virtual methods inherited from MCTTokenType
	virtual void List(RCPointerArray<HBufC>& aTokens, 
					  TRequestStatus& aStatus);
	virtual void CancelList();
	virtual void OpenToken(const TDesC& aTokenInfo, MCTToken*& aToken, 
						   TRequestStatus& aStatus);
	virtual void OpenToken(TCTTokenHandle aHandle, MCTToken*& aToken, 
						   TRequestStatus& aStatus);
	virtual void CancelOpenToken();

	virtual ~CHwTlsTokenType();
protected:

	HBufC* iTokenInfo;
	};
	
	
		

class CHwTLSSessionCache : public CBase
{
	
public:

	static CHwTLSSessionCache* NewL(
		TTLSServerAddr& aServerAddr, 
		TTLSSessionData& aData, 
		TDes8& aMasterSecret,
		HBufC8* aEncodedCert );	

	~CHwTLSSessionCache();
	
	void SetValues( TTLSServerAddr& aServerAddr, 
		TTLSSessionData& aData, 
		TDes8& aMasterSecret,
		HBufC8* aEncodedCert );
	
	void AddKeyInfo( const TCTTokenObjectHandle& aClientKeyObject,
					CCTCertInfo* aClientCertInfo );

	void SetResumable(
		TBool aResumable,
		TBool aServerAuthenticated, 
		TBool aClientAunthenticated);
		
	TBool IsResumable();

	const TTLSServerAddr ServerAddr();
	const TTLSSessionData ReadData();
	HBufC8* ServerCertificate(); // returned in encoded format
	CCTCertInfo* ClientCertificate(); // can be NULL
	const TCTTokenObjectHandle ClientKeyHandle();
	TInt64 CreationTime();
	
	const TPtrC8 MasterSecret();
		
private:
		
	
	TTLSServerAddr iServerAddr;
	TTLSSessionData iSessionData;
	TCTTokenObjectHandle iClientKeyObject;
	TBuf8<KTLSMasterSecretLen> iMasterSecret;
	HBufC8* iEncodedServerCert;
	CCTCertInfo* iClientCertInfo;
	TInt64 iCreationTime;
	TBool	iResumable;
	
};

#endif // __HWTLSTOKENTYPEPLUGIN_H__
