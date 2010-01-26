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

#include "stepbase.h"

#include <f32file.h>

void CStepBase::InitializeL()
	{
	// Read the certificate parameter and construct
	// the required certificate
	
	_LIT(KCertificateName, "cert");
	TPtrC fileName;
	
	if (!GetStringFromConfig(ConfigSection(), KCertificateName, fileName))
		{
			
		_LIT(KMessage, "Could not read certificate path from INI, abort.");
		Logger().Write(KMessage);
		
		SetTestStepResult(EAbort);
		User::Leave(KErrAbort);
		}
	else
		{
		_LIT(KMessageFormat, "Using certificate file for test '%S'");
		Logger().WriteFormat(KMessageFormat, &fileName);
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
	
HBufC* CStepBase::SubjectLC()
	{
	HBufC* subject = iCertificate->SubjectL();
	CleanupStack::PushL(subject);
	return subject;
	}
	
CStepBase::~CStepBase()
	{
	iSession.Close();
	delete iCertificate;
	}
