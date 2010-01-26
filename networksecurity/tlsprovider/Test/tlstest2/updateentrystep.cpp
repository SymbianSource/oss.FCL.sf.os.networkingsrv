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
 @file updateentrystep.h
 @internalTechnology	
*/

#include "updateentrystep.h"

#include <x509cert.h>

const TInt KDefaultResponseDelay = 3;

CUpdateEntryStep::CUpdateEntryStep()
	: iResponseDelay(KDefaultResponseDelay)
	{
	SetTestStepName(KUpdateEntryStep);
	}

TVerdict CUpdateEntryStep::doTestStepPreambleL()
	{
	// Initialise the tests
	InitializeL();

	// Parse the parameters required
	_LIT(KCertEntryApproved, "approved");
	_LIT(KResponseDelay, "delay");
	_LIT(KCancel, "cancelled");
			
	// Ignore return code from GetBoolFromConfig to keep default of EFalse.
	GetBoolFromConfig(ConfigSection(), KCancel, iCancel);
	
	// Ignore return status from GetIntFromConfig to keep default value.
	GetIntFromConfig(ConfigSection(), KResponseDelay, iResponseDelay);

	if (iCancel)
		{
		INFO_PRINTF3(_L("Certificate entry will be %S with a delay of %d seconds."), &KResponseDelay, iResponseDelay);
		}
	else if (!GetBoolFromConfig(ConfigSection(), KCertEntryApproved, iApproveEntry))
		{
		INFO_PRINTF1(_L("Could not read certificate approval status from INI, abort."));
			
		SetTestStepResult(EAbort);
		return EAbort;
		}
	else if (iApproveEntry)
		{
		INFO_PRINTF3(_L("Certificate entry will be %S with a delay of %d seconds."), &KResponseDelay, iResponseDelay);
		}
	else
		{
		INFO_PRINTF3(_L("Certificate entry will be %S with a delay of %d seconds."), &KResponseDelay, iResponseDelay);
		}
	
				
	return EPass;
	
	}
	
TVerdict CUpdateEntryStep::doTestStepL()
	{
	
	// don't continue if previous phases have aborted
	if (TestStepResult() != EPass)
		{
		return TestStepResult();
		}

	// Entry state should be ENewEntry at this point.
	TCacheEntryState state = Session().GetStateL();
	
	if (state != ENewEntry)
		{
		INFO_PRINTF4(_L("State of cache entry for certificate '%S' is %d, but should be %d (ENewEntry)."), SubjectLC(), state, ENewEntry);
		CleanupStack::PopAndDestroy(1); // subject	
		SetTestStepResult(EFail);
		return EFail;
		}
	else
		{
		INFO_PRINTF2(_L("State of cache entry for certificate '%S' is ENewEntry."), SubjectLC());
		CleanupStack::PopAndDestroy(1); // subject			
		}

	// Wait specifed number of seconds
	User::After(iResponseDelay * 1000000);

	if (iCancel)
		{
		Session().Cancel();
		// log the action
		INFO_PRINTF2(_L("Cancelled cert cache session opened for certificate '%S'."), SubjectLC());
		CleanupStack::PopAndDestroy(1); // subject
		}
	else
		{
		Session().SetStateL(iApproveEntry ? EEntryApproved : EEntryDenied);
		// log the action
		INFO_PRINTF2(_L("Updated cache entry for certificate '%S'."), SubjectLC());
		CleanupStack::PopAndDestroy(1); // subject
		}
	
	
	SetTestStepResult(EPass);
	return EPass;
	}
	
TVerdict CUpdateEntryStep::doTestStepPostambleL()
	{
	if (TestStepResult() == EPass)
		{
		INFO_PRINTF1(_L("Step suceeded."));
		return EPass;
		}
	else
		{
		INFO_PRINTF1(_L("Step failed."));
		return EFail;
		}
	}
