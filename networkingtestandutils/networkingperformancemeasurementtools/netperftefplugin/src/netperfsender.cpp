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
// Netperf's packet sender. Sends UDP/TCP to remote iperf, records
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
#include "netperfsender.h"

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


/*static*/ CWriter* CWriter::NewL(RSocket& aSock,CIperfSender& aOwner, TInt aPacketSizeInBytes, TInt aSendThroughput_kbps, 
		TSockAddr& aSendTo, TBool aUseLowerLayerPacketGenerator)
	{
	CWriter* self = new(ELeave) CWriter(aSock,aOwner,aPacketSizeInBytes,aSendThroughput_kbps,aSendTo,aUseLowerLayerPacketGenerator);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	
CWriter::~CWriter()
	{
	Cancel();
#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
	delete iDelayTimestamper;
#endif
	}



CTestExecuteLogger& CWriter::Logger()
	{
	return iOwner.Logger();
	}

	
void CWriter::ConstructL()
	{
	CTimer::ConstructL();
	CActiveScheduler::Add(this);

	TInt64 tmp = iFastCounterFrequency;
	tmp *= iOwner.GetSamplingPeriod_us();
	iReportPeriodInFastCounterUnits = tmp / 1e6 ;

	iBuf.SetMax();
	iPacket.Set(iBuf.MidTPtr(0,iPacketSizeInBytes));
	User::LeaveIfError(iIperfProtocol.AssembleIperfHeader(iPacket, iSendTo.Port(), iSendThroughput_kbps));

	iOwner.SetUpJitterBufferL();

#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
	// Set up delay meter:
	TDelayMeterPrefs delayPrefs;
	delayPrefs.iSamplingPeriodInMilliseconds=iOwner.GetSamplingPeriod_ms();
	delayPrefs.iTestDurationInSeconds=iOwner.GetTestDuration_sec();
	delayPrefs.iProtocol=iOwner.GetTcpMode()?KProtocolInetTcp:KProtocolInetUdp; // udp or tcp protocol numbers
	delayPrefs.iDestinationPort=iSendTo.Port();
	delayPrefs.iTimestampSpacing=iOwner.GetPacketSizeInBytes();

	TRAPD(result, iDelayTimestamper = CSendTimestamper::NewL(iSocket,delayPrefs));
	if (result!=KErrNotFound) // swallow this error - it means the delay protocol wasn't there
		{
		User::LeaveIfError(result);
		}

#endif // SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
	}

void CWriter::StartL()
	{
	NETPERF_TRACE_1(_L("Starting writer.."));

	StartRunningFastCounter();
	
	// try to connect 10 times over the next 5 seconds in case remote listener hasn't yet started up..
	TRequestStatus req(KErrCouldNotConnect);
	for (TInt i=0 ; req.Int()==KErrCouldNotConnect && i<10 ; ++i)
		{
		iSocket.Connect(iSendTo,req);
		User::WaitForRequest(req);
		if (req.Int()==KErrNone)
			{
			break;
			}
		NETPERF_TRACE_2(_L("Could not connect (error %d).. Retrying.."),req.Int());
		User::After(5e5);
		}
	User::LeaveIfError(req.Int());
#ifndef SUBOPTIMAL_APP
	User::LeaveIfError(iSocket.SetOpt(KSOSendBuf, KSOLSocket, iPacketSizeInBytes));	// aim for a single cli/srv trip for the data copy  
#endif

	HighRes(1/*microsec*/); // this allows the StartL call to complete (so the calling TEF script can continue).
								// when the timer fires, our RunL can take over the thread and poll for received Stops
	}

/*virtual*/
void CWriter::RunL()
	{
	if (iUseLowerLayerPacketGenerator) // to seed the packet generator protocol
		{
		SendPackets(1);
		// and just sit here til stopped.
		}
	else if (iStopping)
		{
		SendAndReceiveFinalPacket();
		iOwner.RecordCountAndTime();
		}
	else
		{
		// reissue timer request immediately, so when a Stop command is received we'll jump back into this RunL and
		// run the above code to send final packet and record final measurement.
		HighRes(1/*microsec*/);
		SendTakingOverThread();
		}
	}

void CWriter::MarkForStop()
	{
	iStopping=ETrue;
	}

void CWriter::Send()
	{
	}

void CWriter::SendTakingOverThread()
	{
	TReal packetsPerSecond = (iSendThroughput_kbps * 1000.00 );
	packetsPerSecond /= iPacketSizeInBytes * 8.0;
	TReal temp = iFastCounterFrequency;
	temp /= packetsPerSecond;
	TInt32 fcUnitsPerPacket = temp;
	NETPERF_TRACE_2(_L("Specified throughput %dkbps"),iSendThroughput_kbps);
	NETPERF_TRACE_2(_L("Packet size %d bytes"),iPacketSizeInBytes);
	NETPERF_TRACE_2(_L("Therefore, packets per second to be sent: %f"),packetsPerSecond);
	NETPERF_TRACE_2(_L("Fastcounter frequency %d ticks/sec"),iFastCounterFrequency);
	NETPERF_TRACE_2(_L("Therefore, fastcounter units per packet: %d"),fcUnitsPerPacket);

	TInt i=0;
	for ( ; ; ++i)
		{
		// SEND..
		UpdateRunningFastCounter();
		TInt64 numberOfPacketsThatShouldHaveBeenSent = (iRunningFastCounterValue / fcUnitsPerPacket)+1;
		NETPERF_TRACE_2(_L("numberOfPacketsThatShouldHaveBeenSent:%Ld"),numberOfPacketsThatShouldHaveBeenSent);
		TInt packetsToSend = numberOfPacketsThatShouldHaveBeenSent-iNumSentPackets;
		// send a maximum of 100 packets
		SendPackets(packetsToSend<100?packetsToSend:100);

		// MEASURE..
		UpdateRunningFastCounter();
		if (iRunningFastCounterValue > iNextReportRunningFastCounterValue)
			{
			TInt toAdd =	(
							  (iRunningFastCounterValue-iNextReportRunningFastCounterValue-1)
							   / iReportPeriodInFastCounterUnits +1
							) * iReportPeriodInFastCounterUnits;
			iNextReportRunningFastCounterValue += toAdd;

			iOwner.RecordCountAndTime();

			if (iOwner.GetQueuedCommand())
				{
				// stop hogging the thread! let the active scheduler deal with the incoming command.
				return;
				}
			}

		// .. AND WAIT
		if (numberOfPacketsThatShouldHaveBeenSent <= iNumSentPackets)
			{
			// if we have time to spare, delay a bit
			TInt64 nextPacketSendFastCounterValue = (numberOfPacketsThatShouldHaveBeenSent) * fcUnitsPerPacket;
			TInt64 us_toDelay = (nextPacketSendFastCounterValue - iRunningFastCounterValue);
			if(us_toDelay>0)
				{
				us_toDelay *= 1000000;
				us_toDelay /= iFastCounterFrequency;
				TInt32 us_toDelay_32 = us_toDelay;
				NETPERF_TRACE_3(_L("next Packet send due at RFC: %Ld\nso waiting %d us"),
							nextPacketSendFastCounterValue,us_toDelay_32);
				//NETPERF_TRACE_2(_L("that's %Ld RFC units."),us_toDelay);
				User::AfterHighRes(us_toDelay_32); // mmmm.. procedural
				}
			}
		// or if we're maxed out, LOOP HARD!!
		}
	}

TInt CWriter::SendPackets(TInt aNumPacketsToSend)
	{
	NETPERF_TRACE_2(_L("Sending %d packets.."),aNumPacketsToSend);

	TInt retcode;
	for (TInt i=0; i<aNumPacketsToSend; ++i)
		{
		iIperfProtocol.UpdateIperfHeader(
				iPacket,
				iStopping?TIperfUdpHeader::EFinishRequest:TIperfUdpHeader::EData,
				iNumSentPackets);


#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
		// record timestamp outgoing
		if (iDelayTimestamper)
			{
			iDelayTimestamper->Timestamp(iPacket);
			}
#endif
		
		if(iOwner.RecordingJitter())
			{
			iOwner.RecordJitter();
			}
		iOwner.IncrementPacketCount();
		
		TRequestStatus req;
		iSocket.Send(iPacket,0,req);
		User::WaitForRequest(req);
		retcode = req.Int();
		if (retcode)
			{
			NETPERF_TRACE_2(_L("Send failed, error %d. Panicking thread."),retcode);
			User::Panic(_L("CWriter::Send failed"), retcode);
			}
		++iNumSentPackets;
		iTotSize += iPacket.Length();
		}
	return KErrNone;
	}

void CWriter::SendAndReceiveFinalPacket()
	{
	TInt retcode = SendPackets(1); // creates the final packet...

	if (!retcode && !iOwner.GetTcpMode()) // final exchange is udp only
		{
		TRequestStatus recvRequest;

		iSocket.Recv(iBuf, 0, recvRequest);

		for (TInt i=0; i<10; ++i)
			{
			User::After(TTimeIntervalMicroSeconds32(1e6)); // 1 million
			if (recvRequest.Int() != KRequestPending || i==9)
				{
				// received packet or gave up
				break;
				}

			// otherwise try to send it again..
			TRequestStatus sendRequest;
			iSocket.Send(iPacket,0,sendRequest);
			User::WaitForRequest(sendRequest);
			retcode=sendRequest.Int();
			if (retcode)
				{
				break;
				}
			}
		// in case we gave up..
		iSocket.CancelRecv();
		User::WaitForRequest(recvRequest);
		
		if (!retcode)
			{
			retcode=recvRequest.Int();
			}

		if (!retcode)
			{
			// process the packet
			TIperfUdpHeader::TMsg aMsgType;
			TInt aReceivedSeqNum;
			iIperfProtocol.DisassembleIperf(iBuf, EClient, aMsgType,aReceivedSeqNum);

			// We receive the packet but don't extract the contained metrics.
			//  the contained metrics will be caught by the ExeService
			//  that runs Iperf on the test controller PC.
			}
		}

	iSocket.Close();
			
	TRAP(retcode,iOwner.CommandCompletedL(retcode));
	}


//===========================================================================


/*virtual*/ CIperfSender::~CIperfSender()
	{
	}

/*virtual*/ void CIperfSender::Destroy()
	{
	delete iWriter;
	iWriter = NULL;
	ParentClass::Destroy();
	}

/*virtual*/ void CIperfSender::PrepareL()
	{
	ParentClass::PrepareL();
	
	if (iWriter)
		{
		User::LeaveIfError(KErrInUse);
		}
	iWriter = CWriter::NewL(iSocket,*this, iPacketSizeInBytes, iSendThroughput_kbps, iSendToSockAddr, iUseLowerLayerPacketGenerator);
	}

/*virtual*/ void CIperfSender::StartL()
	{
	ParentClass::StartL();
	iWriter->StartL();
	}


/*virtual*/ void CIperfSender::StopL()
	{
	ParentClass::StopL();
	if (iUseLowerLayerPacketGenerator)
		{
		// was already stopped. have been sitting here getting bored.
		//  so tell packet gen to stop. and piffle to hanging around.
		//User::LeaveIfError(RProperty::Set(TUid::Uid(0x10272F4A), 1, 1));
		}
	else
		{
		iWriter->MarkForStop();
		DontCompleteCommandYet(); // as we need to return control to active scheduler to allow it
								  // to send final packet
		}
	}


void CIperfSender::RecordCountAndTime()
	{
	if (iCurrentDataIndex < GetMaxCollectedSamples()) // shouldn't overflow but just in case..
		{
		iCollectedPacketCountSamples[iCurrentDataIndex] = iWriter->iNumSentPackets;
		iCollectedByteCountSamples[iCurrentDataIndex] = iWriter->iTotSize;
		iCollectedTimerSamples[iCurrentDataIndex] = iFastCounterCountsUp?User::FastCounter():(~User::FastCounter());
		++iCurrentDataIndex;
		}
	User::ResetInactivityTime(); // stop the device going to sleep!
	}

/*virtual*/ void CIperfSender::ReportL()
	{
	ParentClass::ReportL();

	TInt fcf;
	HAL::Get(HALData::EFastCounterFrequency,fcf);
	TReal32 fastCounterFreq = fcf;

#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
	TInt numDelayResults=0;
	TDelaySampleSet* delayResults;
	if (iWriter && iWriter->iDelayTimestamper)
		{
		iWriter->iDelayTimestamper->GetResultsL(numDelayResults, delayResults);
		Logger().LogExtra(((TText8*)__FILE__), __LINE__, ESevrInfo,
							_L("num delay results %d"),numDelayResults);
		}
#endif
	
	INFO_PRINTF2(_L("Fast counter frequency: %d"),fcf);

#ifndef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
	INFO_PRINTF1(_L("Measurements at bottom of stack not supported on this release. Unsupported columns (sentOut.. etc.) will be omitted."));
#endif

	INFO_PRINTF1(_L("T   - time(s)"));
	INFO_PRINTF1(_L("dT  - timeDelta(s)"));
	INFO_PRINTF1(_L("Tp  - sentIntoStackDelta(pkts)"));
	INFO_PRINTF1(_L("Tb  - sentIntoStackDelta(bytes)"));
	INFO_PRINTF1(_L("Tr  - sentIntoStackBitrate(kbps)"));
	
#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
	if (numDelayResults)
		{
		INFO_PRINTF1(_L("Bp  - sentOutByStackDelta(pkts)"));
		INFO_PRINTF1(_L("BLp - packetsLostByStack(pkts)"));
		INFO_PRINTF1(_L("D-  - stackMinDelay(usec)"));
		INFO_PRINTF1(_L("Dm  - stackMeanDelay(usec)"));
		INFO_PRINTF1(_L("D+  - stackMaxDelay(usec)"));
		}
#endif
	
	INFO_PRINTF1(_L("Sent packets data begin"));

	
#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
	if (numDelayResults)
		{
		INFO_PRINTF1(_L("T  ,  dT  ,  Tp  ,  Tb  ,  Tr  ,  Bp  ,  BLp  ,  D-  ,  Dm  ,  D+"));
		}
	else
#endif
		{
		INFO_PRINTF1(_L("T  ,  dT  ,  Tp  ,  Tb  ,  Tr"));
		}
	TInt64 runningFastCounter = 0;
	for (TInt top_i=1,bottom_i=0 ; top_i<iCurrentDataIndex ; ++top_i,++bottom_i)
		{
		TInt thisTimerDiff = MRunningFastCounter::CalculateFastCounterDiff(iCollectedTimerSamples[top_i-1],iCollectedTimerSamples[top_i]);
		runningFastCounter += thisTimerDiff;
		TReal timerReal = runningFastCounter;
		timerReal /= fastCounterFreq;
		NETPERF_TRACE_3(_L("FC: %d %d"),iCollectedTimerSamples[top_i-1],iCollectedTimerSamples[top_i]);
		NETPERF_TRACE_2(_L("-> diff: %d"),thisTimerDiff);
		NETPERF_TRACE_2(_L("-> real timer: %.5f"),timerReal);
		TReal timeDiffReal = thisTimerDiff; 
		timeDiffReal /= fastCounterFreq;

		TInt byteDiff=iCollectedByteCountSamples[top_i]-iCollectedByteCountSamples[top_i-1];
		TReal rate = byteDiff; 
		rate /= timeDiffReal; 
		rate /= 1000.0;
		rate *= 8.0;

#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
		if (bottom_i<numDelayResults)
			{
			TDelaySampleSet& delayResult = delayResults[bottom_i];
		
			TInt sentOutByStackDelta = delayResult.GetNumberOfPackets();
			TInt packetsLostByStack = delayResult.GetNumberOfPacketsLost();
			TReal stackMinDelay,stackMeanDelay,stackMaxDelay;
			delayResult.GetTimeValues(fastCounterFreq, stackMinDelay,stackMeanDelay,stackMaxDelay);
			stackMinDelay*=1e6;		// to get in microsec
			stackMeanDelay*=1e6;	// to get in microsec
			stackMaxDelay*=1e6;		// to get in microsec
			
			Logger().LogExtra(((TText8*)__FILE__), __LINE__, ESevrInfo,
					_L("%.2f, %.5f,  %d, %d, %.5f,  %d, %d,  %.5f, %.5f, %.5f"), 
							timerReal,
							timeDiffReal,
							
							iCollectedPacketCountSamples[top_i]-iCollectedPacketCountSamples[top_i-1],
							byteDiff,
							rate,
							
							sentOutByStackDelta,
							packetsLostByStack,
							
							stackMinDelay,stackMeanDelay,stackMaxDelay);
			}
		else
#endif
			{
			Logger().LogExtra(((TText8*)__FILE__), __LINE__, ESevrInfo,
					_L("%.2f, %.5f,  %d, %d, %.5f"), 
							timerReal,
							timeDiffReal,
							iCollectedPacketCountSamples[top_i]-iCollectedPacketCountSamples[top_i-1],
							byteDiff,
							rate);
			}
		}

	INFO_PRINTF1(_L("Sent packets data end"));
	}



//===========================================================================
