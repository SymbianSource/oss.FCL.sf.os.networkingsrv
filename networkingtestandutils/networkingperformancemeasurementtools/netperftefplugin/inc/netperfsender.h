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

#ifndef __NETPERF_SENDER_H__
#define __NETPERF_SENDER_H__

#include "netperfserver.h"

class CIperfSender;
class CWriter : public CTimer, public MRunningFastCounter
	{
public:
	CWriter(RSocket& aSock, CIperfSender& aOwner, TInt aPacketSizeInBytes, TInt aSendThroughput_kbps,
			TSockAddr& aSendTo, TBool aUseLowerLayerPacketGenerator):
	CTimer(CActive::EPriorityStandard),
	iSocket(aSock),
	iOwner(aOwner),
	iPacketSizeInBytes(aPacketSizeInBytes),
	iSendThroughput_kbps(aSendThroughput_kbps),
	iSendTo(aSendTo),
	iUseLowerLayerPacketGenerator(aUseLowerLayerPacketGenerator),
	iPacket(0,0),
	iStopping(EFalse)
		{
		}

	~CWriter();

	CTestExecuteLogger& Logger();

	static CWriter* NewL(RSocket& aSock,CIperfSender& aOwner, TInt aPacketSizeInBytes, TInt aSendThroughput_kbps,
			TSockAddr& aSendTo,	TBool aUseLowerLayerPacketGenerator);
	void ConstructL();

	void StartL();

	// the timer callback
	virtual void RunL();

	void MarkForStop();
//	void StopL();

	void Send();
	void SendTakingOverThread();

	TInt SendPackets(TInt aNumPacketsToSend);
	void SendAndReceiveFinalPacket();

	RSocket iSocket;
	TBuf8<KDefaultBufferSize> iBuf;
	TPtr8 iPacket;
//	TSockXfrLength iXfrLen;
	TInt iNumSentPackets;
	TInt iTotSize;

	CIperfSender& iOwner;
	TInt iPacketSizeInBytes;
	TInt iSendThroughput_kbps;
	TSockAddr iSendTo;
	
	TBool iUseLowerLayerPacketGenerator;

	CIperfProtocol iIperfProtocol;

	TBool iStopping;

#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
	DelayMeter::CSendTimestamper* iDelayTimestamper;
#endif
	};

class CIperfSender : public CIperfTestWorker
	{
public:
	typedef CIperfTestWorker ParentClass;

	CIperfSender(CIperfTestServer& aIperfTestServer, CTestExecuteLogger& aLogger)
		: CIperfTestWorker(aIperfTestServer,aLogger)
		{}
	virtual ~CIperfSender();

	// virtuals from parent
	void PrepareL();
	void StartL();
	void StopL();
	void ReportL();
	void Destroy();

	
	void RecordCountAndTime();

	void SetSendToSockAddr(const TSockAddr& aSendToSockAddr) {iSendToSockAddr=aSendToSockAddr;}

	void SetUseLowerLayerPacketGenerator(TBool aA) {iUseLowerLayerPacketGenerator=aA;}

protected:
	TSockAddr iSendToSockAddr;

	 // set this to use this class to seed packet generator protocol if present
	TBool iUseLowerLayerPacketGenerator;

	// calculated
	TInt iPacketsPerSend;

	// worker object
	CWriter* iWriter;
	};


#endif // __NETPERF_SENDER_H__
