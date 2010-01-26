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

#ifndef __TS_IPSECVPNAPI_H__
#define __TS_IPSECVPNAPI_H__

/**
 * @file ts_ipsec_main4.h header file for test code section 4.x for IPsec
 */

#include <networking/teststep.h>
#include "ts_ipsec_suite.h"

class CIpsecVpnTest : public CTestStep
	{
public:
	};

/**
 * @ Step 1: Created AOs and Loads policy
 */
class CIpsecVpnTest_1 : public CIpsecVpnTest
	{
public:
	CIpsecVpnTest_1(CTestScheduler* aScheduler);
	~CIpsecVpnTest_1();
	// from CTestStep
	virtual enum TVerdict doTestStepL();
private:
	CTestScheduler& iScheduler;
	};

/**
 * @ Step 1: Activates policy
 */
class CIpsecVpnTest_2 : public CIpsecVpnTest
	{
public:
	CIpsecVpnTest_2(CTestScheduler* aScheduler);
	~CIpsecVpnTest_2();
	// from CTestStep
	virtual enum TVerdict doTestStepL();
private:
	CTestScheduler& iScheduler;
	};



#endif
