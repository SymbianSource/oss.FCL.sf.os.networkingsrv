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

/**
 @file ts_ipsec_suite.cpp implements the IPsec test suite
*/
#include <c32root.h>

#include <networking/log.h>
#include <networking/teststep.h>
#include <networking/testsuite.h>
//#include "ts_ipsec_main.h"
//#include "ts_ipsec_main2.h"
//#include "ts_ipsec_main3.h"
//#include "ts_ipsec_main4.h"
//#include "ts_ipsec_step.h"
#include "ts_ipsec_suite.h"

#include "ts_ipsec_crypto.h"
#include "ts_ipsec_polapi.h"
#include "ts_ipsec_vpnapi.h"
#include "ts_ipsec_rconn.h"

#ifndef EKA2
GLDEF_C TInt E32Dll(enum TDllReason)
/**
 * required for all DLLs but not used
 */
	{
	return KErrNone;
	}
#endif // EKA2

EXPORT_C CTestSuiteIpsec* NewTestSuiteIpsecL()
/**
 * this function is exported at ordinal 1.
 * it provides the interface to allow schedule test
 * to create instances of this test suite
 *
 * @return pointer to newly created CTestSuiteIpsec object
 */
    { 
	return new (ELeave) CTestSuiteIpsec;
    }


void CTestSuiteIpsec::AddThisTestStepL(CTestStep* aPtrTestStep)
/**
 * Add a test step into the suite
 */
	{
	// add the step using tyhe base class method
	AddTestStepL(aPtrTestStep);
	}


void CTestSuiteIpsec::InitialiseL( void )
/**
 * constructor for IPSec test suite
 * this creates all the IPSec test steps and
 * stores them inside CTestSuiteEsock
 */
	{
	// Start the Comms Process, but avoid loading the Phonebook Synchronizer
	_LIT(KPhbkSyncCMI, "phbsync.cmi");
	TInt err=StartC32WithCMISuppressions(KPhbkSyncCMI);
	User::LeaveIfError(err);

	// section 1.x
//	AddThisTestStepL( new (ELeave) CIpsecTest1_1 );

	// section 2.x
//	AddThisTestStepL( new (ELeave) CIpsecTest2_1 );

	// section 3.x
//	AddThisTestStepL( new (ELeave) CIpsecTest3_1 );
//	AddThisTestStepL( new (ELeave) CIpsecTest3_3 );
//	AddThisTestStepL( new (ELeave) CIpsecTest3_4 );
//	AddThisTestStepL( new (ELeave) CIpsecTest3_5 );
//	AddThisTestStepL( new (ELeave) CIpsecTest3_6 );

	// section 4.x
//	AddThisTestStepL( new (ELeave) CIpsecTest4_1 );

	// Start the active scheduler before doing this
	
	iScheduler = new(ELeave) CTestScheduler;

	// Section 5 - ipsecpolapi tests
	AddThisTestStepL( new (ELeave) CIpsecPolTest_1(iScheduler) );
	AddThisTestStepL( new (ELeave) CIpsecPolTest_2(iScheduler) );

	// Section 6 - vpnapi tests
	//AddThisTestStepL( new (ELeave) CIpsecVpnTest_1(iScheduler) );

	// Section 7 - Ipsec Connection tests
	AddThisTestStepL( new (ELeave) CIpsecConnTest_1(iScheduler) );
	AddThisTestStepL( new (ELeave) CIpsecConnTest_2(iScheduler) );
	AddThisTestStepL( new (ELeave) CIpsecConnTest_3(iScheduler) );
	AddThisTestStepL( new (ELeave) CIpsecConnTest_4(iScheduler) ); // IPSec plug-in removed
	AddThisTestStepL( new (ELeave) CIpsecTestCypto() );
	}

CTestSuiteIpsec::~CTestSuiteIpsec()
	{
	// Get rid of the AS if present
	if (iScheduler)
		delete iScheduler;
	
	}

TInt CTestSuiteIpsec::HandleCompletion(TAny* aArgs)
	{
	TCallbackArgs* args = (TCallbackArgs*)aArgs;
	((CTestScheduler*)(CActiveScheduler::Current()))->SetResult(args);
	return KErrNone;
	}
/*
void CTestSuiteIpsec::Log( TRefByValue<const TDesC16> format, ... ) 
		{
		VA_LIST aList;
		VA_START( aList, format );

		TBuf <MAX_LOG_LINE_LENGTH> LineBuf;
		TestLog16Overflow iOverflow16;

		LineBuf.AppendFormatList( format, aList, &iOverflow16 );

		CTestSuite::Log(_L("%S"),&LineBuf);
		VA_END( aList );
		}
*/
