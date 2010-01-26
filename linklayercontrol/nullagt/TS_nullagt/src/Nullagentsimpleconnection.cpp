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
// Contain the implementation of the class for this null agent test
// 
//

#include "NullAgentTestSteps.h"
#include "es_sock.h"
#include "in_sock.h"

CTestStepNullAgtSimpleConnection::CTestStepNullAgtSimpleConnection(TPtrC aName)
{
	iTestStepName=aName;
}

enum TVerdict CTestStepNullAgtSimpleConnection::doTestStepL(void)
{
	__UHEAP_MARK;

	TInt r; // the result of various operations
	RSocketServ server;
	RConnection connection;
	TRequestStatus status;

	// connect to the socket server
	r = server.Connect();
	TESTEL(r == KErrNone, r);
	CleanupClosePushL(server);
	
	// we created a socket server so we had it when opening the connection...
	r = connection.Open(server, KAfInet);
	TESTEL(r == KErrNone, r);
	CleanupClosePushL(connection);
	
	// start the connection (outgoing)
	connection.Start(status);
	User::WaitForRequest(status);
	TESTEL(status.Int() == KErrNone, status.Int());
	
	// force it to be torn down
	r = connection.Stop();
	TESTEL(r == KErrNone, r);
	CleanupStack::Pop();

	// close the socket server
	server.Close();
	CleanupStack::Pop();

	__UHEAP_MARKEND;

	return iTestStepResult;
}
