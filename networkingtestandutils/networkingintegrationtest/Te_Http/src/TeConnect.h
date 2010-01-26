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


#ifndef __TE_CONNECT_H__
#define __TE_CONNECT_H__

#include <test/testexecutestepbase.h>
#include <es_sock.h>
#include "TeSocketListener.h"
#include "TeStepBase.h"
#include "TeListenerMgr.h"


class CTestListenerMgr;

class CTestStepConnect : public CTestStepBase						
{

public:
	CTestStepConnect(CTestListenerMgr* aListenerMgr); 

	~CTestStepConnect();

	virtual TVerdict	doTestStepPreambleL();
	virtual	TVerdict	doTestStepL();
	virtual	TVerdict	doTestStepPostambleL();

private:
	CActiveScheduler*		iScheduler;
	CTestListenerMgr*		iListenerMgr;

};

_LIT(KClientConnect,"ClientConnect");

#endif // __TE_CONNECT_H__
