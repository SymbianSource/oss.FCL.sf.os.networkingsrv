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
// Change Cipher Spec protocol messages implementation file.
// 
//

/**
 @file
*/
  
#include "changecipherevents.h"
#include "tlshandshake.h"
#include "recordprotocolevents.h"
#include "handshakereceiveevents.h"


CAsynchEvent* CSendChangeCipherSpec::ProcessL( TRequestStatus& aStatus )
/** 
 * This message consists of a single byte of value 1, which is compressed and encrypted 
 * under the current connection state. Typically this message activates security services
 * (i.e. encryption + MAC).
 *
 * @param aStatus Request status object
 * @return CAsynchEvent* Pointer to the next asynchronous event to be processed.
 */
{
	LOG(Log::Printf(_L("CSendChangeCipherSpec::ProcessL()\n"));)
		
	// Set the message content and its record type.
	iCipherSpecMsg.Copy( iMsgPtr, KChangeCipherSpecMsgLength );
	CRecordComposer& RecordComposer = iRecordComposer;
	RecordComposer.SetUserData( &iCipherSpecMsg );
	RecordComposer.SetRecordType( ETlsChangeCipherContentType );
	
	// Update the History and set the next event to be processed. The next message to be 
	// transmitted is the Finished message and this will be last in the current list.
	iStateMachine->UpdateHistory( ETlsChangeCipherSent );

   //RecordComposer.ChangeCipher(); happens from CRecordComposer itslf after thei record's been sent
   RecordComposer.SetNext( Handshake().NextTxEvent() );
	return RecordComposer.ProcessL( aStatus );
}

TBool CRecvChangeCipherSpec::AcceptRecord( TInt aRecordType ) const
/** 
 * This method determines whether the first byte of a Record protocol header 
 * (content type) can be accepted by an event.
 *
 * @param aRecordType Integer specifying the Record protocol content type
 * @return TBool Boolean indicating whether or not the record should be accepted by  
 * this event.
 */
{
	LOG(Log::Printf(_L("CRecvChangeCipherSpec::AcceptRecord()\n"));)
	TInt nHistory = iStateMachine->History();
	
	return aRecordType == ETlsChangeCipherContentType && 
		(nHistory & ETlsFullHandshake|ETlsFinishedSent == ETlsFullHandshake|ETlsFinishedSent ||
		nHistory & ETlsAbbreviatedHandshake|ETlsServerHelloRecv == ETlsAbbreviatedHandshake|ETlsServerHelloRecv);
}

CAsynchEvent* CRecvChangeCipherSpec::ProcessL( TRequestStatus& aStatus )
/**
 * This method processes a received Change Cipher Spec message. This message should consist 
 * of a single byte of value 1. It is impossible for any other message to follow a CCS msg 
 * in a TLS record.
 */
{
	LOG(Log::Printf(_L("CRecvChangeCipherSpec::ProcessL()\n"));)

	iStateMachine->UpdateHistory( ETlsChangeCipherRecv ); // Update the Handshake history	
	TPtr8 ccsMsg ( iRecordParser.PtrHBuf() );
   User::LeaveIfError( ccsMsg.Length() != KChangeCipherSpecMsgLength ? KErrSSLAlertUnexpectedMessage : KErrNone );
	TUint8 msgValue = ccsMsg[0];

	if ( msgValue != KChangeCipherSpecMsg )
	{
		LOG(Log::Printf(_L("CRecvChangeCipherSpec::ProcessL - Value of CCS message is NOT equal to 1\n"));)
		User::Leave(KErrArgument);
	}
	LOG(Log::Printf(_L("ChangeCipherSpec message of value %d received"), msgValue );)

	// Reset the length of CRecordParser::iUserData for the next message.
	iRecordParser.UserData()->SetLength(0);
   iRecordParser.ChangeCipher();

	return iRecordParser.ProcessL( aStatus );	// Call the Record Parser to read again from the socket.
}
