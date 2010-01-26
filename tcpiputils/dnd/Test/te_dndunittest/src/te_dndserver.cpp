// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Test server for the DND test
// 
//

/**
 @file te_dndServer.cpp
*/
#include "te_dndserver.h"
#include "te_dndinit.h"
#include <c32comm.h>

_LIT(KServerName,"te_dndServer");

/**
* @return - Instance of the test server
* Called inside the MainL() function to create and start the
* CTestServer derived server.
*/
CDndTestServer* CDndTestServer::NewL()
    {
    // __EDIT_ME__ new your server class here
    CDndTestServer * server = new (ELeave) CDndTestServer();
    CleanupStack::PushL(server);
    // CServer base class call
    server->StartL(KServerName);
    CleanupStack::Pop(server);
    return server;
    }

// MainL()
LOCAL_C void MainL()
/**
* Much simpler, uses the new Rendezvous() call to sync with the client
*/
    {
    // Leave the hooks in for platform security
#if (defined __DATA_CAGING__)
    RProcess().DataCaging(RProcess::EDataCagingOn);
    RProcess().SecureApi(RProcess::ESecureApiOn);
#endif
    CActiveScheduler* sched=NULL;
    sched=new(ELeave) CActiveScheduler;
    CActiveScheduler::Install(sched);
    CDndTestServer* server = NULL;
    // Create the CTestServer derived server
    TRAPD(err,server = CDndTestServer::NewL());
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
* @return - Standard Epoc error code on exit
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


/**
    * @return - A CTestStep derived instance
    * Implementation of CTestServer pure virtual
*/
CTestStep* CDndTestServer::CreateTestStep(const TDesC& aStepName)
    {
    CTestStep* testStep = NULL;
    
      
    if(aStepName == KTestStepDND_Init)
        testStep = new CTestStepDND_Init(this);
    
        
    return testStep;
    }
