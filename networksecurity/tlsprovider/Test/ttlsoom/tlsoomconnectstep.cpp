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

#include "tlsoomconnectstep.h"

#include "tlsprovinterface.h"

CTlsOOMConnectStep* CTlsOOMConnectStep::NewL(CTestExecuteLogger& aLogger)
	{
	
	CTlsOOMConnectStep* self = new (ELeave) CTlsOOMConnectStep(aLogger);
	return self;
	
	}
	
CTlsOOMConnectStep::CTlsOOMConnectStep(CTestExecuteLogger& aLogger)
	: CTlsOOMStepBase(aLogger, KNullDesC)
	{
	}
	
void CTlsOOMConnectStep::DoTestStepL()
	{
	
	CTLSProvider* tlsProvider = CTLSProvider::ConnectL();
	delete tlsProvider;
	
	}
