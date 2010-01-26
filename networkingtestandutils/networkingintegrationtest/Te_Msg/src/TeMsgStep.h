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
 @file TeMsgStep.h
*/
#if (!defined __TEMSG_STEP_H__)
#define __TEMSG_STEP_H__
#include <test/testexecutestepbase.h>
#include "TeMsgServer.h"
#include <es_sock.h>

class CTestConnectStep : public CTestStep
	{
public:
	CTestConnectStep();
	~CTestConnectStep();
	virtual TVerdict	doTestStepPreambleL();
	virtual TVerdict	doTestStepL();
	virtual TVerdict	doTestStepPostambleL();

private:
	TInt				iIapNumber;
	TInt				iPort;
	RSocketServ			iSocketServ;
	RSocket				iSocket;
	RConnection			iConnection;

private:
	CActiveScheduler*	iScheduler;

	};

_LIT(KConnectWithOverrides,"ConnectWithOverrides");

#endif
