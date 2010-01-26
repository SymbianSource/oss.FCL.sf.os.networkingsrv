// Copyright (c) 2001-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Describes the implementation of a secure socket class.
// 
//

/**
 @file
*/

#include "SecureSocket.h"
#include "GenericSecureSocket.h"
#include <commsdattypesv1_1.h>

#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <ssl_internal.h>
#include <securesocket_internal.h>
#endif

#include <commsdattypeinfov1_1_internal.h>
#include <commsdattypesv1_1_partner.h>

using namespace CommsDat;

TInt SetTLSData(void);

#define LOADDLL_ORIG_ORDINAL 1
#define UNLOADDLL_ORDINAL 2
#define LOADDLL_NEW_ORDINAL 3

TSecSocketProtocol::~TSecSocketProtocol()
/** 
 * Closes the DLL library. 
 */
{
	iLibrary.Close();
}

TInt CSecureSocketLibraryLoader::OpenL(const TDesC& aProtocolName,TSecSockDllLibraryFunction& aEntryPoint)
/** 
 * Opens the protocol library.
 * Attempts to load the secure socket implementation DLL for the requested 
 * protocol type. 
 * 
 * @param aProtocolName	A reference to a descriptor containing the protocol name, 
 * 						i.e. tls1.0, ssl3.0. Case is ignored.
 * @param aEntryPoint	Entry point into the secureSocket plug-in.
 * @return				KErrNone, if successful; otherwise, another of the system-wide error codes. 
 */
	{
	TLibraryFunction entry;

	CSecureSocketLibraryLoader::OpenWithIdL(LOADDLL_ORIG_ORDINAL, aProtocolName, entry);
	aEntryPoint = reinterpret_cast<TSecSockDllLibraryFunction>(entry);
	if(!aEntryPoint)
		{
		return KErrNotFound;
		}

	return KErrNone;
	}


TInt CSecureSocketLibraryLoader::OpenL(const TDesC& aProtocolName,TSecSockDllLibraryGenericFunction& aEntryPoint)
/** 
 * Opens the protocol library.
 * Attempts to load the secure socket implementation DLL for the requested 
 * protocol type. 
 * 
 * @param aProtocolName	A reference to a descriptor containing the protocol name, 
 * 						i.e. tls1.0, ssl3.0. Case is ignored.
 * @param aEntryPoint	Entry point into the secureSocket plug-in.
 * @return				KErrNone, if successful; otherwise, another of the system-wide error codes. 
 */
	{
	TLibraryFunction entry;

	CSecureSocketLibraryLoader::OpenWithIdL(LOADDLL_NEW_ORDINAL, aProtocolName, entry);
	aEntryPoint = reinterpret_cast<TSecSockDllLibraryGenericFunction>(entry);
	if(!aEntryPoint)
		{
		return KErrNotFound;
		}

	return KErrNone;
	}


void CSecureSocketLibraryLoader::OpenWithIdL(TInt aId, const TDesC& aProtocolName, TLibraryFunction& aEntryPoint)
/** 
 * Opens the protocol library.
 * Attempts to load the secure socket implementation DLL for the requested 
 * protocol type. 
 * 
 * @param aProtocolName	A reference to a descriptor containing the protocol name, 
 * 						i.e. tls1.0, ssl3.0. Case is ignored.
 * @param aEntryPoint	Entry point into the secureSocket plug-in.
 * @return				KErrNone, if successful; otherwise, another of the system-wide error codes. 
 */
{
	//Fetch the TLS data
	TSecureSocketGlobals* global = (TSecureSocketGlobals*)Dll::Tls();
	if ( !global )
	{
		if (SetTLSData() == KErrNone) 
			global = (TSecureSocketGlobals*)Dll::Tls();
	}
	__ASSERT_ALWAYS(global,User::Leave(KErrNoMemory));

	// Increase the globals use count
	global->iUseCount++;

	// Convert to lower case before doing a comparison
	TBuf<32> protocol;
	protocol.Copy(aProtocolName);
	protocol.LowerCase();

	//Walk the list and find the protocol
	global->iSecureSocketProtocolsIter.SetToFirst(); 
	TSecSocketProtocol* secProtocol;
	while ((secProtocol = global->iSecureSocketProtocolsIter++) != NULL)
		{
		if(secProtocol->iName.Compare(protocol))
			//Found a matching protocol name
			break;
		}

	if(secProtocol == NULL)
	//Library not loaded yet - Find a dll name in Commdb that matches proto name
	//and load the dll
		{
		secProtocol = new(ELeave)TSecSocketProtocol;
		CleanupStack::PushL(secProtocol);
		//Find the protocolname record in COMMDB
		TBuf<KMaxFileName> fileName;
		FindItemInDbL(protocol, fileName);

		// Load the library whose name we just fetched
		// Commented out the line below as it loads the SSLADAPTOR.DLL
		// Hardcoded the SSL.DLL. SSL.DLL ultimately should be an ECOM Plugin.
		// (These changes were made as part of GT167 Zephyr work on TLS.
		//(void)User::LeaveIfError(secProtocol->iLibrary.Load(fileName));
		(void)User::LeaveIfError(secProtocol->iLibrary.Load(_L("SSL.DLL")));


		//Add the entry in the list
		secProtocol->iName.Copy(protocol);
		global->iSecureSocketProtocols.AddLast(*secProtocol);
		CleanupStack::Pop();
		}

	// Get the pointer to the NewL function in the DLL
	aEntryPoint = secProtocol->iLibrary.Lookup(aId);	
	
}

void CSecureSocketLibraryLoader::FindItemInDbL(const TDesC& aProtocolName, TDes& aLibraryName)
/**
 * Opens the Commdb and locates the DLL name for a protocol.
 * 
 * @param aProtocolName	A reference to a descriptor containing the protocol name, 
 * 						i.e. tls1.0, ssl3.0. Case is ignored.
 * @param aLibraryName 	On return, name of the library.
 * @exception			This function will leave if the protocol was not found or in OOM conditions. 
 */
{

#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
		CMDBSession* session = CMDBSession::NewL(KCDVersion1_2);
#else
		CMDBSession* session = CMDBSession::NewL(KCDVersion1_1);
#endif
		CleanupStack::PushL(session);
		CCDSecureSocketRecord* ptrRecord = static_cast<CCDSecureSocketRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdSSProtoRecord));
		CleanupStack::PushL(ptrRecord);
		ptrRecord->iSSProtoName = aProtocolName;
		// check if any record is returned back
		if (!ptrRecord->FindL(*session))
			{
			// ignore return from FindL otherwise tests fail
			//User::Leave(KErrNotFound);
			}

		aLibraryName = ptrRecord->iSSProtoLibrary;
		CleanupStack::PopAndDestroy(ptrRecord);
		CleanupStack::PopAndDestroy(session);
}

EXPORT_C void CSecureSocketLibraryLoader::Unload()
/** 
 * Closes and unloads the implementation library.
 */
{
	//Fetch the TLS data
	TSecureSocketGlobals* global = (TSecureSocketGlobals*)Dll::Tls();

	//Cannot assume that tls exists at this point (False with WINS)
	if(!global)
		return;

	// if the use count isn't 0, don't do the unload
	if ( --global->iUseCount )
		return;

	//Walk the list and find the protocol
	global->iSecureSocketProtocolsIter.SetToFirst(); 
	TSecSocketProtocol* secProtocol;
	
	while ((secProtocol = global->iSecureSocketProtocolsIter++) != NULL)
		{
		//Find the entry point for  the resource cleanup function 
		TSecSockDllUnloadFunction cleanupEntryPoint  = reinterpret_cast<TSecSockDllUnloadFunction> (secProtocol->iLibrary.Lookup(UNLOADDLL_ORDINAL));
		//Call the function
		if(cleanupEntryPoint)
			(*cleanupEntryPoint)(NULL);
		global->iSecureSocketProtocols.Remove(*secProtocol);
		delete secProtocol;
		}
	// remove the globals reference
	delete global;
	Dll::SetTls(NULL); // @bug check ret val
}

EXPORT_C CSecureSocket* CSecureSocket::NewL(RSocket& aSocket,const TDesC& aProtocol)
/** 
 * Creates and returns a pointer to a new secure socket.
 *
 * A reference to an already open and connected socket should be passed in, 
 * along with a descriptor that contains the protocol name.
 *
 * @param aSocket	A reference to an open and connected RSocket object.
 * @param aProtocol	A constant descriptor containing the protocol name.
 * @return 			A pointer to the newly created secure socket, or NULL if the creation failed.
 */
{
	CSecureSocket* self = new(ELeave) CSecureSocket;
	CleanupStack::PushL( self );
	self->ConstructL(aSocket,aProtocol);
	CleanupStack::Pop();
	return self;
}

EXPORT_C CSecureSocket* CSecureSocket::NewL(MGenericSecureSocket& aSocket,const TDesC& aProtocol)
/** 
 * Creates and returns a pointer to a new secure socket.
 *
 * A reference to a socket derived from MGenericSecureSocket should be passed in, 
 * along with a descriptor that contains the protocol name.
 *
 * @param aSocket	A reference to an MGenericSecureSocket derived object.
 * @param aProtocol	A constant descriptor containing the protocol name.
 * @return 			A pointer to the newly created secure socket, or NULL if the creation failed.
 */
{
	CSecureSocket* self = new(ELeave) CSecureSocket;
	CleanupStack::PushL( self );
	self->ConstructL(aSocket,aProtocol);
	CleanupStack::Pop();
	return self;
}

void CSecureSocket::ConstructL(RSocket& aSocket, const TDesC& aProtocol)
/** 
 * Standard 2-phase construction.
 */
{
	TSecSockDllLibraryFunction libEntryPointL;
	User::LeaveIfError(CSecureSocketLibraryLoader::OpenL(aProtocol,libEntryPointL));
	//Beware! the following call can leave...
	iSecureImplementation = (MSecureSocket*)libEntryPointL( aSocket, aProtocol );
	if (iSecureImplementation)
		{
		iSecureSocketState = ESecureSocketStateOpen;
		}

}

void CSecureSocket::ConstructL(MGenericSecureSocket& aSocket, const TDesC& aProtocol)
/** 
 * Standard 2-phase construction.
 */
{
	TSecSockDllLibraryGenericFunction libEntryPointL;
	User::LeaveIfError(CSecureSocketLibraryLoader::OpenL(aProtocol,libEntryPointL));
	//Beware! the following call can leave...
	iSecureImplementation = (MSecureSocket*)libEntryPointL( aSocket, aProtocol );
	if (iSecureImplementation)
		{
		iSecureSocketState = ESecureSocketStateOpen;
		}
}

CSecureSocket::~CSecureSocket()
/** 
 *Standard destructor. 
 */
{
	if ((iSecureImplementation) && (iSecureSocketState == ESecureSocketStateOpen))
	{
		iSecureImplementation->Close();
	}

	delete iSecureImplementation;
	// Close and unload the implementation library, Unload checks access count first
	CSecureSocketLibraryLoader::Unload();
}


//
EXPORT_C TInt CSecureSocket::AvailableCipherSuites( TDes8& aCiphers )
/** 
 * Gets the available cipher suites.
 *
 * Note that ciphersuites using NULL encryption or PSK key exchange will not be included
 * unless they have been enabled via SetOpt.
 *
 * @param aCiphers	Descriptor holding the ciphers.
 * @return			KErrNone if successful, a system-wide error code if not.
 */
{
	return iSecureImplementation->AvailableCipherSuites( aCiphers );
}

EXPORT_C void CSecureSocket::CancelAll()
/** 
 *Cancels all the send and receive actions in the SSL state machine. 
 */
{
	iSecureImplementation->CancelAll();
}

EXPORT_C void CSecureSocket::CancelHandshake()
/** 
 *Cancels the handshake. 
 */
{
	iSecureImplementation->CancelHandshake();
}

EXPORT_C void CSecureSocket::CancelRecv()
/** 
 * Cancels a receive action in the SSL state machine. 
 */
{
	iSecureImplementation->CancelRecv();
}

EXPORT_C void CSecureSocket::CancelSend()
/** 
 *Cancels a send action in the SSL state machine.
 */
{
	iSecureImplementation->CancelSend();
}

EXPORT_C const CX509Certificate* CSecureSocket::ClientCert()
/** 
 * Gets the current client certificate.
 * 
 * When a secure socket is acting in server mode, the returned certificate will 
 * be the certificate that the remote client provided. When acting in client mode, 
 * the certificate returned will be local certificate. 
 * 
 * @return	A pointer to the client certificate, or NULL if none exists. 
 */
{
	return iSecureImplementation->ClientCert();
}

EXPORT_C TClientCertMode CSecureSocket::ClientCertMode()
/** 
 * Gets the current client certificate mode.
 *
 * The client certificate mode is used when the socket is acting as a server, 
 * and determines whether a client certificate is requested. 
 *
 * @return	The current mode that is set. 
 */
{
	return iSecureImplementation->ClientCertMode();
}

EXPORT_C TDialogMode CSecureSocket::DialogMode()
/** 
 * Gets the current dialog mode. 
 * 
 * @return The current dialog mode.
 */
{
	return iSecureImplementation->DialogMode();
}

EXPORT_C void CSecureSocket::Close()
/** 
 * Closes the secure connection and the socket.
 *
 * Implementations should terminate the secure connection gracefully as 
 * appropriate to their protocol. The RSocket object is not destoyed: 
 * this is left to the client application. 
 */
{
	iSecureSocketState = ESecureSocketStateClosed;
	iSecureImplementation->Close();
}

EXPORT_C TInt CSecureSocket::CurrentCipherSuite( TDes8& aCipherSuite )
/** 
 * Gets the current cipher suite in use.
 * 
 * The current cipher suite is returned in the referenced buffer 
 * in two byte format as, i.e. [0x??][0x??].
 *
 * @param aCipherSuite	A reference to a descriptor at least 2 bytes long. 
 *						Implementations that differ from the [0x??][0x??] 
 *						format may require larger descriptors. See individual 
 *						implementation notes for details.
 * @return				KErrNone if successful; 
 *						otherwise, another of the system-wide error codes. 
 */
{
	return iSecureImplementation->CurrentCipherSuite( aCipherSuite );
}

EXPORT_C void CSecureSocket::FlushSessionCache()
/** 
 *Flushes the session cache. 
 */
{
	iSecureImplementation->FlushSessionCache();
}

EXPORT_C TInt CSecureSocket::GetOpt(TUint aOptionName, TUint aOptionLevel, TDes8& aOption)
/** 
 * Gets an option.
 *
 * Secure socket implementations may provide options that can 
 * be used with this function. 
 *
 * (nb. Getting the KSoServerNameIndication option is not supported).
 *
 * @param aOptionName	An integer constant which identifies an option. 
 * @param aOptionLevel	An integer constant which identifies the level of an option, 
 * 						i.e. an option level group of related options.
 * @param aOption		An option value packaged in a descriptor.
 * @return 				KErrNone if successful; 
 *						otherwise, another of the system-wide error codes. 
 */
{
	return iSecureImplementation->GetOpt(aOptionName, aOptionLevel, aOption);
}

EXPORT_C TInt CSecureSocket::GetOpt(TUint aOptionName, TUint aOptionLevel, TInt& aOption)
/** 
 * Gets an option.
 *
 * Secure socket implementations may provide options that can 
 * be used with this method. 
 *
 * (nb. Getting the KSoServerNameIndication option is not supported).
 *
 * @param aOptionName	An integer constant which identifies an option. 
 * @param aOptionLevel	An integer constant which identifies the level of an option, 
 * 						i.e. an option level group of related options.
 * @param aOption		An integer option value.
 * @return 				KErrNone if successful; 
 *						otherwise, another of the system-wide error codes. 
 */
{
	return iSecureImplementation->GetOpt(aOptionName, aOptionLevel, aOption);
}

EXPORT_C TInt CSecureSocket::Protocol(TDes& aProtocol)
/**
 * Gets the protocol in use.
 *
 * This method can be used to return the particular protocol/version that is 
 * being used by implementations that support different protocols/versions. 
 *
 * @param aProtocol	A descriptor containing the protocol name/version that is 
 *					being used. Protocol names can be up to 32 characters long, 
 *					and so a descriptor of at least that size is required.
 * @return			KErrNone 
 */
{
	return iSecureImplementation->Protocol( aProtocol );
}

EXPORT_C void CSecureSocket::Recv (TDes8& aDesc, TRequestStatus& aStatus )
/**
 * Receives data from the socket.
 *
 * This is an asynchronous function, and completes 
 * when the descriptor has been filled. Only one Recv() or RecvOneOrMore() 
 * operation can be outstanding at any time.
 *
 * @param aDesc		A descriptor where data read will be placed.
 * @param aStatus 	On completion, KErrNone if successful; 
 *					KErrEof if a remote connection is closed and there is no more data; 
 *					KErrNotReady if called when an operation is still outstanding; 
 *					or another system-wide error code. 
 */
{
	iSecureImplementation->Recv( aDesc, aStatus );
}

EXPORT_C void CSecureSocket::RecvOneOrMore( TDes8& aDesc, TRequestStatus& aStatus, TSockXfrLength& aLen )
/** 
 * Receives data from the socket.
 *
 * This is an asynchronous function, and will complete when at least one byte has been read. 
 * Only one Recv() or RecvOneOrMore() operation can be outstanding at any time.
 * 
 * @param aDesc		A descriptor where data read will be placed.
 * @param aStatus	On completion, KErrNone if successful;
 *					KErrEof if a remote connection is closed and there is no more data;
 * 					KErrNotReady if called when an operation is still outstanding;
 * 					or another system-wide error code.
 * @param aLen		On completion, the length of the descriptor, aDesc. 
 */
{
	iSecureImplementation->RecvOneOrMore( aDesc, aStatus, aLen );
}

EXPORT_C void CSecureSocket::RenegotiateHandshake(TRequestStatus& aStatus )
/**
 * Initiates a renegotiation of the secure connection.
 *
 * This is an asynchronous function that completes when renegotiation is complete. 
 * It is valid for both client and server operation. There can only be one outstanding 
 * RenegotiateHandshake() operation at a time.
 *
 * @param aStatus	On completion, KErrNone if successful;
 * 					KErrNotReady if called when an operation is still outstanding;
 * 	 				or another system-wide error code. 
 */
{
	iSecureImplementation->RenegotiateHandshake( aStatus );
}

EXPORT_C void CSecureSocket::Send( const TDesC8& aDesc, TRequestStatus& aStatus, TSockXfrLength& aLen )
/**
 * Sends data over the socket.
 *
 * This is an asynchronous function. Only one Send() operation can be outstanding at any time. 
 * 
 * @param aDesc		A constant descriptor with the data to be send.
 * @param aStatus	On completion, KErrNone if successful; 
 *					KErrNotReady if called when an operation is still outstanding;
 *				 	or another system-wise error code.
 * @param aLen		On completion, the amount of data sent.
 */
{
	iSecureImplementation->Send( aDesc, aStatus, aLen );
}

EXPORT_C void CSecureSocket::Send( const TDesC8& aDesc, TRequestStatus& aStatus )
/** 
 * Sends data over the socket.
 *
 * This is an asynchronous function. Only one Send() operation can be outstanding 
 * at any time, and the function will complete with the error KErrNotReady if called 
 * when a send is still outstanding.
 *
 * @param aDesc		A constant descriptor. The application must not modify this  
 *					descriptor until the Send() completes.
 * @param aStatus	On completion, KErrNone;
 * 					KErrNotReady if called when a send is still outstanding, if successful;
 *					or another system-wide error code. 
 */
{
	iSecureImplementation->Send( aDesc, aStatus );
}

EXPORT_C const CX509Certificate* CSecureSocket::ServerCert()
/** 
 * Gets the current server certificate.
 *
 * When a secure socket is acting in client mode, the returned certificate will be
 * the certificate for the remote server. When acting in server mode, the certificate
 * returned will be the local certificate. 
 *
 * Note that	the operation in server mode is currently reserved for future use, 
 * and returns NULL.
 * 
 * @return	Pointer to the certificate, or NULL if no certificate is available. 
 */
{
	return iSecureImplementation->ServerCert();
}

EXPORT_C TInt CSecureSocket::SetAvailableCipherSuites(const TDesC8& aCiphers)
/**
 * Sets the list of cipher suites that are available for use.
 *
 * The list of cipher suites should be supplied in a descriptor in the format 
 * as per the TLS RFC, i.e. [0x??][0x??] for each suite. The order of suites is 
 * important, and so they should be listed with the preferred suites first. 
 *
 * Note that ciphersuites using NULL encryption or PSK key exchange will be considered
 * unsupported unless these features have been enabled via SetOpt.
 *
 * Unsupported ciphersuites are silently ignored except that if the list
 * becomes empty KErrNotSupported will be returned.
 *
 * @param aCiphers	Descriptor holding the cipher suites list.
 * @return			KErrNone if successful; otherwise, a system-wide error code. 
 */
{
	return iSecureImplementation->SetAvailableCipherSuites( aCiphers );
}

EXPORT_C TInt CSecureSocket::SetClientCert(const CX509Certificate& aCert)
/**
 * Sets the client certificate to use.
 *
 * When a secure socket is acting in client mode, this method will set the certificate 
 * that will be used if a server requests one. When acting in server mode, if called 
 * this method will perform no action, but will return KErrNotSupported. 
 *
 * Note that this method is currently reserved for future use, and always returns 
 * KErrNotSupported. 
 * 
 * @param aCert	The client certificate.
 * @return		KErrNone if successful; otherwise, a system-wide error code.
 */
{
	return iSecureImplementation->SetClientCert( aCert );
}

EXPORT_C TInt CSecureSocket::SetClientCertMode(const TClientCertMode aClientCertMode)
/** 
 * Sets the client certificate mode.
 *
 * @param aClientCertMode	The client certificate mode to set.
 * @return					KErrNone if successful; otherwise, a system-wide error code.
 */
{
	return iSecureImplementation->SetClientCertMode( aClientCertMode );
}

EXPORT_C TInt CSecureSocket::SetDialogMode(const TDialogMode aDialogMode)
/** 
 * Sets the Dialog mode.
 * 
 * @param aDialogMode	Dialog mode to set.
 * @return				KErrNone if successful, otherwise, a system-wide error code.
 */
{
	return iSecureImplementation->SetDialogMode( aDialogMode );
}

EXPORT_C TInt CSecureSocket::SetProtocol(const TDesC& aProtocol)
/** 
 * Sets the protocol
 * 
 * @param aProtocol	Descriptor holding the protocol name to be set, 
 * 					e.g. "SSL3.0" or "TLS1.0".
 * @return			KErrNone if successful, or KErrNotSupported if the protocol in 
 *					the descriptor isn't recognized.
 */
{
	return iSecureImplementation->SetProtocol( aProtocol );
}

EXPORT_C TInt CSecureSocket::SetOpt(TUint aOptionName, TUint aOptionLevel, const TDesC8& aOption)
/** 
 * Sets an option.
 *
 * SecureSocket implementations may provide options that can be used with this method. 
 * See individual implementation notes for details.
 *
 * In order for full verification of the Server certificate during handshake negotiation
 * the domain name must be set.
 * This is done using the option KSoSSLDomainName, with the option level KSolInetSSL.
 *
 * In order to use a TLS PSK ciphersuite the user must use the the option KSoPskConfig,
 * with the option level KSolInetSSL.
 * The aOption argument should be a TPckgBuf<MSoPskKeyHandler *>. This passes in a pointer
 * to an object which implements the MSoPskKeyHandler interface to decide which PSK identity and
 * value the client wishes to use to secure the connection. See MSoPskKeyHandler for further details.
 * If the MSoPskKeyHandler is NULL then PSK ciphersuites will be disabled again. If you specified an exact list
 * of ciphersuites (by calling SetAvailableCipherSuites) you must update that list to exclude PSK ciphersuites.
 *
 * The option KSoServerNameIndication, with the option level KSolInetSSL can be used to include the 
 * RFC3546 server name indication in the ClientHello sent to the server. This can be used to facilitate
 * secure connections to servers that host multiple 'virtual' servers at a single underlying
 * network address. The aOption argument should be a TPckgBuf<CDesC8Array *>, ownership is passed in.
 * One or more UTF-8 FQDNs can be supplied. Neither trailing dots nor numeric IP addresses should be used.
 *
 * @param aOptionName	An integer constant which identifies an option.
 * @param aOptionLevel	An integer constant which identifies the level of an option: 
 * 						i.e. an option level group of related options.
 * @param aOption		An option value packaged in a descriptor.
 * @return				KErrNone if successful; otherwise, a system-wide error code.
 */
{
	return iSecureImplementation->SetOpt( aOptionName, aOptionLevel, aOption );
}

EXPORT_C TInt CSecureSocket::SetOpt(TUint aOptionName, TUint aOptionLevel, TInt aOption)
/** 
 * Sets an option.
 *
 * SecureSocket implementations may provide options that can be used with this method. 
 * See individual implementation notes for details.
 *
 * By default the TLS_RSA_WITH_NULL_MD5 and TLS_RSA_WITH_NULL_SHA ciphersuites are disabled.
 * These ciphersuites use NULL encryption and therefore offer no protection against evesdropping.
 * Server authentication (and client, if a client certificate is used) is performed and data integrity
 * is still checked (nb. TLS_NULL_WITH_NULL_NULL is never supported).
 * In order to these ciphersuites the user must use the the option KSoEnableNullCiphers, with the
 * option level KSolInetSSL and a non-zero argument. Using an argument of zero will disable them.
 * 
 * @param aOptionName	An integer constant which identifies an option.
 * @param aOptionLevel	An integer constant which identifies the level of an option: 
 * 						i.e. an option level group of related options.
 * @param aOption		An option value as an integer .
 * @return				KErrNone if successful; otherwise, a system-wide error code.
 */
{
	return iSecureImplementation->SetOpt( aOptionName, aOptionLevel, aOption );
}

EXPORT_C TInt CSecureSocket::SetServerCert(const CX509Certificate& aCert)
/** 
 * Sets the server X.509 certificate.
 * 
 * @param aCert	The certificate to use.
 * @return		KErrNone if successful; otherwise, a system-wide error code.
 */
{
	return iSecureImplementation->SetServerCert( aCert );
}

EXPORT_C void CSecureSocket::StartClientHandshake(TRequestStatus& aStatus)
/** 
 * Starts the client handshake.
 * 
 * @param aStatus	On completion, KErrNone if successful; 
 *					otherwise, a system-wide error code.
 */
{
	iSecureImplementation->StartClientHandshake( aStatus );
}

EXPORT_C void CSecureSocket::StartServerHandshake(TRequestStatus& aStatus)
/** 
 * Starts the server handshake.
 * 
 * @param aStatus	On completion, KErrNone if successful;
 *				 	otherwise, a system-wide error code.
 */
{
	iSecureImplementation->StartServerHandshake( aStatus );
}

TInt SetTLSData()
/**
 * Sets the Thread local storage data.
 */
{
	TInt ret=KErrNone;

	// Create a new DllGlobals class
	TSecureSocketGlobals* global = new TSecureSocketGlobals;
	if(!global)
		{
		return KErrNoMemory;
		}
    
	ret = Dll::SetTls( global );

	if(ret)
		{
		delete global;
		}
	else
		{
		global->iUseCount=0;
		}

	return ret;
}
	

