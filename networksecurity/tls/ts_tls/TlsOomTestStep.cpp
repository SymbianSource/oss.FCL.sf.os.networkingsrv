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
// This contains TLS out of memory test
// 
//

// EPOC includes
#include <e32base.h>

#include <ssl.h>

// from t_tls.cpp
#include <e32cons.h>
#include <c32comm.h>
#include <f32file.h>
#include <es_sock.h>

#include <securesocketinterface.h>
#include <securesocket.h>

// Test system includes
#include <networking/log.h>
#include <networking/teststep.h>

#include "T_TLS_test.h"
#include "TestSuiteTls.h"
#include "TlsOomTestStep.h"
#include "T_TLS_cntrl.h"
#include "t_oomClientTest.h"
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <securesocket_internal.h>
#endif

TInt TLSOOMTest(TAny* oOMFailureThresholdAny);


_LIT( KTxtTLS, "T_TLS" );
_LIT( KTxtTLSThreshold, "OOM Threshold: %d" );
_LIT( KTxtTLSStarting, "Starting OOM Test" );


CTlsOomTest::CTlsOomTest()
/** 
 * Constructor.
 * Stores the name of this test case.
 */
{
	iTestStepName = _L("tls_oom");
}

CTlsOomTest::~CTlsOomTest()
/**
 * Destructor
 */
{
}


TVerdict CTlsOomTest::doTestStepL( )
/** 
 * This is the test code for out of memory test.
 */
{
	COomTestData TestData;
	TInt OOMaxThreshold;

	// if the test has not left yet it must be a Pass 
	iTestStepResult = EPass;

	// get the Failure threshold from the test ini file
	if (!GetIntFromConfig(KSectionName, KCfgFailureThreshold, TestData.iOOMThreshold ))
			TestData.iOOMThreshold = KDefCfgFailureThreshold;

	if (!GetIntFromConfig(KSectionName, KCfgMaxThreshold, OOMaxThreshold ))
			OOMaxThreshold = KDefCfgMaxThreshold;

	Log( _L("Test out of memory (__UHEAP_FAILNEXT) values between:%d and %d"), 
			TestData.iOOMThreshold, OOMaxThreshold );

	// get the ip address and port from ini file 
	TPtrC aPtrResult;
	TPtrC* res=&aPtrResult;
	if ( GetStringFromConfig(KSectionName, KCfgIPAddress, aPtrResult))
		TestData.iAddress.Copy( aPtrResult );
	else
		TestData.iAddress.Copy( KDefCfgIPAddress );

	if (!GetIntFromConfig(KSectionName, KCfgIPPort, TestData.iPortNumber ))
			TestData.iPortNumber = KDefCfgIPPort;

	Log( _L("IPaddress: %S port:%d"), res, TestData.iPortNumber );

	__UHEAP_MARK;

	// create a global semaphore that the dialog server searches for to
	// decide if "trust" dialogs should be displayed or not
	RSemaphore	iSemaphore;
	_LIT( KSemaphoreName, "T_AUTOSSL" );
	if ( iSemaphore.CreateGlobal( KSemaphoreName, 0 ) != KErrNone )
		{
		Log( _L("Semaphore creation failed.") );
		}
	TRequestStatus stat;
	TBuf<30> threadName;
	_LIT(KthreadNameBase,"OOMTLS");
	const TUint KDefaultHeapSize=0x10000;
	const TUint KMaxHeapSize=0x80000;
	do
	{
		//Spawn OOM test in its own thread
		RThread t;
		threadName.Copy(KthreadNameBase);
		threadName.AppendNum( TestData.iOOMThreshold );//OMFailureThreshold);
		
		TInt res=t.Create(threadName, TLSOOMTest,KDefaultStackSize,KDefaultHeapSize,KMaxHeapSize,(void*)&TestData);

		Log(KTxtTLSStarting);
		Log(KTxtTLSThreshold,TestData.iOOMThreshold );//OMFailureThreshold);
		t.Logon(stat);
		t.Resume();
		User::WaitForRequest(stat);
		Log(_L("TLS OOM test thread terminated with code: %d state: %d"),stat.Int(), TestData.iRunStep);
		switch (stat.Int())
			{
			case KErrCompletion:
				Log(_L("Secure Socket did not connect"));
				break;
			case KErrNone:
			case KErrEof:
				Log(_L("Secure Socket DID connect\nTest Run complete "));
				break;
			default:
				Log(_L("Secure Socket did not connect "));
				break;
			}

		TestData.iOOMThreshold ++;

		if ( OOMaxThreshold < TestData.iOOMThreshold )
			{
			// if this happens then either:
			// OOMaxThreshold is not high enough (has worked at 846, so 860 sould be ok)
			// or TLS cannot connect to the SSL server because of a test network or server problem.
			// so record this as inconclusive
			Log(_L("Max OOM threshold reached so Test cancelled"));
			iTestStepResult = EInconclusive;
			break;
			}
#ifdef __WINS__
	User::After(3000000); //give openssl change to recover
#endif
	}while(stat!=KErrNone);

	iSemaphore.Close();

	__UHEAP_MARKEND;

	return iTestStepResult;
}

TInt TLSOOMTest(TAny* oAny)
	{

	TInt ret=KErrNone;
	CTrapCleanup* cleanup=CTrapCleanup::New(); // get clean-up stack
	// Create an active scheduler
	CActiveScheduler* myActiveScheduler;
	myActiveScheduler = new(ELeave) CActiveScheduler();
	CActiveScheduler::Install( myActiveScheduler );

	//Fetches params
	COomTestData * TestData = (COomTestData *)oAny;

	__UHEAP_MARK;
	ClientOOMTest* myTest=0;
	TRAP(ret,myTest = ClientOOMTest::NewL(myActiveScheduler));

	if(ret == KErrNone)
		{
		myTest->SetOOMThreshold(TestData->iOOMThreshold );
		myTest->SetIpAddress( TestData->iAddress );
		myTest->SetIpPort( TestData->iPortNumber );

		myTest->Start();
		// Start the scheduler
		myActiveScheduler->Start();
		// return last completion code
      TestData->iRunStep = myTest->State();
		ret = myTest->Error();
		//Force cleaning up of Thread local storage in the loaded DLLs
		//!!!Note only needed to satisfy the OOM testing
		//In normal use Thread Local Storage gets cleared in the E32Dll(EThreadDetach)
		delete myTest;
		}
	CSecureSocketLibraryLoader::Unload();
	__UHEAP_MARKEND;

	// Remove objects from the cleanup stack
	delete myActiveScheduler;
	delete cleanup;
	return ret;
	}
