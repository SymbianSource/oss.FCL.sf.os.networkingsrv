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
 @file
 @internalComponent
*/

#include "Te_IPEventNotifierSuiteServer.h"
#include "IPEventNotifier1Step_ReceiveMFlag.h"
#include "IPEventNotifier2Step_IPReady.h"
#include "IPEventNotifier3Step_LinkLocalAddress.h"
#include "IPEventNotifier4Step_ReceiveOFlag.h"

_LIT(KServerName,"TE_IPEventNotifierSuite");
CTE_IPEventNotifierSuite* CTE_IPEventNotifierSuite::NewL()
/**
   @return - Instance of the test server
   Same code for Secure and non-secure variants
   Called inside the MainL() function to create and start the
   CTestServer derived server.
 */
	{
	CTE_IPEventNotifierSuite * server = new (ELeave) CTE_IPEventNotifierSuite();
	CleanupStack::PushL(server);
	// CServer base class call which can be either StartL or ConstructL,
	// the later will permit Server Logging.

	server->StartL(KServerName);
	CleanupStack::Pop(server);
	return server;
	}


// Secure variants much simpler
// For EKA2, just an E32Main and a MainL()
LOCAL_C void MainL()
/**
   Secure variant
   Much simpler, uses the new Rendezvous() call to sync with the client
 */
	{
	// Leave the hooks in for platform security
#if (defined __DATA_CAGING__)
	RProcess().DataCaging(RProcess::EDataCagingOn);
	RProcess().DataCaging(RProcess::ESecureApiOn);
#endif
	CActiveScheduler* sched=NULL;
	sched=new(ELeave) CActiveScheduler;
	CActiveScheduler::Install(sched);
	CTE_IPEventNotifierSuite* server = NULL;
	// Create the CTestServer derived server
	TRAPD(err,server = CTE_IPEventNotifierSuite::NewL());
	if(!err)
		{
		// Sync with the client and enter the active scheduler
		RProcess::Rendezvous(KErrNone);
		sched->Start();
		}
	delete server;
	delete sched;
	}



GLDEF_C TInt E32Main()
/**
   @return - Standard Epoc error code on process exit
   Secure variant only
   Process entry point. Called by client using RProcess API
 */
	{
	__UHEAP_MARK;
	CTrapCleanup* cleanup = CTrapCleanup::New();
	if(cleanup == NULL)
		{
		return KErrNoMemory;
		}
	TRAP_IGNORE(MainL());
	delete cleanup;
	__UHEAP_MARKEND;
	return KErrNone;
    }


CTestStep* CTE_IPEventNotifierSuite::CreateTestStep(const TDesC& aStepName)
/**
   @return - A CTestStep derived instance
   Secure and non-secure variants
   Implementation of CTestServer pure virtual
 */
	{
	CTestStep* testStep = NULL;
	// They are created "just in time" when the worker thread is created
	// More test steps can be added below 
    if(aStepName == KIPEventNotifier1Step_ReceiveMFlag)
        {
        testStep = new CIPEventNotifier1Step_ReceiveMFlag();
        }
	else if(aStepName == KIPEventNotifier4Step_ReceiveOFlag)
        {
        testStep = new CIPEventNotifier4Step_ReceiveOFlag();
        }
    else if(aStepName == KIPEventNotifier2Step_IPReady)
        {
        testStep = new CIPEventNotifier2Step_IPReady();
		}
    else if(aStepName == KIPEventNotifier3Step_LinkLocalAddress)
    	{
        testStep = new CIPEventNotifier3Step_LinkLocalAddress();
    	}
				
	return testStep;
	}
