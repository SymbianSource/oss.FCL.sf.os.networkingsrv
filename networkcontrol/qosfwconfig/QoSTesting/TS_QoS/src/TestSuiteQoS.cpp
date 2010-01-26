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
// This main DLL entry point for the TS_QoS.dll
//



// EPOC includes
#include <e32base.h>

#include <f32file.h>
#include <s32file.h>

// Test system includes
#include "Log.h"
#include "TestStep.h"
#include "TestStepQoS.h"
#include "QoSTest.h"
#include "TestSuite.h"
#include "TestStepQoS.h"
#include "TestSuiteQoS.h"


GLREF_C void CommInitL(TBool aEnhanced);

// required for all DLLs but not used

// NewTestEtelPacket is exported at ordinal 1
// this provides the interface to allow schedule test
// to create instances of this test suite
EXPORT_C CTestSuiteQoS* NewTestSuiteQoSL() 
    { 
    return new (ELeave) CTestSuiteQoS;
    }

// destructor
CTestSuiteQoS::~CTestSuiteQoS()
    {
    delete iScheduler;
    }

// make a version string available for test system 
_LIT(KTxtVersion,"1.0");
TPtrC CTestSuiteQoS::GetVersion( void )
    {
    return KTxtVersion();
    }

// Add a test step into the suite
void CTestSuiteQoS::AddTestStepL( CTestStepQoS * aPtrTestStep )
    {
    // test steps contain a pointer back to the suite which owns them
    aPtrTestStep->iQoSSuite = this; 

    // add the step using tyhe base class method
    CTestSuite::AddTestStepL(aPtrTestStep);
    }


// constructor for QoS test suite
// this creates all the QoS test steps and
// stores them inside CTestSuiteQoS
void CTestSuiteQoS::InitialiseL( void )
    {
    RLibrary lib;
    RProcess p;


    // add test steps
    AddTestStepL(new(ELeave) CQoSTest_001);
    AddTestStepL(new(ELeave) CQoSTest_002);
    AddTestStepL(new(ELeave) CQoSTest_003);

    AddTestStepL(new(ELeave) CQoSTest_010);
    AddTestStepL(new(ELeave) CQoSTest_011);
    AddTestStepL(new(ELeave) CQoSTest_012);
    AddTestStepL(new(ELeave) CQoSTest_013);
    AddTestStepL(new(ELeave) CQoSTest_014);
    AddTestStepL(new(ELeave) CQoSTest_015);
    AddTestStepL(new(ELeave) CQoSTest_016);
    
/* 
 * These methods are valid for Symbian OS 9.x onwards.
 */ 
    AddTestStepL(new(ELeave) CQoSTest_072);
    AddTestStepL(new(ELeave) CQoSTest_073);
    AddTestStepL(new(ELeave) CQoSTest_075);
    AddTestStepL(new(ELeave) CQoSTest_076);

    AddTestStepL(new(ELeave) CQoSTest_090);
    AddTestStepL(new(ELeave) CQoSTest_091);
    AddTestStepL(new(ELeave) CQoSTest_092);
    AddTestStepL(new(ELeave) CQoSTest_093);
    AddTestStepL(new(ELeave) CQoSTest_094);
    AddTestStepL(new(ELeave) CQoSTest_101);
    AddTestStepL(new(ELeave) CQoSTest_102);
    AddTestStepL(new(ELeave) CQoSTest_103);
    AddTestStepL(new(ELeave) CQoSTest_104);

    AddTestStepL(new(ELeave) CQoSTest_902);

    AddTestStepL(new(ELeave) CQoSTest_120);
    AddTestStepL(new(ELeave) CQoSTest_121);
    AddTestStepL(new(ELeave) CQoSTest_122);
    AddTestStepL(new(ELeave) CQoSTest_123);
    AddTestStepL(new(ELeave) CQoSTest_124);
    AddTestStepL(new(ELeave) CQoSTest_125);
    AddTestStepL(new(ELeave) CQoSTest_126);
    AddTestStepL(new(ELeave) CQoSTest_127);
    AddTestStepL(new(ELeave) CQoSTest_128);
    AddTestStepL(new(ELeave) CQoSTest_129);
    AddTestStepL(new(ELeave) CQoSTest_130);
    AddTestStepL(new(ELeave) CQoSTest_131);
    AddTestStepL(new(ELeave) CQoSTest_132);
    AddTestStepL(new(ELeave) CQoSTest_134);
    AddTestStepL(new(ELeave) CQoSTest_135);
    AddTestStepL(new(ELeave) CQoSTest_136);
    AddTestStepL(new(ELeave) CQoSTest_142);
    AddTestStepL(new(ELeave) CQoSTest_143);
    AddTestStepL(new(ELeave) CQoSTest_144);
    AddTestStepL(new(ELeave) CQoSTest_145);
    AddTestStepL(new(ELeave) CQoSTest_146);

#ifdef SYMBIAN_NETWORKING_UMTSR5
    
    AddTestStepL(new(ELeave) CQoSTest_301);
    AddTestStepL(new(ELeave) CQoSTest_302);
    AddTestStepL(new(ELeave) CQoSTest_303);
    AddTestStepL(new(ELeave) CQoSTest_304);
    AddTestStepL(new(ELeave) CQoSTest_305);
    AddTestStepL(new(ELeave) CQoSTest_306);
    AddTestStepL(new(ELeave) CQoSTest_307);
    
    AddTestStepL(new(ELeave) CQoSTest_401);
    AddTestStepL(new(ELeave) CQoSTest_402);
    AddTestStepL(new(ELeave) CQoSTest_403);
    AddTestStepL(new(ELeave) CQoSTest_404);
    AddTestStepL(new(ELeave) CQoSTest_405);
    AddTestStepL(new(ELeave) CQoSTest_406);
    AddTestStepL(new(ELeave) CQoSTest_407);
    AddTestStepL(new(ELeave) CQoSTest_408);
    AddTestStepL(new(ELeave) CQoSTest_409);
    AddTestStepL(new(ELeave) CQoSTest_410);
    AddTestStepL(new(ELeave) CQoSTest_411);
    AddTestStepL(new(ELeave) CQoSTest_412);
    AddTestStepL(new(ELeave) CQoSTest_413);
    AddTestStepL(new(ELeave) CQoSTest_414);
    
#endif // SYMBIAN_NETWORKING_UMTSR5
    
    AddTestStepL(new(ELeave) CQoSTest_500);
    AddTestStepL(new(ELeave) CQoSTest_501);    

    AddTestStepL(new(ELeave) CQoSTest_700);
    AddTestStepL(new(ELeave) CQoSTest_701);
    AddTestStepL(new(ELeave) CQoSTest_702);
    AddTestStepL(new(ELeave) CQoSTest_703);
    AddTestStepL(new(ELeave) CQoSTest_704);
    AddTestStepL(new(ELeave) CQoSTest_705);
    AddTestStepL(new(ELeave) CQoSTest_706);
    AddTestStepL(new(ELeave) CQoSTest_707);
    AddTestStepL(new(ELeave) CQoSTest_708);
    
    
    AddTestStepL(new(ELeave) CQoSTest_050);
    AddTestStepL(new(ELeave) CQoSTest_051);
    AddTestStepL(new(ELeave) CQoSTest_052);
    AddTestStepL(new(ELeave) CQoSTest_053);
    AddTestStepL(new(ELeave) CQoSTest_054);
    AddTestStepL(new(ELeave) CQoSTest_055);
    AddTestStepL(new(ELeave) CQoSTest_056);
    AddTestStepL(new(ELeave) CQoSTest_057);
    AddTestStepL(new(ELeave) CQoSTest_058);
    AddTestStepL(new(ELeave) CQoSTest_059);
    AddTestStepL(new(ELeave) CQoSTest_060);
    AddTestStepL(new(ELeave) CQoSTest_061);
    AddTestStepL(new(ELeave) CQoSTest_062);
    AddTestStepL(new(ELeave) CQoSTest_064);

    AddTestStepL(new(ELeave) CQoSTest_070);
    AddTestStepL(new(ELeave) CQoSTest_071);
    AddTestStepL(new(ELeave) CQoSTest_077);

    AddTestStepL(new(ELeave) CQoSTest_202);
    AddTestStepL(new(ELeave) CQoSTest_203);

    AddTestStepL(new(ELeave) CQoSTest_601);

    // Create active scheduler for this suite. This is same for all test cases.
    iScheduler = new(ELeave) CMyScheduler;
    CActiveScheduler::Install(iScheduler);

    // Fix for NTRas comm bug: Load the LDD and PDD explicitly 
    CommInitL(EFalse);

    // Create folder for testing purposes
    RFs fs;
    CleanupClosePushL(fs);
    User::LeaveIfError(fs.Connect());
    TInt err = fs.MkDir(_L("c:\\QoSTest\\"));
    if(err != KErrNone && err != KErrAlreadyExists)
        {
        User::Leave(err);
        }
    CleanupStack::PopAndDestroy(); // fs
    }


//
//

void CMyScheduler::Error(TInt aError) const
    {
    _LIT(KTxtSchedulerError,"CMyScheduler - error");
    User::Panic(KTxtSchedulerError,aError);
    }

//


