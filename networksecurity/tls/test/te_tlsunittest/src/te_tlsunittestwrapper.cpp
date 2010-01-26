// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Contains the implementation of all the commands, which is used by the script file.
//



/**
 @file
 @internalTechnology
*/


#include "te_tlsunittestwrapper.h"


// Send buffer size
const TInt KSendBufferSize = 512;
// Receive buffer size
const TInt KReceiveBufferSize = 256;
const TInt KDelay = 1000000;

// Commands
_LIT(KInitialize,    		"Initialize");
_LIT(KServerCert,    		"ServerCert");
_LIT(KPskSetConfig,    		"PskSetConfig");
_LIT(KPskGetConfig,    		"PskGetConfig");
_LIT(KSSLv2handshake,		"Sslv2Handshake");
_LIT(KSSLServerCert,		"SslServerCert");
_LIT(KEnableNullCipher, 	"EnableNullCipher");
_LIT(KCheckGetOptDefault,   "CheckGetOptDefault");
_LIT(KCheckSetOptDefault,   "CheckSetOptDefault");
_LIT(KSetDialogMode,    	"SetDialogMode");
_LIT(KServerCertGetOpt,    	"ServerCertGetOpt");
_LIT(KConnect,  		  	"Connect");
_LIT(KMakePageRequest,  	"MakePageRequest");
_LIT(KGetServerResponse,  	"GetServerResponse");
_LIT(KReadServerResponse,  	"ReadServerResponse");
_LIT(KConnectionClosedL,  	"ConnectionClosed");
_LIT(KTLSHandshake,  		"TLSHandshake");
_LIT(KCurrentCipherSuite,  	"CurrentCipherSuite");
_LIT(KSetProtocol,  		"SetProtocol");
_LIT(KRenegotiateHandshake,	"RenegotiateHandshake");
_LIT(KSetUnsupportedProtocol,	"SetUnsupportedProtocol");
_LIT(KResetCryptoAttributes,"ResetCryptoAttributes");

// HTTP messages
_LIT8(KSimpleGet, "GET ");
_LIT8(KNewLine, "\r\n"); 
_LIT8(KHeader, "Connection: close\r\nUser-Agent: SSL_TEST\r\nAccept-Encoding:\r\nAccept: */*");


// Progress messages
_LIT(KProtocolMessage, "Protocol used in connection: %S");
_LIT(KReceivedMessage,"Received server response");
_LIT(KReceivingMessage,"Receiving server response");
_LIT(KCompleteMessage,"Transaction complete: bytes recieved %d");
_LIT(KRenegotiate,"Renegotiating Handshake");

// Panic code
_LIT( KTLSPanic, "TLS-Panic");

// State reporting messages
_LIT( KStateErrSecureConnected, "Error in state ESecureConnected: %d\n" );
_LIT( KStateErrEGetRequestSent, "Error in state EGetRequestSent: %d\n" );
_LIT( KStateErrEGetPageReceived, "Error in state EGetPageReceived: %d\n" );


// Config file
_LIT(KInputFromConfig,	"InputFromConfig");
_LIT(KDestAddr, 		"DestAddr");
_LIT(KNameDefault, 		"default");
_LIT(KPortNum, 			"Port");

//Test code
_LIT(KCommand, 			"aCommand = %S");
_LIT(KSection, 			"aSection = %S");
_LIT(KAsyncErrorIndex, 	"aAsyncErrorIndex = %D");
_LIT8(KPSK_IDENTITY, 	"Client_identity");
_LIT8(KPSK_KEY,			"0123456789");
_LIT(KTLS1,				"TLS1.0");
_LIT(KTLS2,				"TLS2.0");

_LIT(KYes,"Yes");
_LIT(KInputFile, "C:\\t_secdlg_in.dat");
_LIT(KOutputFile, "C:\\t_secdlg_out.dat");
_LIT(KMessage, "Passphrase of the imported key file");

//Test code error

_LIT(KInetInputFail,	 "TInetAddr Input failed with error: %d");
_LIT(KSockServConnFail,	 "SockServ Connect() failed with error: %d");
_LIT(KSSLGetOptFail, 	 "GetOpt Failed with error: %d");
_LIT(KSSLSetOptFail, 	 "SetOpt Failed with error: %d");
_LIT(KHandshakeFail,	 "Handshake failed with error: %d");


	


/**
Constructor.

@internalTechnology
*/
CTlsUnitTestWrapper::CTlsUnitTestWrapper()
	{
	
	}

/**
Destructor.

@internalTechnology
*/
CTlsUnitTestWrapper::~CTlsUnitTestWrapper()
	{
	
	if(isecuresocket)
		{
		isecuresocket->Close();
		delete isecuresocket;
		}
	if(isocketserv)
		{
		isocketserv->Close();
		delete isocketserv;
		}
	
	}

/**
Function to instantiate TestWrapper.

@return Returns constructed TestWrapper instance pointer

@internalTechnology
*/
CTlsUnitTestWrapper* CTlsUnitTestWrapper::NewL()
	{
	CTlsUnitTestWrapper*ret = new (ELeave) CTlsUnitTestWrapper();
	CleanupStack::PushL(ret);
	ret->ConstructL();
	CleanupStack::Pop(ret);
	return ret;	
	}

/**
Second level constructor, constructs TestWrapper instance.

@internalTechnology
*/
void CTlsUnitTestWrapper::ConstructL()
	{
	isocketserv = new (ELeave) RSocketServ;
	return;
	}

/**
Function to map the input command to respective function.

@return - True Upon successfull command to Function name mapping otherwise False
@param aCommand Function name has to be called
@param aSection INI file paramenter section name
@param aAsyncErrorIndex Error index
@see Refer the script file COMMAND section.

@internalTechnology
*/
TBool CTlsUnitTestWrapper::DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt aAsyncErrorIndex)
	{
	TBool ret = ETrue;
	
	// Print out the parameters for debugging
	INFO_PRINTF2( KCommand, &aCommand );
	INFO_PRINTF2( KSection, &aSection );
	INFO_PRINTF2( KAsyncErrorIndex, aAsyncErrorIndex );
	
	if (KInitialize() == aCommand)
		{
		DoInitialize();
		}
	else if (KSetUnsupportedProtocol() == aCommand)
		{
		DoSetUnsupportedProtocol();
		}
	else if (KServerCert() == aCommand)
		{
		DoServerCert();
		}
	else if (KRenegotiateHandshake() == aCommand)
		{
		DoRenegotiateHandshake();
		}
	else if (KSetProtocol() == aCommand)
		{
		DoSetProtocol();
		}
	else if (KCurrentCipherSuite() == aCommand)
		{
		DoCurrentCipherSuite();
		}
	else if (KMakePageRequest() == aCommand)
		{
		DoMakePageRequest();
		}
	else if (KGetServerResponse() == aCommand)
		{
		DoGetServerResponse();
		}
	else if (KReadServerResponse() == aCommand)
		{
		DoReadServerResponse();
		}
	else if (KConnectionClosedL() == aCommand)
		{
		DoConnectionClosed();
		}
	else if (KConnect() == aCommand)
		{
		DoConnect(aSection);
		}
	else if (KTLSHandshake() == aCommand)
		{
		DoTLSHandshake();
		}
	else if (KServerCertGetOpt() == aCommand)
		{
		DoServerCertGetOpt();
		}
	else if (KEnableNullCipher() == aCommand)
		{
		DoEnableNullCipher();
		}
	else if (KSSLv2handshake() == aCommand)
		{
		DoSSLv2handshake();
		}
	else if (KPskGetConfig() == aCommand)
		{
		DoPskGetConfig();
		}
	else if (KPskSetConfig() == aCommand)
		{
		DoPskSetConfig();
		}
	else if (KCheckSetOptDefault() == aCommand)
		{
		DoCheckSetOptDefault();
		}
	else if (KCheckGetOptDefault() == aCommand)
		{
		DoCheckGetOptDefault();
		}
	else if (KSetDialogMode() == aCommand)
		{
		DoSetDialogMode();
		}
	else
		{
		ret = EFalse;
		User::LeaveIfError(KErrNone); // just to suppress LeaveScan warning
		}
	return ret;
	}
	
/** 
Constructor: Test class
Needs to call the base class Constructor setting the priority of the active object 

@internalTechnology
*/

CTlsUnitTest* CTlsUnitTest::NewL()
	{
	CTlsUnitTest* self = new(ELeave) CTlsUnitTest;
	CleanupStack::PushL( self );
	self->ConstructL();
 	CleanupStack::Pop();		
	return self;
	}


/** 
Constructor: Test class
Needs to call the base class Constructor setting the priority of the active object 

@internalTechnology
*/

CTlsUnitTest::CTlsUnitTest():CActive( EPriorityStandard ), iSndBuffer(0,0), iRcvBuffer(0,0)
   	{
	}


CTlsUnitTest::~CTlsUnitTest()
	{
	// Clean up Active object's permanent resources
	delete (void*)iSndBuffer.Ptr();
	delete (void*)iRcvBuffer.Ptr();
    }


void CTlsUnitTest::ConstructL()
	{
	iSndBuffer.Set((TUint8*)User::AllocL(KSendBufferSize),0,KSendBufferSize);
	iRcvBuffer.Set((TUint8*)User::AllocL(KReceiveBufferSize),0,KReceiveBufferSize);
		
	User::LeaveIfError( iTimer.CreateLocal());
		
	CActiveScheduler::Add( this );
	}

/** 
RunError method used handling error encountered in RunL by test code.

@internalTechnology
*/

TInt CTlsUnitTest::RunError( TInt aError )
	{
	// Panic prevents looping
	__ASSERT_ALWAYS(iState != EConnectionClosed, 
		User::Panic(KTLSPanic,0));

	iRunError = aError;
	// do a switch on the state to get the right err message
	switch (iState)
		{
		case ESecureConnected:
			iwrapperobject->ERR_PRINTF2(KStateErrSecureConnected, aError );
			break;
		case EGetRequestSent:
			iwrapperobject->ERR_PRINTF2(KStateErrEGetRequestSent, aError );
			break;
		case EGetPageReceived:
			iwrapperobject->ERR_PRINTF2(KStateErrEGetPageReceived, aError);
			break;
		default:
			break;
		}
	iwrapperobject->SetError(iRunError);
	CActiveScheduler::Stop();
	
	return KErrNone;
	}


/** 
RunL method used for all asyncronous requests made by test code.

@internalTechnology
*/

void CTlsUnitTest::RunL()
	{
	iRunError =KErrNone;
	//While reading server response, we get KErrEof which is desired in that case.
	if (iStatus!=KErrEof) User::LeaveIfError(iStatus.Int());
		
	switch(iState)
		{
		case ESecureConnected:
			iwrapperobject->INFO_PRINTF1(_L("secure connection made"));
			iState = EIdle;
			CActiveScheduler::Stop();
			break;
			
		case EGetRequestSent:
			iwrapperobject->INFO_PRINTF1(_L("Get request sent to the server"));
			iState = EIdle;
			CActiveScheduler::Stop();
			break;

		case EGetPageReceived:
			iwrapperobject->INFO_PRINTF1(_L("Received response from the server"));
			iState = EIdle;
			CActiveScheduler::Stop();
			break;
				
		case ERenegotiateHandshake:
			iState = EIdle;
			CActiveScheduler::Stop();
			break;
				
		case EConnectionClosed:
			iState = EIdle;
			CActiveScheduler::Stop();
			break;

		default:
			break;
							
		} 
	return; 
	}

/** 
Establishes secure connection to TLS test server by using CSecureSocket::StartClientHandshake method.
	
@param TestWrapper class's object.
@return TInt ,return error code returned by status of StartClientHandshake method.

@internalTechnology
*/

TInt CTlsUnitTest::HandshakeL(CTlsUnitTestWrapper* aObject)
	{
	
	iwrapperobject = aObject;
	iwrapperobject->isecuresocket = CSecureSocket::NewL( iwrapperobject->isocket, KTLS1() );
	
	// Clears any previous options
	iwrapperobject->isecuresocket->FlushSessionCache();
	
	// start the handshake 
	iStatus = KRequestPending;
	iwrapperobject->isecuresocket->StartClientHandshake( iStatus);  
	iState = ESecureConnected; 
	SetActive();
	CActiveScheduler::Start(); 
	
	return iStatus.Int();  
	}

/** 
Makes a page request to the TLS server after secure connection is established by specifying the page to be fetched in iSndBuffer.
	
@param None
@See Asynchronous Send() method used to send the requested page data to the server.

@internalTechnology
*/

void CTlsUnitTest::MakePageRequestL()
	{
	// Create a GET request	
	_LIT8(KUrl1, "https://");
	_LIT8(KUrl2, ":");
	_LIT8(KUrl3, "iwrapperobject->port");
	_LIT8(KUrl4, " HTTP/1.0\r\n");
	_LIT8(KUrl5, "/index.html");
	iSndBuffer+=KSimpleGet;
	iSndBuffer+=KUrl1;
	iSndBuffer+=iwrapperobject->iAddress;
	iSndBuffer+=KUrl2;
	iSndBuffer+=KUrl3;
	iSndBuffer+=KUrl5;
	iSndBuffer+=KUrl4;
	iSndBuffer+=KNewLine;	
	iSndBuffer+=KHeader;
		
	// Send the request
	iState = EGetRequestSent;
	iwrapperobject->isecuresocket->Send( iSndBuffer, iStatus );
	SetActive();
	CActiveScheduler::Start();
	return;
	}

/** 
Makes a asynchronous call to Reccv() for receiving data sent by the server, Recv() returns when the buffer is full.
	
@param None
@See Asynchronous Recv() method used to receive the data from server.

@internalTechnology
*/

void CTlsUnitTest::GetServerResponseL()
	{
	User::LeaveIfError(iStatus.Int());
	iwrapperobject->INFO_PRINTF1(KReceivingMessage);
	
	TBuf8<2> buf; 
	User::LeaveIfError(iwrapperobject->isecuresocket->CurrentCipherSuite( buf ));
	
	TBuf<32> protocol;
	User::LeaveIfError(iwrapperobject->isecuresocket->Protocol( protocol ));
	iwrapperobject->INFO_PRINTF2(KProtocolMessage, &protocol);
	
	// Read asynchonously-returns when buffer full
	iState = EGetPageReceived;
	iwrapperobject->isecuresocket->Recv( iRcvBuffer, iStatus );
	SetActive();
	CActiveScheduler::Start();
	return;
	}

/** 
Processes the data received from the server, and if server sends data more than the receive buffer size, again makes a call to Recv().
	
@param None
@See Asynchronous Recv() method used to receive the data from server.

@internalTechnology
*/

void CTlsUnitTest::ReadServerResponseL()
	{
	if (iStatus!=KErrEof) User::LeaveIfError(iStatus.Int());
	iwrapperobject->INFO_PRINTF1(KReceivedMessage);
	
	iTotalBytesRead += iRcvBuffer.Length();
	
	// Case 1: error is KErrEof (message complete) or no data received, so stop
	if ( ( iStatus==KErrEof ) || ( iRcvBuffer.Length() == 0 ) )
		{
		iwrapperobject->INFO_PRINTF2(KCompleteMessage, iTotalBytesRead);
		// Close the socket neatly
		iState = EConnectionClosed;
		iTimer.After( iStatus, KDelay );
		SetActive();
		CActiveScheduler::Start();
		return;	
		}

		// Case 2: there's more data to get from the server
		iRcvBuffer.SetLength( 0 );
		iState = EGetPageReceived;
		iwrapperobject->isecuresocket->Recv( iRcvBuffer, iStatus );
		SetActive(); 
		CActiveScheduler::Start();
		return;
	}

/** 
Makes consecutive calls to CSecureSocket's RenegotiateHandshake() method.
	
@param None
@See Renegotiation started by the client, but we make two consecutive calls to it to cover the negative scenario.

@internalTechnology
*/

void CTlsUnitTest::RenegotiateHandshake()
	{
	iwrapperobject->INFO_PRINTF1(KRenegotiate);
	iStatus = KRequestPending;
	iState = ERenegotiateHandshake;
	iwrapperobject->isecuresocket->RenegotiateHandshake( iStatus );
	SetActive(); 
	CActiveScheduler::Start();
	iStatus = KRequestPending;
	iwrapperobject->isecuresocket->RenegotiateHandshake( iStatus );
	SetActive(); 
	CActiveScheduler::Start();
	return;
	}

/** 
Gracefully closes the connection by cancelling all Send() and Recv() requests and then closing the socket.
	
@param None

@internalTechnology
*/

void CTlsUnitTest::ConnectionClosed()
	{
	
	iwrapperobject->isocket.CancelAll();
	iwrapperobject->isocket.Close();
	
	iState = EConnectionClosed;
	iStatus = KRequestPending;
	iTimer.After( iStatus, KDelay );
	SetActive();		
	CActiveScheduler::Start();
	}

/** 
Makes consecutive calls to CSecureSocket's ServerCert() method.
	
@param None
@See calling the same API in succession to check for the negative scenario, for coverage !
@internalTechnology
*/

void CTlsUnitTestWrapper::DoServerCert()
	{
	// Get the servers certificateKTLS2
	const CX509Certificate *servCert = isecuresocket->ServerCert();
	// calling the same API in succession to check the negative scenario, for coverage !
	const CX509Certificate *servCert1 = isecuresocket->ServerCert();
	
	}

/**
Function to get options set for the secure socket with corresponding option name and level.

@param  none
@see Sets error as we are expecting KErrOverflow, because insufficient buffer size is specified.

@internalTechnology
*/

void CTlsUnitTestWrapper::DoServerCertGetOpt()
	{
	// Do GetOpt with sufficient buffer size to succesfully get the server cert in the buffer.
	TInt err;
	TBuf8 <1000> buffer;
	err = isecuresocket->GetOpt(KSoSSLServerCert,KSolInetSSL,buffer);
	if (err != KErrNone)
		{
		ERR_PRINTF2(KSSLGetOptFail, err);
		SetError(err);
		}
	
	// Do GetOpt with insufficient buffer size, expecting KErrOverflow and set the error.
	TBuf8 <2> buffer1;
	err = isecuresocket->GetOpt(KSoSSLServerCert,KSolInetSSL,buffer1);
	if (err != KErrNone)
		{
		ERR_PRINTF2(KSSLGetOptFail, err);
		SetError(err);
		}
	}

/**
Function to read from the config file and connect to the server by opening a socket and then
establishing a secure connection by making a call to CTlsUnitTest::HandshakeL(CTlsUnitTestTestWrapper* aObject)
and then fetching the certificate returned by SSL server through GetOpt call.

@param  aSection Current ini file command section
@see Sets error on upon config file read error.

@internalTechnology
*/

void CTlsUnitTestWrapper::DoConnect(const TDesC& aSection)
	{
	
	TInt error = KErrNone;
	TInt recvBufSize = KErrNotFound, sendBufSize = KErrNotFound;
	TInt noDelayEnabled = KErrNotFound;

			
	User::LeaveIfError(isocketserv->Connect());
		
	// Open a socket
		
	User::LeaveIfError(isocket.Open( *isocketserv, KAfInet, KSockStream, KProtocolInetTcp ));
					
	TPtrC ptrToReadFromConfig(KNameDefault());
			
	// Get destination address from config file	 		
	TBool returnValue = GetStringFromConfig(aSection, KDestAddr, ptrToReadFromConfig);
	if (!returnValue)
		{
		ERR_PRINTF1(_L("Reading config file failed, while reading DestAddr"));	 
		SetError(KErrUnknown);
		return;
		}
	else
		{
		iAddress.Copy(ptrToReadFromConfig) ;
		}
						
	// Create address
	TInetAddr ipAddr;
	TInt err = ipAddr.Input(ptrToReadFromConfig);	
	if(err != KErrNone)
		{
		INFO_PRINTF2(KInetInputFail, err);
		SetError(err);
		return;
		}
			
	// Get destination Port number from config file	
			
	returnValue = GetIntFromConfig(aSection, KPortNum, iport);
	if (!returnValue)
		{
		ERR_PRINTF1(_L("Reading config file failed, while reading DestAddr"));	 
		SetError(KErrUnknown);
		return;
		}
			
	// set port number	
	ipAddr.SetPort(iport);	
	if(err != KErrNone)
		{
		INFO_PRINTF2(KInetInputFail, err);
		SetError(err);
		return;
		}
			
	//Socket connect() call.
	isocket.Connect(ipAddr, istatus);
	User::WaitForRequest(istatus);
	if(istatus.Int() != KErrNone)
		{
		INFO_PRINTF1( _L("Connection failed to specified address and port.") );
		User::Leave(KErrCouldNotConnect);
		}
	else
		{
		INFO_PRINTF1( _L("Connected to the specified address and port.") );
		}
	}

/**
Function to Initialize the Handshake by making an object of the Active Object class and making a call to the HandshakeL() method of the active object class.

@param  none
@see Handshake completed and secure connection with the server established.

@internalTechnology
*/

void CTlsUnitTestWrapper::DoTLSHandshake()
	{
	TInt error;
	iActiveObjTest = CTlsUnitTest::NewL();
	CleanupStack::PushL(iActiveObjTest);
		
	// Making a secure connection by making a call to HandshakeL()
	error = iActiveObjTest->HandshakeL(this);	
	if(error != KErrNone)
		{
		INFO_PRINTF2(KHandshakeFail, error);
		SetError(error);
		return;
		}
				
	CleanupStack::Pop(iActiveObjTest);
	return;
	}

/**
Function to Initialize the connection by opening a connection to the socket server and opening a socket.

@param  none
@see Connection to socket server made and a TCP socket opened.

@internalTechnology
*/

void CTlsUnitTestWrapper::DoInitialize()
	{
	// Connect to the socket server.
	User::LeaveIfError(isocketserv->Connect());
	// Open a socket
	User::LeaveIfError(isocket.Open( *isocketserv, KAfInet, KSockStream, KProtocolInetTcp ));
	
	return;
	}

/**
Function to get options set for the secure socket with corresponding option name and level.

@param  none
@see Sets error as we are expecting KErrOverflow, because insufficient buffer size is specified.

@internalTechnology
*/

void CTlsUnitTestWrapper::DoSetDialogMode()
	{
	TInt err;
	//Create a secure socket
	isecuresocket = CSecureSocket::NewL(isocket, KTLS1());
	TBuf8 <2> buffer;
	
	// Do GetOpt with insufficient buffer size, expecting KErrOverflow and set the error.
	err = isecuresocket->GetOpt(KSoDialogMode,KSolInetSSL,buffer);
	if (err != KErrNone)
		{
		ERR_PRINTF2(KSSLGetOptFail, err);
		SetError(err);
		}
	
	isecuresocket->Close();
	
	return;
	}


/**
Function to set options for the secure socket with invalid option name.

@param  none
@see Sets error as we are expecting KErrNotSupported, because we specify a invalid option name 
making it a dafault case and return KErrNotSupported.

@internalTechnology
*/

void CTlsUnitTestWrapper::DoCheckSetOptDefault()
	{
	TInt err;
	//Create a secure socket
	isecuresocket = CSecureSocket::NewL(isocket, KTLS1());
	TBufC8 <2> buffer;
	
	// Do SetOpt with invalid option name, expecting KErrNotSupported and set the error.
	err = isecuresocket->SetOpt(KSolInetSSL,KSolInetSSL,buffer);
	if (err != KErrNone)
		{
		ERR_PRINTF2(KSSLSetOptFail, err);
		SetError(err);
		}
	
	isecuresocket->Close();
	
	return;
	}

/**
Function to Get options set for the secure socket with invalid option name.

@param  none
@see Sets error as we are expecting KErrNotSupported, because we specify a invalid option name 
making it a dafault case and return KErrNotSupported.

@internalTechnology
*/

void CTlsUnitTestWrapper::DoCheckGetOptDefault()
	{
	TInt err;
	//Create a secure socket
	isecuresocket = CSecureSocket::NewL(isocket, KTLS1());
	TBuf8 <2> buffer;
	
	// Do GetOpt with invalid option name, expecting KErrNotSupported and set the error.
	err = isecuresocket->GetOpt(KSolInetSSL,KSolInetSSL,buffer);
	if (err != KErrNone)
		{
		ERR_PRINTF2(KSSLGetOptFail, err);
		SetError(err);
		}
	
	isecuresocket->Close();
	
	return;
	}

/**
Function to Get PskConfig options set for the secure socket with invalid option name.

@param  none
@see Sets error as we are expecting KErrArgument, because we specified insufficient buffer size 
to hold the MSoPskKeyHandler object. 

@internalTechnology
*/

void CTlsUnitTestWrapper::DoPskGetConfig()
	{
	TInt err;
	TBuf8 <2> buffer;
	//Create a secure socket 
	isecuresocket = CSecureSocket::NewL(isocket, KTLS1());
	
	// Do GetOpt with insufficient buffer size, expecting KErrArgument and set the error.
	err = isecuresocket->GetOpt(KSoPskConfig,KSolInetSSL,buffer);
	if (err != KErrNone)
		{
		ERR_PRINTF2(KSSLGetOptFail, err);
		SetError(err);
		}
	
	TPckgBuf<MSoPskKeyHandler *> pskConfigPkg1;
	pskConfigPkg1() = this;
	
	// Do GetOpt with sufficient buffer size to succesfully get the MSoPskKeyHandler object in the buffer.
	err = isecuresocket->GetOpt(KSoPskConfig,KSolInetSSL,pskConfigPkg1);
	if (err != KErrNone)
		{
		ERR_PRINTF2(KSSLGetOptFail, err);
		SetError(err);
		}
	
	isecuresocket->Close();
		
	return;
	}

/**
Function to Set PskConfig options for the secure socket with invalid option name.

@param  none
@see Sets error as we are expecting KErrArgument, because we specified buffer size smaller than
the MSoPskKeyHandler object. 

@internalTechnology
*/

void CTlsUnitTestWrapper::DoPskSetConfig()
	{
	TInt err;
	TBufC8 <2> buffer1;
	isecuresocket = CSecureSocket::NewL(isocket, KTLS1());
	
	// Do SetOpt with insufficient buffer size, expecting KErrArgument and set the error.
	err = isecuresocket->SetOpt(KSoPskConfig,KSolInetSSL,buffer1);
	if (err != KErrNone)
		{
		ERR_PRINTF2(KSSLSetOptFail, err);
		SetError(err);
		}

	TPckgBuf<MSoPskKeyHandler *> pskConfigPkg;
	pskConfigPkg() = this;
	
	// Do SetOpt with sufficient buffer size to succesfully set the options.
	err = isecuresocket->SetOpt(KSoPskConfig, KSolInetSSL, pskConfigPkg);
	if (err != KErrNone)
		{
		ERR_PRINTF2(KSSLSetOptFail, err);
		SetError(err);
		}

	TPckgBuf<MSoPskKeyHandler *> pskConfigPkg2;
	pskConfigPkg2() = this;
	
	// Do GetOpt once again, but this time after succesfully setting the PSk options. 
	err = isecuresocket->GetOpt(KSoPskConfig,KSolInetSSL,pskConfigPkg2);
	if (err != KErrNone)
		{
		ERR_PRINTF2(KSSLGetOptFail, err);
		SetError(err);
		}
	
	isecuresocket->Close();
	
	return;
	}

/**
Function to Get and Set SSLv2Handshake realted options.

@param  none
@see This option is no longer supported, just including it from the coverage point of view.

@internalTechnology
*/

void CTlsUnitTestWrapper::DoSSLv2handshake()
	{
	TInt err;
	//Create a secure socket
	isecuresocket = CSecureSocket::NewL(isocket, KTLS1());
	
	// Do SetOpt with option name KSoUseSSLv2Handshake, this is not supported anymore, always returns KErrNone.
	err = isecuresocket->SetOpt(KSoUseSSLv2Handshake,KSolInetSSL,ETrue);
		
	if (err != KErrNone)
		{
		ERR_PRINTF2(KSSLSetOptFail, err);
		SetError(err);
		}
	TBool buffer;
	
	// Do GetOpt with option name KSoUseSSLv2Handshake, this is not supported anymore, always returns KErrNone.
	err = isecuresocket->GetOpt(KSoUseSSLv2Handshake,KSolInetSSL,buffer);
	if (err != KErrNone)
		{
		ERR_PRINTF2(KSSLGetOptFail, err);
		SetError(err);
		}
	isecuresocket->Close();
	
	return;
	}

/**
Function to enable Null Cipher suite selection to be allowed by SetOpt and also to get the 
options set for the secure socket by querying through getOpt passing KSoEnableNullCiphers as option name.

@param  none
@see Sets the error upon any errors returned by the call to GetOpt or SetOpt.

@internalTechnology
*/

void CTlsUnitTestWrapper::DoEnableNullCipher()
	{
	TInt err;
	//Create a secure socket
	isecuresocket = CSecureSocket::NewL(isocket, KTLS1());
	
	// Do SetOpt specifying Null cipher selection to Etrue.
	err = isecuresocket->SetOpt(KSoEnableNullCiphers,KSolInetSSL,ETrue);
	
	if (err != KErrNone)
		{
		ERR_PRINTF2(KSSLSetOptFail, err);
		SetError(err);
		}
	TBool buffer;
	
	// Do GetOpt and get whether Null cipher selection is enabled or not in the boolean variable buffer.
	err = isecuresocket->GetOpt(KSoEnableNullCiphers,KSolInetSSL,buffer);
	if (err != KErrNone)
		{
		ERR_PRINTF2(KSSLGetOptFail, err);
		SetError(err);
		}
	isecuresocket->Close();
	
	return;
	}

/** 
Get the PSK client identity from the ini files
Get the Pre Shared Key value from the ini files.

*/

void CTlsUnitTestWrapper::GetPskL(const HBufC8 */*aPskIdentityHint*/, HBufC8 *&aPskIdentity, HBufC8 *&aPskKey)
	{   
    aPskIdentity = KPSK_IDENTITY().AllocL();
	aPskKey = KPSK_KEY().AllocL();
	return;
	}

/** 
Making a call to the AO's GetServerResponseL()method.
@param  none
@internalTechnology
*/

void CTlsUnitTestWrapper::DoGetServerResponse()
	{
	iActiveObjTest->GetServerResponseL();
	}

/** 
Making a call to the AO's ConnectionClosed()method.
@param  none
@internalTechnology
*/

void CTlsUnitTestWrapper::DoConnectionClosed()
	{
	iActiveObjTest->ConnectionClosed();
	}

/** 
Making a call to the AO's ReadServerResponseL()method.
@param  none
@internalTechnology
*/

void CTlsUnitTestWrapper::DoReadServerResponse()
	{
	iActiveObjTest->ReadServerResponseL();
	}

/** 
Making a call to the AO's MakePageRequestL()method.
@param  none
@internalTechnology
*/

void CTlsUnitTestWrapper::DoMakePageRequest()
	{
	iActiveObjTest->MakePageRequestL();
	}

/** 
Making a call to the AO's RenegotiateHandshake()method.
@param  none
@internalTechnology
*/

void CTlsUnitTestWrapper::DoRenegotiateHandshake()
	{
	iActiveObjTest->RenegotiateHandshake();
	}

/** 
Making a call to CSecureSocket's CurrentCipherSuite() method with insufficient buffer size.
@param  none
@see Sets error as we are expecting KErrOverflow, because we specified buffer size smaller than the method expects. 
@internalTechnology
*/

void CTlsUnitTestWrapper::DoCurrentCipherSuite()
	{
	TBuf8<1> buf;
	buf.SetLength(1);
	SetError(isecuresocket->CurrentCipherSuite(buf));
	}

/** 
Making a call to CSecureSocket's SetProtocol() method specifying unsupported protocol .
@param  none
@see Sets error as we are expecting KErrNotSupported, because we specified unsupported protocol as the argument. 
@internalTechnology
*/

void CTlsUnitTestWrapper::DoSetUnsupportedProtocol()
	{
	SetError(isecuresocket->SetProtocol(KTLS2));
	}

/** 
Making a call to CSecureSocket's Protocol() method with insufficient buffer size.
@param  none
@see Sets error as we are expecting KErrOverflow, because we specified buffer size smaller than the method expects. 
@internalTechnology
*/

void CTlsUnitTestWrapper::DoSetProtocol()
	{
	TBuf<2> buf;
	buf.SetLength(1);
	SetError(isecuresocket->Protocol(buf));
	}
