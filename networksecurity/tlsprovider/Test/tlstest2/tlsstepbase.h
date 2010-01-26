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
 @file tlsstepbase.h
 @internalTechnology	
*/

#ifndef __TLSSTEPBASE_H__
#define __TLSSTEPBASE_H__

#include <e32base.h>
#include <testexecutestepbase.h>
#include <tlstypedef.h>
#include <bigint.h>
#include <asymmetrickeys.h>
#include <secdlgimpldefs.h>

#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <tlstypedef_internal.h>
#endif


#define KNServer1  _L8("192.168.30.2") 
#define KSessionId1 _L8("11111111112222222222333333333322") 

_LIT(KServerRandomFile, "ServerRandomFile");
_LIT(KClientRandomFile, "ClientRandomFile");
_LIT(KDhParamFile, "DHParamFile");

_LIT(KCipherHighByte, "CipherHighByte");
_LIT(KCipherLowByte, "CipherLowByte");

_LIT(KProtocolMajorVersion, "ProtocolMajorVersion");
_LIT(KProtocolMinorVersion, "ProtocolMinorVersion");

_LIT(KServerCert, "ServerCert");
_LIT(KDomainName, "DomainName");
_LIT(KServerKey, "ServerKey");

_LIT(KExpectedResult, "ExpectedResult");
_LIT(KExpectedCertCount, "ExpectedCertCount");

_LIT8(KLocalHost, "127.0.0.1");
_LIT(KServerSection,"serversection");

_LIT(KUseNullCipher,"UseNullCipher");
_LIT(KUsePsk,"UsePsk");
_LIT(KPskKey,"PskKey");
_LIT(KPskIdentity,"PskIdentity");
_LIT(KSessionDelay,"SessionDelay");

_LIT(KServerDNAvailable, "ServerDNAvailable");
_LIT(KInputFile, "\\t_secdlg_in.dat");
_LIT(KOutputFile, "\\t_secdlg_out.dat");
_LIT(KYes,"Yes");
_LIT(KDialogOption,"DialogOption");

class CGenericActive;
class CDecPKCS8Data;
class CTLSProvider;
class CTLSSession;
class RTlsCacheClient;

class CTlsStepBase : public CTestStep
	{
public:

	void ConstructL();
	inline CTLSProvider* Provider();
	inline CTLSSession* Session();
	inline const RArray<TTLSCipherSuite>& CipherSuites();
	
	inline const RInteger& Prime();
	inline const RInteger& Generator();
	inline const CDHKeyPair* KeyPair();
	
	inline HBufC8* ClientMacSecret();
	inline HBufC8* ServerMacSecret();
	inline HBufC8* ClientWriteSecret();
	inline HBufC8* ServerWriteSecret();
	inline HBufC8* ClientInitVector();
	inline HBufC8* ServerInitVector();

	inline TBool UseNullCipher();
	// make PSK accesible.
	inline HBufC8*  PskKey();
	inline HBufC8*  PskIdentity();
	inline TBool UsePsk();

	// Test methods
	
	TInt ClientCertificate(CX509Certificate* aCert);
	TInt ClientCertificate(HBufC8*& aCertBuf);
	TInt ClientCertificate(RPointerArray<HBufC8>* aClientCertArray);
	TInt CertificateVerifySignatureL(CMessageDigest* iMd5DigestInput, CMessageDigest* iShaDigestInput, HBufC8*& aOutput);

	TInt GetCipherSuitesL();
	TInt GetCipherSuitesWithCancelL();
	TInt GetCipherSuitesL(CTLSProvider* & aTLSProviderInstance, RArray<TTLSCipherSuite> & aCipherSuites);
	TInt VerifyServerCertificateL(CX509Certificate*& aCertOut);
	TInt VerifyServerCertificateL(CTLSProvider* & aTLSProviderInstance, CX509Certificate*& aCertOut);
	TInt VerifyServerCertificateWithCancelL(CX509Certificate*& aCertOut);
	
	TInt CreateSessionL();
	TInt CreateSessionWithCancelL();
	TInt CreateSessionL(CTLSProvider* & aTLSProviderInstance, CTLSSession* aCTLSSession);
	TInt CreateSessionAddedL(TInt aHiByte,TInt aLoByte);
	TInt VerifyGetSessionL(TTLSServerAddr& aServerName, TInt& aSessionIdLength);
	TInt VerifyGetSessionL(CTLSProvider* & aTLSProviderInstance , TTLSServerAddr& aServerName, TInt& aSessionIdLength);
	TInt ClientKeyExchange(HBufC8*& aMessageOut);
	TInt ClientKeyExchange(CTLSSession* &aCTLSSession, HBufC8*& aMessageOut);
	TInt ClientKeyExchangeWithCancel(HBufC8*& aMessageOut);
	TInt GenerateClientFinishedL(CMessageDigest* aShaDigest, CMessageDigest* aMd5Digest, HBufC8*& aMessageOut);
	TInt VerifyServerFinishedL(CMessageDigest* aShaDigest, CMessageDigest* aMd5Digest, const TDesC8& aMessage);
	TInt CipherSuiteIndex(const TTLSCipherSuite& aSuite);
	
	TInt ClearSessionCacheL(TTLSSessionNameAndID &aSessionNameAndId);
	TInt ClearSessionCacheWithCancelL(TTLSSessionNameAndID &aSessionNameAndId);
	TInt ClearSessionCacheL(CTLSProvider* & aTLSProviderInstance ,TTLSSessionNameAndID &aSessionNameAndId);
	void SessionCancelReq();
	void ProviderCancelReq();
	TInt RetrieveServerCert(CX509Certificate*& aCert);
	TInt ReadPskToBeUsedL();
	void ReadUseNullCipher();
	TInt ReadGetSessionDelayL();
	void StandardAttrInit( CTlsCryptoAttributes* tlsCryptoAttributes);
	
	// test computation
	
	HBufC8* DerivePreMasterSecretL(CTLSProvider* & aTLSProviderInstance, const TDesC8& aClientKeyExMessage);
	HBufC8* DerivePreMasterSecretL(const TDesC8& aClientKeyExMessage);
	HBufC8* ComputeMasterSecretL(CTLSProvider* & aTLSProviderInstance, const TDesC8& aPremasterSecret);
	HBufC8* ComputeMasterSecretL(const TDesC8& aPremasterSecret);
	
	HBufC8* ComputeMacL(const TDesC8& aData, TInt64 aSequenceNumber, TRecordProtocol& aType, TBool aIsServerMac);
	HBufC8* EncryptRecordL(const TDesC8& aData, TInt64 aSequenceNumber, TRecordProtocol& aType, TBool aIsServerCrypt);
	HBufC8* ComputeFinishedMessageL(CMessageDigest* aShaDigest, CMessageDigest* aMd5Digest,
	const TDesC8& aMasterSecret, TBool aClientFinished);

	HBufC8* ComputeTlsMasterSecretL(const TDesC8& aPremasterSecret);
	HBufC8* ComputeSslMasterSecretL(const TDesC8& aPremasterSecret);
	
	TInt SessionServerCertificate(CX509Certificate*& aCertOut);
	TInt SessionServerCertificateWithCancel(CX509Certificate*& aCertOut);
	
	// INI read methods
	
	void DeleteSecureDialogFilesL();
	void SetDialogRecordL(RFileWriteStream& aStream, TSecurityDialogOperation aOp, const TDesC& aLabelSpec, 
			              const TDesC& aResponse1, const TDesC& aResponse2);

	HBufC8* ServerRandomL();
	HBufC8* ClientRandomL();
	void ReadDHParamsL();
	
	TTLSCipherSuite CipherSuiteL();
	TTLSProtocolVersion ProtocolVersionL();
	TTLSSessionId SessionId();
	
	HBufC8* ServerCertificateL();
	TPtrC DomainNameL();
	CDecPKCS8Data* ServerPrivateKeyL();
	
	// secure dialog clean up.
	
	
	~CTlsStepBase();

	// PSK related 
 	TBool GetKeyFromConfigL(const TDesC& aSectName, const TDesC16& aIniValueName, TPtrC8 & aResult);
	HBufC8* StringToHexLC(const TDes8 &aString);

	TBool iUsePsk; 
	HBufC8* iPskKey;
	HBufC8* iPskIdentity;

	// null cipher setting related
	TBool iUseNullCipher;

private:
	HBufC8* ReadRandomL(const TDesC& aTag);
	
	
	
	void ComputeTlsCipherKeysL(const TDesC8& aMasterSecret, const TDesC8& aRandom);
	void ComputeSslCipherKeysL(const TDesC8& aMasterSecret, const TDesC8& aRandom);
	
	HBufC8* ComputeTlsMacL(const TDesC8& aData, TInt64 aSequenceNumber, TRecordProtocol& aType, TBool aIsServerMac);
	HBufC8* ComputeSslMacL(const TDesC8& aData, TInt64 aSequenceNumber, TRecordProtocol& aType, TBool aIsServerMac);
	
	HBufC8* ComputeTlsFinishedL(CMessageDigest* aShaDigest, CMessageDigest* aMd5Digest,
		const TDesC8& aMasterSecret, TBool aClientFinished);
	HBufC8* ComputeSslFinishedL(CMessageDigest* aShaDigest, CMessageDigest* aMd5Digest,
		const TDesC8& aMasterSecret, TBool aClientFinished);
	
private:
	CGenericActive* iActive;
	CActiveScheduler* iSched;
	
	CTLSProvider* iProvider;
	CTLSSession* iSession;
	
	RArray<TTLSCipherSuite> iSuites;
	
	// DH key if required...
	RInteger iPrime;
	RInteger iGenerator;
	CDHKeyPair* iKeyPair;
	
	// bulk cipher key params
	HBufC8* iClientMacSecret;
	HBufC8* iServerMacSecret;
	
	HBufC8* iClientWriteSecret;
	HBufC8* iServerWriteSecret;
	
	HBufC8* iClientInitVector;
	HBufC8* iServerInitVector;
		
	};

inline CTLSProvider* CTlsStepBase::Provider()
	{
	return iProvider;
	}
	
inline CTLSSession* CTlsStepBase::Session()
	{
	return iSession;
	}

inline const RArray<TTLSCipherSuite>& CTlsStepBase::CipherSuites()
	{
	return iSuites;
	}

inline const RInteger& CTlsStepBase::Prime()
	{
	return iPrime;
	}

inline const RInteger& CTlsStepBase::Generator()
	{
	return iGenerator;
	}

inline const CDHKeyPair* CTlsStepBase::KeyPair()
	{
	return iKeyPair;
	}

inline HBufC8* CTlsStepBase::ClientMacSecret()
	{
	return iClientMacSecret;
	}
	
inline HBufC8* CTlsStepBase::ServerMacSecret()
	{
	return iServerMacSecret;
	}
	
inline HBufC8* CTlsStepBase::ClientWriteSecret()
	{
	return iClientWriteSecret;
	}
	
inline HBufC8* CTlsStepBase::ServerWriteSecret()
	{
	return iServerWriteSecret;
	}
	
inline HBufC8* CTlsStepBase::ClientInitVector()
	{
	return iClientInitVector;
	}
	
inline HBufC8* CTlsStepBase::ServerInitVector()
	{
	return iServerInitVector;
	}

inline TBool CTlsStepBase::UseNullCipher()
	{
	return iUseNullCipher;
	}

inline HBufC8* CTlsStepBase::PskKey()
	{
	return iPskKey;
	}
	
inline HBufC8* CTlsStepBase::PskIdentity()
	{
	return iPskIdentity;
	}

inline TBool CTlsStepBase::UsePsk()
	{
	return iUsePsk;
	}


#endif /* __TLSSTEPBASE_H__ */
