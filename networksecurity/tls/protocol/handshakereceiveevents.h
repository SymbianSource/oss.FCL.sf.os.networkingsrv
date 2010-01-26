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
* Header file containing classes for received Handshake protocol messages.
* Note that the Hello Request message, though a Handshake protocol message,
* is handled separately. 
* 
*
*/



/**
 @file HandshakeReceiveEvents.h
*/
#include "tlsevent.h"
#include "tlshandshakeitem.h"
#include "tlsconnection.h"
#include <tlstypedef.h>

#ifndef _HANDSHAKERECEIVEEVENTS_H_
#define _HANDSHAKERECEIVEEVENTS_H_

class CHandshakeHeader;
class CRecordParser;
class CHandshake;
class CStateMachine;
class CTLSProvider;
class CTLSSession;
class CHandshakeReceive : public CTlsEvent
/**
 * @class This abstract class describes received SSL3.0 and TLS1.0 Handshake protocol
 * messages (event classes). It owns the received message.
 */
{
public:
   CHandshakeReceive( CTLSProvider& aTlsProvider, CStateMachine& aStateMachine, CRecordParser& aRecordParser );
   ~CHandshakeReceive();

   CHandshake& Handshake(); //MUST NOT be called from CHelloRequest since that 
                           //is processed from CRecvAppData state machine
   virtual TBool AcceptMessage( const TUint8 aHandshakeType ) const = 0; // Determines object to accept a Handshake message.
   static TInt RxOffset();

protected:
   CHandshakeHeader* iHandshakeMessage; // Currently processed incoming message
   CRecordParser& iRecordParser;
   TSglQueLink iRxlink;			// Link object (singly linked list of received messages)
};


inline CHandshakeReceive::CHandshakeReceive( CTLSProvider& aTlsProvider, CStateMachine& aStateMachine, CRecordParser& aRecordParser ) :
   CTlsEvent( &aTlsProvider, &aStateMachine ),
   iRecordParser( aRecordParser )
{
}

inline CHandshake& CHandshakeReceive::Handshake()
{
	return (CHandshake&) *iStateMachine;
}

inline TInt CHandshakeReceive::RxOffset()
{
	return _FOFF(CHandshakeReceive, iRxlink);
}
/////////////////////////////////////////////////////////////////////////////////////////////////
//
class CHandshakeParser : public CTlsEvent
/**
 * @class This class collects a full handshake message and picks the next event  
 * class (from its list of allowed ones) to process the message.
 */
{
public:
   CHandshakeParser( CStateMachine& aStateMachine, CRecordParser& aRecordParser );
   ~CHandshakeParser();

   virtual CAsynchEvent* ProcessL( TRequestStatus& aStatus );
   virtual TBool AcceptRecord( TInt aRecordType ) const;
   
   TPtr8 Message();
   void SetMessageAsUserDataL( TInt aWaitingFor );
   void DestroyRxList();
   void AddToList( CHandshakeReceive& aRxMsgItem);

   CHandshake& Handshake();

protected:
   TInt ParseHeaderL();		//	Sets iNextEvent to a proper message parser
   CTlsEvent* LookUpEventL( const TUint8 aHandshakeType );
   CAsynchEvent* ProcessNextL( TRequestStatus& aStatus );

protected:
   TSglQue<CHandshakeReceive> iMessageList;		// List of expected Handshake protocol messages to receive
   TSglQueIter<CHandshakeReceive> iRxListIter;	// List iterator
   CRecordParser& iRecordParser;
   HBufC8* iMessage;							// Received handshake message 
   TPtr8 iMessagePtr;							// Pointer to the buffer returned by the Handshake Parser.
   TInt iWaitingFor;							// Number of bytes to read
   TUint8   iMessageType;     //received message type
};


/////////////////////////////////////////////////////////////////////////////////////////////////
/** SSL3.0/TLS1.0 RECEIVED MESSAGES **/

class CServerHello : public CHandshakeReceive
/**
 * @class This class represents a Server Hello message (received by the protocol).
 */
{
public:
   CServerHello( CTLSProvider& aTlsProvider, CStateMachine& aStateMachine, CRecordParser& aRecordParser );
   ~CServerHello();

   virtual TBool AcceptMessage( const TUint8 aHandshakeType ) const;
   virtual CAsynchEvent* ProcessL( TRequestStatus& aStatus );

protected:
   TBool iCipherListRead; //for re-negotiation only due to a very bad TLS provider
   RArray<TTLSCipherSuite> iCipherList; //---------""------------------------
};


class CCertificateReq : public CHandshakeReceive
/**
 * @class This class represents a Certificate Request message (received by the protocol).
 */
{
public:
   CCertificateReq( CTLSProvider& aTlsProvider, CStateMachine& aStateMachine, CRecordParser& iRecordParser );

   virtual TBool AcceptMessage( const TUint8 aHandshakeType ) const;
   virtual CAsynchEvent* ProcessL( TRequestStatus& aStatus );
};


class CServerCertificate : public CHandshakeReceive
/**
 * @class This class represents a Server Certificate message (received by the protocol).
 */
{
public:
   CServerCertificate( CTLSProvider& aTlsProvider, CStateMachine& aStateMachine, CRecordParser& iRecordParser );

   virtual TBool AcceptMessage( const TUint8 aHandshakeType ) const;
   virtual CAsynchEvent* ProcessL( TRequestStatus& aStatus );
};

class CServerKeyExch : public CHandshakeReceive
/**
 * @class This class represents a Server Key exchange message (received by the protocol).
 */
{
public:
   CServerKeyExch( CTLSProvider& aTlsProvider, CStateMachine& aStateMachine, CRecordParser& aRecordParser );

   virtual TBool AcceptMessage( const TUint8 aHandshakeType ) const;
   virtual CAsynchEvent* ProcessL( TRequestStatus& aStatus );

protected:
   void CreateMessageL( TTLSKeyExchangeAlgorithm aKeyExchange, TAlgorithmId aSignAlgorithm );
};


class CServerHelloDone : public CHandshakeReceive
/**
 * @class This class represents a Server Hello Done message (received by the protocol).
 */
{
public:
   CServerHelloDone( CTLSProvider& aTlsProvider, CStateMachine& aStateMachine, CRecordParser& aRecordParser );

   virtual TBool AcceptMessage( const TUint8 aHandshakeType ) const;
   virtual CAsynchEvent* ProcessL( TRequestStatus& aStatus );

};

class CRecvFinished : public CHandshakeReceive
/**
 * @class This class represents a Finished message from the server (received by the protocol).
 */
{
public:
   CRecvFinished( CTLSProvider& aTlsProvider, CStateMachine& aStateMachine, CRecordParser& aRecordParser );
   ~CRecvFinished();

   virtual TBool AcceptMessage( const TUint8 aHandshakeType ) const;
   virtual CAsynchEvent* ProcessL( TRequestStatus& aStatus );

private:
	CSHA1* iShaPtr;					// Copy of SHA1 hash object, used to hash the handshake messages
	CMD5*  iMd5Ptr;					// Copy of MD5 hash object, used to hash the handshake messages
};


// Inline methods - CHandshakeParser
inline CHandshakeParser::CHandshakeParser( CStateMachine& aStateMachine, CRecordParser& aRecordParser ) :
   CTlsEvent( NULL, &aStateMachine ),	
   iMessageList( CHandshakeReceive::RxOffset() ),
   iRxListIter( iMessageList ),
   iRecordParser( aRecordParser ),
   iMessagePtr(NULL, 0),
   iWaitingFor( KTlsHandshakeHeaderSize )
{
	LOG(Log::Printf(_L("CHandshakeParser::CHandshakeParser()\n"));)
}

inline CHandshake& CHandshakeParser::Handshake()
/**
 * This method returns a reference to a Handshake negotiation state machine.
 */
{
	LOG(Log::Printf(_L("CHandshakeParser::CHandshakeParser()\n"));)
	return (CHandshake&) *iStateMachine;
}

// Inline method - CServerHello
inline CServerHello::CServerHello( CTLSProvider& aTlsProvider, CStateMachine& aStateMachine, CRecordParser& aRecordParser ) :
   CHandshakeReceive( aTlsProvider, aStateMachine, aRecordParser )
{
}

// Inline method - CCertificateReq
inline CCertificateReq::CCertificateReq( CTLSProvider& aTlsProvider, CStateMachine& aStateMachine, CRecordParser& aRecordParser ) :
   CHandshakeReceive( aTlsProvider, aStateMachine, aRecordParser )
{
}

// Inline method - CServerCertificate
inline CServerCertificate::CServerCertificate( CTLSProvider& aTlsProvider, CStateMachine& aStateMachine, CRecordParser& aRecordParser ) :
   CHandshakeReceive( aTlsProvider, aStateMachine, aRecordParser )
{
}

// Inline method - CServerKeyExch
inline CServerKeyExch::CServerKeyExch( CTLSProvider& aTlsProvider, CStateMachine& aStateMachine, CRecordParser& aRecordParser ) :
   CHandshakeReceive( aTlsProvider, aStateMachine, aRecordParser )
{
}

// Inline method - CServerHelloDone
inline CServerHelloDone::CServerHelloDone( CTLSProvider& aTlsProvider, CStateMachine& aStateMachine, CRecordParser& aRecordParser ) :
   CHandshakeReceive( aTlsProvider, aStateMachine, aRecordParser )
{
}

// Inline method - CRecvFinished
inline CRecvFinished::CRecvFinished( CTLSProvider& aTlsProvider, CStateMachine& aStateMachine, CRecordParser& aRecordParser ) :
   CHandshakeReceive( aTlsProvider, aStateMachine, aRecordParser ),
   iShaPtr( NULL ),
   iMd5Ptr( NULL )
{
}

#endif
