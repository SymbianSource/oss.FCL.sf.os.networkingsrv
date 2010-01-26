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
 @file entrystatusstep.cpp
 @internalTechnology	
*/

#include "entrystatusstep.h"
#include <x509cert.h>

// Default delay before checking state
const TInt KStateCheckDelay = 1000000;

CEntryStatusStep::CEntryStatusStep()
	{
	SetTestStepName(KEntryStatus);
	}
	
TVerdict CEntryStatusStep::doTestStepPreambleL()
	{
	InitializeL();
	SetTestStepResult(EPass);

	_LIT(KRequestChangeNotify, "requestchangenotify");
	_LIT(KCertEntryState, "state");
	_LIT(KNewEntryString, "ENewEntry");
	_LIT(KEntryAwaitingApprovalString, "EEntryAwaitingApproval");
	_LIT(KEntryDeniedString, "EEntryDenied");
	_LIT(KEntryApprovedString, "EEntryApproved");
	_LIT(KCancelPoint, "cancelpoint");
	_LIT(KOpen, "Open");	
	_LIT(KGetState, "GetStateL");
	_LIT(KChangeNotify, "ChangeNotify");

	TPtrC cancelPoint;
	if (!GetStringFromConfig(ConfigSection(), KCancelPoint, cancelPoint))
		{
		iCancelPoint = ENoCancel;
		}
	else if (cancelPoint.CompareF(KOpen) == 0)
		{
		iCancelPoint = EAfterOpen;
		INFO_PRINTF2(_L("This test step will call Cancel() on the cert cache session after %S()"),&KOpen);
		}
	else if (cancelPoint.CompareF(KGetState) == 0)
		{
		iCancelPoint = EAfterGetState;
		INFO_PRINTF2(_L("This test step will call Cancel() on the cert cache session after %S()"),&KGetState);
		}
	else if (cancelPoint.CompareF(KChangeNotify) == 0)
		{
		iCancelPoint = EAfterChangeNotify;
		INFO_PRINTF2(_L("This test step will call Cancel() on the cert cache session after %S()"),&KChangeNotify);
		}
	
	// Check if this step should wait for change notification.
	if (!GetBoolFromConfig(ConfigSection(), KRequestChangeNotify, iRequestChangeNotify))
		{
		iRequestChangeNotify = EFalse;
		}
	else if (iRequestChangeNotify)
		{
		if (iCancelPoint == ENoCancel)
			{
			INFO_PRINTF1(_L("This test step will wait for change notification."));
			}
		else if (iCancelPoint != EAfterChangeNotify)
			{
			INFO_PRINTF1(_L("Invalid test config, requesting notification but cancelling earlier."));
			SetTestStepResult(EAbort);
			return EAbort;
			}

		_LIT(KRequirePendingApproval, "requirependingapproval");
		if (!GetBoolFromConfig(ConfigSection(), KRequirePendingApproval, iRequirePendingApproval))
			{
			iRequirePendingApproval = ETrue;
			}
		if (iRequirePendingApproval)
			{
			INFO_PRINTF1(_L("This step will fail if the state is not initially EEntryAwaitingApproval."));
			}
		else
			{
			INFO_PRINTF1(_L("Notification will be requested even if the state is not initially EEntryAwaitingApproval."));
			}
		}

	TPtrC expectedState;

	if (!GetStringFromConfig(ConfigSection(), KCertEntryState, expectedState))
		{
		INFO_PRINTF1(_L("Could not read expected certificate approval state from INI, abort."));
		SetTestStepResult(EAbort);
		}
	else
		{
		if (expectedState.CompareF(KNewEntryString) == 0)
			{
			iExpectedState = ENewEntry;
			INFO_PRINTF2(_L("Certificate state is expected to be %S."),&KNewEntryString);
		 	}
		else if (expectedState.CompareF(KEntryAwaitingApprovalString) == 0)
			{
			iExpectedState = EEntryAwaitingApproval;
			INFO_PRINTF2(_L("Certificate state is expected to be %S."),&KEntryAwaitingApprovalString);
			}
		else if (expectedState.CompareF(KEntryApprovedString) == 0)
			{
			iExpectedState = EEntryApproved;
			INFO_PRINTF2(_L("Certificate state is expected to be %S."),&KEntryApprovedString);
			}
		else if (expectedState.CompareF(KEntryDeniedString) == 0)
			{
			iExpectedState = EEntryDenied;
			INFO_PRINTF2(_L("Certificate state is expected to be %S."),&KEntryDeniedString);
			}
		else
			{
			INFO_PRINTF1(_L("Invalid expected certificate state, abort."));
			SetTestStepResult(EAbort);
			}
		}
	return TestStepResult();
	}
	
TVerdict CEntryStatusStep::doTestStepL()
	{
	// don't continue if previous phases have aborted
	if (TestStepResult() != EPass)
		{
		return TestStepResult();
		}

	// Delay briefly to ensure that any update entry steps in concurrent tests can
	// check first (which sets the state to EAwaitingApproval.)
	User::After(KStateCheckDelay);

	// Cancel if set to do so before checking state.
	if (iCancelPoint == EAfterOpen)
		{
		INFO_PRINTF1(_L("Cancelling..."));
		Session().Cancel();
		}

	iState = Session().GetStateL();
	
	// log the action
	INFO_PRINTF3(_L("State of cache entry for certificate '%S' is %d."), SubjectLC(), iState);
	CleanupStack::PopAndDestroy(1); // subject

	if (iCancelPoint == EAfterGetState)
		{
		INFO_PRINTF1(_L("Cancelling..."));
		Session().Cancel();
		iState = Session().GetStateL();
		INFO_PRINTF3(_L("State of cache entry for certificate '%S' is %d."), SubjectLC(), iState);
		CleanupStack::PopAndDestroy(1); // subject
		}
	else if (iRequestChangeNotify)
		{
		if (iState == EEntryAwaitingApproval || !iRequirePendingApproval)
			{
			TRequestStatus status;
			Session().RequestNotify(status);
			if (iCancelPoint == EAfterChangeNotify)
				{
				INFO_PRINTF1(_L("Cancelling..."));
				Session().Cancel();
				}

			User::WaitForRequest(status);

			User::LeaveIfError(status.Int());
			iState = Session().GetStateL();

			// log the action
			INFO_PRINTF3(_L("Got cache change notify for certificate '%S', state = %d."), SubjectLC(), iState);
			CleanupStack::PopAndDestroy(1); // certificate status
			}
		else
			{
			// log the action
			INFO_PRINTF2(_L("Cannot wait for change notify, entry state is not %d (EEntryAwaitingApproval.)"), EEntryAwaitingApproval);
			SetTestStepResult(EFail)		;
			}
		}
	
	return TestStepResult();
	
	}
	
TVerdict CEntryStatusStep::doTestStepPostambleL()
	{
	
	if (TestStepResult() == EPass)
		{
		INFO_PRINTF1(_L("Step suceeded. Checking result..."));
		}
	else
		{
		INFO_PRINTF1(_L("Step failed."));
		return TestStepResult();
		}
		
	if (iExpectedState == iState)
		{
		INFO_PRINTF1(_L("Expected state %d matches actual state."));
		SetTestStepResult(EPass);
		return EPass;
		}
	else
		{
		INFO_PRINTF3(_L("Expected state %d does not match actual state %d!"), iExpectedState, iState);
		SetTestStepResult(EFail);
		return EFail;
		}
	
	}
	

	

	
