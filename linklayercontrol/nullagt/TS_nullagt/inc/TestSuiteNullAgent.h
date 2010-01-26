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
// Contains the declarations of the test suite classes used to test 
// the Null Agent using the integration test framework (scheduleTest)
// 
//

#ifndef __TESTSUITENULLAGT_H__
#define __TESTSUITENULLAGT_H__

#include <networking/testsuite.h>

class CTestSuiteNullAgt : public CTestSuite
	{
public:
	void InitialiseL(void);
	CTestSuiteNullAgt::~CTestSuiteNullAgt() {};
	TPtrC GetVersion(void);
	
private:
	};

IMPORT_C CTestSuiteNullAgt* CreateTestSuite(void);

#endif /* __TESTSUITENULLAGT_H__ */
