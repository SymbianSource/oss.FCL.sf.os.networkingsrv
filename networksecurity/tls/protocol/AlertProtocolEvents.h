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
* Alert protocol messages class header file.
* Describes a concrete class for SSL3.0/TLS1.0 Alert Protocol messages (events).
* These messages convey the severity of the message and a description of the alert.  
* 
*
*/



/**
 @file AlertProtocolEvents.h
*/
 
#ifndef _ALERTPROTOCOLEVENTS_H_
#define _ALERTPROTOCOLEVENTS_H_

#include "tlsevent.h"

const TUint8 KAlertMsgLength = 2;	// Length of an Alert message.

// Alert level
enum ETlsAlertLevel
{
	EAlertWarning = 1,					/** Warning */
	EAlertFatal = 2						/** Fatal alert */
};

class CRecordComposer;
class CStateMachine;
class CSendAlert : public CTlsEvent
/**
 * @class This class describes an Alert message sent by the protocol.
 * @brief This message consists of 2 bytes describing an Alert level and its
 * description. It is compressed and encrypted under the current connection state.
 *
 * Note that Alert messages (sent or received) are not part of the Handshake protocol 
 * and as such do not derive from the CHandshakeTransmit or CHandshakeReceive classes.
 */
{
public:
   CSendAlert( CStateMachine& aStateMachine, CRecordComposer& aRecordComposer );

   virtual CAsynchEvent* ProcessL( TRequestStatus& aStatus );

private:
   CRecordComposer& iRecordComposer;
   TBuf8<KAlertMsgLength> iAlertMsg; //message to send
};

class CRecordParser;
class CRecvAlert : public CTlsEvent
/**
 * @class This class describes an Alert message received by the protocol (i.e., sent 
 * by the server). 
 * @brief This message consists of 2 bytes describing an Alert level and its
 * description. It is decompressed and decrypted under the current connection 
 * state.
 */
{
public:
   CRecvAlert( CStateMachine& aStateMachine, CRecordParser& aRecordParser, CSendAlert& aSendAlert );

   virtual TBool AcceptRecord( TInt aRecordType ) const;
   virtual CAsynchEvent* ProcessL( TRequestStatus& aStatus );
public:
   CRecordParser& iRecordParser;
   CSendAlert& iSendAlert;
};

// Inline methods
inline CSendAlert::CSendAlert( CStateMachine& aStateMachine, CRecordComposer& aRecordComposer ) :
   CTlsEvent( NULL, &aStateMachine ),
   iRecordComposer( aRecordComposer )
/**
 * Constructor
 */
{
}

inline CRecvAlert::CRecvAlert( CStateMachine& aStateMachine, CRecordParser& aRecordParser, CSendAlert& aSendAlert ) :
   CTlsEvent( NULL, &aStateMachine ),
   iRecordParser( aRecordParser ),
   iSendAlert( aSendAlert )
/**
 * Constructor
 */
{
}

#endif

