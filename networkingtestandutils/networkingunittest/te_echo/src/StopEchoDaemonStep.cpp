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
 @file StopEchoDaemonStep.cpp
 @internalTechnology
*/
#include "StopEchoDaemonStep.h"
#include "Te_echoSuiteDefs.h"

CStopEchoDaemonStep::~CStopEchoDaemonStep()
/**
 * Destructor
 */
	{
	}

CStopEchoDaemonStep::CStopEchoDaemonStep()
/**
 * Constructor
 */
	{
	SetTestStepName(KStopEchoDaemonStep);
	}

TVerdict CStopEchoDaemonStep::doTestStepPreambleL()
/**
 * @return - TVerdict code
 * Override of base class virtual
 */
	{
	SetTestStepResult(EFail);
	ReadConfigFromIniL();
	return TestStepResult();
	}


TVerdict CStopEchoDaemonStep::doTestStepL()
/**
 * @return - TVerdict code
 * Override of base class pure virtual
 * Our implementation only gets called if the base class doTestStepPreambleL() did
 * not leave. That being the case, the current test result value will be EPass.
 */
	{
	REchoDaemonSession session;
	session.Connect();
	
	INFO_PRINTF3(_L("Stopping Echo Daemon on Iap %d (protocol %d)"), iIap, iProtocol);

	TInt err = session.Stop(iIap, iProtocol,iPort);
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



TVerdict CStopEchoDaemonStep::doTestStepPostambleL()
/**
 * @return - TVerdict code
 * Override of base class virtual
 */
	{
	return TestStepResult();
	}
