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
// Secure Sockets
// 
//

/**
 @file
*/

#ifndef __SECURESOCKET_H__
#define __SECURESOCKET_H__

#include <e32base.h>
#include <e32cons.h>
#include <c32comm.h>
#include <es_sock.h>
#include <ssl.h>

#include <sslerr.h>
#include <x509cert.h>

#include <securesocketinterface.h>
#ifndef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <securesocket_internal.h>
#endif

class MGenericSecureSocket;

class CSecureSocket : public CBase
/** 
 * Secure sockets class. 
 * 
 * @publishedAll
 * @released
 *
 * @since v6.2 */
 // New secure sockets can be created through the static CSecureSocket::NewL method.
	{
public:
	IMPORT_C static CSecureSocket* NewL(RSocket& aSocket,const TDesC& aProtocol);
	IMPORT_C static CSecureSocket* NewL(MGenericSecureSocket& aSocket,const TDesC& aProtocol);

	/** Standard destructor. */
	~CSecureSocket();
 
	// export CSecureSocket methods
	IMPORT_C TInt AvailableCipherSuites( TDes8& aCiphers );
	IMPORT_C void CancelAll();
	IMPORT_C void CancelHandshake();	
	IMPORT_C void CancelRecv();
	IMPORT_C void CancelSend();
	IMPORT_C const CX509Certificate* ClientCert();
	IMPORT_C TClientCertMode ClientCertMode();
	IMPORT_C TDialogMode DialogMode();
	IMPORT_C void Close();
	IMPORT_C TInt CurrentCipherSuite( TDes8& aCipherSuite );
	IMPORT_C void FlushSessionCache();
	IMPORT_C TInt GetOpt(TUint aOptionName, TUint aOptionLevel, TDes8& aOption);
    IMPORT_C TInt GetOpt(TUint aOptionName, TUint aOptionLevel, TInt& aOption);
	IMPORT_C TInt Protocol(TDes& aProtocol);
	IMPORT_C void Recv (TDes8& aDesc, TRequestStatus& aStatus );
	IMPORT_C void RecvOneOrMore( TDes8& aDesc, TRequestStatus& aStatus, TSockXfrLength& aLen );
	IMPORT_C void RenegotiateHandshake(TRequestStatus& aStatus );
	IMPORT_C void Send( const TDesC8& aDesc, TRequestStatus& aStatus, TSockXfrLength& aLen );
	IMPORT_C void Send( const TDesC8& aDesc, TRequestStatus& aStatus );
	IMPORT_C const CX509Certificate* ServerCert();
	IMPORT_C TInt SetAvailableCipherSuites(const TDesC8& aCiphers);
	IMPORT_C TInt SetClientCert(const CX509Certificate& aCert);
	IMPORT_C TInt SetClientCertMode(const TClientCertMode aClientCertMode);
	IMPORT_C TInt SetDialogMode(const TDialogMode aDialogMode);
	IMPORT_C TInt SetProtocol(const TDesC& aProtocol);
	IMPORT_C TInt SetOpt(TUint aOptionName, TUint aOptionLevel, const TDesC8& aOption=TPtrC8(NULL,0));
	IMPORT_C TInt SetOpt(TUint aOptionName, TUint aOptionLevel, TInt aOption);
	IMPORT_C TInt SetServerCert(const CX509Certificate& aCert);	
	IMPORT_C void StartClientHandshake(TRequestStatus& aStatus);
	IMPORT_C void StartServerHandshake(TRequestStatus& aStatus);	

private:
	void ConstructL(RSocket& aSocket,const TDesC& aProtocol);
	void ConstructL(MGenericSecureSocket& aSocket,const TDesC& aProtocol);

	enum {ESecureSocketStateOpen, ESecureSocketStateClosed};

	TUint iSecureSocketState;

	TInt iUNUSED;
	MSecureSocket* iSecureImplementation;
	};

#endif // __SECURESOCKET_H__
