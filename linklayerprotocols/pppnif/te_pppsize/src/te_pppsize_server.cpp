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
 @file te_pppsize_server.cpp
*/


//Symbian OS Headers
#include <c32comm.h>
#include <e32base.h>

//Test Framework Header
#include <test/testexecuteserverbase.h>

//Test Step Header
#include "te_pppsize_server.h"
#include "TestSteps.h"

_LIT(KServerName, "TE_PPPSize");


LOCAL_C void MainL()
{
#ifdef __DATA_CAGING__
	RProcess().DataCaging(RProcess::EDataCagingOn);
	RProcess().DataCaging(RProcess::ESecureApiOn);
#endif

	CActiveScheduler* sched=NULL;
	sched=new(ELeave) CActiveScheduler;
	CActiveScheduler::Install(sched);
	CPPPSizeServer* server = NULL;
	// Create the CTestServer derived server
	TRAPD(err,server = CPPPSizeServer::NewL());
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


CPPPSizeServer::CPPPSizeServer()
{
}

CPPPSizeServer::~CPPPSizeServer()
{
	
}

CPPPSizeServer* CPPPSizeServer::NewL()
{
	CPPPSizeServer* server = new(ELeave) CPPPSizeServer();
	CleanupStack::PushL(server);
	// CServer base class call
	// Name the server using the system-wide unique string
	// Clients use this to create server sesssions
	server->StartL(KServerName);
	CleanupStack::Pop();
	return server;
}

CTestStep* CPPPSizeServer::CreateTestStep(const TDesC &aStepName)
{
	CTestStep* testStep = NULL;
	//  Test step name constant in the test step header file
	// Created "just in time"
	// Just one created here but create as many as required 
	if (aStepName == KPPPMinMaxMMU) {
		testStep = new(ELeave) CPPPMinMaxMMU();
	}
	return testStep;
}

