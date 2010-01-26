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
 @file ts_ipsec_main2.h header file for test code section 2.x for IPsec
*/

#ifndef TS_IPSEC_MAIN2_H__
#define TS_IPSEC_MAIN2_H__

#include "ts_ipsec_step.h"


/**
 * @test 2.1 Compare policies
 *
 * Parameters
 *
 * <pre>
 *   PolicyFile       - name of .pol policy file
 *   PolicyName       - name of the policy
 *   PolicyStatus     - expected status (ACTIVE or INACTIVE)
 * </pre>
 */
class CIpsecTest2_1 : public CTestStepIpsec2
	{
public:
	CIpsecTest2_1();
	// from CTestStep
	virtual enum TVerdict doTestStepL();
	};




#endif // TS_IPSEC_MAIN2_H__
