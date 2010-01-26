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
* SSL3.0 and TLS1.0 Handshake negotiation header file.
* This file describes the Handshake negotiation state machine.
* 
*
*/



/**
 @file TlsHandshake.h
*/

#ifndef _TLSHANDSHAKE_H_
#define _TLSHANDSHAKE_H_

#include <x509cert.h>
#include <comms-infras/statemachine.h>
#include "LOGFILE.H"
#include <tlstypedef.h>

// History of the Handshake negotiation (i.e. events that have occured).
enum ETlsHandshakeHistory {
	ETlsHandshakeStart = 0,					/** Start of handshake negotiation */
	ETlsClientHelloSent = 0x0001,			/** Client Hello has been sent */
	ETlsServerHelloRecv = 0x0002,			/** Server Hello has been received */
	ETlsServerCertificateRecv = 0x0004,		/** Server Certificate has been received */
	ETlsServerKeyExchRecv = 0x0008,			/** Server Key Exchange has been received */
	ETlsCertificateReqRecv = 0x0010,			/** Certificate Request has been received */
	ETlsServerHelloDoneRecv = 0x0020,		/** Server Hello Done has been received */
	ETlsClientCertificateSent = 0x0040,		/** Client Certificate has been sent */
	ETlsClientKeyExchSent = 0x0080,			/** Client Key Exchange has been sent */
	ETlsCertificateVerifySent = 0x0100,		/** Certificate Verify has been sent */
	ETlsFinishedSent = 0x0200,				/** Client Finished has been sent */
	ETlsFinishedRecv = 0x0400,				/** Server Finished has been received */
	ETlsChangeCipherSent = 0x0800,			/** Change CipherSpec has been sent */
	ETlsChangeCipherRecv = 0x1000,			/** Change CipherSpec has been received */
	ETlsFullHandshake = 0x2000,				/** Full Handshake negotiation */
	ETlsAbbreviatedHandshake = 0x4000,		/** Abbreviated handshake negotiation */
	ETlsUsingPskKeyExchange = 0x8000		/** Using PSK for the key exchange */
};

const TInt8 KNullSessionId = 0;				// Zero value session id. 

class CTlsConnection;
class CTlsEvent;
class CSHA1;  
class CMD5;
class CSendAlert;
class CTLSSession;
class CHandshake : public CStateMachine
/** 
 * @class This class describes a state machine which handles SSL3.0/TLS1.0
 * handshake negotiations.
 */
{
public:
	static CHandshake* NewL( CTlsConnection& aTlsConnection ); 
   
	~CHandshake();
	void StartL( TRequestStatus* aClientStatus, MStateMachineNotify* aStateMachineNotify );

	CTlsEvent* InitiateReceiveL();
	CTlsEvent* InitiateTransmitL();
	void UpdateVerify( TDesC8& aMessage );
	CSHA1* SHA1Verify() const;
	CMD5*  MD5Verify() const;
	CSendAlert* SendAlert() const;
	TPtr8& ComposeMsg();
	TBool SessionReUse() const;
	CX509Certificate*& ServerCert();
	TAlgorithmId& SignatureAlg();
	void SetCertificateVerifyReqd( TBool aCertVerify );
	TBool CertificateVerifyReqd() const;
	void SetNegotiatedVersion( const TTLSProtocolVersion* aTlsVersion );
	TSglQue<CTlsEvent>& TxMessageList();
	void DestroyTxList();
	void AddToList( CTlsEvent& aMsgItem);
   void GetServerAddrInfo( TTLSServerAddr& serverInfo );
   void ResetCryptoAttributes();
   CTLSSession*& TlsSession();

   CTlsEvent* NextTxEvent();       //fetches the next event from the Tx queue and shifts
                                    //the queue iterator one element up
	
	void Cancel(TInt aLastError);


protected:
	CHandshake( CTlsConnection& aTlsConnection );
	void ConstructL( CTlsConnection& aTlsConnection );

	virtual void DoCancel();

protected:
	CTlsConnection& iTlsConnection;		// Reference to the TlsConnection object that's negotiating a handshake
	CSHA1* iSHA1Verify;					// SHA1 hash object, used to hash the handshake messages
	CMD5*  iMD5Verify;					// MD5 hash object, used to hash the handshake messages
public:
	TSglQue<CTlsEvent> iTransmitList;	// List of events (messages) to transmit
protected:
	TSglQueIter<CTlsEvent> iTxListIter;	// List iterator
	CSendAlert* iSendAlert;				// Pointer to a 'Send Alert' object
public:
	TPtr8 iComposeMsg;					// Pointer to a state machine fragment passed to a record composer object
	CX509Certificate* iServerCert;		// Server certificate. Used to process ServerKeyExchange message
	TAlgorithmId iSigAlg;				// Signature algorithm. Used to process the CertificateVerify message.
protected:
	TBool iCertVerify;					// Boolean value indicating whether the CertificateVerify message should be sent.
};


// Inline methods

inline CHandshake::CHandshake(CTlsConnection& aTlsConnection) :
   iTlsConnection(aTlsConnection),
   iTransmitList( CTlsEvent::TxOffset() ),
   iTxListIter( iTransmitList ),
   iComposeMsg(NULL, 0),
   iCertVerify(EFalse)
/**
 * Constructor. 
 * This method does the following:
 * 1 - Initialise a reference to the secure connection object.
 * 2 - Initialise iComposeMsg, the pointer descriptor used to access the data 
 *	   in the state machine's heap descriptor object, to NULL
 * 3 - Set the history of the negotiation, iHistory to the start. This object 
 *     keeps track of the negotiation and which messages have been sent or 
	   received thus far.
 */
{
	LOG(Log::Printf(_L("CHandshake::CHandshake()\n"));)
	iHistory = ETlsHandshakeStart; 
}

inline CTlsEvent* CHandshake::NextTxEvent()
{
   return iTxListIter++;
}

inline TPtr8& CHandshake::ComposeMsg()
/** 
 * This method returns a pointer descriptor (to the state machine's data fragment)
 */
{
	LOG(Log::Printf(_L("CHandshake::ComposeMsg()\n"));)
    return iComposeMsg;
}

inline CX509Certificate*& CHandshake::ServerCert()
/** 
 * This method returns a reference to the Server's certificate
 */
{
	LOG(Log::Printf(_L("CHandshake::ServerCert()\n"));)
    return iServerCert;
}

inline TAlgorithmId& CHandshake::SignatureAlg()
/** 
 * This method returns a reference to the Key exchange algorithm.
 */
{
	LOG(Log::Printf(_L("CHandshake::SignatureAlg()\n"));)
    return iSigAlg;
}

inline void CHandshake::SetCertificateVerifyReqd( TBool aCertVerify )
{
   iCertVerify = aCertVerify;
}

inline TBool CHandshake::CertificateVerifyReqd() const
/** 
 * This method returns a reference to a boolean which indicates whether the Certificate
 * Verify message should be sent.
 */
{
	LOG(Log::Printf(_L("CHandshake::CertificateVerifyReqd()\n"));)
    return iCertVerify;
}

inline CSendAlert* CHandshake::SendAlert() const
/** 
 * This method returns a pointer to a 'Send Alert' object. It is used when the 
 * handshake state machine has to send an alert.
 */
{
	LOG(Log::Printf(_L("CHandshake::SendAlert()\n"));)
	return iSendAlert;
}

inline CSHA1* CHandshake::SHA1Verify() const
/** 
 * This method returns a pointer to a SHA1 object. This is used to calculate a 
 * SHA hash of messages.
 */
{
	LOG(Log::Printf(_L("CHandshake::SHA1Verify()\n"));)
	return iSHA1Verify;
}

inline CMD5* CHandshake::MD5Verify() const
/** 
 * This method returns a pointer to a MD5 object. This is used to calculate a 
 * MD5 hash of messages.
 */
{
	LOG(Log::Printf(_L("CHandshake::MD5Verify()\n"));)
	return iMD5Verify;
}

inline TSglQue<CTlsEvent>& CHandshake::TxMessageList()
/**
 * This method returns the list header (of the messages to be transmitted).
 */
{
	LOG(Log::Printf(_L("CHandshake::TxMessageList()\n"));)
	return iTransmitList;
}

#endif
