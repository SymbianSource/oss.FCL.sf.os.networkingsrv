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
// Configuration Tests (test docs 4.5)
// 
//

/**
 @file
*/
#include "te_dhcpTestStep6.h"
#include <test/testexecutelog.h>
#include <commdbconnpref.h>
#include <in_sock.h>
#include <nifman.h>
#include <comms-infras/es_config.h>
#include <comms-infras/startprocess.h>
#include "Te_TestDaemonClient.h"


TVerdict CDhcpTestStep12_1::doTestStepL()
/** 
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Inform Scenario (test case 4.7.1)
*/
	{
	SetTestStepResult(EFail);
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);
	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
#ifdef _DEBUG
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv4, 67));
#endif		
	
	err = connNET1.Start(iConnPrefs);
	TESTEL(err == KErrNotFound, err);
	
	err = connNET1.Stop();
	TESTEL(err == KErrNotReady, err);

	CleanupStack::PopAndDestroy(&connNET1);
	CleanupStack::PopAndDestroy(&eSock);
	
	SetTestStepResult(EPass);
	return TestStepResult();
	}

