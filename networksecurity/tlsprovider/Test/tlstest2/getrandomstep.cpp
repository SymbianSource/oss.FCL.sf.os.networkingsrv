// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
 @file getrandomstep.cpp
 @internalTechnology
*/
#include "getrandomstep.h"
#include <tlsprovinterface.h>
#include <tlstypedef.h>

_LIT(KEpochDate, "19700000:");

CGetRandomStep::CGetRandomStep()
	{
	SetTestStepName(KGetRandomStep);
	}
	
TVerdict CGetRandomStep::doTestStepPreambleL()
	{
	ConstructL();
	return EPass;
	}

TVerdict CGetRandomStep::doTestStepL()
	{
	// Get three buffers of random...
	TBuf8<KTLSServerClientRandomLen> random1;
	TBuf8<KTLSServerClientRandomLen> random2;
	TBuf8<KTLSServerClientRandomLen> random3;
	
	
	INFO_PRINTF1(_L("Invoking TLS Provider - Generate Random"));
	
	Provider()->GenerateRandom(random1);
	Provider()->GenerateRandom(random2);
	Provider()->GenerateRandom(random3);
	
	// check all the buffers are of sane length
	if (random1.Length() != KTLSServerClientRandomLen ||
		random2.Length() != KTLSServerClientRandomLen ||
		random3.Length() != KTLSServerClientRandomLen)
		{
		INFO_PRINTF1(_L("Failed! Not all random buffers are of the required length!"));
		SetTestStepResult(EFail);
		return TestStepResult();
		}
	
	// check the timestamps on the data are all roughly sane....
	
	TTime now;
	now.UniversalTime();
	TTime epoch(KEpochDate);
	
	TUint stamp1 = GetTimestamp(random1);
	TUint stamp2 = GetTimestamp(random2);
	TUint stamp3 = GetTimestamp(random3); 
	
	if (stamp1 > stamp2 || stamp2 > stamp3)
		{
		INFO_PRINTF1(_L("Failed! Timestamps suggest an insane clock!"));
		// uh oh, timestamps are going backwards...
		SetTestStepResult(EFail);
		return TestStepResult();
		}
		
	// check all the time stamps are within a sane interval from "now"
	TTime time1 = epoch + TTimeIntervalSeconds(stamp1);
	TTime time2 = epoch + TTimeIntervalSeconds(stamp2);
	TTime time3 = epoch + TTimeIntervalSeconds(stamp3);
	
	// read the interval from the config file...
	TInt interval(0);
	if (!GetIntFromConfig(ConfigSection(), KMaxTimestampInterval, interval))
		{
		User::Leave(KErrNotFound);
		}
	
	TTimeIntervalSeconds interval1;
	TInt err1 = now.SecondsFrom(time1, interval1);
	TTimeIntervalSeconds interval2;
	TInt err2 = now.SecondsFrom(time2, interval2);
	TTimeIntervalSeconds interval3;
	TInt err3 = now.SecondsFrom(time3, interval3);
	
	if (err1 != KErrNone || err2 != KErrNone || err3 != KErrNone)
		{
		INFO_PRINTF1(_L("Failed! Could not get time intervals from 'now'!"));
		SetTestStepResult(EFail);
		return TestStepResult();
		}
		
	if (interval1.Int() > interval ||
		interval2.Int() > interval ||
		interval3.Int() > interval)
		{
		// Looks like the timestamp algorithm is bad.
		INFO_PRINTF1(_L("Failed! Clock appears to be running far too fast!"));
		SetTestStepResult(EFail);
		return TestStepResult();
		}
		
	// now, check the random portion of the data... 
	// just make sure it isn't all the same string...
	
	TPtrC8 ptr1 = random1.Mid(4);
	TPtrC8 ptr2 = random2.Mid(4);
	TPtrC8 ptr3 = random3.Mid(4);
	
	if (ptr1 == ptr2 || ptr2 == ptr3 || ptr3 == ptr1)
		{
		INFO_PRINTF1(_L("Failed! Random data isn't actually random!"));
		SetTestStepResult(EFail);
		return TestStepResult();
		}
		
	INFO_PRINTF1(_L("Test step passed."));
	SetTestStepResult(EPass);
	return TestStepResult();
	}
	
TUint CGetRandomStep::GetTimestamp(const TDesC8& aRandom)
	{
	TUint ret = aRandom[0];
	for (TInt i = 1; i < 4; ++i)
		{
		ret <<= 8;
		ret += aRandom[i];
		}
	return ret;
	}
