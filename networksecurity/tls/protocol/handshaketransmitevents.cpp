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
// Implementation file for transmitted handshake events - ClientHello, 
// ClientKeyExchange, ClientCertificate, CertificateVerify, and Client Finished
// messages.
// 
//

/**
 @file
*/

#include "tlshandshakeitem.h"
#include "handshaketransmitevents.h"
#include "recordprotocolevents.h"
#include "tlshandshake.h"
#include <tlstypedef_internal.h>
#include <ssl_internal.h>

CHandshakeTransmit::~CHandshakeTransmit()
{
	LOG(Log::Printf(_L("CHandshakeTransmit::~CHandshakeTransmit()"));)
   
	iRecordComposer.SetUserData( NULL );
	iRecordComposer.ResetCurrentPos();
	delete iHandshakeMessage;
	iHandshakeMessage = NULL;
}

void CHandshakeTransmit::ComposeHandshakeHeader( TInt aHistoryUpdate, ETlsHandshakeMessage aHandshakeMessage, TDesC8& aDesComposeMsg )
/**
 * This method updates the State Machine's history, composes a header for the 
 * handshake message and sets the Record protocol content type.
 */
{
	LOG(Log::Printf(_L("CHandshakeTransmit::ComposeHandshakeHeader()"));)

	iStateMachine->UpdateHistory( aHistoryUpdate );
	HandshakeMessage()->ComposeHeader( aHandshakeMessage, aDesComposeMsg.Length() - KTlsHandshakeHeaderSize );
	iRecordComposer.SetRecordType( ETlsHandshakeContentType );

	Handshake().UpdateVerify( aDesComposeMsg );

	// Set the protocol version in the Record to the negotiated protocol, set the 
	// data buffer in the Record composer to the data in the handshake message, 
	// set up the next asynchronous event.
	iRecordComposer.SetUserData( &aDesComposeMsg );
}

CClientCertificate::~CClientCertificate()
/**
 * Destructor.
 */
{
	LOG(Log::Printf(_L("CClientCertificate::~CClientCertificate()"));)
	iCertArray.ResetAndDestroy();
}

CClientKeyExch::~CClientKeyExch()
/**
 * Destructor.
 */
{
	LOG(Log::Printf(_L("CClientKeyExch::~CClientKeyExch()"));)
	delete iKeyExchBuf;
}

CCertificateVerify::~CCertificateVerify()
/**
 * Destructor.
 */
{
	LOG(Log::Printf(_L("CCertificateVerify::~CCertificateVerify()"));)
	delete iSignature;
}


CSendFinished::~CSendFinished()
/**
 * Destructor.
 */
{
	LOG(Log::Printf(_L("CSendFinished::~CSendFinished()"));)
	delete iFinishedMsg;
	delete iShaHashPtr;
	delete iMd5HashPtr;
}

//
//
CClientHello::~CClientHello()
{
	iCipherList.Close();
}

CAsynchEvent* CClientHello::ProcessL( TRequestStatus& aStatus )
/**
 * This asynchronous method processes a ClientHello message. This message 
 * initiates Handshake negotiation.
 *
 * It creates a CClientHelloMsg object which contains the items needed for a ClientHello 
 * message. The CClientHelloMsg constructor initialises the object with some basic values. 
 * Other message items will need to be retrieved via the TLS Provider and will
 * be used to complete construction of this object.
 * 
 * The required items are a session id (if one currently exists), client random  
 * value, proposed protocol version, cipher suites and compression algorithm.
 * 
 * @param aStatus TRequestStatus object
 * @return CAsynchEvent* Pointer to the next asynchronous event to be processed
 */
{
	LOG(Log::Printf(_L("CClientHello::ProcessL()"));)

	// Get a reference to CHandshake::iComposeMsg (TPtr8 descriptor). This 
	// pointer descriptor is used to access the CStateMachine::iFragment heap
	// descriptor and contains the message to be transmitted.
	TPtr8& ptrComposeMsg = Handshake().ComposeMsg();

	switch ( iClientHelloStates )
	{
		case ETlsGetSessionInfo:
		{
			LOG(Log::Printf(_L("CClientHello::ProcessL() - ETlsGetSessionInfo"));)

			iClientHelloStates = ETlsGetCiphers;
			__ASSERT_DEBUG( !iHandshakeMessage, TlsPanic(ETlsPanicHandshakeMsgAlreadyExists));
			CTlsCryptoAttributes& cryptoAttributes = *iTlsProvider->Attributes();

			if(!cryptoAttributes.iServerNames)
				{
				// If this type is changed be very careful to check all the code which uses iHandshakeMessage...
				iHandshakeMessage = new(ELeave)CClientHelloMsg;
				}
			else
				{
				// If this type is changed be very careful to check all the code which uses iHandshakeMessage...
				iHandshakeMessage = new(ELeave)CClientHelloWithExtensionsMsg;
				}
			LOG(Log::Printf(_L("iHandshakeMessage %x - %x"), iHandshakeMessage, (TUint)iHandshakeMessage + sizeof( CClientHelloMsg ));)

			Handshake().GetServerAddrInfo( cryptoAttributes.iSessionNameAndID.iServerName );

			cryptoAttributes.iSessionNameAndID.iSessionId.Zero();		// Reset the length.
			iTlsProvider->GetSessionL(cryptoAttributes.iSessionNameAndID.iServerName, 
			cryptoAttributes.iSessionNameAndID.iSessionId, aStatus);

			break;
		}
		case ETlsGetCiphers:
		{
			LOG(Log::Printf(_L("CClientHello::ProcessL() - ETlsGetCiphers"));)
			
			iClientHelloStates = ETlsComposeHello;
			__ASSERT_DEBUG( iHandshakeMessage, TlsPanic(ETlsPanicNullHandshakeMsg) );
      
			// Get the list of available ciphers the Provider.
			iTlsProvider->CipherSuitesL(iCipherList, aStatus); 
			break;
		}
		case ETlsComposeHello:
		{
			LOG(Log::Printf(_L("CClientHello::ProcessL() - ETlsComposeHello"));)
			
			iClientHelloStates = ETlsGetSessionInfo;
			__ASSERT_DEBUG( iHandshakeMessage, TlsPanic(ETlsPanicNullHandshakeMsg));

			CTlsCryptoAttributes &cryptoAttributes = *iTlsProvider->Attributes();
      
			// Get and set the variable length (CVariableItem) parameters.
			// Cipher suites
			TDes8& desCipherSuites = iTlsProvider->Attributes()->iProposedCiphers;
			if ( desCipherSuites.Length() == 0 )
			{
				// The user has not already setup the ciphersuite list via CSecureSocket::SetAvailableCipherSuites
				// so we set it to all the ciphersuites which are supported and configured.
				for ( TInt loop = 0; loop < iCipherList.Count(); ++loop )
				{
					const TTLSCipherSuite &cipherSuite = iCipherList[loop];
						
					desCipherSuites.Append( cipherSuite.iHiByte );
					desCipherSuites.Append( cipherSuite.iLoByte );
				}
			}
			iCipherList.Close();
					
			// Get the Compression and session id
			// Compute and set the length of the Variable length items.
			const TDesC8& desCompression = iRecordComposer.SupportedCompression();
			TDes8& desSessionId = iTlsProvider->Attributes()->iSessionNameAndID.iSessionId;

			CClientHelloMsg* pHelloMsg = (CClientHelloMsg*)iHandshakeMessage;
			pHelloMsg->iCipherSuite.Header().SetInitialValue( desCipherSuites.Length() );
			pHelloMsg->iCompression.Header().SetInitialValue( desCompression.Length() );
			pHelloMsg->iSessionId.Header().SetInitialValue( desSessionId.Length() );

			CClientServerNameExtension *serverNameExtension = 0;
			if(cryptoAttributes.iServerNames)
				{
				CClientHelloWithExtensionsMsg* pHelloExtMsg = static_cast<CClientHelloWithExtensionsMsg*>(pHelloMsg);

				serverNameExtension = CClientServerNameExtension::NewLC();
				pHelloExtMsg->AddExtensionL(serverNameExtension);
				CleanupStack::Pop(serverNameExtension);

				TInt serverNameCount = cryptoAttributes.iServerNames->Count();
				for(TInt i=0; i<serverNameCount; ++i)
					{
					const TPtrC8 &serverName = (*cryptoAttributes.iServerNames)[i];
					CClientServerNameEntry *serverNameEntry = CClientServerNameEntry::NewLC(serverName.Length());
					serverNameExtension->AddServerNameEntryL(serverNameEntry);
					CleanupStack::Pop(serverNameEntry);
					}
				}
      
			// Compute the length of the Client Hello message and allocate an 
			// appropriate-sized buffer.
			HBufC8* pFragment = iStateMachine->ReAllocL( pHelloMsg->iRecord.CalcTotalInitialiseLength() );
			ptrComposeMsg.Set( pFragment->Des() );
			ptrComposeMsg.Zero();
		
			// Initialise each item's pointer to point to the start of its part of 
			// the buffer and set each items' value.
			pHelloMsg->iRecord.InitialiseL( ptrComposeMsg ); 
      
			pHelloMsg->iCipherSuite.SetBody( desCipherSuites ); 
			pHelloMsg->iCompression.SetBody( desCompression );
			pHelloMsg->iSessionId.SetBody( desSessionId );

			iBody.Set( pHelloMsg->iVersion.GetBodyDes() );
			iBody[0] = iTlsProvider->Attributes()->iProposedProtocol.iMajor;
			iBody[1] = iTlsProvider->Attributes()->iProposedProtocol.iMinor;

			// Generate random directly into message buffer and set the value in the
			// Provider also.
			iBody.Set( pHelloMsg->iRandom.GetBodyDes() );
			iTlsProvider->GenerateRandom( iBody );
			iTlsProvider->Attributes()->iMasterSecretInput.iClientRandom = iBody;

			if(serverNameExtension)
				{
				// Set type field
				serverNameExtension->SetBigEndian( CExtensionNode::KExtServerName );

				TInt serverNameCount = cryptoAttributes.iServerNames->Count();
				for(TInt i=0; i<serverNameCount; ++i)
					{
					// Get server name
					const TPtrC8 &serverName = (*cryptoAttributes.iServerNames)[i];

					// Create the server name entry
					CClientServerNameEntry *entry = serverNameExtension->Node(i);
					// Set type to DNS
					entry->SetBigEndian( CClientServerNameEntry::KExtServerNameTypeDns );

					TPtr8 serverNameDes(0,0);
					serverNameDes.Set( entry->iName.GetBodyDes() );

					serverNameDes = serverName;
					}
				}
		
			// Create handshake header, update the history, and set the record protocol
			// content type. Hard-coded start using SSL3.0 framing to support TLS->SSL fall
			// back. Set the Record composer's data buffer to the handshake message and set
			// up the next asynchronous event.
			ComposeHandshakeHeader( ETlsClientHelloSent, ETlsClientHelloMsg, ptrComposeMsg );
			CRecordComposer& recordComposer = iRecordComposer; //'this' will be deleted
            if ( !recordComposer.TlsVersion() )
                {
                // Always default to the newest version, if the server doesn't
                // support it we will get an SSL3.0 server hello and we will
                // rollback to that
                recordComposer.SetVersion( &KTLS1_0 );
                }
            else
                {
                recordComposer.SetVersion( &iTlsProvider->Attributes()->iProposedProtocol );
                }


			LOG(Log::Printf(_L("CClientHello message"));)
#ifdef _DEBUG
      	LOG(iHandshakeMessage->iRecord.Dump( KSSLLogDir,KSSLLogFileName);)
#endif
			recordComposer.SetNext( Handshake().InitiateReceiveL() );
			// !'this' is now deleted !
			return recordComposer.ProcessL( aStatus );
		}
		default:
		{
			LOG(Log::Printf(_L("CClientHello::ProcessL() - Default case statement - ERROR"));)
			__ASSERT_ALWAYS( EFalse, TlsPanic(ETlsPanicInvalidProcessState));
			break;
		}
	}
	return this;
}

//
//
CAsynchEvent* CClientKeyExch::ProcessL( TRequestStatus& aStatus )
/**
 * This method processes a Client Key exchange message. The content of the message is 
 * passed to the Protocol via the TLS Provider API.
 * 
 * For the RSA key exchange algorithm, this message will consist of an encrypted premaster secret.
 * For the DH key exchange algorithm, it will consist of the DH parameters.
 * For the PSK key exchange algorithm, it will consist of the PSK identity.
 * For the DHE_PSK Key Exchange Algorithm it will consist of the PSK identity followed
 * by the ClientDiffieHellmanPublic params.
 * For the RSA_PSK Key Exchange Algorithm it will consist of the PSK identity followed
 * by the EncryptedPreMasterSecret.
 *
 * @param aStatus TRequestStatus object
 * @return CAsynchEvent* Pointer to the next event to be processed
 */
{
	LOG(Log::Printf(_L("CClientKeyExch::ProcessL()"));)
	
	// Get a pointer to the handshake message buffer and get the Key exchange algorithm.
	TPtr8& ptrComposeMsg = Handshake().ComposeMsg();

	switch ( iClientKeyExcStates )
	{
		case ETlsGetKeyExchangeMsg:
			{
				LOG(Log::Printf(_L("CClientKeyExch::ProcessL() - ETlsGetKeyExchangeMsg"));)
//	__UHEAP_MARK; 
//	__UHEAP_MARKEND;

			const CTlsCryptoAttributes *cryptoAttributes = iTlsProvider->Attributes();

			switch(cryptoAttributes->iPublicKeyParams->iKeyType)
				{
				case EPsk:
				case EDhePsk:
				case ERsaPsk:
					if(!cryptoAttributes->iPskConfigured)
						{
						// Server asked for a ciphersuite with a PSK key exchange algorithm, but PSK is not configured.
						// This can happen if the client forces the ciphersuite, without configuring the PSK callback info.
						User::Leave(KErrSSLAlertIllegalParameter);
						return 0;
						}
					// Callback is set so call it
					delete cryptoAttributes->iPublicKeyParams->iValue4; // Make sure we do not leak a PSK identity
					cryptoAttributes->iPublicKeyParams->iValue4 = 0;
					delete cryptoAttributes->iPublicKeyParams->iValue5; // Make sure we do not leak a PSK value
					cryptoAttributes->iPublicKeyParams->iValue5 = 0;
					cryptoAttributes->iPskKeyHandler->GetPskL(cryptoAttributes->iPskIdentityHint,
															  cryptoAttributes->iPublicKeyParams->iValue4,
															  cryptoAttributes->iPublicKeyParams->iValue5);
					if((cryptoAttributes->iPublicKeyParams->iValue4==0) || (cryptoAttributes->iPublicKeyParams->iValue5==0))
						{
						User::Leave(KErrSSLAlertIllegalParameter);
						}
					break;
				default:
					break;
				}


				iClientKeyExcStates = ETlsComposeKeyExchange;
				iTlsProvider->TlsSessionPtr()->ClientKeyExchange( iKeyExchBuf, aStatus);
				break;
			}
		case ETlsComposeKeyExchange:
			{
				LOG(Log::Printf(_L("CClientKeyExch::ProcessL() - ETlsComposeKeyExchange"));)
				
				iClientKeyExcStates = ETlsGetKeyExchangeMsg;
				TTLSKeyExchangeAlgorithm keyExcAlg = iTlsProvider->Attributes()->iCurrentCipherSuite.CipherDetails()->iKeyExAlg;
				__ASSERT_DEBUG( !iHandshakeMessage, TlsPanic(ETlsPanicHandshakeMsgAlreadyExists));

				iBody.Set( iKeyExchBuf->Des() );	// Get a TPtr to the message contents
            CClientKeyExchMsg* pKeyExchMsg = NULL; //to make compiler happy
				if ( keyExcAlg == ERsa )
				{
               if ( iTlsProvider->Attributes()->iNegotiatedProtocol.iMinor )
               {
					   iHandshakeMessage = pKeyExchMsg = new(ELeave)CRsaClientKeyExchMsg31( iBody.Length() );	
LOG(Log::Printf(_L("iHandshakeMessage %x - %x"), iHandshakeMessage, (TUint)iHandshakeMessage + sizeof( CRsaClientKeyExchMsg31 ));)
               }
               else
               {
					   iHandshakeMessage = pKeyExchMsg = new(ELeave)CRsaClientKeyExchMsg30( iBody.Length() );	
LOG(Log::Printf(_L("iHandshakeMessage %x - %x"), iHandshakeMessage, (TUint)iHandshakeMessage + sizeof( CRsaClientKeyExchMsg30 ));)
               }
				}
				else if ( keyExcAlg == EDHE )
				{
					iHandshakeMessage = pKeyExchMsg = new(ELeave)CDhExplicitClientKeyExchMsg( iBody.Length() );
LOG(Log::Printf(_L("iHandshakeMessage %x - %x"), iHandshakeMessage, (TUint)iHandshakeMessage + sizeof( CDhExplicitClientKeyExchMsg ));)
				}
				else if (keyExcAlg == EPsk)
				{
					iHandshakeMessage = pKeyExchMsg = new(ELeave)CPskClientKeyExchMsg( iBody.Length() );
LOG(Log::Printf(_L("iHandshakeMessage %x - %x"), iHandshakeMessage, (TUint)iHandshakeMessage + sizeof( CPskClientKeyExchMsg ));)
				}
				else 
				{
					LOG(Log::Printf(_L("CClientKeyExch::ProcessL() - Unsupported key exchange"));)
					User::Leave(KErrSSLAlertIllegalParameter);
				}
				// Allocate memory in the state machine's buffer for the message
				HBufC8* pFragment = iStateMachine->ReAllocL( pKeyExchMsg->iRecord.CalcTotalInitialiseLength() );
				ptrComposeMsg.Set( pFragment->Des() );
				ptrComposeMsg.Zero();

				// Initialise the item's pointer to point to the start of its part of 
				// the buffer and set its value.
				pKeyExchMsg->iRecord.InitialiseL( ptrComposeMsg ); 
	  			pKeyExchMsg->SetKeyExchnage( iBody );

				// Create handshake header, update the handshake messages hash and history, set the 
				// record protocol content type
				ComposeHandshakeHeader( ETlsClientKeyExchSent, ETlsClientKeyExchMsg, ptrComposeMsg );

#ifdef _DEBUG
         	LOG(iHandshakeMessage->iRecord.Dump( KSSLLogDir,KSSLLogFileName);)
#endif
				CRecordComposer& recordComposer = iRecordComposer; //'this' will be deleted
				recordComposer.SetNext( Handshake().InitiateTransmitL() );
				return recordComposer.ProcessL( aStatus );			
			}
		default:
			{
				LOG(Log::Printf(_L("CClientKeyExch::ProcessL() - Default case statement - ERROR"));)
				__ASSERT_DEBUG( EFalse, TlsPanic(ETlsPanicInvalidProcessState));
				break;
			}
	} // switch

	return this;
}

//
//
CAsynchEvent* CClientCertificate::ProcessL( TRequestStatus& aStatus )
/**
 * This asynchronous method processes a Client certificate message.
 * This message is only sent if a Server certificate has been received (always true for
 * this implementation as only authenticated servers are supported) and if a Certificate
 * Request has been received by the Server.
 *
 * @param aStatus TRequestStatus object
 * @return CAsynchEvent* Pointer to the next event to be processed
 */
{
	TPtr8& ptrComposeMsg = Handshake().ComposeMsg();

	switch ( iClientCertStates )
	{
		case ETlsGetCertInfo:
			{
				LOG(Log::Printf(_L("CClientCertificate::ProcessL() - ETlsGetCertInfo"));)

				iClientCertStates = ETlsComposeClientCert;
				__ASSERT_DEBUG( !iHandshakeMessage, TlsPanic(ETlsPanicHandshakeMsgAlreadyExists));
				iHandshakeMessage = new(ELeave)CCertificateMsg;
LOG(Log::Printf(_L("iHandshakeMessage %x - %x"), iHandshakeMessage, (TUint)iHandshakeMessage + sizeof( CCertificateMsg ));)

				if( iTlsProvider->TlsSessionPtr() == NULL )
 				{
 					User::Leave(KErrSSLNullTlsSession);
 				}

				iTlsProvider->TlsSessionPtr()->ClientCertificate( &iCertArray, aStatus );
				break;
			}
		case ETlsComposeClientCert:
			{
				LOG(Log::Printf(_L("CClientCertificate::ProcessL() - ETlsGetCertInfo"));)

				iClientCertStates = ETlsGetCertInfo;

				// Compose Client certificate message - Allocate memory in the state machine's 
				// buffer for the message; initialise the record object and set its value.
				CCertificateMsg* pClientCertMsg = (CCertificateMsg*)iHandshakeMessage;

				// If the Client Certificate message is NULL, there is no need to send a Certificate
				// Verify message. A certificate message will be sent with an empty body.
				TInt certCount = iCertArray.Count();
 				if (certCount != 0)
				{
					Handshake().SetCertificateVerifyReqd( ETrue );
					for(TInt i=0; i<certCount; i++)
 						{
 						pClientCertMsg->iCertificateList.AddNodeL( iCertArray[i]->Length() );
 						}
				}
				HBufC8* pFragment = iStateMachine->ReAllocL( pClientCertMsg->iRecord.CalcTotalInitialiseLength() );
				ptrComposeMsg.Set( pFragment->Des() ); 
				ptrComposeMsg.Zero();
				pClientCertMsg->iRecord.InitialiseL( ptrComposeMsg ); 
				if(certCount != 0)
 					{
 					CListNode* currentNode = pClientCertMsg->iCertificateList.First();
 					for(TInt i=0; i<certCount; i++)
 						{
 						TPtrC8 cert = iCertArray[i]->Des();
 						currentNode->SetBody(cert);
 						currentNode = currentNode->Next();
 						}
					}
		
				// Create handshake header, update the history, and set the record protocol,
				// content type, Update the handshake messages hash
				ComposeHandshakeHeader( ETlsClientCertificateSent, ETlsClientCertificateMsg, ptrComposeMsg );   
				CRecordComposer& recordComposer = iRecordComposer; //'this' will be deleted
#ifdef _DEBUG
         	LOG(iHandshakeMessage->iRecord.Dump( KSSLLogDir,KSSLLogFileName);)
#endif

				recordComposer.SetNext( Handshake().NextTxEvent() );
				return recordComposer.ProcessL( aStatus );
			}
		default:
			{
				// This should never happen
				LOG(Log::Printf(_L("CClientCertificate::ProcessL() - 'Default' statement - unknown state"));)
				__ASSERT_DEBUG( EFalse, TlsPanic(ETlsPanicInvalidProcessState));
				break;
			}
	} // switch

	return this;
}

//
//
CAsynchEvent* CCertificateVerify::ProcessL( TRequestStatus& aStatus )
/**
 * This asynchronous method processes a Certificate Verify message.
 * The message is computed differently for the SSL3.0 and TLS1.0 protocols. However,
 * the input passed to the Provider is the same and the differences are handled in the
 * Security components.
 * This message is sent only if the Client certificate is NOT NULL and if the Client 
 * Certificate has signing capability. This is always true for this implementation as 
 * static/fixed DH is NOT supported.
 *
 * @param aStatus TRequestStatus object
 * @return CAsynchEvent* Pointer to the next event to be processed
 */
{
	LOG(Log::Printf(_L("CCertificateVerify::ProcessL"));)

	TPtr8& ptrComposeMsg = Handshake().ComposeMsg();
	
	TAlgorithmId& signatureAlg = Handshake().SignatureAlg();

	CSHA1* sha1HashPtr = Handshake().SHA1Verify();
	CMD5* md5HashPtr = Handshake().MD5Verify();

	switch ( iCertVerifyStates )
	{
		case ETlsGetSignature:
			{
				LOG(Log::Printf(_L("CCertificateVerify::ProcessL() - ETlsGetSignature"));)

				iCertVerifyStates = ETlsComposeCertVerify;
				__ASSERT_DEBUG( !iHandshakeMessage, TlsPanic(ETlsPanicHandshakeMsgAlreadyExists));

				if ( signatureAlg == ERSA)
				{
					LOG(Log::Printf(_L("CCertificateVerify::ProcessL - RSA signature"));)
					iHandshakeMessage = new(ELeave)CRsaCertificateVerifyMsg;
LOG(Log::Printf(_L("iHandshakeMessage %x - %x"), iHandshakeMessage, (TUint)iHandshakeMessage + sizeof( CRsaCertificateVerifyMsg ));)

//					signatureLength = KTlsMd5Length + KTlsShaLength;
				}
				else if ( signatureAlg == EDSA )
				{
					LOG(Log::Printf(_L("CCertificateVerify::ProcessL - DSA signature"));)
					iHandshakeMessage = new(ELeave)CDsaCertificateVerifyMsg;
LOG(Log::Printf(_L("iHandshakeMessage %x - %x"), iHandshakeMessage, (TUint)iHandshakeMessage + sizeof( CDsaCertificateVerifyMsg ));)

//					signatureLength = KTlsShaLength;
				}
				else
				{
					LOG(Log::Printf(_L("CCertificateVerify::ProcessL() - Unsupported signing algorithm"));)
					User::Leave(KErrSSLAlertIllegalParameter);
				}

				// Allocate a buffer of the right size for the signature and call the Provider.
				__ASSERT_DEBUG( !iSignature, TlsPanic(ETlsPanicSignatureAlreadyExists) );
				//iSignature = HBufC8::NewL( signatureLength ); signature allocated by token
         	//LOG(Log::Printf(_L("iSignature %x - %x"), iSignature, iSignature + signatureLength + sizeof( HBufC8 ));)
				iTlsProvider->TlsSessionPtr()->CertificateVerifySignatureL( md5HashPtr, sha1HashPtr, iSignature, aStatus );

				break;
			}
		case ETlsComposeCertVerify:
			{
				// Compose Certificate Verify message - Allocate memory in the state 
				// machine's buffer for the message; initialise the record object and set 
				// its value.
				LOG(Log::Printf(_L("CCertificateVerify::ProcessL() - ETlsComposeCertVerify"));)

				iCertVerifyStates = ETlsGetSignature;
            TPtrC8 sigDes = iSignature->Des();
            CCertificateVerifyMsg* pCertVerifyMsg = static_cast<CCertificateVerifyMsg*>(iHandshakeMessage);
            pCertVerifyMsg->SetSignatureLength( sigDes );
				HBufC8* pFragment = iStateMachine->ReAllocL( pCertVerifyMsg->iRecord.CalcTotalInitialiseLength() );
				ptrComposeMsg.Set( pFragment->Des() ); 
				ptrComposeMsg.Zero();
				pCertVerifyMsg->iRecord.InitialiseL( ptrComposeMsg ); 
            pCertVerifyMsg->SetSignature( sigDes );

				// Create handshake header, update the history, and set the record protocol,
				// content type, Update the handshake messages hash
				ComposeHandshakeHeader( ETlsCertificateVerifySent, ETlsCertificateVerifyMsg, ptrComposeMsg );
      
#ifdef _DEBUG
         	LOG(iHandshakeMessage->iRecord.Dump( KSSLLogDir,KSSLLogFileName);)
#endif
            iRecordComposer.SetNext( Handshake().NextTxEvent() );
				return iRecordComposer.ProcessL( aStatus );

			}	// case ETlsComposeCertVerify
		default:
			{	// This should never happen
				LOG(Log::Printf(_L("CCertificateVerify::ProcessL() - 'Default' statement - unknown state"));)
				__ASSERT_DEBUG( EFalse, TlsPanic(ETlsPanicInvalidProcessState));
				break;
			}
	}// switch

	return this;
}

//
//
CAsynchEvent* CSendFinished::ProcessL( TRequestStatus& aStatus )
/** 
 * This asynchronous method processes a transmitted Finished message.
 * The input to the Finished message is the same for both SSL3.0 and TLS1.0. However
 * the calculations for the end result (and thus the output) are different.
 *
 * @param aStatus TRequestStatus object
 * @return CAsynchEvent* Pointer to the next event to be processed
 */
{
	LOG(Log::Printf(_L("CSendFinished::ProcessL()"));)

	switch( iFinishedStates)
	{
		case ETlsGetFinishedMsg:
			{
				LOG(Log::Printf(_L("CSendFinished::ProcessL() - ETlsGetFinishedMsg"));)
			
				iFinishedStates = ETlsComposeFinished;
				__ASSERT_DEBUG( !iHandshakeMessage, TlsPanic(ETlsPanicHandshakeMsgAlreadyExists));

				// Send the information to the Provider to calculate the message body
				iShaHashPtr = static_cast<CSHA1*>( Handshake().SHA1Verify()->CopyL() ); 
				iMd5HashPtr = static_cast<CMD5*>( Handshake().MD5Verify()->CopyL() );
				iTlsProvider->TlsSessionPtr()->ClientFinishedMsgL( iMd5HashPtr, iShaHashPtr, iFinishedMsg, aStatus);				
				break;
			}
		case ETlsComposeFinished:
			{
				LOG(Log::Printf(_L("CSendFinished::ProcessL() - ETlsComposeFinished"));)

				iFinishedStates = ETlsGetFinishedMsg;
				TPtr8& ptrComposeMsg = Handshake().ComposeMsg();

				iBody.Set( iFinishedMsg->Des() );	// Get a TPtr to the message contents
   			__ASSERT_DEBUG( !iHandshakeMessage,  TlsPanic(ETlsPanicHandshakeMsgAlreadyExists));
				iHandshakeMessage = new(ELeave)CFinishedMsg( iBody.Length() );
LOG(Log::Printf(_L("iHandshakeMessage %x - %x"), iHandshakeMessage, (TUint)iHandshakeMessage + sizeof( CFinishedMsg ));)
				CFinishedMsg* pFinishedMsg = (CFinishedMsg*)iHandshakeMessage;

				HBufC8* pFragment = iStateMachine->ReAllocL( pFinishedMsg->iRecord.CalcTotalInitialiseLength() );
				ptrComposeMsg.Set( pFragment->Des() ); 
				ptrComposeMsg.Zero();

				pFinishedMsg->iRecord.InitialiseL( ptrComposeMsg ); 
				pFinishedMsg->iFinishedData.SetBody ( iBody );			

				// Create handshake header, update the history, and set the record protocol,
				// content type, Update the handshake messages hash
				ComposeHandshakeHeader( ETlsFinishedSent, ETlsFinishedMsg, ptrComposeMsg );      
#ifdef _DEBUG
         	LOG(iHandshakeMessage->iRecord.Dump( KSSLLogDir,KSSLLogFileName);)
#endif
			
				// Set up next event.
				CRecordComposer& recordComposer = iRecordComposer; //'this' will be deleted		
				recordComposer.SetNext( Handshake().InitiateReceiveL() );
				return recordComposer.ProcessL( aStatus );
			}
		default:
			{	// This should never happen
				LOG(Log::Printf(_L("CSendFinished::ProcessL() - 'Default' statement - unknown state"));)
				__ASSERT_DEBUG( EFalse, TlsPanic(ETlsPanicInvalidProcessState) );
				break;
			}
	} // switch
	
	return this;
}

