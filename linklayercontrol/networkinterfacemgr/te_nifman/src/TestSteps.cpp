// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// OpenCloseBogus.cpp: implementation of the COpenCloseBogus class.
// 
//

/**
 @file teststeps.cpp
*/
//
#include <e32base.h>
#include <testexecutelog.h>
#include "TestSteps.h"
#include "tnifman.h"
#include "te_nifman_server.h"
#include <es_dummy.h>
#include <c32root.h>

//
// Construction/Destruction
//

COpenCloseBogus::COpenCloseBogus()
	{
	SetTestStepName(KOpenCloseBogus);
	}

COpenCloseBogus::~COpenCloseBogus()
	{ }

TVerdict COpenCloseBogus::doTestStepL()
	{
	SetTestStepResult(EPass);

	INFO_PRINTF1(_L("============================================\n"));
	INFO_PRINTF1(_L("Test : test1\n"));
	INFO_PRINTF1(_L("============================================\n"));

	RTestNif one;

	INFO_PRINTF1(_L("Open/close with bogus name"));
	TInt Err = one.Open(_L("testagent"));
	TESTEL(Err == KErrNone, Err);
	one.Close();

	RLibrary r;

	Err = r.Load(_L("bogus.agt"));
	TESTEL(Err == KErrNotFound, Err);

	return TestStepResult();
	}

//
// CSocketServerShutdown Class
//

//
// Construction/Destruction
//

CSocketServerShutdown::CSocketServerShutdown()
	{
	SetTestStepName(KSocketServerShutdown);
	}

CSocketServerShutdown::~CSocketServerShutdown()
	{ }

TVerdict CSocketServerShutdown::doTestStepL()
   /** 
   Connects to the Comms Rootserver and requests unload of the Socket Server CPM.
   Returns when it is unloaded.
   @returns Error code
   */
	{
	SetTestStepResult(EPass);

	INFO_PRINTF1(_L("============================================\n"));
	INFO_PRINTF1(_L("Test : test2\n"));
	INFO_PRINTF1(_L("============================================\n"));

	INFO_PRINTF1(_L("Connecting to Rootserver"));
   	// Connect to root server
   	RRootServ rootserver;
   	TInt err(rootserver.Connect());
   	if (KErrNone != err)
 	    {
   		return EFail;
        }
   
	INFO_PRINTF1(_L("Waiting for socket server shutdown"));
   	// Ask Socket Server to unload when there is no sessions
   	TRequestStatus status;
   	TCFModuleName name(_L8("CSocketServer"));
   	rootserver.UnloadCpm(status, name, EGraceful);
   
   	// Wait for that to happen
   	User::WaitForRequest(status);
   
   	rootserver.Close();

	INFO_PRINTF1(_L("[  OK  ]\n"));
	INFO_PRINTF1(_L("Socket Server Shutdown\n"));

	return TestStepResult();
	}

//
// CStartStopInterfaces Class
//

//
// Construction/Destruction
//

CStartStopInterfaces::CStartStopInterfaces()
	{
	SetTestStepName(KStartStopInterfaces);
	}

CStartStopInterfaces::~CStartStopInterfaces()
	{ }


TVerdict CStartStopInterfaces::doTestStepL()
	{
	SetTestStepResult(EPass);

	INFO_PRINTF1(_L("============================================\n"));
	INFO_PRINTF1(_L("Test : test3\n"));
	INFO_PRINTF1(_L("============================================\n"));

	return TestStepResult();
	}

CProgressNotification::CProgressNotification()
	{
	SetTestStepName(KProgressNotification);
	}

CProgressNotification::~CProgressNotification()
	{ }


TVerdict CProgressNotification::doTestStepL()
	{
	SetTestStepResult(EPass);
	INFO_PRINTF1(_L("============================================\n"));
	INFO_PRINTF1(_L("Test : test4\n"));
	INFO_PRINTF1(_L("============================================\n"));

	return TestStepResult();
	}

CConnectReconnect::CConnectReconnect()
	{
	SetTestStepName(KConnectReconnect);
	}

CConnectReconnect::~CConnectReconnect()
	{ }


TVerdict CConnectReconnect::doTestStepL()
	{
	SetTestStepResult(EPass);
	INFO_PRINTF1(_L("============================================\n"));
	INFO_PRINTF1(_L("Test : test6\n"));
	INFO_PRINTF1(_L("============================================\n"));


	return TestStepResult();
	}


CTest8::CTest8()
	{
	SetTestStepName(KTest8);
	}

CTest8::~CTest8()
	{ }


TVerdict CTest8::doTestStepL()
	{
	SetTestStepResult(EFail);

	INFO_PRINTF1(_L("============================================\n"));
	INFO_PRINTF1(_L("Test : test8\n"));
	INFO_PRINTF1(_L("============================================\n"));

	return TestStepResult();
	}


CBinderLayerDown::CBinderLayerDown()
	{
	SetTestStepName(KBinderLayerDown);
	}

CBinderLayerDown::~CBinderLayerDown()
	{ }


TVerdict CBinderLayerDown::doTestStepL()
	{
	SetTestStepResult(EPass);

	INFO_PRINTF1(_L("============================================\n"));
	INFO_PRINTF1(_L("Test : test20\n"));
	INFO_PRINTF1(_L("============================================\n"));

	INFO_PRINTF1(_L("Testing binder layer down functionality ..."));
	return TestStepResult();
	}

CTest5::CTest5()
	{
	SetTestStepName(KTest5);
	}

CTest5::~CTest5()
	{}

TVerdict CTest5::doTestStepL()
	{
	SetTestStepResult(EPass);
	
	INFO_PRINTF1(_L("============================================\n"));
	INFO_PRINTF1(_L("Test : test5 - not implemented\n"));
	INFO_PRINTF1(_L("============================================\n"));

	return TestStepResult();
	}

//
// Construction/Destruction
//

COpenClosePSD::COpenClosePSD()
	{
	SetTestStepName(KOpenClosePSD);
	}

COpenClosePSD::~COpenClosePSD()
	{ }

TVerdict COpenClosePSD::doTestStepL()
	{
	SetTestStepResult(EPass);

	INFO_PRINTF1(_L("============================================\n"));
	INFO_PRINTF1(_L("Test : test21\n"));
	INFO_PRINTF1(_L("============================================\n"));
	return TestStepResult();
	}

