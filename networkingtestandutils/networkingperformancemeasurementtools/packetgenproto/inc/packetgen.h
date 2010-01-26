// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
//  Library used by top (netperfte) and bottom of stack (delaymeterplugin)
//  to record the stack delay i.e. the time taken for each packet to
//  traverse the comms stack.
//

/**
 @file
 @internalTechnology
*/

#ifndef __PACKETGEN_H
#define __PACKETGEN_H

//#include <hal.h>
#include <networking/packetgenprotofactory.h>

class RMBufChain;

namespace PacketGen
{
union UStamp
	{
	TUint32 iStamps[2];
	TUint8 iChar[8];
	};

const TUint KPacketGenControlLevel = CPacketGenProtoFactory::EUid;

// for the pub/sub interface
enum
	{
	KCommandToPacketGen = 1,
	KDataTransfer = 2,
	KResponseFromPacketGen = 3,
	};

// commands issued with the control call (i.e. via KCommandToDelayMeter pubsub property)
enum
	{
	KPacketGenControl_StartOutboundGeneration = 0,
	KPacketGenControl_StopOutboundGeneration = 1,
	//KPacketGenControl_StartInboundGeneration = 4, - these would be to test the ability of the stack to cope with high bitrate unconstrained by bearer.. hmm maybe not that useful as it'd just be a battle over CPU
	//KPacketGenControl_StopInboundGeneration = 5,
	};

struct TPacketGenPrefs
	{
	TPacketGenPrefs::TPacketGenPrefs() :
		iSamplingPeriodInMilliseconds(1000),
		iTestDurationInSeconds(60)
		{
		}
		
	//TInt iRequestedRateInKbps;  // later. for now we go full throttle
	TInt iTestDurationInSeconds;
	TInt iDestinationPort; // destination port number to match
	};

void DefinePubSubKeysL();
void UndefinePubSubKeys();

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


} // namespace PacketGen


#endif // __PACKETGEN_H

