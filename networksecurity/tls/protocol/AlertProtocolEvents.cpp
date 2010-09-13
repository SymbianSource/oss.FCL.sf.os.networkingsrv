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
// SSL3.0 and TLS1.0 Alert protocol source file.
// Describes the implementation of the Alert protocol (send and receive)
// events classes
// 
//

/**
 @file AlertProtocolEvents.cpp
*/

#include "AlertProtocolEvents.h"
#include "tlsrecorditem.h"
#include "recordprotocolevents.h"
#include "tlshandshake.h"
#include "applicationdata.h"
#include <sslerr.h>

// @todo There must be a means of notifying the TLS Provider when a fatal 
// alert happens (as this invalidates the session; i.e., no new connections
// can subsequently be made with that session id). Can ClearSessionCache be 
// used but identifying only one session (as opposed to all a client's 
// sessions)?

//TLS specific error <-> alert code mapping
struct TTLSErrorAlertCodeMap 
{
   TInt iTLSErrorCode;
   TInt iAlertCode;
};

const TUint KWarningAlertsCount = 2;

enum ETLSAlertCode {
   EAlertclose_notify = 0, 

   EAlertunexpected_message = 10,
   EAlertbad_record_mac = 20,
   EAlertdecryption_failed = 21,
   EAlertrecord_overflow = 22,
   EAlertdecompression_failure = 30,
   EAlerthandshake_failure = 40,
   EAlertbad_certificate = 42,
   EAlertunsupported_certificate = 43,
   EAlertcertificate_revoked = 44,
   EAlertcertificate_expired = 45,
   EAlertcertificate_unknown = 46,
   EAlertillegal_parameter = 47,
   EAlertunknown_ca = 48,
   EAlertaccess_denied = 49,
   EAlertdecode_error = 50,
   EAlertdecrypt_error = 51,
   EAlertexport_restriction = 60,
   EAlertprotocol_version = 70,
   EAlertinsufficient_security = 71,
   EAlertinternal_error = 80,
   EAlertuser_canceled = 90,
   EAlertno_renegotiation = 100
};

const TTLSErrorAlertCodeMap glbTLSErrorAlertCodeMap[] = 
{//the first two alerts are warnings the rest is fatal
   {KErrSSLAlertCloseNotify, 0},          //close_notify(0), 
   {KErrSSLAlertNoRenegotiation,100},     //no_renegotiation(100),

   {KErrSSLAlertUnexpectedMessage,10},    //unexpected_message(10),
   {KErrSSLAlertBadRecordMac,20},         //bad_record_mac(20),
   {KErrSSLAlertDecryptionFailed,21},     //decryption_failed(21),
   {KErrSSLAlertRecordOverflow,22},       //record_overflow(22),
   {KErrSSLAlertDecompressionFailure,30}, //decompression_failure(30),
   {KErrSSLAlertHandshakeFailure,40},     //handshake_failure(40),
   {KErrSSLAlertBadCertificate,42},       //bad_certificate(42),
   {KErrSSLAlertUnsupportedCertificate,43},//unsupported_certificate(43),
   {KErrSSLAlertCertificateRevoked,44},   //certificate_revoked(44),
   {KErrSSLAlertCertificateExpired,45},   //certificate_expired(45),
   {KErrSSLAlertCertificateUnknown,46},   //certificate_unknown(46),
   {KErrSSLAlertIllegalParameter,47},     //illegal_parameter(47),
   {KErrSSLAlertUnknownCA,48},            //unknown_ca(48),
   {KErrSSLAlertAccessDenied,49},         //access_denied(49),
   {KErrSSLAlertDecodeError,50},          //decode_error(50),
   {KErrSSLAlertDecryptError,51},         //decrypt_error(51),
   {KErrSSLAlertExportRestriction,60},    //export_restriction(60),
   {KErrSSLAlertProtocolVersion,70},      //protocol_version(70),
   {KErrSSLAlertInsufficientSecurity,71}, //insufficient_security(71),
   {KErrSSLAlertInternalError,80},        //internal_error(80),
   {KErrSSLAlertUserCanceled,90}         //user_canceled(90),
};
//the error which apparently don't map to any alert
//		KErrSSLAlertAccessDenied:
//		KErrSSLAlertDecodeError:	
//		KErrSSLAlertDecryptError:	

CAsynchEvent* CSendAlert::ProcessL( TRequestStatus& aStatus )
{
	// @todo The processing code can be put into a separate function (code reuse/reduce bloat).
	// @todo Ensure that alerts are set up correctly (i.e., the code that should cause an 
	// alert to be sent is set up correctly.

	TInt error = iStateMachine->LastError();
	LOG(Log::Printf(_L("CSendAlert::ProcessL(). Error value = %d"), error ));
   iAlertMsg.SetLength( 0 );
	
	switch ( error )
	{
		case KErrEof:
			{	// CTlsConnection::Recv() completion status when there is no more data to 
				// receive. This is not a SSL/TLS error.
				TRequestStatus* p=&aStatus;
				User::RequestComplete( p, KErrNone );
				
				return NULL; //stop the state machine
			}
		case KErrSSLAlertCloseNotify:
			{
			    if ( iStateMachine->iStatus.Int() == KRequestPending ||
			            iRecordComposer.CurrentPos() != 0 ||
			            iRecordComposer.UserData()!= NULL )
			        {
			        LOG(Log::Printf(_L("Previous data send request is in the pending state"));)
			        return this;
			        }
				//Upon sending the close_notify from server report KErrEof to the application 
				//to be intact with existing behaviour.	
				iStateMachine->SetLastError( KErrEof );
				iAlertMsg.Append( EAlertWarning );
				iAlertMsg.Append( EAlertclose_notify );
				iRecordComposer.SetNext( this );
            break;
			}
	    case KErrDisconnected:
			{
			/* Fix for the TLS Client hang issue DEF130128.
			 * Server might have disconnected the TLS connection. 
			 * Terminate the state machine */
				TRequestStatus* p=&aStatus;
				User::RequestComplete(p, KErrDisconnected);

				return NULL; //stop the state machine
			}
      case KErrArgument:
         {
            iStateMachine->SetLastError( KErrSSLAlertIllegalParameter );
				iAlertMsg.Append( EAlertFatal );
				iAlertMsg.Append( EAlertillegal_parameter );
				iRecordComposer.SetNext( NULL );
            break;
         }
		case KErrCancel:
			{// A user_canceled alert should be followed by a close_notify alert. So this
			 // event will be the next one to be processed. 
				iAlertMsg.Append( EAlertWarning );
				iAlertMsg.Append( EAlertclose_notify );
				iRecordComposer.SetNext( NULL );
            break;
			}
		default:			
			{	//find the matching alert code to send
            TUint nIndex = 0;
            while ( nIndex < sizeof( glbTLSErrorAlertCodeMap )/sizeof( glbTLSErrorAlertCodeMap[0] ) &&
               glbTLSErrorAlertCodeMap[nIndex].iTLSErrorCode != error )
               {
               nIndex++;
               }
				// Set the message content and its record type.
            iAlertMsg.Append( nIndex >= KWarningAlertsCount ? EAlertFatal : EAlertWarning );
				iAlertMsg.Append( nIndex < sizeof( glbTLSErrorAlertCodeMap )/sizeof( glbTLSErrorAlertCodeMap[0] ) ?
               glbTLSErrorAlertCodeMap[nIndex].iAlertCode : EAlertunexpected_message );
            if ( iAlertMsg[0] == EAlertFatal )
            {
				   iRecordComposer.SetNext( NULL );
            }
            break;
			}

	} //switch

	iRecordComposer.SetUserData( &iAlertMsg );
	iRecordComposer.SetRecordType( ETlsAlertContentType );
	iRecordComposer.ResetCurrentPos();
	return iRecordComposer.ProcessL( aStatus );
}

CAsynchEvent* CRecvAlert::ProcessL( TRequestStatus& aStatus )
{
	// Get the Alert message contents from the Record Parser's iPtrHBuf (this descriptor is
	// always set to point to the decrypted (when necessary) data.
	TPtr8 alertMsg( NULL, 0 );
	alertMsg.Set( iRecordParser.PtrHBuf() ); 
   User::LeaveIfError( alertMsg.Length() != KAlertMsgLength ? KErrSSLAlertUnexpectedMessage : KErrNone );
	TUint8 alertLevel = alertMsg[0];
	TUint8 alertDesc = alertMsg[1];
	LOG(Log::Printf(_L("CRecvAlert::ProcessL(). Alert level = %d"), alertLevel ));
	LOG(Log::Printf(_L("CRecvAlert::ProcessL(). Alert description = %d"), alertDesc ));

	if ( alertLevel == EAlertFatal )
	    {
	TRequestStatus* p=&aStatus;
	    User::RequestComplete( p, KErrSSLAlertHandshakeFailure );
	    iStateMachine->SetLastError( KErrSSLAlertHandshakeFailure );
	    }
	else if ( alertLevel == EAlertWarning )
	   {// In all circumstances, when a warning alert is received, we carry on as normal.
		// There is no need to set the next event as this will be unchanged from normal
		// operation. For a Close_notify alert, we must send one in response.
		// So the next event will be CSendAlert sending a close-notify alert.
	    TRequestStatus* p=&aStatus;
	    User::RequestComplete( p, KErrNone );
      if ( alertDesc == EAlertclose_notify )
         {
         iStateMachine->SetLastError( KErrSSLAlertCloseNotify );
         return &iSendAlert;
         }
      else if ( alertDesc != EAlertno_renegotiation )
         {
         return &iRecordParser;
         }
      else if ( iRecordParser.ReadActive() )
         {//we must be in data mode already to receive this alert
      //if alertDesc == EAlertno_renegotiation for the moment it means that re-negotiation completed successfully
         return NULL;
         }
      alertDesc = EAlertillegal_parameter;
      }
	//Complete the request immediately and close the connection.
   //and set the statemachine error code to return to the client
   TUint nIndex = 0;
   while ( nIndex < sizeof( glbTLSErrorAlertCodeMap )/sizeof( glbTLSErrorAlertCodeMap[0] ) &&
      glbTLSErrorAlertCodeMap[nIndex].iAlertCode != alertDesc )
      {
      nIndex++;
      }
   iStateMachine->SetLastError( nIndex < sizeof( glbTLSErrorAlertCodeMap )/sizeof( glbTLSErrorAlertCodeMap[0] ) ?
      glbTLSErrorAlertCodeMap[nIndex].iTLSErrorCode : KErrSSLAlertIllegalParameter );

	return NULL;
}

TBool CRecvAlert::AcceptRecord( TInt aRecordType ) const
/**
 * This method accepts an Alert event. Alerts are always accepted.
 */
{
	LOG(Log::Printf(_L("CRecvAlert::AcceptRecord()\n"));)  
	return aRecordType == ETlsAlertContentType; 
}

