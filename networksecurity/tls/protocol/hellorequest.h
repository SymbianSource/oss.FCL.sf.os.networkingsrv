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
* Hello Request message class header file.
* Describes a concrete class for SSL3.0/TLS1.0 Hello Request message (event).
* This message may be sent by the server at any time.
* 
*
*/



/**
 @file HelloRequest.h
*/

#include <comms-infras/statemachine.h>
#include <comms-infras/asynchevent.h>
#include "handshakereceiveevents.h"
#include "tlsconnection.h"
#include "tlshandshakeitem.h"
#include "recordprotocolevents.h"

#ifndef _HELLOREQUEST_H_
#define _HELLOREQUEST_H_

class CTlsConnection;
class CHelloRequest : public CHandshakeReceive,  public MStateMachineNotify
/**
 * Describes a Hello Request message.
 * This (empty) message is a simple notification that the client should begin  
 * the negotiation process anew, by sending a client hello message when convenient.
 */
{
public:
   CHelloRequest( CTlsConnection& aTlsConnection, CStateMachine& aStateMachine );

   // CHandshakeReceive virtuals
   virtual TBool AcceptMessage( const TUint8 aHandshakeType ) const;
   virtual CAsynchEvent* ProcessL( TRequestStatus& aStatus );

   // MStateMachineNotify virtuals
   virtual TBool OnCompletion( CStateMachine* aStateMachine );

private:
   CTlsConnection& iTlsConnection;
};

#endif
