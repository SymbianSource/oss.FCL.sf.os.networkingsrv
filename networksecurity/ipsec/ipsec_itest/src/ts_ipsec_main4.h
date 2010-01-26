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
 @file ts_ipsec_main4.h header file for test code section 4.x for IPsec
*/

#ifndef TS_IPSEC_MAIN4_H__
#define TS_IPSEC_MAIN4_H__

#include "ts_ipsec_step.h"


/**
 * @test 4.1 OOM - Out of Memory testing
 */
class CIpsecTest4_1 : public CTestStepIpsec4
	{
public:
	CIpsecTest4_1();
	// from CTestStep
	virtual enum TVerdict doTestStepL();
protected:
	void TestOomPolicyListL();
	};


#endif // TS_IPSEC_MAIN4_H__
