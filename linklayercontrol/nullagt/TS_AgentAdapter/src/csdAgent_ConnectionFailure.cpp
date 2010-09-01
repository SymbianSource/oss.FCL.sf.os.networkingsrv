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


CTestStepCsdAgtConnectionFailure::CTestStepCsdAgtConnectionFailure(TPtrC aName)
{
	iTestStepName=aName;
}

enum TVerdict CTestStepCsdAgtConnectionFailure::doTestStepL(void)
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
		
	// Connection_Start_Failure.script
	// Test Case2: Start the connection (outgoing) which results in a -1 Failure, it pulls pppLcp record from cedout.cfg
	// It verifies AgentAdapter State transition from EConnecting to EDisconnecting state 
	connection.Start(status);
	User::WaitForRequest(status);
	TESTEL(status.Int() == KErrNotFound, status.Int());
    
	// Verifies the AgentAdapter State is in EDisconnecting State, Upon Agent Start ending in  a failure
    // Try to stop connection that failed to start
	err = connection.Stop();
	TESTEL(err == KErrNotReady, err);
	CleanupStack::Pop();

	// close the socket server
	server.Close();
	CleanupStack::Pop();

	__UHEAP_MARKEND;

	return iTestStepResult;
	}



