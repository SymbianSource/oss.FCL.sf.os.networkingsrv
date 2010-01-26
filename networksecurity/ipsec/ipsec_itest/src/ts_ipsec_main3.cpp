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
 @file ts_ipsec_main3.cpp Implements main test code for IPsec
*/

#include <networking/log.h>
#include <networking/teststep.h>
#include "ts_ipsec_step.h"
#include "ts_ipsec_main3.h"
#include "ipsecapi.h"

#if defined(SYMBIAN_CRYPTO)
#include <cryptostrength.h>
#else
#include <cryptalg.h>
#endif


_LIT(KPolicyName, "PolicyName");
_LIT(KPolicyFile, "PolicyFile");
_LIT(KPolicyVersion, "PolicyVersion");
_LIT(KPolicyDesc, "PolicyDesc");
_LIT(KContactInfo, "ContactInfo");
_LIT(KIssuerName, "IssuerName");

//
// implementation of CIpsecTest3_1
//


CIpsecTest3_1::CIpsecTest3_1()
/**
 * Each test step initialises it's own name. Stores the name of this test case.
 * This is the name that is used by the script file
 */
	{
	_LIT(KTestStepName, "test3.1");
	iTestStepName.Copy(KTestStepName);
	}


enum TVerdict CIpsecTest3_1::doTestStepL()
/**
 * Get policy count
 */
	{
	_LIT(KThisSection, "Section3.1");
	_LIT(KPolicyCount, "PolicyCount");

	Log(_L("Test - Get policy count"));

	CIPSecAPI* api = 0;
	TRAPD(result, api = CIPSecAPI::NewL());
	Log(_L("CIPSecAPI::NewL returned %d"), result);
	TESTEL(result==KErrNone, result);
	CleanupStack::PushL(api);
	
	TInt policy_count;
	const TBool ret = GetIntFromConfig(KThisSection, KPolicyCount, policy_count);
	if(!ret)
		{
		Log(_L("Could not read integer from config file"));
		TESTL(EFalse);
		}

	TInt sys = api->PolicyCount();
	Log(_L("Comparing policy counts [arg=%d, sys=%d]"), policy_count, sys);
	TEST(policy_count == sys);

	CleanupStack::PopAndDestroy(api);

	// test steps return a result
	return iTestStepResult;
	}


//
// implementation of CIpsecTest3_3 - Get policy info
//


CIpsecTest3_3::CIpsecTest3_3()
/**
 * Each test step initialises it's own name. Stores the name of this test case.
 * This is the name that is used by the script file
 */
	{
	_LIT(KTestStepName, "test3.3");
	iTestStepName.Copy(KTestStepName);
	}


enum TVerdict CIpsecTest3_3::doTestStepL()
/**
 * Get policy details
 */
	{
	TBool ret;
	_LIT(KThisSection, "Section3.3");

	/**
	 * read the parameters from the .ini file
	 */
	TPtrC name;
	ret = GetStringFromConfig(KThisSection, KPolicyFile, name);
	if(!ret)
		{
		Log(_L("Could not get PolicyFile from config file"));
		TESTL(EFalse);
		}

	TPtrC arg_name;
	ret = GetStringFromConfig(KThisSection, KPolicyName, arg_name);
	if(!ret)
		{
		Log(_L("Could not get PolicyName from config file"));
		TESTL(EFalse);
		}

	TPtrC arg_version;
	ret = GetStringFromConfig(KThisSection, KPolicyVersion, arg_version);
	if(!ret)
		{
		Log(_L("Could not get PolicyVersion from config file"));
		TESTL(EFalse);
		}

	TPtrC arg_desc;
	ret = GetStringFromConfig(KThisSection, KPolicyDesc, arg_desc);
	if(!ret)
		{
		Log(_L("Could not get PolicyDesc from config file"));
		TESTL(EFalse);
		}

	TPtrC arg_issuer_name;
	ret = GetStringFromConfig(KThisSection, KIssuerName, arg_issuer_name);
	if(!ret)
		{
		Log(_L("Could not get IssuerName from config file"));
		TESTL(EFalse);
		}

	TPtrC arg_contact_info;
	ret = GetStringFromConfig(KThisSection, KContactInfo, arg_contact_info);
	if(!ret)
		{
		Log(_L("Could not get ContactInfo from config file"));
		TESTL(EFalse);
		}

	/**
	 * get details from ipsec manager
	 */
	CIPSecAPI* api = 0;
	TRAPD(res, api = CIPSecAPI::NewL());
	Log(_L("CIPSecAPI::NewL returned %d"), res);
	TESTEL(res==KErrNone, res);
	CleanupStack::PushL(api);

	TIPSecPolicyDetails sys_info;
	TPolicyID id;
	id.iPolicyID.Copy(name);
	res = api->PolicyDetails(id, sys_info);
	Log(_L("api->PolicyDetails returned %d"), res);
	TESTEL(res == KErrNone, res);
	
	/**
	 * and now compare them
	 */
	Log(_L("Comparing policy name    [arg=%S, sys=%S]\n"), &arg_name, &sys_info.iPolicyName);
	TEST(arg_name == sys_info.iPolicyName);
	Log(_L("Comparing policy version [arg=%S, sys=%S]\n"), &arg_version, &sys_info.iPolicyVersion);
	TEST(arg_version == sys_info.iPolicyVersion);
	Log(_L("Comparing policy desc    [arg=%S, sys=%S]\n"), &arg_desc, &sys_info.iPolicyDescription);
	TEST(arg_desc == sys_info.iPolicyDescription);
	Log(_L("Comparing issuer name    [arg=%S, sys=%S]\n"), &arg_issuer_name, &sys_info.iIssuerName);
	TEST(arg_issuer_name == sys_info.iIssuerName);
	Log(_L("Comparing contact info   [arg=%S, sys=%S]\n"), &arg_contact_info, &sys_info.iContactInfo);
	TEST(arg_contact_info == sys_info.iContactInfo);

	CleanupStack::PopAndDestroy(api);

	// test steps return a result
	return iTestStepResult;
	}


//
// implementation of CIpsecTest3_4
//


CIpsecTest3_4::CIpsecTest3_4()
/**
 * Each test step initialises it's own name. Stores the name of this test case.
 * This is the name that is used by the script file
 */
	{
	_LIT(KTestStepName, "test3.4");
	iTestStepName.Copy(KTestStepName);
	}


enum TVerdict CIpsecTest3_4::doTestStepL()
/**
 * Delete policy
 */
	{
	_LIT(KThisSection, "Section3.4");

	Log(_L("Test - Delete policy"));

	CIPSecAPI* api = 0;
	TRAPD(ret, api = CIPSecAPI::NewL());
	Log(_L("CIPSecAPI::NewL returned %d"), ret);
	TESTEL(ret == KErrNone, ret);
	CleanupStack::PushL(api);
	
	/**
	 * read the parameters from the .ini file
	 */
	TPtrC name;
	const TBool b = GetStringFromConfig(KThisSection, KPolicyName, name);
	if(!b)
		{
		Log(_L("Could not get PolicyName from config file"));
		TESTL(EFalse);
		}

	TPolicyID id;
	id.iPolicyID.Copy(name);
	Log(_L("Deleting policy %S"), &(id.iPolicyID));
	ret = api->DeletePolicy(id);
	Log(_L("api->DeletePolicy returned %d"), ret);
	TESTEL(ret==KErrNone, ret);

	CleanupStack::PopAndDestroy(api);

	// test steps return a result
	return iTestStepResult;
	}


//
// implementation of CIpsecTest3_5 - activate policy
//


CIpsecTest3_5::CIpsecTest3_5()
/**
 * Each test step initialises it's own name. Stores the name of this test case.
 * This is the name that is used by the script file
 */
	{
	_LIT(KTestStepName, "test3.5");
	iTestStepName.Copy(KTestStepName);
	}


enum TVerdict CIpsecTest3_5::doTestStepL()
/**
 * activate IPSec with given policy
 */
	{
	_LIT(KThisSection, "Section3.5");

	Log(_L("Test - Activate ipsec"));

	TPtrC ptrResult;
	const TBool b = GetStringFromConfig(KThisSection, KPolicyName, ptrResult);
	if(!b)
		{
		Log(_L("Could not read PolicyName from config file"));
		TESTL(EFalse);
		}

	Log(_L("PolicyFile = %S"), &ptrResult);

	/**
	 * checking crypto libraries
	 */
	const TCrypto::TStrength strength = TCrypto::Strength();
	Log(_L("Crypto strength is %d"), strength);

	switch (strength)
		{
#if defined(SYMBIAN_CRYPTO)
	case TCrypto::EWeak:
#else
	case TCrypto::ECrypto_40:
	case TCrypto::ECrypto_56:
	case TCrypto::ECrypto_64:
	case TCrypto::ECrypto_128:
#endif
		Log(_L("WARNING! your crypto library is too weak !!!"));
		Log(_L("         need at least 256 bits strength."));
		Log(_L("         fetch cryptalg source from mainline"));
		Log(_L("         and change '56' to '256' in bld.inf file"));
		Log(_L("         and then do 'abld export'"));
		break;
			
	default:
		break;
		}

	CIPSecAPI* api = 0;
	TRAPD(ret, api = CIPSecAPI::NewL());
	Log(_L("CIPSecAPI::NewL returned %d"), ret);
	TESTEL(ret == KErrNone, ret);
	CleanupStack::PushL(api);

	TPolicyID policyID;
	policyID.iPolicyID.Copy(ptrResult);
	ret = api->ActivateIPSec(policyID);
	Log(_L("CIPSecAPI::ActivateIPSec returned %d"), ret);
	TESTEL(ret == KErrNone, ret);

	const TBool active = api->IsIPSecActive();
	Log(_L("IsIPSecActive returned %d"), TInt(active));
	TESTL(active);

	CleanupStack::PopAndDestroy(api);

	// test steps return a result
	return iTestStepResult;
	}


//
// implementation of CIpsecTest3_6
//


CIpsecTest3_6::CIpsecTest3_6()
/**
 * Each test step initialises it's own name. Stores the name of this test case.
 * This is the name that is used by the script file
 */
	{
	_LIT(KTestStepName, "test3.6");
	iTestStepName.Copy(KTestStepName);
	}


enum TVerdict CIpsecTest3_6::doTestStepL()
/**
 * Deactivate IPSec
 */
	{
	Log(_L("Test - Deactivate IPSec"));

	CIPSecAPI* api = 0;
	TRAPD(ret, api = CIPSecAPI::NewL());
	Log(_L("CIPSecAPI::NewL returned %d"), ret);
	TESTEL(ret == KErrNone, ret);
	CleanupStack::PushL(api);

	ret = api->DeactivateIpsec();
	Log(_L("DeactivateIpsec returned %d"), ret);
	TESTEL(ret == KErrNone, ret);

	TInt retries = 10;
	TBool active = ETrue;
	while(retries-- && active)
		{
		Log(_L("deactivate try %d"), retries);
		active = api->IsIPSecActive();
		User::After(1000000); // 1 second delay
		}
	Log(_L("Final value of active is %d"), active);
	TESTL(!active);

	CleanupStack::PopAndDestroy(api);

	// test steps return a result
	return iTestStepResult;
	}
