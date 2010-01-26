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

#ifndef __BCTESTSECTION1_H__
#define __BCTESTSECTION1_H__

#include "NifmanBCTestStep.h"

class CTestStepEsockShutdown : public CNifmanBCTestStep
	{
public:
	CTestStepEsockShutdown();
	enum TVerdict doTestStepL();
private:
	TInt UnloadEsock();
	TInt LoadEsock();
	};

class CTestStep1_1 : public CNifmanBCTestStep
	{
public:
	CTestStep1_1();
	enum TVerdict doTestStepL();
	};

class CTestStep1_2 : public CNifmanBCTestStep
	{
public:
	CTestStep1_2();
	enum TVerdict doTestStepL();
	};

class CTestStep1_3 : public CNifmanBCTestStep
	{
public:
	CTestStep1_3();
	enum TVerdict doTestStepL();
	};

class CTestStep1_4 : public CNifmanBCTestStep
	{
public:
	CTestStep1_4();
	enum TVerdict doTestStepL();
	};

class CTestStep1_5 : public CNifmanBCTestStep
	{
public:
	CTestStep1_5();
	enum TVerdict doTestStepL();
	};

class CTestStep1_6 : public CNifmanBCTestStep
	{
public:
	CTestStep1_6();
	enum TVerdict doTestStepL();
	};

class CTestStep1_7 : public CNifmanBCTestStep
	{
public:
	CTestStep1_7();
	enum TVerdict doTestStepL();
	};

#endif

