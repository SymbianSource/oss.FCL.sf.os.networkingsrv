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
 @file ciphersuitestep.h
 @internalTechnology	
*/
#ifndef __TLSHANDSHAKESTEP_H__
#define __TLSHANDSHAKESTEP_H__

#include <e32base.h>
#include <tlstypedef.h>
#include <securesocket.h>
#include <es_sock.h> 
#include <in_sock.h> 
#include <testexecutestepbase.h>
#include "ciphersuitesstep.h"
#include <ssl.h>
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <ssl_internal.h>
#endif

_LIT(KHandShakeTestStep, "HandShakeStep");
_LIT(KClientCert, "ClientCert");

/** Number of chars allowed for address & page settings */
const TInt KSettingFieldWidth = 128;

// Send buffer size
const TInt KSendBufferSize = 256;
// Receive buffer size
const TInt KReceiveBufferSize = 512;
// Max server name size, limited by test server
const TInt KMaxServerNameLength = 255;

// forward declarations:
class CHandShakeStep;

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

class CHandShakeTesterActive : public CActive
	{
	public:
	CHandShakeTesterActive( CTestExecuteLogger& aLogger );
	~CHandShakeTesterActive();
	
	//Active
	void DoCancel()	{return; };
	TVerdict		DoSecureConnectionTestL(CHandShakeStep* aStep);
	TInt			HandShakeL();
	TInt			MakePageRequest();
	void			GetPageReceivedL();
	TInt			VerifyPageReceived();
	TInt			VerifyFinalCipherUsed();
	void 			OutputPageToFileL(const TDesC8& aPageReceived);
	void			RunL();
	virtual TInt	RunError(TInt aError);
	// Generic
	
	public:
	
	CTestExecuteLogger& Logger(){return iLogger;}
	CTestExecuteLogger& iLogger;
	
	CHandShakeStep* 	        iStepPointer;
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

class CHandShakeStep : public CTestStep, public MSoPskKeyHandler
	{
	public:
	CHandShakeStep();
	
	TVerdict doTestStepPreambleL();
	TVerdict doTestStepL();
	TVerdict doTestL();
	void     ConnectL();
	HBufC8*  ReadCipherSuitesL();
	CDesC8ArrayFlat*  ReadServerNamesL();	
	void     SetPskL();
	void     SetNullCipherL();
	void     CloseConnection();

	virtual void GetPskL(const HBufC8 *aPskIdentityHint, HBufC8 *&aPskIdentity, HBufC8 *&aPskKey);	

	~CHandShakeStep();   
	
	CHandShakeTesterActive* 		iActiveObjTest;
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
	// Data received buffer (single packet) 
	RBuf8 iRcvBuffer;
	// Page received (made by appending packets)
	RBuf8 iRcvPage; 
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

#endif /* __TLSHANDSHAKESTEP_H__ */
