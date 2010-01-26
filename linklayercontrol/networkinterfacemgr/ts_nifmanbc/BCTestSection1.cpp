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
//

#include "BCTestSection1.h"
#include <cdbstore.h>
#include <c32root.h>

CTestStep1_1::CTestStep1_1() 
//
// C'tor
//
	{

	_LIT(KTestStepName, "Test1_1");
	iTestStepName = KTestStepName();
	}

enum TVerdict CTestStep1_1::doTestStepL()
//
// Test RNif and RGenericAgent Open functions
//
	{

	RNif nif;
    
	TInt err(KErrNone);

    err = Nifman::CheckIniConfig();
	TESTE(err==KErrNone, err);
    
	err = nif.Open();
	TESTE(err==KErrNone, err);
	nif.Close();

	err = nif.Open(_L("genconn"));
	TESTE(err==KErrNone, err);
	nif.Close();

	err = nif.Open(_L("GENCONN"));
	TESTE(err==KErrNone, err);
	nif.Close();

	err = nif.Open(_L("gEnConN"));
	TESTE(err==KErrNone, err);
	nif.Close();

	err = nif.Open(_L("genconn.agt"));
	TESTE(err==KErrNone, err);
	nif.Close();

	err = nif.Open(_L("csd"));
	TESTE(err==KErrNotSupported, err);
	nif.Close();

	err = nif.Open(_L("bogus"));
	TESTE(err==KErrNotSupported, err);
	nif.Close();

	RGenericAgent agent;

	err = agent.Open();
	TESTE(err==KErrNone, err);
	agent.Close();

	return iTestStepResult;
	}


CTestStep1_2::CTestStep1_2() 
//
// C'tor
//
	{

	_LIT(KTestStepName, "Test1_2");
	iTestStepName = KTestStepName();
	}

enum TVerdict CTestStep1_2::doTestStepL()
//
// Test RGenericAgent Start function
//
	{

	TInt err(KErrNone);

	RGenericAgent agent;
	err = agent.Open();
	TESTE(err==KErrNone, err);

	TRequestStatus status;
	agent.StartOutgoing(status);
	User::WaitForRequest(status);

	agent.Stop();
	agent.Close();

	return iTestStepResult;
	}

CTestStep1_3::CTestStep1_3() 
//
// C'tor
//
	{

	_LIT(KTestStepName, "Test1_3");
	iTestStepName = KTestStepName();
	}

enum TVerdict CTestStep1_3::doTestStepL()
//
// Test RGenericAgent progress retrieval
//
	{

	TInt err(KErrNone);
	RGenericAgent agent;

	err = agent.Open();
	TESTE(err==KErrNone, err);

	TNifProgressBuf progress;
	TRequestStatus  progStatus;

	agent.ProgressNotification(progress, progStatus);
		
	TRequestStatus startStatus;
	agent.StartOutgoing(startStatus);

    TBool bConnected = EFalse; //-- this flag indicates that we have started connection

	FOREVER
		{

		User::WaitForRequest(progStatus, startStatus);

		if(progStatus.Int() == KRequestPending)
			{
			// progStatus is still pending, so startStatus must have been completed

			if(startStatus.Int() != KErrNone)
				{
				Log(_L("Start completed with error %d"), startStatus.Int());
				iTestStepResult = EFail;
				break;
				}

			    bConnected = ETrue; //-- the connection has been started successfully
			}
		else
			{

			if(progStatus.Int() != KErrNone)
				{
				Log(_L("ProgressNotification completed with error %d"), progStatus.Int());
				iTestStepResult = EFail;
				break;
				}

            //-- re-subscribe to the notification ASAP
            agent.ProgressNotification(progress, progStatus);
            
            Log(_L("Progress: stage %d, error %d"), progress().iStage, progress().iError);            

            //-- it's necessarily to check the progress status, because it can change (and changes)
            //-- very quickly even during logging, so WaitForRequest above might not be reached
            agent.Progress(progress());

            //Log(_L("--Progress: stage %d, error %d"), progress().iStage, progress().iError);            
			if( bConnected && progress().iStage == KAgentUninitialised )
				{
				    //-- the connection has been established before and became KConnectionUninitialised
				    //-- a good reason to exit.
				    Log(_L("Agent has finished"));
			    	agent.CancelProgressNotification();
					User::WaitForRequest(progStatus);
			    	break;
				}

            

			}
		}

	if(startStatus.Int() == KRequestPending)
		{
		err = agent.Stop();
		TESTE(err==KErrNone, err);
		User::WaitForRequest(startStatus);

		TESTE(startStatus.Int() == KErrCancel, startStatus.Int());
		}

	if(progStatus.Int() == KRequestPending)
		{
		agent.CancelProgressNotification();
		User::WaitForRequest(progStatus);

		TESTE(progStatus.Int() == KErrCancel, progStatus.Int());
		}

	agent.Close();

	return iTestStepResult;
	}

CTestStep1_4::CTestStep1_4() 
//
// C'tor
//
	{

	_LIT(KTestStepName, "Test1_4");
	iTestStepName = KTestStepName();
	}

enum TVerdict CTestStep1_4::doTestStepL()
//
// Test RGenericAgent CancelOutgoingErrorNotification function
//
	{

	TInt err(KErrNone);
	RGenericAgent agent;

	err = agent.Open();
	TESTEL(err==KErrNone, err);
	CleanupClosePushL(agent);

	TRequestStatus startStatus;
	agent.StartOutgoing(startStatus);
	TESTEL(startStatus.Int()==KRequestPending, startStatus.Int());

	agent.CancelOutgoingErrorNotification();

	User::WaitForRequest(startStatus);
	TESTE(startStatus.Int()==KErrCancel, startStatus.Int());

	TNifProgressBuf progress;
	TRequestStatus progStatus;

	agent.ProgressNotification(progress, progStatus);

	FOREVER																    
		{
		User::WaitForRequest(progStatus);
		TESTE(progStatus.Int()==KErrNone, progStatus.Int());

        agent.ProgressNotification(progress, progStatus);
        //-- it's necessarily to check the progress status, because it can change (and changes)
        //-- very quickly even during logging or calling  ProgressNotification
        agent.Progress(progress());
		Log(_L("Progress: stage %d, error %d"), progress().iStage, progress().iError);
		
		if(progress().iStage == KAgentUninitialised)
			{
			Log(_L("Agent has finished"));
			agent.CancelProgressNotification();
			User::WaitForRequest(progStatus);
			break;
			}

		}

	agent.Close();
	CleanupStack::Pop(); // agent

	return iTestStepResult;
	}

CTestStep1_5::CTestStep1_5() 
//
// C'tor
//
	{

	_LIT(KTestStepName, "Test1_5");
	iTestStepName = KTestStepName();
	}

enum TVerdict CTestStep1_5::doTestStepL()
//
// Test double connection start with the same overrides
//
	{
	
	TInt err(KErrNone);

	CCommsDbConnectionPrefTableView::TCommDbIapConnectionPref iapPref;
	iapPref.iRanking = 1;
	iapPref.iDirection = ECommDbConnectionDirectionOutgoing;
	iapPref.iDialogPref = ECommDbDialogPrefDoNotPrompt;
	
	CStoreableOverrideSettings* overrides = CStoreableOverrideSettings::NewL(CCommDbOverrideSettings::EParamListPartial);
	CleanupStack::PushL(overrides);
	err = overrides->SetConnectionPreferenceOverride(iapPref);
	TESTEL(err==KErrNone, err);
	err = overrides->SetIntOverride(TPtrC(CONNECTION_ATTEMPTS), KNullDesC, 1);
	TESTEL(err==KErrNone, err);

	RGenericAgent conn1;
	err = conn1.Open();
	TESTEL(err==KErrNone, err);
	CleanupClosePushL(conn1);

	err = conn1.StartOutgoing(*overrides);
	TESTE(err==KErrNone, err);

	RGenericAgent conn2;
	conn2.Open();
	TESTEL(err==KErrNone, err);
	CleanupClosePushL(conn1);

	err = conn2.StartOutgoing(*overrides);
	TESTE(err==KErrNone, err);

	err = conn1.Stop();
	TESTE(err==KErrNone, err);

	err = conn2.Stop();
	TESTE(err==KErrNone, err);

	conn2.Close();
	CleanupStack::Pop();  // conn2

	conn1.Close();
	CleanupStack::Pop();  // conn1

	CleanupStack::PopAndDestroy(overrides);
	return iTestStepResult;
	}

CTestStep1_6::CTestStep1_6() 
//
// C'tor
//
	{

	_LIT(KTestStepName, "Test1_6");
	iTestStepName = KTestStepName();
	}

enum TVerdict CTestStep1_6::doTestStepL()
//
// Test double connection start with the differing overrides
//
	{
	
	TInt err(KErrNone);

	CCommsDbConnectionPrefTableView::TCommDbIapConnectionPref iapPref;
	iapPref.iRanking = 1;
	iapPref.iDirection = ECommDbConnectionDirectionOutgoing;
	iapPref.iDialogPref = ECommDbDialogPrefDoNotPrompt;
	iapPref.iBearer.iIapId = 1;
	
	CStoreableOverrideSettings* overrides1 = CStoreableOverrideSettings::NewL(CCommDbOverrideSettings::EParamListPartial);
	CleanupStack::PushL(overrides1);
	err = overrides1->SetConnectionPreferenceOverride(iapPref);
	TESTEL(err==KErrNone, err);
	err = overrides1->SetIntOverride(TPtrC(CONNECTION_ATTEMPTS), KNullDesC, 1);
	TESTEL(err==KErrNone, err);

	CCommsDbConnectionPrefTableView::TCommDbIapConnectionPref iapPref2;
	iapPref2.iRanking = 1;
	iapPref2.iDirection = ECommDbConnectionDirectionOutgoing;
	iapPref2.iBearer.iIapId = 2;

	CStoreableOverrideSettings* overrides2 = CStoreableOverrideSettings::NewL(CCommDbOverrideSettings::EParamListPartial);
	CleanupStack::PushL(overrides2);
	err = overrides2->SetConnectionPreferenceOverride(iapPref2);
	TESTEL(err==KErrNone, err);

	RGenericAgent conn1;
	err = conn1.Open();
	TESTEL(err==KErrNone, err);
	CleanupClosePushL(conn1);

	err = conn1.StartOutgoing(*overrides1);
	TESTE(err==KErrNone, err);

	RGenericAgent conn2;
	conn2.Open();
	TESTEL(err==KErrNone, err);
	CleanupClosePushL(conn1);

	err = conn2.StartOutgoing(*overrides2);
	TESTE(err==KErrInUse, err);

	err = conn1.Stop();
	TESTE(err==KErrNone, err);

	conn2.Close();
	CleanupStack::Pop();  // conn2

	conn1.Close();
	CleanupStack::Pop();  // conn1

	CleanupStack::PopAndDestroy(overrides2);
	CleanupStack::PopAndDestroy(overrides1);

	return iTestStepResult;
	}

CTestStep1_7::CTestStep1_7() 
//
// C'tor
//
	{

	_LIT(KTestStepName, "Test1_7");
	iTestStepName = KTestStepName();
	}

enum TVerdict CTestStep1_7::doTestStepL()
//
// Test GetActiveXSetting() functions
//
	{
	
	TInt err(KErrNone);

	RGenericAgent agent;
	err = agent.Open();
	TESTEL(err==KErrNone, err);
	CleanupClosePushL(agent);
	
	// haven't started the connection, following calls should fail

	TUint32 intSetting;
	err = agent.GetActiveIntSetting(TPtrC(MODEM_BEARER), TPtrC(LAST_SOCKET_ACTIVITY_TIMEOUT), intSetting);
	TESTE(err==KErrNotReady, err);

	TBool boolSetting;
	err = agent.GetActiveBoolSetting(TPtrC(DIAL_OUT_ISP), TPtrC(SERVICE_IP_ADDR_FROM_SERVER), boolSetting);
	TESTE(err==KErrNotReady, err);

	TBuf<KCommsDbSvrMaxFieldLength> des16Setting;
	err = agent.GetActiveDesSetting(TPtrC(IAP), TPtrC(COMMDB_NAME), des16Setting);
	TESTE(err==KErrNotReady, err);

	agent.Close();
	CleanupStack::Pop();  // agent

	// start again

	err = agent.Open();
	TESTEL(err==KErrNone, err);
	CleanupClosePushL(agent);

	// this time start the interface and see that it works now
	TRequestStatus status;
	agent.StartOutgoing(status);
	User::WaitForRequest(status);

	err = agent.GetActiveIntSetting(TPtrC(MODEM_BEARER), TPtrC(LAST_SOCKET_ACTIVITY_TIMEOUT), intSetting);
	TESTE(err==KErrNone, err);

	err = agent.GetActiveBoolSetting(TPtrC(DIAL_OUT_ISP), TPtrC(SERVICE_IP_ADDR_FROM_SERVER), boolSetting);
	TESTE(err==KErrNone, err);

	err = agent.GetActiveDesSetting(TPtrC(IAP), TPtrC(COMMDB_NAME), des16Setting);
	TESTE(err==KErrNone, err);

	agent.Close();
	CleanupStack::Pop();  // agent

	return iTestStepResult;
	}

