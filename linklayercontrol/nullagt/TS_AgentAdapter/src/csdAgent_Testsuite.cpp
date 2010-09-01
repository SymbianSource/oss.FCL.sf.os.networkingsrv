// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Contain the Test Steps for Connection/Connection Failure tests for CSD agent
// 
//
#include <CsdAgentTestSuite.h>
#include <CsdAgentTestSteps.h>

#include "c32comm.h"

EXPORT_C CTestSuiteCsdAgt* CreateTestSuite(void)
{
	return new (ELeave) CTestSuiteCsdAgt();
}

void CTestSuiteCsdAgt::InitialiseL()
	{
 	_LIT(KPhbkSyncCMI, "phbsync.cmi");
    TInt ret = StartC32WithCMISuppressions(KPhbkSyncCMI);
    
    if ( KErrNone != ret && KErrAlreadyExists != ret )
		{
		User::Leave( ret );
		}

    AddTestStepL(new (ELeave) CTestStepCsdAgentConnection(_L("Connection")) );
    AddTestStepL(new (ELeave) CTestStepCsdAgtConnectionFailure(_L("ConnectionStartFailure")) );
	}
	
TPtrC CTestSuiteCsdAgt::GetVersion(void)
{
    _LIT(KVersion, "1.0");
    return KVersion();
}
