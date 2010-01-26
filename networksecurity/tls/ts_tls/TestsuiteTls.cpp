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
// This main DLL entry point for the TS_Tls.dll
// 
//



// EPOC includes
#include <e32base.h>

#include <e32cons.h>
#include <c32comm.h>
#include <f32file.h>
#include <es_sock.h>
#include <in_sock.h>

//#include <ssl.h>
#include <securesocketinterface.h>
#include <securesocket.h>

// Test system includes
#include <networking/log.h>
#include <networking/teststep.h>
#include <networking/testsuite.h>

#include "TeststepTls.h"
#include "TestSuiteTls.h"

#include "T_Autossl_Const.h"
#include "TlsTestStep1.h"
#include "TlsTestSection2.h"
#include "TlsTestSection3.h"
#include "TlsOomTestStep.h"
#include "T_TLS_PSK_Test.h"
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <securesocket_internal.h>
#endif


EXPORT_C CTestSuiteTls* NewTestSuiteTls( void ) 
    { 
	return new (ELeave) CTestSuiteTls;
    }


CTestSuiteTls::~CTestSuiteTls()
/**
 * Destructor
 * Force an unload of the securesocket library, as it is not normally unloaded until
 * the thread detaches, but for OOM testing, need to explicitly unload it.
 */
{
	CSecureSocketLibraryLoader::Unload();
}

// make a version string available for test system 
#ifdef _DEBUG
_LIT(KTxtVersion,"1.1 (udeb)");
#else
_LIT(KTxtVersion,"1.1");
#endif

TPtrC CTestSuiteTls::GetVersion( void )
{
	return KTxtVersion();
}


void CTestSuiteTls::AddTestStepL( CTestStepTls * ptrTestStep )
/** 
 * This method adds a test step into the suite using the base class method.
 * Test steps contain a pointer back to the suite which owns them.
 */
{
	ptrTestStep->iTlsSuite = this; 
	CTestSuite::AddTestStepL(ptrTestStep);
}

void CTestSuiteTls::InitialiseL( void )
/**
 * This method is a second phase constructor for the Tls test suite.
 * It creates all the Tls test steps and stores them inside CTestSuiteTls.
 */
{
	
//	iSuiteName = _L("Tls");	// store the name of this test suite

	// Add the test steps
	AddTestStepL( new(ELeave) CTestStepT_Tls );
	AddTestStepL( new(ELeave) CTlsOomTest );
	AddTestStepL( new(ELeave) CTlsTestSection2_1 );
	AddTestStepL( new(ELeave) CTlsTestSection3_1 );
  	AddTestStepL( new(ELeave) CTlsRenegotiateTest );
  	AddTestStepL( new(ELeave) CTlsCancelRecvTest );
  	AddTestStepL( new(ELeave) CTlsOldGetOptsTest );
  	AddTestStepL( new(ELeave) CTlsOpenConnection );
  	AddTestStepL( new(ELeave) CTlsCloseConnection );
  	AddTestStepL( new(ELeave) CTlsFailSuiteSelection );
  	AddTestStepL( new(ELeave) CTestStepDialogMode_Tls );
	AddTestStepL( new(ELeave) CTlsPskTestStep );     	
}


