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
// This contains CTestSuiteSelfTest 
// 
//

#if (!defined __TEST_SUITE_SELF_TEST_H_)
#define __TEST_SUITE_SELF_TEST_H_

#include "networking/testsuite.h"
#include <es_sock.h>

class  CTestSuiteSelfTest : public CTestSuite
{
public:
	
	void InitialiseL( void );
	virtual ~CTestSuiteSelfTest();
	void AddTestStepL( CTestStepSelfTest * ptrTestStep );
	TPtrC GetVersion( void );

private:

};


#endif /* __TEST_SUITE_SELF_TEST_H_ */
