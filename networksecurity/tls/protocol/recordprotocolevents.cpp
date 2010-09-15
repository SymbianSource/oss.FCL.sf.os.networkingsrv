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
// Record protocol source file. 
// Describes the implementation of the Record protocol classes: Record parser 
// (CRecordParser) and the Record composer (CRecordComposer).
//
//

/**
 @file
*/

#include <es_sock.h>
#include "recordprotocolevents.h"
#include "AlertProtocolEvents.h"
#include "handshakereceiveevents.h"
#include "changecipherevents.h"
#include "tlshandshake.h"

void CRecordParser::ReConstructL( CStateMachine* aStateMachine, const TPtr8& /*aHeldData*/, CSendAlert& aSendAlert )
/**
 * This method sets the iStateMachine and iNext (CAsynchEvent*) data members 
 * inherited from the CAsynchEvent class, in addition to some of its own data members. 
 * It also creates a list of the expected Record protocol content types.
 */
    {
	LOG(Log::Printf(_L("CRecordParser::ReConstructL(CStateMachine*, const TPtr8&, CSendAlert&)"));)

	iStateMachine = aStateMachine;

    if (!iDataIn)
        {
        iDataIn = HBufC8::NewL( KTlsRecordPrealocate );
        }

    ReConstructL();

	CTlsEvent* alertItem = new(ELeave)CRecvAlert( *iStateMachine, *this, aSendAlert );
	//coverity[leave_without_push]
  	LOG(Log::Printf(_L("CRecvAlert %x - %x"), alertItem, (TUint)alertItem + sizeof( CRecvAlert ));)
	iExpRecordTypes.AddLast( *alertItem );

	iHandshakeParser = new(ELeave)CHandshakeParser( *iStateMachine, *this);
  	LOG(Log::Printf(_L("CHandshakeParser %x - %x"), iHandshakeParser, (TUint)iHandshakeParser + sizeof( CHandshakeParser ));)
	iExpRecordTypes.AddLast( *iHandshakeParser );
    }


void CRecordParser::ReConstructL()
/**
 * This method resets the state of the object as if it had just been created.
 */
    {
	LOG(Log::Printf(_L("CRecordParser::ReConstructL()"));)

	DestroyList();

	iHeldData.Zero();

	iReadState = iHeldData.Length() ? ETlsFragmentRecord : ETlsReceiveRecordHeader; 
   
	// Add TLS record protocol content types to the list of expected types
	// It is important that the Handshake parser object is last in the list
	if (iStateMachine->History() == ETlsHandshakeStart)
    	{
		// Change Cipher Spec protocol applies only during handshake negotiation
		__ASSERT_DEBUG( iTlsProvider, TlsPanic(ETlsPanicTlsProviderNotReady));
		CTlsEvent* ccsItem = new(ELeave)CRecvChangeCipherSpec( *iStateMachine, *this );
		//coverity[leave_without_push]
		LOG(Log::Printf(_L("RecvChangeCipherSpec %x - %x"), ccsItem, (TUint)ccsItem + sizeof( CRecvChangeCipherSpec ));)
		iExpRecordTypes.AddLast( *ccsItem );
	    }
	
	iNext = NULL;
    }


CRecordParser::~CRecordParser()
{
	LOG(Log::Printf(_L("CRecordParser::~CRecordParser()"));)

	DestroyList();
	delete iDecryptedData;
   delete iDataIn;
   Reset();
}

void CRecordParser::DestroyList()
{
	LOG(Log::Printf(_L("CRecordParser::DestroyList() of expected record protocol Content types"));)

	CTlsEvent* listItem;
    
   TSglQueIter<CTlsEvent> expListIter( iExpRecordTypes );
    expListIter.SetToFirst(); 
    while ( (listItem = expListIter++) != NULL )
        {
        iExpRecordTypes.Remove(*listItem);
        delete listItem;
        };
}

void CRecordParser::ChangeCipher()
{
   if ( iStateMachine->History() & ETlsChangeCipherSent )
   {
		__ASSERT_DEBUG( iActiveTlsSession != iTlsProvider->TlsSessionPtr(), TlsPanic(ETlsPanicInvalidTlsSession));
      delete iActiveTlsSession;
   }
   iActiveTlsSession = iTlsProvider->TlsSessionPtr();
   iReadSequenceNum = 0;
}

void CRecordParser::Reset()
{
	iReadSequenceNum = 0;	// reset the sequence number
   if ( iTlsProvider && iTlsProvider->TlsSessionPtr() && iActiveTlsSession != iTlsProvider->TlsSessionPtr() )
   {//this could happen just in case we re-negotiated unsuccessfully but passed CTlsProvider::Create()
      delete iActiveTlsSession;
   }
   iActiveTlsSession = NULL;
}

CTlsEvent* CRecordParser::LookUpEventL( TInt aRecordType )
/**
 * This method is used to determine which record protocol content type (handshake,
 * change cipher spec, alert, application data) will process a received message.
 * It cycles through the list of expected record types to see which one would
 * accept the received record.
 * 
 * @param aRecordType An integer representing the content type.
 * @return CTlsEvent* A pointer to the event that will process the received message.
 */
{
	LOG(Log::Printf(_L("CRecordParser::LookUpEventL()"));)

   TSglQueIter<CTlsEvent> expListIter( iExpRecordTypes );
	expListIter.SetToFirst();
	CTlsEvent* pEvent;

	while ( ((pEvent = expListIter++) != 0) && !pEvent->AcceptRecord( aRecordType ) )
      {
      }
   
	// When processing application data, NULL is returned as this is the only class processing it
	if ( !pEvent )
   {
      if ( aRecordType != ETlsAppDataContentType )
      {
		   User::Leave( KErrSSLAlertUnexpectedMessage );
      }
      else if ( iStateMachine->History() != KTlsApplicationData )
      {//application adta received during handshaking
         if ( iIgnoreRecordAllowed )
         {//in case client's initiated re-negotiation while the server is still sending some app data
   			LOG(Log::Printf(_L("Application data received while re-negotiating => skip the record"));)
            iIgnoreRecord = 1;
         }
         else
         {
     		   User::Leave( KErrSSLAlertUnexpectedMessage );
         }
      }
   }
   else
   {//once i've received any record but application data one no record will be ignored
      iIgnoreRecordAllowed = 0;
      iIgnoreRecord = 0;
   }
	return pEvent;

}

TInt CRecordParser::ParseHeaderL()
/**
 * This method parses the record header to check that the Protocol version is as 
 * expected. It also retrieves and returns the length of the record fragment from 
 * the Record Protocol header.
 */
{
	LOG(Log::Printf(_L("CRecordParser::ParseHeaderL()"));)

	TUint8* dataPtr = (TUint8*) iDataIn->Des().Ptr();
   iTlsRecordType = static_cast<ETlsRecordType>(dataPtr[KTlsRecordTypeOffset]);
	iNext = LookUpEventL( iTlsRecordType );

   const TTLSProtocolVersion& tlsVersion = iTlsProvider->Attributes()->iNegotiatedProtocol;
   if ( tlsVersion.iMajor )
      {
	   if ( dataPtr[KTlsRecordVersionOffset] != tlsVersion.iMajor ||
		   dataPtr[KTlsRecordVersionOffset + 1] != tlsVersion.iMinor )
	      {
		   User::Leave( KErrSSLAlertProtocolVersion );
	      }
      }
   //no protocol's been agreed yet => must be one of these
	else if ( (dataPtr[KTlsRecordVersionOffset] != KSSL3_0.iMajor ||
		   dataPtr[KTlsRecordVersionOffset + 1] != KSSL3_0.iMinor) &&
         (dataPtr[KTlsRecordVersionOffset] != KTLS1_0.iMajor ||
		   dataPtr[KTlsRecordVersionOffset + 1] != KTLS1_0.iMinor) )
	      {
		   User::Leave( KErrSSLAlertProtocolVersion );
	      }

	//extract and return the fragment length from the header
	TBigEndian value;
	return value.GetValue( dataPtr + KTlsRecordLengthOffset, KTlsRecordBodyLength );
}

void CRecordParser::CancelAll()
/**
 * Cancel all read functionality.
 * This method should also cancel decompression when it is supported.
 */
{
	LOG(Log::Printf(_L("CRecordParser::CancelAll()"));)

	iSocket.CancelRead();
	// Cancel the asynchronous service request made.
	iSocket.CancelRecv();
	iTlsProvider->CancelRequest();
	if ( iActiveTlsSession )
	    {
		iActiveTlsSession->CancelRequest();
		}

    TRAPD(ret, ReConstructL());
    if ( ret != KErrNone )
        {
        LOG(Log::Printf(_L("ReConstructL() returned the error %d"), ret);)
        }
}

void CRecordParser::DispatchData()
{
	TInt nUserFree = iUserData ? iUserMaxLength - iUserData->Length() : 0; 
	LOG(Log::Printf(_L("CRecordParser::DispatchData() - %x, %d"), iUserData, nUserFree);)

	if ( iHeldData.Length() )
	{
   	LOG(Log::Printf(_L("iHeldData.Length(), %d"), iHeldData.Length());)
		// Client has data left. Assert that its buffer exists.
		__ASSERT_DEBUG( iUserData, TlsPanic(ETlsPanicNoUserData));

		TInt nToCopy = Min( nUserFree, iHeldData.Length() ); // amount of data to copy
		iUserData->Append( iHeldData.Ptr(), nToCopy );
			
		TInt n = iHeldData.Length() - nToCopy;	// Reset the held data ptr to what is left-over
		iHeldData.Set( (TUint8*)iHeldData.Ptr() + nToCopy, n, n );
			
	}	
	else
	{
		iHeldData.Set(iPtrHBuf);

     	LOG(Log::Printf(_L("iHeldData.Length() == 0 %d"), iHeldData.Length());)
      if ( iUserData && nUserFree >= iHeldData.Length() )
		{	// User's data buffer is big enough, copy data directly into it.
			iUserData->Append(iHeldData.Ptr(), iHeldData.Length());
			iHeldData.Zero();
		}
		else if ( iUserData && nUserFree < iHeldData.Length() )
		{	// User's data buffer is not long enough. Copy nUserFree bytes and
			// save the rest for later
			iUserData->Append( iHeldData.Ptr(), nUserFree );
			TInt n = iHeldData.Length() - nUserFree;
			iHeldData.Set( (TUint8*)iHeldData.Ptr() + nUserFree, n, n ); // Left-over data is in iHeldData
		}
	}
#ifdef _DEBUG
   if ( iUserData )
      {
  	   LOG(Log::Printf(_L("CRecordParser::DispatchData() end iUserData->Length() = %d, %d"), iUserData->Length(), iHeldData.Length());)
      }
#endif
}

CAsynchEvent* CRecordParser::ProcessL( TRequestStatus& aStatus )
/**
 * This asynchronous method parses a received SSL/TLS record.
 * The record protocol is responsible for decrypting, message authentication (verifying 
 * data), decompressing and reassembling data. 
 *
 * Note that the record header is transmitted in the clear (i.e., it is not encrypted 
 * or MACed). As such, only the record body is passed through to Security for deciphering
 * and validation. Also, only NULL compression is supported. MAC verification + Decryption 
 * only happen when security services are active.
 * 
 * @param aStatus TRequestStatus object
 * @return CAsynchEvent* A pointer to an asynchronous event (the next event to be processed).
 */
{

	if ( iStateMachine->History() == KTlsApplicationData )
	{
	LOG(Log::Printf(_L("CRecordParser::ProcessL()"));)
	}
	switch ( iReadState )
	{
	case ETlsReceiveRecordHeader:	
		{
			LOG(Log::Printf(_L("CRecordParser::ProcessL() - ETlsReceiveRecordHeader"));)

			// Prepare to receive a new record fragment. Receive the header initially.
			// Parse it and determine the fragment length. Check that the record
			// fragment length does not exceed the allowed maximum for a SSL/TLS record
			if ( iStateMachine->Fragment() )	// For App data rx, this pointer is NULL
			{
				iStateMachine->Fragment()->Des().Zero(); 
			}
			iPtrHBuf.Set( (TUint8*)iDataIn->Des().Ptr(), 0, KTlsRecordHeaderSize );

			iSocket.Recv( iPtrHBuf, 0, aStatus );
			iReadState = ETlsReceiveRecordBody;
			break;
		}
	case ETlsReceiveRecordBody:
		{
			TInt fragLength = ParseHeaderL();	// Returns the length of the fragment
            //parameters swap due to the defect
			LOG(RFileLogger::HexDump(KSSLLogDir,KSSLLogFileName,EFileLoggingModeAppend, NULL, NULL, iPtrHBuf.Ptr(), iPtrHBuf.Length() ));
			LOG(Log::Printf(_L("CRecordParser::ProcessL() - ETlsReceiveRecordBody of length %x"), fragLength);)

    		User::LeaveIfError( fragLength > KTlsRecordMaxBodySize || fragLength < 0 ? KErrSSLAlertRecordOverflow : KErrNone );
         if ( fragLength > iDataIn->Des().MaxLength() )
            {
            delete iDataIn;
            iDataIn = NULL;
            iDataIn = HBufC8::NewL( fragLength );
            }
			
			// Read the record fragment and overwrite the header (already been evaluated)
			iPtrHBuf.Set( (TUint8*)iDataIn->Des().Ptr(), 0, fragLength );	
			iSocket.Recv( iPtrHBuf, 0, aStatus );

			iReadState = ETlsCipherRecord;
			break;
		}
	case ETlsCipherRecord:
		{
			LOG(Log::Printf(_L("CRecordParser::ProcessL() - ETlsCipherRecord"));)

         if ( iIgnoreRecord )
         {//nothing to be done => skip it all
				iReadState = ETlsReceiveRecordHeader;
         }
         else
         {
			   // Decrypt and verify the MACed record fragment, if appropriate
			   // Initialise the buffer for the plain text data.
			   if (iActiveTlsSession) // Security is active
			   {
				   TRecordProtocol type = (TRecordProtocol) iTlsRecordType;
				   User::LeaveIfError( iActiveTlsSession->DecryptAndVerifyL(iPtrHBuf, iDecryptedData, iReadSequenceNum, type ) );
				   // If error is not zero, it should generate an alert.
  				   iPtrHBuf.Set( iDecryptedData->Des() );
				   User::LeaveIfError( iPtrHBuf.Length() > KTlsRecordPlainText ? KErrSSLAlertRecordOverflow : KErrNone );
			   }
	   
			   // Update the sequence number
			   iReadSequenceNum++;	// Increment the sequence number

			   iReadState = ETlsCompressRecord;
         }

         TRequestStatus* p=&aStatus;
			User::RequestComplete( p, KErrNone );

			break;
		}
	case ETlsCompressRecord:
		{
			LOG(Log::Printf(_L("CRecordParser::ProcessL() - ETlsCompressRecord"));)
					
			// De-compresses the SSL/TLS plain text data. Only NULL compression is 
			// currently supported (identity operation). Fall through to the next state. 
			iReadState = ETlsFragmentRecord;
		}
	case ETlsFragmentRecord:
		{
			LOG(Log::Printf(_L("CRecordParser::ProcessL() - ETlsFragmentRecord"));)

			// In application data mode, if the Client's data buffer is big enough,
			// then copy the verified data directly into it as an append. Otherwise,
			// copy as much as is possible (dependent on the free space in the client's
			// buffer.
			// The 'if' statement below solves the case where, in Application data 
			// mode, the clients data buffer (iUserData) is not big enough to hold the 
			// received record fragment. nUserFree is the amount of free space in the 
			// User's data buffer. This is the amount of data that is appended to its
			// buffer whilst the rest is held till the client frees up space.
			
         if ( (iStateMachine->History() == KTlsApplicationData && iTlsRecordType == ETlsAppDataContentType) ||
            (iStateMachine->History() != KTlsApplicationData && iTlsRecordType == ETlsHandshakeContentType))
         {//don't put wrong data into the user buffer
            DispatchData();
         }
	      if ( iHeldData.Length() == 0 )
	      {	//prepare to read new record and clear the statemachine buffer
		      iReadState = ETlsReceiveRecordHeader;
	      }

			TRequestStatus* p=&aStatus;
			User::RequestComplete( p, KErrNone );
        	LOG(Log::Printf(_L("CRecordParser::ProcessL() iNext == %x"), iNext);)
			return iNext;	// record body parser looked up in ParseHeaderL() function 
								// or NULL if in application data mode
		}	// case statement
	default:
		{
			LOG(Log::Printf(_L("CRecordParser::ProcessL() - Default statement, unknown state"));)
			__ASSERT_ALWAYS( EFalse, TlsPanic(ETlsPanicInvalidProcessState));
		}
	}	// switch statement
   
	return this;
}

//
void CRecordComposer::ReConstructL( CStateMachine* aStateMachine, TInt aCurrentPos )
/**
 * This method sets the iStateMachine and iNext (CAsynchEvent*) data members 
 * inherited from the CAsynchEvent class, in addition to some of its own data members.
 * It also sets the record's content type. Once set, this will remain, unless it's 
 * interrupted by an alert or handshake negotiation. In this instance, the 
 * ReConstructL() function will be called again.
 */
{
	LOG(Log::Printf(_L("CRecordComposer::ReConstructL()"));)

	iStateMachine = aStateMachine;
	iWriteState = ETlsFragmentRecord;
	iCurrentPos = aCurrentPos;

	TUint8* dataPtr = (TUint8*) iDataOut.Ptr();
	dataPtr[KTlsRecordTypeOffset] = ETlsAppDataContentType; // Default setting
   
	iNext = 0;
}

void CRecordComposer::ChangeCipher()
{
   if ( iStateMachine->History() & ETlsChangeCipherRecv )
   {
		__ASSERT_DEBUG( iActiveTlsSession != iTlsProvider->TlsSessionPtr(), TlsPanic(ETlsPanicInvalidTlsSession));
      delete iActiveTlsSession;
   }
   iActiveTlsSession = iTlsProvider->TlsSessionPtr();
   iWriteSequenceNum = 0;
}

void CRecordComposer::ComposeHeader( TInt aLength )
/**
 * This method composes a Record protocol header. It sets the Protocol version,
 * and the length of the Protocol message (the length is in big endian format). 
 *
 * Note that the content type has previously been set by the message class which 
 * prepared the Record payload. For the first transmitted message (ClientHello),
 * SSL v3.0 is used (to enable TLS to SSL fallback). The record composer's version
 * object is then set to NULL.
 * Subsequent transmitted messages then use the negotiated protocol version.
 */
{
	LOG(Log::Printf(_L("CRecordComposer::ComposeHeader()"));)

	TUint8* dataPtr = (TUint8*) iDataOut.Ptr();
   if ( iTlsVersion )
      {
   	dataPtr[KTlsRecordVersionOffset] = iTlsVersion->iMajor;
	   dataPtr[KTlsRecordVersionOffset + 1] = iTlsVersion->iMinor;
      }
   else
      {
   	dataPtr[KTlsRecordVersionOffset] = KSSL3_0.iMajor;
	   dataPtr[KTlsRecordVersionOffset + 1] = KSSL3_0.iMinor;
      }
      
	TBigEndian value;
	value.SetValue( (TUint8*)iDataOut.Ptr() + KTlsRecordLengthOffset, KTlsRecordBodyLength, aLength );
}

void CRecordComposer::CancelAll()
/**
 * Cancel all write functionality.
 * This method should also cancel compression when it is supported.
 */
{
	LOG(Log::Printf(_L("CRecordComposer::CancelAll()"));)

	iSocket.CancelSend();
	iTlsProvider->CancelRequest();
	
	if (iActiveTlsSession)
		iActiveTlsSession->CancelRequest();
}

CAsynchEvent* CRecordComposer::ProcessL( TRequestStatus& aStatus )
/**
 * This asynchronous method composes a SSL/TLS record.
 * The record protocol is responsible for fragmenting, compressing, message 
 * authentication (adding a MAC) and encrypting data. It then transmits the result.
 *
 * Compression, MAC computation and Encryption only happen when security services are 
 * active. At present, only NULL compression is supported.
 * 
 * @param aStatus TRequestStatus object
 * @return CAsynchEvent* A pointer to an asynchronous event (the next event to be 
 * processed).
 */
{
	LOG(Log::Printf(_L("CRecordComposer::ProcessL()"));)

	// Get a pointer to the data to be transmitted via the Record layer.
	TDesC8* pFragment = iUserData ? iUserData : iStateMachine->Fragment();
	
	switch ( iWriteState )
	{
	case ETlsFragmentRecord:
		{
			LOG(Log::Printf(_L("CRecordComposer::ProcessL() - ETlsFragmentRecord"));)

			// Fragments the SSL/TLS plain text data.
			// The current position within the plain text buffer is initially set to 
			// zero by the state machine (when CRecordComposer::ReconstructL() is called). 
			// The maximum amount of plain text that can be sent is KTlsRecordPlainText  
			// and this data is stored in the iPtrHBuf descriptor.

			iSentPlainTextLength = Min( pFragment->Length() - iCurrentPos, KTlsRecordPlainTextToSend ); 
			iPtrHBuf.Set( (TUint8*)pFragment->Ptr() + iCurrentPos, SentPlainTextLength() );
			iCurrentPos += iSentPlainTextLength; // keep track of fragmented plain text

			iWriteState = ETlsCompressRecord;
			// Fall through to the next case statement (no TRequestStatus to complete).
		}
	case ETlsCompressRecord:	
		{
			LOG(Log::Printf(_L("CRecordComposer::ProcessL() - ETlsCompressRecord"));)

			// Compresses the SSL/TLS plain text data. 
			// NULL compression only so fall through to the next state
			iWriteState = ETlsCipherRecord;
		}
	case ETlsCipherRecord:
		{
			// Apply a MAC and encrypt the compressed SSL/TLS data.
			// MAC and encryption uses the current SSL/TLS connection state and only 
			// happens when the keys have been created and activated. The connection
			// state specifies the compression, encryption and MAC algorithms.
			// Set iPtrEncryptTo to a maximum length of the correct size (plainText + cipherText length)
			iPtrEncryptTo.Set( (TUint8*)iDataOut.Ptr() + KTlsRecordHeaderSize, 0, iPtrHBuf.Length() + KTlsRecordCipherText);

			// Check that security is active
			if (iActiveTlsSession)
			{ 
   			LOG(Log::Printf(_L("CRecordComposer::ProcessL() - ETlsCipherRecord - Encrypt plain text"));)
				TRecordProtocol type = (TRecordProtocol) iDataOut[KTlsRecordTypeOffset];
				User::LeaveIfError( iActiveTlsSession->EncryptL(iPtrHBuf, iEncryptedData, iWriteSequenceNum, type) );
				iPtrEncryptTo.Copy( iEncryptedData->Des() );
				User::LeaveIfError( iPtrEncryptTo.Length() > iDataOut.MaxLength() ? KErrSSLAlertRecordOverflow : KErrNone );
			}
			else
			{ // No security is active. Copy the data into the iPtrEncryptTo buffer
   			LOG(Log::Printf(_L("CRecordComposer::ProcessL() - ETlsCipherRecord - No Encryption plain text"));)
				iPtrEncryptTo.Copy(iPtrHBuf);
			}

			// Get the record length and compose the header. Prepend it to the record data
			iDataOut.SetLength( iPtrEncryptTo.Length() + KTlsRecordHeaderSize );
			ComposeHeader( iPtrEncryptTo.Length() );		
			iWriteState = ETlsTransmitRecord;
		}
	case ETlsTransmitRecord:
		{
			LOG(Log::Printf(_L("CRecordComposer::ProcessL() - ETlsTransmitRecord"));)
            //parameters swap due to the defect
			LOG(RFileLogger::HexDump(KSSLLogDir,KSSLLogFileName,EFileLoggingModeAppend, NULL, NULL, iDataOut.Ptr(), iDataOut.Length() ));

			// Transmit the Record data (header + data), activate the Write state (if
			// appropriate), increment or reset the Sequence number.
			iSocket.Send( iDataOut, 0, aStatus );
			
			iWriteSequenceNum++;	// Increment the sequence number
			if ( iDataOut[KTlsRecordTypeOffset] == ETlsChangeCipherContentType )
			{
				ChangeCipher();
			}

			iWriteState = ETlsFragmentRecord;
			
			// Check to see if there is any more plain text data to secure and transmit.
			// If yes, process this data. Otherwise, process the next asynchronous event.
			// The content type is reset to Application data as a default
      		
			if ( iCurrentPos >= pFragment->Length() )
			{
				if ( !IsAppData() )
				{	// See CSendAppData::OnCompletion().
					iCurrentPos = 0; // All the plain text has been secured and transmitted
				}

				return iNext;
			}

			break;
		}
	default:
		{
			LOG(Log::Printf(_L("CRecordComposer::ProcessL() - Default statement, unknown state"));)
			break;
		}
	} // switch statement

   return this;
}

CRecordComposer::~CRecordComposer()
/** 
 * Standard Destructor.
 */
{
	LOG(Log::Printf(_L("CRecordComposer::~CRecordComposer()"));)
	delete iEncryptedData;
}
