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
//


#ifndef __SECURESOCKETINTERFACE_H__
#define __SECURESOCKETINTERFACE_H__

#include <e32base.h>
#include <sslerr.h>
#include <x509cert.h>
#include <es_sock.h>

// File description
/** 
 * @file SecureSocketInterface.h
 * Definition of the MSecureSocket class.
 *
 * @publishedAll
 * @released
 */

/** 
 * Server client certificate mode.
 * Specifies if client certificates will be asked for when in server mode, and also if they are optional
 * or must be provided to complete the handshake successfully.
 *
 * @since v7.0
 */
enum TClientCertMode
	{
	/** Client certificates won't be asked for during handshake negotiation. */
	EClientCertModeIgnore,
	/** Client certificates will be requested, but are not compulsory, 
	  * and the handshake will continue if the client doesn't supply one. */
	EClientCertModeOptional,
	/** Client certificates must be supplied, and the handshake will fail if 
	  * the client does not provide a one. */
	EClientCertModeRequired
	};

/**
 * Untrusted certificate dialog mode.
 * When an untrusted certificate is received, the dialog mode determines if the handshake 
 * fails automatically, or if a dialog is displayed allowing the user the option of continuing
 * anyway. 
 *
 * @since v7.0
 */
enum TDialogMode
	{
	/** All untrusted certificates result in a user dialog. */
	EDialogModeAttended,
	/** Untrusted certificates are canceled without user confirmation. */
	EDialogModeUnattended
	};


class MSecureSocket
/**
 * Abstract interface API for secure socket implementations.
 *
 * MSecureSocket is the interface that secure socket implementations must adhere to.
 * The API supports both client and server operation, see individual implementations'
 * documentation for details of client/server operation that they may support.
 * 
 * Secure socket implementations will be used to secure an already open and connected socket. 
 * The class must be passed a reference to an already open and connected socket when a new 
 * secure socket is created. New secure sockets are created through the CSecureSocket class, 
 * which hides the MSecureSocket class and the underlying plug-in nature of implementations 
 * from applications. Secure socket implementations MUST provide a NewL function that 
 * matches the following:
 *
 * @code
 * static MSecureSocket* NewL( RSocket& aSocket, const TDesC& aProtocol );
 * @endcode
 *
 * aSocket A reference to an already opened and connected socket.
 *
 * aProtocol A descriptor containing the name of a protocol, i.e. SSL3.0, TLS1.0, that the 
 * application must specify when it creates the secure socket. The maximum length that can 
 * be specified for a protocol name is 32 characters.
 *
 * For error code definitions see SSLErr.h
 *
 * @since 6.2
 */
	{

public:

	 /** 
	 * Gets the list of cipher suites that are available to use. 
	 * The list of cipher suites that will be used by default will be returned in the descriptor.
	 * They are returned in the order that they will be used during a handshake, and are assumed to
	 * be in the format as per the SSL/TLS RFCs, i.e. [0x??][0x??] for each suite. 
	 * See individual implementation notes for any differences.
	 *
	 * @param aCiphers	A reference to a descriptor, should be at least 64 bytes long. 
	 * @return			Any one of the system error codes, or KErrNone on success. */
	virtual TInt AvailableCipherSuites( TDes8& aCiphers ) = 0;

	 /**
	 * Cancels all outstanding operations. 
	 * This method will cancel all outstanding operations with the exception of Shutdown, 
	 * which cannot be canceled once started. 
	 * See individual implementation notes for behaviour after canceling. */
	virtual void CancelAll() = 0;

	 /**
	 * Cancels an outstanding handshake operation. 
	 * This method is used to cancel the StartClientHandshake, StartServerHandshake and 
	 * RenegociateHandshake operations. 
	 * See individual implementation notes for behaviour after canceling.*/
	virtual void CancelHandshake() = 0;

	 /** 
	 * Cancels any outstanding read operation.
	 * See individual implementation notes for behaviour after canceling. */
	virtual void CancelRecv() = 0;

	 /** 
	 * Cancels any outstanding send operation. 
	 * See individual implementation notes for behaviour after canceling. */
	virtual void CancelSend() = 0;

	 /**
	 * Gets the current client certificate.
	 *
	 * When a secure socket is acting in server mode, the returned certificate will be the certificate that the remote
	 * client provided. 
	 * When acting in client mode, the certificate returned will be the one that the client will send to the 
	 * remote server if requested. 
	 * 
 	 * Note that if there is no client certificate defined, either in server or client mode, 
	 * this method will return NULL.
	 *
	 * @return A pointer to the client certificate, or NULL if none exists.*/
	virtual const CX509Certificate* ClientCert() = 0;

	 /** 
	 * Returns the current client certificate mode.
	 * The client certificate mode is used when the socket is acting as a server, and determines if a 
	 * client certificate is requested.
	 * @see TClientCertMode for details of each mode.
	 * @return TClientCertMode The current mode that is set. */
	virtual TClientCertMode ClientCertMode() = 0; 

	 /** 
	 * Closes the secure connection.
	 * Implementations should terminate the secure connection gracefully as appropriate to their protocol. 
	 * It is assumed that they also close the socket when finished unless explicitly stated. They MUST NOT 
	 * destroy the RSocket object, this is left to the client application.
	 */
	virtual void Close() = 0;

	 /**
	 * Gets the current cipher suite in use.
	 * The current cipher suite is returned in the referenced buffer.
	 * 
	 * Note that it is assumed that implementations return cipher suites in two byte format 
	 * as is the case with the TLS/SSL protocols, i.e. [0x??][0x??]. 
	 * Implementations should specify if they differ.
	 *
	 * @param aCipherSuite		A reference to a descriptor at least 2 bytes long, 
								implementations that differ from the [0x??][0x??] format may 
								require larger descriptors. See individual implementations 
								notes for details. 
	 * @return					Any one of the system error codes, or KErrNone on success. */
	virtual TInt CurrentCipherSuite( TDes8& aCipherSuite ) = 0;

	 /**
	 * Gets the current dialog mode.
	 * @see		TDialogMode for description of valid modes.
	 * @return	TDialogMode The current dialog mode. */
	virtual TDialogMode	DialogMode() = 0; 

	 /** 
	 * Flushes the session cache.
	 *
	 * If protocols implement a session cache, this method will cause that cache to be flushed. */
	virtual void FlushSessionCache() = 0;

	 /**
	 * Gets an option.
	 *
	 * SecureSocket implementations may provide options that can be read with this method.
	 * See individual implementation notes for details.
	 *
	 * @param aOptionName	An integer constant which identifies an option.	 
	 * @param aOptionLevel	An integer constant which identifies level of an option.	 
	 * @param aOption		Option value packaged in a descriptor.
	 * @return				KErrNone if successful, otherwise another of the system-wide error codes. */
	virtual TInt GetOpt(TUint aOptionName,TUint aOptionLevel,TDes8& aOption) = 0;

	 /**
	 * Gets an option.
	 *
	 * Secure socket implementations may provide options that can be read with this method.
	 * See individual implementation notes for details.
	 *
	 * @param aOptionName	An integer constant which identifies an option.
	 * @param aOptionLevel	An integer constant which identifies level of an option.	 
	 * @param aOption		Option value as an integer.
	 * @return				KErrNone if successful, otherwise another of the system-wide error codes. */
	virtual TInt GetOpt(TUint aOptionName,TUint aOptionLevel,TInt& aOption) = 0;

	 /**
	 * Get the protocol in use.
	 *
	 * This method can be used to return the particular protocol/version that is being 
	 * used by implementations that support different protocols/versions.
	 * See individual implementation notes for details.
	 *
	 * @param aProtocol		A descriptor containing the protocol name/version that is being 
							used. Protocol names can be upto 32 characters long, and so a 
							descriptor of at least that size is required.
	 * @return			KErrNone if successful; otherwise, another of the system-wide error codes. */
	virtual TInt Protocol(TDes& aProtocol) = 0;

	 /**
	 * Receives data from the socket.
	 *
	 * This is an asynchronous method, and will complete when the descriptor has been filled. 
	 * Only one Recv or RecvOneOrMore operation can be outstanding at any time.
	 *
	 * @param aDesc		A descriptor where data read will be placed.
	 * @param aStatus	On completion, will contain an error code: see the system-wide error 
						codes. Note that KErrEof indicates that a remote connection is 
						closed, and that no more data is available for reading. */
	virtual void Recv(TDes8& aDesc, TRequestStatus & aStatus) = 0;

	 /** 
	 * Receives data from the socket. 
	 *
	 * This is an asynchronous call, and will complete when at least one byte has been read.
	 * Only one Recv or RecvOneOrMore operation can be outstanding at any time.
	 *
	 * @param aDesc		A descriptor where data read will be placed.
	 * @param aStatus	On completion, will contain an error code: see the system-wide error 
	 *					codes. Note that KErrEof indicates that a remote connection is closed,
     *					and that no more data is available for reading.
	 * @param aLen		On return, a length which indicates how much data was read. 
	 *					This is the same as the length of the returned aDesc. */
	virtual void RecvOneOrMore(TDes8& aDesc, TRequestStatus& aStatus, TSockXfrLength& aLen) = 0;

	 /**
	 * Initiates a renegotiation of the secure connection.
	 *
	 * This is an asynchronous method that completes when renegotiation is complete. 
	 * It is valid for both client and server operation. 
	 * There can only be one outstanding RenegotiateHandshake operation at a time.
	 *
	 * @param aStatus	On completion, will contain an error code: see the system-wide error 
						codes. */
	virtual void RenegotiateHandshake(TRequestStatus& aStatus) = 0;

	 /** 
	 * Send data over the socket. 
	 *
	 * This is an asynchronous call. Only one Send operation can be outstanding at any time. 
	 * @param aDesc		A constant descriptor containing the data to be sent.
	 * @param aStatus	On completion, will contain an error code: see the system-wide error 
						codes. */
	virtual void Send(const TDesC8& aDesc, TRequestStatus& aStatus) = 0;

	 /** 
	 * Send data over the socket.
	 *
	 * This is an asynchronous call. Only one Send operation can be outstanding at any time.
	 *
	 * @param aDesc		A constant descriptor.
	 * @param aStatus	On completion, will contain an error code: see the system-wide error 
	 *					codes. 
	 * @param aLen		Filled in with amount of data sent before completion */
	virtual void Send(const TDesC8& aDesc, TRequestStatus& aStatus, TSockXfrLength& aLen) = 0;

	 /**
	 * Gets the current server certificate.
	 *
	 * When a secure socket is acting in client mode, the returned certificate will be the 
	 * certificate for the remote server.
	 * When acting in server mode, the certificate returned will be the one that is being 
	 * used as the server certificate.
	 *
	 * @return CX509Certificate A pointer to the certificate. */ 
	virtual const CX509Certificate* ServerCert() = 0;

	 /** 
	 * Sets a list of cipher suites that are available to use. 
	 *
	 * It is assumed that implementations require a list of cipher suites supplied in a descriptor in two
	 * byte format as is the case with the TLS/SSL protocols, i.e. [0x??][0x??]. 
	 * It is also assumed that the order of suites is important, and so they should be listed
	 * with the preferred suites first. 
	 * Implementations should specify if they differ.
	 *
	 * @param aCiphers	A descriptor containing the list or ciphers suites to use. 
	 * @return			Any one of the system error codes, or KErrNone on success. */
	virtual TInt SetAvailableCipherSuites(const TDesC8& aCiphers) = 0;

	 /**
	 * Sets the client certificate to use.
	 *
	 * When a secure socket is acting in client mode, this method will set the certificate 
	 * that will be used if a server requests one.
	 * When acting in server mode, this method will perform no action, but will return KErrNotSupported.
	 * @param aCert		A reference to the certificate to use.
	 * @return			Any one of the system error codes, or KErrNone on success. */
	virtual TInt SetClientCert(const CX509Certificate& aCert) = 0;

	 /** 
	 * Set the client certificate mode. 
	 *
	 * When a secure socket is acting in server mode, the client certificate mode determines
	 * if clients will be requested to provide a certificate.
	 * When acting in client mode, this method will perform no action, but will return KErrNotSupported.
	 *
	 * @see TClientCertMode for details of each available mode.
	 * @param aClientCertMode	The client certificate mode to use.
	 * @return					Any one of the system error codes, or KErrNone on success. */
	virtual TInt SetClientCertMode(const TClientCertMode aClientCertMode) = 0;

	 /**
	 * Set the untrusted certificate dialog mode.
	 *
	 * Determines if a dialog is displayed when an untrusted certificate is received.
	 *
	 * @see TDialogMode for details of each available mode.
	 * @param aDialogMode	The dialog mode to use.
	 * @return				Any one of the system error codes, or KErrNone on success. */
	virtual TInt SetDialogMode(const TDialogMode aDialogMode) = 0;

	 /** 
	 * Sets a socket option. 
	 *
	 * Secure socket implementations may provide options that can be set with this method.
	 * See individual implementation notes for details.
	 *
	 * @param aOptionName	An integer constant which identifies an option.
	 * @param aOptionLevel	An integer constant which identifies level of an option: 
	 *						i.e. an option level groups related options together.
	 * @param aOption		Option value packaged in a descriptor
	 * @return				Any one of the system error codes, or KErrNone on success. */
	virtual TInt SetOpt(TUint aOptionName,TUint aOptionLevel, const TDesC8& aOption=KNullDesC8()) = 0;
																					   
	 /** 
	 * Sets an option.
	 *
	 * SecureSocket implementations may provide options that can be set with this method.
	 * See individual implementation notes for details.
	 *
	 * @param aOptionName	An integer constant which identifies an option.
	 * @param aOptionLevel	An integer constant which identifies level of an option: 
	 *						i.e. an option level groups related options together.
	 * @param aOption		Option value as an integer
	 * @return				Any one of the system error codes, or KErrNone on success. */
	virtual TInt SetOpt(TUint aOptionName,TUint aOptionLevel,TInt anOption) = 0;

	 /**
	 * Set a specific protocol/version to use.
	 *
	 * This method can be used to select a particular protocol version to use in 
	 * implementations that support different protocols/versions.
	 * See individual implementation notes for details.
	 *
	 * @param aProtocol A reference to a descriptor containing the protocol name/version to use.
	 * @return			KErrNone if successful, a system-wide error code if not. */
	virtual TInt SetProtocol(const TDesC& aProtocol) = 0;

	 /**
	 * Set the server certificate.
	 *
	 * When acting in server mode, this method will set the certificate that is to be used 
	 * as the server certificate.
	 * When acting in client mode, this method will perform no action, but will return KErrNotSupported.
	 *
	 * @param aCert The certificate to use.
	 * @return		Any one of the system error codes, or KErrNone on success. */
	virtual TInt SetServerCert(const CX509Certificate& aCert) = 0;

	 /**
	 * Start acting as a client and initiate a handshake with the remote server.
	 *
	 * This is an asynchronous call, and will only complete when the handshake completes 
	 * and the secure connection is established, or it fails.
	 *
	 * @param aStatus On completion, any one of the system error codes, or KErrNone on success. */
	virtual void StartClientHandshake(TRequestStatus& aStatus) = 0;

	 /**
	 * Start acting as a server and listen for a handshake from the remote client.
	 *
	 * This is an asynchronous call, and will only complete when a client completes the 
	 * handshake, or if it fails. Normally, the socket passed in will usually have been 
	 * previously used in a call to Accept() on a listening socket, but this is not required. 
	 *
	 * @param aStatus On completion, any one of the system error codes, or KErrNone on success. */
	virtual void StartServerHandshake(TRequestStatus& aStatus) = 0;

	/** Standard destructor. */
	virtual ~MSecureSocket() {};

	};

#endif // __SECURESOCKETINTERFACE_H__

