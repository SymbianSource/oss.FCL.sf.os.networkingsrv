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
// Netperf's packet receiver. Receives UDP/TCP from remote iperf, records
// metrics and reports them.
// Based on CIperfTestWorker from netperfserver.h
// 
//

/**
 @file
 @internalTechnology
*/

#include <e32property.h>
#include <hal.h>
#include <es_sock.h>
#ifdef SYMBIAN_OLD_EXPORT_LOCATION
#include <networking/netperftrace.h>
#else
#include "netperftrace.h"
#endif
#include "netperfreceiver.h"

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


CReader::CReader(RSocket& aSock, CIperfReceiver& aOwner)
	: CActive(CActive::EPriorityStandard),
	iSocket(aSock),
	iNumReads(0),
	iTotSize(0),
	iLastSeqNum(0),
	iPacketsLost(0),
	iPacketsOutOfOrder(0),
	iOwner(aOwner),
	iEchoReceivedData(aOwner.GetEchoReceivedData())
#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
	,iDelayRecorder(NULL)
#endif
	{
	}


/*static*/ CReader* CReader::NewL(RSocket& aSock, CIperfReceiver& aOwner)
	{
	CReader* self = new(ELeave) CReader(aSock,aOwner);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	

CReader::~CReader()
	{
	iTcpReadingSock.Close();
#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
	delete iDelayRecorder;
#endif
	}
	


void CReader::ConstructL()
	{
	CActiveScheduler::Add(this);

	TSockAddr listenAddr;
	listenAddr.SetPort(iOwner.GetReceivePort());
	User::LeaveIfError(iSocket.Bind(listenAddr));

	User::LeaveIfError(iBuf.CreateMax(iOwner.GetPacketSizeInBytes()));
#ifndef SUBOPTIMAL_APP
	User::LeaveIfError(iSocket.SetOpt(KSORecvBuf, KSOLSocket, iOwner.GetPacketSizeInBytes()));	// aim for a single cli/srv trip for the data copy
#endif	
	if (iOwner.GetTcpMode())
		{
		iSocket.Listen(5);
		User::LeaveIfError(iTcpReadingSock.Open(iOwner.SockServL()));
		iSocket.Accept(iTcpReadingSock, iStatus);
		iNumReads=-1;
		}
	else
		{
		iSocket.Recv(iBuf, 0, iStatus);
		}
	SetActive();

	TInt64 tmp = iFastCounterFrequency;
	tmp *= iOwner.GetSamplingPeriod_us();
	iReportPeriodInFastCounterUnits = tmp / 1e6 ;
	StartRunningFastCounter();
 	
	iOwner.SetUpJitterBufferL();
	
#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
	// Set up delay meter recorder:
	TDelayMeterPrefs delayPrefs;
	delayPrefs.iSamplingPeriodInMilliseconds=iOwner.GetSamplingPeriod_ms();
	delayPrefs.iTestDurationInSeconds=iOwner.GetTestDuration_sec();
	delayPrefs.iProtocol=iOwner.GetTcpMode() ? KProtocolInetTcp : KProtocolInetUdp ;
	delayPrefs.iDestinationPort=iOwner.GetReceivePort();
	delayPrefs.iTimestampSpacing=iOwner.GetPacketSizeInBytes();
	
	TRAPD(result, iDelayRecorder = CReceiveRecorder::NewL(delayPrefs));
	if (result!=KErrNotFound) // swallow this error - it means the delay protocol wasn't there
		{
		User::LeaveIfError(result);
		}
#endif // SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY	

	NETPERF_TRACE_1(_L("set up ok"));
	}
	
void CReader::RunL()
	{
	NETPERF_TRACE_1(_L("CReader::RunL"));

	if(iOwner.RecordingJitter())
		{
		iOwner.RecordJitter();
		}
	iOwner.IncrementPacketCount();
	
	if (iStatus.Int()==KErrEof)
		{
		// tcp sending client closed connection.. so just sleep..
		iOwner.RecordCountAndTime();
		return;		
		}
	User::LeaveIfError(iStatus.Int());

	if (iNumReads == -1)
		{
		// client just connected via tcp...
		// so no buffer to read.
		}
	else
		{
		TInt receivedSeqNum = BigEndian::Get32(iBuf.Ptr());
		if (receivedSeqNum >= iLastSeqNum + 1) // next in sequence or missing packets
			{
			if(receivedSeqNum-iLastSeqNum-1 > 1e6)
				{
				NETPERF_TRACE_4(_L("Received unexpected sequence number 0x%08x (%d)- ignoring (expected %d)"), receivedSeqNum,receivedSeqNum,iLastSeqNum+1);
				}
			else
				{
				iPacketsLost += (receivedSeqNum-iLastSeqNum-1);
				iLastSeqNum = receivedSeqNum;
				}
			}
		else // out of sequence or same packet again (??!)
			{
			++iPacketsOutOfOrder;
			}

#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
		if (iDelayRecorder)
			{
			iDelayRecorder->RecordDelay(iBuf);
			}
#endif
		
#if _NETPERF_TRACE
		if (iBuf.Length() != iOwner.GetPacketSizeInBytes() )
			NETPERF_TRACE_3(_L("Received %d bytes (but expected %d"), iBuf.Length(),iOwner.GetPacketSizeInBytes() );
#endif
		iTotSize += iBuf.Length();
		}

	++iNumReads;

	if (iOwner.GetTcpMode())
		{
#ifndef SUBOPTIMAL_APP
		iTcpReadingSock.Recv(iBuf, 0, iStatus);
#else
		iTcpReadingSock.RecvOneOrMore(iBuf, 0, iStatus, iXfrLen);
#endif
		}
	else // less client calls if we use the big chunks function
		{
		iSocket.Recv(iBuf, 0, iStatus);
		}

	
	UpdateRunningFastCounter();	
	
	if (iRunningFastCounterValue > iNextReportRunningFastCounterValue)
		{
		while (iRunningFastCounterValue > iNextReportRunningFastCounterValue)
			{
			iNextReportRunningFastCounterValue += iReportPeriodInFastCounterUnits;
			}
		iOwner.RecordCountAndTime();
		}

	SetActive();
	}
	
void CReader::DoCancel()
	{
	iSocket.CancelRead();
	}



//===========================================================================



/*virtual*/ CIperfReceiver::~CIperfReceiver()
	{
	delete[] iPacketsLost;
	delete[] iPacketsOutOfOrder;
	}

/*virtual*/ void CIperfReceiver::Destroy()
	{
	iReader->Cancel();
	delete iReader;
	iReader = NULL;
	ParentClass::Destroy();
	}

/*virtual*/ void CIperfReceiver::PrepareL()
	{
	ParentClass::PrepareL();
	
	iPacketsLost = new(ELeave) TUint32[GetMaxCollectedSamples()];
	iPacketsOutOfOrder = new(ELeave) TUint32[GetMaxCollectedSamples()];

	if (iReader)
		{
		User::LeaveIfError(KErrInUse);
		}
	iReader = CReader::NewL(iSocket,*this);
	}

void CIperfReceiver::RecordCountAndTime()
	{
	if (iCurrentDataIndex < GetMaxCollectedSamples()) // shouldn't overflow but just in case..
		{
		iCollectedPacketCountSamples[iCurrentDataIndex] = iReader->iNumReads;
		iCollectedByteCountSamples[iCurrentDataIndex] = iReader->iTotSize;
		iPacketsLost[iCurrentDataIndex] = iReader->iPacketsLost;
		iPacketsOutOfOrder[iCurrentDataIndex] = iReader->iPacketsOutOfOrder;
		iCollectedTimerSamples[iCurrentDataIndex] = iFastCounterCountsUp?User::FastCounter():(~User::FastCounter());
		++iCurrentDataIndex;
		}
	User::ResetInactivityTime(); // stop the device going to sleep!
	}

/*virtual*/ void CIperfReceiver::ReportL()
	{
	ParentClass::ReportL();

	TInt fcf;
	HAL::Get(HALData::EFastCounterFrequency,fcf);
	TReal32 fastCounterFreq = fcf;

	INFO_PRINTF2(_L("Fast counter frequency: %d"),fcf);

	
#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
	TInt numDelayResults=0;
	TDelaySampleSet* delayResults=NULL;
	if (iReader && iReader->iDelayRecorder)
		{
		iReader->iDelayRecorder->GetResultsL(numDelayResults, delayResults);
		Logger().LogExtra(((TText8*)__FILE__), __LINE__, ESevrInfo,
							_L("num delay results %d"),numDelayResults);

		}
#endif	
	
	
#ifndef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
	INFO_PRINTF1(_L("Measurements at bottom of stack not supported on this release. Unsupported columns (stackMeanDelay.. etc.) will be omitted."));
#endif


	INFO_PRINTF1(_L("T   - time(s)"));
	INFO_PRINTF1(_L("dT  - timeDelta(s)"));
	INFO_PRINTF1(_L("Tp  - receivedFromStackDelta(pkts)"));
	INFO_PRINTF1(_L("Tb  - receivedFromStackDelta(bytes)"));
	INFO_PRINTF1(_L("Tr  - receivedFromStackBitrate(kbps)"));
	INFO_PRINTF1(_L("TLp - lostAtTopOfStack(pkts)"));
	INFO_PRINTF1(_L("TOp - outOfOrderAtTopOfStack(pkts)"));
#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
	if (numDelayResults)
		{
		INFO_PRINTF1(_L("Bp  - receivedByStackDelta(pkts)"));
		INFO_PRINTF1(_L("BLp - lostAtBottomOfStack(pkts)"));
		INFO_PRINTF1(_L("D-  - stackMinDelay(usec)"));
		INFO_PRINTF1(_L("Dm  - stackMeanDelay(usec)"));
		INFO_PRINTF1(_L("D+  - stackMaxDelay(usec)"));
		}
#endif
	
	INFO_PRINTF1(_L("Received packets data begin"));

#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
	if (numDelayResults)
		{
		INFO_PRINTF1(_L("T  ,  dT  ,  Tp  ,  Tb  ,  Tr  ,  TLp  ,  TOp  ,  Bp  ,  BLp  ,  D-  ,  Dm  ,  D+"));
		}
	else
#endif
		{
		INFO_PRINTF1(_L("T  ,  dT  ,  Tp  ,  Tb  ,  Tr  ,  TLp  ,  TOp"));
		}
	
	TInt64 runningFastCounter = 0;
	for (TInt top_i=1,bottom_i=0 ; top_i<iCurrentDataIndex ; ++top_i,++bottom_i)
		{
		TInt thisTimerDiff = MRunningFastCounter::CalculateFastCounterDiff(iCollectedTimerSamples[top_i-1],iCollectedTimerSamples[top_i]);
		runningFastCounter += thisTimerDiff;
		TReal timerReal = runningFastCounter; 
		timerReal /= fastCounterFreq;
		TReal timeDiffReal = thisTimerDiff; 
		timeDiffReal /= fastCounterFreq;

		TInt byteDiff=iCollectedByteCountSamples[top_i]-iCollectedByteCountSamples[top_i-1];
		TReal rate = byteDiff;
		rate /= timeDiffReal;
		const TReal bits_per_byte = 8.0;
		rate *= bits_per_byte; // rate is now in bits per second
		const TReal bits_per_kbit = 1000.0;
		rate /= bits_per_kbit; // rate is now in kbits per second

#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
		if (bottom_i<numDelayResults)
			{
			TDelaySampleSet& delayResult = delayResults[bottom_i];
			
			TInt receivedByStackDelta = delayResult.GetNumberOfPackets();
			TInt lostAtBottomOfStack = delayResult.GetNumberOfPacketsLost();
			TReal stackMinDelay,stackMeanDelay,stackMaxDelay;
			delayResult.GetTimeValues(fastCounterFreq, stackMinDelay,stackMeanDelay,stackMaxDelay);
			stackMinDelay*=1e6;		// to get in microsec
			stackMeanDelay*=1e6;	// to get in microsec
			stackMaxDelay*=1e6;		// to get in microsec
			
			Logger().LogExtra(((TText8*)__FILE__), __LINE__, ESevrInfo,
					_L("%.2f, %.5f, %d, %d, %.5f,  %d, %d,  %d, %d,  %.5f, %.5f, %.5f"), 
						timerReal,
						timeDiffReal,
						
						iCollectedPacketCountSamples[top_i]-iCollectedPacketCountSamples[top_i-1],
						byteDiff,
						rate,
						
						iPacketsLost[top_i]-iPacketsLost[top_i-1],
						iPacketsOutOfOrder[top_i]-iPacketsOutOfOrder[top_i-1],

						receivedByStackDelta,
						lostAtBottomOfStack,
						
						stackMinDelay,stackMeanDelay,stackMaxDelay);
			}
		else
#endif
			{
			Logger().LogExtra(((TText8*)__FILE__), __LINE__, ESevrInfo,
						_L("%.2f, %.5f,  %d, %d, %.5f,  %d, %d"),
							timerReal,
							timeDiffReal,
							iCollectedPacketCountSamples[top_i]-iCollectedPacketCountSamples[top_i-1],
							byteDiff,
							rate,
							iPacketsLost[top_i]-iPacketsLost[top_i-1],
							iPacketsOutOfOrder[top_i]-iPacketsOutOfOrder[top_i-1] );
			}
		}
	
	INFO_PRINTF1(_L("Received packets data end"));
	}


//===========================================================================

