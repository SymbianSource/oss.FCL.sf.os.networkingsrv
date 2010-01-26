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
// ts_ipsec_polapi.cpp
// 
//

/**
 @file ts_ipsec_polapi.cpp Implements main test code for IPsec
*/

#include <networking/log.h>

#include "vpntester.h" // gets the active object
#include "ts_ipsec_vpnapi.h"

CIpsecVpnTest_1::CIpsecVpnTest_1(CTestScheduler* aScheduler) : iScheduler(*aScheduler)
/**
 * Each test step initialises it's own name. Stores the name of this test case.
 * This is the name that is used by the script file
 */
	{
	_LIT(KTestStepName, "IpsecVpnApiTest1");
	iTestStepName.Copy(KTestStepName);
	}

CIpsecVpnTest_1::~CIpsecVpnTest_1()
	{
	}

enum TVerdict CIpsecVpnTest_1::doTestStepL()
	{
	__UHEAP_MARK;

	// Create the active objects
	CVpnTester* tester = CVpnTester::NewLC((CTestSuiteIpsec*)this->iSuite);
	
	_LIT(KPolicyDir,  "PolicyDir");
		
	TPtrC ptrResult;
	const TBool b = GetStringFromConfig(iTestStepName, KPolicyDir, ptrResult);
	if(!b)
		{
		Log(_L("Could not read policy file from config"));
		TESTL(EFalse);
		}

	Log(_L("PolicyDir = %S"), &ptrResult);

	tester->StartPolicyImportL(ptrResult);	// transfers policy ownership

	// The scheduler starts, and completes when the AOs are done
	iScheduler.Start();

	TInt err = tester->GetResult();

	Log(_L("The policy  was loaded with result %d"), err);

	if (!err)
		err = tester->ListPoliciesL();
	
	if (!err)
		tester->DeletePolicyL();

	err = tester->GetResult();

	Log(_L("Policy unload with result %d"), err);
	
	// Get rid of the Active objects
	CleanupStack::PopAndDestroy(tester);
	
	// Find the verdict from the callbacks
	__UHEAP_MARKEND;
	return err?EFail:EPass;
	}
