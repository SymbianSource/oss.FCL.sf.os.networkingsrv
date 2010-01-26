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

/**
 @file
*/

#include "tlsoomstepwrapper.h"

#include "tlsoomconnectstep.h"
#include "tlsoomcipherstep.h"
#include "tlsoomauthstep.h"
#include "tlsoomkeyexchangestep.h"


CTlsOOMStepWrapper::CTlsOOMStepWrapper(const TDesC& aStepName)
	: iStepName(aStepName)
	{
	}
	
TVerdict CTlsOOMStepWrapper::doTestStepL()
	{
	
	CActiveScheduler* scheduler = new (ELeave) CActiveScheduler;
	CleanupStack::PushL(scheduler);
	CActiveScheduler::Install(scheduler);
	
	CTlsOOMStepBase* step;
	
	if (iStepName == KConnectStep)
		{
		
		step = CTlsOOMConnectStep::NewL(Logger());
		
		}
	else if (iStepName == KCipherStep)
		{
		
		step = CTlsOOMCipherStep::NewL(Logger());
		
		}
	else if (iStepName == KAuthStep)
		{
		
		TPtrC confPath = GetConfigPath();
		step = CTlsOOMAuthStep::NewL(confPath, Logger());
		
		}
	else if (iStepName == KKeyExchangeStep)
		{
		
		TPtrC confPath = GetConfigPath();
		TPtrC serverName = GetTlsServerName();
		TPtrC sessionID = GetTlsSessionID();
		
		step = CTlsOOMKeyExchangeStep::NewL(confPath, serverName, sessionID,
			Logger());
		
		}
	
	TVerdict result = step->Start();
	delete step;
	CleanupStack::PopAndDestroy(scheduler);
	
	return result;
	
	}
	
TPtrC CTlsOOMStepWrapper::GetConfigPath()
	{
	
	TPtrC result;
	_LIT(KConfigFilePath, "CONFPATH");
	GetStringFromConfig(ConfigSection(), KConfigFilePath, result); 
	return result;
	
	}
	
TPtrC CTlsOOMStepWrapper::GetTlsServerName()
	{
	
	TPtrC result;
	_LIT(KServerName, "SERVERNAME");
	GetStringFromConfig(ConfigSection(), KServerName, result);
	return result;
	
	}
	
TPtrC CTlsOOMStepWrapper::GetTlsSessionID()
	{
	
	TPtrC result;
	_LIT(KSessionID, "SESSIONID");
	GetStringFromConfig(ConfigSection(), KSessionID, result);
	return result;
	
	}
