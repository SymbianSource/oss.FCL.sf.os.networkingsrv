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
// Contain the implementation of the class for generic CSD agent test
// 
//
#include "CsdAgentTestSteps.h"
#include "es_sock.h"
#include "in_sock.h"
#include "commdbconnpref.h"

CTestStepCsdAgentConnection::CTestStepCsdAgentConnection(TPtrC aName)
{
    iTestStepName=aName;
}

enum TVerdict CTestStepCsdAgentConnection::doTestStepL(void)
	{
    
	__UHEAP_MARK;
	TInt err;
	RSocketServ server;
	RConnection connection;
	TRequestStatus status;
		
	err = server.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(server);
		
	err = connection.Open(server,KAfInet);
	TESTEL(err == KErrNone, err);
	 CleanupClosePushL(connection);
		
	//connection_start.script
	//TestCase1: Start the connection (outgoing) which results in 0, KErrNone, it pulls pppLcp for te_ppsize.xml
	// Verifies AgentAdapter state transition from EConnecting to EConnected 
    TCommDbConnPref prefs;
    prefs.SetIapId(0);
    prefs.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);
    
    connection.Start(prefs, status);
    User::WaitForRequest(status);
    TESTEL(status.Int() == KErrNone, status.Int());

    // Verifies AgentAdapter state transition from EConnected to EDisconnecting
    err = connection.Stop();
    TESTEL(err == KErrNone, err);
    CleanupStack::Pop();
        
    // close the socket server
	server.Close();
	CleanupStack::Pop();

	__UHEAP_MARKEND;

	return iTestStepResult;
	}
