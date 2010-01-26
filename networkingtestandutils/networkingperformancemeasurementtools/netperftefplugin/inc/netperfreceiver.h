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

#ifndef __NETPERF_RECEIVER_H__
#define __NETPERF_RECEIVER_H__

#include "netperfserver.h"

class CIperfReceiver;


class CReader : public CActive, public MRunningFastCounter
	{
public:
	CReader(RSocket& aSock, CIperfReceiver& aOwner);
	~CReader();
	
	static CReader* NewL(RSocket& aSock, CIperfReceiver& aOwner);
	void ConstructL();
	void RunL();
	void DoCancel();

	RSocket& iSocket;
	RSocket iTcpReadingSock;
	RBuf8 iBuf;
#ifdef SUBOPTIMAL_APP
	TSockXfrLength iXfrLen;
#endif
	TInt iNumReads;
	TInt iTotSize;
	TInt iLastSeqNum;
	TInt iPacketsLost;
	TInt iPacketsOutOfOrder;

	CIperfReceiver& iOwner;
	TBool iEchoReceivedData;
            
	TBool iRecord;

#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
	DelayMeter::CReceiveRecorder* iDelayRecorder;
#endif
	};


class CIperfReceiver : public CIperfTestWorker
	{
public:
	typedef CIperfTestWorker ParentClass;

	CIperfReceiver(CIperfTestServer& aIperfTestServer, CTestExecuteLogger& aLogger)
		: CIperfTestWorker(aIperfTestServer,aLogger)
		{}
	virtual ~CIperfReceiver();

	// virtuals from parent
	virtual void PrepareL();
	virtual void ReportL();
	virtual void Destroy();
	
	void RecordCountAndTime();

	void SetReceivePort(TInt aReceivePort) {iReceivePort = aReceivePort;}
	TInt GetReceivePort() const {return iReceivePort;}
	
	void SetEchoReceivedData(TBool aEchoReceivedData) {iEchoReceivedData = aEchoReceivedData;}
	TBool GetEchoReceivedData() const {return iEchoReceivedData;}
	
protected:

	TBool iEchoReceivedData;
	TInt iReceivePort;
	
	TUint32* iPacketsLost;
	TUint32* iPacketsOutOfOrder;

	
	// worker object
	CReader* iReader;
	};

#endif // __NETPERF_RECEIVER_H__
