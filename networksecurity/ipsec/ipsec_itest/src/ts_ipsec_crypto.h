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

/**
 @file ts_ipsec_crypto.h header file for main test code for IPsec
*/

#ifndef TS_IPSEC_CRYPTO_H__
#define TS_IPSEC_CRYPTO_H__

#include <networking/teststep.h>
#include "ts_ipsec_suite.h"

/**
 * @test 1
 * Check Crypto strength
 */
class CIpsecTestCypto : public CTestStep
	{
public:
	CIpsecTestCypto();
	// from CTestStep
	virtual enum TVerdict doTestStepL();
	};

#endif // TS_IPSEC_CRYPTO_H__
