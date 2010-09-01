// Copyright (c) 2003-2010 Nokia Corporation and/or its subsidiary(-ies).
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
 
#ifndef __T_TLS_TEST_H__
#define __T_TLS_TEST_H__

#include <e32base.h>
#include <e32cons.h>
#include <c32comm.h>
#include <es_sock.h>
#include <in_sock.h>
#include <ssl.h>
#include <x509cert.h>
#include <securesocketinterface.h>
#include <networking/log.h>
#include <networking/teststep.h>

#include "TeststepTls.h"

#include "T_HTTPOB.H"
#include "T_Autossl_Const.h"

#include <securesocket.h>
#include <genericsecuresocket.h>


//
// string constants
//

// State error message text
_LIT( KStateErrConnected, "Error in state: EConnected:%d" );
_LIT( KStateErrReceivePage, "Error in state: EReceivePage:%d" );
_LIT( KStateErrFinished, "Error in state: EFinished:%d" );
_LIT( KLogBytesRead,		"Read %d bytes from socket\n" );
_LIT( KLogSendingRequest,	"Sending request:" );
_LIT( KLogTestPassed,		"[Success]" );
_LIT( KLogTestFailed,		"[FAILED]" );

class CTLSTest : public CActive
	{
private:
	enum TSSLTestRunStates  
		{
  		ESocketConnected,
		ESecureConnected,
		EGetRequestSent,
		EDataReceived,
		ESecureRenegotiated,
		ESecureConnectedReneg,
		EGetRequestSentReneg,
		EDataReceivedReneg,
		EConnectionClosed,
		EDummyConnection
		};

public:
	// Construct/destruct
	static CTLSTest *NewL();
	~CTLSTest();
#ifdef HTTP_ALLOW_UNTRUSTED_CERTIFICATES
	void ConnectL( const TDesC &aAddress, 
			const TInt aPortNum, 
			const TDesC &aPage, 
			const TDesC8 &aCipherSuite, 
			const TInt aCipher, 
			const TInt aSimpleGet, 
			const TInt aTestEndDelay, 
			const TDesC8& aDNSName, 
			const TDesC& aProtocol, 
			TBool aUseGenericSocket, 
			TBool aEAPKeyDerivation,
			TBool aTLSDialogMode,
			TInt aTLSDialogModeValue,
			TInt aExpectedErrorCode );
#else
	void ConnectL( const TDesC &aAddress, 
	            const TInt aPortNum, 
	            const TDesC &aPage, 
	            const TDesC8 &aCipherSuite, 
	            const TInt aCipher, 
	            const TInt aSimpleGet, 
	            const TInt aTestEndDelay, 
	            const TDesC8& aDNSName, 
	            const TDesC& aProtocol, 
	            TBool aUseGenericSocket, 
	            TBool aEAPKeyDerivation );
#endif  // HTTP_ALLOW_UNTRUSTED_CERTIFICATES

	void SetConsole( CTestStepTls * aTestStep );
	TBool InUse();
	TBool TestingSite( const TDesC &aAddress, const TInt aPortNum );
	TInt RunError( TInt aError );
	void HexDump(const TText* aHeader, const TText* aMargin, const TUint8* aPtr, TInt aLen);

	TInt KeyDerivationTests(TBool aSocketOpen);
	
private:
	// Construction
	CTLSTest();	 
	void ConstructL();

	// Methods from CActive
	void RunL();
	void DoCancel();
	
	RSocketServ iSocketServ;
	RSocket iSocket;
	RTimer iTimer;
	RHostResolver iHostResolver;
	TInetAddr iInetAddr;
	CTestStepTls * iTestStep;
	TBuf<128>	iAddress;
	TBuf8<256>	iDNSName;
	TInt		iPortNum;
	TInt		iCipher;
	TBuf8<KCipherBufSize>	iCipherSuites;
	TBuf8<128>	iPage;
	TBuf<32>	iProtocol;
	TBuf8<2>	iCipherBuf;
	TInt		iSimpleGet;
	TInt		iTestEndDelay;
	TBool		iTestPassed;		// true if the test completed successfully
	TBool		iCiphersMatch;		// true if expected cipher was selected by the server
	TBool		iUseGenericSocket;
	TBool		iEAPKeyDerivation;
#ifdef HTTP_ALLOW_UNTRUSTED_CERTIFICATES
	TBool      iTLSDialogMode;
	TInt       iTLSDialogModeValue;
	TInt       iExpectedErrorCode;
#endif  // HTTP_ALLOW_UNTRUSTED_CERTIFICATES
	TSockXfrLength iBytesRead;
	TBuf8<5000> iRcvBuffer;
	TBuf8<256>	iSndBuffer;
	TSockXfrLength iBytesSent;
	TInt		iTotalBytesRead;

	TBool	iFirstRunFlag;		
	TBool	iInUse;
	TInt	iRunState;
	
	CSecureSocket* iTlsSocket;	 
	CSecureSocket* iTlsSocket2;	 
	CGenericSecureSocket<RSocket>* iGenericSocket;

	TInt GetCertInfo(const CX509Certificate&);
	};

#endif
