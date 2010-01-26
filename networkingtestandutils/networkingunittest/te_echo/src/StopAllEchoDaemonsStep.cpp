// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
 @file StopAllEchoDaemonsStep.cpp
 @internalTechnology
*/
#include "StopAllEchoDaemonsStep.h"
#include "Te_echoSuiteDefs.h"

CStopAllEchoDaemonsStep::~CStopAllEchoDaemonsStep()
/**
 * Destructor
 */
	{
	}

CStopAllEchoDaemonsStep::CStopAllEchoDaemonsStep()
/**
 * Constructor
 */
	{
	SetTestStepName(KStopAllEchoDaemonsStep);
	}

TVerdict CStopAllEchoDaemonsStep::doTestStepPreambleL()
/**
 * @return - TVerdict code
 * Override of base class virtual
 */
	{
	SetTestStepResult(EFail);
	return TestStepResult();
	}


TVerdict CStopAllEchoDaemonsStep::doTestStepL()
/**
 * @return - TVerdict code
 * Override of base class pure virtual
 * Our implementation only gets called if the base class doTestStepPreambleL() did
 * not leave. That being the case, the current test result value will be EPass.
 */
	{
	REchoDaemonSession session;
	session.Connect();
	
	INFO_PRINTF1(_L("Stopping all Echo Daemons"));
	
	TInt err = session.StopAll();
	if (err == KErrNone)
		{
		INFO_PRINTF1(_L("Success"));
		SetTestStepResult(EPass);
		}
	else
		{
		INFO_PRINTF2(_L("Failed %d"), err);
		}
		
	session.Close();
	
	SetTestStepResult(EPass);
	
	return TestStepResult();
	}



TVerdict CStopAllEchoDaemonsStep::doTestStepPostambleL()
/**
 * @return - TVerdict code
 * Override of base class virtual
 */
	{
	return TestStepResult();
	}
