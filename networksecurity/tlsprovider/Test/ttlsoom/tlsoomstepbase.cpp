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

#include "tlsoomstepbase.h"

#include <TestExecuteLog.h>

TVerdict CTlsOOMStepBase::Start()
	{
	
	for (TInt i = 1; i < KMaxOOMSteps; i++)
		{
		
		__UHEAP_MARK;
		__UHEAP_FAILNEXT(i);
		
		TRAPD(err, DoTestStepL());
		
		__UHEAP_MARKEND;
		__UHEAP_RESET;
		
		if (err == KErrNone && iStatus == KErrNone)
			{
		
			return EPass;
			
			}
		
		
		}
		
	return EFail;
	
	}
	
void CTlsOOMStepBase::RunL()
	{
	CActiveScheduler::Stop();
	}
	
CTlsOOMStepBase::CTlsOOMStepBase(CTestExecuteLogger& aLogger, const TDesC& aConfigPath)
	: CActive(EPriorityStandard),
	  iLogger(aLogger),
	  iConfigPath(aConfigPath)
	{
	CActiveScheduler::Add(this);
	}

void CTlsOOMStepBase::DoCancel()
	{
	}
	
// Leaves the tlsprovider and certificate on the stack
// the attributes become owned by the provider.

void CTlsOOMStepBase::InitTlsProviderLC(CTLSProvider*& aTlsProvider, CTlsCryptoAttributes*& aCryptoAttributes,
	HBufC8*& aServerCert)
	{
	
	aTlsProvider = CTLSProvider::ConnectL();
	CleanupStack::PushL(aTlsProvider);
	
	RArray<TTLSCipherSuite> userCipherSuites;
	CleanupClosePushL(userCipherSuites);
	iStatus = KRequestPending;
	aTlsProvider->CipherSuitesL(userCipherSuites, iStatus);
	SetActive();
	CActiveScheduler::Start();
	TInt err = (0 == userCipherSuites.Count()) ? KErrNotSupported : KErrNone; 
	User::LeaveIfError(err);
	CleanupStack::PopAndDestroy(&userCipherSuites);

	aCryptoAttributes = aTlsProvider->Attributes();
	
	RFs fs;
	User::LeaveIfError(fs.Connect());
	CleanupClosePushL(fs);
	
	// Read server random
	
	static const TInt KRandomDataLen = 32;
	
	TBuf8<KRandomDataLen> input;
	_LIT(KServerRandomFileName, "server.rnd");
	ReadFileDataL(input, KServerRandomFileName, fs);
	aCryptoAttributes->iMasterSecretInput.iServerRandom.Copy(input.Ptr(), KRandomDataLen);
	
	// Read Client Random
	input.Zero();
	_LIT(KClientRandomFileName, "client.rnd");
	ReadFileDataL(input, KClientRandomFileName, fs);
	aCryptoAttributes->iMasterSecretInput.iClientRandom.Copy(input.Ptr(), KRandomDataLen);
	
	// Read server key parameters
	
	TBuf8<1000> paramInput;
	_LIT(KParamFileFormat, "keyparam%d.prm");
	
	TBuf<14> paramFile;
	
	for (TInt i = 1; i <= 3; i++)
		{
		
		paramFile.Format(KParamFileFormat, i);
		ReadFileDataL(paramInput, paramFile, fs);
		
		switch (i)
			{
			case 1:
				 aCryptoAttributes->iPublicKeyParams->iValue1 = paramInput.AllocL();
				 break;
			case 2:
				 aCryptoAttributes->iPublicKeyParams->iValue2 = paramInput.AllocL();
				 break; 
			case 3:
				 aCryptoAttributes->iPublicKeyParams->iValue3 = paramInput.AllocL();
				 break; 
			}
			
		paramInput.Zero();
		
		}
		
	aCryptoAttributes->iPublicKeyParams->iKeyType = ERsa;
	
	// Read server certificate
	
	TBuf8<1000> certInput;
	_LIT(KCertFileName, "server.cer");
	ReadFileDataL(certInput, KCertFileName, fs);
	
	aServerCert = certInput.AllocL();
	
	CleanupStack::PopAndDestroy(&fs);
	CleanupStack::PushL(aServerCert);
	
	}
	
void CTlsOOMStepBase::ReadFileDataL(TDes8& aDest, const TDesC& aFileName, RFs& aFileServer)
	{
	
	TPath fileName;	
	RFile file;
	_LIT(KFileNameFormat, "%S\\%S");
	
	fileName.Format(KFileNameFormat, &iConfigPath, &aFileName);
	
	User::LeaveIfError(file.Open(aFileServer, fileName, EFileRead));
	CleanupClosePushL(file);
	
	User::LeaveIfError(file.Read(aDest));
	
	CleanupStack::PopAndDestroy(&file);
	
	}
