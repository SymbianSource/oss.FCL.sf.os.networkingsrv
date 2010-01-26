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
// TeListenerMgr.cpp: implementation of the CTestListenerMgr class.
// 
//

#include "TeListenerMgr.h"
#include "TeSocketListener.h"


CTestListenerMgr::CTestListenerMgr()
	{
	}   

CTestListenerMgr::~CTestListenerMgr()
	{
	}

CTestSocketListener* CTestListenerMgr::CreateListenerL(TPtrC /*aListener*/, 
CTestStepBase* aTestStepBase)
	{
	iListener = CTestSocketListener::NewL(this, aTestStepBase);
	return iListener;
	}

CTestSocketConnector* CTestListenerMgr::CreateConnectorL(TPtrC /*aListener*/,
CTestStepBase* aTestStepBase)
	{
	iConnector = CTestSocketConnector::NewL(this, aTestStepBase);
	return iConnector;
	}
