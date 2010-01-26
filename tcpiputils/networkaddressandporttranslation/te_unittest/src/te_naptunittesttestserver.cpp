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
// Contains the implementation of TEF3.0 respective test server
//



/**
 @file
 @internalTechnology
*/
  
#include "te_naptunittesttestserver.h"

/**
Function to instantiate TestServer.

@return Returns the TestServer instance pointer.

@internalTechnology
*/
CNaptUnitTestTestServer* CNaptUnitTestTestServer::NewL()
	{
	CNaptUnitTestTestServer* server = new (ELeave) CNaptUnitTestTestServer();
	CleanupStack::PushL(server);
	server->ConstructL();
	CleanupStack::Pop(server);
	return server;
	}

/**
Function to instantiate TestBlock.

This function is invoked by the TestEngine while parsing START_TEST_BLOCK command in the
script file. 

@return Returns the TestBlock instance pointer.

@internalTechnology
*/
CTestBlockController* CNaptUnitTestTestServer::CreateTestBlock()
	{
	CTestBlockController* testBlockController=NULL;
	TRAPD(err, testBlockController = CreateTestBlockL());
	if(err == KErrNone)
		{
		return testBlockController;
		}
	else
		{
		return NULL;
		}
	}

/**
Function to instantiate TestBlock.

@internalTechnology
*/
CTestBlockController* CNaptUnitTestTestServer::CreateTestBlockL()
	{
	return new (ELeave) CNaptUnitTestTestBlock();
	}

/**
Function to kick start the TestServer.

@internalTechnology
*/
LOCAL_C void MainL()
	{
#if (defined __DATA_CAGING__)
	RProcess().DataCaging(RProcess::EDataCagingOn);
	RProcess().SecureApi(RProcess::ESecureApiOn);
#endif
	CActiveScheduler* sched=NULL;
	sched=new(ELeave) CActiveScheduler;
	CActiveScheduler::Install(sched);
	CNaptUnitTestTestServer* server = NULL;
	TRAPD(err, server = CNaptUnitTestTestServer::NewL());
	if(!err)
		{
		RProcess::Rendezvous(KErrNone);
		sched->Start();
		}
	delete server;
	delete sched;
	}
/**
TestServer entry point.

@return Returns KErrNone upon successfull completion, KErrNoMemory in low memory conditions.

@internalTechnology
*/
GLDEF_C TInt E32Main()
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
