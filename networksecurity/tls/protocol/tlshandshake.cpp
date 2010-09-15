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
// SSL3.0 and TLS1.0 Handshake Negotiation source file.
// Describes the implementation of a Handshake Negotiation state machine.
// 
//

/**
 @file
*/
  
#include "tlshandshakeitem.h"
#include "AlertProtocolEvents.h"
#include "handshakereceiveevents.h"
#include "handshaketransmitevents.h"
#include "recordprotocolevents.h"
#include "changecipherevents.h"
#include "tlsconnection.h"
#include "tlshandshake.h"  

void CHandshakeHeader::ComposeHeader( ETlsHandshakeMessage aHandshakeMessage, TInt aLength )
/**
 * This method composes a handshake header.
 */ 
{
	LOG(Log::Printf(_L("CHandshakeHeader::ComposeHeader()"));)
	
	__ASSERT_DEBUG( iPtr8 != NULL, TlsPanic(ETlsPanicNullPointerToHandshakeHeaderBuffer) );
	iPtr8[KTlsHandshakeTypeOffset] = static_cast<TUint8>(aHandshakeMessage);
   
	TBigEndian value; 
	value.SetValue( iPtr8 + KTlsHandshakeLengthOffset, KTlsHandshakeBodyLength, aLength );	
}

//
CHandshake* CHandshake::NewL(CTlsConnection& aTlsConnection)
/**
 * This method creates a Handshake negotiation object.
 */
{
	LOG(Log::Printf(_L("CHandshake::NewL()"));)

	CHandshake* self = new(ELeave) CHandshake(aTlsConnection);
  	LOG(Log::Printf(_L("self %x - %x"), self, (TUint)self + sizeof( CHandshake ));)
	CleanupStack::PushL(self);
	self->ConstructL(aTlsConnection);
	CleanupStack::Pop(self);
	return self;
}

CHandshake::~CHandshake()
/**
 * Destructor.
 * Destroys the list of handshake messages to transmit, the hash objects and the
 * 'Send Alert' object
 */
{
	LOG(Log::Printf(_L("CHandshake::~CHandshake()"));)

	DestroyTxList();
	delete iSHA1Verify;
	delete iMD5Verify;
	delete iSendAlert;
   delete iServerCert;
}

void CHandshake::DestroyTxList()
{
	LOG(Log::Printf(_L("CHandshake::DestroyTxList() of transmitted handshake messages"));)

	CTlsEvent* listItem;
    
    iTxListIter.SetToFirst(); 
    while ( (listItem = iTxListIter++) != NULL )
        {
        iTransmitList.Remove(*listItem);
        delete listItem;
        };
}
void CHandshake::ConstructL(CTlsConnection& aTlsConnection)
/**
 * Standard 2 phase construction.
 * This method creates and returns pointers to Hash objects; 
 * Re-constructs the record parser and composer objects (setting them up 
 * specifically for Handshake negotiation);
 * Creates an Alert object (for sending Alerts) and
 * Initiates transmission of the first asynchronous event, the Client Hello message.
 *
 * @param aTlsConnection The connection that initiated the handshake negotiation.
 */
{
	LOG(Log::Printf(_L("CHandshake::ConstructL()"));)
   
	iSHA1Verify = CSHA1::NewL();
	iMD5Verify = CMD5::NewL();
   
	iSendAlert = new(ELeave)CSendAlert( *this, aTlsConnection.RecordComposer() );
  	LOG(Log::Printf(_L("iSendAlert %x - %x"), SendAlert(), (TUint)iSendAlert + sizeof( CSendAlert ));)
	aTlsConnection.RecordParser().ReConstructL( this, TPtr8(NULL, 0), *iSendAlert );
	aTlsConnection.RecordComposer().ReConstructL( this, 0 );
	iActiveEvent = InitiateTransmitL(); // pointer to the 1st event/message to transmit
}

void CHandshake::StartL( TRequestStatus* aClientStatus, MStateMachineNotify* aStateMachineNotify )
/**
 * This method starts the handshake negotiation state machine by calling the 
 * Start() method of the base state machine class (CStateMachine).
 *
 * @param aClientStatus Pointer to a TRequestStatus object that completes when  
 * handshake negotiation is completed.
 * @param aStateMachineNotify Pointer to a MStateMachineNotify interface object.
 */
{
	LOG(Log::Printf(_L("CHandshake::StartL()"));)

	// Obtain a reference to a Record parser object and use its Handshake parser
	// to set the user's data descriptor and its length (CRecordParser::iUserData 
	// and iUserMaxLength).
	CRecordParser& RecordParser = iTlsConnection.RecordParser();
	CHandshakeParser* pParser = RecordParser.HandshakeParser();
	pParser->SetMessageAsUserDataL( KTlsHandshakeHeaderSize );

	CStateMachine::Start( aClientStatus, (CAsynchEvent*)iSendAlert, aStateMachineNotify );
}

CTlsEvent* CHandshake::InitiateReceiveL()
/**
 * This method creates a list of Handshake messages that are expected from 
 * the Server.
 */
{
	LOG(Log::Printf(_L("CHandshake::InitiateReceiveL()"));)

	CTLSProvider& tlsProvider = iTlsConnection.TlsProvider();

	CRecordParser& recordParser = iTlsConnection.RecordParser();
	CHandshakeParser* pParser = recordParser.HandshakeParser();
	__ASSERT_DEBUG( pParser != 0, TlsPanic(ETlsPanicNullPointerToHandshakeRecordParser));
	__ASSERT_DEBUG( History() != 0, TlsPanic(ETlsPanicNullStateMachineHistory));
	
	pParser->DestroyRxList();
	CHandshakeReceive* rxItem;

	if ( History() & ETlsFinishedRecv ) // Handshake negotiation is complete
	{
		return NULL;
	}
	
	if ( History() == ETlsClientHelloSent ) // and nothing else has happened
	{
		rxItem = new(ELeave)CServerHello( tlsProvider, *this, recordParser );
		//coverity[leave_without_push]
     	LOG(Log::Printf(_L("CServerHello %x - %x"), rxItem, (TUint)rxItem + sizeof( CServerHello ));)
		pParser->AddToList( *rxItem );
	}
	else if ( History() & ETlsFullHandshake )			// Full handshake
	{
		if ( History() & ETlsServerHelloDoneRecv)		// Server hello done received
		{
			rxItem = new(ELeave)CRecvFinished( tlsProvider, *this, recordParser );
			//coverity[leave_without_push]
     	LOG(Log::Printf(_L("CRecvFinished %x - %x"), rxItem, (TUint)rxItem + sizeof( CRecvFinished ));)
			pParser->AddToList( *rxItem );
		}
		else											// Start of the handshake
		{
			if ( (History() & ETlsUsingPskKeyExchange) == 0)
				{
				rxItem = new(ELeave)CServerCertificate( tlsProvider, *this, recordParser );
				//coverity[leave_without_push]
     			LOG(Log::Printf(_L("CServerCertificate %x - %x"), rxItem, (TUint)rxItem + sizeof( CServerCertificate ));)
				pParser->AddToList( *rxItem );
				}
			
			rxItem = new(ELeave)CServerKeyExch( tlsProvider, *this, recordParser );
			//coverity[leave_without_push]
     		LOG(Log::Printf(_L("CServerKeyExch %x - %x"), rxItem, (TUint)rxItem + sizeof( CServerKeyExch ));)
			pParser->AddToList( *rxItem );

			if ( (History() & ETlsUsingPskKeyExchange) == 0)
				{
				rxItem = new(ELeave)CCertificateReq( tlsProvider, *this, recordParser );
				//coverity[leave_without_push]
   	  			LOG(Log::Printf(_L("CCertificateReq %x - %x"), rxItem, (TUint)rxItem + sizeof( CCertificateReq ));)
				pParser->AddToList( *rxItem );
				}
			
			rxItem = new(ELeave)CServerHelloDone( tlsProvider, *this, recordParser );
			//coverity[leave_without_push]
     		LOG(Log::Printf(_L("CServerHelloDone %x - %x"), rxItem, (TUint)rxItem + sizeof( CServerHelloDone ));)
			pParser->AddToList( *rxItem );
		}
	}
	else if ( History() & ETlsAbbreviatedHandshake )	// Abbreviated handshake	
	{	
		rxItem = new(ELeave)CRecvFinished( tlsProvider, *this, recordParser );
		//coverity[leave_without_push]
     	LOG(Log::Printf(_L("CRecvFinished %x - %x"), rxItem, (TUint)rxItem + sizeof( CRecvFinished ));)
		pParser->AddToList( *rxItem );
	}
	
	return &recordParser;
}

CTlsEvent* CHandshake::InitiateTransmitL()
/**
 * This method initiates transmission of SSL/TLS events.
 * It destroys the current list of events/messages and checks its current 
 * position in terms of the state machine's history. 
 * It also gets references to the TLS provider interface and the Record composer
 * object. (It needs access to cryptography services for the message items and it 
 * needs a record composer to build a TLS record to transmit)
 * 
 * This method is called whenever the protocol has to transmit messages/events, 
 * i.e., at the start of Handshake negotiations and the start of Client authentication. 
 */
{
	LOG(Log::Printf(_L("CHandshake::InitiateTransmitL()"));)

	DestroyTxList();

	// Handshake negotiation is complete
	if ( History() & ETlsFinishedSent )
		return NULL;

	CTLSProvider& tlsProvider = iTlsConnection.TlsProvider();
	CRecordComposer& recordComposer = iTlsConnection.RecordComposer();
	CTlsEvent* txItem;

	if ( History() == ETlsHandshakeStart )
	{
		// Create a ClientHello message, initialise it and add it to the Transmit list
		txItem = new(ELeave)CClientHello( tlsProvider, *this, recordComposer );
		//coverity[leave_without_push]
     	LOG(Log::Printf(_L("CClientHello %x - %x"), txItem, (TUint)txItem + sizeof( CClientHello ));)
		AddToList( *txItem );
	}
	else if ( (History() & ETlsClientKeyExchSent) || (History() & ETlsAbbreviatedHandshake) )
	{
		// As fixed DH is not supported, the Certificate Verify message is always sent,
		// providing a non-NULL Client Certificate message is sent.
		if ( CertificateVerifyReqd() )
		{	
			txItem = new(ELeave)CCertificateVerify( tlsProvider, *this, recordComposer );
			//coverity[leave_without_push]
     	LOG(Log::Printf(_L("CCertificateVerify %x - %x"), txItem, (TUint)txItem + sizeof( CClientHello ));)
			AddToList( *txItem );
		}

		txItem = new(ELeave)CSendChangeCipherSpec( tlsProvider, *this, recordComposer );
		//coverity[leave_without_push]
     	LOG(Log::Printf(_L("CSendChangeCipherSpec %x - %x"), txItem, (TUint)txItem + sizeof( CClientHello ));)
		AddToList( *txItem );

		txItem = new(ELeave)CSendFinished( tlsProvider, *this, recordComposer );
		//coverity[leave_without_push]
     	LOG(Log::Printf(_L("CSendFinished %x - %x"), txItem, (TUint)txItem + sizeof( CClientHello ));)
		AddToList( *txItem );
	}
	else
	{
		if ( History() & ETlsCertificateReqRecv )
		{	
			txItem = new(ELeave)CClientCertificate( tlsProvider, *this, recordComposer );
			//coverity[leave_without_push]
     	LOG(Log::Printf(_L("CClientCertificate %x - %x"), txItem, (TUint)txItem + sizeof( CClientHello ));)
			AddToList( *txItem );
		}
		txItem = new(ELeave)CClientKeyExch( tlsProvider, *this, recordComposer );
		//coverity[leave_without_push]
     	LOG(Log::Printf(_L("CClientKeyExch %x - %x"), txItem, (TUint)txItem + sizeof( CClientHello ));)
		AddToList( *txItem );
	}

   iTxListIter.SetToFirst();
   return NextTxEvent();
}

void CHandshake::SetNegotiatedVersion( const TTLSProtocolVersion* aTlsVersion )
{
	iTlsConnection.RecordComposer().SetVersion( aTlsVersion );
}

void CHandshake::UpdateVerify( TDesC8& aMessage )
/**
 * When the first Finished message is computed, it is necessary to copy the iMD5Verify
 * and iSHA1Verify to new CMD5 & CSHA1 to see all ::Final and be able to update the members 
 * to compute the second Finished message.
 */
{
	LOG(Log::Printf(_L("CHandshake::UpdateVerify()"));)	

	iMD5Verify->Update( aMessage );
	iSHA1Verify->Update( aMessage );
}

void CHandshake::DoCancel()
	{
	LOG(Log::Printf(_L("CHandshake::DoCancel()"));)

    if ( iTlsConnection.TlsSession() )
        {
        iTlsConnection.TlsSession()->CancelRequest();
        }

	iTlsConnection.RecordComposer().CancelAll();
	iTlsConnection.RecordParser().CancelAll();	
	}

void CHandshake::Cancel(TInt aLastError)
    {
	LOG(Log::Printf(_L("CHandshake::Cancel(TInt)"));)
	CStateMachine::Cancel(aLastError);
	if (!iErrorEvent)
		{
		OnCompletion(); // This will cause the instance to be deleted
		}
    }

void CHandshake::AddToList( CTlsEvent& aMsgItem )
{
	LOG(Log::Printf(_L("CHandshake::AddToList()"));)

    TxMessageList().AddLast(aMsgItem);
}

void CHandshake::GetServerAddrInfo( TTLSServerAddr& serverInfo )
{
   iTlsConnection.GetServerAddrInfo( serverInfo );
}

CTLSSession*& CHandshake::TlsSession()
{
   return iTlsConnection.TlsSession();
}

void CHandshake::ResetCryptoAttributes()
{
   iTlsConnection.ResetCryptoAttributes();
}

TBool CHandshake::SessionReUse() const
/** 
 * This method returns a boolean which indicates whether a session should be reused.
 */
{
	LOG(Log::Printf(_L("CHandshake::SessionReUse()"));)
	return iTlsConnection.SessionReUse();
}

CExtensionNode::CExtensionNode(CItemBase* aNext) :
	CConstItem(aNext, KTlsExtensionTypeLength)
{
}

CExtensionNode::~CExtensionNode()
{
}

TInt CExtensionNode::ExtensionLength()
	/**
		For ParseL (internalisation) to work correctly this function must return the total extension length.
		Any derived class must override this function by calling this one and then adding on its extra members.
		Unfortunately we can not walk the record linked list to perform this calculation because GetItemLength
		is not virtual (and not define in CItemBase).
	*/
{
	return GetItemLength();
}


CKnownExtensionNode::CKnownExtensionNode() :
	CExtensionNode(&iOpaqueData),
	iOpaqueData(NULL, KTlsExtensionLength)
{
}

void CKnownExtensionNode::ConstructOpaqueDataWrapperL(CItemBase* aNext)
{
	iOpaqueData.AddNodeL(aNext);
}

CKnownExtensionNode::~CKnownExtensionNode()
{
	// Must not delete the opaque data because it points to members of a class derived from this one...
	iOpaqueData.DropNodes();
}

CCompoundList::~CCompoundList()
	{
	TInt nodeCount = iRecords.Count();
	for(TInt i=0; i<nodeCount; ++i)
		{
		delete iRecords[i].First();
		}
	iRecords.Reset();
	}

void CCompoundList::AddNodeL( CItemBase* aNode )
/** 
 * Adds the item to the tail of the compound list. 
 
 * Once added the item is owned by the iNodes pointer array
 *
 * @param aItemBase the item to be added
 */
	{
	// Add node to end of list. 
	// Each node is managed by a TRecord object
	TRecord record(aNode);
	iRecords.AppendL(record);
	}

TInt CCompoundList::CalcTotalInitialiseLength() const
/** 
 * Calculates the length of the data buffer to hold the data for all items in the 
   dynamic record.
 
 * An initial value must be assigned to the items' headers.
 *
 * @return the buffer length
 */
	{
	TInt n = CCompoundListHeader::CalcTotalInitialiseLength();
	TInt nodeCount = iRecords.Count();
	for(TInt i=0; i<nodeCount; ++i)
		{
		n += iRecords[i].CalcTotalInitialiseLength();
		}
	return n;
	}

void CCompoundList::InitialiseL( TPtr8& aBuf8 )
/** 
 * Initialises the pointers and length for all the items in the dynamic record
 *
 * @param aBuf8  An 8 bit modifiable pointer descriptor to hold the item's data
 */
	{
	TInt tmp = aBuf8.Length();
	CCompoundListHeader::InitialiseL( aBuf8 );
	TInt n = aBuf8.Length();

	TInt nodeCount = iRecords.Count();
	for(TInt i=0; i<nodeCount; ++i)
		{
		iRecords[i].InitialiseL( aBuf8 );
		}
	CCompoundListHeader::SetBigEndian( aBuf8.Length() - n );
	}

void CCompoundList::ParseL( TPtr8& /*aDes8*/ )
/** 
 * Checks the input and initialises the items' pointers to point to the beginning 
 * of theirs input buffers
 *
 * @param aDes8 An 8 bit modifiable pointer descriptor representing the descriptor to be checked
 */
	{
	ASSERT(false);
	}

