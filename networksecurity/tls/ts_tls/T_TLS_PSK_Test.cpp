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
 @file T_TLS_PSK_Test.cpp
 @internalTechnology
*/

#include "T_TLS_PSK_Test.h"
#include <tlsprovinterface.h>

/** Constructor: Test class
	Needs to call the base class Constructor setting the priority of the active object 
	*/
CTlsPskTest::CTlsPskTest():CActive( EPriorityStandard )
   	{
	CActiveScheduler::Add( this );
	}
	
CTlsPskTest::~CTlsPskTest()
	{
    }

/** Test activities
	1. Starts the handshake with TLS test server.
	2. Requests a web page from TLS test server if start succeeds
	3. Checks the page content if web page retrival succeeds
	4. Checks the cipher suite used against the against the one expected

	@param CTlsPskTestStep pointer
	@return TVeredic EPass if all test sequence was completed an succesful.
	*/
TVerdict CTlsPskTest::DoSecureConnectionTestL(CTlsPskTestStep* aMaster)
	{
	
	iStepPointer = aMaster;

	// Starts the handshake with TLS test server. 
	TInt error_Returned = HandShakeL();

	//Check to see if handshake has succeeded
	if (error_Returned != iStepPointer->iExpectedHandshakeError)
		{
		iStepPointer->LogEvent( _L("Handshake failed") );
		return EFail;
		} 
	else if( error_Returned != KErrNone) 
		{
		//Error received, handshake with test TLS server failed
		iStepPointer->LogEvent( _L("Test abbreviated as expected handshake error was received."));
		return EPass;  //Test case terminates
		}
		
	// Request a web page from TLS test server as start succeeded
	if (MakePageRequest() != KErrNone)
		{
		iStepPointer->LogEvent( _L("Making a page request failed."));
		return EFail;
		} 

	// Retrive the receive webpage
	error_Returned = GetPageReceived();
 	if (error_Returned != KErrEof)
		{
		iStepPointer->LogEvent( _L("Page reception failed."));
		return EFail;
		} 
		
	//	Check the page content as web page retrival succeeded
	if (VerifyPageReceived() != KErrNone)
		{
		iStepPointer->LogEvent( _L("Page verification failed, received not what expected."));
		return EFail;
		}
  
	//	Checks the cipher suite used against the against the one expected
	if (VerifyFinalCipherUsed() != KErrNone)
		{
		iStepPointer->LogEvent( _L("The final cipher Suite used not what expected."));
		return EFail;
		}   
	return EPass; 
	}

/** Get the PSK client identity from the ini files
	Get the Pre Shared Key value from the ini files.

	*/	
 void CTlsPskTestStep::GetPskL(const HBufC8 */*aPskIdentityHint*/, HBufC8 *&aPskIdentity, HBufC8 *&aPskKey)
	{   
    
	aPskIdentity = KPSK_IDENTITY().AllocL();
	aPskKey = KPSK_KEY().AllocL();

	return;
	}

/** Establishes secure connection to TLS test server by using CSecureSocket::StartClientHandshake method.
	
	@param None
	@return TInt ,return error code returned by status of StartClientHandshake method.
	*/
TInt CTlsPskTest::HandShakeL()
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

	// Set cipher suits to be presented in client hello message for handshake.
	TInt setCipherSuitesError;
	setCipherSuitesError = iStepPointer->iTlsSocket->SetAvailableCipherSuites( *(iStepPointer->iBufCipherSuitesFromIni));   

	if ( setCipherSuitesError != iStepPointer->iExpectedSetSuitesError)
		{
		iStepPointer->LogEvent( _L("The SetAvailableCipherSuites method returned wrong error") );
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
TInt  CTlsPskTest::MakePageRequest()
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
TInt CTlsPskTest::GetPageReceived()
	{
	iStepPointer->iRcvBuffer.SetLength( 0 );
	iStepPointer->iTlsSocket->Recv( iStepPointer->iRcvBuffer, iStatus );
	iState = EGetPageReceived;
	SetActive();
	CActiveScheduler::Start();
	return iStatus.Int();  
	}

/** Compares the received page content against expected content.
    At present the expected content is hardcoded in constant KExpectedPageContent,
    if more sofisticated web pages checkings are needed test code may need to be modified to
    read expected content from binary file.
  
	@param None
	@return KErrNone if webpage is what is expected , otherwise returns KErrGeneral.
	*/	
TInt CTlsPskTest::VerifyPageReceived()
	{

	if( iStepPointer->iRcvBuffer == KExpectedPageContent)
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
TInt CTlsPskTest::VerifyFinalCipherUsed()
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
		iStepPointer->LogEvent( _L("Final cipher suite and expected differ") );
		return KErrGeneral;
		}
	}

TInt CTlsPskTest::RunError(TInt aError)
	{
	iRunError = aError;
	CActiveScheduler::Stop();
	return KErrNone;
	}



/** RunL method used for all asyncronous requests made by test code.
   	*/
void CTlsPskTest::RunL()
	{
		iRunError =KErrNone;
	
		User::LeaveIfError(iStatus.Int());
	
		switch(iState)
			{
			case ESecureConnected:
				iStepPointer->LogEvent( _L("Secure connection made the remote server.") );
				iState = EIdle;
				CActiveScheduler::Stop();
				break;
			case EGetRequestSent:
				iStepPointer->LogEvent( _L("Page requested from the remote server.") );
				iState = EIdle;
				CActiveScheduler::Stop();
				break; 
			case EGetPageReceived:
				// Should never arrive to this point as return for receive command
				// will return a KErrEof if everything was OK. 
				iState = EIdle;
				CActiveScheduler::Stop();
				break; 
			default:
				{
				iStepPointer->LogEvent( _L("HandShakeTesterActive: State corrupted.") );
				User::Leave(KErrCorrupt);
				}
			} 
	return; 
	}

CTlsPskTestStep::CTlsPskTestStep()
	{
	iTestStepName = KHandShakeTestStep;
	iBufCipherSuitesFromIni = NULL;
	}

CTlsPskTestStep::~CTlsPskTestStep()
	{
	delete iBufCipherSuitesFromIni;
	iSndBuffer.Close();
	iRcvBuffer.Close();	
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

TVerdict CTlsPskTestStep::doTestStepPreambleL()
	{
	
		
	// Reads cipher suites to be sent in client hello message.
	iBufCipherSuitesFromIni = ReadCipherSuitesL();
			

	// Reads web address, this value is mandatory.
	TPtrC   webAddress;
	
	if(GetStringFromConfig(KSectionName, KWebAddress, webAddress))
		{
		iConnectSettings.iAddress = webAddress;
		}
	else
		{
		LogEvent( _L("Failed to read web address value from INI file.") );
		User::Leave(KErrNotFound);
		}

	// Reads page web to be requested, this value is mandatory
	TPtrC   page;
	if(GetStringFromConfig(KSectionName, KWebPage, page))
		{
		iConnectSettings.iPage.Copy(page) ;
		}
	else
		{
		LogEvent( _L("Failed to read page value from INI file.") );
		User::Leave(KErrNotFound);
		}

	// Reads port, this value is mandatory
	TInt   portNum;
	if(GetIntFromConfig(KSectionName, KPortNum, portNum))
		{
		iConnectSettings.iPortNum = portNum;
		}
	else
		{
		LogEvent( _L("Failed to read Port Number from INI file.") );
		User::Leave(KErrNotFound);
		}

	// Reads expected final cipher suits, mandatory value
	TInt cipher;	
	if(GetHexFromConfig(KSectionName, KExpectedFinalCipher, cipher))
		{
		iExpectedFinalCipher = cipher;
		}
	else
		{
		LogEvent( _L("Failed to read Expect final cipher value from INI file.") );
		User::Leave(KErrNotFound);
		}
		
	// Reads expected set cipher suites error, optional value
	TInt expectedSetSuitesError;	
	if(GetIntFromConfig(KSectionName, KExpectedSetCipherError, expectedSetSuitesError))
		{
		iExpectedSetSuitesError = expectedSetSuitesError;
		}
	else
		{
		iExpectedSetSuitesError = KErrNone; 
		}
		
	// Reads expected handshake  error, optional value
	TInt expectedHandshakeError;	
	if(GetIntFromConfig(KSectionName, KExpectedHandshakeError, expectedHandshakeError))
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
		
	// Reads PSK options (optional)
	TBool option;	
	if(GetBoolFromConfig(KSectionName, KUsePsk, option))
		{
		iUsePsk = option;
		}
	else
		{
		iUsePsk = EFalse;
		} 
		
	// Reads Null cipher options (optional)
 	TBool nullCipher;	
	if(GetBoolFromConfig(KSectionName, KUseNullCipher, nullCipher))
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

TVerdict CTlsPskTestStep::doTestStepL()
	{
	return doTestL();;
	}


TVerdict CTlsPskTestStep::doTestL()
	{
	TVerdict testResult(EFail);
    //SetTestStepResult(EFail); 

	// Instantiates scheduler
 	iSched=new(ELeave) CActiveScheduler; 
    CleanupStack::PushL(iSched);  
	CActiveScheduler::Install(iSched);
	
	// Initialises and connects the RSocket
	ConnectL();
    
	// Creates active tester object which will deal will CSecureSocket instance 
	iActiveObjTest = new (ELeave) CTlsPskTest();
	CleanupStack::PushL(iActiveObjTest);
	
	// Creates secure socket and start client handshake.
 	testResult = iActiveObjTest->DoSecureConnectionTestL(this);
	
	// Deletes and closes all production code been tested.
 	CloseConnection();

	CleanupStack::PopAndDestroy(iActiveObjTest);
	iActiveObjTest = NULL;
	CleanupStack::PopAndDestroy(iSched);
	iSched=NULL;
    
	return testResult;
	}

void  CTlsPskTestStep::SetPskL()
	{

	if(iUsePsk)
		{
		TPckgBuf<MSoPskKeyHandler *> pskConfigPkg;
		pskConfigPkg() = this;
		User::LeaveIfError(iTlsSocket->SetOpt(KSoPskConfig, KSolInetSSL, pskConfigPkg));
		}

	}
	
void  CTlsPskTestStep::SetNullCipherL()
	{
	
		if(iUseNullCipher)
 		{
 		User::LeaveIfError(iTlsSocket->SetOpt(KSoEnableNullCiphers, KSolInetSSL, ETrue));
 		}
	
	}


void CTlsPskTestStep::CloseConnection()
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
	iSocketServ.Close();
	}

	
void CTlsPskTestStep::ConnectL()
	{

	TRequestStatus rs;
	RHostResolver hostResolver;
	
	// Connect the socket server
	User::LeaveIfError( iSocketServ.Connect());	

	// Interpret server address.Success if already in dotted-decimal format
	if (iInetAddr.Input(iConnectSettings.iAddress) != KErrNone)
		
		{
		// Connect to a host resolver (for DNS resolution) - happens sychronously
		TInt retVal = hostResolver.Open( iSocketServ, KAfInet, KProtocolInetTcp);
		
		if(retVal != KErrNone)
			{
			LogEvent( _L("Failed to open host resolver.") );
			User::Leave(KErrGeneral);
			} 	 

		CleanupClosePushL(hostResolver);

		// Try to resolve symbolic name
		TNameEntry nameEntry;

		retVal = hostResolver.GetByName( iConnectSettings.iAddress, nameEntry );
		if(retVal != KErrNone)
			{
			LogEvent( _L("Failed to get name from host resolver.") );
			User::Leave(KErrGeneral);
			} 

		TSockAddr sockAddr = nameEntry().iAddr;
		iInetAddr = iInetAddr.Cast( sockAddr );
		
		hostResolver.Close();
		
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
		LogEvent( _L("Connection failed to specified address and port.") );
		User::Leave(KErrCouldNotConnect);
		}
	else
		{
		LogEvent( _L("Connected to the specified address and port.") );
		}
	}
	
/** Read list of cipher to be included in client hello message.

@param None
@return HBufC8 list of cipher suites
*/

HBufC8* CTlsPskTestStep::ReadCipherSuitesL()
	{
	
	TInt numCipherSuites;
	TName  fCipherSuite;
	TInt   iniCipherSuite;  
	HBufC8* allSuites = NULL;  
	
	if(GetIntFromConfig(KSectionName,KNumCipherSuites,numCipherSuites))
		{
		// Each cipher suite uses 2 bytes
		allSuites = HBufC8::NewMaxL( numCipherSuites * 2);
	
		for(TInt index = 1;index <= numCipherSuites ; ++index)
			{
			fCipherSuite.Format(_L("%S%d"), &KCipherSuiteBase, index);
			if(GetHexFromConfig(KSectionName, fCipherSuite,iniCipherSuite))
				{
				// Two bytes for each cipher suite
				allSuites->Des()[(index - 1) * 2] =	( ( iniCipherSuite & 0xFF00) >> 8);
				allSuites->Des()[( (index - 1) * 2 ) + 1] =	iniCipherSuite & 0xFF;
				}
			}
		}
	
	return allSuites;
	}  

/** Read list of server to be included in client hello message form INI file.

@param None
@return CDesC8ArrayFlat list of server names
*/

CDesC8ArrayFlat * CTlsPskTestStep::ReadServerNamesL()
	{
 
	TInt numServerNames;
	TName  fServerName;
	TPtrC  serverName;  
	CDesC8ArrayFlat* allServerNames = NULL;   
	
	if(GetIntFromConfig(KSectionName,KNumServerNames,numServerNames))
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
				if(GetStringFromConfig(KSectionName, fServerName, serverName))
					{
					HBufC8 *serverName8bited = HBufC8::NewMaxLC(serverName.Length());
					serverName8bited->Des().Copy(serverName);
		 			allServerNames->AppendL(serverName8bited->Des());
		 			CleanupStack::PopAndDestroy(serverName8bited);
					}
				else
					{
					//INFO_PRINTF2(_L("Could not found ServerName %d from INI file"), index );
					LogEvent( _L("Could not found spcified indexed serverName from INI file.") );	
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

/**
 * Reads the value present from the test steps ini file within the mentioned section name and key name
 * Copies the value to the TInt reference passed in
 * @param aSectName - Section within the test steps ini file
 * @param aKeyName - Name of a key within a section
 * @return aResult - The integer value of the Hex input
 * @return TBool - ETrue for found, EFalse for not found 
 */	

TBool CTlsPskTestStep::GetHexFromConfig(const TDesC& aSectName,const TDesC& aKeyName,TInt& aResult)
	{
	TPtrC result;
	if(!iConfigData)
		return EFalse;
	if(!iConfigData->FindVar(aSectName, aKeyName, result))
		return EFalse;
	TLex lex(result);
	TInt err = lex.Val((TUint &)aResult, EHex);
	if(err)
		{
		return EFalse;
		}
	return(ETrue);
	}

void CTlsPskTestStep::LogEvent( const TDesC& aMessage )
	{
	/** printf format log */
	Log(aMessage);
	}
