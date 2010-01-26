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
 
#include <hal.h>
#include <hal_data.h>
#include <commdbconnpref.h>
#include <in_sock.h>
#ifdef SYMBIAN_OLD_EXPORT_LOCATION
#include <networking/netperftrace.h>
#else
#include "netperftrace.h"
#endif
#include "netperfserver.h"
#include "netperftest.h"
#include "iperfprotocol.h"

_LIT(KServerName,"NetperfTE");

#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
#ifdef SYMBIAN_OLD_EXPORT_LOCATION
#include <networking/delaymeterprotofactory.h> //
#include <networking/delaymeter.h> //
#else
#include "delaymeterprotofactory.h"
#include "delaymeter.h"
#endif
using namespace DelayMeter;
#endif // SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY

class TDes16MyIgnoreOverflow : public TDes16Overflow
	{
public:
	virtual void Overflow(TDes16 &) {}
	};

CIperfTestServer* CIperfTestServer::NewL()
/**
 * @return - Instance of the test server
 * Called inside the MainL() function to create and start the
 * CTestServer derived server.
 */
	{
	CIperfTestServer * server = new (ELeave) CIperfTestServer();
	CleanupStack::PushL(server);
	
	// Either use a StartL or ConstructL, the latter will permit
	// Server Logging.

	//server->StartL(KServerName); 
	server->ConstructL(KServerName);	// from CTestBase
	
	CleanupStack::Pop(server);
	return server;
	}


/*virtual*/ CIperfTestServer::~CIperfTestServer()
	{
	iWorkers.ResetAndDestroy();
	if (iSockServ)
		{
		iSockServ->Close();
		delete iSockServ;
		}
	}


// EKA2 much simpler
// Just an E32Main and a MainL()
LOCAL_C void MainL()
/**
 * Much simpler, uses the new Rendezvous() call to sync with the client
 */
	{
	// Leave the hooks in for platform security
#if (defined __DATA_CAGING__)
	RProcess().DataCaging(RProcess::EDataCagingOn);
	RProcess().SecureApi(RProcess::ESecureApiOn);
#endif
	CActiveScheduler* sched=NULL;
	sched=new(ELeave) CActiveScheduler;
	CActiveScheduler::Install(sched);
	CIperfTestServer* server = NULL;
	// Create the CTestServer derived server
	TRAPD(err,server = CIperfTestServer::NewL());
	if (!err)
		{
		// Sync with the client and enter the active scheduler
		RProcess::Rendezvous(KErrNone);
		sched->Start();
		}
	delete server;
	delete sched;
	}

// Only a DLL on emulator for typhoon and earlier

GLDEF_C TInt E32Main()
/**
 * @return - Standard Epoc error code on exit
 */
	{
	CTrapCleanup* cleanup = CTrapCleanup::New();
	if (cleanup == NULL)
		{
		return KErrNoMemory;
		}
	TRAP_IGNORE(MainL());
	delete cleanup;
	return KErrNone;
    }

// Create a thread in the calling process
// Emulator typhoon and earlier

CTestStep* CIperfTestServer::CreateTestStep(const TDesC& aStepName)
/**
 * @return - A CTestStep derived instance
 * Implementation of CTestServer pure virtual
 */
	{
	CTestStep* testStep = NULL;

	if (aStepName == KTestSetupReceiver)
		testStep = new CIperfTestSetupReceiver(this);
	else if (aStepName == KTestSetupSender)
		testStep = new CIperfTestSetupSender(this);
	else if (aStepName == KTestSetupCpuSponge)
		testStep = new CIperfTestSetupCpuSponge(this);
	else if (aStepName == KTestStart)
		testStep = new CIperfTestStart(this);
	else if (aStepName == KTestStop)
		testStep = new CIperfTestStop(this);
	else if (aStepName == KTestReport)
		testStep = new CIperfTestReport(this);
	else if (aStepName == KTestDestroy)
		testStep = new CIperfTestDestroy(this);
	else if (aStepName == KTestStartProfiler)
		testStep = new CIperfStartProfile(this);
	else if (aStepName == KTestStopProfiler)
		testStep = new CIperfStopProfile(this);

	return testStep;
	}



RSocketServ& CIperfTestServer::SockServL()
	{
	if (!iSockServ)
		{
		iSockServ=new(ELeave)RSocketServ;
#ifndef SUBOPTIMAL_APP
   		TSessionPref pref;
   		pref.iAddrFamily = KAfInet;
   		pref.iProtocol = KProtocolInetUdp;
   		TInt err = iSockServ->Connect(pref);
#else
		TInt err = iSockServ->Connect();
#endif
		if(err != KErrNone)
			{
			RDebug::Printf("NETPERF: connecting to socket server failed with %d", err);
			}
		User::LeaveIfError(err);

		User::LeaveIfError(iSockServ->ShareAuto());
		}
	return *iSockServ;
	}



//===========================================================================


// runs in owner thread
/*virtual*/ CTestWorker::~CTestWorker()
	{
	}


// RUNS IN WORKER THREAD
/*static*/ TInt CTestWorker::WorkerThreadEntryPoint(TAny* iArgs)
	{
	// Need a cleanup stack
	CTrapCleanup* cleanupStack = CTrapCleanup::New();

	CTestWorker* inst = static_cast<CTestWorker*>(iArgs);
	
	TRAPD(error, inst->RunWorkerL());

	// signal the initiator of the action that there was trouble
	//  .. on startup
	RThread::Rendezvous(error);
	
	//  .. or later
	TRAP_IGNORE(inst->SignalOwnerThreadL(error));

	// Remove our cleanup stack
	delete cleanupStack;

	return error;
	}

// RUNS IN WORKER THREAD
void CTestWorker::SignalOwnerThreadL(TInt aCode)
	{
	User::LeaveIfError(RProperty::Set(TUid::Uid(KIperfServerUid), reinterpret_cast<TUint32>(this)+1,aCode));
	}

// runs in owner thread
void CTestWorker::SignalWorkerThreadL(TInt aCode)
	{
	User::LeaveIfError(RProperty::Set(TUid::Uid(KIperfServerUid), reinterpret_cast<TUint32>(this),aCode));
	}

// RUNS IN WORKER THREAD
void CTestWorker::RunWorkerL()
	{
	// Create and install the active scheduler we know we will need
	CActiveScheduler* scheduler = new(ELeave) CActiveScheduler;
	CleanupStack::PushL(scheduler);
	CActiveScheduler::Install(scheduler);

	PrepareL();

	// Create the controller active object.. it listens for commands
	//  over pub/sub
	iControl = CTestWorkerControl::NewL(this);
	CleanupStack::PushL(iControl); // member data shouldn't *usually* be put
									// on the cleanup stack :-)
									// but the active objects need some
									// way to access the test worker control object
	
	// Start the active scheduler which will manage notification requests
	CActiveScheduler::Start();

	// Cleanup
	CleanupStack::PopAndDestroy(iControl);
	CleanupStack::PopAndDestroy(scheduler);
	}

// RUNS IN WORKER THREAD
void CTestWorker::CommandCompletedL(TInt aResult)
	{
	iCommandIsComplete=ETrue;
	iControl->CommandCompletedL(aResult);
	}

// RUNS IN WORKER THREAD
TInt CTestWorker::GetQueuedCommand()
	{
	return iControl->GetQueuedCommand();
	}

static const TInt KWorkerThreadStackSize = (32 * 1024);

void CTestWorker::SpawnWorkerThreadL(TThreadPriority aPriority,TName aThreadName)
	{
	User::LeaveIfError(iWorkerThread.Create(
		aThreadName,
		CTestWorker::WorkerThreadEntryPoint,
		KWorkerThreadStackSize,
		&User::Heap(),
		static_cast<TAny*>(this)));

	iWorkerThread.SetPriority(aPriority);
	
	// set up a rendezvous to see when worker is ready...
	TRequestStatus req;
	iWorkerThread.Rendezvous(req);

	// Get the thread started
	iWorkerThread.Resume();

	INFO_PRINTF1(_L("Awaiting worker thread startup.."));

	// wait for the signal..
	User::WaitForRequest(req);

	INFO_PRINTF1(_L("Worker thread started."));

	CheckForWorkerThreadDeathL();
	}


void CTestWorker::CheckForWorkerThreadDeathL()
	{
	TInt error = iWorkerThread.ExitReason();
	if (error)
		{
		INFO_PRINTF2(_L("Worker already died, error %d"),error);
		User::Leave(error);
		}
	
	TExitType et = iWorkerThread.ExitType();
	if (et != EExitPending)
		{
		INFO_PRINTF2(_L("Worker already died, reason %d"),et);
		User::Leave(KErrDied);
		}
	}	

void CTestWorker::SendCommandL(TUint aCommand)
	{
	CheckForWorkerThreadDeathL();

	TInt result = RProperty::Define(TUid::Uid(KIperfServerUid), reinterpret_cast<TUint32>(this)+1, RProperty::EInt);
	if (result != KErrAlreadyExists)
		{
		User::LeaveIfError(result);
		}

	RProperty prop;
	User::LeaveIfError(prop.Attach(TUid::Uid(KIperfServerUid), reinterpret_cast<TUint32>(this)+1));
	TRequestStatus propertyReq;
	prop.Subscribe(propertyReq);
	
	SignalWorkerThreadL(aCommand);

	TRequestStatus logonReq;
	iWorkerThread.Logon(logonReq);

	User::WaitForRequest(propertyReq, logonReq);
	
	if (propertyReq == KRequestPending)
		{
		// Worker thread terminated
		prop.Cancel();
		User::WaitForRequest(propertyReq);
		CheckForWorkerThreadDeathL();
		return;
		}

	// Property
	iWorkerThread.LogonCancel(logonReq);
	User::WaitForRequest(logonReq);

	result = propertyReq.Int();
	if (result==KErrNone)
		{
		prop.Get(result);
		}

	User::LeaveIfError(result);
	}


void CTestWorker::ShutdownWorkerThreadL()
	{
	// Send command to the thread managing the notification that we want it to die
	// this will then wait upon the thread to complete the command
	TRAPD(err, SendCommandL(KTestWorkerDestroy));
	if (err!=KErrDied)
		{
		User::LeaveIfError(err);
		}

	// Now wait for the thread to complete
	TRequestStatus logonReq;
	iWorkerThread.Logon(logonReq);
	User::WaitForRequest(logonReq);

	iWorkerThread.Close();
	
	User::LeaveIfError(logonReq.Int());
	}



//===========================================================================


/*static*/ CTestWorkerControl* CTestWorkerControl::NewL(CTestWorker* aTestWorker)
	{
	CTestWorkerControl* newInst = new (ELeave) CTestWorkerControl(aTestWorker);
	CleanupStack::PushL(newInst);
	newInst->ConstructL();
	CleanupStack::Pop();
	return newInst;
	}


void CTestWorkerControl::ConstructL()
	{
	CActiveScheduler::Add(this);

	TInt result = RProperty::Define(TUid::Uid(KIperfServerUid), reinterpret_cast<TUint32>(iTestWorker), RProperty::EInt);
	if (result == KErrNone || result == KErrAlreadyExists)
		{
		result = iProperty.Attach(TUid::Uid(KIperfServerUid), reinterpret_cast<TUint32>(iTestWorker));
		}
	if (result != KErrNone && result != KErrAlreadyExists)
		{
		User::Panic(_L("CTestWorkerControl::ConstructL"), result);
		}
	
	iProperty.Subscribe(iStatus);
	SetActive();

	// tell caller we're ready..
	RThread::Rendezvous(KErrNone);
	}


void CTestWorkerControl::RunL()
	{
	TInt propertyValue;
	TInt result = iProperty.Get(propertyValue);
	if (result != KErrNone)
		{
		User::Panic(_L("CTestWorkerControl::RunL"), result);
		}

	switch(propertyValue)
		{
		case KTestWorkerStart:
			TRAP(result,iTestWorker->StartL());
			break;
		case KTestWorkerStop:
			TRAP(result,iTestWorker->StopL());
			break;
		case KTestWorkerReport:
			TRAP(result,iTestWorker->ReportL());
			break;
		case KTestWorkerDestroy:
			iTestWorker->Destroy();
			CActiveScheduler::Stop();
			return;
		default:
			User::Panic(_L("CTestWorkerControl::RunL"), propertyValue);
		}

	if (iTestWorker->IsCommandComplete())
		{
		CommandCompletedL(result);
		}
	}

void CTestWorkerControl::CommandCompletedL(TInt aResult)
	{
	// repeat subscribe..
	iProperty.Subscribe(iStatus);
	SetActive();
	
	// now signal the calling thread (the TEF test step) that
	//  this command has finished and it should continue running the script
	iTestWorker->SignalOwnerThreadL(aResult);
	}

TInt CTestWorkerControl::GetQueuedCommand()
	{
	TInt commandCode=0;
	if (iStatus.Int() != KRequestPending)
		{
		// somebody sent a command. let's sneak a look at it..
		iProperty.Get(commandCode);
		}
	return commandCode;
	}

void CTestWorkerControl::DoCancel()
	{
	iProperty.Cancel();
	}


//===========================================================================


/*virtual*/ CIperfTestWorker::~CIperfTestWorker()
	{
	delete iCollectedPacketCountSamples;
	delete iCollectedByteCountSamples;
	delete iCollectedTimerSamples;
	delete iJitterTimestampBuffer;
	}

/*virtual*/ void CIperfTestWorker::PrepareL()
	{
	if (iCollectedTimerSamples)
		{
		User::LeaveIfError(KErrInUse);
		}

	iCollectedTimerSamples = new(ELeave) TUint32[GetMaxCollectedSamples()];	
	iCollectedPacketCountSamples = new(ELeave) TUint32[GetMaxCollectedSamples()];
	iCollectedByteCountSamples = new(ELeave) TUint32[GetMaxCollectedSamples()];
	
	// startup timer
	HAL::Get(HALData::EFastCounterCountsUp,iFastCounterCountsUp);
	iFastCounterBeforeConnectionStart = iFastCounterCountsUp?User::FastCounter():(~User::FastCounter());

	TInt ret = iConnection.Open(iIperfTestServer.SockServL());
	User::LeaveIfError(ret);

	if (iIap > 0)
		{
		TCommDbConnPref pref;
		pref.SetIapId(iIap);
		pref.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);
		User::LeaveIfError(iConnection.Start(pref));
		}
#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
	else if (iSnap > 0)
		{
		TConnSnapPref pref(iSnap);
		User::LeaveIfError(iConnection.Start(pref));
		}
#endif // SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
	else
		{
		User::LeaveIfError(iConnection.Start());
		}
	// startup timer
	iFastCounterAfterConnectionStart = iFastCounterCountsUp?User::FastCounter():(~User::FastCounter());
	
	User::LeaveIfError(iSocket.Open(iIperfTestServer.SockServL(),
									KAfInet,
									iTcpMode?KSockStream:KSockDatagram,
									iTcpMode?KProtocolInetTcp:KProtocolInetUdp,
									iConnection));
	}

/*virtual*/ void CIperfTestWorker::StartL()
	{
	if (iRunning)
		{
		User::Leave(KErrInUse);
		}
	iRunning=ETrue;
	
	iCurrentDataIndex = 0;
	}

/*virtual*/ void CIperfTestWorker::StopL()
	{
	if (! iRunning)
		{
		User::Leave(KErrNotReady);
		}
	iRunning=EFalse;
	
	}

RSocketServ& CIperfTestWorker::SockServL()
	{
	return iIperfTestServer.SockServL();
	}

/*virtual*/ void CIperfTestWorker::Destroy()
	{
	iSocket.Close();
	iConnection.Close();
	}


	void CIperfTestWorker::SetUpJitterBufferL()
		{
		TInt numberOfPacketsToBeSent =
			iTestDuration.Int() * iSendThroughput_kbps*(1000/8) / iPacketSizeInBytes;

		TInt firstPacketOnWhichToSampleJitter=numberOfPacketsToBeSent/4;
		TInt nsamp=firstPacketOnWhichToSampleJitter*2;
		if(nsamp>1000)
			{
			nsamp=1000;
			}
		NETPERF_TRACE_4(_L("npackets %d, first to sample %d, last to sample %d"),
			numberOfPacketsToBeSent ,firstPacketOnWhichToSampleJitter, firstPacketOnWhichToSampleJitter+nsamp);

		iJitterStartPacket=firstPacketOnWhichToSampleJitter;
		iJitterRecordingFinished=EFalse;
		iJitterTimestampBuffer = new (ELeave) TUint32[nsamp];
		iJitterTimestamp = NULL;
		iJitterLastTimestampInBuffer = &iJitterTimestampBuffer[nsamp-1];

		NETPERF_TRACE_3(_L("iJitterTimestampBuffer %08x, iJitterLastTimestampInBuffer %08x"),iJitterTimestampBuffer, iJitterLastTimestampInBuffer);
		}
	
	TBool CIperfTestWorker::RecordingJitter()
		{
		if(iPacketCount == iJitterStartPacket)
			{
			iJitterTimestamp = iJitterTimestampBuffer;
			}
		else if(iJitterRecordingFinished || !iJitterTimestamp)
			{
			return EFalse;
			}
		return ETrue;
		}

	void CIperfTestWorker::RecordJitter()
		{
		*iJitterTimestamp = User::FastCounter();
		if(iJitterTimestamp == iJitterLastTimestampInBuffer)
			{
			iJitterRecordingFinished=ETrue;
			}
		else
			{
			iJitterTimestamp++;
			}
		}


/*virtual*/ void CIperfTestWorker::ReportL()
	{
	TInt fcf;
	HAL::Get(HALData::EFastCounterFrequency,fcf);
	TReal fastCounterFreq = fcf;

	TInt thisTimerDiff = MRunningFastCounter::CalculateFastCounterDiff(iFastCounterBeforeConnectionStart,iFastCounterAfterConnectionStart);
	TReal timeTaken = thisTimerDiff;
	timeTaken /=  fastCounterFreq;
	
	Logger().LogExtra(((TText8*)__FILE__), __LINE__, ESevrInfo,
					_L("Summary statistics begin"));
	Logger().LogExtra(((TText8*)__FILE__), __LINE__, ESevrInfo,
					_L("Connection Startup took %.5fs"),
						timeTaken);
	Logger().LogExtra(((TText8*)__FILE__), __LINE__, ESevrInfo,
					_L("Total client packets transferred: %d"),
						iPacketCount);
	Logger().LogExtra(((TText8*)__FILE__), __LINE__, ESevrInfo,
					_L("Summary statistics end"));


	NETPERF_TRACE_3(_L("iJitterTimestamp %08x, iJitterTimestampBuffer %08x"),iJitterTimestamp, iJitterTimestampBuffer);

	if(iJitterTimestamp && iJitterTimestamp!=iJitterTimestampBuffer)
		{
		// report jitter data
		TUint32* lastToReport=iJitterTimestamp;
		TUint32* thisTimestamp=iJitterTimestampBuffer;
	
		Logger().LogExtra(((TText8*)__FILE__), __LINE__, ESevrInfo,
						_L("Jitter timings in microseconds begin"));
	
		TInt previousTimestamp=*thisTimestamp;
		++thisTimestamp;
		TInt count=1;
		RBuf thisLine;
		thisLine.CreateL(150); // enough for 10 numbers
		TDes16MyIgnoreOverflow ignoreOverflow;
		for ( ; thisTimestamp<=lastToReport ; ++thisTimestamp,++count )
			{
			TInt fcDiff = *thisTimestamp - previousTimestamp;
			TReal secDiff = fcDiff;
			secDiff /= fastCounterFreq;
			secDiff *= 1e6; // in microseconds
			
			thisLine.AppendFormat(_L("%.1f "), &ignoreOverflow, secDiff);

			if(count%10 == 0)
				{
				Logger().LogExtra(((TText8*)__FILE__), __LINE__, ESevrInfo,
							thisLine);
				thisLine.SetLength(0);
				}

			previousTimestamp=*thisTimestamp;
			}
		if(thisLine.Length())
			{
			Logger().LogExtra(((TText8*)__FILE__), __LINE__, ESevrInfo,
				thisLine);
			}
		thisLine.Close();		

		Logger().LogExtra(((TText8*)__FILE__), __LINE__, ESevrInfo,
						_L("Jitter timings in microseconds end"));
	
		}		
	}

//===========================================================================


MRunningFastCounter::MRunningFastCounter()
	{
	TInt fcf;
	HAL::Get(HALData::EFastCounterFrequency,fcf);
	iFastCounterFrequency=fcf;
	HAL::Get(HALData::EFastCounterCountsUp,iFastCounterCountsUp);
	iRunningFastCounterValue = 0;

	iLastFastCounterValue = iFastCounterCountsUp?User::FastCounter():(~User::FastCounter());

	// to be set by owner : iReportPeriodInFastCounterUnits, iNextReportRunningFastCounterValue
	
	}


void MRunningFastCounter::StartRunningFastCounter()
	{
	iRunningFastCounterValue = 0;
	iLastFastCounterValue = iFastCounterCountsUp?User::FastCounter():(~User::FastCounter());
	iNextReportRunningFastCounterValue = iReportPeriodInFastCounterUnits;
	}

void MRunningFastCounter::UpdateRunningFastCounter()
	{
	TInt currentFastCounterValue = iFastCounterCountsUp?User::FastCounter():(~User::FastCounter());
	TUint fastCounterDelta = CalculateFastCounterDiff(iLastFastCounterValue,currentFastCounterValue);
	iRunningFastCounterValue += fastCounterDelta;
	NETPERF_TRACE_3(_L("current RFC: %Ld, current FC: %d"),iRunningFastCounterValue,currentFastCounterValue);
	iLastFastCounterValue = currentFastCounterValue;	
	}



//===========================================================================

