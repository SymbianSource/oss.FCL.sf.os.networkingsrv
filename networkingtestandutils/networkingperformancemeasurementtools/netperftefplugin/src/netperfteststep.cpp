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

#include <e32base.h>
#include <test/testexecutelog.h>

#include "assert.h"
#include "netperfteststep.h"
#include "netperftest.h"

//_LIT( delim, "|" );

CIperfTestStep::CIperfTestStep(CIperfTestServer* aOwner)
	:iServer(aOwner) 
	{
	}

CIperfTestStep::~CIperfTestStep()
	{
	}

TInt CIperfTestStep::ReadIapL()
	{
	TInt iap=0;
	GetIntFromConfig( ConfigSection(), _L("IAP"), iap);
	if (iap > 0)
		{
		INFO_PRINTF2(_L("Will use IAP %d.."), iap);
		}
	return iap;
	}

TInt CIperfTestStep::ReadSnapL()
	{
	TInt snap=0;
	GetIntFromConfig( ConfigSection(), _L("SNAP"), snap);
	if (snap > 0)
		{
#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
		INFO_PRINTF2(_L("Will use Access Point %d.."), snap);
#else
		INFO_PRINTF1(_L("Access Point selection is not supported.."));
		snap = 0;
#endif // SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
		}
	return snap;
	}

TInt CIperfTestStep::ReadPacketSizeInBytesL()
	{
	TInt packetSizeInBytes=480;
	GetIntFromConfig( ConfigSection(), _L("PacketSizeInBytes"), packetSizeInBytes);
	INFO_PRINTF2(_L("Will use %d as packetSizeInBytes.."), packetSizeInBytes);
	return packetSizeInBytes;
	}

TInt CIperfTestStep::ReadTestDurationL()
	{
	TInt testDuration_sec=10;
	GetIntFromConfig( ConfigSection(), _L("TestDurationInSeconds"), testDuration_sec);
	INFO_PRINTF2(_L("Test duration will be %d seconds.."),testDuration_sec);
	return testDuration_sec;
	}

TInt CIperfTestStep::ReadSamplingPeriodL()
	{
	TInt samplingPeriod_ms=1000;
	GetIntFromConfig( ConfigSection(), _L("SamplingPeriodInMilliseconds"), samplingPeriod_ms);
	INFO_PRINTF2(_L("Will sample every %dms.."),samplingPeriod_ms);
	return samplingPeriod_ms;
	}


CTestWorker* CIperfTestStep::GetWorkerL()
	{
	CTestWorker* worker = iServer->Workers().Find(ConfigSection());
	if (worker==NULL)
		{
		User::Leave(KErrDied);
		}
	worker->SetLogger(Logger());
	return worker;
	}



