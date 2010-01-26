// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
	_LIT(KCancelMessageFmt, "This test step will call Cancel() on the cert cache session after %S()");
	if (!GetStringFromConfig(ConfigSection(), KCancelPoint, cancelPoint))
		{
		iCancelPoint = ENoCancel;
		}
	else if (cancelPoint.CompareF(KOpen) == 0)
		{
		iCancelPoint = EAfterOpen;
		Logger().WriteFormat(KCancelMessageFmt, &KOpen);
		}
	else if (cancelPoint.CompareF(KGetState) == 0)
		{
		iCancelPoint = EAfterGetState;
		Logger().WriteFormat(KCancelMessageFmt, &KGetState);
		}
	else if (cancelPoint.CompareF(KChangeNotify) == 0)
		{
		iCancelPoint = EAfterChangeNotify;
		Logger().WriteFormat(KCancelMessageFmt, &KChangeNotify);
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
			_LIT(KMessage, "This test step will wait for change notification.");
			Logger().Write(KMessage);
			}
		else if (iCancelPoint != EAfterChangeNotify)
			{
			_LIT(KErrorMessage, "Invalid test config, requesting notification but cancelling earlier.");
			Logger().Write(KErrorMessage);
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
			_LIT(KMessage2, "This step will fail if the state is not initially EEntryAwaitingApproval.");
			Logger().Write(KMessage2);		
			}
		else
			{
			_LIT(KMessage2, "Notification will be requested even if the state is not initially EEntryAwaitingApproval.");
			Logger().Write(KMessage2);		
			}
		}

	TPtrC expectedState;

	if (!GetStringFromConfig(ConfigSection(), KCertEntryState, expectedState))
		{
		_LIT(KMessage, "Could not read expected certificate approval state from INI, abort.");
		Logger().Write(KMessage);
		
		SetTestStepResult(EAbort);
		}
	else
		{
		_LIT(KMessageFmt, "Certificate state is expected to be %S.");
		if (expectedState.CompareF(KNewEntryString) == 0)
			{
			iExpectedState = ENewEntry;
			Logger().WriteFormat(KMessageFmt, &KNewEntryString);
			}
		else if (expectedState.CompareF(KEntryAwaitingApprovalString) == 0)
			{
			iExpectedState = EEntryAwaitingApproval;
			Logger().WriteFormat(KMessageFmt, &KEntryAwaitingApprovalString);
			}
		else if (expectedState.CompareF(KEntryApprovedString) == 0)
			{
			iExpectedState = EEntryApproved;
			Logger().WriteFormat(KMessageFmt, &KEntryApprovedString);
			}
		else if (expectedState.CompareF(KEntryDeniedString) == 0)
			{
			iExpectedState = EEntryDenied;
			Logger().WriteFormat(KMessageFmt, &KEntryDeniedString);
			}
		else
			{
			_LIT(KMessage, "Invalid expected certificate state, abort.");
			Logger().Write(KMessage);
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

	_LIT(KCancelMessage, "Cancelling...");

	// Cancel if set to do so before checking state.
	if (iCancelPoint == EAfterOpen)
		{
		Logger().Write(KCancelMessage);
		Session().Cancel();
		}

	iState = Session().GetStateL();
	
	// log the action
	_LIT(KMessageFmt, "State of cache entry for certificate '%S' is %d.");
	Logger().WriteFormat(KMessageFmt, SubjectLC(), iState);
	CleanupStack::PopAndDestroy(1); // subject

	if (iCancelPoint == EAfterGetState)
		{
		Logger().Write(KCancelMessage);
		Session().Cancel();
		iState = Session().GetStateL();
		Logger().WriteFormat(KMessageFmt, SubjectLC(), iState);
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
				Logger().Write(KCancelMessage);
				Session().Cancel();
				}

			User::WaitForRequest(status);

			User::LeaveIfError(status.Int());

			iState = Session().GetStateL();

			// log the action
			_LIT(KMessageFormat, "Got cache change notify for certificate '%S', state = %d.");
			Logger().WriteFormat(KMessageFormat, SubjectLC(), iState);
			CleanupStack::PopAndDestroy(1); // certificate status
			}
		else
			{
			// log the action
			_LIT(KMessageFormat, "Cannot wait for change notify, entry state is not %d (EEntryAwaitingApproval.)");
			Logger().WriteFormat(KMessageFormat, EEntryAwaitingApproval);
			SetTestStepResult(EFail)		;
			}
		}
	
	return TestStepResult();
	
	}
	
TVerdict CEntryStatusStep::doTestStepPostambleL()
	{
	
	if (TestStepResult() == EPass)
		{
		_LIT(KMessage, "Step suceeded. Checking result...");
		Logger().Write(KMessage);
		}
	else
		{
		_LIT(KMessage, "Step failed.");
		Logger().Write(KMessage);
		return TestStepResult();
		}
		
	if (iExpectedState == iState)
		{
		_LIT(KMessageFmt, "Expected state %d matches actual state.");
		Logger().WriteFormat(KMessageFmt, iState);
		SetTestStepResult(EPass);
		return EPass;
		}
	else
		{
		_LIT(KMessageFmt, "Expected state %d does not match actual state %d!");
		Logger().WriteFormat(KMessageFmt, iExpectedState, iState);
		SetTestStepResult(EFail);
		return EFail;
		}
	
	}
	

	

	
