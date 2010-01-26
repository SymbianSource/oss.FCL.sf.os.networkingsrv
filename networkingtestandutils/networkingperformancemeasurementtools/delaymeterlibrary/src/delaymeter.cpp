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
// Library used by top (netperfte) and bottom of stack (delaymeterplugin)
// to record the stack delay i.e. the time taken for each packet to
// traverse the comms stack.
// 
//

/**
 @file
 @internalTechnology
*/

#include <e32property.h>
#include <in_sock.h>
#ifdef SYMBIAN_OLD_EXPORT_LOCATION
#include <networking/netperftrace.h>
#else
#include "netperftrace.h"
#endif
#include "delaymeter.h"
#include <es_mbman.h>

// for panic
_LIT(KDelayMeterName,"DelayMeter");
enum
	{
	KPanicCouldNotUndefinePubSubKeys = 1001
	};

using namespace DelayMeter;

void TDelaySampleSet::Clear()
	{
	iNumberOfPackets=0;
	iNumberOfPacketsLost=0;
	iDelayTotalInFastCounterUnits=0;
	iMinDelayInFastCounterUnits=KMaxTUint32;
	iMaxDelayInFastCounterUnits=0;
	}
	
void TDelaySampleSet::StoreDelay(TInt aFcDiff)
	{
	++iNumberOfPackets;
	
	iDelayTotalInFastCounterUnits += aFcDiff;

	if (aFcDiff<iMinDelayInFastCounterUnits)
		{
		iMinDelayInFastCounterUnits=aFcDiff;
		}
	if (aFcDiff>iMaxDelayInFastCounterUnits)
		{
		iMaxDelayInFastCounterUnits=aFcDiff;
		}
	}

EXPORT_C void TDelaySampleSet::GetTimeValues(TReal32 aFastCounterFreq,
									TReal& aMinDelayInSeconds,
									TReal& aMeanDelayInSeconds,
									TReal& aMaxDelayInSeconds) const
	{
	aMinDelayInSeconds = iMinDelayInFastCounterUnits;
	aMinDelayInSeconds /= aFastCounterFreq;

	aMeanDelayInSeconds = iDelayTotalInFastCounterUnits;
	aMeanDelayInSeconds /= iNumberOfPackets;
	aMeanDelayInSeconds /= aFastCounterFreq;

	aMaxDelayInSeconds = iMaxDelayInFastCounterUnits;
	aMaxDelayInSeconds /= aFastCounterFreq;
	}





	
void DelayMeter::AllocateResultsBufferL(TDelayMeterPrefs& aPrefs, TInt& aNumberOfResults, TDelaySampleSet*& aResults)
	{
	aNumberOfResults = (aPrefs.iTestDurationInSeconds * 1000) / aPrefs.iSamplingPeriodInMilliseconds;
	aResults = new(ELeave) TDelaySampleSet[aNumberOfResults];
	}


EXPORT_C void DelayMeter::DefinePubSubKeysL()
	{
	TInt result = RProperty::Define(TUid::Uid(KDelayMeterControlLevel), KCommandToDelayMeter, RProperty::EInt);
	if (result == KErrAlreadyExists) {result=KErrNone;}
	User::LeaveIfError(result);
	
	result = RProperty::Define(TUid::Uid(KDelayMeterControlLevel), KDataTransfer, RProperty::ELargeByteArray);
	if (result == KErrAlreadyExists) {result=KErrNone;}
	User::LeaveIfError(result);

	result = RProperty::Define(TUid::Uid(KDelayMeterControlLevel), KResponseFromDelayMeter, RProperty::EInt);
	if (result == KErrAlreadyExists) {result=KErrNone;}
	User::LeaveIfError(result);
	}

EXPORT_C void DelayMeter::UndefinePubSubKeys()
	{
	TInt result = RProperty::Delete(TUid::Uid(KDelayMeterControlLevel), KCommandToDelayMeter)
		|| RProperty::Delete(TUid::Uid(KDelayMeterControlLevel), KDataTransfer)
		|| RProperty::Delete(TUid::Uid(KDelayMeterControlLevel), KResponseFromDelayMeter);
	if (result)
		{  // try to avoid a deadlock later
		User::Panic(KDelayMeterName,KPanicCouldNotUndefinePubSubKeys);
		}
	}

void DelayMeter::SendControlL(TInt aLevel, TInt aName, TDes8& aDes)
	{
	//	iSocket.SetOpt(KDelayMeterControlLevel,KDelayMeterControl_StartSendRecorder,optionDes);
	// rjl - this is potentially how we could configure an instance of the DelayMeter, more directly
	//  through the data plane. However at the moment we only need 1 instance, so we can configure
	//   the delaymeter globally using a pub/sub property (which limits us to 1 instance).
	
	// set the argument if possible. if not, we should assume the
	//  delay protocol isn't running because it should've set it up.
	TInt result=RProperty::Set(TUid::Uid(aLevel), KDataTransfer, aDes);
	User::LeaveIfError(result);

	// listen before we poke..
	RProperty response;
	User::LeaveIfError(response.Attach(TUid::Uid(aLevel), KResponseFromDelayMeter));
	TRequestStatus responseStatus;
	response.Subscribe(responseStatus);

	// ok, now poke:
	result = RProperty::Set(TUid::Uid(aLevel), KCommandToDelayMeter, aName);
	if (result!=KErrNone)
		{
		response.Cancel();
		}
	// did we hear a published response? or a cancel response because we couldn't poke?
	User::WaitForRequest(responseStatus);

	// now leave with the previous error if there was a problem
	User::LeaveIfError(result);

	// leave with the error if there was a problem raised by the other end
	User::LeaveIfError(responseStatus.Int());

	TInt retrievedResponse;
	User::LeaveIfError(response.Get(retrievedResponse));
	User::LeaveIfError(retrievedResponse);
	User::LeaveIfError(RProperty::Get(TUid::Uid(aLevel), KDataTransfer, aDes)); // fetch result buffer.
	}




CRecorder::CRecorder():
	iNumberOfResults(0),
	iResults(NULL),
	iCurrentResult(NULL),
	iSamplingPeriodInFastCounterUnits(0),
	iFastCounterValueForNextSample(0)
	{}


/*virtual*/
void CRecorder::SetupL(const TDelayMeterPrefs& aPrefs)
	{
	TInt fcf;
	HAL::Get(HALData::EFastCounterFrequency,fcf);
	
	TReal fastCounterFreq = fcf;
	TReal samplingperiodinsec = aPrefs.iSamplingPeriodInMilliseconds / 1000;
	iSamplingPeriodInFastCounterUnits = fastCounterFreq / samplingperiodinsec;
	
	if (iSamplingPeriodInFastCounterUnits > KMaxTInt32)
		{
		// Yipe! specified a far too large sampling period (or you've got an insanely quick fastcounter)
		// Either way we can't expected to work under these conditions! Abort!
		User::Leave(KErrTooBig);
		}

	iPrefs=aPrefs;
	AllocateResultsBufferL(iPrefs, /* returns -> */ iNumberOfResults, iResults);
	iCurrentResult = iResults; // can increment til end of results buffer
	iCurrentResult->Clear();
	}

EXPORT_C void CRecorder::StoreDelay(TInt previousFastCounterValue)
	{
	TUint32 currentFastCounter = IncreasingFastCounter();

	// see if we need to sample
	if (iFastCounterValueForNextSample)
		{
		TBool sample=EFalse;
		
		
		if (iFastCounterValueForNextSample<currentFastCounter && currentFastCounter-iFastCounterValueForNextSample > KMaxTInt32)
			{
			// next sample point was set past wrap.. current is high, sample point is low..
			// so we haven't reached it yet.
			}
		else if (iFastCounterValueForNextSample-currentFastCounter > KMaxTInt32)
			{
			// sample point was set right at top of range.. but now current time is low.
			// so we definitely wrapped around and passed it.
			sample=ETrue;
			}
		else if (currentFastCounter > iFastCounterValueForNextSample)
			{
			// normal business, no wrap. is current time > sample time?
			sample=ETrue;
			}
	
		if (sample)
			{
			iFastCounterValueForNextSample += iSamplingPeriodInFastCounterUnits;

			// take the sample. i.e. move on to using the next result set.
			if (iCurrentResult < &iResults[iNumberOfResults-1])
				{
				// stick at end if we've run out of memory. shouldn't happen and a boundary case.
				iCurrentResult++;
				iCurrentResult->Clear();
				}
			}
		}
	else
		{
		// first time..
		iFastCounterValueForNextSample = currentFastCounter + iSamplingPeriodInFastCounterUnits;
		}

	TInt fcdiff =CalculateFastCounterDiff(previousFastCounterValue,currentFastCounter);
	iCurrentResult->StoreDelay(fcdiff);
	}

/*virtual*/
void CRecorder::GetResultsL(TInt& aNumResults, TDelaySampleSet*& aResults)
	{
	aNumResults = (iCurrentResult-iResults+1);
	aResults = iResults;
	}
	

// finds offset for timestamp within this packet.
//  returns KErrNotFound if a timestamp in this packet is not possible / appropriate
TInt CRecorder::CalculateTcpTimestampOffset(TInt aStatedLength, TUint8* aHeaderPayload)
	{
	TUint8*& t = aHeaderPayload;
	
 	const TInt ipHeaderLength = 20;
	t+=ipHeaderLength; // skip IP header so we can clip code from tcp_hdr.h
	TInt tcpHeaderAndDataLength = aStatedLength-ipHeaderLength;
	TInt tcpHeaderLength = (t[12] >> 2) & (0xF << 2);
	TInt dataLength = tcpHeaderAndDataLength-tcpHeaderLength;
	
	TUint sequenceNumber = (t[4] << 24) | (t[5] << 16) | (t[6] << 8) | t[7];

	NETPERF_TRACE_5(_L("TCP flags %02x, seq %d, dataLen %d, nextStampSeq %d"),t[13],sequenceNumber,dataLength,iNextStampPos);

	if ( t[13] & 2/*KTcpCtlSYN*/)
		{
		ASSERT(!iNextStampPos); // first packet.. put timestamp at beginning. SYN itself takes 1.
		ASSERT(dataLength == 0); // not expecting data in a SYN
		iNextStampPos = sequenceNumber + iPrefs.iTimestampSpacing-sizeof(UStamp);
		NETPERF_TRACE_2(_L("SYN: nextStampSeq %d"),iNextStampPos);
		return KErrNotFound;
		}
	
	if (dataLength==0)
		{
		// it was a control packet. but sequence number needs to be at least 1 more.
		iNextStampPos++;
		NETPERF_TRACE_2(_L("control packet seen. updating nextStampSeq %d"),iNextStampPos);
		//  let's hope this didn't arrive out of sequence because we can't afford to
		//  queue packets - if we did that we may as just well reimplement the stack.. huff
		return KErrNotFound;
		}
	
	TInt timestampOffsetInThisPacketData = iNextStampPos - sequenceNumber;
	NETPERF_TRACE_2(_L("timestamp offset in this packet data: %d"),timestampOffsetInThisPacketData);
	if (timestampOffsetInThisPacketData < 0)
		// missed it.. presumably due to this packet coming in out of sequence..
		//   but we're not queuing. so recalculate
		{
		// round up to nearest stamppos.. to ensure timestampOffsetInThisPacketData is positive or 0
		iNextStampPos += iPrefs.iTimestampSpacing * (-timestampOffsetInThisPacketData / iPrefs.iTimestampSpacing + 1);
		timestampOffsetInThisPacketData = iNextStampPos - sequenceNumber;
		NETPERF_TRACE_1(_L("negative offset means timestamp was in a previous packet."));
		NETPERF_TRACE_2(_L(" so finding first timestamp from start of this packet: %d"),iNextStampPos);
		NETPERF_TRACE_2(_L(" (timestamp offset %d in this packet data)"),timestampOffsetInThisPacketData);
		}
	
	ASSERT(timestampOffsetInThisPacketData>=0);
	
	if (timestampOffsetInThisPacketData > dataLength)
		{
		// timestamp belongs in a later packet. Nothing to do for now..
		NETPERF_TRACE_2(_L("timestamp at offset %d belongs in a later packet. Nothing to do for now.."),
							timestampOffsetInThisPacketData);
		return KErrNotFound;
		}
	
	if (timestampOffsetInThisPacketData + sizeof(UStamp) > dataLength)
		{
		// timestamp either belongs in next packet or straddles the boundary.. punt it along.
		iNextStampPos += iPrefs.iTimestampSpacing;
		NETPERF_TRACE_1(_L("timestamp either belongs in next packet or straddles the boundary.."));
		NETPERF_TRACE_2(_L(".. nextStampSeq pushed on to %d"),iNextStampPos);
		return KErrNotFound;
		}
	
	// ok, finally we're allowed to stamp
	//
	TInt result=ipHeaderLength+tcpHeaderLength+timestampOffsetInThisPacketData;
	NETPERF_TRACE_2(_L("offset for timestamp found in this packet! offset: %d"),result);
	return result;
	}




CSendTimestamper::CSendTimestamper(RSocket& /*aSocket*/, const TDelayMeterPrefs& aPrefs) :
	//iSocket(aSocket),
	// rjl - this is potentially how we could configure an instance of the DelayMeter, more directly
	//  through the data plane. However at the moment we only need 1 instance, so we can configure
	//   the delaymeter globally using a pub/sub property (which limits us to 1 instance).

	iPrefs(aPrefs),
	iNumberOfResults(0),
	iResults(NULL)
	{
	}

/*static*/
EXPORT_C CSendTimestamper* CSendTimestamper::NewL(RSocket& aSocket, const TDelayMeterPrefs& aPrefs)
	{
	CSendTimestamper* inst = new (ELeave) CSendTimestamper(aSocket,aPrefs);
	CleanupStack::PushL(inst);
	inst->SetupL(); // talks to the protocol via the socket option
	CleanupStack::Pop(inst);
	return inst;
	}


EXPORT_C void CSendTimestamper::SetupL()
	{
	// sends setopt to delay meter layer below
	TPtr8 optionDes((TUint8*)&iPrefs,sizeof(iPrefs),sizeof(iPrefs));
	SendControlL(KDelayMeterControlLevel,KDelayMeterControl_StartSendRecorder,optionDes);	
	}

EXPORT_C void CSendTimestamper::GetResultsL(TInt& aNumResults, TDelaySampleSet*& aResults)
	{
	if (iResults)
		{
		aResults = 0;
		aNumResults = NULL;
		User::Leave(KErrNotSupported); // a "warning" that we're not retrieving the results again.
		}

	// for collecting the results
	AllocateResultsBufferL(iPrefs, /* returns -> */ iNumberOfResults,iResults);
	
	// sends getopt to socket
	TInt size=sizeof(TDelaySampleSet) * iNumberOfResults;
	TPtr8 optionDes((TUint8*)iResults,size,size);
	// copy the prefs in so it gets hold of the right send_recorder
	Mem::Copy(iResults,&iPrefs,sizeof(iPrefs));
	SendControlL(KDelayMeterControlLevel,KDelayMeterControl_GetSendRecorderResults,optionDes);

	ASSERT(optionDes.Length() % sizeof(TDelaySampleSet) == 0);
	iNumberOfResults = optionDes.Length() / sizeof(TDelaySampleSet);

	aNumResults=iNumberOfResults;
	aResults=iResults;
	}
	
EXPORT_C void CSendTimestamper::Timestamp(TDes8& aOutgoingPacket)
	{
	TUint8* stampPtr = const_cast<TUint8*>(aOutgoingPacket.Ptr());
	stampPtr += aOutgoingPacket.Length() - sizeof(TUint32)*2;

	TUint32 magicCookie = KDelayMeterMagicCookie;
	Mem::Copy(stampPtr, (TUint8*)(&magicCookie), sizeof(TUint32));
	stampPtr += sizeof(TUint32);
	TUint32 fastCounter = IncreasingFastCounter();
	Mem::Copy(stampPtr, (TUint8*)(&fastCounter), sizeof(TUint32));
	}




EXPORT_C CSendRecorder::CSendRecorder()
	{
	}

EXPORT_C void CSendRecorder::RecordDelay(RMBufChain& aPayloadBufChain, TInt aStatedLength)
	{
	UStamp stamp;
	TInt timestampOffsetInPayload=-1;
	
	TUint8* t = aPayloadBufChain.First()->Ptr();
	if (iPrefs.iProtocol == KProtocolInetTcp)
		{
		timestampOffsetInPayload = CalculateTcpTimestampOffset(aStatedLength,t);
		}
	else if (iPrefs.iProtocol == KProtocolInetUdp)
		{
		timestampOffsetInPayload = aStatedLength-sizeof(stamp);
		}
	
	if (timestampOffsetInPayload<0)
		{ // -1 if no timestamp in this packet.. or protocol we don't care about..
		return;
		}
	
	TPtr8 stampDes(reinterpret_cast<TUint8 *>(&stamp),sizeof(stamp),sizeof(stamp));
	aPayloadBufChain.CopyOut(stampDes, timestampOffsetInPayload);

	// where's my cookie?!
	if (stamp.iStamps[0] == KDelayMeterMagicCookie)
		{
		TUint32 oldFastcounterValue=stamp.iStamps[1];
		StoreDelay(oldFastcounterValue);
		}
	}
	

EXPORT_C CReceiveTimestamper::CReceiveTimestamper()
	{
	}

// N.B. this function assumes aPayloadBufChain has already been aligned at least up to the TCP/UDP
//    checksum (position 38/28). This saves us calling Align() more than once.
EXPORT_C void CReceiveTimestamper::Timestamp(RMBufChain& aPayloadBufChain,TInt aStatedLength)
	{
	TUint8* t = aPayloadBufChain.First()->Ptr();
	TInt timestampOffsetInPayload=-1, checksumOffsetInPayload=0;
	
	if (iPrefs.iProtocol == KProtocolInetTcp)
		{
		timestampOffsetInPayload = CalculateTcpTimestampOffset(aStatedLength,t);
		checksumOffsetInPayload=36;
		}
	else if (iPrefs.iProtocol == KProtocolInetUdp)
		{
		timestampOffsetInPayload = aStatedLength-sizeof(UStamp);
		checksumOffsetInPayload=26;
		}

	if (timestampOffsetInPayload<0)
		{ // -1 if no timestamp in this packet.. or protocol we don't care about..
		return;
		}

	TimestampAndUpdateChecksum(aPayloadBufChain,timestampOffsetInPayload,t+checksumOffsetInPayload);
	}


void CReceiveTimestamper::TimestampAndUpdateChecksum(RMBufChain& aPayloadBufChain,TInt aStampOffset,TUint8* aChecksumPtr)
	{
	ASSERT(aStampOffset>0); // packet really should be big enough!

	//Prepare stamp with existing data
	UStamp stamp;
	TPtr8 stampDes(reinterpret_cast<TUint8 *>(&stamp),sizeof(stamp),sizeof(stamp));
	aPayloadBufChain.CopyOut(stampDes, aStampOffset);
	
	//Extract the original checksum.
	TUint32 checksum = (aChecksumPtr[0] << 8) + aChecksumPtr[1];
	
	//Modify checksum with existing data
	TInt i;
	for (i=0;i<sizeof(stamp);i+=2)
		{
		checksum += (stamp.iChar[i] << 8) + stamp.iChar[i+1];
		}

	//Modify stamp
	//   write cookie
	//   write timestamp
	NETPERF_TRACE_2(_L("old-word1: %08x"),stamp.iStamps[0]);
	NETPERF_TRACE_2(_L("old-word2: %d"),stamp.iStamps[1]);
	stamp.iStamps[0]=KDelayMeterMagicCookie;
	stamp.iStamps[1]=IncreasingFastCounter();
	NETPERF_TRACE_2(_L("new-stamp: %08x"),stamp.iStamps[0]);
	NETPERF_TRACE_2(_L("new-fcntr: %d"),stamp.iStamps[1]);
	aPayloadBufChain.CopyIn(stampDes, aStampOffset);
	
	//Update checksum with new data
	for (i=0;i<sizeof(stamp);i+=2)
		{
		checksum -= (stamp.iChar[i] << 8) + stamp.iChar[i+1];
		}
	
	//copy checksum back
	aChecksumPtr[0] =  (TUint8)((checksum >> 8) & 0xff);
	aChecksumPtr[1] =  (TUint8)( checksum & 0xff);
	aChecksumPtr[1] += (TUint8)((checksum >>16) & 0xff);
	}


CReceiveRecorder::CReceiveRecorder()
	{
	}

/*static*/
EXPORT_C CReceiveRecorder* CReceiveRecorder::NewL(const TDelayMeterPrefs& aPrefs)
	{
	CReceiveRecorder* inst = new (ELeave) CReceiveRecorder();
	CleanupStack::PushL(inst);
	inst->SetupL(aPrefs); // talks to the protocol
	CleanupStack::Pop(inst);
	return inst;
	}



/*virtual*/
void CReceiveRecorder::SetupL(const TDelayMeterPrefs& aPrefs)
	{
	CRecorder::SetupL(aPrefs);
	
	// sends signal to protocol to tell it to timestamp
	TPtr8 optionDes((TUint8*)&iPrefs,sizeof(TDelayMeterPrefs),sizeof(TDelayMeterPrefs));
	SendControlL(KDelayMeterControlLevel,KDelayMeterControl_StartReceiveTimestamper,optionDes);
	}

EXPORT_C void CReceiveRecorder::RecordDelay(TDesC8& aIncomingPacket)
	{
	TUint8* stampPtr = const_cast<TUint8*>(aIncomingPacket.Ptr());
	stampPtr += aIncomingPacket.Length() - sizeof(TUint32)*2;

	TUint32 magicCookie = KDelayMeterMagicCookie;
	if (Mem::Compare(stampPtr, sizeof(TUint32), (TUint8*)(&magicCookie), sizeof(TUint32))==0)
		{
		NETPERF_TRACE_2(_L("Timestamp seen at position %d."),aIncomingPacket.Length() - sizeof(TUint32)*2);
		stampPtr += sizeof(TUint32);
		TUint32 bottomFastCounterValue = 0;
		Mem::Copy(&bottomFastCounterValue, stampPtr, sizeof(TUint32));
		StoreDelay(bottomFastCounterValue);
		}
	else
		{
		NETPERF_TRACE_6(_L("Timestamp cookie not seen at position %d. Bytes seen: %02x%02x%02x%02x"),
				aIncomingPacket.Length() - sizeof(TUint32)*2, stampPtr[0],stampPtr[1],stampPtr[2],stampPtr[3]);
		}
	}


/*virtual*/
void CReceiveRecorder::GetResultsL(TInt& aNumResults, TDelaySampleSet*& aResults)
	{
	TInt numberOfResultsFromTimestamper;
	TDelaySampleSet* resultsFromTimestamper;
	AllocateResultsBufferL(iPrefs, /* returns -> */ numberOfResultsFromTimestamper, resultsFromTimestamper);
	TInt size=sizeof(TDelaySampleSet) * numberOfResultsFromTimestamper;
	TPtr8 optionDes(reinterpret_cast<TUint8*>(resultsFromTimestamper),size,size);
	// copy the prefs in so it gets hold of the right receive_timestamper
	Mem::Copy(resultsFromTimestamper,&iPrefs,sizeof(iPrefs));
	SendControlL(KDelayMeterControlLevel,KDelayMeterControl_GetReceiveTimestamperResults,optionDes);
	delete[] resultsFromTimestamper; // this can be merged with bottom loss rates if needed.

	CRecorder::GetResultsL(aNumResults,aResults);
	}

