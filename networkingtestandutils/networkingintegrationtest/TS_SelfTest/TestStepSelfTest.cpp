// Copyright (c) 2001-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// This contains CTestCase which is the base class for all the TestCase DLLs
// 
//

// EPOC includes
#include <e32base.h>

// Test system includes
#include "networking/log.h"
#include "networking/teststep.h"
#include "TestStepSelfTest.h"
#include "TestSuiteSelfTest.h"

// constructor
CTestStepSelfTest::CTestStepSelfTest() 
{
}

// destructor
CTestStepSelfTest::~CTestStepSelfTest()
{
}

TInt CTestStepSelfTest::CallStateMachine()
{
	return 0;
}
