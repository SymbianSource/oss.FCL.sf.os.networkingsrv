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

/**
 @file
*/

#include <e32base.h>

#include "ttlsoom.h"
#include "tlsoomstepwrapper.h"

_LIT(KServerName,"TTlsOOM");

CTlsOOMServer* CTlsOOMServer::NewL()
	{
	CTlsOOMServer * server = new (ELeave) CTlsOOMServer();
	CleanupStack::PushL(server);
	
	// Either use a StartL or ConstructL, the latter will permit Server Logging.

	//server->StartL(KServerName); 
	server->ConstructL(KServerName);
	CleanupStack::Pop(server);
	return server;
	}

LOCAL_C void MainL()
	{
	// Leave the hooks in for platform security
#if (defined __DATA_CAGING__)
	RProcess().DataCaging(RProcess::EDataCagingOn);
	RProcess().SecureApi(RProcess::ESecureApiOn);
#endif
	CActiveScheduler* sched = NULL;
	CleanupStack::PushL(sched);
	sched=new(ELeave) CActiveScheduler;
	CActiveScheduler::Install(sched);
	CTlsOOMServer* server = NULL;
	TRAPD(err,server = CTlsOOMServer::NewL());
	CleanupStack::PushL(server);
	
	if(!err)
		{
		RProcess::Rendezvous(KErrNone);
		sched->Start();
		}
		
	CleanupStack::PopAndDestroy(2, sched); // server
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

CTestStep* CTlsOOMServer::CreateTestStep(const TDesC& aStepName)
	{
	CTestStep* testStep = new CTlsOOMStepWrapper(aStepName);
	return testStep;
	}
