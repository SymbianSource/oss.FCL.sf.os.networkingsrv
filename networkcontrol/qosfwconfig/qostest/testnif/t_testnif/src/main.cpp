// Copyright (c) 2001-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <e32std.h>
#include <c32comm.h>
#include <e32hal.h>
#include "ss_pman.h"
#include "common.h"
#include "TestMgr.h"
#include "Tlog.h"

RTest test(_L("TestNif UnitTest"));
RLog  TestLog(_L("c:\\testnif.log"), &test);

void StartClientL();

TInt E32Main()
{
	
	test.Start(_L("Doing TestNif Unit Test"));
	TTime time;         // time in microseconds since 0AD nominal Gregorian
	TDateTime dateTime; // year-month-day-hour-minute-second-microsecond

	time.HomeTime(); // set time to home time
	dateTime=time.DateTime();    // convert to fields

	_LIT(KFormat1,"These tests were performed on  : %d/%d/%d %d:%d:%d\n");
	test.Title();
	TestLog.Printf(_L("Unit Test Version 0.1\n"));
	TestLog.Printf(KFormat1, dateTime.Day() + 1, TInt(dateTime.Month()+1), dateTime.Year(), dateTime.Hour(), dateTime.Minute(), dateTime.Second());
	TestLog.Printf(_L("Launching the tests....\n"));
	TestLog.Printf(_L("====================================\n"));

	TestLog.Printf(_L("Initialising test environment\n"));

	CTrapCleanup* pTrapCleanup=CTrapCleanup::New();
	if (!pTrapCleanup)
	{
		TestLog.Printf(_L("Failed to allocate CTrapCleanup object"));
		return KErrNoMemory;
	}

	CActiveScheduler* pActiveScheduler=new CActiveScheduler;
	if (!pActiveScheduler)
	{
		TestLog.Printf(_L("Failed to allocate CActiveScheduler object"));
		return KErrNoMemory;
	}
	CActiveScheduler::Install(pActiveScheduler);
	// Go go!!!
	TInt err=KErrNone;
	TRAP(err, StartClientL());
	CHECKPOINT(err,KErrNone,_L("Leave in StartClientL()"));

	TestLog.Printf(_L("Press any key......\n"));
	test.Getch();



	TestLog.Printf(_L("Cleaning up test environment"));
	SocketServer::__DbgForceKillMBufManager();//for creation see StartClientL below
	delete pActiveScheduler;
	delete pTrapCleanup;

	test.End();

	return err;
}

void StartClientL()
{
	SocketServer::__DbgForceLoadMBufManagerL();//without this the PPP'll have GPF while trying to get the buffers
	CTestMgr* pMgr= CTestMgr::NewL();		
	CleanupStack::PushL(pMgr);
	pMgr->StartLoadingAgentL();
	CActiveScheduler::Start();
	CleanupStack::PopAndDestroy();  // pMgr
}

