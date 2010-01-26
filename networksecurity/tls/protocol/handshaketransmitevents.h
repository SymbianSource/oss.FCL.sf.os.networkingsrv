/**
* Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* Header file containing classes for transmitted Handshake protocol messages.
* 
*
*/



/**
 @file HandshakeTransmitEvents.h
*/
#include "tlsevent.h"
#include "tlsconnection.h"
#include <tlstypedef.h>

#ifndef _HANDSHAKETRANSMITEVENTS_H_
#define _HANDSHAKETRANSMITEVENTS_H_

class CHandshake;
class CHandshakeHeader;
class CRecordComposer;
class CHandshakeTransmit : public CTlsEvent
/**
 * @class This abstract class describes transmitted SSL3.0 and TLS1.0 Handshake 
 * protocol messages (event classes). It owns the transmitted message.
 */
{
public:
   CHandshakeTransmit( CTLSProvider& aTlsProvider, CStateMachine& aStateMachine, CRecordComposer& aRecordComposer );
   ~CHandshakeTransmit();
   CHandshake& Handshake();

   CHandshakeHeader* HandshakeMessage() const;

protected:
   void ComposeHandshakeHeader( TInt aHistoryUpdate, ETlsHandshakeMessage aHandshakeMessage, TDesC8& aDesComposeMsg );

protected:
   CHandshakeHeader* iHandshakeMessage; // Currently processed outgoing message
   CRecordComposer& iRecordComposer;
};

/////////////////////////////////////////////////////////////////////////////////////////////////
/** SSL3.0/TLS1/0 TRANSMITTED MESSAGES **/

enum EClientHelloStates
{
	ETlsGetSessionInfo,				/** Get Session information */
	ETlsGetCiphers,					/** Get the list of proposed ciphers */
	ETlsComposeHello				/** Compose the Hello message */
};

enum EClientCertificateStates
{
	ETlsGetCertInfo,				/** Get the encoded Client certificate */
	ETlsComposeClientCert			/** Compose the Client certificate message */
};

enum EClientKeyExchangeStates
{
	ETlsGetKeyExchangeMsg,			/** Get the Client key exchange message data */
	ETlsComposeKeyExchange			/** Compose the message. */
};

enum ECertificateVerifyStates
{
	ETlsGetSignature,				/** Get the signed info (signature). */
	ETlsComposeCertVerify			/** Compose the Certificate Verify message */
};

enum EFinishedStates
{
	ETlsGetFinishedMsg,				/** Get the Finished message data */
	ETlsComposeFinished				/** Compose the message. */
};

class CClientHello : public CHandshakeTransmit
/**
 * @class This class is used to process a Client Hello message (transmitted by the protocol).
 */
{
public:
   CClientHello( CTLSProvider& aTlsProvider, CStateMachine& aStateMachine, CRecordComposer& aRecordComposer );
   ~CClientHello();
   
   virtual CAsynchEvent* ProcessL( TRequestStatus& aStatus );
private:
   EClientHelloStates iClientHelloStates;	// Client Hello states, used to compose the message
   TPtr8 iBody;								// Body of Client Hello message
   RArray<TTLSCipherSuite> iCipherList; //CTlsProvider::CipherSuites returns an array rather than descriptor
};

class CClientKeyExch : public CHandshakeTransmit
/**
 * @class This class is used to process a Client Key Exchange message (transmitted by the protocol).
 */
{
public:
   CClientKeyExch( CTLSProvider& aTlsProvider, CStateMachine& aStateMachine, CRecordComposer& aRecordComposer );
   virtual CAsynchEvent* ProcessL( TRequestStatus& aStatus );
   ~CClientKeyExch();
   
private:
	EClientKeyExchangeStates iClientKeyExcStates;	// Client Key exchange states, used for message composition
	HBufC8* iKeyExchBuf;							// Buffer for the Key exchange message
	TPtr8 iBody;									// Body of Client Key exchange message

};

class CClientCertificate : public CHandshakeTransmit
/**
 * @class This class is used to process a Client Certificate message (transmitted by the protocol).
 * @brief This message is used to authenticate a client.
 */
{
public:
   CClientCertificate( CTLSProvider& aTlsProvider, CStateMachine& aStateMachine, CRecordComposer& aRecordComposer );
   virtual CAsynchEvent* ProcessL( TRequestStatus& aStatus );
   ~CClientCertificate();

private:
   EClientCertificateStates iClientCertStates;	// Client Certificate states, used to compose the message.
   RPointerArray<HBufC8> iCertArray;
};

class CCertificateVerify : public CHandshakeTransmit
/**
 * @class This class is used to process a Client Verify message (transmitted by the protocol).
 */
{
public:
   CCertificateVerify( CTLSProvider& aTlsProvider, CStateMachine& aStateMachine, CRecordComposer& aRecordComposer );
   ~CCertificateVerify();
   virtual CAsynchEvent* ProcessL( TRequestStatus& aStatus );

private:
	ECertificateVerifyStates iCertVerifyStates;	// Certificate Verify states, used to compose the message.
	HBufC8* iSignature;
};

class CSendFinished : public CHandshakeTransmit
/**
 * @class This class is used to process a Finished message (transmitted by the protocol).
 */
{
public:
   CSendFinished( CTLSProvider& aTlsProvider, CStateMachine& aStateMachine, CRecordComposer& aRecordComposer );
   ~CSendFinished();
   virtual CAsynchEvent* ProcessL( TRequestStatus& aStatus );

private:
	EFinishedStates iFinishedStates;	// Finished states, used to compose the message.
	HBufC8* iFinishedMsg;				// Buffer for the Finished message
	TPtr8 iBody;						// Body of the Finished message
	CSHA1* iShaHashPtr;					// Copy of SHA1 hash object, used to hash the handshake messages
	CMD5*  iMd5HashPtr;					// Copy of MD5 hash object, used to hash the handshake messages
};

// Inline methods

// CHandshakeTransmit inline methods.
inline CHandshakeTransmit::CHandshakeTransmit( CTLSProvider& aTlsProvider, CStateMachine& aStateMachine, CRecordComposer& aRecordComposer ) :
   CTlsEvent( &aTlsProvider, &aStateMachine ),
   iRecordComposer( aRecordComposer )
{
}

inline CHandshakeHeader* CHandshakeTransmit::HandshakeMessage() const
{
   return iHandshakeMessage;
}

inline CHandshake& CHandshakeTransmit::Handshake()
/**
 * This method returns a reference to a Handshake negotiation state machine.
 */
{
	return (CHandshake&) *iStateMachine;
}

// CClientHello inline methods
inline CClientHello::CClientHello( CTLSProvider& aTlsProvider, CStateMachine& aStateMachine, CRecordComposer& aRecordComposer ) :
   CHandshakeTransmit( aTlsProvider, aStateMachine, aRecordComposer ),
   iClientHelloStates( ETlsGetSessionInfo ),
   iBody( NULL, 0 )
{
}

// CClientKeyExch inline methods
inline CClientKeyExch::CClientKeyExch( CTLSProvider& aTlsProvider, CStateMachine& aStateMachine, CRecordComposer& aRecordComposer ) :
	CHandshakeTransmit( aTlsProvider, aStateMachine, aRecordComposer ),
	iClientKeyExcStates( ETlsGetKeyExchangeMsg ),
	iBody( NULL, 0 )
{
	iKeyExchBuf = NULL;
}

// CClientCertificate inline methods
inline CClientCertificate::CClientCertificate( CTLSProvider& aTlsProvider, CStateMachine& aStateMachine, CRecordComposer& aRecordComposer ) :
   CHandshakeTransmit( aTlsProvider, aStateMachine, aRecordComposer ),
   iClientCertStates( ETlsGetCertInfo )
{
	   
}

// CCertificateVerify inline methods
inline CCertificateVerify::CCertificateVerify( CTLSProvider& aTlsProvider, CStateMachine& aStateMachine, CRecordComposer& aRecordComposer ) :
   CHandshakeTransmit( aTlsProvider, aStateMachine, aRecordComposer ),
   iCertVerifyStates( ETlsGetSignature )
{
	iSignature = NULL;
}

// CSendFinished inline methods
inline CSendFinished::CSendFinished( CTLSProvider& aTlsProvider, CStateMachine& aStateMachine, CRecordComposer& aRecordComposer ) :
   CHandshakeTransmit( aTlsProvider, aStateMachine, aRecordComposer ),
   iFinishedStates( ETlsGetFinishedMsg ),
   iBody ( NULL, 0 ),
   iShaHashPtr ( NULL ),
   iMd5HashPtr ( NULL )	
{
	iFinishedMsg = NULL;
}

#endif
