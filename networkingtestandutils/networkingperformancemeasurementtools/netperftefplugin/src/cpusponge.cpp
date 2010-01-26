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
// Test worker in netperfte which measures spare CPU time on
// the system by running a busy loop at idle priority
// 
//

/**
 @file
 @internalTechnology
*/

#include "cpusponge.h"
#include <hal.h>

CCpuSponge::CCpuSponge(CTestExecuteLogger& aLogger)
	: CTestWorker(aLogger),
	iEaterThreadPriority(EPriorityMuchLess)
	{
	iStopEatingCPU = EFalse;
	iMeasures = NULL;
	iMsrsCount = 0;
	iFirstMsrTime = 0;
	iWorkUnit = 0;
	}

/*virtual*/ CCpuSponge::~CCpuSponge()
	{
	// threads share heap so though the alloc was done by the worker thread, the
	// deletion can be done by TEF threads
	delete(iMeasures);

	}

/*virtual*/ void CCpuSponge::Destroy()
	{
	// no resources (specifically signals) to clear up in the worker thread
	}

/*virtual*/ void CCpuSponge::PrepareL()
	{
	// spawn cpu-eating thread
	User::LeaveIfError(
		iEaterThread.Create(_L("CPU-Sponge"), CCpuSponge::CPUEaterThreadEntryPoint, 8192, NULL, this));
	
	// allocate space for results
	iMeasures = new (ELeave) SMsrItem[KMaxRecordItems];

	// prepare worker property
	TInt ret;
	ret = iPropWorker.Define(TUid::Uid(KCpuSpongeSignalUid), KCpuSpongeSignalKey, RProperty::EInt);
	if (ret != KErrAlreadyExists)
	{
		User::LeaveIfError(ret);
	}
	ret = iPropWorker.Attach(TUid::Uid(KCpuSpongeSignalUid), KCpuSpongeSignalKey);
	User::LeaveIfError(ret);
	
	// prepare eater property
	ret = iPropEater.Define(TUid::Uid(KCpuSpongeSignalUid), KCpuSpongeResponseKey, RProperty::EInt);
	if (ret != KErrAlreadyExists)
	{
		User::LeaveIfError(ret);
	}
	ret = iPropEater.Attach(TUid::Uid(KCpuSpongeSignalUid), KCpuSpongeResponseKey);
	User::LeaveIfError(ret);
	
	// calibrate
	TRequestStatus stat;
	iPropWorker.Subscribe(stat);
	iEaterThread.Resume();
   	User::WaitForRequest(stat);
	}

/*virtual*/ void CCpuSponge::StartL()
	{
	RProperty::Set(TUid::Uid(KCpuSpongeSignalUid), KCpuSpongeResponseKey, 0);
	}

/*virtual*/ void CCpuSponge::StopL()
	{
	// signalize thread termination flag
	iStopEatingCPU = ETrue;
	
	TRequestStatus req;
	iEaterThread.Logon(req);
	User::WaitForRequest(req);
	User::LeaveIfError(req.Int());
	}

/*virtual*/ void CCpuSponge::ReportL()
	{
	TInt fastCounterFreq; // fast counter frequency
	User::LeaveIfError(HAL::Get(HALData::EFastCounterFrequency, fastCounterFreq));

	INFO_PRINTF1(_L("CPU usage data begin.."));
	INFO_PRINTF1(_L("time(s),timeDelta(s),itersCount,cpuIdlePercentage"));
	
	TUint32 last = iFirstMsrTime;
	TInt64 runningFastCounter = (TUint)iFirstMsrTime; // ensure we start positive
	for (TInt i = 0; i < iMsrsCount; i++)
		{
		SMsrItem *item = &iMeasures[i];
		TInt timediff = CalculateFastCounterDiff(last, item->timestamp);
		last = item->timestamp;
		runningFastCounter+=timediff;

		TReal currUnit = TReal(item->iters) / timediff;

		TReal32 timeDiffReal = timediff;
		timeDiffReal /= fastCounterFreq;

		TReal32 timerReal = runningFastCounter;
		timerReal /= fastCounterFreq;

		INFO_PRINTF5(_L("%.6f, %.6f, %d, %.2f"), timerReal, timeDiffReal, item->iters, currUnit / iWorkUnit * 100);
		}
	INFO_PRINTF1(_L("CPU usage data end."));
	}

/*static*/ TInt CCpuSponge::CPUEaterThreadEntryPoint(TAny *iArgs)
	{
	// Need a cleanup stack
	CTrapCleanup* cleanupStack = CTrapCleanup::New();

	TRAPD(error, (static_cast<CCpuSponge*>(iArgs))->CPUEaterL();)

	// Remove our cleanup stack
	delete cleanupStack;

	return error;
	}

void CCpuSponge::CPUEaterL()
	{
	RThread().SetPriority(iEaterThreadPriority);
	TTime end, now;
	TUint32 tmStart, tmEnd;
	TReal workUnit;

	TUint8* sieve = new (ELeave) TUint8[KSieveSize / 8 + 1];
	CleanupStack::PushL(sieve);
	
	//
	// calibration phase
	//
	
	for (TInt i = 0; i < KCalibrationCount; i++)
		{
		TInt iter = 0;
		end.UniversalTime();
		end += TTimeIntervalSeconds(KCPURunLength);
		tmStart = User::FastCounter();
		do
			{
			GeneratePrimes(sieve, KSieveSize);
			TInt numPrimes = CountPrimes(sieve, KSieveSize);
			now.UniversalTime();
			iter++;
			}
		while (!iStopEatingCPU && now < end);
		
		tmEnd = User::FastCounter();
		workUnit = TReal(iter) / CalculateFastCounterDiff(tmStart, tmEnd);
		if (workUnit > iWorkUnit)
			iWorkUnit = workUnit;
		}
	
	TRequestStatus stat;
	iPropEater.Subscribe(stat);

	// calibration has finished, signalize control thread
	RProperty::Set(TUid::Uid(KCpuSpongeSignalUid), KCpuSpongeSignalKey, 0);
	// wait for signal
   	User::WaitForRequest(stat);
	
	
	//
	// measurement phase
	//

	iFirstMsrTime = User::FastCounter();
	while (!iStopEatingCPU && iMsrsCount < KMaxRecordItems)
		{
		TInt iter = 0;
		end.UniversalTime();
		end += TTimeIntervalSeconds(KCPURunLength);
		do
			{
			GeneratePrimes(sieve, KSieveSize);
			TInt numPrimes = CountPrimes(sieve, KSieveSize);
			now.UniversalTime();
			iter++;
			}
		while (!iStopEatingCPU && now < end);
		
		// store raw values to array
		iMeasures[iMsrsCount].iters = iter;
		iMeasures[iMsrsCount].timestamp = User::FastCounter();
		iMsrsCount++;
		}

	CleanupStack::Pop(sieve);
	delete sieve;
	
	// signalize end of measuring
	iStopEatingCPU++;
	}

// Sieve of Eratosthenes code; rubbishy waste of CPU.
// code from http://badcomputer.org/unix/code/eratos/c1.bot

#define getbit(arr, index) (arr[index / 8] & 0x80 >> index % 8)
#define clearbit(arr, index) (arr[index / 8] &= ~(0x80 >> index % 8))

/*static*/ void CCpuSponge::InitSieves(TUint8 *sieves, TInt limit)
	{
	int i;
	*sieves++ = 0x3f;
	for (i = 8; i <= limit; i += 8 )
		{
		*sieves++ = 0xff;
		}
	}

/*static*/ void CCpuSponge::RemoveMultiples(TUint8 *sieves, TInt limit, TInt currPrime)
	{
	int i;
	for (i = currPrime << 1; i <= limit; i += currPrime)
		{
		clearbit(sieves, i);
		}
	}

/*static*/ int CCpuSponge::NextPrime(TUint8 *sieves, TInt limit, TInt currPrime)
	{
	currPrime++;
	for (; currPrime <= limit; currPrime++)
		{
		if (getbit(sieves, currPrime))
			{
			return currPrime;
			}
		}
	return currPrime;
	}

/*static*/ void CCpuSponge::GeneratePrimes(TUint8 *sieves, TInt limit)
	{
	InitSieves(sieves, limit);
	int currPrime = 2;
	while (currPrime * currPrime <= limit)
		{
		RemoveMultiples(sieves, limit, currPrime);
		currPrime = NextPrime(sieves, limit, currPrime);
		}
	}

/*static*/ int CCpuSponge::CountPrimes(TUint8 *sieves, TInt limit)
	{
	int num = 0;
	int i;
	for (i = 0; i <= limit; i++)
		{
		if (getbit(sieves, i))
			{
			++num;
			}
		}
	return num;
	}

