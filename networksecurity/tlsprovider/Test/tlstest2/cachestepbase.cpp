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
 @file cachestepbase.cpp
 @internalTechnology	
*/

#include "cachestepbase.h"

#include <f32file.h>

void CCacheStepBase::InitializeL()
	{
	// Read the certificate parameter and construct
	// the required certificate
	
	_LIT(KCertificateName, "cert");
	TPtrC fileName;
	
	if (!GetStringFromConfig(ConfigSection(), KCertificateName, fileName))
		{
		
		INFO_PRINTF1(_L("Could not read certificate path from INI, abort."));
				
		SetTestStepResult(EFail);
		User::Leave(KErrNotFound);
		}
	else
		{
		INFO_PRINTF2(_L("Using certificate file for test '%S"),&fileName);
		}
		
	RFs fs;
	User::LeaveIfError(fs.Connect());
	CleanupClosePushL(fs);
	
	RFile file;
	User::LeaveIfError(file.Open(fs, fileName, EFileRead | EFileShareReadersOnly));
	CleanupClosePushL(file);
	
	TInt size;
	User::LeaveIfError(file.Size(size));
	
	HBufC8* certBuffer = HBufC8::NewL(size);
	TPtr8 buf = certBuffer->Des();
	CleanupStack::PushL(certBuffer);
	User::LeaveIfError(file.Read(buf));
	
	// construct the certificate from the raw data
	iCertificate = CX509Certificate::NewL(*certBuffer);
	
	CleanupStack::PopAndDestroy(3, &fs);
	
	// connect to the tls cache server
	User::LeaveIfError(iSession.Open(Certificate()));
	
	}
	
HBufC* CCacheStepBase::SubjectLC()
	{
	HBufC* subject = iCertificate->SubjectL();
	CleanupStack::PushL(subject);
	return subject;
	}
	
CCacheStepBase::~CCacheStepBase()
	{
	iSession.Close();
	delete iCertificate;
	}
