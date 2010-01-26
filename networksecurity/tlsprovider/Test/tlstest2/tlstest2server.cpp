// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
 @file tlstest2server.cpp
 @internalTechnology
*/
#include "tlstest2server.h"

#include "getrandomstep.h"
#include "ciphersuitesstep.h"
#include "verifyservercertstep.h"
#include "verifysignaturestep.h"
#include "clientkeyexchangestep.h"
#include "clientcertificatestep.h"
#include "clientfinishedstep.h"
#include "serverfinishedstep.h"
#include "encryptstep.h"
#include "decryptstep.h"
#include "keyderivationstep.h"
#include "verifyCreateMethodStep.h"
#include "verifyGetSessionstep.h"
#include "verifyCancellationstep.h"
#include "TSecureConnection.h"
#include "handshakestep.h"
#include "newkeyderivationstep.h"
#include "startupcommsstep.h"
#include "checkfilesstep.h"
#include "cachedservcertstep.h"
#include "negativegetsessionstep.h"
#include "multicancelstep.h"
#include "delayedgetsessionstep.h"
#include "createnegativestep.h"
#include "servcertwithdialogstep.h"
#include "updateentrystep.h"
#include "entrystatusstep.h"

CTlsTest2Server* CTlsTest2Server::NewLC()
	{
	CTlsTest2Server* self = new (ELeave) CTlsTest2Server;
	CleanupStack::PushL(self);
	// Force crypto to be loaded, and stay loaded, to speedup tests
	self->iSha = CSHA1::NewL();
	self->ConstructL(KTlsTest2Server);
	return self;
	}

CTlsTest2Server::~CTlsTest2Server()
{
	delete iSha;
}
	
CTestStep* CTlsTest2Server::CreateTestStep(const TDesC& aStepName)
	{
	CTestStep* ret = NULL;
	if (aStepName == KGetRandomStep)
		{
		ret = new CGetRandomStep;
		}
	else if (aStepName == KCipherSuitesStep)
		{
		ret = new CCipherSuitesStep;
		}
	else if (aStepName == KServerCertStep)
		{
		ret = new CVerifyServerCertStep;
		}
	else if (aStepName == KVerifySignatureStep)
		{
		ret = new CVerifySignatureStep;
		}
	else if (aStepName == KClientKeyExchangeStep)
		{
		ret = new CClientKeyExchangeStep;
		}
	else if (aStepName == KClientCertificateStep)
		{
		ret = new CClientCertificateStep;
		}
	else if (aStepName == KClientFinishedStep)
		{
		ret = new CClientFinishedStep;
		}
	else if (aStepName == KServerFinishedStep)
		{
		ret = new CServerFinishedStep;
		}
	else if (aStepName == KEncryptStep)
		{
		ret = new CEncryptStep;
		}
	else if (aStepName == KDecryptStep)
		{
		ret = new CDecryptStep;
		}
	else if (aStepName == KKeyDerivationStep)
		{
		ret = new CKeyDerivationStep;
		}
	else if (aStepName == KCreateMethodStep)
		{
		ret = new CCreateMethodStep;
		}
	else if (aStepName == KGetSessionStep)
		{
		ret = new CVerifyGetSessionStep;
		}
	else if (aStepName == KVerifyCancellationStep)
		{
		ret = new CVerifyCancellationStep;	
		}
	else if (aStepName == KTlsSecureConnectionTestStep)
		{
		ret = new CTSecureConnectionStep(aStepName);	
		}
	else if (aStepName == KHandShakeTestStep)
		{
		ret = new CHandShakeStep;	
		}
	else if (aStepName == KNewKeyDerivationStep)
		{
		ret = new CNewKeyDerivationStep;	
		}
	else if (aStepName == KStartupCommsStep)
		{
		ret = new CStartupCommsStep;
		}
	else if (aStepName == KCheckFilesStep)
		{
		ret = new CCheckFilesStep;
		}
	else if (aStepName == KCachedServCertStep)
		{
		ret = new CCachedServCertStep;
		}
	else if (aStepName == KNegativeGetSessionStep)
		{
		ret = new CNegativeGetSessionStep;
		}
	else if (aStepName == KMultiCancelStep)
		{
		ret = new CMultiCancelStep;
		}
	else if (aStepName == KDelayedGetSessionStep)
		{
		ret = new CDelayedGetSessionStep;
		}
	else if (aStepName == KNegativeCreatedStep)
		{
		ret = new CNegativeCreateStep;
		}
	else if (aStepName == KServCertWithDialogStep)
		{
		ret = new CServCertWithDialogStep;
		}
 	else if (aStepName == KEntryStatus)
		{
		ret = new CEntryStatusStep;
		}
	else if (aStepName == KUpdateEntryStep)
		{
		ret = new CUpdateEntryStep;
		}  
		
	return ret;
	
	}
