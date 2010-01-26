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
//

/**
 @file ts_ipsec_step.h header file for IPsec test step
*/

#ifndef TS_IPSEC_STEP_H__
#define TS_IPSEC_STEP_H__


class CTestSuite;
class CTestSuiteIpsec;


/**
 * IPsec test section 1.x - test CIPSecMan
 */
class CTestStepIpsec1 : public CTestStep
	{
public:
	CTestStepIpsec1();
protected:
	CTestSuiteIpsec* iIpsecTestSuite;    //< pointer to suite which owns this test
	};


/**
 * IPsec test section 2.x - test CIPSecManPolicy
 */
class CTestStepIpsec2 : public CTestStep
	{
public:
	CTestStepIpsec2();
protected:
	CTestSuiteIpsec* iIpsecTestSuite;    //< pointer to suite which owns this test
	};


/**
 * IPsec test section 3.x - test CIPSecManPolicyList
 */
class CTestStepIpsec3 : public CTestStep
	{
public:
	CTestStepIpsec3();
protected:
	CTestSuiteIpsec* iIpsecTestSuite;    //< pointer to suite which owns this test
	};


/**
 * IPsec test section 4.x
 */
class CTestStepIpsec4 : public CTestStep
	{
public:
	CTestStepIpsec4();
protected:
	CTestSuiteIpsec* iIpsecTestSuite;    //< pointer to suite which owns this test
	};


#endif // TS_IPSEC_STEP_H__
