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
 @file TeHttpServer.h
*/
#ifndef __TEHTTP_SERVER_H__
#define __TEHTTP_SERVER_H__
#include <test/testexecuteserverbase.h>

#include <e32base.h>
#include "TeListenerMgr.h"

class CTestListenerMgr;
class CTestHttpServer : public CTestServer
	{
public:
	static CTestHttpServer* NewL();
	~CTestHttpServer();
	virtual CTestStep* CreateTestStep(const TDesC& aStepName);

protected:
	CTestHttpServer();
	void ConstructL(const TDesC& aName);

private:
	CTestListenerMgr*	iListenerMgr;
	};

#endif //__TEHTTP_SERVER_H__
