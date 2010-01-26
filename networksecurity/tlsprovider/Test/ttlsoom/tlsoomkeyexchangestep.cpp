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

#include "tlsoomkeyexchangestep.h"

#include <x509cert.h>


CTlsOOMKeyExchangeStep* CTlsOOMKeyExchangeStep::NewL(const TDesC& aConfigPath, const TDesC& aServerName,
	const TDesC& aSessionID, CTestExecuteLogger& aLogger)
	{
	
	CTlsOOMKeyExchangeStep* self = new (ELeave) CTlsOOMKeyExchangeStep(aConfigPath, aServerName, aSessionID, aLogger);
	return self;
	
	}
	
CTlsOOMKeyExchangeStep::CTlsOOMKeyExchangeStep(const TDesC& aConfigPath, const TDesC& aServerName, 
	const TDesC& aSessionID, CTestExecuteLogger& aLogger)
	: CTlsOOMStepBase(aLogger, aConfigPath),
	  iSessionID(aSessionID),
	  iServerName(aServerName)
	{
	}
	
void CTlsOOMKeyExchangeStep::DoTestStepL()
	{
	
	CTLSProvider* tlsProvider;
	CTLSSession* tlsSession;
	CTlsCryptoAttributes* tlsAttributes;
	HBufC8* serverCert;
	
	InitTlsProviderLC(tlsProvider, tlsAttributes, serverCert);
	
	TTLSCipherSuite cipherSuite;
	cipherSuite.iHiByte = 0;
	cipherSuite.iLoByte = 3;
	
	tlsAttributes->iClientAuthenticate = EFalse;
	tlsAttributes->iDialogNonAttendedMode = ETrue;
	tlsAttributes->iCurrentCipherSuite = cipherSuite;
	
	tlsAttributes->iSessionNameAndID.iServerName.iAddress.Copy(iServerName);
	tlsAttributes->iSessionNameAndID.iServerName.iPort = 10;
	tlsAttributes->iSessionNameAndID.iSessionId.Append(iSessionID);
	 
	tlsAttributes->iCompressionMethod = ENullCompression;
		
	tlsAttributes->iNegotiatedProtocol.iMajor = 3;
	tlsAttributes->iNegotiatedProtocol.iMinor = 1; 
	tlsAttributes->iProposedProtocol.iMajor = 3;
	tlsAttributes->iProposedProtocol.iMinor = 1; 
		
	tlsAttributes->iPublicKeyParams->iKeyType = ERsa;
	
	CX509Certificate* serverCertObj;
	iStatus = KRequestPending;
	tlsProvider->VerifyServerCertificate(*serverCert, serverCertObj, iStatus);
	CleanupStack::PushL(serverCertObj);
	SetActive();
	CActiveScheduler::Start();
	
	if (iStatus != KErrNone)
		{
		User::Leave(iStatus.Int());
		}
	
	iStatus = KRequestPending;
	tlsProvider->CreateL(tlsSession, iStatus);
	CleanupStack::PushL(tlsSession);
	SetActive();
	CActiveScheduler::Start();
	
	// leave if the status is failure
	if (iStatus != KErrNone)
		{
		User::Leave(iStatus.Int());
		}
	
	HBufC8* clientKeyExch;
	
	iStatus = KRequestPending;
	tlsSession->ClientKeyExchange(clientKeyExch, iStatus);
	CleanupStack::PushL(clientKeyExch);
	SetActive();
	CActiveScheduler::Start();
	
	CleanupStack::PopAndDestroy(5, tlsProvider); //serverCert, tlsSession, clientKeyExchange, serverCertObj
	
	}
