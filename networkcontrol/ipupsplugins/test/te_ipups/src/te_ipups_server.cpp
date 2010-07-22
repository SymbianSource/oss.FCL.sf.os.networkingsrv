// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
 @test
 @internalComponent - Internal Symbian test code 
*/

#include "te_ipups_server.h"
#include "te_ipups_ups_step.h"
#include "te_ipups_notify_count.h"
#include "te_ipups_delete_decision_db.h"
#include <bacline.h>

CTeIpUpsSuite* CTeIpUpsSuite::NewL(const TDesC& aName)
/**
 * @return - Instance of the test server
 */
	{
	CTeIpUpsSuite * server = new (ELeave) CTeIpUpsSuite();
	CleanupStack::PushL(server);
	server->ConstructL(aName);
	CleanupStack::Pop(server);
	return server;
	}


LOCAL_C void MainL()
/**
 * Main implementation
 */
	{
	CActiveScheduler* sched=NULL;
	sched=new(ELeave) CActiveScheduler;
	CActiveScheduler::Install(sched);
	CTeIpUpsSuite* server = NULL;
		
	CCommandLineArguments* args = CCommandLineArguments::NewLC();
	TPtrC exeName = args->Arg(0);
	TParse fullName;
	fullName.Set(exeName, NULL, NULL);
	CleanupStack::PopAndDestroy(args);
	
	// Create the CTestServer derived server
	TRAPD(err,server = CTeIpUpsSuite::NewL(fullName.Name()));
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
 * @return - Standard Epoc error code on process exit
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


CTestStep* CTeIpUpsSuite::CreateTestStep(const TDesC& aStepName)
/**
 * @return - A CTestStep derived instance
 */
	{
	CTestStep* testStep = NULL;
	
	if(aStepName == KIpUpsClientStep)
		{
		testStep = new CIpUpsStep();
		}
	else if(aStepName == KIpUpsNotifyCount)
		{
		testStep = new CIpUpsNotifyCount();
		}
	else if(aStepName == KIpUpsDeleteDecisionDB)
		{
		testStep = new CIpUpsDeleteDecisionDB();
		}
	
	return testStep;
	}
