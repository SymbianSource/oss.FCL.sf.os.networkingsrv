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
// ts_ipsec_polapi.cpp
// 
//

/**
 @file ts_ipsec_polapi.cpp Implements main test code for IPsec
*/

#include <networking/log.h>

#include "ts_ipsec_crypto.h"

#if defined(SYMBIAN_CRYPTO)
#include <cryptostrength.h>
#else
#include <cryptalg.h>
#endif

CIpsecTestCypto::CIpsecTestCypto()
/**
 * Each test step initialises it's own name. Stores the name of this test case.
 * This is the name that is used by the script file
 */
	{
	_LIT(KTestStepName, "IpsecTestCypto");
	iTestStepName.Copy(KTestStepName);
	}

enum TVerdict CIpsecTestCypto::doTestStepL()
	{
	__UHEAP_MARK;

	/**
	 * checking crypto libraries
	 */
	const TCrypto::TStrength strength = TCrypto::Strength();
	
	switch (strength)
		{
#if defined(SYMBIAN_CRYPTO)
	case TCrypto::EWeak:
#else
	case TCrypto::ECrypto_40:
	case TCrypto::ECrypto_56:
	case TCrypto::ECrypto_64:
	case TCrypto::ECrypto_128:
#endif
		Log(_L("Error!!! Your crypto library is too weak !!!"));
		Log(_L("         need at least 256 bits strength."));
		Log(_L("         Use strong Crypto"));
		User::Invariant();
		break;
			
	default:
		Log(_L("Crypto strength is appropriate"));
		break;
		}
	
	// Find the verdict from the callbacks
	__UHEAP_MARKEND;
	return EPass;
	}
