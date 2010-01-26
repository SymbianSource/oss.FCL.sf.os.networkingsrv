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

#ifndef __CPU_SPONGE_H__
#define __CPU_SPONGE_H__

#include <test/testexecuteserverbase.h>
#include <e32property.h>
#include "netperfserver.h"


// measuring interval duration (seconds)
const TInt KCPURunLength = 1;
// calibration steps count
const TInt KCalibrationCount = 3;

// number of CPU time records
const TInt KMaxRecordItems = 500;

// RProperty signalization
const TUint KCpuSpongeSignalUid = 0x10272F49;
const TUint KCpuSpongeSignalKey = 1683;
const TUint KCpuSpongeResponseKey = 1684;

const TInt KSieveSize = 16 * 1024;


struct SMsrItem
{
	TInt iters;
	TUint32 timestamp;
};



class CCpuSponge : public CTestWorker
{
public:
	CCpuSponge(CTestExecuteLogger& aLogger);
	virtual ~CCpuSponge();
	virtual void Destroy();
	
	virtual void PrepareL();
	virtual void StartL();
	virtual void StopL();
	virtual void ReportL();


	void SetEaterThreadPriority(TInt aPriority) {iEaterThreadPriority=(TThreadPriority)aPriority;}

private:
	static TInt CPUEaterThreadEntryPoint(TAny *iArgs);
	void CPUEaterL();
	
	static void InitSieves(TUint8 *sieves, TInt limit);
	static void RemoveMultiples(TUint8 *sieves, TInt limit, TInt currPrime);
	static int  NextPrime(TUint8 *sieves, TInt limit, TInt currPrime);
	static void GeneratePrimes(TUint8 *sieves, TInt limit);
	static int  CountPrimes(TUint8 *sieves, TInt limit);

	static TInt CalculateFastCounterDiff(TInt lastTimerValue, TInt thisTimerValue)
		{
		TInt thisTimerDiff = thisTimerValue - lastTimerValue;
		return thisTimerDiff;	
		}
	
private:
	TThreadPriority iEaterThreadPriority;

	TBool iStopEatingCPU;
	RThread iEaterThread;
	RProperty iPropEater;
	RProperty iPropWorker;
	SMsrItem *iMeasures;
	TInt iMsrsCount;
	TUint32 iFirstMsrTime;
	TReal iWorkUnit;
};

#endif // __CPU_SPONGE_H__
