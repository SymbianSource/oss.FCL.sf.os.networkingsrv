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
// netperfte common functionality used by various test steps,
// mostly for reading fields from the ini file.
// 
//

/**
 @file
 @internalTechnology
*/

#ifndef __NETPERF_TEST_STEP_H__
#define __NETPERF_TEST_STEP_H__

#include <test/testexecuteserverbase.h>

class CTestWorker;
class CIperfTestServer;
class CIperfTestStep : public CTestStep
	{
public:
	CIperfTestStep(CIperfTestServer* aOwner);
	~CIperfTestStep();
		
	TInt ReadIapL();
	TInt ReadSnapL();
	TInt ReadPacketSizeInBytesL();
	TInt ReadTestDurationL();
	TInt ReadSamplingPeriodL();
	CTestWorker* GetWorkerL();

	CIperfTestServer * iServer;

	};

#endif // __NETPERF_TEST_STEP_H__
