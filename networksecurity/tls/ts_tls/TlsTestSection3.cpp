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
// This contains TLS test section 3
// 
//

// Test system includes
#include "T_TLS_test.h"
#include "TlsTestSection3.h"
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <securesocket_internal.h>
#endif

CTlsTestSection3_1::CTlsTestSection3_1()
/**
 * Constructor.
 * Stores the name of this test case.
 */
{
	iTestStepName = _L("tls_TestSection3_1");
}


CTlsTestSection3_1::~CTlsTestSection3_1()
/**
 * Destructor.
 */
{
}


TVerdict CTlsTestSection3_1::doTestStepL( )
{
	Log(_L("SSL Socket options test"));

	__UHEAP_MARK;
	// assume that everything passed, unless we leave
	iTestStepResult = EPass;
	TInt error(KErrNone);
    TInt recvBufSize(-1), sendBufSize(-1);
	TInt noDelayEnabled(-1);

	// Create an active scheduler
	CActiveScheduler* myActiveScheduler = new(ELeave) CActiveScheduler();
	CleanupStack::PushL( myActiveScheduler );
	CActiveScheduler::Install( myActiveScheduler );

	// Connect the socket server
	RSocketServ socketServ;
	error = socketServ.Connect();
	TESTEL(error == KErrNone, error);
	CleanupClosePushL(socketServ);

	// Open a socket
	RSocket socket;
	error = socket.Open( socketServ, KAfInet, KSockStream, KProtocolInetTcp );
	TESTEL(error == KErrNone, error);
	CleanupClosePushL(socket);

	Log(_L("Creating a secure socket"));
	CSecureSocket* secureSocket = NULL;
	secureSocket = CSecureSocket::NewL( socket, KDefCfgProtocol );

	// Now do sSetOpt's and GetOpt's at 
	Log(_L("Setting KSORecvBuf to 1000"));
    error = secureSocket->SetOpt(KSORecvBuf, KSOLSocket, 1000);
    TESTEL(error == KErrNone, error);

	Log(_L("Checking KSORecvBuf is set to 1000"));
    error = secureSocket->GetOpt(KSORecvBuf, KSOLSocket, recvBufSize);
    TESTEL(error == KErrNone, error);
    TESTEL(recvBufSize == 1000, recvBufSize);

    error = secureSocket->GetOpt(KSOSendBuf, KSOLSocket, sendBufSize);
    TESTEL(error == KErrNone, error);
	Log(_L("Read sendbuf size of %d"), sendBufSize);

	Log(_L("Read KSoTcpNoDelay of %d"), noDelayEnabled);
    error = secureSocket->GetOpt(KSoTcpNoDelay, KSolInetTcp, noDelayEnabled);
    TESTEL(error == KErrNone, error);

    error = secureSocket->SetOpt(KSoTcpNoDelay, KSolInetTcp, !noDelayEnabled);
    TESTEL(error == KErrNone, error);

	TDialogMode dialogMode = secureSocket->DialogMode();
	Log( _L("Dialog mode setting is: %d"), dialogMode );			

    error = secureSocket->SetDialogMode(EDialogModeUnattended);
    TESTEL(error == KErrNone, error);
    TESTL(secureSocket->DialogMode()==EDialogModeUnattended);
	Log( _L("Dialog mode setting is: %d"), EDialogModeUnattended );			

    error = secureSocket->SetDialogMode(dialogMode);
    TESTEL(error == KErrNone, error);
    TESTL(secureSocket->DialogMode()==dialogMode);
	Log( _L("Dialog mode reset to: %d"), dialogMode );			

	secureSocket->Close();
	delete secureSocket;
	secureSocket = NULL;

	CleanupStack::PopAndDestroy(); // socket
	CleanupStack::PopAndDestroy(); // socketServ

	CActiveScheduler::Install( NULL );
	CleanupStack::PopAndDestroy( myActiveScheduler );

	// unload the securesocket library
	CSecureSocketLibraryLoader::Unload();
	
	__UHEAP_MARKEND;

	return iTestStepResult;
}
