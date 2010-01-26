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

/**
 @file TestSteps.cpp
*/


#include <e32math.h>
#include <c32comm.h>
#include <in_sock.h>

#include "ss_pman.h"
#include "TestMgr.h"
#include "es_mbman.h"


#include <testexecutelog.h>
#include "TestSteps.h"
#include "main.h"

//
// Construction/Destruction
//

CPPPANVL::CPPPANVL()
	{
	SetTestStepName(KPPPANVL);
	}

CPPPANVL::~CPPPANVL()
	{ }

#if defined (__WINS__)
#define PDD_NAME _L("ECDRV")
#define LDD_NAME _L("ECOMM")
#define LDD_FNAME _L("ECOMM")
#else
#define PDD_NAME _L("EUART1")
#define LDD_NAME _L("ECOMM")
#define LDD_FNAME _L("FCOMM")
#endif

void CPPPANVL::CommInitL(TBool aEnhanced)
	{
	TInt err=User::LoadPhysicalDevice(PDD_NAME);
	if (!(err==KErrNone || err==KErrAlreadyExists))
		{
		ERR_PRINTF1(_L("Failed load physical device"));
		User::Leave(err);
		}

#if defined PDD2_NAME
# if defined (__EPOC32__)
	TMachineInfoV1Buf info;
	TEST_CHECKL(UserHal::MachineInfo(info),KErrNone,_L("Failed load physical device"));
	if (info().iMachineName.Compare(_L("PROTEA_RACKC"))==0
		|| info().iMachineName.Compare(_L("PROTEA_RACKD"))==0)
		{
# endif //__EPOC32__
		err = User::LoadPhysicalDevice(PDD2_NAME);
		if (err!=KErrNone && err!=KErrAlreadyExists)
			User::Leave(err);
# if defined (__EPOC32__)
		}
# endif //__EPOC32__
#endif //PDD2_NAME

	if (aEnhanced)
		err=User::LoadLogicalDevice(LDD_FNAME);
	else
		err=User::LoadLogicalDevice(LDD_NAME);
	if (err!=KErrNone && err!=KErrAlreadyExists)
		{
		ERR_PRINTF2(_L("Failed load physical device: %s"), aEnhanced ? LDD_FNAME : LDD_NAME);
		User::Leave(err);
		}

 	_LIT(KPhbkSyncCMI, "phbsync.cmi");
    err = StartC32WithCMISuppressions(KPhbkSyncCMI);
	if (err!=KErrNone && err!=KErrAlreadyExists)
		{
		ERR_PRINTF1(_L("Failed to start comm process"));
		User::Leave(err);
		}
	}

void CPPPANVL::StartClientL()
	{
	CTestMgr* pMgr= CTestMgr::NewL(Logger(), this);
	CleanupStack::PushL(pMgr);
	pMgr->StartEngineL();
	// set the mgr going
	CActiveScheduler::Start();
	CleanupStack::PopAndDestroy();  // pMgr
	}

TVerdict CPPPANVL::doTestStepL()
	{
	SetTestStepResult(EPass);
	INFO_PRINTF1(_L("------------------------------------------"));
	INFO_PRINTF1(_L("-------- PPP <-> ANVL test -----------"));
	INFO_PRINTF1(_L("------------------------------------------"));

	INFO_PRINTF1(_L("Initialising test environment"));

	CTrapCleanup* pTrapCleanup=CTrapCleanup::New();
	if (!pTrapCleanup)
		{
		INFO_PRINTF1(_L("Failed to allocate CTrapCleanup object"));
		return EFail;
		}
	// initialise serial comms
	TInt err=KErrNone;
	TRAP(err, CommInitL(EFalse));

	TEST(err == KErrNone);
	if (err)
		{
		INFO_PRINTF1(_L("Failed to init Comm"));
		return EFail;
		}

	CActiveScheduler* pActiveScheduler=new CActiveScheduler;
	if (!pActiveScheduler)
		{
		INFO_PRINTF1(_L("Failed to allocate CActiveScheduler object"));
		return EFail;
		}
	CActiveScheduler::Install(pActiveScheduler);

	TRAP(err, doCreateMBufL())

	delete pActiveScheduler;
	delete pTrapCleanup;
	return TestStepResult();
	}

static const TInt KMBuf_MaxAvail = 393216;
static const TInt KMBuf_MBufSize = 128;
static const TInt KMBuf_MBufSizeBig = 1600;
static const TInt KMBuf_InitialAllocation = 128;
static const TInt KMBuf_MinGrowth = 64;
static const TInt KMBuf_GrowthThreshold = 40;
void CPPPANVL::doCreateMBufL()
	{
	TInt err;

    // Initialize the MBuf manager
   	MMBufSizeAllocator *mBufSizeAllocator;
    CMBufManager *mbufMgr = CMBufManager::NewL(KMBuf_MaxAvail, mBufSizeAllocator);
	CleanupStack::PushL(mbufMgr);

	if (mbufMgr && mBufSizeAllocator)
		{
		// configure the mbuf size allocation info
		mBufSizeAllocator->AddL(KMBuf_MBufSize,    KMBuf_InitialAllocation, KMBuf_MinGrowth, KMBuf_GrowthThreshold);
		mBufSizeAllocator->AddL(KMBuf_MBufSizeBig, KMBuf_InitialAllocation, KMBuf_MinGrowth, KMBuf_GrowthThreshold);
		}

	// Go go!!!
	TRAP(err, StartClientL());
	TEST(err == KErrNone);
	if (err)
		{
		INFO_PRINTF1(_L("Leave in StartClientL()"));
		}

	INFO_PRINTF1(_L("Cleaning up test environment"));

	CleanupStack::PopAndDestroy(mbufMgr);
	}
