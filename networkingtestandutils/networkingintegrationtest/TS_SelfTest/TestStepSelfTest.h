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
// This contains CTestStepSelfTest which is the base class for all 
// the Psd Agx suite test steps
// 
//

#if (!defined __SELF_TEST_TESTSTEP_H__)
#define __SELF_TEST_TESTSTEP_H__

class CTestSuite;
class CTestSuiteSelfTest;

class CTestStepSelfTest : public CTestActiveStep
{
public:
	CTestStepSelfTest();
	~CTestStepSelfTest();
	virtual TInt CallStateMachine();
	// pointer to suite which owns this test 
	CTestSuiteSelfTest * iSelfTestSuite;

private:

};



#endif /* __SELF_TEST_TESTSTEP_H__ */
