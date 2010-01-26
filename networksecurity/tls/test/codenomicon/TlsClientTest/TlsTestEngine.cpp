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

#include "TlsTestEngine.h"

// Send buffer size
const TInt KSendBufferSize = 512;
// Receive buffer size
const TInt KReceiveBufferSize = 256;
// Maximum number of connection attempts
const TInt KMaxAttempts = 200;


_LIT(KYes,"Yes");
_LIT(KInputFile, "C:\\t_secdlg_in.dat");
_LIT(KOutputFile, "C:\\t_secdlg_out.dat");
_LIT(KMessage, "Passphrase of the imported key file");

// HTTP messages
_LIT8(KSimpleGet, "GET ");
_LIT8(KNewLine, "\r\n"); 
_LIT8(KUrl, "https://192.168.0.1:443 HTTP/1.1\r\n");
_LIT8(KHeader, "Connection: close\r\nUser-Agent: SSL_TEST\r\nAccept-Encoding:\r\nAccept: */*");

// Progress messages
_LIT(KConnnectedMessage, "\nConnecting to %S:%d%S\n");
_LIT(KSecureConnnectingMessage, "\nMaking secure connection");
_LIT(KGettingPageMessage, "\nRequesting web page");
_LIT(KReceivingMessage,"\nReceiving server response");
_LIT(KCipherSuiteInUseMessage,"\nCipher suite in use: %S");
_LIT(KProtocolMessage, "\nProtocol used in connection: %S");
_LIT(KReceivedMessage,"\nReceived server response");
_LIT(KCompleteMessage,"\nTransaction complete: bytes recieved %d");
_LIT(KFileErrorMessage,"\nError in writing data to file");
_LIT(KCancelledMessage,"\nConnection closed");
_LIT(KFailSocketMessage,"\nFailed to open socket: %d");

// State reporting messages
_LIT( KStateErrESocketConnected1, "\nError in state connecting socket: %d\n" );
_LIT( KStateErrESocketConnected, "\nError in state ESocketConnected: %d\n" );
_LIT( KStateErrESettingCiphers, "\nError in state ESettingCiphers: %d\n" );
_LIT( KStateErrSecureConnected, "\nError in state ESecureConnected: %d\n" );
_LIT( KStateErrGetRequestSent, "\nError in state EGetRequestSent: %d\n" );
_LIT( KStateErrEDataReceived, "\nError in state EDataReceived: %d\n" );

// Panic code
_LIT( KSecEnginePanic, "SEC-ENGINE");

//
// CSecEngine
//

CSecEngine* CSecEngine::NewL()
	{
	CSecEngine* self = new(ELeave) CSecEngine;
	CleanupStack::PushL( self );
	self->ConstructL();
	CleanupStack::Pop();		
	return self;
	}

// Constructor should also call the parent constructor to set the priority
// of the active object.
CSecEngine::CSecEngine() 
: CActive(0), iSndBuffer(0,0), iRcvBuffer(0,0)
	{
	}

CSecEngine::~CSecEngine()
	{
	// Cancel any outstanding request- this cleans up
	// resources created during a connection
	//Cancel();	
	ConnectionClosed();
	// Clean up engine's permanent resources
	delete (void*)iSndBuffer.Ptr();
	delete (void*)iRcvBuffer.Ptr();
	iTimer.Close();
	iSocketServ.Close();
	}

void CSecEngine::ConstructL()
	{
	iSndBuffer.Set((TUint8*)User::AllocL(KSendBufferSize),0,KSendBufferSize);
	iRcvBuffer.Set((TUint8*)User::AllocL(KReceiveBufferSize),0,KReceiveBufferSize);
	// Connect the socket server
	User::LeaveIfError( iSocketServ.Connect());	
	// Create a local timer
	User::LeaveIfError( iTimer.CreateLocal());
	// Set initial state
	iRunState = ESocketConnected;
	iInUse = EFalse;

	CActiveScheduler::Add( this );
	
	iFs.Connect();
	CFileMan* fileMan = CFileMan::NewL(iFs);
	CleanupStack::PushL(fileMan);
	TInt err = fileMan->Delete(KInputFile);
	if ( err != KErrNotFound && err != KErrNone )
		{
		User::LeaveIfError(err);
		}
	err = fileMan->Delete(KOutputFile);
	if (err != KErrNotFound && err != KErrNone )
		{
		User::LeaveIfError(err);
		}
	CleanupStack::PopAndDestroy(fileMan);	

	RFileWriteStream stream;
	err = stream.Open(iFs, KInputFile, EFileWrite | EFileShareExclusive);
	if (err == KErrNotFound)
		{
		err = stream.Create(iFs, KInputFile, EFileWrite | EFileShareExclusive);
		}
	User::LeaveIfError(err);
	stream.PushL();
	
	MStreamBuf* streamBuf = stream.Sink();
	streamBuf->SeekL(MStreamBuf::EWrite, EStreamEnd);

	stream.WriteInt32L(0x400);
	stream.WriteInt32L(35);
	stream.WriteL(KMessage);
	stream.WriteInt32L(3);
	stream.WriteL(KYes);
	stream.WriteInt32L(0);
	stream.WriteL(KNullDesC);

	stream.CommitL();
	CleanupStack::PopAndDestroy(); // stream
	
	}

void CSecEngine::ConnectL(const TConnectSettings& aConnectSettings)
	{
	iConnectSettings = &aConnectSettings;

	// Set initial values for flags & buffers
	iSuccess = ETrue;
	iInUse = ETrue;
	iRunState = ESocketConnected;
	iSndBuffer.SetLength( 0 );
	iRcvBuffer.SetLength( 0 );
	iTotalBytesRead = 0;

	// Interpret server address
	if (iInetAddr.Input(iConnectSettings->iAddress) != KErrNone)
		// Success if already in dotted-decimal format
		{
		// Connect to a host resolver (for DNS resolution) - happens sychronously
		User::LeaveIfError( iHostResolver.Open( iSocketServ, KAfInet, KProtocolInetTcp ));
		// Try to resolve symbolic name
		TNameEntry nameEntry;
		User::LeaveIfError (iHostResolver.GetByName( iConnectSettings->iAddress, nameEntry ));
		TSockAddr sockAddr = nameEntry().iAddr;
		iInetAddr = iInetAddr.Cast( sockAddr );
		iHostResolver.Close();
		}
		
	// Store other connection parameters
	iInetAddr.SetPort( iConnectSettings->iPortNum );
							
	// Open a TCP socket
	TInt ret = iSocket.Open( iSocketServ, KAfInet, KSockStream, KProtocolInetTcp );
	
	if(ret != KErrNone)
		{
		iConsole->Printf(KStateErrESocketConnected1, ret);	
		}
		else
		{
			
		User::LeaveIfError(ret);
		}
	

	// Connect to the server, asynchronously
	iSocket.Connect( iInetAddr, iStatus );	
	SetActive();
	CActiveScheduler::Start();

	// Print status
	iConsole->Printf(KConnnectedMessage, 
		&iConnectSettings->iAddress, 
		iConnectSettings->iPortNum, 
		&iConnectSettings->iPage );	
	}

void CSecEngine::SetConsole( CConsoleBase& aConsole )
	{
	iConsole = &aConsole;
	}

void CSecEngine::SetOutputFile( RFile& aOutputFile )
	{
	iOutputFile = &aOutputFile;
	}

TBool CSecEngine::InUse()
	{
	return iInUse;
	}

void CSecEngine::RunL()
	{
	switch ( iRunState )
		{
	case ESocketConnected:
		MakeSecureConnectionL();
		break;

	case ESecureConnected:
		MakePageRequestL();
		break;

	case EGetRequestSent:
		GetServerResponseL();
		break;

	case EDataReceived:
		ReadServerResponseL();
		break;

	case EConnectionClosed:
		ConnectionClosed();
		break;

	default:
		break;
		} // end switch
	}

TInt CSecEngine::RunError( TInt aError )
	{
	// Panic prevents looping
	__ASSERT_ALWAYS(iRunState != EConnectionClosed, 
		User::Panic(KSecEnginePanic,0));

	// do a switch on the state to get the right err message
	switch (iRunState)
		{
		case ESocketConnected:
			iConsole->Printf(KStateErrESocketConnected, aError );
			break;
		case ESettingCiphers:
			iConsole->Printf(KStateErrESettingCiphers, aError );
			break;
		case ESecureConnected:
			iConsole->Printf(KStateErrSecureConnected, aError);
			break;
		case EGetRequestSent:
			iConsole->Printf(KStateErrGetRequestSent, aError);
			break;
		case EDataReceived:
			iConsole->Printf(KStateErrEDataReceived, aError);
			break;
		default:
			break;
		}

	iRunState = EConnectionClosed;
	iSuccess = EFalse;
	iTimer.After( iStatus, 1000000 );
	SetActive();

	return KErrNone;
	}

void CSecEngine::DoCancel()
	{
	iConsole->Printf(KCancelledMessage);
	ConnectionClosed();
	}

void CSecEngine::MakeSecureConnectionL()
	{
	// if secure connection failed, reconnect
	if(iStatus.Int() != KErrNone)
		{
		if(++iCounter >= KMaxAttempts)
			{
			User::Leave(iStatus.Int());
			}
			
		iRunState = ESocketConnected;
		
		iSocket.CancelAll();
//		iTlsSocket->CancelAll();
//		iTlsSocket->Close();
		delete iTlsSocket;
		iTlsSocket =0;
		iSocket.Close();
		
		// Open a TCP socket
		TInt ret = iSocket.Open( iSocketServ, KAfInet, KSockStream, KProtocolInetTcp );
		while(KErrNone != ret)
		{		
		iConsole->Printf(KFailSocketMessage, ret);
		ret = iSocket.Open( iSocketServ, KAfInet, KSockStream, KProtocolInetTcp );
		}
		

		// Connect to the server, asynchronously
		iSocket.Connect( iInetAddr, iStatus );	
		SetActive();

		// Print status
		iConsole->Printf(KConnnectedMessage, 
			&iConnectSettings->iAddress, 
			iConnectSettings->iPortNum, 
			&iConnectSettings->iPage );	
	
			
		return;
			
		
		}
	
	iCounter = 0;
	

	// Construct the TLS socket, to use the TLS1.0 protocol.
	// Specifying SSL3.0 would also use the same implementation
	iConsole->Printf(KSecureConnnectingMessage);
	_LIT(KTLS1,"TLS1.0");
	iTlsSocket = CSecureSocket::NewL( iSocket, KTLS1 );

	// Set any options before the handshake starts

	// Clears any previous options
	iTlsSocket->FlushSessionCache();
	
/*	Note: You could here set the available ciphers with code such as the following:
	TBuf8<2> buf;
	buf.SetLength(2);
	buf[0]=0; buf[1]=10;
	iTlsSocket->SetAvailableCipherSuites( buf ); */
	
	// start the handshake 
	iTlsSocket->StartClientHandshake( iStatus );

	iRunState = ESecureConnected;
	SetActive();
	}

void CSecEngine::MakePageRequestL()
	{
	// if secure connection failed, reconnect
	if(iStatus.Int() != KErrNone)
		{
		if(++iCounter >= KMaxAttempts)
			{
			User::Leave(iStatus.Int());
			}

		iRunState = ESocketConnected;
		iSocket.CancelAll();
//		iTlsSocket->CancelAll();
//		iTlsSocket->Close();
		delete iTlsSocket;
		iTlsSocket =0;
		iSocket.Close();
	
		// Open a TCP socket
		TInt ret = iSocket.Open( iSocketServ, KAfInet, KSockStream, KProtocolInetTcp );
		while(KErrNone != ret)
			{		
			iConsole->Printf(KFailSocketMessage, ret);			
			ret = iSocket.Open( iSocketServ, KAfInet, KSockStream, KProtocolInetTcp );
			}
		

		// Connect to the server, asynchronously
		iSocket.Connect( iInetAddr, iStatus );	
		SetActive();

		// Print status
		iConsole->Printf(KConnnectedMessage, 
			&iConnectSettings->iAddress, 
			iConnectSettings->iPortNum, 
			&iConnectSettings->iPage );	
	
			
		return;
			
		
		}
		
		
	iCounter = 0;
	
	iConsole->Printf(KGettingPageMessage);
	
	// Create a GET request	
	iSndBuffer+=KSimpleGet;
	iSndBuffer+=KUrl;
	iSndBuffer+=KNewLine;	
	iSndBuffer+=KHeader;	
	
	
	// Send the request
	iRunState = EGetRequestSent;
	iTlsSocket->Send( iSndBuffer, iStatus, iBytesSent );
	SetActive();
	}

void CSecEngine::GetServerResponseL()		
	{
	// The get request has been sent, can now try and receive the data
	User::LeaveIfError(iStatus.Int());
	iConsole->Printf(KReceivingMessage);
	
	// Print the cipher suite that has been negotiated.
	TBuf8<2> buf; 
	User::LeaveIfError(iTlsSocket->CurrentCipherSuite( buf ));
	PrintCipherNameL(buf);

	// Print the protocol version string
	TBuf<32> protocol;
	User::LeaveIfError(iTlsSocket->Protocol( protocol ));
	iConsole->Printf(KProtocolMessage, &protocol );
	
	// Print info about the server's certificate
	const CX509Certificate *servCert = iTlsSocket->ServerCert();
	if ( servCert ) PrintCertInfo( *servCert );
	
	// Read asynchonously-returns when buffer full
	iRunState = EDataReceived;
	iTlsSocket->Recv( iRcvBuffer, iStatus );
	SetActive();
	}

void CSecEngine::ReadServerResponseL()
	{
	// Any error other than KErrEof means the test is a failure
	if (iStatus!=KErrEof) User::LeaveIfError(iStatus.Int());
	iConsole->Printf(KReceivedMessage);

	// Put the received data in the output file & reset the receive buffer
	iTotalBytesRead += iRcvBuffer.Length();
	//TInt ret = iOutputFile->Write(iRcvBuffer);
	//if (ret != KErrNone) iConsole->Printf(KFileErrorMessage);
	
	// Case 1: error is KErrEof (message complete) or no data received, so stop
	if ( ( iStatus==KErrEof ) || ( iRcvBuffer.Length() == 0 ) )
		{
		iConsole->Printf(KCompleteMessage, iTotalBytesRead);
		// Close the socket neatly
		iRunState = EConnectionClosed;
		iTimer.After( iStatus, 1000000 );
		SetActive();
		return;	
		}

	// Case 2: there's more data to get from the server
	iRcvBuffer.SetLength( 0 );
	iRunState = EDataReceived;
	iTlsSocket->Recv( iRcvBuffer, iStatus );
	SetActive(); 
	}

void CSecEngine::ConnectionClosed()
	{
	if (!iInUse) return;
	// Clean up
	iHostResolver.Close();
	iSocket.CancelAll();
	//iTlsSocket->CancelAll();
	//iTlsSocket->Close();
	delete iTlsSocket;
	iTlsSocket =0;
	iSocket.Close();

	// Close the output file
	//iOutputFile->Close();

	// Wait here for an unload of the ssl.dll to make sure that a session is not
	// reconnected next time. 
	User::After( 1000000 );
	iInUse = EFalse;
	CActiveScheduler::Stop();
	}

void CSecEngine::PrintCipherNameL(const TDes8& aBuf)
	{
	TLex8 lex(aBuf);
	TUint cipherCode=aBuf[1];
	if ((cipherCode<1) || (cipherCode > 0x1B))
		User::Leave(KErrArgument);
	const TText* KCipherNameArray[0x1B] = 
		{
		_S("TLS_RSA_WITH_NULL_MD5"),
		_S("TLS_RSA_WITH_NULL_SHA"),
		_S("TLS_RSA_EXPORT_WITH_RC4_40_MD5"),
		_S("TLS_RSA_WITH_RC4_128_MD5"),
		_S("TLS_RSA_WITH_RC4_128_SHA"),
		_S("TLS_RSA_EXPORT_WITH_RC2_CBC_40_MD5"),
		_S("TLS_RSA_WITH_IDEA_CBC_SHA"),
		_S("TLS_RSA_EXPORT_WITH_DES40_CBC_SHA"),
		_S("TLS_RSA_WITH_DES_CBC_SHA"),
		_S("TLS_RSA_WITH_3DES_EDE_CBC_SHA"),
		_S("TLS_DH_DSS_EXPORT_WITH_DES40_CBC_SHA"),
		_S("TLS_DH_DSS_WITH_DES_CBC_SHA"),
		_S("TLS_DH_DSS_WITH_3DES_EDE_CBC_SHA"),
		_S("TLS_DH_RSA_EXPORT_WITH_DES40_CBC_SHA"),
		_S("TLS_DH_RSA_WITH_DES_CBC_SHA"),
		_S("TLS_DH_RSA_WITH_3DES_EDE_CBC_SHA"),
		_S("TLS_DHE_DSS_EXPORT_WITH_DES40_CBC_SHA"),
		_S("TLS_DHE_DSS_WITH_DES_CBC_SHA"),
		_S("TLS_DHE_DSS_WITH_3DES_EDE_CBC_SHA"),
		_S("TLS_DHE_RSA_EXPORT_WITH_DES40_CBC_SHA"),
		_S("TLS_DHE_RSA_WITH_DES_CBC_SHA"),
		_S("TLS_DHE_RSA_WITH_3DES_EDE_CBC_SHA"),
		_S("TLS_DH_anon_EXPORT_WITH_RC4_40_MD5"),
		_S("TLS_DH_anon_WITH_RC4_128_MD5"),
		_S("TLS_DH_anon_EXPORT_WITH_DES40_CBC_SHA"),
		_S("TLS_DH_anon_WITH_DES_CBC_SHA"),
		_S("TLS_DH_anon_WITH_3DES_EDE_CBC_SHA")
		};

	TPtrC name(KCipherNameArray[cipherCode-1]);
	iConsole->Printf(KCipherSuiteInUseMessage, &name );
	}

void CSecEngine::PrintCertInfo(const CX509Certificate& aSource)
	{
	_LIT(KCertInfoMessage1, "\nCertificate received: \n\tIssuer %S \n\tSubject %S");
	_LIT(KCertInfoMessage2, "\n\tValid from %S to %S");
	_LIT(KDateString,"%F%/0%M%/1%Y%/2%D%/3");

	TRAP_IGNORE(
		// Print issuer and subject
		HBufC* issuer = aSource.IssuerL();
		HBufC* subject = aSource.SubjectL();
		iConsole->Printf(KCertInfoMessage1, issuer, subject);
		delete issuer;
		delete subject;
		
		// Print validity period
		TBuf<20> startTime;
		TBuf<20> finishTime;
		aSource.ValidityPeriod().Start().FormatL(startTime,KDateString);
		aSource.ValidityPeriod().Finish().FormatL(finishTime,KDateString);
		iConsole->Printf(KCertInfoMessage2, &startTime, &finishTime);
		);
	}
	
//
// CTlsWatchdog
//


// Constructor should also call the parent constructor to set the priority
// of the active object.

CTlsWatchdog::CTlsWatchdog(CSecEngine* aEngine, CConsoleBase* aConsole) 
: CTimer(EPriorityStandard),iEngine(aEngine), iConsole(aConsole)
	{
	CActiveScheduler::Add( this );
	CTimer::ConstructL();
	}

CTlsWatchdog::~CTlsWatchdog()
	{
	// Cancel any outstanding request
	Cancel();
	
	}

void CTlsWatchdog::RunL()
	{
	iEngine->Cancel();
	delete iEngine;	
	}

