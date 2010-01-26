// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
 @file delayedgetsessionstep.cpp
 @internalTechnology
*/
#include "delayedgetsessionstep.h"
#include <tlsprovinterface.h>

#define KServer2  _L8("192.168.10.11")
#define KSessionId4 _L8("444444444455555555556666666666")
#define KSessionId2 _L8("222222222233333333334444444444")

CDelayedGetSessionStep::CDelayedGetSessionStep()
	{
	SetTestStepName(KDelayedGetSessionStep);
	}
	
TVerdict CDelayedGetSessionStep::doTestStepL()
	{
	TInt sessionDelay;
	sessionDelay = ReadGetSessionDelayL();
	
	// first we have to retrieve the available cipher suites
	TInt err = GetCipherSuitesL();  
	TInt sessionIdLength(0) ;
	
	CTlsCryptoAttributes* tlsCryptoAttributes = Provider()->Attributes();
				
	CX509Certificate* cert = NULL;
	err = VerifyServerCertificateL(cert);
	delete cert; // don't really need the cert
	
	err = CreateSessionL();
		
	// ensure we succeeded
	if (err != KErrNone)
		{
		INFO_PRINTF2(_L("Failed! Create Session failed! (Error %d)"), err);
		SetTestStepResult(EFail);
		}
	
	HBufC8* keyExMessage = NULL;
	err = ClientKeyExchange(keyExMessage);
	
	if (err != KErrNone)
		{
		INFO_PRINTF2(_L("Failed! Key exchange failed! (Error %d)"), err);
		SetTestStepResult(EFail);
		}
	CleanupStack::PushL(keyExMessage);	
	
	// Call ServerFinished to do the chache
	// derive the premaster secret from the key exchange method	
 	INFO_PRINTF1(_L("Deriving master secret."));
	HBufC8* premaster = DerivePreMasterSecretL(*keyExMessage);
	CleanupStack::PopAndDestroy(keyExMessage);  
	 
	// compute the master secret from the premaster.
	CleanupStack::PushL(premaster);
	HBufC8* master = ComputeMasterSecretL(*premaster);
	CleanupStack::PopAndDestroy(premaster);
	CleanupStack::PushL(master);  
	
	// do the caching 
	ValidateServerFinishL(*master);
	
	CleanupStack::PopAndDestroy(master);
	
	err = VerifyGetSessionL(tlsCryptoAttributes->iSessionNameAndID.iServerName, sessionIdLength);
	// case A			
	if (err != KErrNone || sessionIdLength == 0)
		{
		INFO_PRINTF1(_L("Case A Failed! GetSession failed!"));
		SetTestStepResult(EFail);
		return TestStepResult();
		}
	
	RTimer timer;
	TRequestStatus timerStatus;
	timer.CreateLocal();
	TTimeIntervalMicroSeconds32 waitTime( 1000000*(sessionDelay));
	timer.After( timerStatus, waitTime);
	User::WaitForRequest(timerStatus);
	timer.Close();  
	
	err = 0;
	sessionIdLength = 0;
	
	err = VerifyGetSessionL(tlsCryptoAttributes->iSessionNameAndID.iServerName, sessionIdLength);
	// case B, delay should have caused session to be cleared.
	if ( sessionIdLength != 0)
		{
		INFO_PRINTF1(_L("Case B Failed! GetSession failed!"));
		SetTestStepResult(EFail);
		}
						
	return TestStepResult();
	}

