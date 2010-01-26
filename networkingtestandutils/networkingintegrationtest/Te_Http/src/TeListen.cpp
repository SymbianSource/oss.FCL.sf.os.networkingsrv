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

// includes
#include <test/testexecuteclient.h>
#include <e32base.h>
#include <c32root.h>
#include "TeListen.h"
#include "TeSocketListener.h"


_LIT(KConfig,				"Config");
_LIT(KPort,					"Port");

#if defined (__WINS__)
#define PDD_NAME _L("ECDRV")
#define LDD_NAME _L("ECOMM")
#else
#define PDD_NAME _L("EUART1")
#define LDD_NAME _L("ECOMM")
#endif

CTestStepListen::CTestStepListen(CTestListenerMgr* aListenerMgr) 
: CTestStepBase(aListenerMgr), iListenerMgr(aListenerMgr)
	{
	SetTestStepName(KServerListen);
	}

CTestStepListen::~CTestStepListen()
	{
	}

TVerdict CTestStepListen::doTestStepPreambleL()
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

TVerdict CTestStepListen::doTestStepPostambleL()
	{
	CActiveScheduler::Install(NULL);
	delete iScheduler;
	iScheduler=NULL;
	return CTestStep::doTestStepPostambleL();
	}

TVerdict CTestStepListen::doTestStepL()
	{
	if ( TestStepResult()==EPass )
		{
		// Create the listener socket object
		CreateListenerL();

		// Connect to the socket server
		User::LeaveIfError(iListenerMgr->iSocketServOne.Connect());

		// Push the iSocketServOne onto the cleanup stack
		CleanupClosePushL(iListenerMgr->iSocketServOne);

		// Get the port number from the ini file
		TInt port = 0;

		if ( !GetIntFromConfig(KConfig, KPort, port) )
			{
			ERR_PRINTF1(_L("Failed to read Port from ini file"));
			SetTestStepResult(EFail);
			return EFail;
			}
		INFO_PRINTF2((_L("Port = %d")), port);

		// Socket listener starts listening on the specified port
		iListener->Listen( (TUint16) port);

		// Start the active scheduler
		CActiveScheduler::Start();

		// Pop the socket server object
		CleanupStack::Pop(&iListenerMgr->iSocketServOne);
			
		// Close the socket server
		iListenerMgr->iSocketServOne.Close();
		}

	return TestStepResult();
	}
