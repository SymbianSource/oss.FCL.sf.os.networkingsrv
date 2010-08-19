// Copyright (c) 2003-2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "Tlsprovinterface.h"
#include "tlsprovider.h"
#include <badesca.h>
#include <featdiscovery.h>
#include <featureuids.h>

//
//                                   CTlsProvider
//

EXPORT_C  CTLSProvider* CTLSProvider::ConnectL()
	{

	CTlsProviderImpl* self = CTlsProviderImpl::ConnectL();
	if(self)
      {
   	CleanupStack::PushL(self);
		CTLSProvider *prov = new (ELeave)CTLSProvider(self);
   	CleanupStack::Pop(self);
      return prov;
      }
	else
		return NULL;
	}

CTLSProvider::CTLSProvider(CTlsProviderImpl* aProviderPtr)
	{
	iTlsProviderImpl = aProviderPtr;
	}

/**
This synchronous method is used to request a number of randomly generated bytes from 
the TLS Provider
4bytes of time + 28 bytes of random numbers

RFC: The current time and date in standard UNIX 32-bit format (seconds
since the midnight starting Jan 1, 1970, GMT) according to the sender's internal clock. 
Clocks are not required to be set correctly by the basic TLS Protocol; higher level or application
protocols may define additional requirements. 
But having a 32bit signed format will mean we will encounter a "Year 2038 problem" 
@param aBuffer A buffer of Random bytes in the aBuffer argument.
@return None

*/
EXPORT_C void CTLSProvider::GenerateRandom(TDes8& aBuffer)
	{
	iTlsProviderImpl->GenerateRandom(aBuffer);
	}

/**
This method provides a CTLSSession object to the protocol when the handshake has reached
the stage where ServerHelloDone has been recevied. The main process of this function is to
select the token and initialize it.
If Client Authentication was required, appropriate cert and key selection will be done. If running in a 
DialogNonAttended Mode, then a certificate will be automatically selected. Pls note that ClientAuthentication
can only be done with Dialog mode set to true 
If Client Authentication was not required, the Software Token will be selected and if any
failures were encountered during its initialization, then the other available tokens will
be entertained. If a call to GetSession was made earlier and if the SessionId passed from Server matches
the one returned earlier, then this function attempts an Abbreviated handshake

@see CTlsCryptoAttributes
@param aTlsCryptoAttributes The result of the operation - List of supported cipher 
							suites ordered according to their priorities	
@param aStatus asynchronous request status set on the completion 
@pre CipherSuites or GetSession APIs should be called atleasts once 
@post CTLSSession object and the token is now initialised. If for client authentication, appropriate
	  certificate has been selected
@return void Asynchronous function

*/
EXPORT_C void CTLSProvider::CreateL(CTLSSession*& aTlsSession,TRequestStatus& aStatus)
	{
	iTlsSession = &aTlsSession;
	iTlsProviderImpl->CreateL(aTlsSession,aStatus);
	}
		
/**
This asynchronous method reads the list of supported cipher suites from the 
available tokens. This list is used within the SSL/TLS Protocol's ClientHello 
message. The list of cipher suites is constructed from the symmetric ciphers and 
digest algorithms supported by the Security subsystem and the union of all the 
key exchange and signature algorithms supported by the available tokens. If a previous call 
to GetSession was made, then the token information obtained with it would be used to get this list.

A list created in this way is then checked against iSupported and iPriority fields 
of TSetOfCipherSuites entries and unsupported cipher suites are removed from the list while the
rest are ordered according to the priorities assigned in TSetOfCipherSuites. (It is guaranteed that 
all cipher suites in the TSetOfCipherSuites table marked as supported are implemented in Symbian software
emulation of cryptographic token for both SSL v3.0 and TLS v1.0 protocols. The selection of cipher suite 
in the way described here ensures that whatever cipher suite and protocol version is eventually negotiated
by the handshake peers, there will always be a token capable of providing required service).

@param aUserCipherSuiteList Output - List of supported cipher 
							suites ordered according to their priorities							
@param aStatus asynchronous request status set on the completion 
@pre None
@post The token handles obtained are not released and stored for further iterations
@return void Asynchronous function

*/
EXPORT_C void CTLSProvider::CipherSuitesL(
		RArray<TTLSCipherSuite>& aUserCipherSuiteList, 
		TRequestStatus& aStatus)
	{
	iTlsProviderImpl->CipherSuitesL(aUserCipherSuiteList,aStatus);
	}


/**
This asynchronous method is used to validate a received Server certificate.
If the validation fails then an UI will prompt the user to decide whether to
continue with this handshake or cancel it.A dialog will be popped if the server certificate
is invalid. The user then has the choice to close the connection or continue.
The dialog mode can be turned off by setting the iDialogNonAttendedMode mode flag in
CTlsCryptoAttributes 

@see CTlsCryptoAttributes
@param aEncodedServerCerts A string read from ServerCertificate handshake message
@param aServerCert	Output - Server certificate create from the encoded form
@param aStatus asynchronous request status set on the completion 
@return void Asynchronous function

*/
EXPORT_C void CTLSProvider::VerifyServerCertificate(
		const TDesC8& aEncodedServerCerts, 
		CX509Certificate*& aServerCert,		  			
		TRequestStatus& aStatus)
	{
	iTlsProviderImpl->VerifyServerCertificate(aEncodedServerCerts,aServerCert,aStatus);
	}


/**
This synchronous method uses the server's public key to ensure that the 
signature was indeed created by the server 

@return Boolean value with the result of the verification, ETrue if positively verified,
		otherwise EFalse.
@param aServerPublicKey The server's public key; used to decrypt the signature created
						with the server's private key.							
@param aDigest The data which we expect to extract from the server's signature when decrypted 
@param aSignature The server's signature
@pre None
@post None
@return ETrue if positively verified, otherwise EFalse

*/
EXPORT_C TBool CTLSProvider::VerifySignatureL(
		const CSubjectPublicKeyInfo& aServerPublicKey, 
		const TDesC8& aDigest, 
		const TDesC8& aSig)
	{
	return(iTlsProviderImpl->VerifySignatureL(aServerPublicKey,aDigest,aSig));
	}


/**
This asynchronous method will provide a session identifier object associated with a 
given server indicated by the aServerName argument. If a session has previously been
established to that server and remains held in the cache of one of the available tokens
and is resumable, then the aSessionId parameter will contain that session id to that session. 
The client (SSL/TLS protocol) can attempt to perform an abbreviated handshake in this case. If no
resumable session to the server is cached, then the session aSessionId member will be zero length, 
which would force the protocol to perform a full handshake. 

For performace issues, if this is the first function to be called in a CTlsProviderImpl, then
as we obtain the interface to the token, the Key and signature exchange algorithms are also
retrieved from the token and any subsequent calls to CTlsProviderImpl::CipherSuites() will
complete from this existing list.


@param aServerName The name of the server the SSL/TLS protocol is intending to connect.
@param aSessionId  output - the ID corresponding the session retrieved
@param aStatus asynchronous request status set on the completion 
@post If a sessionId is obtained, then the details of the token will be stored locally
@return void Asynchronous function

*/
EXPORT_C void CTLSProvider::GetSessionL(	
		TTLSServerAddr& aServerName,
		TTLSSessionId& aSessionId,
		TRequestStatus& aStatus) 
	{
	iTlsProviderImpl->GetSessionL(aServerName,aSessionId,aStatus);
	}


/**
This asynchronous method will clear the session identified by Server Name and its Id.(This data is 
available to the caller as a result of the call to the GetSession() method). This operation may fail
if the session cached on a token is in use (this maybe possible if a hardware token, e.g. a WIM is used.)
When an abbevated handshake fails, Tls provider will ask the corresponding token to clear the cache.
TlsProvider keeps track of this by storing the output from the last GetSession call.

For an explicit call to this API, the function internally will call GetSession to determine the
token associated with this session and then attempts to clear it.

@param aServerNameAndId  Name of the Server and its corresponding sessionId that have to be flushed.
@param aStatus asynchronous request status set on the completion 
@return void Asynchronous function

*/
EXPORT_C void CTLSProvider::ClearSessionCacheL(
		TTLSSessionNameAndID& aServerNameAndId, 		
		TRequestStatus& aStatus)
	{
	iTlsProviderImpl->ClearSessionCacheL(aServerNameAndId,aStatus);
	}


EXPORT_C CTlsCryptoAttributes* CTLSProvider::Attributes()
	{
	return (iTlsProviderImpl->Attributes());
	}

/**
This method returns a pointer to an CTLSSession object. 
@param none
@pre Create should be called successfully, otherwise a NULL pointer is returned
*/
EXPORT_C CTLSSession* CTLSProvider::TlsSessionPtr()
	{
   if(iTlsSession)
      return *iTlsSession;
   else
      return NULL;
	}
	
	
/**
This synchronous method cancels all the asynchronous operations of CTlsProvider

@param none
@return void
*/
EXPORT_C void CTLSProvider::CancelRequest()
	{
	iTlsProviderImpl->CancelRequest();
	}


/**
This synchronous method saves the handshake parameters, creates a new implementation
object and intitializes its CryptoAttributes with the handshake parameters. Also it 
retains a handle to the token and thereby holds on to the last saved session.
The function is used for renegotiation and for abbreviated handshakes where handshake
params differ but session has to live linger

@param none
@return void
*/
EXPORT_C void CTLSProvider::ReConnectL()
	{

	//Get hold of the token handle before deleting the impl objects, which will save the session for us.
	MCTToken* tNewToken = iTlsProviderImpl->GetTokenHandle();

	if(tNewToken)
		{
		if(iTokenInterface)
			iTokenInterface->Release();
		iTokenInterface = tNewToken;
		tNewToken = NULL;
		}
		
	if(TlsSessionPtr())
   	{
      ((TlsSessionPtr()->Attributes())->iMasterSecretInput).iClientRandom = (Attributes()->iMasterSecretInput).iClientRandom;
   	}

	TBool allowUntrustedCertificates = EFalse;
	allowUntrustedCertificates = CFeatureDiscovery::IsFeatureSupportedL(NFeature::KFeatureIdFfHttpAllowUntrustedCertificates);

	// Save old implementation object
	CTlsProviderImpl *oldTlsProviderImpl = iTlsProviderImpl;
	iTlsProviderImpl = NULL;	
	CleanupStack::PushL(oldTlsProviderImpl);
	CTlsCryptoAttributes* oldAttr = oldTlsProviderImpl->Attributes();

	//Create a new implementation object
	iTlsProviderImpl = CTlsProviderImpl::ConnectL();

	if(oldAttr)
		{
		// Copy user configuration options from the old impl

		CTlsCryptoAttributes* oldAttr = oldTlsProviderImpl->Attributes();

		Attributes()->iProposedProtocol = oldAttr->iProposedProtocol;
		Attributes()->iProposedCiphers = oldAttr->iProposedCiphers;
		
		if( allowUntrustedCertificates )
			{
			Attributes()->iDialogMode = oldAttr->iDialogMode;
			}
		else
			{
			Attributes()->iDialogNonAttendedMode = oldAttr->iDialogNonAttendedMode;
			}

		Attributes()->idomainName = oldAttr->idomainName;

		// Copy NULL ciphersuite setting
   		Attributes()->iAllowNullCipherSuites = oldAttr->iAllowNullCipherSuites;

 		// Copy PSK settings
   		Attributes()->iPskConfigured = oldAttr->iPskConfigured;
   		Attributes()->iPskKeyHandler = oldAttr->iPskKeyHandler;
		Attributes()->iPskIdentityHint = (oldAttr->iPskIdentityHint) ? (oldAttr->iPskIdentityHint->AllocL()) : (0);

  		CDesC8Array *oldServerNames = oldAttr->iServerNames;
  		if(oldServerNames)
  			{
  			TInt count = oldServerNames->Count();
			CDesC8ArrayFlat *serverNamesCopy = new(ELeave) CDesC8ArrayFlat(count);
			CleanupStack::PushL(serverNamesCopy);

			for(TInt i=0; i<count; ++i)
				{
				serverNamesCopy->AppendL((*oldServerNames)[i]);
				}
			
			Attributes()->iServerNames = serverNamesCopy;
			CleanupStack::Pop(serverNamesCopy);
			}
		}

	// Delete the old impl
	CleanupStack::PopAndDestroy(oldTlsProviderImpl);
	oldTlsProviderImpl = 0;
	oldAttr = 0;
	
	if(TlsSessionPtr())
		{
		Attributes()->iProposedCiphers = (TlsSessionPtr()->Attributes())->iProposedCiphers;
		(Attributes()->iMasterSecretInput).iClientRandom = ((TlsSessionPtr()->Attributes())->iMasterSecretInput).iClientRandom;
		Attributes()->iProposedProtocol = (TlsSessionPtr()->Attributes())->iProposedProtocol;
		Attributes()->iNegotiatedProtocol = (TlsSessionPtr()->Attributes())->iNegotiatedProtocol;
		if( allowUntrustedCertificates )
			{
			Attributes()->iDialogMode = (TlsSessionPtr()->Attributes())->iDialogMode;
			}
		else
			{
			Attributes()->iDialogNonAttendedMode = (TlsSessionPtr()->Attributes())->iDialogNonAttendedMode;
			}

      	Attributes()->iSessionNameAndID.iServerName = (TlsSessionPtr()->Attributes())->iSessionNameAndID.iServerName;
      	Attributes()->iSessionNameAndID.iSessionId = (TlsSessionPtr()->Attributes())->iSessionNameAndID.iSessionId;

		}
	iTlsSession = NULL;	
	}

EXPORT_C CTLSProvider::~CTLSProvider()
	{
	if(iTokenInterface)
		iTokenInterface->Release();
	if(iTlsProviderImpl)
		delete iTlsProviderImpl;
	}


//
//                                   CTlsSession
//

EXPORT_C CTLSSession* CTLSSession::NewL(CTlsSessionImpl* aSessionPtr)
	{
	if(aSessionPtr)
		{
		CTLSSession* self;
		self = new (ELeave)CTLSSession(aSessionPtr);		
		return self;  
		}
	else
		return NULL;

	}

CTLSSession::CTLSSession(CTlsSessionImpl* aSessionPtr)
	{
	iTlsSessionImpl = aSessionPtr;
	}


/**
This method returns the ClientKeyExchange message.
The following functionalities are executed
The ClientKeyExchange operation includes the following 

Inside the token:
1. The generation of the pre-master secret,
2. The derivation of the master secret, 
3. Generation of keys for bulk data encryption and MAC secrets, 
4. ClientKeyExchange message created

Inside the Provider
1. If weak Export cipher is used, generation of Weak keys
2. Creation of CTlsSessionImpl's member objects responsible for symmetric encryption and HMAC hashing. 
3. ClientKeyExchange message returned


@param aClientKeyExch Output - Client key Exchange message is returned
@param aStatus asynchronous request status set on the completion 
@post Encryption,Decryption and HMAC objects are created
@return void Asynchronous function
@internalTechnology
*/
EXPORT_C void CTLSSession::ClientKeyExchange(		
		HBufC8*& aClientKeyExch,			
		TRequestStatus& aStatus)
	{
	iTlsSessionImpl->ClientKeyExchange(aClientKeyExch,aStatus);
	}


/**
This asynchronous method is used to retrieve a client certificate in its encoded form,
which would be normally send out to the server. If the connection has reached a stage
where the session would have been cached, then this API would retrieve the information
(CCTCertInfo) from token's cache and returns the retrieved certificate from the Certstore.
In all other case, the information (CCTCertInfo) is stored locally

@param aEncodedClientCert Output -  Encoded client sertificate 
@param aStatus asynchronous request status set on the completion 
@return void Asynchronous function
@internalTechnology
*/
EXPORT_C void CTLSSession::ClientCertificate(
		HBufC8*& aEncodedClientCert,
		TRequestStatus& aStatus)
	{
	iTlsSessionImpl->ClientCertificate(aEncodedClientCert,aStatus);
	}


/**
This asynchronous method is used to retrieve a client certificate in an X509 form,
which would be normally used for displaying it to the user. If the connection has reached
a stage where the session would have been cached, then this API would retrieve the information
(CCTCertInfo) from token's cache and returns the retrieved certificate from the Certstore.
In all other case, the information (CCTCertInfo) is stored locally.

@param aX509ClientCert Output -  Client certificate in returned in an x509 format
@param aStatus asynchronous request status set on the completion 
@return void Asynchronous function
@internalTechnology
*/
EXPORT_C void CTLSSession::ClientCertificate(
		CX509Certificate*& aX509ClientCert,
		TRequestStatus& aStatus)
	{
	iTlsSessionImpl->ClientCertificate(aX509ClientCert,aStatus);
	}


/**
This asynchronous method is used to retrieve a client certificate chain in its encoded form, which would be normally send 
out to the server. If the connection has reached a stage where the session would have been cached, then this API would 
retrieve the information (CCTCertInfo) from token's cache and returns the retrieved certificate from the Certstore. In all 
other case, the information (CCTCertInfo) is stored locally
@param aClientCertArray- The client certificate chain used in this session in encoded form
@param aStatus - Asynchronous request status value
*/
EXPORT_C void CTLSSession::ClientCertificate(
	RPointerArray<HBufC8>* aClientCertArray,
	TRequestStatus& aStatus)
	{
	iTlsSessionImpl->ClientCertificate(aClientCertArray, aStatus);
	}


/**
This asynchronous method is used to retrieve the associated server certificate of the current
session in its X509 form, which would be normally used for displaying it to the user. If the
connection has reached a stage where the session would have been cached, then this API would
retrieve the certificate from the Token. In all other case, the certificate is stored locally.

@param aX509ServerCert Output -  Server certificate returned in an x509 format
@param aStatus asynchronous request status set on the completion 
@return void Asynchronous function
@internalTechnology
*/
EXPORT_C void CTLSSession::ServerCertificate(
		CX509Certificate*& aX509ServerCert,
		TRequestStatus& aStatus)
	{
	iTlsSessionImpl->ServerCertificate(aX509ServerCert,aStatus);
	}


EXPORT_C void CTLSSession::ClientFinishedMsgL(		
		CMessageDigest* aMd5DigestInput,
		CMessageDigest* aShaDigestInput,
		HBufC8*& aOutput, 
		TRequestStatus& aStatus)
	{
	iTlsSessionImpl->ClientFinishedMsgL(aMd5DigestInput,aShaDigestInput,aOutput,aStatus);
	}


/**
This asynchronous method verifies SSL/TLS protocol's Server 'Finished' message. 
This method generates ServerFinished message given as an input hash of concatenation of all 
handshake messages (as specified by RFC2246 and SSL3.0 specification). In order to create required 
output, the function calls MTLSSesssion::PHash with the following input string:
TLS Protocol
	"client finished" + iMd5DigestInput + iShaDigestInput

SSL Protocol
	(iMd5DigestInput +" SRVR") + (iShaDigestInput +" SRVR")

@param iMd5DigestInput Md5 hash of Handshake message
@param iShaDigestInput Md5 hash of Handshake message
@param aActualFinishedMsg Server's received 'Finished' message  
@param aStatus asynchronous request status set on the completion 
@return void Asynchronous function
@internalTechnology
*/
EXPORT_C void CTLSSession::VerifyServerFinishedMsgL(	
		CMessageDigest* aMd5DigestInput,
		CMessageDigest* aShaDigestInput,	
		const TDesC8& aActualFinishedMsg,  
		TRequestStatus& aStatus)
	{
	iTlsSessionImpl->VerifyServerFinishedMsgL(aMd5DigestInput,aShaDigestInput,aActualFinishedMsg,aStatus);
	}

/**
This asynchronous method generates a signature of the given input with client private key. Used in
client certificate verify message for both SSL/TLS case. This input for this message  is a hash of 
concatenation of all the handshake messages exchanged thus far (as specified by RFC2246 and SSL3.0 
specification). The hash of the handshake is finalised and then send to the token for signing.

@param iMd5DigestInput Md5 hash of Handshake message
@param iShaDigestInput Md5 hash of Handshake message
@param aOutput Client's Certificate Verify message
@param aStatus asynchronous request status set on the completion 
@return void Asynchronous function
@internalTechnology
*/
EXPORT_C void CTLSSession::CertificateVerifySignatureL(
		CMessageDigest* aMd5DigestInput,
		CMessageDigest* aShaDigestInput,
		HBufC8*& aOutput, 
		TRequestStatus& aStatus)
	{
	iTlsSessionImpl->CertificateVerifySignatureL(aMd5DigestInput,aShaDigestInput,aOutput,aStatus);
	}


/**
On receiving an input data, this synchronous method computes the MAC, appends it into the input and
encrypts the whole block. The encryption and decryption objects should have been created
in an earlier call to ClientKeyExchange before calling this API. the MAC computations
differ for TLS and SSL. 

@param aInput Data to be encrypted
@param aOutput	Encrypted output
@param aSeqNumber Sequence number of the packet
@param aType Record protocol type, e.g: handshake, alerts, application data, etc
@pre ClientKeyExchange method should have been called 
@see CTlsSessionImpl::ClientKeyExchange
@return TInt Returns an SSL/TLS specific Alert code in case of an error
@leave Symbian Wide error code
@internalTechnology
*/
EXPORT_C TInt CTLSSession::EncryptL(
		const TDesC8& aInput,
		HBufC8*& aOutput,
        TInt64& aSeqNumber,
		TRecordProtocol& aType)
	{
	return (iTlsSessionImpl->EncryptL(aInput,aOutput,aSeqNumber,aType));
	}


/**
On receiving an input data, this synchronous method decrypts the data, parses the actual data from
its MAC and verifies it. If the MAC is verified correctly, then the output will be returned
The encryption and decryption objects should have been created in an earlier call to 
ClientKeyExchange before calling this API


@param aInput Data to be Decrypted
@param aOutput	Dencrypted output
@param aSeqNumber Sequence number of the packet
@param aType Record protocol type, e.g: handshake, alerts, application data, etc
@pre ClientKeyExchange method should have been called 
@see CTlsSessionImpl::ClientKeyExchange
@return TInt Returns an SSL/TLS specific Alert code in case of an error
@leave Symbian Wide error code
@internalTechnology
*/
EXPORT_C TInt CTLSSession::DecryptAndVerifyL(
		const TDesC8& aInput,
		HBufC8*& aOutput,
		TInt64& aSeqNumber,
		TRecordProtocol& aType)
	{
	return (iTlsSessionImpl->DecryptAndVerifyL(aInput,aOutput,aSeqNumber,aType));
	}

/**
This method returns the CTlsCryptoAttributes structure associated with this session
@param None
@return pointer to CTlsCryptoAttributes structure
*/
EXPORT_C CTlsCryptoAttributes* CTLSSession::Attributes()
	{
	return (iTlsSessionImpl->Attributes());
	}

/**
This synchronous method provides the EAP string that is derived or 
computed from master secret key, "client key encryption" string (or any
string from client), client and server hello random values. The EAP
string returned would be of 128 + 64 bytes length. 
@pre object should should have been initialised (by call to InitL) and master 
	 secret generated (by call to ClientKeyExchange) before call to this method
@post session is cached in case of successful handshake 
@param aLabel			   Contains the string "client key encryption".
@param aMasterSecretInput  The structure containing "Client and Server Random" binary data.
@param aKeyingMaterial     Contains the EAP 128 + 64 bytes string.
@see CTlsSessionImpl::ClientKeyExchange
@return TInt Returns an SSL/TLS specific Alert code in case of an error.
@internalTechnology
*/
EXPORT_C TInt CTLSSession::KeyDerivation(
		const TDesC8& aLabel, 
		const TTLSMasterSecretInput& aMasterSecretInput, 
		TDes8& aKeyingMaterial)
	{
	if (iTlsSessionImpl == NULL)
		{
		return KErrNotReady;
		}
	else
		{
		return (iTlsSessionImpl->KeyDerivation(aLabel,aMasterSecretInput,aKeyingMaterial));
		}
	}
	
/**
This synchronous method cancels all the asynchronous operations of CTLSProvider

@param none
@return void
@internalTechnology
*/
EXPORT_C void CTLSSession::CancelRequest()
	{
	iTlsSessionImpl->CancelRequest();
	}

EXPORT_C CTLSSession::~CTLSSession()
	{
	delete iTlsSessionImpl;
	}

