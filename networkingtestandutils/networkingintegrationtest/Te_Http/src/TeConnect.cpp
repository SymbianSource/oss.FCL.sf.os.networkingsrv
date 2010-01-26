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
//

// includes
#include <test/testexecuteclient.h>
#include <e32base.h>
#include <e32std.h>
#include <c32root.h>

#include "TeConnect.h"
#include "TeSocketListener.h"


_LIT(KConfig,				"Config");
_LIT(KLocalHost,			"LocalHost");
_LIT(KPort,					"Port");

#if defined (__WINS__)
#define PDD_NAME _L("ECDRV")
#define LDD_NAME _L("ECOMM")
#else
#define PDD_NAME _L("EUART1")
#define LDD_NAME _L("ECOMM")
#endif

CTestStepConnect::CTestStepConnect(CTestListenerMgr* aListenerMgr)
: CTestStepBase(aListenerMgr), iListenerMgr(aListenerMgr)
	{
	SetTestStepName(KClientConnect);
	}

CTestStepConnect::~CTestStepConnect()
	{
	}

TVerdict CTestStepConnect::doTestStepPreambleL()
	{
	TVerdict	ret = CTestStep::doTestStepPreambleL();
	if (ret != KErrNone)
		{
		SetTestStepResult(EFail);
		return ret;
		}
	else
		{
		iScheduler = new CActiveScheduler();
		CActiveScheduler::Install(iScheduler);

		return ret;
		}
	}

TVerdict CTestStepConnect::doTestStepPostambleL()
	{
	CActiveScheduler::Install(NULL);
	delete iScheduler;
	iScheduler=NULL;
	return CTestStep::doTestStepPostambleL();
	}

TVerdict CTestStepConnect::doTestStepL()
	{
	if ( TestStepResult()==EPass )
		{
		// Create the connector socket object
		CreateConnectorL();

		// Connect to the socket server
		User::LeaveIfError(iListenerMgr->iSocketServTwo.Connect());

		// Push the iSocketServTwo onto the cleanup stack
		CleanupClosePushL(iListenerMgr->iSocketServTwo);

		// Get the port number from the ini file
		TInt port = 0;

		if ( !GetIntFromConfig(KConfig, KPort, port) )
			{
			ERR_PRINTF1(_L("Failed to read Port from ini file"));
			SetTestStepResult(EFail);
			return EFail;
			}
		INFO_PRINTF2((_L("Client side from config file : Port = %d")), port);

		// Get the local host from the ini file
	
		TPtrC		localhost;

		if ( GetStringFromConfig(KConfig, KLocalHost, localhost) )
			{
			INFO_PRINTF2(_L("Client Side from config file : LocalHost is : %S"), &localhost);
			}
		else
			{
			ERR_PRINTF1(_L("Client Side from config file : No LocalHost"));
			SetTestStepResult(EFail);
			return EFail;
			}

		// Start connecting to the listener socket on the specific port
		iConnector->ConnectL(localhost, (TUint16)port);
		
		// Start the active scheduler
		CActiveScheduler::Start();

		// Pop the socket server object
		CleanupStack::Pop(&iListenerMgr->iSocketServTwo);

		// Close the socket server
		iListenerMgr->iSocketServTwo.Close();
		}

	return TestStepResult();
	}

