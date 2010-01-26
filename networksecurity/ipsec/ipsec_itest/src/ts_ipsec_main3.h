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
 @file ts_ipsec_main3.h header file for test code section 3.x for IPsec
*/

#ifndef TS_IPSEC_MAIN3_H__
#define TS_IPSEC_MAIN3_H__

#include "ts_ipsec_step.h"


/**
 * @test 3.1 Get policy count
 *
 * Parameters
 *
 * <pre>
 *   PolicyCount       - expected number of policies in the system
 * </pre>
 */
class CIpsecTest3_1 : public CTestStepIpsec3
	{
public:
	CIpsecTest3_1();
	// from CTestStep
	virtual enum TVerdict doTestStepL();
	};


/**
 * @test 3.3 Get policy details
 *
 * Parameters
 *
 * <pre>
 *   PolicyFile        - name of policy file
 *   PolicyName        - name of the policy
 *   PolicyVersion     - policy version
 *   PolicyDescription - policy description
 *   IssuerName        - issuer name
 *   ContactInfo       - e.g. email adress
 * </pre>
 */
class CIpsecTest3_3 : public CTestStepIpsec3
	{
public:
	CIpsecTest3_3();
	// from CTestStep
	virtual enum TVerdict doTestStepL();
	};


/**
 * @test 3.4 Delete policy
 *
 * Parameters
 *
 * <pre>
 *   PolicyName       - name of the policy to be deleted
 * </pre>
 */
class CIpsecTest3_4 : public CTestStepIpsec3
	{
public:
	CIpsecTest3_4();
	// from CTestStep
	virtual enum TVerdict doTestStepL();
	};


/**
 * @test 3.5 Activate policy
 *
 * Parameters
 *
 * <pre>
 *   PolicyName       - name of the policy to be activated
 * </pre>
 */
class CIpsecTest3_5 : public CTestStepIpsec3
	{
public:
	CIpsecTest3_5();
	// from CTestStep
	virtual enum TVerdict doTestStepL();
	};


/**
 * @test 3.6 Deactivate policy
 *
 * Parameters
 *
 * <pre>
 *   PolicyName       - name of the policy to be deactivated
 * </pre>
 */
class CIpsecTest3_6 : public CTestStepIpsec3
	{
public:
	CIpsecTest3_6();
	// from CTestStep
	virtual enum TVerdict doTestStepL();
	};


#endif // TS_IPSEC_MAIN3_H__
