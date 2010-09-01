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
 
#include "T_TLS_test.h"
#include <e32svr.h>
#include <ssl.h>

#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <ssl_internal.h>
#endif



const TInt KHexDumpWidth = 16;
const TInt KEAPKeyLength = 128+64;


TInt CTLSTest::RunError( TInt aError )
	{

	iTestStep->Log( _L("RunL err %d"), aError );

	if ( iTlsSocket )
		{
		iTlsSocket->Close();

		delete iTlsSocket;
		iTlsSocket = 0;

		
		}

	iInUse = EFalse;

	return KErrNone;
	}

CTLSTest* CTLSTest::NewL()
	{
	CTLSTest* self = new(ELeave) CTLSTest;
	CleanupStack::PushL( self );
	self->ConstructL();
	CleanupStack::Pop();		
	return self;
	}

// Constructor should also call the parent constructor to set the priority
// of the active object.
CTLSTest::CTLSTest() : CActive(0)
	{
	}

CTLSTest::~CTLSTest()
	{
	// Cancel any outstanding request
	Cancel();	

	delete iTlsSocket;
	delete iTlsSocket2;
	delete iGenericSocket;

	iTimer.Close();

	// Close the resources in the reverse order
	iHostResolver.Close();
	iSocket.Close();
	iSocketServ.Close();

//	__UHEAP_MARK; 
//	__UHEAP_MARKEND;
	}

void CTLSTest::ConstructL()
	{

	// Connect the socket server
	User::LeaveIfError( iSocketServ.Connect() );
	
	// Create a local timer
	User::LeaveIfError( iTimer.CreateLocal() );

	// Connect to a host resolver
	User::LeaveIfError( iHostResolver.Open( iSocketServ, KAfInet, KProtocolInetTcp ) );

	iRunState = ESocketConnected;
	iInUse = EFalse;

	CActiveScheduler::Add( this );
	}

void CTLSTest::RunL()
	{
	THTTPMessage *iMyHTTP;
	TInt err = KErrNone;
#ifdef __WINS__
	TText margin = 0;
	TText header = 0;
	TSockXfrLength aLen;
#endif

	switch ( iRunState )
		{
	case ESocketConnected:
		{
		iTestStep->Log( _L("STATE: ENotSecureConnected Status: %d"), iStatus.Int() );

		// As the receive buffer is static, set it's length to 0 here,
		// so if there is an error somewhere, the previous buffer's contents 
		// wont be dumped to the log file again.
		iRcvBuffer.SetLength( 0 );
		iBytesRead = 0;
		iTotalBytesRead = 0;
		iFirstRunFlag = ETrue;

		if ( iStatus != KErrNone )
			{
			iTestStep->Log( KStateErrConnected, iStatus.Int() );
			iRunState = EConnectionClosed;
			iTestPassed = EFalse;
			iTimer.After( iStatus, 1000000 );
			SetActive();			 
			break;
			}

		//=========================================================
		// Construct the Tls socket
		if ( !iTlsSocket )
			{
			TInt lcode = KErrNone;
			if(iUseGenericSocket)
				{
				iTestStep->Log( _L("Creating SecureSocket using Generic Socket") );
				TRAP(lcode, iTlsSocket = CSecureSocket::NewL( *iGenericSocket, iProtocol ) );
				}
			else
				{
				iTestStep->Log( _L("Creating SecureSocket using RSocket") );
				TRAP(lcode, iTlsSocket = CSecureSocket::NewL( iSocket, iProtocol ) );
				}
			if ( lcode )
				{ 
				iTestStep->Log( _L("Error creating secure socket: %d"), lcode );
				iRunState = EConnectionClosed;
				iTestPassed = EFalse;
				iTimer.After( iStatus, 1000000 );
				SetActive();			 
				break;
				}
			}

		//=========================================================

		if( iEAPKeyDerivation )
			{
			if(KeyDerivationTests(EFalse) != KErrNone)
				{
				iRunState = EConnectionClosed;
				iTestPassed = EFalse;
				iTimer.After( iStatus, 1000000 );
				SetActive();			 
				break;
				}
			}


		//======================================================================================
		// Set any options before the handshake starts
		if ( !iTlsSocket2 )
			{
			iTestStep->Log( _L("Flush session cache") );
			iTlsSocket->FlushSessionCache();
			}

		//=========================================================
		// Set the cipher suite list
		if ( iCipherSuites.Length() > 1 )
			{

			// Create the buffer with the cipher suites
			TBuf8<KCipherBufSize>	cipherBuf;
//			TBuf8<KCipherBufSize>	iCipherSuites;
			TBuf8<3>	tempBuf;
			TInt	i;
			TInt	value;
			TInt	ret = KErrNone;
			TInt	cCount = 0;			// used as an array index into the cipherBuf descriptor
			TLex8	myLex;


			// The cipher buffer will actually be the same length as the aCipherSuite passed in
			// because it's in binary format with leading 0's.
			cipherBuf.SetLength( iCipherSuites.Length() );

			iTestStep->Log( _L("Cipher suites") );

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
				iTestStep->Log( _L(":%X"), value );

				if ( value == 99 )
					{
					cipherBuf[ cCount++ ] = 0xFF;
					cipherBuf[ cCount++ ] = 0xFF;
					}
				else
					{
					// The actual cipher suite list that must be passed in the socket options
					// is in a binary format of 0x0,0xCipherValue,0x0,CipherValue etc				
					cipherBuf[ cCount++ ] = 0;
					cipherBuf[ cCount++ ] = (unsigned char)value;
					}

				} // end of for loop

			if ( cipherBuf.Length() )
				{
#ifdef __WINS__
				iTestStep->Log( _L("Setting avilable cipher suites") );
				HexDump( &header, &margin, &cipherBuf[0], cipherBuf.Length() );
#endif

				ret = iTlsSocket->SetOpt(KSoEnableNullCiphers, KSolInetSSL, ETrue);
				
				if ( ret != KErrNone )
					{
					TPtrC errorText = iTestStep->EpocErrorToText(ret);
					iTestStep->Log( _L("Couldnt set NULL cipher suite option :%S  (%d)"),&errorText, ret );
					iTestPassed = EFalse;
					}

				ret = iTlsSocket->SetAvailableCipherSuites( cipherBuf );

				if ( ret != KErrNone )
					{
					TPtrC errorText = iTestStep->EpocErrorToText(ret);
					iTestStep->Log( _L("Couldnt set available cipher suites error:%S  (%d)"),&errorText, ret );
					iTestPassed = EFalse;
					}
				}

			iTestStep->Log( _L("") );
			} // end of if iciphersuites .length

		// end of setting the cipher suite list
		//=========================================================


		TBuf<8> prot;
		err = iTlsSocket->Protocol( prot );
    	
		//*****************************************************
		if ( !err )
			{
			iTestStep->Log( _L("Protocol set for use:") );
			iTestStep->Log( prot );
			}

		err = iTlsSocket->SetProtocol( prot );
		if ( err )
			{
			iTestStep->Log( _L("Failed to set Protocol for use: (%d)"),err );
			}
#ifdef HTTP_ALLOW_UNTRUSTED_CERTIFICATES
		if(iTLSDialogMode) 
		    {
            iStatus = iTlsSocket->SetOpt(KSoDialogMode,KSolInetSSL,iTLSDialogModeValue);
            if ( iStatus != KErrNone )
                {
                if(iExpectedErrorCode == iStatus.Int())
                    {
                    iTestStep->Log( _L("iTlsSocket->SetOpt() failed with KErrArgument because of invalid DialogModeValue "));
                    iRunState = EConnectionClosed;
                    iTestPassed = ETrue;
                    }
                else
                    {
                    iTestStep->Log( _L("iTlsSocket->SetOpt() failed with error: %d"), iStatus.Int());
                    }
                iTlsSocket->Close();                 
                iTimer.After( iStatus, 1000000 );
                SetActive();
                break;
                }
		    }
#endif  // HTTP_ALLOW_UNTRUSTED_CERTIFICATES
		// Set the domain name we're connecting to...
		iStatus = iTlsSocket->SetOpt(KSoSSLDomainName,KSolInetSSL, iDNSName);
		if ( iStatus != KErrNone )
			{
			iTestStep->Log( KStateErrReceivePage, iStatus.Int() );
			iTestStep->Log( KStateErrReceivePage, iStatus.Int() );
			iRunState = EConnectionClosed;
			iTestPassed = EFalse;
			iTlsSocket->Close();				 
			iTimer.After( iStatus, 1000000 );
			}
			// end of setting options
			//======================================================================================
		else
			{
			//=========================================================
			// start the handshake 
			iTlsSocket->StartClientHandshake( iStatus );
			//=========================================================
			iRunState = ESecureConnected;
			}

		SetActive();
		break;
		}

	case ESecureConnected:
	case ESecureRenegotiated:
		{
		iTestStep->Log( _L("STATE: ESecureConnected/ESecureRenegotiated Status: %d"), iStatus.Int() );

		// The secure connection has now been made.
		// Send a get request for the page.

		// Send the get request




		if ( iStatus != KErrNone )
			{
#ifdef HTTP_ALLOW_UNTRUSTED_CERTIFICATES
            if(iExpectedErrorCode == iStatus.Int())
                {
                iTestStep->Log( _L("Received Untrusted CA"));
                iRunState = EConnectionClosed;
                iTestPassed = ETrue;
                }
            else
                {
#endif  // HTTP_ALLOW_UNTRUSTED_CERTIFICATES
                TPtrC errorText = iTestStep->EpocErrorToText(iStatus.Int());
                iTestStep->Log( _L("ESecureConnected:%S %d"),&errorText, iStatus.Int() );
                iTestStep->Log( KStateErrReceivePage, iStatus.Int() );
                iTestStep->Log( KStateErrReceivePage, iStatus.Int() );
                iRunState = EConnectionClosed;
                iTestPassed = EFalse;
#ifdef HTTP_ALLOW_UNTRUSTED_CERTIFICATES
                }
#endif  // HTTP_ALLOW_UNTRUSTED_CERTIFICATES
                iTlsSocket->Close();                 
                iTimer.After( iStatus, 1000000 );
                SetActive();
                break;                
            }
		else
			{
			iTestStep->Log( _L("ESecureConnected:KErrNone %d"),iStatus.Int() );
			}

		TDes8* pSendDes;
		if ( !iTlsSocket2 )
			{
			pSendDes = &iRcvBuffer;
			iRcvBuffer.SetLength( iRcvBuffer.MaxLength() );
			iTestStep->Log( _L("Sending %d bytes"), iRcvBuffer.Length() );
			}
		else
			{
			pSendDes = &iSndBuffer;
			// Can a simple "GET /page.htm" request be used? 
			if ( iSimpleGet )
				{
				iSndBuffer.Copy( _L("GET ") );
				iSndBuffer.Append( iPage );
				iSndBuffer.Append( _L("") );
				iTestStep->Log( _L("Using simple get") );
				}
			else
				{
				// build and send a HTTP GET request to retrieve a page
				iMyHTTP = new THTTPMessage;
				iMyHTTP->Method(_L8("GET"));
				iMyHTTP->URI( iPage );
				iMyHTTP->AddHeaderField(_L8("Connection"),_L8("close"));
				iMyHTTP->AddHeaderField(_L8("User-Agent"),_L8("SSL_TEST"));
				iMyHTTP->AddHeaderField(_L8("Accept-Encoding"));
				iMyHTTP->AddHeaderField(_L8("Accept"),_L8("*/*"));
				iMyHTTP->GetHeader(iSndBuffer);
				delete iMyHTTP;
				}

			iTestStep->Log( KLogSendingRequest );
#ifdef __WINS__
			HexDump( &header, &margin, &iSndBuffer[0], iSndBuffer.Length() );
#endif
			}

		iRunState = iRunState == ESecureRenegotiated ? EGetRequestSentReneg : EGetRequestSent;

		//=========================================================
		// send the request
		iTlsSocket->Send( *pSendDes, iStatus, iBytesSent );
		//=========================================================	

		if( iEAPKeyDerivation )
			{
			if(KeyDerivationTests(ETrue) != KErrNone)
				{
				iRunState = EConnectionClosed;
				iTestPassed = EFalse;
				iTimer.After( iStatus, 1000000 );
				SetActive();			 
				break;
				}
			}
		
		SetActive();
		break;
		}

	case EGetRequestSent:
	case EGetRequestSentReneg:
		{

		// The get request has been sent, can now try and receive the data

		iTestStep->Log( _L("STATE: EGetRequestSent/EGetRequestSentReneg Status: %d"), iStatus.Int() );

		if ( iStatus != KErrNone )
			{
			iTestStep->Log( _L("EGetRequestSent: %d"), iStatus.Int() );
			iRunState = EConnectionClosed;
			iTestPassed = EFalse;
			iTlsSocket->Close();
			iTimer.After( iStatus, 1000000 );
			SetActive();			 
			break;
			}

		// Before that, check which cipher suite has been negotiated.
		TBuf8<4> buf; 
		// no need to set descriptor length as in case with the old getopt, 
		// CurrentCipherSuite will do that
		err = iTlsSocket->CurrentCipherSuite( buf );
		if ( err )
			{
			iTestStep->Log( _L("CurrentCipherSuite: %d"), err );
			}
		else
			{
#ifdef __WINS__
			iTestStep->Log( _L("Cipher suite in use:") );
			HexDump( &header, &margin, &buf[0], buf.Length() );
#endif
			}

		// Get the servers certificate
		const CX509Certificate *servCert = iTlsSocket->ServerCert();

		if ( servCert )
			{
			GetCertInfo( *servCert );
			}
		else
			{
			iTestStep->Log( _L("No server certificate is available") );
			}

		// Get the protocol version string
		TBuf<32> protocol;

		err = iTlsSocket->Protocol( protocol );

		if ( !err )
			{
			iTestStep->Log( _L("Protocol used in connection:") );
			iTestStep->Log( protocol );
			}

		// New tests added as part of GT167 Zephyr.
		// These tests are not meant to be in a logical order. They simply exercise the 
		// Secure socket API. Logical ordering of the tests should happen in further
		// development of the TLS test code and all supported APIS should be called/tested.

		// Get the Client certificate
		const CX509Certificate *clientCert = iTlsSocket->ClientCert();

		if ( clientCert )
			{
			GetCertInfo( *clientCert );
			}
		else
			{
			iTestStep->Log( _L("No client certificate is available") );
			}

		// Get the Client cert mode. This API is NOT supported in client mode.
		TClientCertMode certMode = iTlsSocket->ClientCertMode();
		if ( certMode )
			{
			iTestStep->Log( _L("ClientCertMode() is not supported, Default value is: %d"), EClientCertModeIgnore );
			iTestStep->Log( _L("Current mode setting is: %d"), certMode );			
			}

		iTestStep->Log( _L("Set Client Cert mode to: %d"), EClientCertModeOptional );
		err = iTlsSocket->SetClientCertMode(EClientCertModeOptional);
		if (err != KErrNotSupported)
			{
			iTestStep->Log( _L("SetClientCertMode Failed: %d"), err );
			}

		// Get the Dialog mode.
		TDialogMode dialogMode = iTlsSocket->DialogMode();
		iTestStep->Log( _L("Dialog mode setting is: %d"), dialogMode );			

		// GetOpt() API: Current cipher suite option
		TUint optionName = KSoCurrentCipherSuite;
		TUint optionLevel = KSolInetSSL;
		TBuf8<30> option;  // Note that the size of 30 is arbitrary.

		TInt retValue = iTlsSocket->GetOpt(optionName, optionLevel, option);

		// GetOpt() API: Available cipher cipher suites option
		option.FillZ();		// reset the descriptor before reuse
		option.Zero();	
		optionName = KSoAvailableCipherSuites;
		retValue = iTlsSocket->GetOpt(optionName, optionLevel, option);

		// GetOpt() API: Dialog mode option
		option.FillZ();		// reset the descriptor before reuse
		option.Zero();	
		optionName = KSoDialogMode;
		retValue = iTlsSocket->GetOpt(optionName, optionLevel, option);

		// GetOpt() API: Server certificate option
		option.FillZ();		// reset the descriptor before reuse
		option.Zero();	
		optionName = KSoSSLServerCert;
		//		retValue = iTlsSocket->GetOpt(optionName, optionLevel, option);

		// SetOpt() API: Dialog mode option
		option.FillZ();		// reset the descriptor before reuse
		option.Zero();	
		optionName = KSoDialogMode;
		//		retValue = iTlsSocket->SetOpt(optionName, optionLevel, option);
		// End of the addition of new tests for GT167 Zephyr

		iRunState = iRunState == EGetRequestSent ? EDataReceived : EDataReceivedReneg;
		if ( iTlsSocket2 )
			{
			iRcvBuffer.Zero();
			iTlsSocket->Recv( iRcvBuffer, iStatus );
			SetActive();
			break;
			}
		iRcvBuffer.Zero();

		if( iEAPKeyDerivation )
			{
			if(KeyDerivationTests(ETrue) != KErrNone)
				{
				iRunState = EConnectionClosed;
				iTestPassed = EFalse;
				iTlsSocket->Close();
				iTimer.After( iStatus, 1000000 );
				SetActive();			 
				break;
				}
			}

		// Fall through
		}

	case EDataReceived:
	case EDataReceivedReneg:
		{
		iTestStep->Log( _L("STATE: EDataReceived/EDataReceivedReneg Status: %d"), iStatus.Int() );
		iTestStep->Log( _L("EDataReceived length:%d"), iRcvBuffer.Length() );		

		// Any error other than KErrEof means the test is a failure
		if ( iStatus!=KErrNone && iStatus!=KErrEof)
			{
			// Close the socket neatly
			iRunState = EConnectionClosed;
			iTlsSocket->Close();
			iTimer.After( iStatus, 1000000 );
			SetActive();
			break;	
			}

		// Log the received buffer
		if ( iRcvBuffer.Length() )
			{
			iTestStep->Log( KLogBytesRead, iRcvBuffer.Length() );
#ifdef __WINS__
			HexDump( &header, &margin, &iRcvBuffer[0], iRcvBuffer.Length() );
#endif
			iTotalBytesRead += iRcvBuffer.Length();
			if ( iStatus==KErrEof  )
				{
				iRunState = EConnectionClosed;
				iTlsSocket->Close();
				iTimer.After( iStatus, 1000000 );

				if( iEAPKeyDerivation )
					{
					if(KeyDerivationTests(EFalse) != KErrNone)
						{
						iTestPassed = EFalse;
						}
					}
				}				
			else
				{
				//read again
				iRcvBuffer.SetLength( 0 );
				iTlsSocket->Recv( iRcvBuffer, iStatus );
				}

			SetActive(); 
			break;
			}
		else
			{
			if ( iRunState == EDataReceived )
				{
				iRunState = ESecureRenegotiated;
				iTestStep->Log( _L("******* Trying renegotiate *******") );
				iTlsSocket->FlushSessionCache();
				iTlsSocket->RenegotiateHandshake( iStatus );

				if( iEAPKeyDerivation )
					{
					if(KeyDerivationTests(ETrue) != KErrNone)
						{
						iRunState = EConnectionClosed;
						iTestPassed = EFalse;
						iTlsSocket->Close();
						iTimer.After( iStatus, 1000000 );
						SetActive();			 
						break;
						}
					}

				SetActive(); 
				break;
				}
			else
				{
				iRunState = EConnectionClosed;
				}
			}

		// Fall through
		}

	case EConnectionClosed:
		{
		iTestStep->Log( _L("STATE: EConnectionClose Status: %d"), iStatus.Int() );

		if ( iStatus != KErrNone )
			{
			iTestStep->Log( KStateErrFinished, iStatus.Int() );
			iTestStep->Log( KStateErrFinished, iStatus.Int() );
			iTestPassed = EFalse;
			}

		if ( iTotalBytesRead )
			{
			iTestStep->Log( KLogBytesRead, iTotalBytesRead );
			}

		if ( iTestPassed )
			{
			iTestStep->iTestStepResult = EPass;
			iTestStep->Log( KLogTestPassed );
			}
		else
			{
			iTestStep->iTestStepResult = EFail;
			iTestStep->Log( KLogTestFailed );
			}

		if ( iTlsSocket2 ) //to test abreviatted handshake
			{
			iInUse = EFalse;
			iTlsSocket->Close();
			delete iTlsSocket;
			iTlsSocket =0;
			delete iTlsSocket2;
			iTlsSocket2 =0;
			}
		else if ( iTlsSocket )
			{//keep the first instance live not to destroy the session cache
			iTlsSocket2 = iTlsSocket;
			iTlsSocket = NULL;
			iTestStep->Log(_L("Connecting to %s:%d"), iAddress.PtrZ(), iPortNum );
			iTlsSocket2->Close();
			User::LeaveIfError( iSocket.Open( iSocketServ, KAfInet, KSockStream, KProtocolInetTcp ) );	
			iSocket.Connect( iInetAddr, iStatus );
			SetActive();
			iRunState = ESocketConnected;
			}
		else
			{
			iInUse = EFalse;
			}

		// can wait here for an unload of the ssl.dll to make sure that a session is not
		// reconnected next time. The option to set the unload timeout via a SetOpt
		// may not currently work, so this is a sure way to stop a reconnection.
		if ( iTestEndDelay )
			{
			iTestStep->Log( _L("Waiting for %d seconds"), iTestEndDelay );
			User::After( 1000000 * iTestEndDelay );
			}

		break;
		}

	case EDummyConnection:
		iTestStep->Log( _L("STATE: EDummyConnection Status: %d"), iStatus.Int() );
		iInUse = EFalse;
		break;

		// end switch
		}
	}

void CTLSTest::DoCancel()
	{	
	// Cancel the connect
	iSocket.CancelConnect();
	}
#ifdef HTTP_ALLOW_UNTRUSTED_CERTIFICATES
void CTLSTest::ConnectL( const TDesC &aAddress, 
				const TInt aPortNum, 
				const TDesC &aPage, 
				const TDesC8 &aCipherSuite, 
				const TInt aCipher, 
				const TInt aSimpleGet, 
				const TInt aTestEndDelay, 
				const TDesC8& aDNSName ,
				const TDesC& aProtocol, 
				TBool aUseGenericSocket, 
				TBool aEAPKeyDerivation,
				TBool aTLSDialogMode,
				TInt aTLSDialogModeValue,
				TInt aExpectedErrorCode )
#else
void CTLSTest::ConnectL( const TDesC &aAddress, 
                const TInt aPortNum, 
                const TDesC &aPage, 
                const TDesC8 &aCipherSuite, 
                const TInt aCipher, 
                const TInt aSimpleGet, 
                const TInt aTestEndDelay, 
                const TDesC8& aDNSName ,
                const TDesC& aProtocol, 
                TBool aUseGenericSocket, 
                TBool aEAPKeyDerivation )
#endif  // HTTP_ALLOW_UNTRUSTED_CERTIFICATES
	{
#if 0
	iRunState = EDummyConnection;
	TRequestStatus* p=&iStatus;
	User::RequestComplete( p, KErrNone );
   SetActive();
   return;
#endif

	TInt err;
	TNameEntry nameEntry;
	TNameRecord nameRecord;
	TSockAddr sockAddr;

	iInUse = ETrue;

	err = iInetAddr.Input( aAddress );
	if ( err != KErrNone )
		{
		err = iHostResolver.GetByName( aAddress, nameEntry );
		if ( err == KErrNone )
			{
			nameRecord = nameEntry();
			sockAddr = nameRecord.iAddr;
			iInetAddr = iInetAddr.Cast( sockAddr );
			}
		}

	iInetAddr.SetPort( aPortNum );
	iAddress = aAddress;
	iDNSName = aDNSName;
	iPortNum = aPortNum;
	iCipherSuites.Copy( aCipherSuite );
	iProtocol.Copy( aProtocol );
	iCipher = aCipher;
	iPage.Copy( aPage );
	iSimpleGet = aSimpleGet;
	iTestEndDelay = aTestEndDelay;
	iUseGenericSocket = aUseGenericSocket;
	iEAPKeyDerivation = aEAPKeyDerivation;
#ifdef HTTP_ALLOW_UNTRUSTED_CERTIFICATES
	iTLSDialogMode = aTLSDialogMode;
	iTLSDialogModeValue = aTLSDialogModeValue;
	iExpectedErrorCode = aExpectedErrorCode;
#endif  // HTTP_ALLOW_UNTRUSTED_CERTIFICATES
	// Print info to the log
	iTestStep->Log( _L("*****Connecting to*****") );
	iTestStep->Log( _L("%s:%d "), iAddress.PtrZ(), iPortNum ); 


	// Open the socket
	User::LeaveIfError( iSocket.Open( iSocketServ, KAfInet, KSockStream, KProtocolInetTcp ) );	
	
	if(iUseGenericSocket)
		{
		iGenericSocket = new(ELeave)CGenericSecureSocket<RSocket>(iSocket);
		}

	iTestPassed = ETrue;
	iCiphersMatch = EFalse;

	iRunState = ESocketConnected;
	
	// Open the socket
	User::LeaveIfError( iInetAddr.Input( iAddress ));
	iInetAddr.SetPort( iPortNum );

	iTestStep->Log(_L("Connecting to %s:%d"), iAddress.PtrZ(), iPortNum );
	iSocket.Connect( iInetAddr, iStatus );	

	SetActive();
	}

void CTLSTest::SetConsole( CTestStepTls * aTestStep )
	{
	iTestStep = aTestStep;
	}

TBool CTLSTest::InUse()
	{
	return iInUse;
	}

TBool CTLSTest::TestingSite( const TDesC &aAddress, const TInt aPortNum )
	{
		TInt match = iAddress.Compare( aAddress );

		if ( ( aPortNum == iPortNum ) && ( match == 0 ) && ( iInUse ) )
			return ETrue ;
		else 
			return EFalse;
	}

void CTLSTest::HexDump(const TText* aHeader, const TText* aMargin, const TUint8* aPtr, TInt aLen)
	{

	TBuf<0x100> buf;
	buf.SetLength(0);
	TInt i = 0;
	const TText* p = aHeader;
	while (aLen>0)
		{
		TInt n = aLen>KHexDumpWidth ? KHexDumpWidth : aLen;
		buf.AppendFormat(_L("%s%04x : "), p, i);
		TInt j;
		for (j=0; j<n; j++)
			buf.AppendFormat(_L("%02x "), aPtr[i+j]);
		while (j++<KHexDumpWidth)
			buf.AppendFormat(_L("   "));
		buf.AppendFormat(_L(" "));
		for (j=0; j<n; j++)
			{
			TUint8 byteAsChar;
			byteAsChar = aPtr[i+j]<32 || aPtr[i+j]>126 ? '.' : aPtr[i+j];
			if ((byteAsChar == '%') || (byteAsChar == '<'))
				byteAsChar = '.';
			buf.AppendFormat(_L("%c"), byteAsChar );
			}
		iTestStep->Log(buf);
		buf.SetLength(0);
		aLen -= n;
		i += n;
		p = aMargin;
		}
	}



TInt CTLSTest::GetCertInfo(const CX509Certificate& aSource)
	{
	TText margin = 0;
	TText header = 0;

	iTestStep->Log( _L("------Certificate Info------") );

	iTestStep->Log( _L("FingerPrint") );
	HexDump( &header, &margin, aSource.Fingerprint().Ptr(), aSource.Fingerprint().Length() );

	iTestStep->Log( _L("SerialNumber") );
	HexDump( &header, &margin, aSource.SerialNumber().Ptr(), aSource.SerialNumber().Length() );
							   
	const CSubjectPublicKeyInfo& publicKeyInfo = aSource.PublicKey();
	iTestStep->Log( _L("PublicKeyInfo") );
	HexDump( &header, &margin, publicKeyInfo.KeyData().Ptr(), publicKeyInfo.KeyData().Length() );

	iTestStep->Log( _L("----------------------------") );

	return KErrNone;
	}


TInt CTLSTest::KeyDerivationTests(TBool aSocketOpen)
	{
	if( aSocketOpen )
		{
		// Input should be no longer than 100 bytes
		iTestStep->Log( _L("Attempt to get EAP Keys with too large an input descriptor") );
		TBuf8<KEAPKeyLength> tooBig;
		tooBig.Fill('X',101);
		TInt err = iTlsSocket->GetOpt(KSoKeyingMaterial, KSolInetSSL, tooBig);
		if(err != KErrArgument)
			{
			iTestStep->Log( _L("FAILED: Did not return KErrArgument.  Error: %d"), err );
			if(err == KErrNone)
				{
				err = KErrGeneral;
				}
			return err;
			}
		iTestStep->Log( _L("Passed") );

		// Descriptor should be of size to fit 128+64 bytes
		iTestStep->Log( _L("Attempt to get EAP Keys with too small an input descriptor") );
		TBuf8<KEAPKeyLength-5> tooSmall;
		err = iTlsSocket->GetOpt(KSoKeyingMaterial, KSolInetSSL, tooSmall);
		if(err != KErrArgument)
			{
			iTestStep->Log( _L("FAILED: Did not return KErrArgument.  Error: %d"), err );
			if(err == KErrNone)
				{
				err = KErrGeneral;
				}
			return err;
			}
		iTestStep->Log( _L("Passed") );

		// Should be able to generate key
		iTestStep->Log( _L("Attempt to get EAP Keys with a valid input descriptor") );
		TBuf8<KEAPKeyLength+5> retVal;
		err = iTlsSocket->GetOpt(KSoKeyingMaterial, KSolInetSSL, retVal);
		if(err != KErrNone)
			{
			iTestStep->Log( _L("FAILED: Call failed with error: %d"), err );
			if(err == KErrNone)
				{
				err = KErrGeneral;
				}
			return err;
			}
		if(retVal.Length() != KEAPKeyLength)
			{
			iTestStep->Log( _L("FAILED: Call OK but Key returned is the wrong size.  Expected=%d Actual=%d"), 128+64, retVal.Length() );
			err = KErrGeneral;
			return err;
			}
		iTestStep->Log( _L("Passed") );
		}
	else
		{
		// Can only generate keys whilst have an open socket that has completed the SSL handshake
		iTestStep->Log( _L("Attempt to get EAP Keys before handshake/after socket closed") );
		TBuf8<KEAPKeyLength> retVal;
		TInt err = iTlsSocket->GetOpt(KSoKeyingMaterial, KSolInetSSL, retVal);
		if(err != KErrNotReady)
			{
			iTestStep->Log( _L("FAILED: Did not return KErrNotReady.  Error: %d"), err );
			if(err == KErrNone) err = KErrGeneral;
			return err;
			}
		iTestStep->Log( _L("Passed") );
		}

	return KErrNone;
	}
