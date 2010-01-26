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

/**
 @file ts_ipsec_main2.cpp Implements main test code for IPsec
*/

#include <networking/log.h>
#include <networking/teststep.h>
#include "ts_ipsec_step.h"
#include "ts_ipsec_main2.h"
#include "ipsecapi.h"


//
// implementation of CIpsecTest2_1 - Create new policy object from file
//


CIpsecTest2_1::CIpsecTest2_1()
/**
 * Each test step initialises it's own name. Stores the name of this test case.
 * This is the name that is used by the script file
 */
	{
	_LIT(KTestStepName, "test2.1");
	iTestStepName.Copy(KTestStepName);
	}


enum TVerdict CIpsecTest2_1::doTestStepL()
/**
 * compare policy
 */
	{
	_LIT(KThisSection, "Section2.1");
	_LIT(KPolicyFile, "PolicyFile");
	_LIT(KPolicyName, "PolicyName");
	_LIT(KPolicyStatus, "PolicyStatus");
	_LIT(KActive, "ACTIVE");
	_LIT(KInactive, "INACTIVE");
	TBool ret;

	/**
	 * get parameters from ini file
	 */
	TPtrC file;
	ret = GetStringFromConfig(KThisSection, KPolicyFile, file);
	if(!ret)
		{
		Log(_L("Could not read policy file from config file, error %d"), ret);
		TESTL(EFalse);
		}

	TPtrC name;
	ret = GetStringFromConfig(KThisSection, KPolicyName, name);
	if(!ret)
		{
		Log(_L("Could not read policy name from config file, error %d"), ret);
		TESTL(EFalse);
		}

	TPtrC status;
	ret = GetStringFromConfig(KThisSection, KPolicyStatus, status);
	if(!ret)
		{
		Log(_L("Could not read policy status from config file, error %d"), ret);
		TESTL(EFalse);
		}

	/**
	 * fetch information from the system
	 */
	CIPSecAPI* api = 0;
	TRAP(ret, api = CIPSecAPI::NewL());
	Log(_L("CIPSecAPI::NewL returned %d"), ret);
	TESTEL(ret == KErrNone, ret);
	CleanupStack::PushL(api);

	TPolicyID id;
	id.iPolicyID.Copy(file);
	TIPSecPolicy pol;
	ret = api->Policy(id, pol);
	Log(_L("api->Policy() returned %d"), ret);
	TESTEL(ret == KErrNone, ret);

	/**
	 * compare information
	 */
	Log(_L("Comparing policy name    [arg=%S, sys=%S]\n"), &name, &pol.iPolicyName);
	TEST(pol.iPolicyName == name);

	Log(_L("Comparing policy ID      [arg=%S, sys=%S]\n"), &id.iPolicyID, &pol.iPolicyID.iPolicyID);
	TEST(pol.iPolicyID.iPolicyID == id.iPolicyID);

	Log(_L("Comparing status         [arg=%S, sys=%d]\n"), &status, pol.iStatus);
	if(status == KActive)
		TEST(pol.iStatus == EActive);
	else if(status == KInactive)
		TEST(pol.iStatus == EInactive);
	else
		{
		Log(_L("internal error!"));
		TEST(EFalse);
		}

	CleanupStack::PopAndDestroy(api);

	return iTestStepResult;
	}
