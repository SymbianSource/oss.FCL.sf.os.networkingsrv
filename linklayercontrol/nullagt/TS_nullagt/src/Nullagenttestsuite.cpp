// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "TestSuiteNullAgent.h"
#include "NullAgentTestSteps.h"

#include "c32comm.h"


EXPORT_C CTestSuiteNullAgt* CreateTestSuite(void)
{
	return new (ELeave) CTestSuiteNullAgt();
}

void CTestSuiteNullAgt::InitialiseL(void)
{
	// start c32 process
 	// When bootstrapping C32 we have to avoid the PhBkSyncServer being started, since
 	// it needs a different CommDB
 	_LIT(KPhbkSyncCMI, "phbsync.cmi");
    TInt ret = StartC32WithCMISuppressions(KPhbkSyncCMI);
	if ( KErrNone != ret && KErrAlreadyExists != ret )
		{
		User::Leave( ret );
		}

	// add each step to the suite
	AddTestStepL( new(ELeave) CNullAgentPreCopy());
	AddTestStepL( new(ELeave) CTestStepNullAgtSimpleConnection(_L("Connection")) );
	AddTestStepL( new(ELeave) CTestStepNullAgtLoopbackTest(_L("Loopback")) );
	AddTestStepL( new(ELeave) CTestStepNullAgtReconnect(_L("Reconnect")) );
	AddTestStepL( new(ELeave) CTestStepNullAgtNotifications(_L("Notification")) );
	AddTestStepL( new(ELeave) CTestStepNullAgtOverrides(_L("Overrides")) );
	AddTestStepL( new(ELeave) CTestStepNullAgtMultipleConnections(_L("Multiple")) );
	AddTestStepL( new(ELeave) CTestStepNullAgtCommDbIf(_L("CommDbIf")) );
	AddTestStepL( new(ELeave) CTestStepNullAgtConnectionCancel(_L("Cancel")) );
	AddTestStepL( new(ELeave) CNullAgentPostDelete());
}

TPtrC CTestSuiteNullAgt::GetVersion(void)
{
	_LIT(KVersion, "1.0");
	return KVersion();
}
