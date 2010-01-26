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
* This is the header file with the declartions of the RConnection test step classes that basically 
* aren't proper tests at all - thet are just helper functions encapsulated as test steps so that they
* can be called from the scripts.
* 
*
*/



/**
 @file TS_DummyPPPHelperSteps.h
*/

#if (!defined __TS_DUMMYPPPHELPERSTEPS_H__)
#define __TS_DUMMYPPPHELPERSTEPS_H__

#include "TS_DummyPPPStep.h"

class TS_DummyPPPCommInit : public TS_DummyPPPStep
{
public:
	TS_DummyPPPCommInit(TPtrC aName);
	virtual ~TS_DummyPPPCommInit();

	virtual enum TVerdict doTestStepL(void);
};

class TS_DummyPPPForceCCoverWrite : public TS_DummyPPPStep
{
public:
	TS_DummyPPPForceCCoverWrite(TPtrC aName);
	virtual ~TS_DummyPPPForceCCoverWrite();

	virtual enum TVerdict doTestStepL(void);
};

#endif
