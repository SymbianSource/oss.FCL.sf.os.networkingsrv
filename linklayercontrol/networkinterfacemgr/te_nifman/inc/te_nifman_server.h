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
 @file te_nifman_server.h
*/

#ifndef _TE_NIFMAN_SERVER_H
#define _TE_NIFMAN_SERVER_H

#include <testexecuteserverbase.h>

_LIT(KOpenCloseBogus, "OpenCloseBogus");
_LIT(KSocketServerShutdown, "SocketServerShutdown");
_LIT(KProgressNotification, "ProgressNotification");
_LIT(KStartStopInterfaces, "StartStopInterfaces");
_LIT(KBinderLayerDown, "BinderLayerDown");
_LIT(KConnectReconnect, "ConnectReconnect");
_LIT(KTest8, "Test8");
_LIT(KTest5, "Test5");
_LIT(KOpenClosePSD, "OpenClosePSD");

class CNifmanServer : public CTestServer
	{
public:
	virtual CTestStep* CreateTestStep(const TDesC& aStepName);
	static CNifmanServer* NewL();
	virtual ~CNifmanServer();
private:
	CNifmanServer();
	};

#endif

