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


#ifndef __TE_INIT_H__
#define __TE_INIT_H__

#include <test/testexecutestepbase.h>
#include <es_sock.h>
#include "TeSocketListener.h"
#include "TeStepBase.h"
#include "TeListenerMgr.h"

class CTestListenerMgr;

class CTestStepInit : public CTestStepBase						
{

public:
	CTestStepInit(CTestListenerMgr* aListenerMgr);
	~CTestStepInit();

//	virtual TVerdict	doTestStepPreambleL();
	virtual	TVerdict	doTestStepL();
//	virtual TVerdict	doTestStepPostambleL();

private:
	CTestListenerMgr*		iListenerMgr;
};

_LIT(KInit,"Init");

#endif  // __TE_INIT_H__
