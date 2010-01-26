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

#include "tlsoomcipherstep.h"

#include "tlsprovinterface.h"

CTlsOOMCipherStep* CTlsOOMCipherStep::NewL(CTestExecuteLogger& aLogger)
	{
	
	CTlsOOMCipherStep* self = new (ELeave) CTlsOOMCipherStep(aLogger);
	return self;
	
	}
	
CTlsOOMCipherStep::CTlsOOMCipherStep(CTestExecuteLogger& aLogger)
	: CTlsOOMStepBase(aLogger, KNullDesC)
	{
	}
	
void CTlsOOMCipherStep::DoTestStepL()
	{
	
	RArray<TTLSCipherSuite> cipherList;
	
	CTLSProvider* tlsProvider = CTLSProvider::ConnectL();
	CleanupStack::PushL(tlsProvider);
	
	iStatus = KRequestPending;
	tlsProvider->CipherSuitesL(cipherList, iStatus);
	SetActive();
	CActiveScheduler::Start();
	
	cipherList.Close();
	CleanupStack::PopAndDestroy(tlsProvider);
	
	}
