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

#ifndef __TESTEPBASE_H__
#define __TESTEPBASE_H__

#include <test/testexecuteclient.h>

#include "TeSocketListener.h"
#include "TeSocketConnector.h"
#include "TeListenerMgr.h"
#include "TeHttpServer.h"

class CTestListenerMgr;
class CTestSocketListener;
class CTestSocketConnector;

class CTestStepBase : public CTestStep  
{
public:
 	CTestStepBase(CTestListenerMgr* aListenerMgr);
	~CTestStepBase();

	void					CreateListenerL();
	void					CreateConnectorL();
	
public:
	CTestSocketListener*	iListener; // owned by the iListenerMgr
	CTestSocketConnector*   iConnector; // owned by the iListenerMgr

private:
	CTestListenerMgr*		iListenerMgr;
	CActiveScheduler*		iScheduler;

};

#endif // __TESTEPBASE_H__
