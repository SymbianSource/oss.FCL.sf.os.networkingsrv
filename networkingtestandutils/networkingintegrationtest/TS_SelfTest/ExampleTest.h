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
// ExampleTest.cpp
// This file contains an example Test step implementation 
// This demonstrates the various functions provided
// by the CTestStep base class which are available within
// a test step 
// 
//


#if (!defined __EXAMPLETEST_H__)
#define __EXAMPLETEST_H__


class CExampleTest : public CTestStepSelfTest
{
public:
	CExampleTest();
	~CExampleTest();
	virtual enum TVerdict doTestStepL( void );

};

class CExampleTest2 : public CTestStepSelfTest
{
public:
	CExampleTest2();
	~CExampleTest2();
	virtual enum TVerdict doTestStepL( void );

};

class CExampleTest3 : public CTestStepSelfTest
{
public:
	CExampleTest3();
	~CExampleTest3();
	virtual enum TVerdict doTestStepL( void );

};

class CExampleTest4 : public CTestStepSelfTest
{
public:
	CExampleTest4();
	virtual enum TVerdict doTestStepL( void );

};

class CExampleTest5 : public CTestStepSelfTest
{
public:
	CExampleTest5();
	virtual enum TVerdict doTestStepL( void );

};

class CPanicTest : public CTestStepSelfTest
{
public:
	CPanicTest();
	virtual enum TVerdict doTestStepL( void );

};

class CFlogTest : public CTestStepSelfTest
{
public:
	CFlogTest();
	virtual enum TVerdict doTestStepL( void );

};

class CTimerTest : public CTestStepSelfTest
{
public:
	CTimerTest();
	virtual enum TVerdict doTestStepL( void );

};

class CErrorCodeTest : public CTestStepSelfTest
{
public:
	CErrorCodeTest();
	virtual enum TVerdict doTestStepL( void );

};

class CSchedulerTest : public CTestStepSelfTest
{
public:
	enum TestState
		{
		EStepOne,
		EStepTwo,
		EStepThree
		};
	CSchedulerTest();
	virtual enum TVerdict doTestStepL( void );
	virtual TInt CallStateMachine();
private:
	RTimer iTimer;
	TestState iState;

};
#endif //(__EXAMPLETEST_H__)
