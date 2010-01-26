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

#include "tlsoomauthstep.h"

CTlsOOMAuthStep* CTlsOOMAuthStep::NewL(const TDesC& aConfigPath, CTestExecuteLogger& aLogger)
	{
	
	CTlsOOMAuthStep* result = new (ELeave) CTlsOOMAuthStep(aConfigPath, aLogger);
	return result;
	
	}
	
CTlsOOMAuthStep::CTlsOOMAuthStep(const TDesC& aConfigPath, CTestExecuteLogger& aLogger)
	: CTlsOOMStepBase(aLogger, aConfigPath)
	{
	}
	
void CTlsOOMAuthStep::DoTestStepL()
	{
	
	CTLSProvider* tlsProvider = NULL;
	CTLSSession* tlsSession = NULL;
	CTlsCryptoAttributes* tlsAttributes = NULL;
	HBufC8* serverCert = NULL;
	
	InitTlsProviderLC(tlsProvider, tlsAttributes, serverCert);
	
	
	TTLSCipherSuite cipherSuite;
	cipherSuite.iHiByte = 0;
	cipherSuite.iLoByte = 3;
	
	// Use client authentication to get maximum coverage
	
	tlsAttributes->iClientAuthenticate = ETrue;
	tlsAttributes->iDialogNonAttendedMode = ETrue;
	tlsAttributes->iCurrentCipherSuite = cipherSuite;
	tlsAttributes->iNegotiatedProtocol = KTLS1_0;
	
	iStatus = KRequestPending;
	tlsProvider->CreateL(tlsSession, iStatus);
	CleanupStack::PushL(tlsSession);
	SetActive();
	CActiveScheduler::Start();
	
	CleanupStack::PopAndDestroy(3, tlsProvider); // serverCert, tlsSession
	
	}
	
