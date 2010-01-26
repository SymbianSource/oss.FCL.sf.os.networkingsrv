// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
 @file 
 @internalComponent
*/


#include "stressteststeps.h"

using namespace te_ppploopback;



/**
 Logs an error and leave.
 
 If provided error code != KErrNone, logs error message and leaves.
 Format of the message is:
 "ERROR: %%s,  error code= %%d", where 's' and 'd' are the message and the error code.
 
 @param aErrorCode the error code to check.
 @param aMessage   the message to log.
 */	
void CPppStressTestStep::LogAndLeaveIfErrorL(const TInt aErrorCode, const TDesC& aMessage)
	{
	
	if(KErrNone != aErrorCode)
		{
		INFO_PRINTF3(_L("ERROR: %s,  error code= %d"), aMessage.Ptr(), aErrorCode);
		SetTestStepResult(EFail);
		User::Leave(aErrorCode);
		}	
	}


/**
C++ constructor
Sets up the test step name

*/
CPppStressTestStep::CPppStressTestStep()
	{
	SetTestStepName(KPppStressTestStepName);		
	}


/**
Carries out the stress testing of Incoming PPP
by establishing a link and tearing it down many times.

@pre Phonebook Synchronizer is disabled
*/
enum TVerdict CPppStressTestStep::doTestStepL()
	{
	
	
	__UHEAP_MARK;
		
		
	SetTestStepResult(EFail);
	
	
	// Load test configuration.	
	TInt iterations = 0;
	GetIntFromConfig(ConfigSection(), _L("TestIterations"), iterations);
	iterations = iterations == 0 ? 
				 10 :  	      // Default
				 iterations; // From config file
	
	// Microseconds to PPP peer connection. Used to make sure
	// that LCP packets can be missed, or to put server in idle mode
	TInt microsecsToPeerConn =0;
	GetIntFromConfig(ConfigSection(), _L("MicrosecsToPeerConn"), microsecsToPeerConn);
	
	//Which PPP instances sends the first Configure Request, server or client 	
	TBool doStartServerFirst = ETrue;
	GetBoolFromConfig(ConfigSection(), _L("DoStartServerFirst"), doStartServerFirst);
	
	
	INFO_PRINTF4(_L("Config file: TestIterations= %d, MicrosecsToPeerConn= %d, DoStartServerFirst= %d. "), iterations, microsecsToPeerConn, doStartServerFirst);	
	
	// Write ppp.ini and pppd.ini files
	ConfigurePppServerL();
	ConfigurePppClientL();
	
	//
	// Test memory leak in Esock
	//
	RSocketServ esockForHeapCheck;
	LogAndLeaveIfErrorL(esockForHeapCheck.Connect(), _L("RSocketServ::Connect {for Heap Check}"));	
	
	//
	// Set up the connections, but do not start:
	//
	INFO_PRINTF1(_L("Setting up Esock and RConnections.... "));	
	LogAndLeaveIfErrorL(iEsock.Connect(),         _L("RSocketServ::Connect"));
	
	
	
	iServerConnPrefs.SetIapId(iSvrIapId);
	iServerConnPrefs.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);
	
	iClientConnPrefs.SetIapId(iClIapId);
	iClientConnPrefs.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);	
	
		
	// Logging granularity		
	TInt iterationsPerLogEntry = iterations / 100;
	if(iterationsPerLogEntry < 20)
		{
		iterationsPerLogEntry = 20;
		}
	
	// Keep count of errors: we do not stop the test after the first one.
	TInt connErrCount 	 = 0;
	TInt disconnErrCount = 0;
	
	//----------------------------------------------------------------------------------------------------------
	// The testing itself:
	// Create and destroy connections:
	for(int curIteration = 0; curIteration < iterations; curIteration++)
		{
		__UHEAP_MARK;
		
			
		
		TRequestStatus serverConnReqSt;
		TRequestStatus clientConnReqSt;
		
		//LogAndLeaveIfErrorL(esockForHeapCheck.__DbgMarkHeap(), _L("__DbgMarkHeap"));
		
		
		LogAndLeaveIfErrorL(iServerConn.Open(iEsock), _L("RConnection[Server]::Open"));
		LogAndLeaveIfErrorL(iClientConn.Open(iEsock), _L("RConnection[Client]::Open"));	
		
		if(doStartServerFirst)
			{			
			iServerConn.Start(iServerConnPrefs, serverConnReqSt); 
			if(microsecsToPeerConn > 0)
				{
				//INFO_PRINTF2(_L("Pausing for %d microseconds to miss LCP Configure Request from Server... "), microsecsToPeerConn);
				User::After(microsecsToPeerConn);
				}					
			iClientConn.Start(iClientConnPrefs, clientConnReqSt); 
			}
		else // Start client first
			{
			iClientConn.Start(iClientConnPrefs, clientConnReqSt); 
			if(microsecsToPeerConn > 0)
				{
				//INFO_PRINTF1(_L("Pausing for %d microseconds to miss LCP Configure Request from Client... "));
				User::After(microsecsToPeerConn);
				}
			iServerConn.Start(iServerConnPrefs, serverConnReqSt);
			}
			
		
	    //INFO_PRINTF1(_L("Waiting for RConnection::Start request to finish "));
		// Wait for PPP connection establishment request to complete.
		User::WaitForRequest(serverConnReqSt, clientConnReqSt);
		User::WaitForRequest(clientConnReqSt, serverConnReqSt);
				
		// Check if there was an error establishing connection (Do we actually have a working PPP connection or not?)
		if( KErrNone != serverConnReqSt.Int()  || 
			KErrNone != clientConnReqSt.Int()
		   ){
			++connErrCount;
			INFO_PRINTF4(_L("ERROR [at connection= %d]: PPP server = %d, client = %d"),curIteration, serverConnReqSt.Int(), clientConnReqSt.Int() );
			
			serverConnReqSt = KErrNone;
			clientConnReqSt	= KErrNone;		
			}
			
		
		// Regardless if there is an error, we stop the PPP and proceed to the next iteration:
		
		TInt clientDisconnErr = iClientConn.Stop();
		
		if(KErrNone != clientDisconnErr)
			{
			++disconnErrCount;
			INFO_PRINTF3(_L("ERROR [at disconnection= %d]: Peer = %d"),curIteration, clientDisconnErr );
			}
		
		iServerConn.Stop(); // This returns an error 
		// Feedback 
		if((curIteration % iterationsPerLogEntry) == 0) 
			{
			INFO_PRINTF3(_L("Executed iteration = %d of %d"), curIteration, iterations );
			}
		
		
		iClientConn.Close();
		iServerConn.Close();
		
		
		//LogAndLeaveIfErrorL(esockForHeapCheck.__DbgMarkEnd(0), _L("__DbgMarkEnd"));
		__UHEAP_MARKEND;
		
		User::After(3 * 1000000); // Between Iterations
		}
		
	INFO_PRINTF4(_L("Test summary: Executed %d connection attempts. Failures at connection = %d, at disconnection = %d."), iterations, connErrCount, disconnErrCount );
	// Process the error buffers:
	if(0 == connErrCount && 
	   0 == disconnErrCount) 
		{
		SetTestStepResult(EPass);
		}
		
	//-------------------------------------------------------------------------------------------------------
	// Not relevant for incoming PPP stress testing
	INFO_PRINTF1(_L("Closing RConnection and ESock"));
	iServerConn.Close();
	iClientConn.Close();
	iEsock.Close();
	
	INFO_PRINTF1(_L("Test Completed."));	
	
	
	__UHEAP_MARKEND;	
	return TestStepResult();	
	}



