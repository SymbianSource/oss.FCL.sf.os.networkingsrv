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
// Common netperfte code for sender/receiver/CPU monitor
// Includes CTestWorker spawned thread support for TEF.
// 
//

/**
 @file
 @internalTechnology
*/
 
#ifndef __NETPERF_SERVER_H__
#define __NETPERF_SERVER_H__

#include <e32property.h>
#include <es_sock.h>
#include <test/testexecuteserverbase.h>
#include "simplemaptemplate.h"
#include "iperfprotocol.h"
#include "netperftrace.h"


#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
namespace DelayMeter { class CSendTimestamper; }
namespace DelayMeter { class CReceiveRecorder; }
#endif

const TInt KIperfServerUid = { 0x10272F46 };

const TUint KDefaultBufferSize = 32768;

enum
	{
	KTestWorkerStart	= 1,
	KTestWorkerStop		= 2,
	KTestWorkerReport	= 3,
	KTestWorkerDestroy	= 4
	};


class CIperfTestServer;
class CTestWorkerControl;
class CTestWorker : public CBase
	{
public:
	CTestWorker(CTestExecuteLogger& aLogger)
		: iLogger(&aLogger),
		  iCommandIsComplete(ETrue)
		{}

	virtual ~CTestWorker();

	virtual void PrepareL() = 0;
	virtual void StartL() = 0;
	virtual void StopL() = 0;
	virtual void ReportL() = 0;
	virtual void Destroy() = 0;

	void SetLogger(CTestExecuteLogger& aLogger) {iLogger=&aLogger;}

	CTestExecuteLogger& Logger() {return *iLogger;}

	void SpawnWorkerThreadL(TThreadPriority aPriority,TName aThreadName);
	void CheckForWorkerThreadDeathL();

	void SendCommandL(TUint aCommand);
	void SignalOwnerThreadL(TInt aCode);
	void SignalWorkerThreadL(TInt aCode);
	
	
	void CommandCompletedL(TInt aResult);
	void DontCompleteCommandYet() {iCommandIsComplete=EFalse;}
	TBool IsCommandComplete() {return iCommandIsComplete;}
	TInt GetQueuedCommand();

	void ShutdownWorkerThreadL();

	static TInt WorkerThreadEntryPoint(TAny* aArgs);
	void RunWorkerL();
	
protected:
	RThread iWorkerThread; // thread in which this runs
	CTestExecuteLogger* iLogger;

	CTestWorkerControl* iControl; // owned by worker thread mainloop, NOT by this object
	TBool iCommandIsComplete;
	};


class CTestWorkerControl : public CActive
	{
public:
	static CTestWorkerControl* NewL(CTestWorker* aTestWorker);
	
	CTestWorkerControl(CTestWorker* aTestWorker) :
		CActive(CActive::EPriorityHigh),
		iTestWorker(aTestWorker)
		{
		}
	
	void ConstructL();
	void RunL();
	void CommandCompletedL(TInt aResult);
	TInt GetQueuedCommand();
	void DoCancel();
	
protected:
	RProperty iProperty;
	CTestWorker* iTestWorker;
	};


class CIperfTestWorker : public CTestWorker
	{
public:
	typedef CTestWorker ParentClass;

	CIperfTestWorker(CIperfTestServer& aIperfTestServer, CTestExecuteLogger& aLogger)
		: iIperfTestServer(aIperfTestServer),
		  ParentClass(aLogger),
		  iPacketCount(0)
		{}
	
	virtual ~CIperfTestWorker();

	void SetIap(TInt aIap) { iIap=aIap; }
	void SetSnap(TInt aSnap) { iSnap=aSnap; }

	void SetTcpMode(TBool aTcpMode) {iTcpMode = aTcpMode;}
	TBool GetTcpMode() const {return iTcpMode;}
	
	void SetPacketSizeInBytes(TInt aPacketSizeInBytes) {iPacketSizeInBytes = aPacketSizeInBytes;}
	TInt GetPacketSizeInBytes() const {return iPacketSizeInBytes;}

	void SetThroughputInKilobitsPerSecond(const TInt a) {iSendThroughput_kbps=a;}
	
	RSocketServ& SockServL();

	
	void SetTestDuration_sec(TInt aTestDuration_sec)
		{ iTestDuration = aTestDuration_sec; }
	TInt GetTestDuration_sec() const
		{ return iTestDuration.Int(); }


	void SetSamplingPeriod_ms(TInt aSamplingPeriod_ms)
		{ iSamplingPeriod = aSamplingPeriod_ms * 1000; }
	TInt GetSamplingPeriod_ms() const
		{ return iSamplingPeriod.Int() / 1000; }
	TInt GetSamplingPeriod_us() const
		{ return iSamplingPeriod.Int(); }

	TInt GetMaxCollectedSamples() const
		{
		TInt a = GetTestDuration_sec() * 1000 / GetSamplingPeriod_ms() ;
		return a+4; // a few for luck, in case startup is slow
		}

	void IncrementPacketCount()
		{
		++iPacketCount;
		}
	
	void SetUpJitterBufferL();
	TBool RecordingJitter();
	void RecordJitter();
	
	virtual void PrepareL();
	virtual void StartL();
	virtual void StopL();
	virtual void ReportL();
	virtual void Destroy();
	
protected:

	TInt iIap;
	TInt iSnap;
	TBool iTcpMode;
	TInt iPacketSizeInBytes;
	TInt iSendThroughput_kbps;
	
	TInt iPacketCount;
	
	CIperfTestServer& iIperfTestServer;
	RConnection iConnection;
	RSocket iSocket;

	TUint32* iCollectedPacketCountSamples;
	TUint32* iCollectedByteCountSamples;
	
	TUint32 iFastCounterBeforeConnectionStart;
	TUint32 iFastCounterAfterConnectionStart;	

	TBool	 iJitterStartPacket;
	TBool	 iJitterRecordingFinished;
	TUint32* iJitterTimestampBuffer;
	TUint32* iJitterTimestamp;
	TUint32* iJitterLastTimestampInBuffer;
	
protected: // from CSTW

	TBool iRunning;
	TTimeIntervalSeconds iTestDuration;
	TTimeIntervalMicroSeconds32 iSamplingPeriod;
	TBool iFastCounterCountsUp;

	TInt iCurrentDataIndex;
	TUint32* iCollectedTimerSamples;
	};

	
class CIperfTestServer : public CTestServer
		{
	public:
		typedef CTestServer ParentClass;
		
		void ConnectSocketServL();

		static CIperfTestServer* NewL();
		virtual ~CIperfTestServer();
		virtual CTestStep* CreateTestStep(const TDesC& aStepName);

		RSocketServ& SockServL();

		RPointerDesMap<CTestWorker>& Workers()
			{ return iWorkers; }

	private:
		RSocketServ* iSockServ;

		RPointerDesMap<CTestWorker> iWorkers;	
		};


class MRunningFastCounter
	{
public:
	MRunningFastCounter();

	static TInt CalculateFastCounterDiff(TInt lastTimerValue, TInt thisTimerValue)
		{
		TInt thisTimerDiff = thisTimerValue - lastTimerValue;
		return thisTimerDiff;	
		}

	void StartRunningFastCounter();	
	void UpdateRunningFastCounter();	
		
protected:
	TInt32 iFastCounterFrequency;
	TBool iFastCounterCountsUp;
 	TInt32 iReportPeriodInFastCounterUnits;
	
	TInt64 iRunningFastCounterValue;
	TInt32 iLastFastCounterValue;
	TInt64 iNextReportRunningFastCounterValue;
	};

		
	
#endif // __NETPERF_SERVER_H__


