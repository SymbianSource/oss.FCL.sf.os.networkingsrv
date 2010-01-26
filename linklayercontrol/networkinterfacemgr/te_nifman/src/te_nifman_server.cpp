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
 @file te_nifman_server.cpp
*/


//Symbian OS Headers
#include <c32comm.h>
#include <e32base.h>

//Test Framework Header
#include <testexecuteserverbase.h>

//Test Step Header
#include "te_nifman_server.h"
#include "TestSteps.h"

_LIT(KServerName, "TE_Nifman");

LOCAL_C void MainL()
	{
#ifdef __DATA_CAGING__
	RProcess().DataCaging(RProcess::EDataCagingOn);
	RProcess().DataCaging(RProcess::ESecureApiOn);
#endif

	CActiveScheduler* sched=NULL;
	sched=new(ELeave) CActiveScheduler;
	CActiveScheduler::Install(sched);
	CNifmanServer* server = NULL;
	// Create the CTestServer derived server
	TRAPD(err,server = CNifmanServer::NewL());
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
	{
	CTrapCleanup* cleanup = CTrapCleanup::New();
	if(cleanup == NULL)
		{
		return KErrNoMemory;
		}
	TRAPD(err,MainL());
	delete cleanup;
	return KErrNone;
	}


CNifmanServer::CNifmanServer()
	{
 	// When bootstrapping C32 we have to avoid the PhBkSyncServer being started, since
 	// it needs a different CommDB
 	_LIT(KPhbkSyncCMI, "phbsync.cmi");
    (void)StartC32WithCMISuppressions(KPhbkSyncCMI);
	}

CNifmanServer::~CNifmanServer()
	{ }

CNifmanServer* CNifmanServer::NewL()
	{
	CNifmanServer* server = new(ELeave) CNifmanServer();
	CleanupStack::PushL(server);
	// CServer base class call
	// Name the server using the system-wide unique string
	// Clients use this to create server sesssions
	server->StartL(KServerName);
	CleanupStack::Pop();
	return server;
	}

CTestStep* CNifmanServer::CreateTestStep(const TDesC &aStepName)
	{
	CTestStep* testStep = NULL;
	//  Test step name constant in the test step header file
	// Created "just in time"
	// Just one created here but create as many as required 
	if (aStepName == KOpenCloseBogus) 
		{
		testStep = new COpenCloseBogus();
		}
	else if (aStepName == KSocketServerShutdown)
		{
		testStep = new CSocketServerShutdown();
		}
	else if (aStepName == KConnectReconnect) 
		{
		testStep = new CConnectReconnect();
		}
	else if (aStepName == KStartStopInterfaces)	
		{
		testStep = new CStartStopInterfaces();
		}
	else if (aStepName == KProgressNotification)
		{
		testStep = new CProgressNotification();
		}
	else if (aStepName == KBinderLayerDown) 
		{
		testStep = new CBinderLayerDown();
		}
	else if (aStepName == KTest8) 
		{
		testStep = new CTest8();
		}
	else if (aStepName == KTest5) 
		{
		testStep = new CTest5();
		}
	else if (aStepName == KOpenClosePSD) 
		{
		testStep = new COpenClosePSD();
		}
	return testStep;
	}


