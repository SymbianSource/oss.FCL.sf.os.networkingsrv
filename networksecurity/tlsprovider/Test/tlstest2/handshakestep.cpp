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
 @file handshakestep.cpp
 @internalTechnology
*/
#include "handshakestep.h"

#include <tlsprovinterface.h>


_LIT(KWebAddress,"WebAddress");
_LIT(KWebPage,"WebPage");
_LIT(KPortNum,"PortNum");
_LIT(KExpectedFinalCipher, "ExpectedFinalCipherSuit");
_LIT(KExpectedSetCipherError, "ExpectedSetCipherError");
_LIT(KExpectedHandshakeError, "ExpectedHandshakeError");
_LIT8(KSimpleGet, "GET /");
_LIT8(KGetTail, " HTTP/1.0\r\n\r\n");
_LIT8(KExpectedPageContent, "Hello world");
_LIT8(KPSK_IDENTITY, "Client_identity");
_LIT8(KPSK_KEY, "0123456789"); 
_LIT(KNumServerNames, "NumServerNames");
_LIT(KServerNameBase, "ServerName");



// CertRequest tester active.
CHandShakeTesterActive::CHandShakeTesterActive( CTestExecuteLogger& aLogger ) : 
   CActive( EPriorityStandard ),
   iLogger( aLogger )
	{
	CActiveScheduler::Add( this );
	}
	
CHandShakeTesterActive::~CHandShakeTesterActive()
	{
   
	}

/** This is the core of the test, it handshakes a TLS test server, if succesfull it
    requests a web page from the TLS server, if the page was received succesfully it
    checks its content. Finally it checks the cipher suite used is what was expected. 
	@param None
	@return TVeredic EPass if all test sequence was completed an succesful.
	*/
TVerdict CHandShakeTesterActive::DoSecureConnectionTestL(CHandShakeStep* master)
	{
	TInt error_Returned;
	
	iStepPointer = master;
	// Negociates secure connection.
	error_Returned = HandShakeL();
	if (error_Returned != iStepPointer->iExpectedHandshakeError)
		{
		ERR_PRINTF3(_L("Handshake failed, Expected error: %D Received Error: %D"), iStepPointer->iExpectedHandshakeError, error_Returned);
		return EFail;
		} 
	else if( error_Returned != KErrNone) 
		{
		// An error was received but it was expected, communication can not continue as handshake failed.
		ERR_PRINTF2(_L("Test abbreviated as expected handshake error %D was received."), error_Returned);
		return EPass;  // Terminates test.	
		}
		
	// request a web page to server.
	if (MakePageRequest() != KErrNone)
		{
		ERR_PRINTF1(_L("Page request failed"));
		return EFail;
		} 

	// Invokes appropiate methods to receive web page.
	GetPageReceivedL();
		
	//	Verify that the page received has the expected content.
 	if (VerifyPageReceived() != KErrNone)
		{
		ERR_PRINTF1(_L("Page received not what expected"));
		return EFail;
		}   
		
	// debug, outputs page received to file.
	// OutputPageToFileL(iStepPointer->iRcvPage); // debug 	
		
	// 	Check that the cipher suite negociated on handshake is the one
	//  expected by the tester.
	if (VerifyFinalCipherUsed() != KErrNone)
		{
		ERR_PRINTF1(_L("The final cipher Suite used not what expected"));
		return EFail;
		}   
	return EPass; 
	}

void CHandShakeStep::GetPskL(const HBufC8 */*aPskIdentityHint*/, HBufC8 *&aPskIdentity, HBufC8 *&aPskKey)
	{   
    
	aPskIdentity = KPSK_IDENTITY().AllocL();
	aPskKey = KPSK_KEY().AllocL();

	return;
	}

/** Establishes secure connection to TLS test server by using 
    CSecureSocket::StartClientHandshake method.
	@param None
	@return TInt ,return error code returned by status of StartClientHandshake method.
	*/
TInt CHandShakeTesterActive::HandShakeL()
	{
	_LIT(KTLS1,"TLS1.0");
	iStepPointer->iTlsSocket = CSecureSocket::NewL( iStepPointer->iSocket, KTLS1 );

	// Clears any previous options
	iStepPointer->iTlsSocket->FlushSessionCache();

	// Sets Call back if needed.
	iStepPointer->SetPskL();
	
	// Sets Null cipher if needed.
	iStepPointer->SetNullCipherL();

	// No client authentication or dialogs for this test.
	iStepPointer->iTlsSocket->SetClientCertMode(EClientCertModeIgnore);
	iStepPointer->iTlsSocket->SetDialogMode(EDialogModeUnattended);
	
	// Sets Server Name
	if(iStepPointer->iUseServerNames)
		{
		TPckgC<CDesC8Array *> serverNamePkg(iStepPointer->iServerNamesArray);
		User::LeaveIfError(iStepPointer->iTlsSocket->SetOpt(KSoServerNameIndication, KSolInetSSL, serverNamePkg));	
		}

	// Alternatively dialog mode attended could be used for manual tests.
	// 	iStepPointer->iTlsSocket->SetClientCertMode(EClientCertModeOptional); // debug
	// 	iStepPointer->iTlsSocket->SetDialogMode(EDialogModeAttended); // debug

	// Set cipher suits to be presented in client hello message for handshake.
	TInt setCipherSuitesError;
	setCipherSuitesError = iStepPointer->iTlsSocket->SetAvailableCipherSuites( *(iStepPointer->iBufCipherSuitesFromIni));   

	if ( setCipherSuitesError != iStepPointer->iExpectedSetSuitesError)
		{
		ERR_PRINTF3(_L("The SetAvailableCipherSuites method returned error: %d but error expected was: %d"), setCipherSuitesError , iStepPointer->iExpectedSetSuitesError);
		User::Leave(KErrGeneral);	
		}
	// start the handshake 
	iStepPointer->iTlsSocket->StartClientHandshake( iStatus);  
	iState = ESecureConnected; 
	SetActive();
	CActiveScheduler::Start(); 

	return iStatus.Int();  
	}	

/** Requests web page to TLS test server. 
	@param None
	@return TInt ,return error code returned by status of CSecureSocket::Send method.
	*/
TInt  CHandShakeTesterActive::MakePageRequest()
	{
	// Create a GET request
	iStepPointer->iSndBuffer += KSimpleGet;
	iStepPointer->iSndBuffer += iStepPointer->iConnectSettings.iPage;
	iStepPointer->iSndBuffer += KGetTail;

	// Send the request
	iStepPointer->iTlsSocket->Send( iStepPointer->iSndBuffer, iStatus, iStepPointer->iBytesSent );
	iState = EGetRequestSent;
	SetActive();
	CActiveScheduler::Start();
	return iStatus.Int();  
	}

/** Receives web page previously requested to TLS test server. 
	@param None
	@return TInt ,return error code returned by status of CSecureSocket::Recv method.
	*/
void CHandShakeTesterActive::GetPageReceivedL()
	{
	// check for unexpected error.
	if(iStatus != KErrEof && iStatus != KErrNone)
		{
		ERR_PRINTF2(_L("Unexpected error received on get page requested: %d"), iStatus.Int());
		User::Leave(iStatus.Int());		
		}
	
	// First receive invoked (status equals to KErrNone)
	if(iStatus == KErrNone) 
		{
		iStepPointer->iRcvBuffer.SetLength( 0 );
		iStepPointer->iTlsSocket->Recv( iStepPointer->iRcvBuffer, iStatus );
		iState = EGetPageReceived;
		SetActive();
		CActiveScheduler::Start();
		INFO_PRINTF1(_L("New packet received (for page request)"));
		// Append received packet to page received so far. 
		iStepPointer->iRcvPage.Append(iStepPointer->iRcvBuffer);	
		}
	
	// KErrEof or buffer with length zero  received.
	if(iStatus == KErrEof || iStepPointer->iRcvBuffer.Length() == 0)
		{
		// Tells the state machine to stop requesting for more packets.
		iState = EIdle;
		return;
		}

	}

/** Compares the received page content against expected content.
    At present the expected content is hardcoded in constant KExpectedPageContent,
    if more sofisticated web pages checkings are needed test code may need to be modified to
    read expected content from binary file.  
	@param None
	@return KErrNone if webpage is what is expected , otherwise returns KErrGeneral.
	*/	
TInt CHandShakeTesterActive::VerifyPageReceived()
	{
	if( iStepPointer->iRcvPage == KExpectedPageContent)
		{
		return KErrNone;
		}
	else
		{
		return KErrGeneral;
		}
	}

/** Checks that the cipher suit agreed in the TLS handshake is what the tester
	was expecting.
   	@param None
	@return KErrNone if final cipher suite is what is expected , otherwise returns KErrGeneral.
	*/
TInt CHandShakeTesterActive::VerifyFinalCipherUsed()
	{
	TBuf8<2>	finalCipherBuffer; 
	TInt    	finalCipher=0;
	iStepPointer->iTlsSocket->CurrentCipherSuite(finalCipherBuffer);
	// Converts two bytes buffer in single integer.
	finalCipher += finalCipherBuffer[1];
	finalCipher += (finalCipherBuffer[0] << 8);

	if( iStepPointer->iExpectedFinalCipher == finalCipher)
		{
		return KErrNone;
		}
	else   
		{
		ERR_PRINTF3(_L("Used cipher: %d. Expected cipher: %d"), finalCipher, iStepPointer->iExpectedFinalCipher );
		return KErrGeneral;
		}
	}
	
void CHandShakeTesterActive::OutputPageToFileL(const TDesC8& aPageReceived)
	{
	RFs	fs;
	User::LeaveIfError (fs.Connect());
	TDriveUnit sysDrive (RFs::GetSystemDrive());
	TBuf<128> rName (sysDrive.Name());;
	rName.Append(_L("\\tlstest2\\myresults\\"));
			
	TInt err = fs.MkDir(rName);
	if (err != KErrNone && err != KErrAlreadyExists)
		{
		// This line will panic if directory does not exist.
		User::Leave(err);	
		}
				
	RFile file;
	CleanupClosePushL(file);
	
	_LIT(KExtension, ".html");
	rName.Append(iStepPointer->ConfigSection());
	rName.Append(KExtension);
	rName.LowerCase();
	User::LeaveIfError(file.Replace(fs, rName, EFileWrite | EFileStream));
	User::LeaveIfError(file.Write(aPageReceived));
	CleanupStack::PopAndDestroy(&file);
	fs.Close ();
	}

TInt CHandShakeTesterActive::RunError(TInt aError)
	{
	iRunError = aError;
	CActiveScheduler::Stop();
	return KErrNone;
	}



/** RunL method used for all asyncronous requests made by test code.
   	*/
void CHandShakeTesterActive::RunL()
	{
		iRunError =KErrNone;
	
		User::LeaveIfError(iStatus.Int());
	
		switch(iState)
			{
			case EIdle:
				CActiveScheduler::Stop();
				break;
			
			case ESecureConnected:
				INFO_PRINTF1(_L(" secure connection made "));
				iState = EIdle;
				CActiveScheduler::Stop();
				break;
			case EGetRequestSent:
				INFO_PRINTF1(_L(" Page requested "));
				iState = EIdle;
				CActiveScheduler::Stop();
				break; 
			case EGetPageReceived:
				// Keeps getting packages until KErrEof is received.  
				GetPageReceivedL();
				// This line will be reach only after all packages received.
				CActiveScheduler::Stop();
				break; 
			default:
				{
				INFO_PRINTF1(_L("HandShakeTesterActive: State corrupted."));
				User::Leave(KErrCorrupt);
				}
			} 
	return; 
	}

CHandShakeStep::CHandShakeStep()
	{
	SetTestStepName(KHandShakeTestStep);
	}

CHandShakeStep::~CHandShakeStep()
	{
	delete iBufCipherSuitesFromIni;
	iSndBuffer.Close();
	iRcvBuffer.Close();	
	iRcvPage.Close();
	if (iActiveObjTest)
		{
		delete iActiveObjTest;
		}
	}

/** Test preamble reads all ini values needed for test, these are:
 	NumCipherSuites: (int) The number of cipher suit to be included in client hello message.
   	CipherSuiteX: (hex) individual cipher suite to be included in client hello message.
	WebAddress: (string) address of the test server could be decimal doted or human readable.
	WebPage: (string) Test page to be requested to test server.
	PortNum: (int) listening port used by test server.
	ExpectedFinalCipherSuit: (hex) The cipher suite that is expected to be adopted on hnadshake.
	UsePsk: (Bool) Use to indicate if optional PSK cipher suites are to be included in list 
	of cipher suites send client hello message. 	
	*/
TVerdict CHandShakeStep::doTestStepPreambleL()
	{
	// Reads cipher suites to be sent in client hello message.
	iBufCipherSuitesFromIni = ReadCipherSuitesL();

	// Reads web address, this value is mandatory.
	TPtrC   webAddress;
	if(GetStringFromConfig(ConfigSection(), KWebAddress, webAddress))
		{
		iConnectSettings.iAddress = webAddress;
		}
	else
		{
		ERR_PRINTF1(_L("Failed to read web address value from INI file."));
		User::Leave(KErrNotFound);
		}

	// Reads page web to be requested, this value is mandatory
	TPtrC   page;
	if(GetStringFromConfig(ConfigSection(), KWebPage, page))
		{
		iConnectSettings.iPage.Copy(page) ;
		}
	else
		{
		ERR_PRINTF1(_L("Failed to read page value from INI file."));
		User::Leave(KErrNotFound);
		}

	// Reads port, this value is mandatory
	TInt   portNum;
	if(GetIntFromConfig(ConfigSection(), KPortNum, portNum))
		{
		iConnectSettings.iPortNum = portNum;
		}
	else
		{
		ERR_PRINTF1(_L("Failed to read Port Number from INI file"));
		User::Leave(KErrNotFound);
		}

	// Reads expected final cipher suits, mandatory value
	TInt cipher;	
	if(GetHexFromConfig(ConfigSection(), KExpectedFinalCipher, cipher))
		{
		iExpectedFinalCipher = cipher;
		}
	else
		{
		ERR_PRINTF1(_L("Failed to read Expect final cipher value from INI file."));
		User::Leave(KErrNotFound);
		}
		
	// Reads expected set cipher suites error, optional value
	TInt expectedSetSuitesError;	
	if(GetIntFromConfig(ConfigSection(), KExpectedSetCipherError, expectedSetSuitesError))
		{
		iExpectedSetSuitesError = expectedSetSuitesError;
		}
	else
		{
		iExpectedSetSuitesError = KErrNone; 
		}
		
	// Reads expected handshake  error, optional value
	TInt expectedHandshakeError;	
	if(GetIntFromConfig(ConfigSection(), KExpectedHandshakeError, expectedHandshakeError))
		{
		iExpectedHandshakeError = expectedHandshakeError;
		}
	else
		{
		iExpectedHandshakeError = KErrNone; 
		}
		

	// Initialises send and receive buffers
	iSndBuffer.CreateL(KSendBufferSize);
	iRcvBuffer.CreateL(KReceiveBufferSize);
	iRcvPage.CreateL(KReceiveBufferSize * 8); // Assumes 8 packages will be enough
		
	// Reads PSK options (optional)
	TBool option;	
	if(GetBoolFromConfig(ConfigSection(), KUsePsk, option))
		{
		iUsePsk = option;
		}
	else
		{
		iUsePsk = EFalse;
		} 
		
	// Reads Null cipher options (optional)
 	TBool nullCipher;	
	if(GetBoolFromConfig(ConfigSection(), KUseNullCipher, nullCipher))
		{
		iUseNullCipher = nullCipher;
		}
	else
		{
		iUseNullCipher = EFalse;
		}  
	
	// Read server names from INI file.	
	iServerNamesArray = ReadServerNamesL();
		
	return EPass;
	}

TVerdict CHandShakeStep::doTestStepL()
	{
	doTestL();
	return TestStepResult();
	}


TVerdict CHandShakeStep::doTestL()
	{
	TVerdict testResult;
    SetTestStepResult(EFail); 

	// Instantiates scheduler
 	iSched=new(ELeave) CActiveScheduler; 
    CleanupStack::PushL(iSched);  
	CActiveScheduler::Install(iSched);
	
	// Initialises and connects the RSocket
	ConnectL();
    
	// Creates active tester object which will deal will CSecureSocket instance 
	iActiveObjTest = new (ELeave) CHandShakeTesterActive(Logger());
	CleanupStack::PushL(iActiveObjTest);
	
	// Creates secure socket and start client handshake.
 	testResult = iActiveObjTest->DoSecureConnectionTestL(this);
	
	// Deletes and closes all production code been tested.
 	CloseConnection();

	CleanupStack::PopAndDestroy(iActiveObjTest);
	iActiveObjTest = NULL;
	CleanupStack::PopAndDestroy(iSched);
	iSched=NULL;
    
    SetTestStepResult(testResult);
	return TestStepResult();
	}


void  CHandShakeStep::SetPskL()
	{

	if(iUsePsk)
		{
		TPckgBuf<MSoPskKeyHandler *> pskConfigPkg;
		pskConfigPkg() = this;
		User::LeaveIfError(iTlsSocket->SetOpt(KSoPskConfig, KSolInetSSL, pskConfigPkg));
		}

	}	
	
void  CHandShakeStep::SetNullCipherL()
	{
	
		if(iUseNullCipher)
 		{
 		User::LeaveIfError(iTlsSocket->SetOpt(KSoEnableNullCiphers, KSolInetSSL, ETrue));
 		}
	
	}


void CHandShakeStep::CloseConnection()
	{

	// Clean up
	iSocket.CancelAll();
	if(iTlsSocket)
		{
		iTlsSocket->CancelAll();
		iTlsSocket->Close();
		delete iTlsSocket;
		iTlsSocket =0;
		}

	iSocket.Close();

	}

	
void CHandShakeStep::ConnectL()
	{

	TRequestStatus rs;
	RHostResolver hostResolver;
	
	// Connect the socket server
	User::LeaveIfError( iSocketServ.Connect());	
	
//	User::LeaveIfError( iSocketServ.Connect());	// debug

	// Interpret server address
	if (iInetAddr.Input(iConnectSettings.iAddress) != KErrNone)
		// Success if already in dotted-decimal format
		{
		// Connect to a host resolver (for DNS resolution) - happens sychronously
		TInt retVal = hostResolver.Open( iSocketServ, KAfInet, KProtocolInetTcp);
		
		if(retVal != KErrNone)
			{
			ERR_PRINTF2(_L("Failed to open host resolver, Error:%d"),retVal);
			User::Leave(KErrGeneral);
			} 	 

		CleanupClosePushL(hostResolver);

		// Try to resolve symbolic name
		TNameEntry nameEntry;

		retVal = hostResolver.GetByName( iConnectSettings.iAddress, nameEntry );
		if(retVal != KErrNone)
			{
			ERR_PRINTF2(_L("Failed to get name from host resolver, Error:%d"),retVal);
			User::Leave(KErrGeneral);
			} 

		TSockAddr sockAddr = nameEntry().iAddr;
		iInetAddr = iInetAddr.Cast( sockAddr );
		
		CleanupStack::PopAndDestroy(&hostResolver); 
		}
	// Store other connection parameters
	iInetAddr.SetPort( iConnectSettings.iPortNum );

	// Open a TCP socket
	User::LeaveIfError( iSocket.Open( iSocketServ, KAfInet, KSockStream, KProtocolInetTcp ) );	

	// Connect to the server, asynchronously
	iSocket.Connect( iInetAddr, rs );

	User::WaitForRequest( rs );
	// Print status	
	if(rs.Int() != KErrNone)
		{
		INFO_PRINTF4(_L("Connection failed to %S port:%d Error: %d"), &iConnectSettings.iAddress, iConnectSettings.iPortNum, rs.Int() );
		User::Leave(KErrCouldNotConnect);
		}
	else
		{
		INFO_PRINTF3(_L("Connected to %S port:%d"), &iConnectSettings.iAddress, iConnectSettings.iPortNum );
		}
	}
	
/** Read list of cipher to be included in client hello message.
@param None
@return HBufC8 list of cipher suites
*/

HBufC8* CHandShakeStep::ReadCipherSuitesL()
	{
	
	TInt numCipherSuites;
	TName  fCipherSuite;
	TInt   iniCipherSuite;  
	HBufC8* allSuites = NULL;
		
	if(GetIntFromConfig(ConfigSection(),KNumCipherSuites,numCipherSuites))
		{
		// Each cipher suite uses 2 bytes
		allSuites = HBufC8::NewMaxL( numCipherSuites * 2);
		TPtr8 ptr = allSuites->Des(); 
	
		for(TInt index = 1;index <= numCipherSuites ; ++index)
			{
			fCipherSuite.Format(_L("%S%d"), &KCipherSuiteBase, index);
			if(GetHexFromConfig(ConfigSection(), fCipherSuite,iniCipherSuite))
				{
				// Two bytes for each cipher suite
				ptr[(index - 1) * 2] =	( ( iniCipherSuite & 0xFF00) >> 8);
				ptr[( (index - 1) * 2 ) + 1] =	iniCipherSuite & 0xFF;
				}
			}
		}
	
	return allSuites;
	}  

/** Read list of server to be included in client hello message form INI file.
@param None
@return CDesC8ArrayFlat list of server names
*/


CDesC8ArrayFlat * CHandShakeStep::ReadServerNamesL()
	{
 
	TInt numServerNames;
	TName  fServerName;
	TPtrC  serverName;  
	CDesC8ArrayFlat* allServerNames = NULL;   
	
	if(GetIntFromConfig(ConfigSection(),KNumServerNames,numServerNames))
		{
		
		if( numServerNames >0)
			{
			iUseServerNames = ETrue;
			
			allServerNames = new(ELeave) CDesC8ArrayFlat(1);
			CleanupStack::PushL(allServerNames);
			
			// Reads each server name and appends it to array.
			for(TInt index = 1;index <= numServerNames ; ++index)
				{
				fServerName.Format(_L("%S%d"), &KServerNameBase, index);
				if(GetStringFromConfig(ConfigSection(), fServerName, serverName))
					{
					HBufC8 *serverName8bited = HBufC8::NewMaxLC(serverName.Length());
					serverName8bited->Des().Copy(serverName);
		 			allServerNames->AppendL(serverName8bited->Des());
		 			CleanupStack::PopAndDestroy(serverName8bited);
					}
				else
					{
					INFO_PRINTF2(_L("Could not found ServerName %d from INI file"), index );
					User::Leave(KErrGeneral);	
					}
				}
			CleanupStack::Pop(allServerNames);		
			}
	
		}
	else
		{
		iUseServerNames = EFalse; 	
		}
	 
	return allServerNames;
	}  
	
	

