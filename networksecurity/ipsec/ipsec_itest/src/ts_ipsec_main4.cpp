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
 @file ts_ipsec_main4.cpp Implements main test code for IPsec
*/

#include <networking/log.h>
#include <networking/teststep.h>
#include "ts_ipsec_step.h"
#include "ts_ipsec_main4.h"
#include "ipsecapi.h"

//
// implementation of CIpsecTest4_1
//


CIpsecTest4_1::CIpsecTest4_1()
/**
 * Each test step initialises it's own name. Stores the name of this test case.
 * This is the name that is used by the script file
 */
	{
	_LIT(KTestStepName, "test4.1");
	iTestStepName.Copy(KTestStepName);
	}


enum TVerdict CIpsecTest4_1::doTestStepL()
/**
 * Each test step must supply a implementation for doTestStepL
 *
 <pre>

	   my @testcases = ("3.1", "3.2", "3.3", ...);

	   foreach $test (@testcases) {
	     my $count = 0;
	     my $ret = KErrNoMemory;
	     while($ret == KErrNoMemory) {
		   __UHEAP_FAILNEXT(count);
		   __UHEAP_MARK;

		   # run test cases

		   __UHEAP_MARKEND;
		   $count++;
		 }	
	   }

 </pre>
 *
 */
	{
	TInt count = 0;
    TInt ret = KErrNoMemory;

	/**
	 * OOM test loop
	 */
	while(ret==KErrNoMemory)
        {
        __UHEAP_FAILNEXT(count);
        __UHEAP_MARK;
		TRAP(ret, TestOomPolicyListL());
        __UHEAP_MARKEND;
        count++;
        }

	Log(_L("OOM after %d allocations"), count);
	
	// test steps return a result
	return iTestStepResult;
	}


void CIpsecTest4_1::TestOomPolicyListL()
/**
 * OOM helper function - implements normal usage of PolicyListL method
 */
	{
	TInt ret;
	TIPSecPolicy pol;

	CIPSecAPI* api = CIPSecAPI::NewL();
	CleanupStack::PushL(api);

	CArrayFixFlat<TIPSecPolicy>* list = new (ELeave) CArrayFixFlat<TIPSecPolicy>(5);
	CleanupStack::PushL(list);

	list->AppendL(pol);
	ret = api->PolicyListL(*list);
	(void)ret; // to remove compiler warning

	TEST(api!=NULL);

	CleanupStack::PopAndDestroy(list);
	CleanupStack::PopAndDestroy(api);
	}
