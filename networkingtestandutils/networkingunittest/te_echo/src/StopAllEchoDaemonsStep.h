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
 @file StopAllEchoDaemonsStep.h
 @internalTechnology
*/
#if (!defined __STOPALLECHODAEMONS_STEP_H__)
#define __STOPALLECHODAEMONS_STEP_H__
#include <test/testexecutestepbase.h>
#include "Te_echoSuiteStepBase.h"

class CStopAllEchoDaemonsStep : public CTe_echoSuiteStepBase
	{
public:
	CStopAllEchoDaemonsStep();
	~CStopAllEchoDaemonsStep();
	virtual TVerdict doTestStepPreambleL();
	virtual TVerdict doTestStepL();
	virtual TVerdict doTestStepPostambleL();

	};

_LIT(KStopAllEchoDaemonsStep,"StopAllEchoDaemons");

#endif
