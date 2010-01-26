/**
* Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/



/**
 @file tnifman.h
*/
//////////////////////////////////////////////////////////////////////
#ifndef _TNIFMAN_H_
#define _TNIFMAN_H_

#include <comms-infras/nifagt.h>
#include <comms-infras/nifif.h>
#include <e32test.h>
#include <c32comm.h>
#include <testexecutelog.h>
#include "tnifprog.h"

class RTestNif
	{	
public:

	RTestNif();
	~RTestNif();

	// functionality originally provided by RNif
	TInt AgentInfo(TNifAgentInfo& aInfo);
	TInt DisableTimers(TBool aDisable=ETrue);
	TInt Stop();
	void ProgressNotification(TNifProgressBuf& aProgress, TRequestStatus& aStatus, TUint aSelectedProgress = KConnProgressDefault);
	void CancelProgressNotification();
	TInt Progress(TNifProgress& aProgress);
	TInt LastProgressError(TNifProgress& aProgress);
	static TVersion Version();
	TInt Open(const TDesC& aName=TPtrC());
	TInt NetworkActive(TBool& aIsActive);

	// functionality from the previous version of RTestNif
	TInt Start(TInt aTestNo);
	TInt SetInitialValue(TInt aTestNo);

	// async version of start - not in RNif or previous RTestNif
	void Start(TInt aTestNo, TRequestStatus& aStatus);

	// functionality originally provided by RSessionBase
	void Close();

protected:
	RSocketServ iSocketServ;
	RConnection iConnection;
	TBool iConnectionOpen;
	};

#endif

