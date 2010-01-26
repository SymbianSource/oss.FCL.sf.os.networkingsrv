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
// SSL3.0 and TLS1.0 Connection source file.
// Describes the implementation of a secure (SSL/TLS) connection.
// 
//

/**
 @file
*/
  
#include "tlsconnection.h"
#include "recordprotocolevents.h"
#include "tlshandshake.h"
#include "applicationdata.h"
#include <es_sock.h>
#include <in_sock.h>

#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <ssl_internal.h>
#endif


EXPORT_C MSecureSocket* CTlsConnection::NewL(RSocket& aSocket, const TDesC& aProtocol)
/**
 * Creates and initialises a new CTlsConnection object.
 * 
 * @param aSocket is a reference to an already open and connected socket.
 * @param aProtocol is a descriptor containing the name of the protocol (SSL3.0 or 
 * TLS1.0) the application specified when the secure socket was created.
 * @return A pointer to a newly created Secure socket object.
 */
{
	LOG(Log::Printf(_L("CTlsConnection::NewL(RSocket,Protocol)"));)
	CTlsConnection* self = new(ELeave) CTlsConnection();
  	LOG(Log::Printf(_L("self %x - %x"), self, (TUint)self + sizeof( CTlsConnection ));)

#ifdef _DEBUG
   TInt nBlock;
  	LOG(Log::Printf(_L("RHeap::Size(), RHeap::Size() - RHeap::Available() %d, %d"), User::Heap().Size(), User::Heap().Size() - User::Heap().Available( nBlock ) );)
#endif

	CleanupStack::PushL(self);
	self->ConstructL(aSocket, aProtocol);
	CleanupStack::Pop();
	return self;
}

EXPORT_C MSecureSocket* CTlsConnection::NewL(MGenericSecureSocket& aSocket, const TDesC& aProtocol)
/**
 * Creates and initialises a new CTlsConnection object.
 * 
 * @param aSocket is a reference to socket like object derived from MGenericSecureSocket.
 * @param aProtocol is a descriptor containing the name of the protocol (SSL3.0 or 
 * TLS1.0) the application specified when the secure socket was created.
 * @return A pointer to a newly created Secure socket object.
 */
{
	LOG(Log::Printf(_L("CTlsConnection::NewL(GenericSocket,Protocol)"));)
	CTlsConnection* self = new(ELeave) CTlsConnection();
  	LOG(Log::Printf(_L("self %x - %x"), self, (TUint)self + sizeof( CTlsConnection ));)

#ifdef _DEBUG
   TInt nBlock;
  	LOG(Log::Printf(_L("RHeap::Size(), RHeap::Size() - RHeap::Available() %d, %d"), User::Heap().Size(), User::Heap().Size() - User::Heap().Available( nBlock ) );)
#endif

	CleanupStack::PushL(self);
	self->ConstructL(aSocket, aProtocol);
	CleanupStack::Pop();
	return self;
}

EXPORT_C void CTlsConnection::UnloadDll(TAny* /*aPtr*/)
/**
 Function called prior to unloading DLL.  
 Does nothing in this implementation but is needed to be exported.
 */
{
	LOG(Log::Printf(_L("CTlsConnection::UnloadDll()"));)
}

CTlsConnection::~CTlsConnection()
/**
 * Destructor.
 * The user should ensure that the connection has been closed before destruction,
 * as there is no check for any pending asynch event here (apart from the panic 
 * in ~CActive).
 */
{
	LOG(Log::Printf(_L("CTlsConnection::~CTlsConnection()"));)

#ifdef _DEBUG
   TInt nBlock;
  	LOG(Log::Printf(_L("RHeap::Size(), RHeap::Size() - RHeap::Available() %d, %d"), User::Heap().Size(), User::Heap().Size() - User::Heap().Available( nBlock ) );)
#endif
	DeleteStateMachines();

	delete iRecordParser; //don't change the order of the deletion (see ~CRecordParser & CRecordParser:;Reset)
	delete iRecordComposer;
	delete iGenericSocket;
	delete iClientCert;
	delete iServerCert;
#ifdef _DEBUG
  	LOG(Log::Printf(_L("RHeap::Size(), RHeap::Size() - RHeap::Available() %d, %d"), User::Heap().Size(), User::Heap().Size() - User::Heap().Available( nBlock ) );)
#endif
	delete iTlsProvider;
	delete iTlsSession; 
#ifdef _DEBUG
  	LOG(Log::Printf(_L("RHeap::Size(), RHeap::Size() - RHeap::Available() %d, %d"), User::Heap().Size(), User::Heap().Size() - User::Heap().Available( nBlock ) );)
#endif
}

CTlsConnection::CTlsConnection() : CActive( EPriorityHigh )
/**
 * Constructor .
 * Sets the Active object priority.
 */
{
}

void CTlsConnection::ConstructL(RSocket& aSocket, const TDesC& aProtocol)
/** 
 * Two-phase constructor.
 * Called by CTlsConnection::NewL() to initialise all the 
 * CTlsConnection objects (bar the State machines). It also sets the 
 * protocol for the connection. The Provider interface is created and the Session
 * interface pointer is set to NULL (as no session currently exists).
 * The dialog mode for the connection is set to Attended mode (default) and the current 
 * cipher suite is set to [0x00],[0x00].
 *
 * @param aSocket is a reference to an already open and connected socket.
 * @param aProtocol is a descriptor containing the name of the protocol (SSL3.0 or 
 * TLS1.0) the application specified when the secure socket was created.
 */
{
	LOG(Log::Printf(_L("CTlsConnection::ConstructL(RSocket,Protocol)"));)

	CActiveScheduler::Add(this);		

	iTlsProvider = CTLSProvider::ConnectL();		// Set up Security/crypto interfaces

	User::LeaveIfError( SetProtocol(aProtocol) );
	iTlsProvider->Attributes()->iCurrentCipherSuite.iLoByte = 0x00;
	iTlsProvider->Attributes()->iCurrentCipherSuite.iHiByte = 0x00;
	iTlsProvider->Attributes()->iDialogNonAttendedMode = EFalse;
	iDialogMode = EDialogModeAttended;

	iGenericSocket = new(ELeave)CGenericSecureSocket<RSocket>(aSocket);

	iRecordParser = new(ELeave)CRecordParser( *iGenericSocket, *iTlsProvider );
  	LOG(Log::Printf(_L("iRecordParser %x - %x"), iRecordParser, (TUint)iRecordParser + sizeof( CRecordParser ));)
	iRecordComposer = new(ELeave)CRecordComposer( *iGenericSocket, *iTlsProvider );
  	LOG(Log::Printf(_L("iRecordComposer %x - %x"), iRecordComposer, (TUint)iRecordComposer + sizeof( CRecordComposer ));)

#ifdef _DEBUG
   TInt nBlock;
  	LOG(Log::Printf(_L("RHeap::Size(), RHeap::Size() - RHeap::Available() %d, %d"), User::Heap().Size(), User::Heap().Size() - User::Heap().Available( nBlock ) );)
#endif
}

void CTlsConnection::ConstructL(MGenericSecureSocket& aSocket, const TDesC& aProtocol)
/** 
 * Two-phase constructor.
 * Called by CTlsConnection::NewL() to initialise all the 
 * CTlsConnection objects (bar the State machines). It also sets the 
 * protocol for the connection. The Provider interface is created and the Session
 * interface pointer is set to NULL (as no session currently exists).
 * The dialog mode for the connection is set to Attended mode (default) and the current 
 * cipher suite is set to [0x00],[0x00].
 *
 * @param aSocket is a reference to socket like object derived from MGenericSecureSocket.
 * @param aProtocol is a descriptor containing the name of the protocol (SSL3.0 or 
 * TLS1.0) the application specified when the secure socket was created.
 */
{
	LOG(Log::Printf(_L("CTlsConnection::ConstructL(GenericSocket,Protocol)"));)

	CActiveScheduler::Add(this);		

	iTlsProvider = CTLSProvider::ConnectL();		// Set up Security/crypto interfaces

	User::LeaveIfError( SetProtocol(aProtocol) );
	iTlsProvider->Attributes()->iCurrentCipherSuite.iLoByte = 0x00;
	iTlsProvider->Attributes()->iCurrentCipherSuite.iHiByte = 0x00;
	iTlsProvider->Attributes()->iDialogNonAttendedMode = EFalse;
	iDialogMode = EDialogModeAttended;

	iRecordParser = new(ELeave)CRecordParser( aSocket, *iTlsProvider );
  	LOG(Log::Printf(_L("iRecordParser %x - %x"), iRecordParser, (TUint)iRecordParser + sizeof( CRecordParser ));)
	iRecordComposer = new(ELeave)CRecordComposer( aSocket, *iTlsProvider );
  	LOG(Log::Printf(_L("iRecordComposer %x - %x"), iRecordComposer, (TUint)iRecordComposer + sizeof( CRecordComposer ));)
#ifdef _DEBUG
   TInt nBlock;
  	LOG(Log::Printf(_L("RHeap::Size(), RHeap::Size() - RHeap::Available() %d, %d"), User::Heap().Size(), User::Heap().Size() - User::Heap().Available( nBlock ) );)
#endif
}

void CTlsConnection::RunL()
{
	LOG(Log::Printf(_L("CTlsConnection::RunL()"));)
#ifdef _DEBUG
   TInt nBlock;
  	LOG(Log::Printf(_L("RHeap::Size(), RHeap::Size() - RHeap::Available() %d, %d"), User::Heap().Size(), User::Heap().Size() - User::Heap().Available( nBlock ) );)
#endif

	CActiveScheduler::Stop();
}

void CTlsConnection::DoCancel()
{
}


// MSecureSocket interface
TInt CTlsConnection::AvailableCipherSuites( TDes8& aCiphers )
/** 
 * Retrieves the list of cipher suites that are available to use
 * for handshake negotiation. 
 * Cipher suites are returned in two byte format as is specified in the SSL/TLS 
 * specifications, e.g. [0x00][0x03]. 
 *
 * @param aCiphers A reference to a descriptor which will contain a list of cipher suites. 
 * @return Any one of the system error codes, or KErrNone on success. 
 */
{
	LOG(Log::Printf(_L("CTlsConnection::AvailableCipherSuites()"));)

   if ( !iTlsProvider )
      {
      return KErrNotReady;
      }
	RArray<TTLSCipherSuite> cipherList;
	TRAPD(ret,iTlsProvider->CipherSuitesL(cipherList, iStatus));
	if ( ret != KErrNone )
		return ret;
		
	SetActive();
	CActiveScheduler::Start();

	if ( iStatus.Int() != KErrNone )
	{
		LOG(Log::Printf(_L("Error retrieving the available cipher suites %d"), iStatus.Int() );)
		return iStatus.Int();
	}
	
	// Each cipher suite contains 2 TUint8 elements. Check the length of the user's 
	// descriptor, copy the available cipher suites into it and close the array.
	if ( aCiphers.MaxLength() < (cipherList.Count() * 2) ) 
		return KErrOverflow;

	for (TInt loop = 0; loop < cipherList.Count(); ++loop)
	{
		aCiphers.Append( cipherList[loop].iHiByte );
		aCiphers.Append( cipherList[loop].iLoByte );
	}
	
	cipherList.Close();
	return KErrNone;
}

void CTlsConnection::CancelAll()
/**
 * Cancels all outstanding operations. 
 */
{
	LOG(Log::Printf(_L("CTlsConnection::CancelAll()"));)
	CancelAll( KErrNone ); 
}

void CTlsConnection::CancelHandshake()
/**
 * Cancels an outstanding handshake operation. It is equivalent to
 * a CancelAll() call.
 */
{
	LOG(Log::Printf(_L("CTlsConnection::CancelHandshake()"));)
	CancelAll( KErrNone );
}

void CTlsConnection::CancelRecv()
/** 
 * Cancels any outstanding read data operation.
 */
{
	LOG(Log::Printf(_L("CTlsConnection::CancelRecv()"));)
	if ( iRecvAppData )
		iRecvAppData->Cancel( KErrNone ); //no alert sent ? maybe we should send user abort
		//but then it'll became be asynchronous fn	// @todo - Cancels have not been tested
}

void CTlsConnection::CancelSend()
/** 
 * Cancels any outstanding send data operation.
 */
{
	LOG(Log::Printf(_L("CTlsConnection::CancelSend()"));)
	if ( iSendAppData )
		iSendAppData->Cancel( KErrNone );  //no alert sent ? maybe we should send user abort
		//but then it'll became be asynchronous fn	// @todo
}

const CX509Certificate* CTlsConnection::ClientCert()
/**
 * Returns a pointer to the current client certificate if a Server has
 * requested one. If there is no suitable client certificate available, a NULL pointer 
 * will be returned.
 * A client certificate (if available) can only be returned after the negotiation
 * is complete.
 *
 * @return A pointer to the client certificate, or NULL if none exists or is yet
 * available.
 */
{
	LOG(Log::Printf(_L("CTlsConnection::ClientCert()"));)

	if (!iTlsProvider || !iTlsProvider->TlsSessionPtr())
	{
		LOG(Log::Printf(_L("The Client certificate is not yet available()"));)
		return NULL;
	}
	else
	{
      if ( !iClientCert )
         {
		   iTlsProvider->TlsSessionPtr()->ClientCertificate(iClientCert, iStatus);
	   
		   SetActive();
   		CActiveScheduler::Start();		
         }
		return iClientCert;
	}
}

TClientCertMode CTlsConnection::ClientCertMode()
/** 
 * Returns the current client certificate mode. This is used when the 
 * socket is acting as a server, and determines if a client certificate is requested.
 * This method is not supported as this implementation only acts in Client mode.
 *
 * The closest value that the TClientCertMode enumeration provides that supports this
 * is EClientCertModeIgnore.
 */
{
	LOG(Log::Printf(_L("CTlsConnection::ClientCertMode() - Not Supported"));)
	return EClientCertModeIgnore;
}

void CTlsConnection::Close()
/** 
 * Closes the secure connection.
 * All outstanding operations are cancelled, the internal state machines are deleted
 * and the socket is closed.
 */
{
	LOG(Log::Printf(_L("CTlsConnection::Close()"));)
	//OK cancel with closure alert sent (=> asynch call) and delete the statemachines
	CancelAll( KErrNone /*KErrSSLAlertCloseNotify*/ ); //it's possible to make it asynch
   //(send an alert) but....
   DeleteStateMachines();
   iRecordParser->Socket().Close();
   iRecordComposer->SetVersion( NULL );

   ResetCryptoAttributes();
}

TInt CTlsConnection::CurrentCipherSuite( TDes8& aCipherSuite )
/**
 * Retrieves the current cipher suite in use. 
 * Cipher suites are returned in two byte format as is specified in the SSL/TLS 
 * specifications, i.e. [0x??][0x??]. 
 *
 * This method can only return the current cipher suite when the Server has proposed one
 * to use (i.e., anytime after the Server Hello has been received). Hence, it will only 
 * have a valid value after the Handshake negotiation has completed. If called before 
 * handshake negotiation, it will have the value of the NULL cipher, [0x00][0x00].
 *
 * @param aCipherSuite A reference to a descriptor at least 2 bytes long.
 * @return Any one of the system error codes, or KErrNone on success. 
 */
{
	LOG(Log::Printf(_L("CTlsConnection::CurrentCipherSuite()"));)
	
   if ( !iTlsProvider )
      {
      return KErrNotReady;
      }
	if ( aCipherSuite.MaxLength() < 2 )
	{
		LOG(Log::Printf(_L("CurrentCipherSuite() - Descriptor should be at least 2 bytes long"));)
		return KErrOverflow;
	}

	aCipherSuite.SetLength(2);		// A cipher suite has a 2 byte length.
	aCipherSuite[0] = iTlsProvider->Attributes()->iCurrentCipherSuite.iHiByte;
	aCipherSuite[1] = iTlsProvider->Attributes()->iCurrentCipherSuite.iLoByte;

	return KErrNone;
}

TDialogMode	CTlsConnection::DialogMode()
/**
 * Returns the current dialog mode.
 *
 * @return The current dialog mode.
 */ 
{
	LOG(Log::Printf(_L("CTlsConnection::DialogMode()"));)
	return iDialogMode;
}

void CTlsConnection::FlushSessionCache()
/** 
 * This method does NOT flush the session cache (as this is device-wide). As such, its
 * interpretation has changed from the pre-Zephyr TLS implementation.
 *
 * It is now used as an indication that the client does not intend to reuse an existing
 * session. As such it sets a flag which is called during handshake negotiation which 
 * indicates whether a new session or existing session will be used.
 * The other choice for implementation of this method will be:
 * 1) Call TLS Provider's GetSession() API to retrieve the session information.
 * 2) Call TLS Provider's ClearSessionCache() API (with the retrieved session information) 
 * to remove the particular session from the session cache. Both these APIs are asynchronous.
 *
 * Note that there is no means of indicating the success or failure of this operation
 * to the client.
 */
{
	LOG(Log::Printf(_L("CTlsConnection::FlushSessionCache()"));)
   if ( iTlsProvider )
      {
      TTLSSessionNameAndID sessionNameAndID;
      GetServerAddrInfo( sessionNameAndID.iServerName );
      TRAPD(ret,iTlsProvider->ClearSessionCacheL( sessionNameAndID, iStatus ));
      if (KErrNone!=ret)
      	{
      	LOG(Log::Printf(_L("CTlsConnection: ClearSessionCacheL error: %d"),ret));
      	return;
      	}
	  SetActive();
	  CActiveScheduler::Start();
      }
}

TInt CTlsConnection::GetOpt(TUint aOptionName,TUint aOptionLevel,TDes8& aOption)
/**
 * Gets a Socket option. 
 *
 * @param aOptionName An unsigned integer constant which identifies an option.
 * @param aOptionLevel An unsigned integer constant which identifies the level of an option.	 
 * @param aOption Option value packaged in a descriptor.
 * @return KErrNone if successful, otherwise another of the system-wide error codes.
 */
{
	LOG(Log::Printf(_L("CTlsConnection::GetOpt() method - descriptor Option"));)
	if ( !iTlsProvider || !iTlsProvider->Attributes())
    	{
    	return KErrNotReady;
    	}

	switch(aOptionLevel)
	{
	case KSolInetSSL:
		{
			switch (aOptionName)
			{
				case KSoCurrentCipherSuite:
					{
					// Call the API method that implements the functionality.
					LOG(Log::Printf(_L("Option name: KSoCurrentCipherSuite")));
					return CurrentCipherSuite(aOption);
					}
				case KSoAvailableCipherSuites:
					{
					// Call the API method that implements the functionality.
					LOG(Log::Printf(_L("Option name: KSoAvailableCipherSuites")));
					return AvailableCipherSuites(aOption);
					}
				case KSoDialogMode:
					{
					// Call the API method that implements the functionality.
					LOG(Log::Printf(_L("Option name: KSoDialogMode")));
					
					TDialogMode mode = DialogMode();
					TPckgBuf<TDialogMode> packedMode(mode); // Package the object into a descriptor
					
					if ( aOption.MaxLength() < packedMode.Length() )
						return KErrOverflow;
						
					aOption.Copy(packedMode); 
					return KErrNone;
					}
				case KSoSSLServerCert:
					{
						// Call the API method that implements the functionality.
						LOG(Log::Printf(_L("Option name: KSoSSLServerCert")));
						
						const CX509Certificate* cert = ServerCert();			
						TPckgBuf<const CX509Certificate*> packedCert(cert); // Package the pointer into a descriptor
						
						if ( aOption.MaxLength() < packedCert.Length() )
							return KErrOverflow;
						
						aOption.Copy( packedCert );
						return KErrNone;
					}
				case KSoUseSSLv2Handshake:
					{
					/*	
					This option is no longer supported, but returning KErrNotSupported
					or any other error code will result in a BC break. 
					Hence we return KErrNone untill the break gets approved by SCB
					*/
					return KErrNone;
					}
				case KSoKeyingMaterial:
					{
					/*
					Performs key generation as per RFC2716 
					(PPP EAP TLS Authentication Protocol) section 3.5
					*/
					return GetKeyingMaterial(aOption);
					}
				case KSoEnableNullCiphers:
					{
					*((TInt *)aOption.Ptr()) = iTlsProvider->Attributes()->iAllowNullCipherSuites;
					return KErrNone;
					}				
				
				case KSoPskConfig:
					{
					CTlsCryptoAttributes *attrs = iTlsProvider->Attributes();
					MSoPskKeyHandler *handler = NULL;
					if(attrs->iPskConfigured)
						{
						handler = attrs->iPskKeyHandler;
						}
					if(aOption.Length() < sizeof(MSoPskKeyHandler *))
						{
						return KErrArgument;
						}
					// aOption must be a descriptor wrapped arround a MSoPskKeyHandler pointer
					// For example TPckgBuf<MSoPskKeyHandler *> pskConfigPkg
					*((MSoPskKeyHandler **)aOption.Ptr()) = handler;
					return KErrNone;
					}
				default:
					{
						LOG(Log::Printf(_L("Unknown option name passed in.")));
						return KErrNotSupported;
					}
			}	// switch (name)
		}	// KSolInetSSL
	default:
		{
			// Call the RSocket options directly
			return iRecordComposer->Socket().GetOpt(aOptionName, aOptionLevel, aOption);
		}
	}	// switch (aOptionLevel)
}

TInt CTlsConnection::GetOpt(TUint aOptionName,TUint aOptionLevel,TInt& aOption)
/**
 * Gets a Socket option. 
 *
 * @param aOptionName An integer constant which identifies an option.
 * @param aOptionLevel An integer constant which identifies the level of an option.	 
 * @param aOption Option value as an integer.
 * @return KErrNone if successful, otherwise another of the system-wide error codes.
 */
{ 
	LOG(Log::Printf(_L("CTlsConnection::GetOpt() method - integer Option"));)

	TPtr8 optionDes( (TUint8*)&aOption, sizeof(TInt), sizeof(TInt) );
	return GetOpt(aOptionName, aOptionLevel, optionDes);
}

TInt CTlsConnection::Protocol(TDes& aProtocol)
/**
 * Returns the Protocol version in use. A minimum descriptor size of 8 is
 * defined for the protocol name (a maximum of 32 is specified in the Secure Socket interface).
 *
 * This method can only return the agreed/negotiated Protocol anytime when the handshake 
 * negotiation has completed. If called before this, the value returned is the protocol
 * version proposed by the user.
 *
 * @param aProtocol A reference to a descriptor containing the protocol name in use.
 * @return An Integer value indicating the outcome of the function call.
 */
{
	LOG(Log::Printf(_L("CTlsConnection::Protocol()"));)
   if ( !iTlsProvider )
      {
      return KErrNotReady;
      }

	// Ensure that the descriptor size passed in reaches our minimum requirement of KProtocolDescMinSize (i.e., 8)
	if (aProtocol.MaxSize() < KProtocolDescMinSize) 
		return KErrOverflow;

	TInt ret = KErrNone;
	
   CTlsCryptoAttributes& cryptoAttributes = *iTlsProvider->Attributes();
   //check whether any protocol's been negotiated yet. if no return the proposed protocol
   const TTLSProtocolVersion& tlsVersion = cryptoAttributes.iNegotiatedProtocol.iMajor ?
      cryptoAttributes.iNegotiatedProtocol : cryptoAttributes.iProposedProtocol;
	if (tlsVersion == KSSL3_0)
	{
		aProtocol.Copy(KProtocolVerSSL30);	// SSL 3.0
	}
	else if (tlsVersion == KTLS1_0)
	{
		aProtocol.Copy(KProtocolVerTLS10);	// TLS 1.0
	}
	else
	{
		ret = KErrGeneral;					// Unknown protocol version
		LOG(Log::Printf(_L("CTlsConnection::Protocol() Unknown protocol error %d"), ret );)
	}

	return ret;
}

void CTlsConnection::Recv(TDes8& aDesc, TRequestStatus & aStatus)
/**
 * Receives data from the socket. 
 * It is an asynchronous method, and will complete when the descriptor has been filled. 
 * Only one Recv or RecvOneOrMore operation can be outstanding at any time. 
 * 
 * @param aDesc A descriptor where data read will be placed.
 * @param aStatus On completion, will contain an error code: see the system-wide error 
 * codes. Note that KErrEof indicates that a remote connection is closed, and that no 
 * more data is available for reading.
 */
{
	LOG(Log::Printf(_L("CTlsConnection::Recv()"));)

	aDesc.Zero();
	if ( RecvData( aDesc, aStatus ) )
		iRecvAppData->SetSockXfrLength( NULL );
}

void CTlsConnection::RecvOneOrMore(TDes8& aDesc, TRequestStatus& aStatus, TSockXfrLength& aLen)
/** 
 * Receives data from the socket. 
 * It is an asynchronous call, and will complete when at least one byte has been read.
 * Only one Recv or RecvOneOrMore operation can be outstanding at any time. 
 *
 * @param aDesc A descriptor where data read will be placed.
 * @param aStatus On completion, will contain an error code: see the system-wide error 
 * codes. Note that KErrEof indicates that a remote connection is closed, and that no 
 * more data is available for reading.
 * @param aLen On return, a length which indicates how much data was read. This is
 * the same as the length of the returned aDesc.
 */
{
	LOG(Log::Printf(_L("CTlsConnection::RecvOneOrMore()"));)

	if ( RecvData( aDesc, aStatus ) )
		iRecvAppData->SetSockXfrLength( &aLen() );
}

void CTlsConnection::RenegotiateHandshake(TRequestStatus& aStatus)
/**
 * Initiates a renegotiation of the secure connection. 
 * It is an asynchronous method that completes when renegotiation is complete. 
 * The Client can initiate handshake renegotiation or it can receive a re-negotiation request 
 * from a remote server.
 * Note that the User should cancel any data transfer or wait for its completion before
 * attempting to re-negotiate.
 *
 * @param aStatus On completion, will contain an error code: see the system-wide error 
 * codes.
 */
{
	LOG(Log::Printf(_L("CTlsConnection::RenegotiateHandshake()"));)
   
	TRequestStatus* pStatus = &aStatus;

	// Renegotiation can only happen in data mode
	if ( !IsInDataMode() )		
	{
		User::RequestComplete( pStatus, KErrNotReady );
		return;
	}
   
	// Renegotiation is already taking place or client is tx-ing or rx-ing data
	if ( IsReNegotiating() || iSendAppData->ClientStatus() || iRecvAppData->ClientStatus() )
	{
		User::RequestComplete( pStatus, KErrInUse );
		return;
	}
   
	StartRenegotiation( pStatus );
}

void CTlsConnection::Send(const TDesC8& aDesc, TRequestStatus& aStatus)
/** 
 * Sends data over the socket. 
 * Only one Send operation can be outstanding at any time.
 *
 * @param aDesc A constant descriptor containing the data to be sent.
 * @param aStatus On completion, will contain an error code: see the system-wide 
 * error codes. 
 */
{
	LOG(Log::Printf(_L("CTlsConnection::Send() - Descriptor only"));)

	if ( SendData( aDesc, aStatus ) )
		iSendAppData->SetSockXfrLength( NULL );
}

void CTlsConnection::Send(const TDesC8& aDesc, TRequestStatus& aStatus, TSockXfrLength& aLen)
/** 
 * Sends data over the socket. 
 * Only one Send operation can be outstanding at any time. 
 *
 * @param aDesc A constant descriptor.
 * @param aStatus On completion, will contain an error code: see the system-wide 
 * error codes. 
 * @param aLen Filled in with amount of data sent before completion 
 */
{
	LOG(Log::Printf(_L("CTlsConnection::Send() - Descriptor and Length"));)

	if ( SendData( aDesc, aStatus ) )
		iSendAppData->SetSockXfrLength( &aLen() );
}

const CX509Certificate* CTlsConnection::ServerCert()
/**
 * Returns a pointer to the current server certificate.
 * The returned certificate will be the certificate for the remote server. It is 
 * obtained via the TLS Provider API.
 *
 * A server certificate (if available) can only be returned only after the 
 * negotiation has reached a stage at which one has been received and verified.
 *
 * @return A pointer to the Server's certificate.
 */ 
{
	LOG(Log::Printf(_L("CTlsConnection::ServerCert()"));)

	if ( !iTlsProvider || !iTlsProvider->TlsSessionPtr())
	{
		LOG(Log::Printf(_L("The Server certificate is not yet available()"));)
		return NULL;
	}
	else
	{
		if ( !iServerCert )
         {
		   iTlsProvider->TlsSessionPtr()->ServerCertificate(iServerCert, iStatus);  
		   
		   SetActive();
		   CActiveScheduler::Start();

		   if ( iStatus.Int() != KErrNone )
		      {
			   LOG(Log::Printf(_L("Error retrieving the Server certificate %d"), iStatus.Int() );)
		      }
         }
		return iServerCert;
	}
}

TInt CTlsConnection::SetAvailableCipherSuites(const TDesC8& aCiphers)
/** 
 * A client can be involved in the Handshake negotiation with the remote server by 
 * specifying which cipher suites it wants to use in the negotiation. 
 * The client should first call AvailableCipherSuites() to retrieve all the supported
 * cipher suites. This method can then be used to specify a subset which it wants to
 * use.
 * The list of cipher suites supplied in a descriptor to the protocol MUST be in two
 * byte format, i.e. [0x??][0x??]. The order of suites is important, and so they should 
 * be listed with the preferred suites first. 
 * A client does NOT have to call/use this method. In this instance, the preference
 * order of the cipher suites will be set by the TLS Provider.
 * 
 * @param aCiphers A descriptor containing the list of ciphers suites to use. 
 * @return Any one of the system error codes, or KErrNone on success. 
 */
{
	LOG(Log::Printf(_L("CTlsConnection::SetAvailableCipherSuites()"));)
	if ( !iTlsProvider )
      {
      return KErrNotReady;
      }
	// Get the available cipher suites from the Provider. 
	RArray<TTLSCipherSuite> cipherList; 
	TRAPD(ret,iTlsProvider->CipherSuitesL(cipherList, iStatus));
	if ( ret != KErrNone )
		return ret;
		
	SetActive();
//   TRequestStatus* p=&iStatus;
//   User::RequestComplete( p, KErrNone );
	CActiveScheduler::Start();

	// Cycle through the client's list of ciphers. 
	// Ensure that values in the clients list are in the list of available ciphers.
	TBool valueIsSet;		// Break out of the inner loop once the value is set.
   TInt returnValue = iStatus.Int();
   TDes8& proposedCiphers = iTlsProvider->Attributes()->iProposedCiphers;
   proposedCiphers.Zero();
	for ( TInt outerLoop=0; outerLoop<aCiphers.Length(); outerLoop+=2 )					
	{
		valueIsSet = EFalse;

		for (TInt innerLoop=0; innerLoop<cipherList.Count();++innerLoop)
		{
			if( aCiphers[outerLoop] == cipherList[innerLoop].iHiByte			
			&& aCiphers[outerLoop+1] == cipherList[innerLoop].iLoByte )
			{
				// the suite is valid, so add it to the list of proposed ciphers
				proposedCiphers.Append(aCiphers[outerLoop]);
				proposedCiphers.Append(aCiphers[outerLoop+1]);
				valueIsSet = 1;
			}

			if (valueIsSet)
				break;
		} // inner 'for' statement.
	}	// outer 'for' statement.
	
	cipherList.Close();	// Close the array and free its memory.

	// If no valid ciphers are received from the client, all available cipher suites
	// returned by the protocol will be used
	if ( proposedCiphers.Length() == 0 )
	{
		LOG(Log::Printf(_L("SetAvailableCipherSuites() - No valid ciphers received from client"));)
		returnValue = KErrNotSupported;
	}

	return returnValue;
}

TInt CTlsConnection::SetClientCert(const CX509Certificate& /*aCert*/)
/**
 * Sets the client certificate to use.
 * In client mode, this method will set the certificate that will be used if a 
 * server requests one.
 * Note that this method is NOT supported by the current implementation. Client 
 * Certificates are stored by the Security subsystem and it chooses the appropriate
 * Client certificate to use based on the Server's preference list.
 *
 * @param aCert A reference to the certificate to use.
 * @return Any one of the system error codes, or KErrNone on success. 
 */
{
	LOG(Log::Printf(_L("CTlsConnection::SetClientCert()"));)
	return KErrNotSupported;
}

TInt CTlsConnection::SetClientCertMode(const TClientCertMode /*aClientCertMode*/)
/** 
 * Sets the client certificate mode. 
 * This method only applies to Server mode operation (which is not supported by the 
 * current implementation). In client mode, no action will be performed and 
 * KErrNotSupported will be returned by the Protocol.
 *
 * @param aClientCertMode The client certificate mode to use.
 * @return Any one of the system error codes, or KErrNone on success. 
 */
{
	LOG(Log::Printf(_L("CTlsConnection::SetClientCertMode()"));)
	return KErrNotSupported;
}

TInt CTlsConnection::SetDialogMode(const TDialogMode aDialogMode)
/**
 * Sets the untrusted certificate dialog mode.
 * It determines if a dialog is displayed when an untrusted certificate is received.
 * The default behaviour is for the dialog to be set to EDialogModeAttended (this 
 * is set in the construction of a CTlsConnection object).
 * A client can either set the dialog mode directly by calling this method, or by
 * calling CTlsConnection::SetOpt() with an appropriate option value.
 *
 * @param aDialogMode The dialog mode to use.
 * @return Any one of the system error codes, or KErrNone on success. 
 */
{
	LOG(Log::Printf(_L("CTlsConnection::SetDialogMode()"));)
	
	// This method must ensure that the dialog mode passed in is part of the 
	// TDialogMode enum or has the value EDialogModeUnattended/EDialogModeAttended. 
	// Otherwise, it must return KErrArgument
	TInt ret = KErrNone;
   
    switch(aDialogMode)
    {
        case EDialogModeUnattended:
        case EDialogModeAttended:
            iDialogMode = aDialogMode;
        break;
        
        default:  //-- wrong mode
            LOG(Log::Printf(_L("SetDialogMode() - Unknown dialog mode, default setting (Attended mode) being used"));)
        return KErrArgument;    
    };

    if ( iTlsProvider )
    {
   	    iTlsProvider->Attributes()->iDialogNonAttendedMode = (iDialogMode == EDialogModeUnattended);
    }	

    return ret;
}

TInt CTlsConnection::SetOpt(TUint aOptionName,TUint aOptionLevel, const TDesC8& aOption)
/** 
 * Sets a Socket option. 
 *
 * @param aOption Option value packaged in a descriptor.
 * @param aOptionName An integer constant which identifies an option.
 * @param aOptionLevel An integer constant which identifies the level of an option 
 * (an option level groups related options together).
 * @return Any one of the system error codes, or KErrNone on success.
 */
{
	LOG(Log::Printf(_L("CTlsConnection::SetOpt() method - descriptor Option"));)
	TInt ret=KErrNotSupported;

	if ( !iTlsProvider || !iTlsProvider->Attributes())
    	{
    	return KErrNotReady;
    	}

	switch(aOptionLevel)
	{
	case KSolInetSSL:		// This is the only supported option level in SSL/TLS
		{
			switch (aOptionName)
			{
			case KSoSSLDomainName:		
				{
				LOG(Log::Printf(_L("Option name: KSoSSLDomainName")));
				iTlsProvider->Attributes()->idomainName.Copy(aOption);
				// @todo Create a CX509Name object and set in the TTlsCryptoAttribs
				// structure. Note that this structure has to updated to take a 
				// CX509**** object. The iProposedCiphers buffer is still not correct.
				// It must also have a iProposedProtocol and iNegotiatedProtocol.
				ret = KErrNone;
				break;
				}
			case KSoDialogMode:
				{
				// Call the API method that implements the functionality.
				LOG(Log::Printf(_L("Option name: KSoDialogMode")));
				
				TDialogMode dialogMode = (TDialogMode) ( *(TUint*)aOption.Ptr() );
				ret = SetDialogMode(dialogMode);
				
				break;
				}
			case KSoUseSSLv2Handshake:
				{
				/*	
				This option is no longer supported, but returning KErrNotSupported
				or any other error code will result in a BC break. 
				Hence we return KErrNone untill the break gets approved by SCB
				*/
				ret = KErrNone;
				break;
				}
			case KSoEnableNullCiphers:
				{
				TInt option = *reinterpret_cast<const TInt *>(aOption.Ptr());
				iTlsProvider->Attributes()->iAllowNullCipherSuites = (option != 0);
				ret = KErrNone;
				break;
				}
			case KSoPskConfig:
				{
				/*
					Set the PSK Key Exchange configuration.
					aOption is a TDesC8 wrapper around a MSoPskKeyHandler pointer
				*/
				// aOption must be a descriptor wrapped arround a MSoPskKeyHandler pointer
				// For example TPckgBuf<MSoPskKeyHandler *> pskConfigPkg
				if(aOption.Length() < sizeof(MSoPskKeyHandler *))
					{
					return KErrArgument;
					}
				TPckgBuf<MSoPskKeyHandler *> pskConfigPkg;
				MSoPskKeyHandler *handler = *reinterpret_cast<MSoPskKeyHandler * const *>(aOption.Ptr());

				CTlsCryptoAttributes *attrs = iTlsProvider->Attributes();
				attrs->iPskConfigured = (handler!=0);
				attrs->iPskKeyHandler = handler;
				ret = KErrNone;
				break;
				}
			case KSoServerNameIndication:
				{
				/*
				 * Set the list of server names to be passed to the server in the ClientHello as described in 
				 * RFC3546 "Server Name Indication".
 				 * aOption is a TDesC8 wrapper around a CDesC8Array pointer.
				*/
				if(aOption.Length() < sizeof(CDesC8Array *))
					{
					return KErrArgument;
					}
				CDesC8Array *serverNames = *reinterpret_cast<CDesC8Array * const *>(aOption.Ptr());

				CTlsCryptoAttributes *attrs = iTlsProvider->Attributes();
				delete attrs->iServerNames;
				attrs->iServerNames = serverNames;

				ret = KErrNone;
				break;
				}
			default:
				break;
			}	// KSolInetSSL

			break;
		}
	default:	// Not a supported SSL option, call RSocket::SetOpt directly
		{
			LOG(Log::Printf(_L("Default option level (not supported by protocol)")));
			ret = iRecordComposer->Socket().SetOpt(aOptionName, aOptionLevel, aOption);

			break;
		}	// Default 'level' statement
	}	// switch statement

	return ret;
}

TInt CTlsConnection::SetOpt(TUint aOptionName,TUint aOptionLevel,TInt aOption)
/** 
 * Sets a Socket option.  calls the SetOpt() method defined above.
 *
 * @param aOption Option value as an integer
 * @param aOptionName An integer constant which identifies an option.
 * @param aOptionLevel An integer constant which identifies level of an option (an
 * option level groups related options together.
 * @return Any one of the system error codes, or KErrNone on success.
 */
{
	LOG(Log::Printf(_L("CTlsConnection::SetOpt() method - integer Option"));)

	TPtr8 optionDes( (TUint8*)&aOption, sizeof(TInt), sizeof(TInt) );
	return SetOpt(aOptionName, aOptionLevel, optionDes);	
}

TInt CTlsConnection::SetProtocol(const TDesC& aProtocol)
/**
 * Sets the Secure socket protocol version (SSL v3.0 or TLS v1.0) to 
 * use in the Handshake negotiation. It also initially sets the negotiated protocol 
 * to the requested protocol. A maximum length of 32 is specified in the Secure Socket 
 * interface for the protocol version.
 *
 * 
 * @param aProtocol is a reference to a descriptor containing the protocol version to use.
 * @return Any one of the system error codes, or KErrNone on success.
 */
{
	LOG(Log::Printf(_L("CTlsConnection::SetProtocol()"));)

   if ( !iTlsProvider )
      {
      return KErrNotReady;
      }
	// Convert the Protocol value to upper case before doing a comparison
	TBuf<32> tempBuf;
	tempBuf.Copy(aProtocol);
	tempBuf.UpperCase();

	TInt ret = tempBuf.Compare(KProtocolVerSSL30);
	if ( ret == 0 )
	{
		iTlsProvider->Attributes()->iProposedProtocol = KSSL3_0;
	}
	else
	{
		ret = tempBuf.Compare(KProtocolVerTLS10);
		if ( ret == 0 )
		{
			iTlsProvider->Attributes()->iProposedProtocol = KTLS1_0;
		}
		else
			return KErrNotSupported;
	}
	return KErrNone;
}

TInt CTlsConnection::SetServerCert(const CX509Certificate& /*aCert*/)
/**
 * Reserved for future work, always returns KErrNotSupported. 
 * 
 * @param aCert The certificate to use.
 * @return Any one of the system error codes, or KErrNone on success. 
 */
{
	LOG(Log::Printf(_L("CTlsConnection::SetServerCert()"));)
	return KErrNotSupported;
}

void CTlsConnection::StartClientHandshake(TRequestStatus& aStatus)
/**
 * Starts a client request and initiates a handshake 
 * with the remote server.
 * Configuration retrieval happens during construction of the CTlsConnection object,
 * which progresses the connection into the Idle state. 
 *
 * @param aStatus On completion, any one of the system error codes, or KErrNone 
 * on success (handshake negotiation complete). 
 */
{
	LOG(Log::Printf(_L("CTlsConnection::StartClientHandshake()"));)
	TRequestStatus* pStatus = &aStatus;

	if ( !IsIdle() )	// The connection must be in the Idle state
	{
		User::RequestComplete( pStatus, KErrInUse );
		return;
	}

	StartClientHandshakeStateMachine( pStatus );
}

void CTlsConnection::StartServerHandshake(TRequestStatus& aStatus)
/**
 * Start acting as a server and listen for a handshake from the remote client.
 * This is an asynchronous call, and will only complete when a client completes the 
 * handshake, or if it fails.
 * Normally, the socket passed in will usually have been previously used in a call to 
 * Accept() on a listening socket, but this is not required. 
 * Note that this implementation does not support Server mode operation, so this method
 * is NOT supported.
 *
 * @param aStatus On completion, any one of the system error codes, or KErrNone on success. 
 */
{
	LOG(Log::Printf(_L("CTlsConnection::StartServerHandshake()"));)
	TRequestStatus* pStatus = &aStatus;

	User::RequestComplete( pStatus, KErrNotSupported );
}



//MStateMachineNotify interface
TBool CTlsConnection::OnCompletion( CStateMachine* aStateMachine )
/**
 * Called only when negotiation or renegotiation has completed.
 */
{
	LOG(Log::Printf(_L("CTlsConnection::OnCompletion()"));)
#ifdef _DEBUG
   TInt nBlock;
  	LOG(Log::Printf(_L("RHeap::Size(), RHeap::Size() - RHeap::Available() %d, %d"), User::Heap().Size(), User::Heap().Size() - User::Heap().Available( nBlock ) );)
#endif

   iRecordParser->IgnoreAppData( 0 );
   __ASSERT_DEBUG( !aStateMachine->SuspendRequest(), TlsPanic(ETlsPanicStateMachineStopped) );
   if ( aStateMachine->LastError() != KErrNone )
   {//user will be notified after return from this fn
	   if ( iHandshake != aStateMachine )
	      {
		   return EFalse;
	      }
      else
         {//delete data path in case it's re-negotiation what's failed
         delete iSendAppData;
         iSendAppData = NULL;
         delete iRecvAppData;
         iRecvAppData = NULL;
         ResetCryptoAttributes();
         }
   }
   else
   {//from now on we propose the alrady negotiated protocol untill the connection is closed
      iTlsProvider->Attributes()->iProposedProtocol = iTlsProvider->Attributes()->iNegotiatedProtocol;
      if ( IsReNegotiating() )
      {
         //resume the SM statuses
         TRAPD(ret, iSendAppData->ResumeL();
            iRecvAppData->ResumeL( *this ) );
         if ( ret != KErrNone )
         {//something went completely wrong
          //set last error so that the user will be notified after return from this fn
            aStateMachine->SetLastError( ret );
            delete iSendAppData;
            iSendAppData = NULL;
            delete iRecvAppData;
            iRecvAppData = NULL;
         }
         else
         {
            __ASSERT_DEBUG( !iSendAppData->IsActive(), TlsPanic(ETlsPanicAlreadyActive));
            if ( iSendAppData->ClientStatus() ) //has the SM finished?
            {//no => start it again to resume the task
               iSendAppData->Start( iSendAppData->ClientStatus(), this );
            }
            //recv SM is active when re-negotiation started via HelloRequest
            if ( !iRecvAppData->IsActive() && iRecvAppData->ClientStatus() ) 
            {//not active and not finished => start it again to resume the the task
               iRecvAppData->Start( iRecvAppData->ClientStatus(), this );
            }
         }
      }
      else if ( !IsInDataMode() )
      {
	     // Create the Data state machines so that the user can send/receive data
         __ASSERT_DEBUG( !iRecvAppData && !iSendAppData, TlsPanic(ETlsPanicStateMachineAlreadyExists));
      
	     //don't change the order see CRecvAppData::ResumeL
	     TRAPD( ret, iSendAppData = CSendAppData::NewL( *iRecordComposer );
		     iRecvAppData = CRecvAppData::NewL( *this ) );
         if ( ret != KErrNone )
         {//something went completely wrong
          //set last error so that the user will be notified after return from this fn
            aStateMachine->SetLastError( ret );
            //delete what may have been created
            delete iRecvAppData;
            iRecvAppData = 0;
            delete iSendAppData;
            iSendAppData = 0;
         }
      }
      else
      {
	      // Must be one of the Application data SM
          __ASSERT_DEBUG( iRecvAppData == aStateMachine ||
             iSendAppData == aStateMachine, TlsPanic(ETlsPanicInvalidStateMachine));
          return EFalse; // Don't want to delete either of them.
      }
   }
   __ASSERT_DEBUG( iHandshake == aStateMachine, TlsPanic(ETlsPanicInvalidStateMachine));
   iHandshake = 0;
   return ETrue; // Delete the Handshake state machine.
}


// Internal functions

TBool CTlsConnection::IsIdle() const
/**
 * Returns 'True' if a connection is idle.
 */
{
	LOG(Log::Printf(_L("CTlsConnection::IsIdle()"));)
	return !iHandshake && !iSendAppData;
}


TBool CTlsConnection::SendData( const TDesC8& aDesc, TRequestStatus& aStatus )
/**
 * Starts the Application data transmission state machine, 
 * which sends data to a remote Server.
 *
 * @param aDesc Reference to the user's descriptor (data buffer)
 * @param aClientStatus TRequestStatus object that completes when data transmission
 * is finished.
 */
{
	LOG(Log::Printf(_L("CTlsConnection::SendData()"));)

	TRequestStatus* pStatus = &aStatus;
	if ( !iSendAppData )
	{
		User::RequestComplete( pStatus, KErrNotReady );
		return EFalse;
	}
	else if ( iSendAppData->ClientStatus() )
	{
		User::RequestComplete( pStatus, KErrInUse );
		return EFalse;
	}
	else if ( IsReNegotiating() )
	{
		iSendAppData->SetUserData( (TDesC8*)&aDesc );
		iSendAppData->SetClientStatus( &aStatus );
		//and wait for re-negotiation to finish (see CTlsConnection::OnCompletion)
	}
	else
	{	
		iRecordComposer->SetUserData( (TDesC8*)&aDesc );
   	iRecordComposer->ResetCurrentPos();
		iSendAppData->Start( &aStatus, this );
	}
	
	return ETrue;
}

TBool CTlsConnection::RecvData( TDes8& aDesc, TRequestStatus& aStatus )
/**
 * Starts the Application data reception state machine, 
 * which receives data from a remote Server.
 *
 * @param aDesc Reference to the user's descriptor (data buffer)
 * @param aClientStatus TRequestStatus object that completes when data reception is 
 * finished.
 */
{
	LOG(Log::Printf(_L("CTlsConnection::RecvData()"));)
	TBool result = EFalse;

	TRequestStatus* pStatus = &aStatus;

	if ( !iRecvAppData )
		User::RequestComplete( pStatus, KErrNotReady );
	else if ( iRecvAppData->ClientStatus() || IsReNegotiating() )
      User::RequestComplete( pStatus, KErrInUse );
	else
	{
		iRecordParser->SetUserData( &aDesc );
		iRecordParser->SetUserMaxLength( aDesc.MaxLength() );
		iRecvAppData->Start( &aStatus, this );
		result = ETrue;
	}
   
	return result;
}

void CTlsConnection::StartClientHandshakeStateMachine(TRequestStatus* aStatus)
/**
 * Creates and starts the Handshake negotiation state machine, 
 * which initiates negotiations with the remote Server.
 */
{
	LOG(Log::Printf(_L("CTlsConnection::StartClientHandshakeStateMachine()"));)
	delete iClientCert;
   iClientCert = NULL;
	delete iServerCert;
   iServerCert = NULL;

	// Assert that a Handshake negotiation object doesn't exist and that a request
	// status object is valid.
	__ASSERT_DEBUG( !iHandshake, TlsPanic(ETlsPanicStateMachineAlreadyExists) ); 
	__ASSERT_DEBUG( aStatus, TlsPanic(ETlsPanicInvalidStatus));

	TRAPD( ret, iHandshake = CHandshake::NewL(*this) );
	if ( ret != KErrNone )
		User::RequestComplete(aStatus, ret);
	else
  		{
  		TRAP(ret, iHandshake->StartL(aStatus, this));
  		if (ret!=KErrNone)
  			{
         delete iHandshake;
         iHandshake = NULL;
  			User::RequestComplete(aStatus, ret);
  			}
  		}



#ifdef _DEBUG
   TInt nBlock;
  	LOG(Log::Printf(_L("RHeap::Size(), RHeap::Size() - RHeap::Available() %d, %d"), User::Heap().Size(), User::Heap().Size() - User::Heap().Available( nBlock ) );)
#endif
}

void CTlsConnection::ResetCryptoAttributes()
{//don't change the order of the calls see (see ~CRecordParser & CRecordParser::Reset)
	LOG(Log::Printf(_L("CTlsConnection::ResetCryptoAttributes()"));)
   iRecordComposer->Reset();
   iRecordParser->Reset();
//   iRecordParser->SetTlsProvider( NULL );
//   iRecordComposer->SetTlsProvider( NULL );
   if ( iTlsProvider )
      {
	   TRAPD(ret, iTlsProvider->ReConnectL());		// Set up Security/crypto interfaces
	   delete iTlsSession; 
      iTlsSession = NULL;
      if ( ret )
         {
         delete iTlsProvider;
         iTlsProvider = NULL;
         iRecordParser->SetTlsProvider( iTlsProvider );
         iRecordComposer->SetTlsProvider( iTlsProvider );
         }
      }
}

void CTlsConnection::StartRenegotiation( TRequestStatus* aStatus )
/**
 * Starts Handshake renegotiation. 
 *
 * It suspends the Application Data (transmission and reception) state machines
 * and restarts the Handshake negotiation state m/c.
 * It also creates a new TLS Provider object to access security services. This is 
 * necessary as a new cryptographic token might be selected to create new key material.
 * The session cache is flushed as session reuse should not take place (ensure that entirely
 * new key material is generated).
 */
{
	LOG(Log::Printf(_L("CTlsConnection::StartRenegotiation()"));)
   //User::RequestComplete( aStatus, KErrNotSupported ); //not until we have CTlsProvider::Close()

	iSendAppData->Suspend();
	iRecvAppData->Suspend();
   iRecordParser->IgnoreAppData( 1 );
	StartClientHandshakeStateMachine( aStatus );
}

void CTlsConnection::DeleteStateMachines()
/**
 * Deletes the connections' state machines.
 */
{
	LOG(Log::Printf(_L("CTlsConnection::DeleteStateMachines()"));)

	delete iHandshake;
	iHandshake = NULL;
   
	delete iRecvAppData;
	iRecvAppData = NULL;
	
	delete iSendAppData;
	iSendAppData = NULL;
}

void CTlsConnection::CancelAll( TInt aError )
/**
 * Cancels all outstanding operations. It is called by the Secure socket API, 
 * CTlsConnection::CancelAll().
 */
{
	LOG(Log::Printf(_L("CTlsConnection::CancelAll()"));)

	if ( iRecvAppData )
	{
		iRecvAppData->Cancel( KErrNone );
	}

	if ( iSendAppData )
	{
		// Send an alert if it is not going to be sent by iHandshake
		iSendAppData->Cancel( iHandshake ? KErrNone : aError );
	}

	if ( iHandshake )
	{
		iHandshake->Cancel( aError );
		__ASSERT_DEBUG(!iHandshake, TlsPanic(ETlsPanicStateMachineAlreadyExists));
	}
}

void CTlsConnection::GetServerAddrInfo( TTLSServerAddr& serverInfo )
   {
	LOG(Log::Printf(_L("CTlsConnection::GetServerAddrInfo()"));)
	// Find out if there is an existing stored session for the RSocket object.
	TSockAddr sockAddr;		// Endpoint address for the socket object, TBuf<8>
	iRecordComposer->Socket().RemoteName( sockAddr );
	TInetAddr inetAddr( sockAddr );
	if ( sockAddr.Family() != KAfInet6 )
	   {
		inetAddr.ConvertToV4Mapped();
	   }

	serverInfo.iAddress.Copy(TPtr8( (TUint8*)(&inetAddr.Ip6Address().u.iAddr8[0]), sizeof( TIp6Addr ), sizeof( TIp6Addr )));
	serverInfo.iPort = TUint16(sockAddr.Port()); //see TTLSServerAddr definition
	LOG(Log::Printf(_L("CTlsConnection::GetServerAddrInfo() port: %d"), serverInfo.iPort );)
	LOG(RFileLogger::HexDump(KSSLLogDir,KSSLLogFileName,EFileLoggingModeAppend, NULL, NULL, serverInfo.iAddress.Ptr(), serverInfo.iAddress.Length() ));
   }

TInt CTlsConnection::GetKeyingMaterial(TDes8& aKeyingMaterial)
/*
Performs key generation as per RFC2716 (PPP EAP TLS Authentication Protocol) section 3.5
*/
	{
	if(iTlsSession == NULL)
		{
		LOG(Log::Printf(_L("iTlsSession needs to be created to call this function") );)
		return KErrNotReady;
		}
	
	if(aKeyingMaterial.Length()>KKeyingLabelMaxSize)
		{
		LOG(Log::Printf(_L("Supplied Descriptor is too large.  Size=%d Maximum=%d"), aKeyingMaterial.Length(), KKeyingLabelMaxSize);)
		return KErrArgument;
		}
	
	TBuf8<KKeyingLabelMaxSize> keyingLabel;
	keyingLabel.Copy(aKeyingMaterial);
	
	aKeyingMaterial.Zero();
	TInt err = iTlsSession->KeyDerivation(keyingLabel,iTlsSession->Attributes()->iMasterSecretInput, aKeyingMaterial);
	
	if(err == KErrNone && aKeyingMaterial.Length()<=0)
		{
		LOG(Log::Printf(_L("Failed to derive keys"));)
		err = KErrGeneral;
		}

	return err;
	}
