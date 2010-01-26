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


#ifndef __DELAYMETER_H
#define __DELAYMETER_H

#include <hal.h>
#ifdef SYMBIAN_OLD_EXPORT_LOCATION
#include <networking/delaymeterprotofactory.h>
#else
#include "delaymeterprotofactory.h"
#endif

class RMBufChain;

namespace DelayMeter
{
enum { KDelayMeterMagicCookie = 0xCEFA0BB0 };

union UStamp
	{
	TUint32 iStamps[2];
	TUint8 iChar[8];
	};

// sequence number in packet, seen packet number, total delta, min, max

struct TDelaySampleSet
	{
public:
	void Clear();

	void StoreDelay(TInt aFcDiff);

	IMPORT_C void GetTimeValues(TReal32 aFastCounterFreq,
						TReal& aMinDelayInSeconds,
						TReal& aMeanDelayInSeconds,
						TReal& aMaxDelayInSeconds) const;
	TInt32 GetNumberOfPackets() const {return iNumberOfPackets;}
	TInt32 GetNumberOfPacketsLost() const {return iNumberOfPacketsLost;}
	
protected :
	TUint64 iDelayTotalInFastCounterUnits;
	TUint32 iNumberOfPackets;
	TUint32 iNumberOfPacketsLost;
	TUint32 iMinDelayInFastCounterUnits;
	TUint32 iMaxDelayInFastCounterUnits;
	};

const TUint KDelayMeterControlLevel = CDelayMeterProtoFactory::EUid;

// for the pub/sub interface
enum
	{
	KCommandToDelayMeter = 1,
	KDataTransfer = 2,
	KResponseFromDelayMeter = 3,
	};

// commands issued with the control call (i.e. via KCommandToDelayMeter pubsub property)
enum
	{
	KDelayMeterControl_StartSendRecorder = 0,
	KDelayMeterControl_GetSendRecorderResults = 1,
	KDelayMeterControl_StartReceiveTimestamper = 4,
	KDelayMeterControl_GetReceiveTimestamperResults = 5,
	KDelayMeterControl_NumberOfControlsPerInstance = 16,
	};

struct TDelayMeterPrefs
	{
	TDelayMeterPrefs::TDelayMeterPrefs() :
		iSamplingPeriodInMilliseconds(1000),
		iTestDurationInSeconds(60)
		{
		}
		
	TInt iSamplingPeriodInMilliseconds;
	TInt iTestDurationInSeconds;

	TInt iDestinationPort; // destination port number to match
	TInt iProtocol;

	TInt iTimestampSpacing; // size of sent/received lumps at the client side
	};

void AllocateResultsBufferL(TDelayMeterPrefs& aPrefs, TInt& aNumberOfResults, TDelaySampleSet*& aResults);

inline TInt CalculateFastCounterDiff(TInt lastTimerValue, TInt thisTimerValue)
	{
	TInt thisTimerDiff = thisTimerValue - lastTimerValue;
	return thisTimerDiff;	
	}

IMPORT_C void DefinePubSubKeysL();
IMPORT_C void UndefinePubSubKeys();
void SendControlL(TInt aLevel, TInt aName, TDes8& aDes);

class CFastCounterUser : public CBase
	{
public:
	CFastCounterUser():
		iTimestampOffsetFromEnd(8)
		{
		HAL::Get(HALData::EFastCounterCountsUp,iFastCounterCountsUp);
		}
	TUint32 IncreasingFastCounter() const
		{
		return iFastCounterCountsUp ? User::FastCounter():(~User::FastCounter());
		}


protected:
	TBool iFastCounterCountsUp;
	TInt iTimestampOffsetFromEnd;
	};

class CRecorder : public CFastCounterUser
	{
public:
	CRecorder();
	
	virtual void SetupL(const TDelayMeterPrefs& aPrefs); // stores prefs and allocates memory
	IMPORT_C void StoreDelay(TInt previousFastCounterValue);
	virtual void GetResultsL(TInt& aNumResults, TDelaySampleSet*& aResults);
	
	
	TInt CalculateTcpTimestampOffset(TInt aStatedLength, TUint8* aHeaderPayload);
	
	
	TDelayMeterPrefs iPrefs;

	TInt iNumberOfResults;
	TDelaySampleSet* iResults;
	
	TDelaySampleSet* iCurrentResult;
	
	TUint32 iSamplingPeriodInFastCounterUnits;
	TUint32 iFastCounterValueForNextSample;

	
	TInt32 iNextStampPos;
	};

// lives up in client
class CSendTimestamper : public CFastCounterUser
	{
protected:
	CSendTimestamper(RSocket& /*aSocket*/, const TDelayMeterPrefs& aPrefs);
public:
	IMPORT_C static CSendTimestamper* NewL(RSocket& aSocket, const TDelayMeterPrefs& aPrefs);

	IMPORT_C void SetupL();
	IMPORT_C void GetResultsL(TInt& aNumResults, TDelaySampleSet*& aResults);
	
	
	IMPORT_C void Timestamp(TDes8& aOutgoingPacket);
	
	//RSocket& iSocket;
	// rjl - this is potentially how we could configure an instance of the DelayMeter, more directly
	//  through the data plane. However at the moment we only need 1 instance, so we can configure
	//   the delaymeter globally using a pub/sub property (which limits us to 1 instance).

	TDelayMeterPrefs iPrefs;
	
	TInt iNumberOfResults;
	TDelaySampleSet* iResults;
	};

// lives down in protocol
class CSendRecorder : public CRecorder
	{
public:
	IMPORT_C CSendRecorder();

	IMPORT_C void RecordDelay(RMBufChain& aData, TInt aStatedLength);
	};

// lives down in protocol
class CReceiveTimestamper : public CRecorder
	{
public:
	IMPORT_C CReceiveTimestamper();

	IMPORT_C void Timestamp(RMBufChain& aData, TInt aStatedLength);
	void TimestampAndUpdateChecksum(RMBufChain& aPayloadBufChain,TInt aStampOffset,TUint8* aChecksumPtr);
	};
	


// lives up in client
class CReceiveRecorder : public CRecorder
	{

protected:

	CReceiveRecorder();

public:

	IMPORT_C static CReceiveRecorder* NewL(const TDelayMeterPrefs& aPrefs);

	virtual void SetupL(const TDelayMeterPrefs& aPrefs); // sends setopt to socket to tell it to timestamp
	
	IMPORT_C void RecordDelay(TDesC8& aIncomingPacket);

	virtual void GetResultsL(TInt& aNumResults, TDelaySampleSet*& aResults);
	};


} // namespace DelayMeter


#endif // __DELAYMETER_H

