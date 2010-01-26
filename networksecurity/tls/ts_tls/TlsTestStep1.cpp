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
//

// EPOC includes
#include <e32base.h>

#include <e32cons.h>
#include <c32comm.h>
#include <f32file.h>
#include <es_sock.h>
#include <securesocketinterface.h>
#include <securesocket.h>

#include "T_TLS_test.h"

#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <ssl_internal.h>
#endif


// Test system includes
#include <networking/log.h>
#include <networking/teststep.h>

#include "TestSuiteTls.h"
#include "TlsTestStep1.h"

#include "T_TLS_cntrl.h"


#include "tlsconnection.h"

GLREF_C void CommInitL();

_LIT( KTxtTLS, "T_TLS" );

CTestStepT_Tls::CTestStepT_Tls()
/**
 * Constructor.
 * Store the name of this test case
 */
{
	iTestStepName = _L("t_tls");
}

// destructor
CTestStepT_Tls::~CTestStepT_Tls()
{
}


TVerdict CTestStepT_Tls::doTestStepL( )
/**
 * This is the test code for t_tls.
 */
{
	// if the test has not left yet it must be a Pass 
	iTestStepResult = EPass;

	TRAPD(error,CommInitL()); // init needed comms libs
	__ASSERT_ALWAYS(!error,User::Panic(KTxtTLS,error));

	TRAP(error,TLSTestL()); // more initialization, then do example
	__ASSERT_ALWAYS(!error,User::Panic(KTxtTLS,error));

	return iTestStepResult;
}


// constructor
CTlsRenegotiateTest::CTlsRenegotiateTest()
{
	// store the name of this test case
	iTestStepName = _L("renegotiate");

	iTestType = TLS_TEST_RENEGOTIATE;
}

TVerdict CTlsRenegotiateTest::doTestStepL( )
{
	return CTestStepT_Tls::doTestStepL(  );
}

// constructor
CTlsCancelRecvTest::CTlsCancelRecvTest()
/**
 * Store the name of this test case.
 */
{
	iTestStepName = _L("CancelRecv");

	iTestType = TLS_TEST_CANCEL_RECV;
}

TVerdict CTlsCancelRecvTest::doTestStepL( )
{
	return CTestStepT_Tls::doTestStepL(  );
}


CTlsOldGetOptsTest::CTlsOldGetOptsTest()
/**
 * Constructor.
 * Store the name of this test case.
 */
{
	iTestStepName = _L("oldgetOpts");

	iTestType = TLS_TEST_OLD_GETOPTS;
}

TVerdict CTlsOldGetOptsTest::doTestStepL( )
{
	return CTestStepT_Tls::doTestStepL(  );
}

CTlsOpenConnection::CTlsOpenConnection()
/**
 * Constructor.
 * Store the name of this test case.
 */
{
	iTestStepName = _L("OpenConnection");
}

TVerdict CTlsOpenConnection::doTestStepL( )
{
	_LIT(KSSLProtocol,"tls1.0");
	TRequestStatus Status;

	TRAPD(error,CommInitL()); // init needed comms libs
	__ASSERT_ALWAYS(!error,User::Panic(KTxtTLS,error));

	// Create an active scheduler
	CActiveScheduler* myActiveScheduler;
	myActiveScheduler = new(ELeave) CActiveScheduler();
	CleanupStack::PushL( myActiveScheduler );
	CActiveScheduler::Install( myActiveScheduler );

	// get address and port number 
	TPtrC addr = iAddress;
	TESTL(GetStringFromConfig(KSectionName, KCfgIPAddress, addr));
	iAddress.Copy( addr );

	TESTL(GetIntFromConfig(KSectionName, KCfgIPPort, iPortNum));

	// Connect the socket server
	User::LeaveIfError( iTlsSuite->iSocketServer.Connect() );

	// Open the socket
	User::LeaveIfError( iInetAddr.Input( iAddress ));
	iInetAddr.SetPort( iPortNum );

	Log(_L("Connecting to %s:%d"), iAddress.PtrZ(), iPortNum );
	User::LeaveIfError( iTlsSuite->iSocket.Open( iTlsSuite->iSocketServer, KAfInet, KSockStream, KProtocolInetTcp ) );	

	// connect the socket
	iTlsSuite->iSocket.Connect( iInetAddr, Status );	
	User::WaitForRequest(Status);

	Log(_L("Connect result is %d"), Status.Int() );
	TESTEL(Status==KErrNone, Status.Int());

	User::LeaveIfNull(iTlsSuite->iSecureSocket = CSecureSocket::NewL( iTlsSuite->iSocket,KSSLProtocol()));

	// Remove objects from the cleanup stack
	CleanupStack::PopAndDestroy( 1 ); // myActiveScheduler, 

	return EPass;
}

CTlsCloseConnection::CTlsCloseConnection()
/**
 * Constructor.
 * Store the name of this test case.
 */
{
	iTestStepName = _L("CloseConnection");
}

TVerdict CTlsCloseConnection::doTestStepL( )
{
	Log(_L("Disconnecting"));
	iTlsSuite->iSocket.Close() ;	
	iTlsSuite->iSocketServer.Close();

	delete iTlsSuite->iSecureSocket;

	return EPass;
}



void CTestStepT_Tls::TLSTestL()
	{
	__UHEAP_MARK; 

	CActiveScheduler* myActiveScheduler;
	CController* myController;
	
	// Create an active scheduler
	myActiveScheduler = new(ELeave) CActiveScheduler();
	CleanupStack::PushL( myActiveScheduler );
	CActiveScheduler::Install( myActiveScheduler );

	// Create the controller active object
	myController = CController::NewL();

	// Initiate the controllers timer request
	myController->Start( this );

	// Start the scheduler
	myActiveScheduler->Start();
		
	delete myController;

	// Remove objects from the cleanup stack
	CleanupStack::PopAndDestroy( 1 ); // myActiveScheduler, 
	__UHEAP_MARKEND;

	}


CTlsFailSuiteSelection::CTlsFailSuiteSelection()
/**
 * Constructor.
 * Store the name of this test case.
 */
{
	iTestStepName = _L("FailSuiteSelection");
}

TVerdict CTlsFailSuiteSelection::doTestStepL( )
{
	_LIT(KSSLProtocol,"tls1.0");
	_LIT(KFSSectionName,"FailSuiteSelection");
	TRequestStatus Status;
	TVerdict result = EFail;

	TRAPD(error,CommInitL()); // init needed comms libs
	__ASSERT_ALWAYS(!error,User::Panic(KTxtTLS,error));

	// Create an active scheduler
	CActiveScheduler* myActiveScheduler;
	myActiveScheduler = new(ELeave) CActiveScheduler();
	CleanupStack::PushL( myActiveScheduler );
	CActiveScheduler::Install( myActiveScheduler );

	// get address, port number & cipher suite 
	TPtrC addr = iAddress;
	TESTL(GetStringFromConfig(KSectionName, KCfgIPAddress, addr));
	iAddress.Copy( addr );
	TESTL(GetIntFromConfig(KSectionName, KCfgIPPort, iPortNum));
	TPtrC PtrResult;
	TPtrC* res=&PtrResult;
	TESTL(GetStringFromConfig(KFSSectionName, KCfgCipherSuites, PtrResult));
	iCipherSuites.Copy( PtrResult );
	Log( _L("CipherSuites: %S"), res);
		
	// Connect the socket server
	User::LeaveIfError( iTlsSuite->iSocketServer.Connect() );

	// configure address and port
	User::LeaveIfError( iInetAddr.Input( iAddress ));
	iInetAddr.SetPort( iPortNum );

	// Open the socket
	Log(_L("Connecting to %s:%d"), iAddress.PtrZ(), iPortNum );
	User::LeaveIfError( iTlsSuite->iSocket.Open( iTlsSuite->iSocketServer, KAfInet, KSockStream, KProtocolInetTcp ) );	

	// connect the socket
	iTlsSuite->iSocket.Connect( iInetAddr, Status );	
	User::WaitForRequest(Status);
	Log(_L("Connect result is %d"), Status.Int() );
	TESTEL(Status==KErrNone, Status.Int());

	// create a secureSocket
	User::LeaveIfNull(iSecureSocket = CSecureSocket::NewL( iTlsSuite->iSocket,KSSLProtocol()));

	// configure invalid cipher suite
	TBuf8<KCipherBufSize>	cipherBuf;
	TBuf8<3>	tempBuf;
	TInt	i;
	TLex8	myLex;
	TInt	cCount = 0;			// used as an array index into the cipherBuf descriptor
	TInt	ret;
	TInt	value;

	cipherBuf.SetLength( iCipherSuites.Length() );

	for ( i=0; i<iCipherSuites.Length(); i+=2 )
		{
		// iCipherSuites contains a list of decimal values for each cipher suite that
		// the client should offer to use. They are in a string format, so each decimal 
		// value takes 2 bytes.
		//
		// Copy the 2 bytes of one value into a buffer so that it can be converted into
		// a real decimal value;
		tempBuf.SetLength( 2 );
		tempBuf[0] = iCipherSuites[i];
		tempBuf[1] = iCipherSuites[i+1];

		myLex.Assign( tempBuf );
		ret = myLex.Val( value );
		if ( ret!=KErrNone )
			{
			break; // from for loop
			}
		Log( _L(":%X"), value );

		// The actual cipher suite list that must be passed in the socket options
		// is in a binary format of 0x0,0xCipherValue,0x0,CipherValue etc				
		cipherBuf[ cCount++ ] = 0;
		cipherBuf[ cCount++ ] = (unsigned char)value;

	} // end of for loop

	// Set the cipher suite(s) that the client will support
	ret = iSecureSocket->SetAvailableCipherSuites( cipherBuf );

	// this should fail with KErrNotSupported
	if ( ret == KErrNotSupported )
		{
		result = EPass;
		Log( _L("Test passes as because SetAvailableCipherSuites() returned KErrNotSupported") );
		Log( _L("when setting cipher suites=%S"), res);
		}
	else
		{
		Log( _L("Test failed because SetAvailableCipherSuites() returned %d"),ret );
		Log( _L("with invalid suite %S"), res);
		result = EFail;
		}

	iTlsSuite->iSocket.Close() ;	
	iTlsSuite->iSocketServer.Close();

	// Remove objects from the cleanup stack
	CleanupStack::PopAndDestroy( 1 ); // myActiveScheduler, 

	delete iSecureSocket;

	return result;
}


/**
Constructor.
Store the name of this test case
*/
CTestStepDialogMode_Tls::CTestStepDialogMode_Tls()
{
	iTestStepName = _L("t_tls_DialogMode");
}

/**
 This is the test code for CTestStepDialogMode_Tls
*/
TVerdict CTestStepDialogMode_Tls::doTestStepL( )
{
	__UHEAP_MARK; 

	iTestStepResult = EPass;

    Log(_L("Testing dialog mode change"));

	// Create and install active scheduler
	CActiveScheduler*   myActiveScheduler;
	myActiveScheduler = new(ELeave) CActiveScheduler();
	CleanupStack::PushL( myActiveScheduler );
	CActiveScheduler::Install( myActiveScheduler );

    RSocket  sock;
    
    MSecureSocket* pSecSock = CTlsConnection::NewL(sock, KProtocolVerSSL30);
   
    TDialogMode dlgMode;
    TInt nErr;
  
    //========================================================================================
    //==    Test setting dialog mode via MSecureSocket::SetDialogMode()
    //==    In this case we will use EDialogModeUnattended, EDialogModeAttended enum
    //========================================================================================
    
    //-- 1. test default dialog mode, it shall be EDialogModeAttended
    dlgMode = pSecSock->DialogMode();
    TESTL(dlgMode == EDialogModeAttended);
   
    //-- 2. change dialog mode, check if has been changed correctly
    nErr = pSecSock->SetDialogMode(EDialogModeUnattended);
    TESTL(nErr == KErrNone);
    
    dlgMode =  pSecSock->DialogMode();
    TESTL(dlgMode == EDialogModeUnattended);

    //-- 3. change dialog mode to a different value, check if has been changed correctly
    nErr = pSecSock->SetDialogMode(EDialogModeAttended);
    TESTL(nErr == KErrNone);
    
    dlgMode =  pSecSock->DialogMode();
    TESTL(dlgMode == EDialogModeAttended);
    
    //-- 4. try to pass invalid value as a dialog mode
    const TUint KInvalidModeValue = 0xdead;
    nErr = pSecSock->SetDialogMode(static_cast<TDialogMode>(KInvalidModeValue));
    TESTL(nErr == KErrArgument);
        

    //========================================================================================
    //== Test setting dialog mode via MSecureSocket::SetOpt()
    //== In this case KSSLDialogUnattendedMode, KSSLDialogAttendedMode constants will be used
    //== Check also their consistency with TDialogMode enum values
    //========================================================================================

    TInt nOption=-1;
    
    //-- Set dialog mode to EDialogModeUnattended via MSecureSocket::SetOpt()
    
    nErr = pSecSock->SetOpt(KSoDialogMode,KSolInetSSL,KSSLDialogUnattendedMode);
    TESTL(nErr == KErrNone);
    
    nErr = pSecSock->GetOpt(KSoDialogMode,KSolInetSSL,nOption);
    TESTL(nErr == KErrNone);
    TESTL((TUint)nOption == KSSLDialogUnattendedMode);

    dlgMode =  pSecSock->DialogMode();
    TESTL(dlgMode == EDialogModeUnattended); //-- consistency check


    //-- Set dialog mode to EDialogModeAttended via MSecureSocket::SetOpt()

    nErr = pSecSock->SetOpt(KSoDialogMode,KSolInetSSL,KSSLDialogAttendedMode);
    TESTL(nErr == KErrNone);

    nErr = pSecSock->GetOpt(KSoDialogMode,KSolInetSSL,nOption);
    TESTL(nErr == KErrNone);
    TESTL((TUint)nOption == KSSLDialogAttendedMode);
    
    dlgMode =  pSecSock->DialogMode();
    TESTL(dlgMode == EDialogModeAttended); //-- consistency check

    delete pSecSock;
	

	// Remove objects from the cleanup stack
	CleanupStack::PopAndDestroy( 1 ); // myActiveScheduler, 
	__UHEAP_MARKEND;
	
	return iTestStepResult;
}





