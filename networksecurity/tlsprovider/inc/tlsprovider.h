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
// This file contains types shared between TLS protocol module and
// Security's component: TLS Provider. 
// 
//

/**
 @file 
 @internalTechnology
*/

#ifndef __TLSPROVIDER_H__
#define __TLSPROVIDER_H__


#include <e32std.h>
#include <e32base.h>

#include "3des.h"
#include "rijndael.h"
#include "cbcmode.h"
#include "padding.h"
#include "blocktransformation.h"
#include "bufferedtransformation.h"
#include "arc4.h"
#include "ct.h"
#include "pkixcertchain.h"
#include "x509keys.h"
#include <random.h>
#include <hash.h>

#include "tlstypedef.h"
#include "tlsprovtokeninterfaces.h"
#include "tlsprovider_log.h"
#include "CTlsEncrypt.h"
#include "Ctlsclntauthenticate.h"
#include "Ctlsbrowsetoken.h"

#include <ct/rmpointerarray.h>
#include <mctkeystore.h>
#include "cctcertinfo.h"
#include "tlscacheclient.h"

#include "Tlsprovinterface.h"

#ifdef _USESECDLGSV_
#include "SECDLGCL.H"
#else
#include "secdlg.h"
#endif

#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <tlstypedef_internal.h>
#include <tlsprovtokeninterfaces_internal.h>
#endif

const TInt KUidUnicodeSSLProtocolModule = 0x1000183d;  //INCLUDE SSL.H


class TCTTokenHandle;
class CPKIXCertChain;
class CX509Certificate;
class CPKIXValidationResult;
class CSymmetricCipher;

class CMessageDigest;
class CTlsEncrypt;


//
//  CTlsSessionImpl
//

class MTLSSession;


class CTlsSessionImpl : public CActive
	{
public:
	static CTlsSessionImpl* NewL(
		MTLSSession* aSessionInterface,
		CCTCertInfo* aSelectedCertInfo,
		CCTKeyInfo* aSelectedKeyInfo,
		RPointerArray<CCertificate>* aStoredIntermediatesCACertificates);
	

	void ConstructL(		
		CTlsCryptoAttributes* aTlsCryptoAttributes, 
		HBufC8*  aEncodedServerCerts,					
		TRequestStatus& aStatus);

	void ConstructResumedL(
		CTlsCryptoAttributes* aTlsCryptoAttributes,		
		TRequestStatus& aStatus);

	void ClientKeyExchange(		
		HBufC8*& aClientKeyExch,			
		TRequestStatus& aStatus);

	void ClientCertificate(
		HBufC8*& aEncodedClientCert,
		TRequestStatus& aStatus);


	void ClientCertificate(
		CX509Certificate*& aX509ClientCert,
		TRequestStatus& aStatus);

	void ClientCertificate(
		RPointerArray<HBufC8>* aClientCertArray,
		TRequestStatus& aStatus);


	void ServerCertificate(
		CX509Certificate*& aX509ServerCert,
		TRequestStatus& aStatus);

	void CertificateVerifySignatureL(
		CMessageDigest* aMd5DigestInput,
		CMessageDigest* aShaDigestInput,
		HBufC8*& aOutput, 
		TRequestStatus& aStatus);

	void ClientFinishedMsgL(		
		CMessageDigest* aMd5DigestInput,
		CMessageDigest* aShaDigestInput,
		HBufC8*& aOutput, 
		TRequestStatus& aStatus);


	void VerifyServerFinishedMsgL(	
		CMessageDigest* aMd5DigestInput,
		CMessageDigest* aShaDigestInput,	
		const TDesC8& aActualFinishedMsg,  
		TRequestStatus& aStatus);


	TInt EncryptL(
		const TDesC8& aInput,
		HBufC8*& aOutput,
        TInt64& aSeqNumber,
		TRecordProtocol& aType);
	

	TInt DecryptAndVerifyL(
		const TDesC8& aInput,
		HBufC8*& aOutput,
		TInt64& aSeqNumber,
		TRecordProtocol& aType);

	TInt KeyDerivation(
		const TDesC8& aLabel, 
		const TTLSMasterSecretInput& aMasterSecretInput, 
		TDes8& aKeyingMaterial);

	CTlsCryptoAttributes* Attributes() ;

		
	void CancelRequest();
	
	~CTlsSessionImpl();
private:

	enum TStateLists {  ENullState,EConstruct, EGetClientCerificate,EGetClientKeyExchange,
						EGetServerCertificate,EKeyGeneration,EClientFinishedMsg,
						EVerifyServerFinishedMsg,EComputeDigitalSignature,
						EConnectionEstablished,EGetClientCerificateX509,ECertificateVerifyMsg,EReturnCert,EGetClientCertificateArray};

	TTLSMasterSecretInput iMasterSecretInput;
	TTLSProtocolVersion iProtocolVersion;
	TTLSCipherSuite  iCipherSuiteId;
	

	
	//Helper variables
	TStateLists iOriginalState;
	TStateLists iCurrentState;
	TStateLists iNextState;	
	TInt iServerMsgVerified;
	TInt iAttribute;
	
	RFs iFs;
	
	//Data containers
	HBufC8* iKeyMaterial;
	HBufC8* iEncodedServerCerts;
	HBufC8* iEncodedClientCert;
	HBufC8** iEncodedClientCertHldrPtr;
	
	HBufC8** iComputeDigitalSig;
	HBufC8* iTempHolder;
	HBufC8* iServerCert_rv;
	HBufC8* iServerFinished; 
	HBufC8* iActualFinishedMsg; //Should move it to a comming pointer variable
	
		
	//Caller values
	TRequestStatus* iOriginalRequestStatus;
	CX509Certificate** iClientCertX509;
	CX509Certificate** iX509ServerCert;
	HBufC8** iClientKeyExch;

	//Handles
	MTLSSession* iSessionInterface;
	CTlsCryptoAttributes* iTlsCryptoAttributes;
	CUnifiedCertStore* iPtrUnifiedCertStore;
	CCTCertInfo* iSelectedCertInfo;
	CCTKeyInfo* iSelectedKeyInfo;
	CTlsEncrypt* iEncrypt;
	TBool iAbbrievatedHandshake;
	
	TPtr8 iTempPtr;

 	RPointerArray<CCertificate>* iStoredIntermediatesCACertificates;

 	TBool iConstructionComplete;
	RPointerArray<HBufC8>* iClientCertArray;

private:
	CTlsSessionImpl();

	//Active
	void DoCancel();
	void RunL();
	TInt RunError(TInt aError);
	
	void GetX509CertL(HBufC8*& aEncodedCert,CX509Certificate*& aOutputX509);
	void GenerateFinishedMessageL(CMessageDigest* aMd5DigestInput,
										  CMessageDigest* aShaDigestInput,
										  HBufC8*& aOutput,
										  TBool aIsServer);
	void GenerateKeysL();

	void BuildClientIntermediateCertChainL(RPointerArray<CCertificate> &aCertChain,
										   const CX509Certificate* aClientCert) const;
										   
	TBool MatchRequestedIssuerDN(const CCertificate* aCert) const;

};



//
//  CTlsProviderImpl
//

class MTLSTokenProvider;


class CTokenTypesAndTokens : public CBase
	{
public:
	MTLSTokenProvider* iProviderInterface; 
	CTokenInfo* iTokenInfo;
	TInt iTotalTokenCount;
	TBool iSoftwareToken;	
public:
	void Release();
	~CTokenTypesAndTokens();
	};

class TSessiondata 
	{
public:
	TTLSSessionId	iSessionId;
	MTLSTokenProvider* iProviderInterface; 
	};


const TUid UidProv = { KInterfaceTLSTokenProvider };
const TUid UidSess = { KInterfaceTLSSession };

class CTlsProviderImpl : public CActive
	{
public:
	static CTlsProviderImpl* ConnectL();

	
	void CreateL( 
		CTLSSession*& aTlsSession,		
		TRequestStatus& aStatus);
		

	void CipherSuitesL(
		RArray<TTLSCipherSuite>& aUserCipherSuiteList, 
		TRequestStatus& aStatus);


	void VerifyServerCertificate(
		const TDesC8& aEncodedServerCerts, 
		CX509Certificate*& aServerCert,		  			
		TRequestStatus& aStatus);


	TBool VerifySignatureL(
		const CSubjectPublicKeyInfo& aServerPublicKey, 
		const TDesC8& aDigest, 
		const TDesC8& aSig);


	void GenerateRandom(TDes8& aBuffer);


	void GetSessionL(	
		TTLSServerAddr& aServerName,
		TTLSSessionId& aSessionId,
		TRequestStatus& aStatus) ;


	void ClearSessionCacheL(
		TTLSSessionNameAndID& aServerNameAndId, 		
		TRequestStatus& aStatus);

	CTlsCryptoAttributes* Attributes();

	CTlsSessionImpl* TlsSessionPtr();
	
	
	void CancelRequest();

	MCTToken* GetTokenHandle();

	//Constructor and Destructor
	CTlsProviderImpl();
	~CTlsProviderImpl();
	
private:

	enum TStateLists {	ENullState,ECreate,EGetCiphers,EValidateCertificate,
						EClearSessionCache,EOpenToken,EGetSession,EGetSessionInterface,
						EStartSession,EGetKeyAndSignExAlgrthm,EConstructResumed,
						EConstruct,EClientAuthenticate,EBrowseTokens,EQueryCache,EUserDialog,ENextOrEnd};

	
	//Data containers
	RArray<CTokenTypesAndTokens> iListAllTokensAndTypes;
	HBufC8* iEncodedServerCerts;
	TSessiondata iSessionData;


	//Helper variables	
	TStateLists iOriginalState;
	TStateLists iCurrentState;	
	TStateLists iNextState;	

	//Flags
	TBool iAbbreviatedHandshake;
	

	TInt iTotalTokenTypeCount;
	TInt iCurrentTokentype;
	TInt iCurrentToken;
	TInt iSelectedTypeIndex;
	RFs iFs;

	//Key and certstore helpers		
	CCTKeyInfo* iSelectedKeyInfo;
	CCTCertInfo* iSelectedCertInfo;
 	RPointerArray<CCertificate>	iStoredIntermediatesCACertificates;

	//Handles
	CTlsCryptoAttributes* iTlsCryptoAttributes;
#ifdef _USESECDLGSV_
	RSecurityDialogServer iDialogServ;
	TBool iProceed;
#else
	MSecurityDialog* iSecurityDialog;
#endif
	RTlsCacheClient iCacheClient;
	TValidationStatus iValidationStatus;
	CPKIXCertChain* iServerCertsChain;
	CPKIXValidationResult* iCertVerificationResult;
	
	//Class Handles	
	CTlsClntAuthenticate* iClntAuthenticate;
	CTlsBrowseToken* iPtrTokenSearch;


	//Caller values
	TRequestStatus* iOriginalRequestStatus;
	CX509Certificate** iX509ServerCert;
	CTlsSessionImpl*  iTlsSessionImpl;
	CTLSSession**   iTlsSessionHldr;
	RArray<TTLSCipherSuite>* iUserCipherSuiteList;
	TTLSSessionData iOutputSessionData;
	TTLSSessionNameAndID iServerNameAndId;
	TTLSServerAddr* iPServerName;
	TTLSSessionId* iPSessionId;
	MTLSSession* iSessionInterface;
	TBool iTlsSessionOwnershipPassedToCaller;

	RArray<TTLSProtocolVersion> iReqProtList;
	RArray<TTLSCipherSuite> iSupportedCipherSuiteList;
	CTlsProviderPolicy* iTlsProviderPolicy;
	
private:	
	

	//Active
	void ConstructL();
	void DoCancel();
	void RunL();	

	TInt RunError(TInt aError);
	
	void GetX509CertL(HBufC8*& aEncodedCert,CX509Certificate*& aOutputX509);
	TBool ValidateDNSNameL(const CX509Certificate& aSource);
	TBool NameIsInSubtree(CX509DNSName& aServerName, CX509DNSName& aCertName, TBool aIsWildcard);

	//Local functions	
	void  NextOrEnd();
	void  GetAvailableKeyListL();
	void  ReturnCipherListL();
	void  ReturnSession();
	TBool SelectToken();
   TBool IsCipherAvailable( const TTLSCipherSuiteMapping& aCipherSuiteMapping ) const;

	void ShowUntrustedDialogL(const TValidationStatus aResult);
	void HandleBadCertificateL(const TValidationStatus aResult);
	TBool CheckExtendedKeyUsageL(const CX509Certificate& aSource);
	
	//Active Handlers
	void OnEGetSession();
	void OnEStartSession();	
	void OnEBrowseTokens();
	void OnEGetSessionInterfaceL();
	void ReturnResult();
	void RetrieveSession();
	void OnQueryCacheL();
	void OnEUserDialogL();

#ifdef _DEBUG
	enum TPanic
		{
		ERCLBadUserOrder = 0x10, ERCLBadTokenOrder
		};
	static void Panic(TPanic aPanic);
#endif	
	
	};


#endif //__TLSPROVIDER_H__






