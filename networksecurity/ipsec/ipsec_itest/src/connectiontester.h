// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

#ifndef __CONNECTIONTESTER_H__
#define __CONNECTIONTESTER_H__

#include "connectionutils.h"
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <networking/qoslib_internal.h>
#endif

enum { EIdleBit = 0,
	EStartingBit, EStartedBit, EJoinedBit, ETransferBit,
	EStopedBit, 
	EProgressBit, EServiceChangeBit, EAllInterfaceNotificationBit,
	EIsConnectionActiveBit,
	EErrorBit };

enum TConenctionState 
	{
	EIdle = 1 << EIdleBit,
	EStarting = 1 << EStartingBit,
	EStarted = 1 << EStartedBit,
	EJoined = 1 << EJoinedBit,
	ETransfer = 1 << ETransferBit,
	EStopped = 1 << EStopedBit,
	EProgress = 1 << EProgressBit,
	EServiceChange = 1 << EServiceChangeBit,
	EAllInterfaceNotification = 1 << EAllInterfaceNotificationBit,
	EIsConnectionActive = 1 << EIsConnectionActiveBit,
	EError = 1 << EErrorBit
	};

enum TProgressState
	{
	EProgressStart = 1,
	EProgressDone = 2
	};


// Active object for manipulating a RConnection
class CConnTester : public CActive
	{
public:
	// Creates a conenction to the policy server, and returns
	static CConnTester* NewLC(CLogger* aLogger);

	~CConnTester();
	
	void StartConenction(TInt aIapId = 0, TInt aNetId = 0, TCommDbDialogPref aPref = ECommDbDialogPrefDoNotPrompt);

	// Get and store connection info in iInfo structure
	void GetConnectionInfo(TUint aIapId);

	TInt JoinConnection(TUint aIapId, TUint aNetId);

	TInt StopConnection(TUint aIapId = 0, TUint aNetId = 0);

	void GetProgressL();

#ifdef _USE_QOS
	void StartTransferL(const TInetAddr& anAddr, const TInt aPort, CQoSParameters* aQosParameters = NULL, TInt aTransferType = 0);
#else
	void StartTransferL(const TInetAddr& anAddr, const TInt aPort, TInt aTransferType = 0);
#endif

	void GetStatsL();

	void PrintRoutingTableL();

	TInt State();

	TInt GetResult();

	RConnection& GetConnection();

private:
	CConnTester(CLogger* aLogger);

	void RunL();

	void DoCancel();

private:
	RSocketServ iSS;
	RConnection iConn;
	TCommDbConnPref iPref;

	CLogger* iLogger;
	TProgressState iIsProgressActive;
	TConenctionState iConnState;
	TInt iErr;
	TPckgBuf<TConnectionInfo> iInfo;
		
	CProgress* iProgress; 
	CSender* iSender; 
	};

#endif
