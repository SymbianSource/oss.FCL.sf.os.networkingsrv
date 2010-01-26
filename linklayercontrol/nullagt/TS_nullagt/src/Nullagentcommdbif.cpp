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

CTestStepNullAgtCommDbIf::CTestStepNullAgtCommDbIf(TPtrC aName)
{
	iTestStepName=aName;
}

enum TVerdict CTestStepNullAgtCommDbIf::doTestStepL(void)
{
	__UHEAP_MARK;

	TInt r;                // the result of various operations
	TRequestStatus status; // status of asynchronous ops

	RSocketServ server;
	RConnection connection;

		// as ever we need a socket server to create connections...
	r = server.Connect();
	TESTEL(r == KErrNone, r);
	CleanupClosePushL(server);
	
	// open the connection
	// need a connection to get a null agent... the db reqs go to the agt
	r = connection.Open(server, KAfInet);
	TESTEL(r == KErrNone, r);
	CleanupClosePushL(connection);

	// read an integer value back from commdb (IAP\Id chosen)
	_LIT(KName1, "IAP\\id");
	TBufC<7> name1(KName1);
	TUint32 intval;
	r = connection.GetIntSetting(name1, intval);
	TESTEL(KErrNotReady == r, r);

	// read a bool value back from commdb (DialOutISP\IfPromptForAuth chosen)
	_LIT(KName2, "DialOutISP\\IfPromptForAuth");
	TBufC<27> name2(KName2);
	TBool boolval;
	r = connection.GetBoolSetting(name2, boolval);
	TESTEL(KErrNotReady == r, r);

	// TODO: read the equivalent of DialOutISP\\IfName that works these days

	// read a descriptor back from commdb (DialOutISP\IfName chosen)
	// this should fail as this field does not exist anymore
	_LIT(KName3, "DialOutISP\\IfName");
	TBufC<18> name3(KName3);
	TBuf<128> desval;
	r = connection.GetDesSetting(name3, desval);
	TESTEL(KErrNotReady == r, r);

	// read a long descriptor back from commdb (DialOutISP\LoginScript chosen)
	_LIT(KName4, "DialOutISP\\LoginScript");
	TBufC<23> name4(KName4);
	TBuf<128> longdesval;
	r = connection.GetLongDesSetting(name4, longdesval);
	TESTEL(KErrNotReady == r, r);
	
	// just use defaults, any connection will do for this test
	connection.Start(status);
	User::WaitForRequest(status);
	TESTEL(status.Int() == KErrNone, status.Int());

	// read an integer value back from commdb (IAP\Id chosen)
	_LIT(KName5, "IAP\\id");
	TBufC<7> name5(KName5);
	r = connection.GetIntSetting(name5, intval);
	TESTEL(KErrNone == r, r);

	// read a bool value back from commdb (DialOutISP\IfPromptForAuth chosen)
	_LIT(KName6, "DialOutISP\\IfPromptForAuth");
	TBufC<27> name6(KName6);
	r = connection.GetBoolSetting(name6, boolval);
	TESTEL(KErrNone == r, r);

	// TODO: read the equivalent of DialOutISP\\IfName that works these days

	// read a descriptor back from commdb (DialOutISP\IfName chosen)
	// this should fail as this field does not exist anymore
	_LIT(KName7, "DialOutISP\\IfName");
	TBufC<18> name7(KName7);
	r = connection.GetDesSetting(name7, desval);
	TESTEL(KErrNotFound == r, r);

	// read a long descriptor back from commdb (DialOutISP\LoginScript chosen)
	_LIT(KName8, "DialOutISP\\LoginScript");
	TBufC<23> name8(KName8);
	r = connection.GetLongDesSetting(name8, longdesval);
	TESTEL(KErrNone == r, r);
	
	// destroy the connection
	r = connection.Stop();
	TESTEL(r == KErrNone, r);
	CleanupStack::Pop();

	// close the socket server
	server.Close();
	CleanupStack::Pop();

	__UHEAP_MARKEND;

	return iTestStepResult;
}
