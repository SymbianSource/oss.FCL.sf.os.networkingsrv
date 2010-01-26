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
 @file ts_ipsec_rconn.cpp 
*/

#include <networking/log.h>

#include "ts_ipsec_rconn.h"
#include "connectiontester.h" // gets the active object

#define WAIT_PERIOD 4000000 //Microseconds

namespace
	{	
	_LIT(KNetId, "NetworkId");
	_LIT(KIapId, "IAPId");
	_LIT(KNetId2, "NetworkId2");
	_LIT(KIapId2, "IAPId2");
	_LIT(KCommonSection, "Test_Common");
	}

#define MODIFY_VERDICT(x) if((x != KErrNone) && (isFail != EFail)) \
							isFail = EFail 

//
// Real connection test with progress notification
//
CIpsecConnTest_1::CIpsecConnTest_1(CTestScheduler* aScheduler) : iScheduler(*aScheduler)
/**
 * Each test step initialises it's own name. Stores the name of this test case.
 * This is the name that is used by the script file
 */
	{
	_LIT(KTestStepName, "IpsecConectionTest1");
	iTestStepName.Copy(KTestStepName);
	}

CIpsecConnTest_1::~CIpsecConnTest_1()
	{
	}

enum TVerdict CIpsecConnTest_1::doTestStepL()
	{
	__UHEAP_MARK;
	CLogger *logger = new(ELeave) CLogger(*this->iSuite);
	CleanupStack::PushL(logger);

	// Create the active objects
	CConnTester* tester = CConnTester::NewLC(logger);

	TInt IapId, NetId;
	TBool b = GetIntFromConfig(KCommonSection, KNetId, NetId);
	if(!b)
		{
		Log(_L("Could not read KNetId from config"));
		NetId = 0;
		}

	b = GetIntFromConfig(KCommonSection, KIapId, IapId);
	if(!b)
		{
		Log(_L("Could not read KIapId from config"));
		TESTL(b);
		}
	
	Log(_L("NetworkId = %d"), NetId);
	Log(_L("IAPId = %d"), IapId);

	tester->StartConenction(IapId, NetId);
	tester->GetProgressL();

	// The scheduler starts, and completes when the AOs are done
	iScheduler.Start();

	TInt err = tester->GetResult();

	Log(_L("The Conenction was started with result %d"), err);

	__UHEAP_MARK;
	tester->PrintRoutingTableL();
	__UHEAP_MARKEND;

	if (!err)
		{
		TInetAddr addrRemote;
		TESTL(GetIpAddressFromConfig(KCommonSection, _L("IpAddress"), addrRemote));
		// get the port number
		TInt Port;
		TESTL(GetIntFromConfig(KCommonSection, _L("Port"), Port));
		
		TInt type;
		TESTL(GetIntFromConfig(KCommonSection, _L("TestType"), type));
		
		tester->StartTransferL(addrRemote, Port, type);

		iScheduler.Start();
		err = tester->GetResult();
		Log(_L("The data was tranfered with result %d"), err);
		TESTL(!err);
		TRAP(err, tester->GetStatsL());
		Log(_L("Read stats with result %d"), err);
		TESTL(!err);
		}

	if (!err)
		{
		CConnTester* tester2 = CConnTester::NewLC(logger);
		tester2->StopConnection(IapId, NetId);
		CleanupStack::PopAndDestroy(1);
		}
//	else tester->StopConnection(IapId, NetId);

	err = tester->GetResult();
	Log(_L("The Conenction was stopped with result %d"), err);
	TESTL(!err);
	
	// Get rid of the Active objects
	CleanupStack::PopAndDestroy(2); // tester, logger
		
	// Find the verdict from the callbacks
	__UHEAP_MARKEND;
	return err?EFail:EPass;
	}

//
// 2 connections test
//
CIpsecConnTest_2::CIpsecConnTest_2(CTestScheduler* aScheduler) : iScheduler(*aScheduler)
/**
 * Each test step initialises it's own name. Stores the name of this test case.
 * This is the name that is used by the script file
 */
	{
	_LIT(KTestStepName, "IpsecConectionTest2");
	iTestStepName.Copy(KTestStepName);
	}

CIpsecConnTest_2::~CIpsecConnTest_2()
	{
	}

enum TVerdict CIpsecConnTest_2::doTestStepL()
	{
	__UHEAP_MARK;

	TVerdict isFail = EPass;

	CLogger *logger = new(ELeave) CLogger(*this->iSuite);
	CleanupStack::PushL(logger);

	// Create the active objects
	CConnTester* tester1 = CConnTester::NewLC(logger);
	CConnTester* tester2 = CConnTester::NewLC(logger);
			
	TInt IapId1, NetId1;
	TBool b = GetIntFromConfig(KCommonSection, KNetId, NetId1);
	if(!b)
		{
		Log(_L("Could not read KNetId from config"));
		NetId1 = 0;
		}

	b = GetIntFromConfig(KCommonSection, KIapId, IapId1);
	if(!b)
		{
		Log(_L("Could not read KIapId from config"));
		TESTL(b);
		}

	TInt IapId2, NetId2;
	b = GetIntFromConfig(KCommonSection, KNetId2, NetId2);
	if(!b)
		{
		Log(_L("Could not read KNetId from config"));
		NetId2 = 0;
		}

	b = GetIntFromConfig(KCommonSection, KIapId2, IapId2);
	if(!b)
		{
		Log(_L("Could not read KIapId from config"));
		TESTL(b);
		}
	
	Log(_L("NetworkId 1 => %d"), NetId1);
	Log(_L("IAPId 1 => %d"), IapId1);
	tester1->StartConenction(IapId1, NetId1);
	tester1->GetProgressL();

	Log(_L("NetworkId 2 => %d"), NetId2);
	Log(_L("IAPId 2 => %d"), IapId2);
	tester2->StartConenction(IapId2, NetId2);
	tester2->GetProgressL();

	// The scheduler starts, and completes when the AOs are done
	iScheduler.Start();
	if (tester1->IsActive() || tester2->IsActive())
			iScheduler.Start();

	TInt err = tester1->GetResult();
	TESTEL(!err, err);

	err = tester2->GetResult();
	TESTEL(!err, err);

	__UHEAP_MARK;
	tester1->PrintRoutingTableL();
	__UHEAP_MARKEND;

	if (!err)
		{
		TInetAddr addrRemote;
		TESTL(GetIpAddressFromConfig(KCommonSection, _L("IpAddress"), addrRemote));
		// get the port number
		TInt Port;
		TESTL(GetIntFromConfig(KCommonSection, _L("Port"), Port));
		TInetAddr addrRemote2;
		TESTL(GetIpAddressFromConfig(KCommonSection, _L("IpAddress2"), addrRemote2));
		
		tester1->StartTransferL(addrRemote, Port, CSender::EReadWrite);
		tester2->StartTransferL(addrRemote2, Port, CSender::EReadWrite);
		iScheduler.Start();

		if (tester1->IsActive() || tester2->IsActive())
			iScheduler.Start();

		err = tester1->GetResult();
		Log(_L("The data was tranfered with 1 with result %d"), err);
		TESTE(!err, err);
		MODIFY_VERDICT(err);

		TRAP(err, tester1->GetStatsL());
		Log(_L("Read stats from 1 with result %d"), err);
		TESTE(!err, err);

		err = tester2->GetResult();
		Log(_L("The data was tranfered with 2 with result %d"), err);
		TESTE(!err, err);
		MODIFY_VERDICT(err);

		TRAP(err, tester2->GetStatsL());
		Log(_L("Read stats from 2 with result %d"), err);
		TESTE(!err, err);
		}
		
	tester1->StopConnection(IapId1, NetId1);
	err = tester1->GetResult();
	Log(_L("The Conenction 1 was stopped with result %d"), err);
	TESTE(!err, err);

	tester2->StopConnection(IapId2, NetId2);
	err = tester2->GetResult();
	Log(_L("The Conenction 2 was stopped with result %d"), err);
	TESTE(!err, err);
	
	// Get rid of the Active objects
	CleanupStack::PopAndDestroy(3); //2 testers, 1 logger
	
	Log(_L("Wait for %d seconds "), WAIT_PERIOD/1000000);
	RTimer timer;
	timer.CreateLocal();
	TRequestStatus status;
	timer.After(status, WAIT_PERIOD);
	User::WaitForRequest(status);
	timer.Close();

	// Return the verdict from the callbacks
	__UHEAP_MARKEND;
	return isFail;
	}

//
// 2 connections test - except this one starts and stops the real connection first
//
CIpsecConnTest_3::CIpsecConnTest_3(CTestScheduler* aScheduler) : iScheduler(*aScheduler)
/**
 * Each test step initialises it's own name. Stores the name of this test case.
 * This is the name that is used by the script file
 */
	{
	_LIT(KTestStepName, "IpsecConectionTest3");
	iTestStepName.Copy(KTestStepName);
	}

CIpsecConnTest_3::~CIpsecConnTest_3()
	{
	}

enum TVerdict CIpsecConnTest_3::doTestStepL()
	{
	__UHEAP_MARK;
	CLogger *logger = new(ELeave) CLogger(*this->iSuite);
	CleanupStack::PushL(logger);

	TVerdict isFail = EPass;
	// Create the active objects
	CConnTester* tester1 = CConnTester::NewLC(logger);
	CConnTester* tester2 = CConnTester::NewLC(logger);
			
	TInt IapId1, NetId1;
	TBool b = GetIntFromConfig(KCommonSection, KNetId, NetId1);
	if(!b)
		{
		Log(_L("Could not read KNetId from config"));
		NetId1 = 0;
		}

	b = GetIntFromConfig(KCommonSection, KIapId, IapId1);
	if(!b)
		{
		Log(_L("Could not read KIapId from config"));
		TESTL(b);
		}

	TInt IapId2, NetId2;
	b = GetIntFromConfig(KCommonSection, KNetId2, NetId2);
	if(!b)
		{
		Log(_L("Could not read KNetId from config"));
		NetId2 = 0;
		}

	b = GetIntFromConfig(KCommonSection, KIapId2, IapId2);
	if(!b)
		{
		Log(_L("Could not read KIapId from config"));
		TESTL(b);
		}
	
	Log(_L("NetworkId 1 => %d"), NetId1);
	Log(_L("IAPId 1 => %d"), IapId1);
	tester1->StartConenction(IapId1, NetId1);
	tester1->GetProgressL();

	Log(_L("NetworkId 2 => %d"), NetId2);
	Log(_L("IAPId 2 => %d"), IapId2);
	tester2->StartConenction(IapId2, NetId2);
	tester2->GetProgressL();

	// The scheduler starts, and completes when the AOs are done
	iScheduler.Start();
	if (tester1->IsActive() || tester2->IsActive())
			iScheduler.Start();

	TInt err = tester1->GetResult();
	TESTE(!err, err);
	MODIFY_VERDICT(err);

	err = tester2->GetResult();
	TESTE(!err, err);
	MODIFY_VERDICT(err);

	__UHEAP_MARK;
	tester1->PrintRoutingTableL();
	__UHEAP_MARKEND;

	if (!err)
		{
		TInetAddr addrRemote;
		TESTL(GetIpAddressFromConfig(KCommonSection, _L("IpAddress"), addrRemote));
		// get the port number
		TInt Port;
		TESTL(GetIntFromConfig(KCommonSection, _L("Port"), Port));
		TInetAddr addrRemote2;
		TESTL(GetIpAddressFromConfig(KCommonSection, _L("IpAddress2"), addrRemote2));
		
	//	tester1->StartTransferL(addrRemote, Port, 0);
		tester2->StartTransferL(addrRemote2, Port, 0);
		iScheduler.Start();

		if (tester1->IsActive() || tester2->IsActive())
			iScheduler.Start();

		err = tester1->GetResult();
	//	Log(_L("The data was tranfered with 1 with result %d"), err);
		TESTE(!err, err);
		MODIFY_VERDICT(err);

		TRAP(err, tester1->GetStatsL());
		Log(_L("Read stats from 1 with result %d"), err);
		TESTE(!err, err);
	
		err = tester2->GetResult();
		Log(_L("The data was tranfered with 2 with result %d"), err);
		TESTE(!err, err);
		MODIFY_VERDICT(err);

		TRAP(err, tester2->GetStatsL());
		Log(_L("Read stats from 2 with result %d"), err);
		TESTE(!err, err);
		}
		
	tester1->StopConnection(IapId1, NetId1);
	err = tester1->GetResult();
	Log(_L("The Conenction 1 was stopped with result %d"), err);
	TESTE(!err, err);
	MODIFY_VERDICT(err);

	tester2->StopConnection(IapId2, NetId2);
	err = tester2->GetResult();
	Log(_L("The Conenction 2 was stopped with result %d"), err);
	MODIFY_VERDICT(err);
	
	// Get rid of the Active objects
	CleanupStack::PopAndDestroy(3); // 2 testers, 1 logger
		
	// Return the verdict from the callbacks
	__UHEAP_MARKEND;
	return isFail;
	}


//
// IPSec Removed connections test, Tests that IPSec returns a valid error message when attempting to make an IPSec connection with the plugin removed.
//
CIpsecConnTest_4::CIpsecConnTest_4(CTestScheduler* aScheduler) : iScheduler(*aScheduler)
/**
 * Each test step initialises it's own name. Stores the name of this test case.
 * This is the name that is used by the script file
 */
	{
	_LIT(KTestStepName, "IpsecRemovedConnectionTest4");
	iTestStepName.Copy(KTestStepName);
	}
	
CIpsecConnTest_4::~CIpsecConnTest_4()
	{
	}	

enum TVerdict CIpsecConnTest_4::doTestStepL()
	{
	__UHEAP_MARK;
	CLogger *logger = new(ELeave) CLogger(*this->iSuite);
	CleanupStack::PushL(logger);

	Log(_L("Creating active Objects"));
	// Create the active objects
	CConnTester* tester = CConnTester::NewLC(logger);
	Log(_L("Tester Object Created"));
	TInt IapId, NetId;
	Log(_L("Getting Info from Config"));
	TBool b = GetIntFromConfig(KCommonSection, KNetId, NetId);
	if(!b)
		{
		Log(_L("Could not read KNetId from config"));
		NetId = 0;
		}

	b = GetIntFromConfig(KCommonSection, KIapId, IapId);
	if(!b)
		{
		Log(_L("Could not read KIapId from config"));
		TESTL(b);
		}
	
	Log(_L("NetworkId = %d"), NetId);
	Log(_L("IAPId = %d"), IapId);

	Log(_L("Starting COnnection"));
	tester->StartConenction(IapId, NetId);
	Log(_L("Getting Progress"));
	tester->GetProgressL();

	// The scheduler starts, and completes when the AOs are done
	Log(_L("Starting Scheduler"));
	iScheduler.Start();

	Log(_L("Get result from tester"));
	TInt err = tester->GetResult();

	Log(_L("The Conenction was not started as result %d"), err);
	
	TVerdict isFail = EFail;
	
	if ((err==KErrNotSupported)||(err==KErrNotFound))
		{
		isFail = EPass;
		}
	
	// Get rid of the Active objects
	CleanupStack::PopAndDestroy(2); // tester, logger
		
	// Find the verdict from the callbacks
	__UHEAP_MARKEND;
	return isFail;
	}


