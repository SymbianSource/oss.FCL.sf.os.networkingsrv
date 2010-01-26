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
	_LIT(KCertEntryDenied, "denied");
	_LIT(KMessageFmt, "Certificate entry will be %S with a delay of %d seconds.");
	
	// Ignore return code from GetBoolFromConfig to keep default of EFalse.
	GetBoolFromConfig(ConfigSection(), KCancel, iCancel);
	
	// Ignore return status from GetIntFromConfig to keep default value.
	GetIntFromConfig(ConfigSection(), KResponseDelay, iResponseDelay);

	if (iCancel)
		{
		Logger().WriteFormat(KMessageFmt, &KResponseDelay, iResponseDelay);		
		}
	else if (!GetBoolFromConfig(ConfigSection(), KCertEntryApproved, iApproveEntry))
		{
		_LIT(KErrorMessage, "Could not read certificate approval status from INI, abort.");
		Logger().Write(KErrorMessage);
		
		SetTestStepResult(EAbort);
		return EAbort;
		}
	else if (iApproveEntry)
		{
		Logger().WriteFormat(KMessageFmt, &KCertEntryApproved, iResponseDelay);
		}
	else
		{
		Logger().WriteFormat(KMessageFmt, &KCertEntryDenied, iResponseDelay);
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
		_LIT(KMessageFormat, "State of cache entry for certificate '%S' is %d, but should be %d (ENewEntry).");
		Logger().WriteFormat(KMessageFormat, SubjectLC(), state, ENewEntry);
		CleanupStack::PopAndDestroy(1); // subject	
		SetTestStepResult(EFail);
		return EFail;
		}
	else
		{
		_LIT(KMessageFormat, "State of cache entry for certificate '%S' is ENewEntry.");
		Logger().WriteFormat(KMessageFormat, SubjectLC());
		CleanupStack::PopAndDestroy(1); // subject			
		}

	// Wait specifed number of seconds
	User::After(iResponseDelay * 1000000);

	if (iCancel)
		{
		Session().Cancel();
		// log the action
		_LIT(KMessageFormat, "Cancelled cert cache session opened for certificate '%S'.");
		Logger().WriteFormat(KMessageFormat, SubjectLC());
		CleanupStack::PopAndDestroy(1); // subject
		}
	else
		{
		Session().SetStateL(iApproveEntry ? EEntryApproved : EEntryDenied);
		// log the action
		_LIT(KMessageFormat, "Updated cache entry for certificate '%S'.");
		Logger().WriteFormat(KMessageFormat, SubjectLC());
		CleanupStack::PopAndDestroy(1); // subject
		}
	
	
	SetTestStepResult(EPass);
	return EPass;
	}
	
TVerdict CUpdateEntryStep::doTestStepPostambleL()
	{
	if (TestStepResult() == EPass)
		{
		_LIT(KMessage, "Step suceeded.");
		Logger().Write(KMessage);
		return EPass;
		}
	else
		{
		_LIT(KMessage, "Step failed.");
		Logger().Write(KMessage);
		return EFail;
		}
	}
