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

#ifndef __SECENGINE_H__
#define __SECENGINE_H__

#include <e32cons.h>
#include <c32comm.h>
#include <in_sock.h>
#include <securesocket.h>
#include <x509cert.h>

/** Number of chars allowed for address & page settings */
const TInt KSettingFieldWidth = 128;

/** Connection settings to access a server  */
struct TConnectSettings
	{
	/** Server address (as text) */
	TBuf<KSettingFieldWidth> iAddress;
	/** Server port */
	TInt iPortNum;
	/** Web page to get from the server */
	TBuf8<KSettingFieldWidth> iPage;
	};

/**
	Manages connection to a SSL web server.
*/
class CSecEngine : public CActive
	{
public:
/**
Allocates and constructs a new engine.
@return New object
*/
	static CSecEngine *NewL();
/**
Destructor.
*/
	~CSecEngine();
/**
Initiates the connection to a server and the transaction

	@param aAddress Server address (e.g. www.symbian.com or dotted decimal format)

	@param aPortNum Server port for secure web (e.g. 443)

	@param aPage Web page to get from the server. The leading `/` should be included, e.g. /webpage.html.

	@param aCipherSuite Cipher suites that client will tell server it supports (decimal). This 
	should be in decimal, with 2 characters, ie for suites 3,7,8,9, this field would be 03070809.
	By entering a single `0` for this field, the SSL default cipher suites will be passed.

	@param aCipher Cipher suite that server is expected to use (decimal). This is compared with 
	actual cipher for pass/fail. If this field is 0, no comparisons with the actual cipher 
	suite used will be made.

*/
	void ConnectL( const TConnectSettings& aConnectSettings );
/**
Sets the console to write messages to

@param aConsole The console
*/
	void SetConsole( CConsoleBase& aConsole );

/**
Sets the (opened) file to write server response to

@param aOutputFile The file
*/
	void SetOutputFile( RFile& aOutputFile );

/**
Tests if the connection is in progress.

@return True if in progress else false
*/
	TBool InUse();

private:
	/** Engine states */
	enum TStates  
		{
		/** IP connection initiated */
		ESocketConnected,
		/** Setting the ciphers for a secure connection */
		ESettingCiphers,
		/** Secure socket request initiated */
		ESecureConnected,
		/** Send get page request to server */
		EGetRequestSent,
		/** Server has responded to request */
		EDataReceived,
		/** Connection closed down */
		EConnectionClosed
		};
	
private:
	/** Constructor. */
	CSecEngine();	 
	/** Second phase constructor. */
	void ConstructL();

	// Methods from CActive
	/** Previous state has completed. */
	void RunL();
	/** Cancel request */
	void DoCancel();
	/** Handles a leave occurring in RunL(). */
	TInt RunError( TInt aError );
	
	// Handle particular engine states
	/** Attempts secure connection. */
	void MakeSecureConnectionL();
	/** Sends page request to server */	
	void MakePageRequestL();
	/** Start getting server's response to the request */
	void GetServerResponseL();
	/** Finish getting server's response */
	void ReadServerResponseL();
	/** Handle connection completed */
	void ConnectionClosed();

	/** Prints the name of the indicated cipher */
	void PrintCipherNameL(const TDes8& aBuf);
	/** Prints information about the server's cetificate */
	void PrintCertInfo(const CX509Certificate& aSource);

private:
	// Sockets objects
	/** The socket server */
	RSocketServ iSocketServ;
	/** Socket to make connection on */
	RSocket iSocket;
	/** For resolving DNS addresses */
	RHostResolver iHostResolver;
	/** Server address */
	TInetAddr iInetAddr;
	/** The secure socket interface */
	CSecureSocket* iTlsSocket;

	// Connection parameters
	const TConnectSettings* iConnectSettings;

	// Transfer buffers and counters
	/** Data sent buffer */
	TPtr8 iSndBuffer;
	/** #bytes sent */
	TSockXfrLength iBytesSent;
	/** Data received buffer */
	TPtr8 iRcvBuffer;
	/** #bytes received */
	TInt iTotalBytesRead;

	/** For retries, after a delay */
	RTimer iTimer;

	/** Output console */
	CConsoleBase *iConsole;
	/** Output file */
	RFile* iOutputFile;

	// Flags and state
	/** True if the transation completed successfully */
	TBool		iSuccess;
	/** True if success on first attempt */
	TBool	iFirstRunFlag;
	/** True if connection is in progress */
	TBool	iInUse;
	/** Engine state (a TStates value) */
	TInt	iRunState;
	
	/** Counter for stopping making connections */
	TInt iCounter;
	
	RFs iFs;
	};

 /**
	Watchdog timer to cancel any operation after timeout
*/
class CTlsWatchdog : public CTimer
	{
public:
	/** Constructor. */
	CTlsWatchdog(CSecEngine* aEngine, CConsoleBase* aConsole);
		 
	
	/**
	Destructor.
	*/
	~CTlsWatchdog();
	

private:

	// Methods from CActive
	void RunL();
	
private:
	CSecEngine* iEngine;
	CConsoleBase* iConsole;
	};

#endif
