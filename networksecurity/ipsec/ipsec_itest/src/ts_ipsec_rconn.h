// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef __TS_IPSECRCONN_H__
#define __TS_IPSECRCONN_H__

#include <networking/teststep.h>
#include "ts_ipsec_suite.h"

#include <in_sock.h>

class CIpsecConnTest : public CTestStep
	{
public:
	TBool GetIpAddressFromConfig(const TDesC &aSectName,const TDesC &aKeyName,TInetAddr &anAddr)
		{
		TPtrC result;
		TBool bRet;
		TInt nRet;

		// get string from config file
		bRet = GetStringFromConfig(aSectName, aKeyName, result);

		if (bRet) // string was retrieved successfully
			{
			// convert to IP address
			nRet = anAddr.Input(result);

			// if IP address is invalid
			if (nRet != KErrNone)
				{
				// display error message
				Log(_L("Invalid IP address, section:%S key:%S "),
					&aSectName, &aKeyName );

				bRet = EFalse;
				}
			}

		return bRet;
		}
	};

/**
 * @ Step 1: Starts and stops connection, getting progress
 */
class CIpsecConnTest_1 : public CIpsecConnTest
	{
public:
	CIpsecConnTest_1(CTestScheduler* aScheduler);
	~CIpsecConnTest_1();
	// from CTestStep
	virtual enum TVerdict doTestStepL();
private:
	CTestScheduler& iScheduler;
	};

/**
 * @ Step 2: Starts and stops connection, getting progress
 */
class CIpsecConnTest_2 : public CIpsecConnTest
	{
public:
	CIpsecConnTest_2(CTestScheduler* aScheduler);
	~CIpsecConnTest_2();
	// from CTestStep
	virtual enum TVerdict doTestStepL();
private:
	CTestScheduler& iScheduler;
	};

/**
 * @ Test 3: Starts and stops the main connection first
 */
class CIpsecConnTest_3 : public CIpsecConnTest
	{
public:
	CIpsecConnTest_3(CTestScheduler* aScheduler);
	~CIpsecConnTest_3();
	// from CTestStep
	virtual enum TVerdict doTestStepL();
private:
	CTestScheduler& iScheduler;
	};

/**
* @ Test  4 : Tests that a valid Kerr returned when IPSec plugin removed and connection with IPSec started
*/
class CIpsecConnTest_4 : public CIpsecConnTest
	{
public:
	CIpsecConnTest_4(CTestScheduler* aSceduler);
	~CIpsecConnTest_4();	
	// from CTestStep
	virtual enum TVerdict doTestStepL();
private:
	CTestScheduler& iScheduler;	
	};

#endif
