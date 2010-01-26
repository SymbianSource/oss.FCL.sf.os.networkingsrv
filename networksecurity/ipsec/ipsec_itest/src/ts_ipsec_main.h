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
 @file ts_ipsec_main.h header file for main test code for IPsec
*/

#ifndef TS_IPSEC_MAIN_H__
#define TS_IPSEC_MAIN_H__

#include "ts_ipsec_step.h"


/**
 * @test 1.1 - Notify Policy installed. Notify IPsec manager that a security
 * policy was just installed. IPSec Manager then performs any actions that are
 * required on this event.In particular, IPSec Manager moves the installed
 * policy files from a temporary installation location to their final locations.
 * Normally used by the IPSECINS application.
 */
class CIpsecTest1_1 : public CTestStepIpsec1
	{
public:
	CIpsecTest1_1();
	// from CTestStep
	virtual enum TVerdict doTestStepL();
	};


/**
 * @test 1.2 - Deactivate IPSec when running. Deactivate IPsec completely. This
 * method clears the Security Policy Database and stops the Key Management Daemon.
 */
class CIpsecTest1_2 : public CTestStepIpsec1
	{
public:
	CIpsecTest1_2();
	// from CTestStep
	virtual enum TVerdict doTestStepL();
	};


/**
 * @test 1.3 - Deactivate IPSec when stopped. Deactivate IPsec completely. This
 * method clears the Security Policy Database and stops the Key Management Daemon.
 */
class CIpsecTest1_3 : public CTestStepIpsec1
	{
public:
	CIpsecTest1_3();
	// from CTestStep
	virtual enum TVerdict doTestStepL();
	};


#endif // TS_IPSEC_MAIN_H__
