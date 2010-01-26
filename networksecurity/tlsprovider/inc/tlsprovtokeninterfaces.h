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

/**
 @file 
 @publishedPartner
 @released
*/

#ifndef __TLSPROVTOKENINTERFACES_H__
#define __TLSPROVTOKENINTERFACES_H__


#include <tlstypedef.h>
#include <hash.h>
#include <ct.h>

#ifndef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <tlsprovtokeninterfaces_internal.h>
#endif

// forward declarations 
class CCTCertInfo;
class CCTKeyInfo;

/**@ingroup tlsprovider */
class MTLSTokenProvider : public MCTTokenInterface
/**
This an interface to a cryptographic token that is capable to provide services 
for TLS/SSL protocols. 
@publishedPartner
@released
*/
	{
 public:
	
	virtual const TDesC& Label() = 0;

	/**
	Provides information about session associated with given server 
	indicated by aServerName argument. If a session has previously been established 
	to that server and remains held in the cache of the token and is resumable then 
	on completion, the aOutputSessionData will contain that session id, cipher 
	suite and protocol version information. 
	The aAcceptableCipherSuites and aAcceptableProtVersions parameters allow to impose
	additional conditions on the retrieved session. That is if any of them is non-empty
	then returned aOutputSessionData.iSessionId will be non trivial only when 
	the cipher suite and/or protocol version of cached session match some 
	cipher suite/protocol version included in the lists passed in the arguments.
	
	If no resumable session to the server is cached or cached sessions don't satisfy 
	conditions imposed by a caller, aOutputSessionData.iSessionId will be zero length. 
		

	@released
	@param aServerName the name of the server the TLS protocol is intending to connect to
	@param aAcceptableProtVersions	a list of protocol versions, can be empty
	@param aOutputSessionData session data retrieved from the cache 
	@param aStatus asynchronous request status set on the completion 
	@see CTLSProvider::GetSession
	*/
	virtual void GetSession(
		const TTLSServerAddr& aServerName,
		RArray<TTLSProtocolVersion>& aAcceptableProtVersions,
		TTLSSessionData& aOutputSessionData, 
		TRequestStatus& aStatus) = 0;

	/**
	Clears a session identified by aServerName and aSession.iId from associated token's
	cache.
	The operation may fail if session cached in the token is in use. 
	@released
	@param aServerName the name of the server the TLS protocol tried to connect to
	@param aSession the session identifier object relating to the server 
					indicated in the first parameter
	@param aResult a result of the operation: ETrue if cache cleared, EFalse if not
	@param aStatus asynchronous request status set on the completion 
	*/
	virtual void ClearSessionCache(
		const TTLSServerAddr& aServerName, 
		TTLSSessionId& aSession, 
		TBool& aResult, 
		TRequestStatus& aStatus) = 0;

	/** 
	Retrieves list of protocol versions, key exchange algorithms and signature algorithms
	supported by the token.
	@released
	@param aProtocols a list of supported protocol versions is returned in 
						this parameter
	@param aKeyExchAlgs a list of supported key exchange algorithms is returned in 
						this parameter
	@param aSigAlgs a list of supported signature algorithms is returned in 
						this parameter
	@param aStatus asynchronous request status set on the completion 
	*/
	virtual void CryptoCapabilities( 
		RArray<TTLSProtocolVersion>& aProtocols,
		RArray<TTLSKeyExchangeAlgorithm>& aKeyExchAlgs,
		RArray<TTLSSignatureAlgorithm>& aSigAlgs,
		TRequestStatus& aStatus) = 0;


	/**
	Cancels corresponding asynchronous request.
	@released
	*/
	virtual void CancelGetSession() = 0;

	/**
	Cancels corresponding asynchronous request.
	@released
	*/
	virtual void CancelCryptoCapabilities() = 0;

	/**
	Cancels corresponding asynchronous request.
	@released
	*/
	virtual void CancelClearSessionCache() = 0;

	};




class MTLSSession : public MCTTokenInterface
/**
This is an interface to a cryptographic token for the cryptographic services 
required by the TLS/SSL protocols. 
@publishedPartner
@released
*/
	{
 
 public:
 
 	/**
	The enumeration used by PHash method in order to identify the type of operation
	to be performed.
	@released
	*/
	enum TPHashOp { 
		/**
		Key block generation.
		*/
		EKeyBlockOp, 
		
		/**
		Calculations for Server Finished message.
		*/
		EServerFinishedOp, 

		/**
		Calculations for Client Finished message.
		*/
		EClientFinishedOp,

		/**
		Calculations for CertificateVerify message
		*/
		ECertificateVerifyOp };
 	

	virtual const TDesC& Label() = 0;
	
	
	/**
	Initialisation method. Should allocate memory for internally stored 
	information about the session. The concrete implementation of MTLSSession 
	abstract class should hold information about session identifier, cipher suite 
	and a protocol version. This information is provided in the arguments of this method.
	It is supposed that the information will be stored in class' member variables 
	(which implies that the method doesn't need to talk to asynchronous device and
	that's why the method is synchronous).
	@released
	@param aSessionNameAndID a server name and session identifier
	@param aCipherSuite a cipher suite selected for the session
	@param aCompressionMethod a compression method identifier
	@param aVersion a TLS/SSL protocol version number
	@param aResume ETrue if caller wants to resume cached session, EFalse otherwise
	@leave KTLSBadProtocolVersion if the protocol version passed in the parameter 
									is not supported by the token 
	@leave KTLSBadCipherSuite if the cipher suite passed in the parameter is not supported 
								by the token 
	@leave KTLSErrNotCached if resumption of session is attempted but appropriate entry 
							missing in the cache
	@leave KTLSErrCacheEntryInUse if resumption of session is attempted but appropriate 
									entry is set to non resumable
	@leave KErrNoMemory if it not able to allocate memory for the object's members
	*/
	virtual void InitL(
		const TTLSSessionNameAndID& aSessionNameAndID, 
		const TTLSCipherSuite& aCipherSuite,
		const TTLSCompressionMethod& aCompressionMethod,
		const TTLSProtocolVersion& aVersion,
		TBool aResume ) = 0;

		
	/** 
	Used to retrieve client certificate information from cache. 
	@released
	@param aCertInfo returned client certificate information, set to NULL if 
					no certificate information is cached
	@param aStatus asynchronous request status set on the completion 
	*/
	virtual void ClientCertificate(
				CCTCertInfo*& aCertInfo,
				TRequestStatus& aStatus) = 0; 


	/**
	Used to retrieve server certificate from a session cache. The certificates may not be available 
	if the token has no capability of storing server certificates. 
	@released
	@param aEncodedServerCert encoded server certificate retrieved from cache, 
								NULL if server certificate isn't stored in cache
	@param aStatus asynchronous request status set on the completion 
	*/
	virtual void ServerCertificate(
						HBufC8*& aEncodedServerCert,
						TRequestStatus& aStatus ) = 0;
	
	/**
	Provides the contents of ClientKeyExchange message to TLS/SSL protocol. That is,
	pre-master secret encrypted with server RSA public key in case of RSA key exchange method 
	or DH public value in case of Diffie-Hellman ephemeral key exchange method. 
	When the method completes, a pre-master secret is generated (so the implementation 
	of this method for WIM should use either WIM-KeyTransport or WIM-KeyAgreement primitive) 
	and a master secret is generated as well (which implies that also WIM-DeriveMasterSecret 
	primitive in WIM implementation should have been called). 
	When the method completes the session can be already stored in the token's cache. 
	However mustn't be set to resumable until MTLSSession::ConnectionEstablished method is called.
	It is recommended to apply the following policy of managing the cache: if there is no memory 
	for new entry in the cache the memory should be released by removing the oldest 
	unused session.
	It is probably reasonable to lock the WIM for the duration of this method but it is left as 
	an internal decision of WIM supplier to do so.

	@released
	@pre object should have been initialised by call to InitL
	@post master secret is generated
	@param aMasterSecretInput input needed for master secret generation (client and server
								random values)
	@param aClientHelloVersion protocol version number sent in client hello message
	@param aEncodedServerCert server certificate in encoded format (as received from the server)
	@param aKeyParams structure with server public key parameters retrieved from ServerKeyExchange 
						message, the type flag of the TTLSPublicKeyParams structure should indicate appropriate
						key exchange algorithm to use; the parameter should be set to NULL if 
						is irrelevant for selected cipher suite
	@param aClientKeyExch buffer with key exchange message contents (output parameter)
	@param aStatus asynchronous request status set on the completion 
	*/
	virtual void ClientKeyExchange(
						const TTLSMasterSecretInput& aMasterSecretInput,
						const TTLSProtocolVersion& aClientHelloVersion,
						const TDes8& aEncodedServerCert, 
						const CTLSPublicKeyParams* aKeyParams, 
						HBufC8*& aClientKeyExch, 
						TRequestStatus& aStatus ) = 0;

	

	/**
	Used to generate the key material for security parameters used during bulk data 
	encryption/decryption (server and client: keys, MAC secrets, IVs). The method is also
	to be used to calculate Finish Check. In case of TLS the calculations are done by TLS PRF 
	which uses master secret, label and input string to generate hash. 
	In case of SSL the calculations should be performed using algorithms specified in SSL3.0 
	specification. 
	The type of operation to be performed is indicated by an aOperation argument of type: 
	enum {EKeyBlockOp, EServerFinishedOp, EClientFinishedOp, ECertificateVerifyOp} TPHashOp. 
	This enumerated type is defined  within scope of MTLSSession class. 
	In case of keys generation, an aInputData parameter should be: 
		TLS case:
			"key expansion" + server_random + client_random,
		SSL case:
			server_random + client_random.
	In case of Finish Check calculation aInputData should be: 
		TLS case:
			"client finished" + MD5(handshake_messages) + SHA-1(handshake_messages),
			"server finished" + MD5(handshake_messages) + SHA-1(handshake_messages),
		SSL case:
			handshake_msgs + Sender.server,
			handshake_msgs + Sender.client
	(where Sender is defined as: enum { client(0x434C4E54), server(0x53525652) } Sender; - compare 
	with specification for SSL3.0).
	Apart from the above cases the method should be used in SSL case for generation of hash 
	for client's CertificateVerify message (the calculations require use of master secret). 
	In the latter case aInputData parameter should be:
		handshake_msgs.

	@released
	@pre master secret should have been generated before (by call to ClientKeyExchange)
	@param aInputData the data which is to be used as a seed for computations 
					(master secret is concatenated with aInputData),
	@param aOutput the result of the operation - key block (to be partitioned to required security 
					parameters) or finish check (depending on the input) or certificte verify check
	@param aOperation this argument indicates a type of operation that the method should perform
	@param aStatus asynchronous request status set on the completion 
	*/
	virtual void PHash(
		const TDesC8& aInputData, 
		HBufC8*& aOutput, 
		const TPHashOp& aOperation, 
		TRequestStatus& aStatus) = 0;
		
	/**
	Used to generate Finished check or Certificate Verify check in SSL case. 
	Thus the type of operation to be performed is indicated by an aOperation argument 
	(of type TPHashOp) can be either:
	EServerFinishedOp or EClientFinishedOp or ECertificateVerifyOp.
	The method takes pointers to MD5 and SHA hashing objects updated before
	the call (but not finalized) with:
		handshake_msgs + Sender.server
		or
		handshake_msgs + Sender.client
	(in case of Finished check),
		or:
		handshake_msgs
	(in case of Certificate Verify check).
	
	@released
	@param aMd5Digest the pointer to MD5 hashing object updated with appropriate input
	@param aShaDigest the pointer to SHA hashing object updated with appropriate input
	@param aOutput the result of the operation - cleint or server finish check 
					(depending on the input) 
	@param aOperation this argument indicates a type of operation that the method should perform
	@param aStatus asynchronous request status set on the completion 
	*/
	virtual void PHash(
		CMessageDigest* aMd5Digest, 
		CMessageDigest* aShaDigest,
		HBufC8*& aOutput, 
		const TPHashOp& aOperation,
		TRequestStatus& aStatus ) = 0;

	/**
	Used to sign given input with client private key. The private key comes matches the public key 
	from client certificate selected for authentication.
	@released
	@pre master secret should have been generated before (by call to ClientKeyExchange)
	@param aInput message to be signed
	@param aSignature signature of aInput (output paramter) 
	@param aCertInfo structure containing information about certificate selected for
					client authentication
	@param aKeyInfo structure containing information about private key matching public key from 
					client certificate, this key is used for signing
	@param aStatus asynchronous request status set on the completion 
	*/
	virtual void ComputeDigitalSignature(
		const TDesC8& aInput, 
		HBufC8*& aSignature, 
		CCTCertInfo& aCertInfo,
		CCTKeyInfo& aKeyInfo,
		TRequestStatus& aStatus) = 0;
		
	/**
	Called when handshake is finished. Used to inform the token whether handshake
	was successful or not. If yes, the session data will be stored in the cache and 
	marked as resumable. 
	Otherwise the session should be removed from the cache.
	Additional parameters tell whether Server and Client authentication was successful.
	This information is added to the cache.
	@released
	@pre object should have been initialised (by call to InitL) and master secret generated
		(by call to ClientKeyExchange) before call to this method
	@post session is cached in case of successful handshake 
	@param aSuccessful ETrue if handshake successfully completed, EFalse otherwise
	@param aServerAuthenticated ETrue if Server authenticated, EFalse otherwise
	@param aClientAuthenticated ETrue if Client authenticated, EFalse otherwise
	@param aStatus asynchronous request status set on the completion 
	*/
	virtual void ConnectionEstablished( TBool aSuccessful,
										TBool aServerAuthenticated,
										TBool aClientAuthenticated,
										TRequestStatus& aStatus ) = 0;

	
	
	/** 
	Cancels corresponding asynchronous request.
	@released
	*/
	virtual void CancelClientCertificate() = 0;
	
	
	/**
	Cancels corresponding asynchronous request.
	@released
	*/
	virtual void CancelServerCertificate() = 0;

	/**
	Cancels corresponding asynchronous request.
	@released
	*/
	virtual void CancelClientKeyExchange() = 0;

	/**
	Cancels corresponding asynchronous request.
	@released
	*/
	virtual void CancelPHash() = 0;

	/**
	Cancels corresponding asynchronous request.
	@released
	*/
	virtual void CancelComputeDigitalSignature() = 0;
	
	/**
	Cancels corresponding asynchronous request.
	@released
	*/
	virtual void CancelConnectionEstablished() = 0;
	
	/**
	This synchronous method provides the EAP string that is derived or 
	computed from master secret key, "client key encryption" string (or any
	string from client), client and server hello random values. The EAP
	string returned would be of 128 + 64 bytes length. 
	@pre object should have been initialised (by call to InitL) and master 
		 secret generated (by call to ClientKeyExchange) before call to this method
	@post session is cached in case of successful handshake 
	@param aLabel			   Contains the string "client key encryption".
	@param aMasterSecretInput  The structure containing "Client and Server Random" binary data.
	@param aKeyingMaterial     Contains the EAP 128 + 64 bytes string.
	*/
	virtual TInt KeyDerivation(
		const TDesC8& aLabel, 
		const TTLSMasterSecretInput& aMasterSecretInput, 
		TDes8& aKeyingMaterial);
		
	};

#ifdef _BullseyeCoverage
#pragma suppress_warnings on
#pragma BullseyeCoverage save off
#endif
inline TInt MTLSSession::KeyDerivation(	
		const TDesC8& /*aLabel*/, 
		const TTLSMasterSecretInput& /*aMasterSecretInput*/, 
		TDes8& /*aKeyingMaterial*/)
	{
	return KErrNotSupported;
	}
#ifdef _BullseyeCoverage
#pragma BullseyeCoverage restore
#pragma suppress_warnings off
#endif

/** @} */


#endif //__TLSPROVTOKENINTERFACES_H__
