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
// netperfte specific test steps called from TEF script.
// 
//

/**
 @file
 @internalTechnology
*/
 
#ifndef __NETPERF_TEST_H__
#define __NETPERF_TEST_H__

#include <test/testexecutestepbase.h>
#include "netperfteststep.h"
#include "netperfserver.h"

class CIperfTestSetupReceiver : public CIperfTestStep
	{
public:
	CIperfTestSetupReceiver(CIperfTestServer* aOwner);
	~CIperfTestSetupReceiver();
	virtual TVerdict doTestStepL();
	};

class CIperfTestSetupSender : public CIperfTestStep
	{
public:
	CIperfTestSetupSender(CIperfTestServer* aOwner);
	~CIperfTestSetupSender();
	virtual TVerdict doTestStepL();
	};

class CIperfTestSetupCpuSponge : public CIperfTestStep
	{
public:
	CIperfTestSetupCpuSponge(CIperfTestServer* aOwner);
	~CIperfTestSetupCpuSponge();
	virtual TVerdict doTestStepL();
	};

class CIperfTestStart : public CIperfTestStep
	{
public:
	CIperfTestStart(CIperfTestServer* aOwner);
	~CIperfTestStart();
	virtual TVerdict doTestStepL();
	};

class CIperfTestStop : public CIperfTestStep
	{
public:
	CIperfTestStop(CIperfTestServer* aOwner);
	~CIperfTestStop();
	virtual TVerdict doTestStepL();
	};

class CIperfTestReport : public CIperfTestStep
	{
public:
	CIperfTestReport(CIperfTestServer* aOwner);
	~CIperfTestReport();
	virtual TVerdict doTestStepL();
	};

class CIperfTestDestroy : public CIperfTestStep
	{
public:
	CIperfTestDestroy(CIperfTestServer* aOwner);
	~CIperfTestDestroy();
	virtual TVerdict doTestStepL();
	};

class CIperfStartProfile : public CIperfTestStep
	{
public:
	CIperfStartProfile(CIperfTestServer* aOwner);
	virtual TVerdict doTestStepL();
	};

class CIperfStopProfile : public CIperfTestStep
	{
public:
	CIperfStopProfile(CIperfTestServer* aOwner);
	virtual TVerdict doTestStepL();
	};

_LIT(KTestSetupReceiver,"SetupReceiver");
_LIT(KTestSetupSender,"SetupSender");
_LIT(KTestSetupCpuSponge,"SetupCpuMeter");
_LIT(KTestStart,"Start");
_LIT(KTestStop,"Stop");
_LIT(KTestReport,"Report");
_LIT(KTestDestroy,"Destroy");
_LIT(KTestStartProfiler,"StartProfile");
_LIT(KTestStopProfiler,"StopProfile");

#endif // __NETPERF_TEST_H__
