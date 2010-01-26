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
 @file TestSteps.cpp
*/

#if !defined(_TESTSTEPS_H_)
#define _TESTSTEPS_H_

#include <testexecutestepbase.h>

class CPPPANVL : public CTestStep
{
public:
	TVerdict doTestStepL();
	CPPPANVL();
	virtual ~CPPPANVL();
private:
	void StartClientL();
	void doCreateMBufL();
	void CommInitL(TBool aEnhanced);
};

#endif
