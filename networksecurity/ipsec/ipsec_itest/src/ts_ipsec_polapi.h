// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef __TS_IPSECPOLAPI_H__
#define __TS_IPSECPOLAPI_H__

/**
 * @file ts_ipsec_main4.h header file for test code section 4.x for IPsec
 */

#include <networking/teststep.h>
#include "ts_ipsec_suite.h"

class CIpsecPolTest : public CTestStep
	{
public:
	HBufC8* LoadLC(const TDesC& aName);
	void PrintPolicyL(const HBufC8* aPolicy);
	};

/**
 * @ Step 1: Created AOs and Loads policy
 */
class CIpsecPolTest_1 : public CIpsecPolTest
	{
public:
	CIpsecPolTest_1(CTestScheduler* aScheduler);
	~CIpsecPolTest_1();
	// from CTestStep
	virtual enum TVerdict doTestStepL();
private:
	CTestScheduler& iScheduler;
	};

/**
 * @ Step 1: Activates policy
 */
class CIpsecPolTest_2 : public CIpsecPolTest
	{
public:
	CIpsecPolTest_2(CTestScheduler* aScheduler);
	~CIpsecPolTest_2();
	// from CTestStep
	virtual enum TVerdict doTestStepL();
private:
	CTestScheduler& iScheduler;
	};



#endif
