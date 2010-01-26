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
 @file TeHTTpServer.cpp
*/
#include "TeHttpServer.h"
#include "TeInit.h"
#include "TeConnect.h"
#include "TeListen.h"
#include <test/testexecutelog.h>
#include <e32std.h>


_LIT(KServerName,"Te_Http");

CTestHttpServer* CTestHttpServer::NewL()
/**
 * @return - Instance of the test server
 * Called inside the MainL() function to create and start the
 * CTestServer derived server.
 */
	{
	CTestHttpServer * server = new (ELeave) CTestHttpServer();
	CleanupStack::PushL(server);
	server->ConstructL(KServerName);
	// CServer base class call
	server->StartL(KServerName);
	CleanupStack::Pop(server);
	return server;
	}

void CTestHttpServer::ConstructL(const TDesC& /*aName*/)
	{
	iListenerMgr = new (ELeave) CTestListenerMgr();
	}

CTestHttpServer::CTestHttpServer()
	{
	}

CTestHttpServer::~CTestHttpServer()
	{
	delete iListenerMgr;
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
        CTestHttpServer* server = NULL;
        // Create the CTestHttpServer derived server
        TRAPD(err,server = CTestHttpServer::NewL());
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

CTestStep* CTestHttpServer::CreateTestStep(const TDesC& aStepName)
/**
 * @return - A CTestStep derived instance
 * Secure and non-secure variants
 * Implementation of CTestServer pure virtual
 */
	{
	CTestStep* testStep = NULL;

	if(aStepName == KInit)
		testStep = new CTestStepInit(iListenerMgr);
	if(aStepName == KServerListen)
		testStep = new CTestStepListen(iListenerMgr);
	if(aStepName == KClientConnect)
		testStep = new CTestStepConnect(iListenerMgr);

	return testStep;
	}

