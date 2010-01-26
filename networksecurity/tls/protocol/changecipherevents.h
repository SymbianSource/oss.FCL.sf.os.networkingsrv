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
* Change Cipher Spec protocol events class header file.
* Describes a concrete class for SSL3.0/TLS1.0 Change Cipher spec messages (events),
* sent and received.
* 
*
*/



/**
 @file ChangeCipherEvents.h
*/

#ifndef _CHANGECIPHEREVENTS_H_
#define _CHANGECIPHEREVENTS_H_

#include "tlsevent.h"

const TUint8 KChangeCipherSpecMsgLength = 1;	// Length of ChangeCipherSpec message.
const TUint8 KChangeCipherSpecMsg = 1;			// Content of ChangeCipherSpec message.

class CHandshake;
class CRecordComposer;
class CStateMachine;
class CTLSProvider;
class CSendChangeCipherSpec : public CTlsEvent
/**
 * @class This class describes a Change Cipher Spec message sent by the protocol.
 * @brief This message consists of a single byte of value one, which is compressed
 * and encrypted under the current connection state.
 *
 * Note that Cipher spec messages (sent or received) are not part of the Handshake protocol 
 * and as such do not derive from the CHandshakeTransmit or CHandshakeReceive classes.
 */
{
public:
   CSendChangeCipherSpec( CTLSProvider& aTlsProvider, CStateMachine& aStateMachine, CRecordComposer& aRecordComposer );
   virtual CAsynchEvent* ProcessL( TRequestStatus& aStatus );
   CHandshake& Handshake();		// Used to access the iTransmitList (list of events to transmit).

private:
   CRecordComposer& iRecordComposer;	// Used to compose the transmitted message.
   TBuf8<KChangeCipherSpecMsgLength> iCipherSpecMsg;
   const TUint8* iMsgPtr;
};

inline CSendChangeCipherSpec::CSendChangeCipherSpec( CTLSProvider& aTlsProvider, CStateMachine& aStateMachine, CRecordComposer& aRecordComposer ) :
	CTlsEvent( &aTlsProvider, &aStateMachine ),
	iRecordComposer( aRecordComposer )
{
	iMsgPtr = &KChangeCipherSpecMsg;
}

inline CHandshake& CSendChangeCipherSpec::Handshake()
/**
 * This method returns a reference to a Handshake negotiation state machine.
 */
{
	return (CHandshake&) *iStateMachine;
}



class CRecordParser;
class CRecvChangeCipherSpec : public CTlsEvent
/**
 * @class This class describes a Change Cipher Spec message received by the protocol
 * (i.e., sent by the server). 
 * @brief This message consists of a single byte of value one, which is compressed
 * and encrypted under the current connection state.
 *
 */
{
public:
   CRecvChangeCipherSpec( CStateMachine& aStateMachine, CRecordParser& aRecordParser );
   virtual TBool AcceptRecord( TInt aRecordType ) const;
   virtual CAsynchEvent* ProcessL( TRequestStatus& aStatus );

private:
   CRecordParser& iRecordParser;	// Used to parse the received message.
};

inline CRecvChangeCipherSpec::CRecvChangeCipherSpec( CStateMachine& aStateMachine, CRecordParser& aRecordParser ) :
   CTlsEvent( NULL, &aStateMachine ),
   iRecordParser( aRecordParser )
{
}
#endif

