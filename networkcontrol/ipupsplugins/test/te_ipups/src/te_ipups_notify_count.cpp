// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
 @file
 @test
 @internalComponent - Internal Symbian test code 
*/

#include "te_ipups_ups_step.h"
#include "te_ipups_notify_count.h"
#include "upstestnotifierproperties.h"

CIpUpsNotifyCount::CIpUpsNotifyCount()
/**
 * Constructor
 */
	{
	SetTestStepName(KIpUpsNotifyCount);
	}

CIpUpsNotifyCount::~CIpUpsNotifyCount()
/**
 * Destructor
 */
	{ 	
	}

TVerdict CIpUpsNotifyCount::doTestStepPreambleL()
/**
 * @return - TVerdict code
 */
	{
	TSecurityPolicy nullPolicy(ECapability_None);
    TInt err;
    
	//Properties modified to be returned to test harness from test notifier
    err = RProperty::Define(KUidPSUPSTestNotifCategory, KUnStart, KUnCountKeyType, nullPolicy, nullPolicy);
    if (err != KErrAlreadyExists && err != KErrNone)
    	{
    	User::LeaveIfError(err);
    	}
    
    //Define a new Property which would be used to store the count to retreive later for comparison    
    err = RProperty::Define(KUidPSUPSTestNotifCategory, KUnStoredNotifyCount, KUnStoredCountKeyType, nullPolicy, nullPolicy);
    if (err != KErrAlreadyExists && err != KErrNone)
    	{
    	User::LeaveIfError(err);
    	}
    
 	SetTestStepResult(EPass);
	return TestStepResult();
	}

TVerdict CIpUpsNotifyCount::doTestStepL()
/**
 *	NotifyCount test step either stores the count retireved and store it in separate property defined or
 *	it compares the retireved notify count with the stored value + count increment expected relative to stored.
 * @return - TVerdict code
 */
	{	
	TBool storePromptTriggerCount = EFalse;
	
	GetBoolFromConfig(ConfigSection(),KIpUpsStorePromptTriggerCount, storePromptTriggerCount);
	
	if (storePromptTriggerCount)
		{
		TInt notifyCountReturned = 0;
		User::LeaveIfError(RProperty::Get(KUidPSUPSTestNotifCategory, KUnNotifyCount, notifyCountReturned));
		
		User::LeaveIfError(RProperty::Set(KUidPSUPSTestNotifCategory, KUnStoredNotifyCount, notifyCountReturned));
		
		INFO_PRINTF2(_L("NotifyCount Stored ( %d )"), notifyCountReturned);
		}
	else
		{
		TInt promptTriggerCount = 0;		
		
		GetIntFromConfig(ConfigSection(),KIpUpsPromptTriggerCount, promptTriggerCount);
	
		TInt notifyCountReturned = 0;
		User::LeaveIfError(RProperty::Get(KUidPSUPSTestNotifCategory, KUnNotifyCount, notifyCountReturned));
		
		TInt sotredPromptTriggerCount = 0;
		User::LeaveIfError(RProperty::Get(KUidPSUPSTestNotifCategory, KUnStart+2, sotredPromptTriggerCount));
	
		INFO_PRINTF3(_L("NotifyCount Expected ( %d ) <> NotifyCount Returned ( %d )"), promptTriggerCount+sotredPromptTriggerCount, notifyCountReturned);
	
		TEST ( (promptTriggerCount + sotredPromptTriggerCount) == notifyCountReturned);
		}	
	
	return TestStepResult();
	}
	
TVerdict CIpUpsNotifyCount::doTestStepPostambleL()
/**
 * @return - TVerdict code
 */
	{
	return TestStepResult();
	}
