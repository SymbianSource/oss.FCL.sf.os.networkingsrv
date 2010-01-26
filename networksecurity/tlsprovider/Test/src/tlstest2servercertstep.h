// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef __TLSTEST2SERVERCERTSTEP_H__
#define __TLSTEST2SERVERCERTSTEP_H__

#include <e32base.h>
#include <test/testexecutestepbase.h>

#include "tlsprovinterface.h"

_LIT(KServerCertStep, "ServerCertStep");
_LIT(KServerCert, "ServerCert");
_LIT(KDomainName, "DomainName");
_LIT(KExpectedResult, "ExpectedResult");

class CServerCertStep : public CTestStep
	{
public:
	CServerCertStep();
	~CServerCertStep();
	
	TVerdict doTestStepL();

private:
	CTLSProvider* iProvider;	
	};
	
class CGenericActive : public CActive
	{
public:
	CGenericActive()
		: CActive(EPriorityNormal)
		{
		CActiveScheduler::Add(this);
		};
		
	void Start()
		{
		SetActive();
		};
	
	void RunL()
		{
		CActiveScheduler::Stop();
		};
	
	void DoCancel()
		{
		// do nothing, just wait for completion
		};
		
	};

#endif /* __TLSTEST2SERVERCERTSTEP_H__ */
