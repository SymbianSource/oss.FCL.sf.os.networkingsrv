// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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

/**
 @file T_TLS_PSK_Test.h
 @internalTechnology	
*/
#ifndef _T_TLS_PSK_H
#define _T_TLS_PSK_H

#include <e32base.h>
#include <tlstypedef.h>
#include <securesocket.h>
#include <es_sock.h> 
#include <in_sock.h> 
#include "T_TLS_cntrl.h"
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <ssl_internal.h>
#endif

_LIT(KUseNullCipher,"UseNullCipher");
_LIT(KUsePsk,"UsePsk");
_LIT(KNumCipherSuites, "NumCipherSuites");
_LIT(KCipherSuiteBase, "CipherSuite");
_LIT(KHandShakeTestStep, "TlsPsk");
_LIT(KClientCert, "ClientCert");
_LIT(KWebAddress,"WebAddress");
_LIT(KWebPage,"WebPage");
_LIT(KPortNum,"PortNum");
_LIT(KExpectedFinalCipher, "ExpectedFinalCipherSuit");
_LIT(KExpectedSetCipherError, "ExpectedSetCipherError");
_LIT(KExpectedHandshakeError, "ExpectedHandshakeError");
_LIT8(KSimpleGet, "GET /");
_LIT8(KGetTail, " HTTP/1.0\n");
_LIT8(KExpectedPageContent, "Hello world");
_LIT8(KPSK_IDENTITY, "Client_identity");
_LIT8(KPSK_KEY, "0123456789"); 
_LIT(KNumServerNames, "NumServerNames");
_LIT(KServerNameBase, "ServerName");

/** Number of chars allowed for address & page settings */
const TInt KSettingFieldWidth = 128;

// Send buffer size
const TInt KSendBufferSize = 256;

// Receive buffer size
const TInt KReceiveBufferSize = 256;

// Max server name size, limited by test server
const TInt KMaxServerNameLength = 255;

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

// forward declarations:
class CTlsPskTestStep;

class CTlsPskTest : public CActive
	{
	public:
	CTlsPskTest();
	~CTlsPskTest();
	
	//Active
	void DoCancel()	{return; };
	TVerdict		DoSecureConnectionTestL(CTlsPskTestStep* aStep);
	TInt			HandShakeL();
	TInt			MakePageRequest();
	TInt			GetPageReceived();
	TInt			VerifyPageReceived();
	TInt			VerifyFinalCipherUsed();
	void			RunL();
	virtual TInt	RunError(TInt aError);
	// Generic

	public:
	CTlsPskTestStep* 	        iStepPointer;
	TVerdict                    iTestSuccess;
	TInt						iRunError;
	
	enum EState
		{
		EIdle,
		// Main test
		ESecureConnected,
		EGetRequestSent,
		EGetPageReceived
		};  
	EState	iState;
	
	} ;

class CTlsPskTestStep : public CTestStepTls, public MSoPskKeyHandler
	{

	public:
	CTlsPskTestStep();
	~CTlsPskTestStep();
	
	TVerdict doTestStepPreambleL();
	TVerdict doTestStepL();
	TVerdict doTestL();
	void     ConnectL();
	HBufC8*  ReadCipherSuitesL();
	CDesC8ArrayFlat*  ReadServerNamesL();	
	void     SetPskL();
	void     SetNullCipherL();
	void     CloseConnection();
	TBool GetHexFromConfig(const TDesC& aSectName,const TDesC& aKeyName,TInt& aResult);
	void LogEvent( const TDesC& aMessage );

	virtual void GetPskL(const HBufC8 *aPskIdentityHint, HBufC8 *&aPskIdentity, HBufC8 *&aPskKey);	
    	
	CTlsPskTest* 		iActiveObjTest;
	CActiveScheduler* iSched;
    
	// Connection parameters
	TConnectSettings iConnectSettings;
	// The Secure socket
	CSecureSocket* iTlsSocket;
	// Socket to make connection on  
	RSocket iSocket;
	// Data sent buffer  
	RBuf8 iSndBuffer;
	// #bytes sent 
	TSockXfrLength iBytesSent;
	// Data received buffer  
	RBuf8 iRcvBuffer; 
	// Server address  
	TInetAddr iInetAddr;
	// The socket server  
	RSocketServ iSocketServ;
	// What is the cipher suit that is expected to be selected by web server
	TInt iExpectedFinalCipher;	
	// The cipher suits to be included in hello client message read from INI file
	HBufC8 *iBufCipherSuitesFromIni;
	// PSK optional.
	TBool iUsePsk;
	// Null cipher optional.
	TBool iUseNullCipher;
	// Expected set suit error is optional.
	TInt iExpectedSetSuitesError;
	// Server names are optional.
	TBool iUseServerNames;
	// Array with server names.
	CDesC8ArrayFlat *iServerNamesArray;
	// Expected handshake error is optional.
	 TInt iExpectedHandshakeError;
	};

#endif /* __T_TLS_PSK_H_ */
