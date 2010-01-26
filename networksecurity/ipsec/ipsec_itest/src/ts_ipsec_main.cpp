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
 @file ts_ipsec_main.cpp Implements main test code for IPsec
*/

#include <networking/log.h>
#include <networking/teststep.h>
#include "ts_ipsec_step.h"
#include "ts_ipsec_main.h"
#include "ipsecapi.h"

//
// implementation of CIpsecTest1_1
//


CIpsecTest1_1::CIpsecTest1_1()
/**
 *
 */
	{
	_LIT(KTestStepName, "test1.1");
	iTestStepName.Copy(KTestStepName);
	}


enum TVerdict CIpsecTest1_1::doTestStepL()
/**
 * test - import policies
 */
	{
	_LIT(KThisSection, "Section1.1");
	
	Log(_L("Test - import policy"));

	/**
	 * make sure the act_prof.lst file is deleted
	 */
	
	RFs fs;
	TInt ret = fs.Connect();
	Log(_L("fs.Connect returned %d"), ret);
	TESTEL(ret == KErrNone, ret);
	CleanupClosePushL(fs);

	/**
	 * read parameters
	 */
	TPtrC ptrResult;
	_LIT(KPolicyDir,  "PolicyDir");

	const TBool b = GetStringFromConfig(KThisSection, KPolicyDir, ptrResult);
	if(!b)
		{
		Log(_L("Could not read policy file from config"));
		TESTL(EFalse);
		}

	Log(_L("PolicyDir = %S"), &ptrResult);
	
	CIPSecAPI* api = 0;
	TRAP(ret, api = CIPSecAPI::NewL());
	Log(_L("CIPSecAPI::NewL returned %d"), ret);
	TESTEL(ret == KErrNone, ret);
	CleanupStack::PushL(api);

	ret = api->ImportPolicy(ptrResult);
	Log(_L("ImportPolicy returned %d"), ret);
	TESTEL(ret == KErrNone, ret);

	/**
	 * cleanup
	 */
	CleanupStack::PopAndDestroy(api);
	CleanupStack::PopAndDestroy(&fs);

	return iTestStepResult;
	}
