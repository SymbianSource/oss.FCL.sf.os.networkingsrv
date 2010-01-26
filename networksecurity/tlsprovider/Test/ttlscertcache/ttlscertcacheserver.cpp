// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "ttlscertcacheserver.h"

#include "updateentrystep.h"
#include "entrystatusstep.h"

_LIT(KServerName,"ttlscertcache");

CTlsCertCacheServer* CTlsCertCacheServer::NewL()
/**
 * @return - Instance of the test server
 * Called inside the MainL() function to create and start the
 * CTestServer derived server.
 */
	{
	
	CTlsCertCacheServer* server = new (ELeave) CTlsCertCacheServer();
	CleanupStack::PushL(server);
	// CServer base class call
	server->StartL(KServerName);
	CleanupStack::Pop(server);
	return server;
	}

LOCAL_C void MainL()
/**
 * Much simpler, uses the new Rendezvous() call to sync with the client
 */
	{
	CActiveScheduler* sched=NULL;
	sched=new(ELeave) CActiveScheduler;
	CActiveScheduler::Install(sched);
	
	CTlsCertCacheServer* server = NULL;
	// Create the CTestServer derived server	
	TRAPD(err,server = CTlsCertCacheServer::NewL());
	if(!err)
		{
		// Sync with the client and enter the active scheduler
		RProcess::Rendezvous(KErrNone);
		sched->Start();
		}
	delete server;
	delete sched;
	}
	
TInt E32Main()
	{
	CTrapCleanup* cleanup = CTrapCleanup::New();
	TInt err = KErrNoMemory;
	if (cleanup)
		{
		TRAP(err, MainL());
		}
	return err;
	}


CTestStep* CTlsCertCacheServer::CreateTestStep(const TDesC& aStepName)
	{
	
	CTestStep* step = NULL;
	
	if (aStepName == KUpdateEntryStep)
		{
		step = new CUpdateEntryStep;
		}
	else if (aStepName == KEntryStatus)
		{
		step = new CEntryStatusStep;
		}

	return step;
	}
