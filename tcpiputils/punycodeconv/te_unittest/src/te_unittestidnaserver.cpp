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
//



/**
 @file
 @internalTechnology
*/


#include "te_unittestidnaserver.h"
#include "te_unittestidna.h"

_LIT(KServerName,"testidnserver");

CIdnaTestServer* CIdnaTestServer::NewL()
/**
 * @return - Instance of the test server
 * Same code for Secure and non-secure variants
 * Called inside the MainL() function to create and start the
 * CTestServer derived server.
 */
	{
	CIdnaTestServer * testServer = new (ELeave) CIdnaTestServer();
	CleanupStack::PushL(testServer);

	testServer->ConstructL(KServerName);
	CleanupStack::Pop(testServer);
	return testServer;
	}

LOCAL_C void MainL()
/**
 * Secure variant
 * Much simpler, uses the new Rendezvous() call to sync with the client
 */
	{
  
	CActiveScheduler* sched=NULL;
	sched=new(ELeave) CActiveScheduler;
	CActiveScheduler::Install(sched);
	CIdnaTestServer* testServer = NULL;
	
	// Create the CTestServer derived server
	TRAPD(err,testServer = CIdnaTestServer::NewL());
	if(!err)
		{
		// Sync with the client and enter the active scheduler
		RProcess::Rendezvous(KErrNone);
		sched->Start();
		}
	delete testServer;
	delete sched;
	}

GLDEF_C TInt E32Main()
/**
 * @return - Standard Epoc error code on process exit
 * Secure variant only
 * Process entry point. Called by client using RProcess API
 */
	{
	__UHEAP_MARK;
	CTrapCleanup* cleanup = CTrapCleanup::New();
	if(cleanup == NULL)
		{
		return KErrNoMemory;
		}
	TRAPD(err,MainL());
	delete cleanup;
	__UHEAP_MARKEND;
	return err;
    }

CTestStep* CIdnaTestServer::CreateTestStep(const TDesC& aStepName)
/**
 * @return - A CTestStep derived instance
 * Secure and non-secure variants
 * Implementation of CTestServer pure virtual
 */
	{
	CTestStep* testStep = NULL;

	INFO_PRINTF1(_L("********************************************"));
	INFO_PRINTF1(_L(" Unit Test For IDN Resolution functionality "));
	INFO_PRINTF1(_L("********************************************"));

    if(aStepName == _L("TestIdna01"))
    	{
    	testStep = new CTestIdna01;
		}
    else if(aStepName == _L("TestIdna02"))
    	{
    	testStep = new CTestIdna02;
		}
    else if(aStepName == _L("TestIdna03"))
    	{
    	testStep = new CTestIdna03;
		}
    else if(aStepName == _L("TestIdna04"))
    	{
    	testStep = new CTestIdna04;
		}
    else if(aStepName == _L("TestIdna05"))
    	{
    	testStep = new CTestIdna05;
		}
    else if(aStepName == _L("TestIdna06"))
    	{
    	testStep = new CTestIdna06;
		}
    else if(aStepName == _L("TestIdna07"))
        	{
        	testStep = new CTestIdna07;
    		}
    else if(aStepName == _L("TestIdna08"))
        	{
        	testStep = new CTestIdna08;
    		}
        
    else
    	{
    	INFO_PRINTF1(_L("**** TEST STEP NOT SUPPORTED *******"));
    	}
    
  	return testStep;
	} 
