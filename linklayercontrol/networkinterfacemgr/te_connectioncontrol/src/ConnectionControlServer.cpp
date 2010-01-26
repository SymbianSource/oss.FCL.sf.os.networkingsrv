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
// Implements the testexecute server for the Nifman connection control mechanism.
// For (WINS && !EKA2) versions will be xxxServer.Dll and require a thread to be started
// in the process of the client. The client initialises the server by calling the
// one and only ordinal.
// 
//

/**
 @file
 @internalComponent
*/


#include "ConnectionControlServer.h"
#include "ConnectionControlStep.h"


/** The server name. */
_LIT(KServerName, "TE_ConnectionControl");


// EKA2 much simpler
// just an E32Main and a MainL()
LOCAL_C void MainL()
/**
 * Much simpler, uses the new Rendezvous() call to sync with the client.
 *
 * @internalComponent
 *
 * @leave Doesn't leave. 
 */
	{
	// leave the hooks in for platform security
#if (defined __DATA_CAGING__)
	RProcess().DataCaging(RProcess::EDataCagingOn);
	RProcess().SecureApi(RProcess::ESecureApiOn);
#endif
	CActiveScheduler* sched = NULL;
	sched = new(ELeave) CActiveScheduler;
	CActiveScheduler::Install(sched);

	CConnectionControlServer* server = NULL;
	// create the CTestServer derived server
	TRAPD(err,server = CConnectionControlServer::NewL());
	if (!err)
		{
		// Sync with the client and enter the active scheduler
		RProcess::Rendezvous(KErrNone);
		sched->Start();
		}
	delete server;
	delete sched;
	}

// only a DLL on emulator for typhoon and earlier

GLDEF_C TInt E32Main()
/**
 * Main entry point.
 *
 * @internalComponent
 *
 * @return Standard Epoc error codes.
 */
	{
	CTrapCleanup* cleanup = CTrapCleanup::New();
	if (cleanup == NULL)
		{
		return KErrNoMemory;
		}
	TRAPD(err, MainL());
	delete cleanup;
	return KErrNone;
    }

// Create a thread in the calling process
// Emulator typhoon and earlier


CConnectionControlServer* CConnectionControlServer::NewL()
/**
 * Called inside the MainL() function to create and start the CTestServer derived server.
 *
 * @internalComponent
 *
 * @return Instance of the test server.
 */
	{
	CConnectionControlServer * server = new (ELeave) CConnectionControlServer();
	CleanupStack::PushL(server);
	
	// CServer base class call
	server->StartL(KServerName);
	CleanupStack::Pop(server);
	return server;
	}

CTestStep* CConnectionControlServer::CreateTestStep(
	const TDesC& aStepName)
/**
 * Implementation of CTestServer virtual function.
 *
 * @internalComponent
 *
 * @return A CTestStep derived instance.
 */
	{
	CConnectionControlStep* testStep = NULL;
	
	if (aStepName == KCConnectionControlNullDaemonTest)
		testStep = new CConnectionControlNullDaemonTest();	
	else if (aStepName == KCConnectionControlRegOKStep)
		testStep = new CConnectionControlRegOKStep();
	else if (aStepName == KCConnectionControlRegErrorStep)
		testStep = new CConnectionControlRegErrorStep();				
	else if (aStepName == KCConnectionControlDeregisterOKStep)
		testStep = new CConnectionControlDeregisterOKStep();
	else if (aStepName == KCConnectionControlDeregisterErrorStep)
		testStep = new CConnectionControlDeregisterErrorStep();
	else if (aStepName == KCConnectionControlDeregisterConnectionStep)
		testStep = new CConnectionControlDeregisterConnectionStep();		
	else if (aStepName == KCConnectionControlDeregisterStopNormalStep)
		testStep = new CConnectionControlDeregisterStopNormalStep();		
	else if (aStepName == KCConnectionControlDeregisterStopAuthoritativeStep)
		testStep = new CConnectionControlDeregisterStopAuthoritativeStep();
	else if (aStepName == KCConnectionControlDeregisterDormantStep)
		testStep = new CConnectionControlDeregisterDormantStep();				
	else if (aStepName == KCConnectionControlCsDaemonProgressStep)
		testStep = new CConnectionControlCsDaemonProgressStep();				
	else if (aStepName == KCConnectionControlProgressStep)
		testStep = new CConnectionControlProgressStep();	
	else if (aStepName == KCConnectionControlLinkUpStopNormalStep)
		testStep = new CConnectionControlLinkUpStopNormalStep();	
	else if (aStepName == KCConnectionControlLinkUpStopAuthoritativeStep)
		testStep = new CConnectionControlLinkUpStopAuthoritativeStep();	
	else if (aStepName == KCConnectionControlIoctlStep)
		testStep = new CConnectionControlIoctlStep();	
	else if (aStepName == KCConnectionTimeoutDuringRegistration)
		testStep = new CConnectionTimeoutDuringRegistration();	
	else if (aStepName == KCConnectionTimeoutDuringDeregistration)
		testStep = new CConnectionTimeoutDuringDeregistration();	
	else if (aStepName == KCConnectionControlSimulateInterPDSNHandoff)
		testStep = new CConnectionControlSimulateInterPDSNHandoff();	
	else if (aStepName == KCConnectionControlSimulateInterPDSNHandoffNullDaemon)
		testStep = new CConnectionControlSimulateInterPDSNHandoffNullDaemon();	
	else if (aStepName == KCConnectionControlSimulateInterPDSNHandoffDuringRegistration)
		testStep = new CConnectionControlSimulateInterPDSNHandoffDuringRegistration();	
	else if (aStepName == KCConnectionControlSimulateInterPDSNHandoffDuringDeregistration)
		testStep = new CConnectionControlSimulateInterPDSNHandoffDuringDeregistration();	
	else if (aStepName == KCConnectionControlSimulateInterPDSNHandoffDuringIoctlRequest)
		testStep = new CConnectionControlSimulateInterPDSNHandoffDuringIoctlRequest();	
	else if (aStepName == KCConnectionControlDeregisterCauseTimerShortStep)
		testStep = new CConnectionControlDeregisterCauseTimerShortStep();
	else if (aStepName == KCConnectionControlDeregisterCauseTimerMediumStep)
		testStep = new CConnectionControlDeregisterCauseTimerMediumStep();		
	else if (aStepName == KCConnectionControlDeregisterCauseTimerLongStep)
		testStep = new CConnectionControlDeregisterCauseTimerLongStep();		
			
	
	if (testStep)
		{
		// initialize the test step
		TRAPD(err, testStep->ConstructL());
		
		if (err != KErrNone)
			{
			delete testStep;
			testStep = NULL;
			}
		}

	return testStep;
	}

