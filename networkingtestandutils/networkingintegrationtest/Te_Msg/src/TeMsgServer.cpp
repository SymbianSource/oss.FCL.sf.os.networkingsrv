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

/**
 @file TeMsgServer.cpp
*/
#include "TeMsgServer.h"
#include "TeMsgStep.h"

_LIT(KServerName,"Te_msg");
CTestMsgServer* CTestMsgServer::NewL()
/**
 * @return - Instance of the test server
 * Called inside the MainL() function to create and start the
 * CTestServer derived server.
 */
	{
	CTestMsgServer * server = new (ELeave) CTestMsgServer();
	CleanupStack::PushL(server);
	// CServer base class call
	server->StartL(KServerName);
	CleanupStack::Pop(server);
	return server;
	}



LOCAL_C void MainL()
{
  
#if (defined __DATA_CAGING__)
   	RProcess().DataCaging(RProcess::EDataCagingOn);
//  	RProcess().DataCaging(RProcess::ESecureApiOn);
 	RProcess().SecureApi(RProcess::ESecureApiOn);
#endif

        CActiveScheduler* sched=NULL;
        sched=new(ELeave) CActiveScheduler;
        CActiveScheduler::Install(sched);
        CTestMsgServer* server = NULL;
        // Create the CTestMsgServer derived server
        TRAPD(err,server = CTestMsgServer::NewL());
        if(!err)
                {
                // Sync with the client and enter the active scheduler
                RProcess::Rendezvous(KErrNone);
                sched->Start();
                }
        delete server;
        delete sched;
}



//#if (!defined __DATA_CAGING__)

// Secure variants much simpler
// Just an E32Main and a MainL()
/*LOCAL_C void MainL()
	{
	//RProcess().DataCaging(RProcess::EDataCagingOn);
	//RProcess().DataCaging(RProcess::ESecureApiOn);
	CActiveScheduler* sched=NULL;
	sched=new(ELeave) CActiveScheduler;
	CActiveScheduler::Install(sched);
	CTestMsgServer* server = NULL;
	// Create the CTestServer derived server
	TRAPD(err,server = CTestMsgServer::NewL());
	if(!err)
		{
		// Sync with the client and enter the active scheduler
		RProcess::Rendezvous(KErrNone);
		sched->Start();
		}
	delete server;
	delete sched;
	}
*/


GLDEF_C TInt E32Main()
/**
 * @return - Standard Epoc error code on process exit
 * Secure variant only
 * Process entry point. Called by client using RProcess API
 */
	{
//	__UHEAP_MARK;
	CTrapCleanup* cleanup = CTrapCleanup::New();
	if(cleanup == NULL)
		{
		return KErrNoMemory;
		}
	TRAPD(err,MainL());
	delete cleanup;
//	__UHEAP_MARKEND;
	return KErrNone;
    }

CTestStep* CTestMsgServer::CreateTestStep(const TDesC& aStepName)
/**
 * @return - A CTestStep derived instance
 * Secure and non-secure variants
 * Implementation of CTestServer pure virtual
 */
	{
	CTestStep* testStep = NULL;
	if(aStepName == KConnectWithOverrides)
		testStep = new CTestConnectStep();
	return testStep;
	}
