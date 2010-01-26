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
* SSL3.0/TLS1.0 specific asynchronous events class header file.
* Describes an abstract, base class for SSL3.0/TLS1.0 protocol-specific
* asynchronous events.
* 
*
*/



/**
 @file TlsEvent.h
*/

#include <comms-infras/statemachine.h>
#include <comms-infras/asynchevent.h>
#include "LOGFILE.H"
#include <e32std.h>

#ifndef _TLSEVENT_H_
#define _TLSEVENT_H_


class CTLSProvider;

class CTlsEvent : public CAsynchEvent
/**
 * Describes a SSL3.0/TLS1.0 protocol specific event (i.e. Handshake, ChangeCipherSpec, 
 * Alert and Application data protocol messages, Record composition, Record parsing, etc).
 *
 * @internalComponent
 */
{
public:
   CTlsEvent( CTLSProvider* aTlsProvider, CStateMachine* aStateMachine );
   virtual TBool AcceptRecord( TInt aRecordType ) const;	// Looks up an object to process a Record payload
   void SetTlsProvider( CTLSProvider* aTlsProvider );
   static TInt Offset();
   static TInt TxOffset();

protected:
   CTLSProvider* iTlsProvider;	///< Reference to cryptography service provider (not required by the Alert Protocol).
   TSglQueLink iSlink;			///< Link object (Record protocol content type list)
   TSglQueLink iTxlink;			///< Link object (Transmitted message list)
};


// Inline functions

inline CTlsEvent::CTlsEvent( CTLSProvider* aTlsProvider, CStateMachine* aStateMachine ) :
   CAsynchEvent( aStateMachine ),
   iTlsProvider( aTlsProvider )
{
}

inline void CTlsEvent::SetTlsProvider( CTLSProvider* aTlsProvider )
{
   iTlsProvider = aTlsProvider;
}

/**
 * Returns the offset of a CTlsEvent object. Used by the Record protocol type list.
 */
inline TInt CTlsEvent::Offset()
{
	return _FOFF( CTlsEvent, iSlink );
}

/**
 * Returns the offset of a CTlsEvent object. Used by the Transmitted events list.
 */
inline TInt CTlsEvent::TxOffset()
{
	return _FOFF( CTlsEvent, iTxlink );
}

#endif
