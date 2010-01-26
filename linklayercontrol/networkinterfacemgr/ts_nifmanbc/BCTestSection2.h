// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef __BCTESTSECTION2_H__
#define __BCTESTSECTION2_H__

#include "NifmanBCTestStep.h"

class CTestStep2_1 : public CNifmanBCTestStep
	{
public:
	CTestStep2_1();
	enum TVerdict doTestStepL();
	};

class CTestStep2_2 : public CNifmanBCTestStep
	{
public:
	CTestStep2_2();
	enum TVerdict doTestStepL();
	};

class CTestStep2_3 : public CNifmanBCTestStep
	{
public:
	CTestStep2_3();
	enum TVerdict doTestStepL();
	};

class CTestStep2_4 : public CNifmanBCTestStep
	{
public:
	CTestStep2_4();
	enum TVerdict doTestStepL();
	};

#endif

