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
// Record protocol events header file.
// Describes classes used for Record parsing and composition.
// 
//

/**
 @file 
*/

#include <tlstypedef.h>
#include "tlsevent.h"
#include "tlsrecorditem.h"
#include "genericsecuresocket.h"

#ifndef _RECORDPROTOCOLEVENTS_H_
#define _RECORDPROTOCOLEVENTS_H_

// Enumeration for the Record protocol read and write states
enum ETlsRecordProtocolStates
{
	ETlsFragmentRecord,			/** Record fragmentation/re-assembly */
	ETlsCompressRecord,			/** Record compression/decompress */
	ETlsCipherRecord,			/** Record security (MAC calculation/verification + encryption/decryption) */
	ETlsTransmitRecord,			/** Record transmission */
	ETlsReceiveRecordHeader,	/** Record protocol header reception */
	ETlsReceiveRecordBody		/** Record protocol body reception */
};

// Set by CSendAppData/CRecvAppData costructor as its history (iHistory)
const TInt KTlsApplicationData = (-1);
const TInt KTlsCompressionSize = 1;	// Number of bytes used for compression
                                       
class RSocket;
class CStateMachine;
class CTLSSession;
class CHandshakeParser;
class CSendAlert;

class CRecordParser : public CTlsEvent
/**
 * This class handles Record protocol reception. 
 * It parses the record header, picks a parser for the record payload/body (from its 
 * list of allowed ones) and reads/decrypts the payload.
 *
 * The CRecordParser class (active when a record header has been received) has a list 
 * of CTlsEvent derived objects representing expected SSL\TLS record content types 
 * that can arrive at any moment.
 */
{
	friend class CTlsConnection;	// Used to create or destroy the object

public:
	void ReConstructL( CStateMachine* aStateMachine, const TPtr8& aHeldData, CSendAlert& aSendAlert );
	void Reset();
   void ChangeCipher();

	MGenericSecureSocket& Socket() const;

	virtual CAsynchEvent* ProcessL( TRequestStatus& aStatus );
	void SetUserData( TDes8* aUserData );
	void SetUserMaxLength( TInt aUserMaxLength );
	TDes8* UserData() const;
	TPtr8& HeldData();
	TPtr8& PtrHBuf();
	CHandshakeParser* HandshakeParser() const;
   void IgnoreAppData( TInt aIgnoreRecordAllowed );
	TBool IsAppData() const;
   TInt ReadActive() const;

	void CancelAll();
	TInt CurrentPos() const;

   ETlsRecordType TlsRecordType() const;


protected:
	CRecordParser( MGenericSecureSocket& aSocket, CTLSProvider& aTlsProvider);
	~CRecordParser();

	CTlsEvent* LookUpEventL( TInt aRecordType );
	TInt ParseHeaderL();	// Sets iNext (the next event) to a proper payload parser
	void DestroyList();
   void DispatchData();

private:
	void ReConstructL();
protected:
	CHandshakeParser* iHandshakeParser;	
protected:	
	MGenericSecureSocket& iSocket;

	HBufC8* iDataIn;	// Encrypted (if applicable) data, owned by the class
											// The header is read in first, and then overwritten by the record body.
	TDes8* iUserData;	// Points to a User (application data or handshake) buffer when 
						// parsing application data or processing handshake messages.
	TInt iUserMaxLength;// Maximum length of the user's buffer
	
	TInt64 iReadSequenceNum;	// Read state (reception) sequence number
public:
	TPtr8 iHeldData;			// Buffer for held data. It's length != 0 when we have 
								// undelivered data in the state machine fragment, CStateMachine::iFragment
protected:
	ETlsRecordProtocolStates iReadState;
	CTLSSession* iActiveTlsSession; //active crypto session (own by CRecordParser)
	TUint iIgnoreRecord:1;		// In case when application data arrives before server hello.
	TUint iIgnoreRecordAllowed:1;
public:
	TPtr8 iPtrHBuf;				// Used to pass a HBufC8 and TBuf8 as a TDes8 and to keep 
								// the descriptor over an asynchronous call.
protected:	
	ETlsRecordType iTlsRecordType;
	HBufC8* iDecryptedData;		// Buffer for decrypted data received from the Provider.

	TSglQue<CTlsEvent> iExpRecordTypes;	// List of expected record protocol types
};


//
class CRecordComposer : public CTlsEvent
/**
 * This class handles Record protocol transmissions. 
 * It fragments a long stream into record blocks, adds a MAC, encrypts the payload, 
 * adds a record header and sends the record.
 *
 * @note The iWriteSequenceNum member (write state sequence number) is specified by the
 * TLS specification (section 6.1) as a uint64. For Beech, there is no uint64 type 
 * although one is defined for Cedar (EKA2). As configurability issues have not yet been 
 * finalised, and because a TInt64 type appears more than adequate for our needs, this 
 * type has been used for both this and the corresponding iReadSequenceNum variables.
 */
{
	friend class CTlsConnection;	// Used to create or destroy the object

public:
	void ReConstructL( CStateMachine* aStateMachine, TInt aCurrentPos );
	void Reset();
   void ChangeCipher();

	MGenericSecureSocket& Socket() const;

	virtual CAsynchEvent* ProcessL( TRequestStatus& aStatus );
	void SetUserData( TDesC8* aUserData );
	TDesC8* UserData() const;
	TInt CurrentPos() const;
	void ResetCurrentPos();
	TInt SentPlainTextLength() const;
	const TDesC8& SupportedCompression() const;

	void CancelAll();

	const TTLSProtocolVersion* TlsVersion() const;
	void SetVersion( const TTLSProtocolVersion* aTlsVersion );
	void SetRecordType( ETlsRecordType aTLSRecordType );
	
	~CRecordComposer();

protected:
	CRecordComposer( MGenericSecureSocket& aSocket, CTLSProvider& aTlsProvider );

	void ComposeHeader( TInt aLength );
	TBool IsAppData() const;

protected:
	MGenericSecureSocket& iSocket;
   
	TBuf8<KTlsRecordPrealocate> iDataOut; // Secure (if applicable) data

	TDesC8* iUserData;				// Points to the user's buffer (application data or handshake messages) 
	TInt iCurrentPos;				// Maintains the current position while the 
									// record body is being split up into fragments
	TInt iSentPlainTextLength;		// Keeps the length of currently sent plaintext data

	TInt64 iWriteSequenceNum;		// Write state (transmission) sequence number
	ETlsRecordProtocolStates iWriteState;
   CTLSSession* iActiveTlsSession; //active crypto session (own by CRecordParser) this is aonly a reference

	TPtrC8 iPtrHBuf;				// Descriptor used to pass a HBufC8 and TBuf8 as a 
									// TDes8 and keep the descriptor over an asynch call
	TPtr8 iPtrEncryptTo;			// Pointer descriptor containing encrypted data 
	const TTLSProtocolVersion* iTlsVersion; // In case a different version is used for the 
									// record header than that in TLS Provider
									// (for TLS -> SSL fall back (reference only). It  
									// will be set to NULL as soon as it is used.
	TBuf8<KTlsCompressionSize> iSupportedCompression; // Supported compression. Always NULL at present.
	HBufC8* iEncryptedData;			// Buffer for encrypted data passed to the Provider.
};



// Inline functions - CRecordParser class
inline CRecordParser::CRecordParser( MGenericSecureSocket& aSocket, CTLSProvider& aTlsProvider ) :
   CTlsEvent( &aTlsProvider, 0 ),
   iSocket( aSocket ),
   iReadSequenceNum (0),
   iHeldData( 0, 0 ),
   iReadState( ETlsReceiveRecordHeader ),
   iPtrHBuf( 0, 0 ),
   iDecryptedData( NULL ),
   iExpRecordTypes( CTlsEvent::Offset() )
/** 
 * Basic construction. Each state machine calls ReConstructL() to set the parser
 * up more appropriately for its task.
 */
{
	LOG(Log::Printf(_L("CRecordParser::CRecordParser()\n"));)
}

inline ETlsRecordType CRecordParser::TlsRecordType() const
{
   return iTlsRecordType;
}

inline void CRecordParser::IgnoreAppData( TInt aIgnoreRecordAllowed )
{
   iIgnoreRecordAllowed = aIgnoreRecordAllowed;
}

inline TInt CRecordParser::ReadActive() const
{
   return iActiveTlsSession != NULL;
}

inline TBool CRecordParser::IsAppData() const
/**
 * Indicates whether application data is being parsed.
 * The only case this could happen is if we've received a record fragment 
 * marked as application data (see CRecordParser::LookUpEventL)
 */ 
{
	LOG(Log::Printf(_L("CRecordParser::IsAppData()\n"));)
	return !iNext; 
}

inline MGenericSecureSocket& CRecordParser::Socket() const
/**
 * Returns a reference to the Generic Socket object
 */
{
	LOG(Log::Printf(_L("CRecordParser::Socket()\n"));)
	return iSocket;
}

inline TPtr8& CRecordParser::HeldData()
/**
 * Returns any held data left over from handling application data (if
 * the client's buffer was not big enough, or from processing handshake messages.
 */
{
	LOG(Log::Printf(_L("CRecordParser::HeldData()\n"));)
	return iHeldData;
}

inline TPtr8& CRecordParser::PtrHBuf()
/**
 * Returns the iPtrHBuf descriptor. It is only called when an Alert is being
 * processed (as it is pointing to the message content). It is better to use this than the 
 * iUserData descriptor as we have no control over the size of the application's data buffer
 * when in data mode. The buffer could be too small to contain the Alert message.
 */
{
	LOG(Log::Printf(_L("CRecordParser::PtrHBuf()\n"));)
	return iPtrHBuf;
}

inline CHandshakeParser* CRecordParser::HandshakeParser() const
/**
 * Returns a pointer to the Handshake parser object.
 */
{
	LOG(Log::Printf(_L("CRecordParser::HandshakeParser()\n"));)
	return iHandshakeParser;
}

inline void CRecordParser::SetUserData( TDes8* aUserData )
/**
 * Sets the record parser's iUserData pointer to the user's buffer
 * (application data or handshake messages).
 */
{
	LOG(Log::Printf(_L("CRecordParser::SetUserData()\n"));)

	iUserData = aUserData;
}

inline void CRecordParser::SetUserMaxLength( TInt aUserMaxLength )
/**
 * Sets the record parser's iUserMaxLength attribute
 */
{
	LOG(Log::Printf(_L("CRecordParser::SetUserMaxLength()\n"));)

	iUserMaxLength = aUserMaxLength;
}

inline TDes8* CRecordParser::UserData() const
/**
 * Returns a pointer to the iUserData descriptor. 
 */
{
	LOG(Log::Printf(_L("CRecordParser::UserData()\n"));)
	return iUserData;
}



// Inline functions - CRecordComposer class

inline CRecordComposer::CRecordComposer( MGenericSecureSocket& aSocket, CTLSProvider& aTlsProvider ) :
   CTlsEvent( &aTlsProvider, 0 ),
   iSocket( aSocket ),
   iWriteSequenceNum (0),
   iWriteState( ETlsFragmentRecord ),
   iPtrHBuf( 0, 0 ),
   iPtrEncryptTo( 0, 0 ),
   iEncryptedData( NULL )
/** 
 * Basic construction. Each state machine calls ReConstructL() to set the composer
 * up more appropriately for its task.
 */
{
	LOG(Log::Printf(_L("CRecordComposer::CRecordComposer()\n"));)
	iSupportedCompression.Append(ENullCompression);	// Only NULL compression is supported
}

inline void CRecordComposer::SetVersion( const TTLSProtocolVersion* aTlsVersion )
/**
 * Sets the protocol version to use in the record header. 
 */
{
	LOG(Log::Printf(_L("CRecordComposer::SetVersion()\n"));)
	iTlsVersion = aTlsVersion;
}

inline const TDesC8& CRecordComposer::SupportedCompression() const
/**
 * This method returns the supported compression for the connection.
 */
{
	LOG(Log::Printf(_L("CRecordComposer::SupportedCompression()\n"));)
	return iSupportedCompression;
}

inline const TTLSProtocolVersion* CRecordComposer::TlsVersion() const
{
   return iTlsVersion;
}

inline void CRecordComposer::Reset()
{
	iWriteSequenceNum = 0;	// reset the sequence number
	iActiveTlsSession = NULL;	// Owned by CRecordParser if != CTlsconnection:;iTlsSession
}

inline void CRecordComposer::SetRecordType( ETlsRecordType aTLSRecordType )
/**
 * Sets the Record protocol's content type (ChangeCipherSpec,
 * Alert, Handshake or Application data).
 */
{
	LOG(Log::Printf(_L("CRecordComposer::SetRecordType()\n"));)

	TUint8* dataPtr = (TUint8*) iDataOut.Ptr();
	dataPtr[KTlsRecordTypeOffset] = (TUint8)aTLSRecordType;
}

inline TBool CRecordComposer::IsAppData() const
/**
 * Returns a boolean value indicating whether the connection is
 * in Application data mode.
 */
{
	LOG(Log::Printf(_L("CRecordComposer::IsAppData()\n"));)

	TUint8* dataPtr = (TUint8*) iDataOut.Ptr();
	return dataPtr[KTlsRecordTypeOffset] == ETlsAppDataContentType;
}

inline TInt CRecordComposer::SentPlainTextLength() const
/**
 * Returns the amount/length of the plain text data that is 
 * transmitted.
 */
{
	LOG(Log::Printf(_L("CRecordComposer::SentPlainTextLength()\n"));)
	return iSentPlainTextLength;
}

inline TDesC8* CRecordComposer::UserData() const
/**
 * Returns a pointer to the iUserData descriptor (i.e. the 
 * state machine's (handshake or application) data buffer.
 */
{
	LOG(Log::Printf(_L("CRecordComposer::UserData()\n"));)
	return iUserData;
}

inline void CRecordComposer::SetUserData( TDesC8* aUserData )
/**
 * Sets the iUserData descriptor pointing to the state machine's
 * (client/application data or handshake) buffer.
 */
{
	LOG(Log::Printf(_L("CRecordComposer::SetUserData()\n"));)
	iUserData = aUserData;
}

inline TInt CRecordComposer::CurrentPos() const
/**
 * Returns the current position in a record. It is used to keep track
 * when a record is being fragmented for transmission.
 */
{
	LOG(Log::Printf(_L("CRecordComposer::CurrentPos()\n"));)
	return iCurrentPos;
}

inline void CRecordComposer::ResetCurrentPos()
/**
 * Resets the current position in a record to zero. It is called when 
 * application data tranmission is fully complete.
 */
{
	LOG(Log::Printf(_L("CRecordComposer::ResetCurrentPos()\n"));)
	iCurrentPos = 0;
}

inline MGenericSecureSocket& CRecordComposer::Socket() const
/**
 * Returns a reference to the Generic Socket object
 */
{
	LOG(Log::Printf(_L("CRecordComposer::Socket()\n"));)
	return iSocket;
}

#endif
