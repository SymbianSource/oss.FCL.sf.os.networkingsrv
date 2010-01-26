/**
* Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
 @file StartEchoDaemonStep.h
 @internalTechnology
*/
#if (!defined __STARTECHODAEMON_STEP_H__)
#define __STARTECHODAEMON_STEP_H__
#include <test/testexecutestepbase.h>
#include "Te_echoSuiteStepBase.h"

class CStartEchoDaemonStep : public CTe_echoSuiteStepBase
	{
public:
	CStartEchoDaemonStep();
	~CStartEchoDaemonStep();
	virtual TVerdict doTestStepPreambleL();
	virtual TVerdict doTestStepL();
	virtual TVerdict doTestStepPostambleL();

// Please add/modify your class members here:
private:
	};

_LIT(KStartEchoDaemonStep,"StartEchoDaemon");

#endif
