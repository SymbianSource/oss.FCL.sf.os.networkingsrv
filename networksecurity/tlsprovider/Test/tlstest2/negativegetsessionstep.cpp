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
 @file negativegetsessionstep.cpp
 @internalTechnology
*/
#include "negativegetsessionstep.h"
#include <tlsprovinterface.h>

#define KServer2  _L8("192.168.10.11")
#define KSessionId4 _L8("444444444455555555556666666666")

CNegativeGetSessionStep::CNegativeGetSessionStep()
	{
	SetTestStepName(KNegativeGetSessionStep);
	}
	
TVerdict CNegativeGetSessionStep::doTestStepL()
	{
	// first we have to retrieve the available cipher suites
	TInt err = GetCipherSuitesL();  
	TInt sessionIdLength(0) ;
	
	CTlsCryptoAttributes* tlsCryptoAttributes = Provider()->Attributes();
		
	tlsCryptoAttributes->iSessionNameAndID.iServerName.iAddress.Copy( KServer2 );
	tlsCryptoAttributes->iSessionNameAndID.iServerName.iPort = 10;
	tlsCryptoAttributes->iSessionNameAndID.iSessionId.Copy( KSessionId4 );
	 
	
	tlsCryptoAttributes->iCompressionMethod = ENullCompression;
	tlsCryptoAttributes->iCurrentCipherSuite.iHiByte = 0;
	tlsCryptoAttributes->iCurrentCipherSuite.iLoByte = 3;
			
	tlsCryptoAttributes->iNegotiatedProtocol.iMajor = 3;
	tlsCryptoAttributes->iNegotiatedProtocol.iMinor = 0; 
	
	tlsCryptoAttributes->iProposedProtocol.iMajor = 3;
	tlsCryptoAttributes->iProposedProtocol.iMinor = 0; 
			
	tlsCryptoAttributes->iPublicKeyParams->iKeyType = ERsa;
	tlsCryptoAttributes->iClientAuthenticate = EFalse; 
	tlsCryptoAttributes->iDialogNonAttendedMode = ETrue;
	
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
		return TestStepResult();
		}
	CleanupStack::PushL(keyExMessage);	
	
	// Call ServerFinished to do the chache
	INFO_PRINTF1(_L("Deriving premaster secret."));
			
	// derive the premaster secret from the key exchange method	
	HBufC8* premaster = DerivePreMasterSecretL(*keyExMessage);
	CleanupStack::PopAndDestroy(keyExMessage);
	
	INFO_PRINTF1(_L("Deriving master secret."));
	
	// compute the master secret from the premaster.
	CleanupStack::PushL(premaster);
	HBufC8* master = ComputeMasterSecretL(*premaster);
	CleanupStack::PopAndDestroy(premaster);
	CleanupStack::PushL(master);
	
	// do the caching 
	ValidateServerFinishL(*master);
	
	CleanupStack::PopAndDestroy(master);
	// case A:
	
	err = VerifyGetSessionL(tlsCryptoAttributes->iSessionNameAndID.iServerName,sessionIdLength);
	
	if (err != KErrNone)
		{
		INFO_PRINTF1(_L("Case A Failed! GetSession failed!"));
		SetTestStepResult(EFail);
		return TestStepResult();
		}
		
 	//case B: get a session without a match.
		
	CTLSProvider* tlsProviderB = CTLSProvider::ConnectL();
		
	CTlsCryptoAttributes* tlsCryptoAttributesB = tlsProviderB->Attributes();
	
	tlsCryptoAttributesB->iCurrentCipherSuite.iHiByte = 0;
	tlsCryptoAttributesB->iCurrentCipherSuite.iLoByte = 3;
	
	tlsCryptoAttributesB->iNegotiatedProtocol.iMajor = 3;
	tlsCryptoAttributesB->iNegotiatedProtocol.iMinor = 1; 
	
	tlsCryptoAttributesB->iProposedProtocol.iMajor = 6;
	tlsCryptoAttributesB->iProposedProtocol.iMinor = 6; 
	
	TTLSSessionNameAndID sessionNameAndIDB;
	sessionNameAndIDB.iServerName.iAddress.Copy( KServer2 );
	sessionNameAndIDB.iServerName.iPort = 10;
			
	err = VerifyGetSessionL(tlsProviderB ,sessionNameAndIDB.iServerName,sessionIdLength);
		
	if (err != KErrNone && sessionIdLength != 0)
		{
		INFO_PRINTF1(_L("Case B Failed! GetSession failed!"));
		SetTestStepResult(EFail);
		delete tlsProviderB;
		return TestStepResult();
		}
	
	// case C. get a session with a match.
	
	tlsCryptoAttributesB->iCurrentCipherSuite.iHiByte = 0;
	tlsCryptoAttributesB->iCurrentCipherSuite.iLoByte = 3;
	
	tlsCryptoAttributesB->iNegotiatedProtocol.iMajor = 3;
	tlsCryptoAttributesB->iNegotiatedProtocol.iMinor = 0; 
		
	tlsCryptoAttributesB->iProposedProtocol.iMajor = 3;
	tlsCryptoAttributesB->iProposedProtocol.iMinor = 0; 
	
	err = VerifyGetSessionL(tlsProviderB ,sessionNameAndIDB.iServerName,sessionIdLength);
			
	if (err != KErrNone && sessionIdLength == 0)
		{
		INFO_PRINTF1(_L("Case C Failed! GetSession failed!"));
		SetTestStepResult(EFail);
		delete tlsProviderB;
		return TestStepResult();
		}
		
	err = ClearSessionCacheL(tlsProviderB, sessionNameAndIDB);
	if (err != KErrNone)
		{
		INFO_PRINTF2(_L("Failed! Clear Session Failed! (Error %d)"), err);
		SetTestStepResult(EFail);
		delete tlsProviderB;
		return TestStepResult();
		}
	
	// check that session is gone.
	err = VerifyGetSessionL(tlsProviderB ,sessionNameAndIDB.iServerName,sessionIdLength);
				
	if (err != KErrNone && sessionIdLength != 0)
		{
		INFO_PRINTF1(_L("Case C Failed! Session is still present"));
		SetTestStepResult(EFail);
		}
	
	delete tlsProviderB;
					
	return TestStepResult();
	}

