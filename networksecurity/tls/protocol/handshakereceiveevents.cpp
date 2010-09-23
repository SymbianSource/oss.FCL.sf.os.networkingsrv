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
// Implementation file for Received handshake events - ServerHello, 
// Server Certificate, Server KeyExchange, ServerHelloDone and Server 
// Finished messages. It also contains the implementation for the 
// CHandshakeParser class (parses received handshake messages).
// 
//

/**
 @file
*/

#include "tlshandshakeitem.h"
#include "handshakereceiveevents.h"
#include "recordprotocolevents.h"
#include "tlshandshake.h"
#include "tlsconnection.h"
#include <signed.h>

#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <tlstypedef_internal.h>
#endif

CHandshakeParser::~CHandshakeParser()
/**
 * Destructor. 
 * Destroys the Received messages list, nulls and deletes its pointers
 * and descriptors.
 */
{
	LOG(Log::Printf(_L("CHandshakeParser::~CHandshakeParser()"));)

	DestroyRxList();
	iRecordParser.SetUserData( NULL );
	iRecordParser.SetUserMaxLength( 0 );
	delete iMessage;
}

void CHandshakeParser::DestroyRxList()
{
	LOG(Log::Printf(_L("CHandshakeParser::DestroyRxList() of expected handshake messages"));)

	CHandshakeReceive* listItem;
    
    iRxListIter.SetToFirst(); 
    while ( (listItem = iRxListIter++) != NULL )
        {
        iMessageList.Remove(*listItem);
        delete listItem;
        };
}

void CHandshakeParser::AddToList( CHandshakeReceive& aRxMsgItem )
{
	LOG(Log::Printf(_L("CHandshakeParser::AddToList()"));)

    iMessageList.AddLast(aRxMsgItem);
}

CTlsEvent* CHandshakeParser::LookUpEventL( const TUint8 aHandshakeType )
/**
 * This method is called from CHandshakeParser::ParseHeaderL(). It is used to determine
 * which handshake message (event) will process a received message.
 * 
 * @param aHandshakeType Constant TUint8 representing the Handshake message type.
 * @return CTlsEvent* A pointer to the event that will process the received message.
 */
{
	LOG(Log::Printf(_L("CHandshakeParser::LookUpEventL()"));)

   CHandshakeReceive* pEvent;
   iRxListIter.SetToFirst(); 

	while ( (pEvent = iRxListIter) != 0 && !pEvent->AcceptMessage( aHandshakeType ) )
      {
		iRxListIter++;
      }
	if ( !pEvent )
      {
		User::Leave( KErrSSLAlertUnexpectedMessage );
      }
  
   return pEvent;
}

TInt CHandshakeParser::ParseHeaderL()
/**
 * This method parses a handshake message header and determines which event will
 * process a received message. It also extracts the message length (which is in
 * big endian format) from the message header.
 *
 * @return TInt An integer representing the handshake message length.
 */
{
	LOG(Log::Printf(_L("CHandshakeParser::ParseHeaderL()"));)
   
	iNext = LookUpEventL( iMessageType = iMessagePtr[KTlsHandshakeTypeOffset] );
   
	TBigEndian value; 
	TInt nLength = value.GetValue( iMessagePtr.Ptr() + KTlsHandshakeLengthOffset, KTlsHandshakeBodyLength );
   if ( nLength > KTlsMaxHandshakeBodySize )
      {
      User::Leave( KErrSSLAlertIllegalParameter );
      }
	LOG(RFileLogger::HexDump(KSSLLogDir,KSSLLogFileName,EFileLoggingModeAppend, NULL, NULL, iMessagePtr.Ptr(), iMessagePtr.Length() ));
   return nLength;
}

void CHandshakeParser::SetMessageAsUserDataL( TInt aWaitingFor )
{
   if ( !iMessage )
      {
      iMessage = HBufC8::NewL( 128 );
   	iMessagePtr.Set( iMessage->Des() );
      }
	iRecordParser.SetUserData( &iMessagePtr );
  	LOG(Log::Printf(_L("CHandshakeParser::SetMessageAsUserDataL() - aWaitingFor = %d, %d"), aWaitingFor, iMessagePtr.Length());)
	iRecordParser.SetUserMaxLength( aWaitingFor );
   iWaitingFor = aWaitingFor;
}

TPtr8 CHandshakeParser::Message()
/**
 * This method returns a pointer to a heap descriptor which contains (or
 * will contain) a received Handshake protocol message. We don't want to return a reference to HBuf
 * that's why thi akward TPtr construction. The caller MUST process the buffer before returning
 * to active scheduler.
 */
{
   TPtr8 ptr( const_cast<TUint8*>(iMessagePtr.Ptr()), iMessagePtr.Length(), iMessagePtr.MaxLength() );
   iMessagePtr.SetLength( 0 );
  	LOG(Log::Printf(_L("CHandshakeParser::Message() - %d, %d"), iMessage->Length(), iMessagePtr.Length());)
	return ptr; //we cannot return iMessagePtr since it's reset to zero
}

CAsynchEvent* CHandshakeParser::ProcessNextL( TRequestStatus& aStatus )
{
	// Note that a Finished message is treated differently (see CRecvFinished::ProcessL)
	// update verify happens only whilst negotiating
	if ( iMessageType != ETlsFinishedMsg && iStateMachine->History() != KTlsApplicationData )
	{
		Handshake().UpdateVerify( iMessagePtr );
	}

	SetMessageAsUserDataL( KTlsHandshakeHeaderSize );
	return iNext->ProcessL( aStatus );
   }

TBool CHandshakeParser::AcceptRecord( TInt aRecordType ) const
/** 
 * This virtual method determines whether the first byte of a Record protocol header
 * (content type) can be accepted by an event (in iExpRecordTypes).
 *
 * @param aRecordType Integer specifying the Record protocol content type
 * @return TBool Boolean indicating whether or not the record should be accepted by  
 * this event.
 */
{
	LOG(Log::Printf(_L("CHandshakeParser::AcceptRecord()"));)
	
	return aRecordType == ETlsHandshakeContentType;
}

CAsynchEvent* CHandshakeParser::ProcessL( TRequestStatus& aStatus )
/**
 * This asynchronous message parses a received handshake message. It first processes
 * the message header (extracts the message length) and then the message body.
 *
 * @param aStatus Request status for this event.
 * @return CAsynchEvent* A pointer to an asynchronous event which will process
 * the handshake message.
 */
{
  
  	//********TODO: Remove this If condition when CR741 is submitted*******/
	if(iMessage == NULL)
		{
		TRequestStatus* p=&aStatus;
  		User::RequestComplete( p, KErrNone );
  		return LookUpEventL(ETlsHelloRequestMsg);
		/*			
		Recordparser received a handshake message in application data mode which the
		handshake parser framework isnt primed to do. hence this "null" message.
		
		The iMessage buffer is initialized when and during the initial or 
		client renegotiation handshake and will not be initialized during APP data
		transfer. Since we currently don't support renegotiations from server,
		any handshake message about to be processed in app mode is stopped here, 
		returning an alert.
		*/
		}
	//*********************************************************************/
	
	__ASSERT_DEBUG( iMessage, TlsPanic(ETlsPanicNullHandshakeMsg) );
	
	// Do we have enough data?
	if ( iMessagePtr.Length() == iWaitingFor )
	{
		if ( iMessagePtr.Length() == KTlsHandshakeHeaderSize )
		{	// The handshake header has arrived
      	LOG(Log::Printf(_L("CHandshakeParser::ProcessL() - msg header received"));)
			LOG(RFileLogger::HexDump(KSSLLogDir,KSSLLogFileName,EFileLoggingModeAppend, NULL, NULL, iMessagePtr.Ptr(), iMessagePtr.Length() ));
			iWaitingFor += ParseHeaderL(); // Append message length
         
			if ( iWaitingFor > iMessagePtr.MaxLength() )
			{
				iMessage = iMessage->ReAllocL( iWaitingFor );
         	iMessagePtr.Set( iMessage->Des() );
			}
			else if ( iWaitingFor == KTlsHandshakeHeaderSize )
			{	// A zero length message was received => proceed at once
				// The message length will be set to zero after the message has been processed
				// see ParseHeaderL iNext must be != NULL => assert not necessary 
				return ProcessNextL( aStatus );
			}
		}
		else
		{	// Must be the message body so process it. The message buffer length and 
			// iUserMaxLength will be reset after the message has been processed.
      	LOG(Log::Printf(_L("CHandshakeParser::ProcessL() - msg body received"));)
			iWaitingFor = KTlsHandshakeHeaderSize;	// reset  
			__ASSERT_DEBUG( iNext, TlsPanic(ETlsPanicNoDataToProcess) ); //see ParseHeaderL
		 
			return ProcessNextL( aStatus );
		}
	}
	
	//read again
  	LOG(Log::Printf(_L("CHandshakeParser::ProcessL() - read again iWaitingFor = %d, %d, %d"), iWaitingFor, iMessage->Length(), iMessagePtr.Length());)
   SetMessageAsUserDataL( iWaitingFor );
	return iRecordParser.ProcessL( aStatus );
}

//
//
CHandshakeReceive::~CHandshakeReceive()
{
	LOG(Log::Printf(_L("CHandshakeReceive::~CHandshakeReceive()"));)
	delete iHandshakeMessage;
	iHandshakeMessage = NULL;
}

//
//
CServerHello::~CServerHello()
{
	iCipherList.Close();
}

TBool CServerHello::AcceptMessage( const TUint8 aHandshakeType ) const
/**
 * This method determines whether a Server Hello message (event) should be accepted.
 * It asserts that the Client Hello message has been sent. This is the only condition
 * that must be met before a Server Hello message can be received immediately afterwards.
 * @param aHandshakeType A handshake message type
 * @return TBool Boolean indicating whether a message should be accepted
 */
 {
	LOG(Log::Printf(_L("CServerHello::AcceptMessage()"));)

	__ASSERT_DEBUG( iStateMachine->History() & ETlsClientHelloSent, TlsPanic(ETlsPanicClientHelloMsgNotSent) );
	return !iHandshakeMessage && aHandshakeType == ETlsServerHelloMsg;
}

CAsynchEvent* CServerHello::ProcessL( TRequestStatus& aStatus )
/**
 * This method parses a received Server Hello message. It extracts the items in the 
 * message and passes the information on to Security for processing. 
 * For a Server Hello message, the only item of variable length is the Session Id
 * (hence this item is preceded by its length part).
 *
 * The next event to be processed depends on whether this is a full handshake or an
 * abbreviated handshake.
 *
 * @param aStatus Request status for this event.
 * @return CAsynchEvent* A pointer to the next asynchronous event to be processed.
 */ 
{
	LOG(Log::Printf(_L("CServerHello::ProcessL()"));)
   if ( iRecordParser.ReadActive() && !iCipherListRead )
   {//server's accepted renegotiation => close the old provider to reset token & attributes
   //we still keep the encryptor & decryptor for record level in CRecordParser::iActiveTlsSession
   //& CRecordComposer::iActiveTlsSession
      iTlsProvider->ReConnectL();
   //!!!need to find the session again to force token enumeration this is extremely silly!!!
   //!!!hoping to get the same session at least which should be implied
	
		iTlsProvider->CipherSuitesL(iCipherList, aStatus); 
      iCipherListRead = ETrue;
      return this;
   }
	iCipherList.Close();
	iHandshakeMessage = new(ELeave)CServerHelloMsgWithOptionalExtensions;
	LOG(Log::Printf(_L("iHandshakeMessage %x - %x"), iHandshakeMessage, (TUint)iHandshakeMessage + sizeof( CServerHelloMsg ));)
	
	TPtr8 ptr( iRecordParser.HandshakeParser()->Message() );
	iHandshakeMessage->iRecord.ParseL( ptr );	// Set each item's pointer to the correct part of the received message
#ifdef _DEBUG
	LOG(iHandshakeMessage->iRecord.Dump( KSSLLogDir,KSSLLogFileName);)
#endif
	
	CServerHelloMsg* pHelloMsg = static_cast<CServerHelloMsg*>(iHandshakeMessage);
	
	// Extract the Server Hello data items and fill the TLSProvider's data structure

	TPtr8 body = pHelloMsg->iVersion.GetBodyDes();
   CTlsCryptoAttributes& cryptoAttributes = *iTlsProvider->Attributes();
	cryptoAttributes.iNegotiatedProtocol.iMajor = body[0];
	cryptoAttributes.iNegotiatedProtocol.iMinor = body[1];
   
	TDes8& desSessionId = cryptoAttributes.iSessionNameAndID.iSessionId;
	TInt resumeSession = desSessionId.Compare( pHelloMsg->iSessionId.GetBodyDes() );

   Handshake().SetNegotiatedVersion( &cryptoAttributes.iNegotiatedProtocol ); 
	
   cryptoAttributes.iMasterSecretInput.iServerRandom = pHelloMsg->iRandom.GetBodyDes();

	User::LeaveIfError(pHelloMsg->iSessionId.GetBodyDes().Length() <= 32 ? KErrNone:KErrSSLAlertIllegalParameter);

	desSessionId.Copy( pHelloMsg->iSessionId.GetBodyDes() );
	LOG(RFileLogger::HexDump(KSSLLogDir,KSSLLogFileName,EFileLoggingModeAppend, NULL, NULL, desSessionId.Ptr(), desSessionId.Length() ));

	body.Zero();
	body = pHelloMsg->iCipherSuite.GetBodyDes();
   //we have to get back the cipher from our proposed list
   //current cipher suites start with 00 followed by something != 00 => the below test is sufficient
   //for the known ciphers. In case the server gives us something unknown the security will not be
   //able to find/use any token and should return an error further down the handshake.
   User::LeaveIfError( body.Length() == 2 && 
      cryptoAttributes.iProposedCiphers.Find( body ) != KErrNotFound ? KErrNone : KErrSSLAlertIllegalParameter );
	cryptoAttributes.iCurrentCipherSuite.iLoByte = body[1];
	cryptoAttributes.iCurrentCipherSuite.iHiByte = body[0];


	body.Zero();
	body = pHelloMsg->iCompression.GetBodyDes();
	User::LeaveIfError( body[0] == NULL ? KErrNone : KErrSSLAlertUnexpectedMessage );
	cryptoAttributes.iCompressionMethod = (TTLSCompressionMethod) body[0];
		
	// Update the history and decide on whether it's a full or abbreviated handshake.
	// Abbreviated handshake is only when Client and Server Session Id match, AND the server's
	// Session id is NOT zero (i.e. when it is a resumable session).
	iStateMachine->UpdateHistory( ETlsServerHelloRecv );

	if ( resumeSession == 0 && desSessionId.Length() ) 
      {//server hello done won't be sent => create security parameters here
   	LOG(Log::Printf(_L("Abbreviated Handshake"));)
		iStateMachine->UpdateHistory( ETlsAbbreviatedHandshake ); // Abbreviated Handshake
   	iTlsProvider->CreateL( Handshake().TlsSession(), aStatus);
      }
	else	   
      {
   	LOG(Log::Printf(_L("Full Handshake"));)
		iStateMachine->UpdateHistory( ETlsFullHandshake ); // Full Handshake
	   TRequestStatus* p=&aStatus;
	   User::RequestComplete( p, KErrNone );
      }

      const TTLSCipherSuiteMapping *pCipherDetails = cryptoAttributes.iCurrentCipherSuite.CipherDetails();
      if(pCipherDetails)
      {
	 	if(pCipherDetails->iKeyExAlg == EPsk)
	 	{
	    	// Using PSK for key exchange which impacts list of legal messages and ordering checks
	    	iStateMachine->UpdateHistory( ETlsUsingPskKeyExchange );
	 	}
	 	// Set key exchange type in the CTLSPublicKeyParams structure which will be passed to the TLS token.
	 	iTlsProvider->Attributes()->iPublicKeyParams->iKeyType = pCipherDetails->iKeyExAlg;
      }
	
      // Call InitiateReceiveL() to set up the list of expected messages
      return Handshake().InitiateReceiveL();

}

//
//
TBool CCertificateReq::AcceptMessage( const TUint8 aHandshakeType ) const
/**
 * This method determines whether a Certificate Request message (event) should 
 * be accepted. A server certificate must have been received before a certificate 
 * request can be made.
 *
 * @param aHandshakeType A handshake message type
 * @return TBool Boolean indicating whether a message should be accepted
 */
{
	LOG(Log::Printf(_L("CCertificateReq::AcceptMessage()"));)
	return !iHandshakeMessage && iStateMachine->History() & ETlsServerCertificateRecv && aHandshakeType == ETlsCertificateReqMsg; 
}

CAsynchEvent* CCertificateReq::ProcessL( TRequestStatus& aStatus )
/**
 * This asynchronous method processes a received Certificate Request message.
 *
 * @param aStatus Request status for this event.
 * @return CAsynchEvent* A pointer to the next asynchronous event to be processed.
 */
{
	LOG(Log::Printf(_L("CCertificateReq::ProcessL()"));)

	__ASSERT_DEBUG( !iHandshakeMessage, TlsPanic( ETlsPanicHandshakeMsgAlreadyExists) );
	iHandshakeMessage = new(ELeave) CCertificateReqMsg;
	LOG(Log::Printf(_L("iHandshakeMessage %x - %x"), iHandshakeMessage, (TUint)iHandshakeMessage + sizeof( CCertificateReqMsg ));)
   TPtr8 ptr( iRecordParser.HandshakeParser()->Message() );
	LOG(Log::Printf(_L("ptr.Length() %d"), ptr.Length() );)
    
	iHandshakeMessage->iRecord.ParseL( ptr );
#ifdef _DEBUG
	LOG(iHandshakeMessage->iRecord.Dump( KSSLLogDir,KSSLLogFileName);)
#endif
    
   // Update the Handshake history and set the Certificate Types
   iStateMachine->UpdateHistory( ETlsCertificateReqRecv );
   CCertificateReqMsg* pCertReqMsg = (CCertificateReqMsg*)iHandshakeMessage;
	 
	TPtr8 body = pCertReqMsg->iClientCertificateTypes.GetBodyDes();
   CTlsCryptoAttributes& cryptoAttributes = *iTlsProvider->Attributes();
	
	for (TInt loop = 0; loop < body.Length(); ++loop)
	{
		User::LeaveIfError( cryptoAttributes.iReqCertTypes.Append( (TTLSClientCertType) body[loop] ) );		
	}

	// Set the list of CA Distinguished Names
	CListNode* listNode = pCertReqMsg->iDistinguishedNames.First();

	__ASSERT_DEBUG( cryptoAttributes.iDistinguishedCANames.Count() == 0, TlsPanic(ETlsPanicNoCA ));
   while ( listNode )
	{
		TPtr8 listPtr = listNode->GetBodyDes();
   	LOG(Log::Printf(_L("DistinguishedCAName") );)
		LOG(RFileLogger::HexDump(KSSLLogDir,KSSLLogFileName,EFileLoggingModeAppend, NULL, NULL, listPtr.Ptr(), listPtr.Length() ));
		
		// Append the item into the Distinguished Names array and get the next list item
      HBufC8* buf = listPtr.AllocL();
   	CleanupStack::PushL(buf);
		User::LeaveIfError(cryptoAttributes.iDistinguishedCANames.Append(buf) );
   	CleanupStack::Pop(buf);
		listNode = listNode->Next();
	}
   iTlsProvider->Attributes()->iClientAuthenticate = ETrue;

	TRequestStatus* p=&aStatus;
	User::RequestComplete( p, KErrNone );
    return &iRecordParser;
}

//
//
TBool CServerCertificate::AcceptMessage( const TUint8 aHandshakeType ) const
/**
 * This method determines whether a Server Certificate message (event) should 
 * be accepted.
 *
 * @param aHandshakeType A handshake message type
 * @return TBool Boolean indicating whether a message should be accepted
 */
{
	LOG(Log::Printf(_L("CServerCertificate::AcceptMessage()"));)

	// Assert that a Server Hello message has been received
	__ASSERT_DEBUG( iStateMachine->History() & ETlsServerHelloRecv, TlsPanic(ETlsPanicServerHelloMsgNotReceived));
	return !iHandshakeMessage && aHandshakeType == ETlsServerCertificateMsg;
}

CAsynchEvent* CServerCertificate::ProcessL( TRequestStatus& aStatus )
/**
 * This asynchronous method processes a Server Certificate message. The contents of the
 * message are passed as an uninterpreted buffer to the Security subsystem.
 *
 * @param aStatus Request status for this event.
 * @return CAsynchEvent* A pointer to the next asynchronous event to be processed.
 */
{
	CX509Certificate*& serverCert = Handshake().ServerCert(); 
   if ( serverCert )
      {//store signing alg
      //cannot convert from 'enum TAlgorithmId' to 'enum TTLSSignatureAlgorithm =>
      //=> CTlsProvider MUST use the same enum as security -> the same applies to keyExchange alg
	   const CSubjectPublicKeyInfo& publicKeyInfo = serverCert->PublicKey();

	   TAlgorithmId& signAlgorithm = Handshake().SignatureAlg();
	   signAlgorithm = publicKeyInfo.AlgorithmId();
      if ( signAlgorithm == ERSA )
         {
	      iTlsProvider->Attributes()->isignatureAlgorithm = ERsaSigAlg;
         }
      else if ( signAlgorithm == EDSA )
         {
	      iTlsProvider->Attributes()->isignatureAlgorithm = EDsa;
         }
      else
         {
	      LOG(Log::Printf(_L("Unknown signing algorithm and ridiculous enum redefinitions."));)
         User::Leave( KErrSSLAlertIllegalParameter );
         }
	   TRequestStatus* p=&aStatus;
	   User::RequestComplete( p, KErrNone );
      return &iRecordParser;
      }
	LOG(Log::Printf(_L("CServerCertificate::ProcessL()"));)

	// Update the Handshake history and pass the server certificate chain to TLS Provider
    iStateMachine->UpdateHistory( ETlsServerCertificateRecv );
    
	TPtr8 ptr( iRecordParser.HandshakeParser()->Message() );
	TPtr8 ptrEncoded( ptr );

#ifdef _DEBUG
   if (iHandshakeMessage) 
      {
      LOG(Log::Printf(_L("ERROR: iHandshakeMessage %x - %x, should be NULL"), iHandshakeMessage, (TUint)iHandshakeMessage + sizeof( CCertificateMsg ));)
      }
	__ASSERT_DEBUG( !iHandshakeMessage, TlsPanic(ETlsPanicHandshakeMsgAlreadyExists));
#endif

	iHandshakeMessage = new(ELeave) CCertificateMsg;
	LOG(Log::Printf(_L("iHandshakeMessage %x - %x"), iHandshakeMessage, (TUint)iHandshakeMessage + sizeof( CCertificateMsg ));)
	iHandshakeMessage->iRecord.ParseL( ptr );	// Set each item's pointer to the correct part of the received message
#ifdef _DEBUG
	LOG(iHandshakeMessage->iRecord.Dump( KSSLLogDir,KSSLLogFileName);)
#endif
   //prepare certificate chain for security (this should've really been TLS provider's work 
   CListNode* pNode = ((CCertificateMsg*)iHandshakeMessage)->iCertificateList.First();
   User::LeaveIfError( pNode ? KErrNone : KErrSSLAlertBadCertificate );
   //iPtr1 + iPtr2 setup to delete handshake header+cert list length
   TUint8* iPtr1 = iHandshakeMessage->Ptr();
   TUint8* iPtr2 = pNode->Ptr();
   TInt nDel = 0;
   TInt nToDel = iPtr2 - iPtr1;
   TInt nPos = iPtr1 - iHandshakeMessage->Ptr();
   ptrEncoded.Delete( nPos, nToDel );
   do
      {
      nDel += nToDel;
      iPtr1 = pNode->Ptr() - nDel;
      iPtr2 = pNode->GetBodyPtr() - nDel;
      nToDel = iPtr2 - iPtr1;
      nPos = iPtr1 - iHandshakeMessage->Ptr();
      ptrEncoded.Delete( nPos, nToDel );
      pNode = pNode->Next();
      }
   while ( pNode );
	LOG(RFileLogger::HexDump(KSSLLogDir,KSSLLogFileName,EFileLoggingModeAppend, NULL, NULL, ptrEncoded.Ptr(), ptrEncoded.Length() ));
	iTlsProvider->VerifyServerCertificate( ptrEncoded, serverCert, aStatus);
   
    
   return this;
}

//
//
TBool CServerKeyExch::AcceptMessage( const TUint8 aHandshakeType ) const
/**
 * This method determines whether a Server Key exchange message should be accepted.
 * As server authentication is mandatory for the current implementation, either a Server 
 * certificate must have been received, or we must be using PSK to key exchange and authenticate.
 *
 * @param aHandshakeType A handshake message type
 * @return TBool Boolean indicating whether a message should be accepted
 */
{
	LOG(Log::Printf(_L("CServerKeyExch::AcceptMessage()"));)

	return !iHandshakeMessage && 
			((iStateMachine->History() & ETlsServerCertificateRecv) || (iStateMachine->History() & ETlsUsingPskKeyExchange)) && 
			aHandshakeType == ETlsServerKeyExchMsg; 
}

void CServerKeyExch::CreateMessageL( TTLSKeyExchangeAlgorithm aKeyExchange, TAlgorithmId aSignAlgorithm )
   {
	if ( aKeyExchange == ERsa )		// Provider's enum for RSA differs from Security, hence the 2 enums values
   	{
      if ( aSignAlgorithm == ERSA )
         {
         iHandshakeMessage = new(ELeave) CRsaRsaServerKeyExchMsg;
LOG(Log::Printf(_L("iHandshakeMessage %x - %x"), iHandshakeMessage, (TUint)iHandshakeMessage + sizeof( CRsaRsaServerKeyExchMsg ));)
         }
   	else if ( aSignAlgorithm == EDSA)
         {
         iHandshakeMessage = new(ELeave) CRsaDsaServerKeyExchMsg;
LOG(Log::Printf(_L("iHandshakeMessage %x - %x"), iHandshakeMessage, (TUint)iHandshakeMessage + sizeof( CRsaDsaServerKeyExchMsg ));)
         }
      }
   else if ( aKeyExchange == EDHE )
      {
      if ( aSignAlgorithm == ERSA )
         {
         iHandshakeMessage = new(ELeave) CDhRsaServerKeyExchMsg;
LOG(Log::Printf(_L("iHandshakeMessage %x - %x"), iHandshakeMessage, (TUint)iHandshakeMessage + sizeof( CDhRsaServerKeyExchMsg ));)
         }
   	else if ( aSignAlgorithm == EDSA)
         {
         iHandshakeMessage = new(ELeave) CDhDsaServerKeyExchMsg;
LOG(Log::Printf(_L("iHandshakeMessage %x - %x"), iHandshakeMessage, (TUint)iHandshakeMessage + sizeof( CDhDsaServerKeyExchMsg ));)
         }
      }
   else if ( aKeyExchange == EPsk )
   		 {
          iHandshakeMessage = new(ELeave) CPskServerKeyExchMsg;
LOG(Log::Printf(_L("iHandshakeMessage %x - %x"), iHandshakeMessage, (TUint)iHandshakeMessage + sizeof( CPskServerKeyExchMsg ));)
   		 }
   
   if ( !iHandshakeMessage )
      {
		LOG(Log::Printf(_L("CServerKeyExch::ProcessL() - Unknown signing algorithm"));)
		User::Leave(KErrSSLAlertIllegalParameter);
	   }
   }

CAsynchEvent* CServerKeyExch::ProcessL( TRequestStatus& aStatus )
/**
 * This message processes a Server Key exchange message. 
 * The protocol needs the Key exchange algorithm from the cipher suite and the signing
 * algorithm, if relevant, from the certificate, in order to interpret and process this 
 * message correctly.
 *
 * @param aStatus Request status for this event.
 * @return CAsynchEvent* A pointer to the next asynchronous event to be processed.
 */
{
	LOG(Log::Printf(_L("CServerKeyExch::ProcessL()"));)

	// Get the Key exchange Algorithm and set this value in the Provider.
	const TTLSCipherSuite cipherSuite = iTlsProvider->Attributes()->iCurrentCipherSuite;

	TTLSKeyExchangeAlgorithm keyExchange = iTlsProvider->Attributes()->iPublicKeyParams->iKeyType;

	TAlgorithmId& signAlgorithm = Handshake().SignatureAlg();

	// Create and parse the Server Key exchange message. Set all necessary information.
   CreateMessageL( keyExchange, signAlgorithm );
	TPtr8 ptr( iRecordParser.HandshakeParser()->Message() );
	iHandshakeMessage->iRecord.ParseL( ptr );
#ifdef _DEBUG
	LOG(iHandshakeMessage->iRecord.Dump( KSSLLogDir,KSSLLogFileName);)
#endif
	CServerKeyExchMsg* pKeyExcMsg = (CServerKeyExchMsg*)iHandshakeMessage;

	pKeyExcMsg->CopyParamsL( iTlsProvider->Attributes() );

	if(!(iStateMachine->History() & ETlsUsingPskKeyExchange))
		{
		// Under pre-shared keys, the ServerKeyExch message does not have certificates (see RFC 4279). In
		// this case, we do not process the certs.
		TBuf8<KTlsMd5Length + KTlsShaLength> msgDigest;
   		pKeyExcMsg->ComputeDigestL( iTlsProvider->Attributes()->iMasterSecretInput.iClientRandom,
        		                    iTlsProvider->Attributes()->iMasterSecretInput.iServerRandom,
                               		msgDigest );
   		TPtr8 signature( pKeyExcMsg->Signature() );
		CX509Certificate* serverCert = Handshake().ServerCert(); 
		__ASSERT_DEBUG( serverCert, TlsPanic(ETlsPanicNullServerCertificate) );
		const CSubjectPublicKeyInfo& publicKeyInfo = serverCert->PublicKey();

   		User::LeaveIfError( iTlsProvider->VerifySignatureL(publicKeyInfo, msgDigest, signature) ?
        					KErrNone : KErrSSLAlertBadCertificate );
		}
	
	// Update the Handshake history. Clear message buffer at the end and return the next
	// item to be processed.
   iStateMachine->UpdateHistory( ETlsServerKeyExchRecv );
   
	TRequestStatus* p=&aStatus;
	User::RequestComplete( p, KErrNone );
   return &iRecordParser;
}

TBool CServerHelloDone::AcceptMessage( const TUint8 aHandshakeType ) const
/** 
 * This method decides whether a 'ServerHelloDone' message can be accepted.
 * This message can only be received after a Server hello message. However,
 * as this protocol implementation requires that a Server be authenticated,
 * this message can only be accepted after a Server's certificate has been received, or we must
 * be using PSK to key exchange and authenticate.
 * 
 * @param aHandshakeType A handshake message type
 * @return TBool Boolean indicating whether a message should be accepted
 */
{
	LOG(Log::Printf(_L("CServerHelloDone::AcceptMessage()"));)

	return !iHandshakeMessage &&
			((iStateMachine->History() & ETlsServerCertificateRecv) || (iStateMachine->History() & ETlsUsingPskKeyExchange)) && 
			aHandshakeType == ETlsServerHelloDoneMsg; 
}

CAsynchEvent* CServerHelloDone::ProcessL( TRequestStatus& aStatus )
/**
 * This method processes a Server Hello Done message.
 * Once this message has been received, the Server's params (from the negotiation)
 * can begin to be processed.
 *
 * @param aStatus Request status for this event.
 * @return CAsynchEvent* A pointer to the next asynchronous event to be processed.
 */
{
	LOG(Log::Printf(_L("CServerHelloDone::ProcessL()"));)
	
	iHandshakeMessage = new(ELeave)CServerHelloDoneMsg;
   LOG(Log::Printf(_L("iHandshakeMessage %x - %x"), iHandshakeMessage, (TUint)iHandshakeMessage + sizeof( CServerHelloDoneMsg ));)
	TPtr8 ptr( iRecordParser.HandshakeParser()->Message() );
	iHandshakeMessage->iRecord.ParseL( ptr );	// Set each item's pointer to the correct part of the received message
#ifdef _DEBUG
	LOG(iHandshakeMessage->iRecord.Dump( KSSLLogDir,KSSLLogFileName);)
#endif
	
	// Update the Handshake history
	// A ServerHelloDone message always has an empty body
	User::LeaveIfError( ptr.Length() ? KErrSSLAlertUnexpectedMessage : KErrNone );
	iStateMachine->UpdateHistory( ETlsServerHelloDoneRecv );
   
    // Call InitiateTransmitL() to start protocol transmissions. 
	// Set the next event to be processed as the Record composer object
	iTlsProvider->CreateL( Handshake().TlsSession(), aStatus);
	return Handshake().InitiateTransmitL();
}

//
//
TBool CRecvFinished::AcceptMessage( const TUint8 aHandshakeType ) const
/** 
 * This method decides whether a received 'Finished' message can be accepted.
 * A 'Finished' message can only be accepted after a 'ChangeCipherSpec' message 
 * has been received.
 * 
 * @param aHandshakeType A handshake message type
 * @return TBool Boolean indicating whether a message should be accepted
 */
{
	LOG(Log::Printf(_L("CRecvFinished::AcceptMessage()"));)
	__ASSERT_DEBUG( iStateMachine->History() & ETlsChangeCipherRecv, TlsPanic(ETlsPanicChangeCipherMsgNotReceived) );
	return !iHandshakeMessage && aHandshakeType == ETlsFinishedMsg; 
}

CAsynchEvent* CRecvFinished::ProcessL( TRequestStatus& aStatus )
/**
 * This asynchronous method processes a received Server Finished message.
 * Note that the Finished message takes an integer which is the size of the message 
 * body only (i.e. minus the Handshake header).
 */
{
	LOG(Log::Printf(_L("CRecvFinished::ProcessL()"));)

	TPtr8 ptr( iRecordParser.HandshakeParser()->Message() );
   TInt nLen = ptr.Length();
	iHandshakeMessage = new(ELeave)CFinishedMsg( nLen - KTlsHandshakeHeaderSize);
   LOG(Log::Printf(_L("iHandshakeMessage %x - %x"), iHandshakeMessage, (TUint)iHandshakeMessage + sizeof( CFinishedMsg ));)
	
	iHandshakeMessage->iRecord.ParseL( ptr );	// Set each item's pointer to the correct part of the received message
#ifdef _DEBUG
	LOG(iHandshakeMessage->iRecord.Dump( KSSLLogDir,KSSLLogFileName);)
#endif
	CFinishedMsg* pFinishedMsg = (CFinishedMsg*)iHandshakeMessage;
	TPtr8 body = pFinishedMsg->iFinishedData.GetBodyDes();

	// Pass the information to the Provider
	iShaPtr = static_cast<CSHA1*>( Handshake().SHA1Verify()->CopyL() ); 
	iMd5Ptr = static_cast<CMD5*>( Handshake().MD5Verify()->CopyL() );
	iTlsProvider->TlsSessionPtr()->VerifyServerFinishedMsgL( iMd5Ptr, iShaPtr, body, aStatus);
		
	ptr.Set( iRecordParser.HandshakeParser()->Message() );
   ptr.SetLength( nLen );
	Handshake().UpdateVerify( ptr );
	iStateMachine->UpdateHistory( ETlsFinishedRecv );

	return Handshake().InitiateTransmitL();	
}


CRecvFinished::~CRecvFinished()
/**
 * Destructor.
 */
{
	LOG(Log::Printf(_L("CRecvFinished::~CRecvFinished()"));)
	delete iShaPtr;
	delete iMd5Ptr;
}

CGenericExtensionList::CGenericExtensionList( CItemBase* aNext )
	: CCompoundList(aNext, KTlsExtensionLength)
{
}

void CGenericExtensionList::ParseL( TPtr8& aDes8 )
{
	if(aDes8.Length() == 0)
		{
		return; // No Extension list at all
		}
	CCompoundListHeader::ParseL( aDes8 );
	TInt nLenExpected = CCompoundListHeader::GetBigEndian();

	if(nLenExpected > aDes8.Length())
		{
		User::Leave( KErrBadDescriptor );
		}
	
	while ( nLenExpected > 0 )
		{
		CGenericExtension *ext = CGenericExtension::NewLC( 0 );
		TRecord record(ext);

		record.ParseL( aDes8 );

		AddNodeL(ext);
		CleanupStack::Pop(ext);

		nLenExpected -= ext->ExtensionLength();
		}
	if(nLenExpected < 0)
		{
		User::Leave( KErrBadDescriptor );
		}
}

CGenericExtension* CGenericExtensionList::Node(TInt aIndex) const
{
	return static_cast<CGenericExtension*>(CCompoundList::Node(aIndex));
}

// End of file
