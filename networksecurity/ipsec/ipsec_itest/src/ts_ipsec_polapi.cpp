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
 @file ts_ipsec_polapi.cpp Implements main test code for IPsec
*/

#include <networking/log.h>
#include <utf.h>

#include "PolicyTester.h" // gets the active object
#include "ts_ipsec_polapi.h"

namespace
	{
	HBufC8* ReadPolicyLC(const TFileName& aPolicyName)
		{
		// Load policy from the file
		RFs fsSession;
		User::LeaveIfError(fsSession.Connect());
		CleanupClosePushL(fsSession);

		RFile file;
		User::LeaveIfError(file.Open(fsSession,aPolicyName,EFileShareReadersOnly));
		
		TInt size;
		User::LeaveIfError(file.Size(size));
		HBufC8* policy = HBufC8::NewLC(size);
		TPtr8 Ptr = policy->Des();
		User::LeaveIfError(file.Read(Ptr));
		CleanupStack::Pop(); // policy
		CleanupStack::PopAndDestroy(); //fsSession
		CleanupStack::PushL(policy);
		return policy;
		}
	}

void CIpsecPolTest::PrintPolicyL(const HBufC8* aPolicy)
	{
	TBuf<100> uPolicy;
	User::LeaveIfError(CnvUtfConverter::ConvertToUnicodeFromUtf8(uPolicy, *aPolicy));
	Log(uPolicy);
	}

HBufC8* CIpsecPolTest::LoadLC(const TDesC& aName)
	{
	TPtrC poldir;

	_LIT(KThisSection, "IpsecPolicyTest");
	
	const TBool b = GetStringFromConfig(KThisSection, aName, poldir);
	if(!b)
		{
		Log(_L("Could not read poldir from config"));
		TESTL(b);
		}

	TFileName file(poldir);
	
	HBufC8* policy = ReadPolicyLC(file);

	Log(_L("From File %S, Loading policy - "), &file);
	PrintPolicyL(policy);
	return policy;
	}

CIpsecPolTest_1::CIpsecPolTest_1(CTestScheduler* aScheduler) : iScheduler(*aScheduler)
/**
 * Each test step initialises it's own name. Stores the name of this test case.
 * This is the name that is used by the script file
 */
	{
	_LIT(KTestStepName, "IpsecPolicyTest1");
	iTestStepName.Copy(KTestStepName);
	}

CIpsecPolTest_1::~CIpsecPolTest_1()
	{
	}

enum TVerdict CIpsecPolTest_1::doTestStepL()
	{
	__UHEAP_MARK;

	// Create the active objects
	CPolicyTester* tester = CPolicyTester::NewLC((CTestSuiteIpsec*)this->iSuite);
	
	_LIT(KPolicyNum, "Policy1");
	HBufC8* policy = LoadLC(KPolicyNum);
	tester->StartPolicyLoadL(policy);	// transfers policy ownership
	CleanupStack::Pop();

	// The scheduler starts, and completes when the AOs are done
	iScheduler.Start();

	TInt err = tester->GetResult();

	Log(_L("The policy ID %d was loaded with result %d"), tester->GetId(), err);

	if (!err)
		tester->StartActivatePolicyL();
		
	iScheduler.Start();

	err = tester->GetResult();

	Log(_L("The policy ID %d was activated with result %d"), tester->GetId(), err);
	//test code added

	
	TIpsecSaSpec saspec;
	TPckg<TIpsecSaSpec> saspecpckg(saspec);
	
	TIpsecSelectorInfo selectorinfo;
		
        const TUint32 KInetAddrRemote = INET_ADDR(192,168,10,3);
        selectorinfo.iRemote.SetAddress( KInetAddrRemote );
	
        const TUint32 KInetAddrRemoteMask = INET_ADDR(255,255,255,255);
        selectorinfo.iRemoteMask.SetAddress(KInetAddrRemoteMask);
    
        selectorinfo.iSaIndex = 0;
    
        TPckgC<TIpsecSelectorInfo> selectorinfopckg(selectorinfo);
	
	err = tester->StartMatchPolicyL(selectorinfopckg, saspecpckg);
	Log(_L("The policy ID %d returned matchselector request completion with result %d"), tester->GetId(), err);
	
	iScheduler.Start();

	err = tester->GetResult();
	Log(_L("The policy ID %d returned matchselector Get Result request completion with result %d"), tester->GetId(), err);

	if(err)
	    {
	    CleanupStack::PopAndDestroy(tester);
	    __UHEAP_MARKEND;
	    return EFail;
	    }
	
	Log(_L("Identity reference is %S"), &saspec.iName);
	Log(_L("Transport mode is %d"), saspec.iTransportMode);
	Log(_L("More SA's exist ? %d"), saspec.iMoreSasExist);
	Log(_L("SA type is %u"), saspec.iType);
	Log(_L("Authentication algorithm type is %u"), saspec.iAalg);
	Log(_L("Authentication Algorithm length is %u"), saspec.iAalgLen);
	Log(_L("Encryption Algorithm type is %u"), saspec.iEalg);
	Log(_L("Encryption Algorithm length is %u"), saspec.iEalgLen);
	Log(_L("Replay Window length is %u"), saspec.iReplayWindowLength);
	Log(_L("Perfect forward secrecy is %u"), saspec.iPfs);
	Log(_L("Source Specific is %u"), saspec.iSrcSpecific);
	
	// sanity test changes where iSrcSpecific bit field is added to TIpsecSaSpec struct
	if(saspec.iSrcSpecific !=1) // src_specific is set in 1ipsec_onlypol policy file
	    {
	    CleanupStack::PopAndDestroy(tester);
	    __UHEAP_MARKEND;
	    return EFail;
	    }

	// Test code added for PDEF134521
	
	TInetAddr remoteGatewayAddr;
	CArrayFixFlat<TIpsecSelectorInfo>* selectorInfoArray = new (ELeave) CArrayFixFlat<TIpsecSelectorInfo>(2);
	
	CleanupStack::PushL(selectorInfoArray);
	selectorInfoArray->SetReserveL(1);
	
	const TUint32 KInetAddrRemoteGateway = INET_ADDR(192,168,10,3);
    remoteGatewayAddr.SetAddress( KInetAddrRemoteGateway );
	TInetAddrPckg remoteGatewayAddrPckg(remoteGatewayAddr);
	
	err = tester->StartAvailableSelectorsL(remoteGatewayAddrPckg, selectorInfoArray);
	iScheduler.Start();
	
	err = tester->GetResult();
	Log(_L("The policy ID %d returned Available Selectors Get Result request completion with result %d"), tester->GetId(), err);
	
	if(err)
	    {
	    CleanupStack::PopAndDestroy(tester);
	    __UHEAP_MARKEND;
	    return EFail;
	    }
	
	TBuf<60> addr1, addr2, addr3, addr4, addr5;
	TIpsecSelectorInfo selectorInfo; 
	
	for(int i =0; i<selectorInfoArray->Count(); i++)  
		{
		selectorInfo = selectorInfoArray->At(i);
		selectorInfo.iRemote.Output(addr1);
		selectorInfo.iRemoteMask.Output(addr2);
		
		selectorInfo.iLocal.Output(addr3);
		selectorInfo.iLocalMask.Output(addr4);
		
		selectorInfo.iTunnel.Output(addr5);
		
		Log(_L("Direction %d"), selectorInfo.iDirection);
		
		Log(_L("Remote Port %d"), selectorInfo.iRemote.Port());
		Log(_L("Remote Host Address %S"), &addr1);
		Log(_L("Remote Host Mask %S"), &addr2);
		
		Log(_L("Local Port %d"), selectorInfo.iLocal.Port());
		Log(_L("Local Host Address %S"), &addr3);
		Log(_L("Local Host Mask %S"), &addr4);
		
		Log(_L("Remote Gateway Address %S"), &addr5);
		
		Log(_L("Protocol %d"), selectorInfo.iProtocol);
		
		Log(_L("SA index %d"), selectorInfo.iSaIndex);
		}
		
	CleanupStack::PopAndDestroy(selectorInfoArray);

	// End test case for PDEF134521

	tester->StartUnloadPolicyL();

	iScheduler.Start();

	err = tester->GetResult();

	Log(_L("The policy ID %d was unloaded with result %d"), tester->GetId(), err);
	
	// Get rid of the Active objects
	CleanupStack::PopAndDestroy(tester);
	
	// Find the verdict from the callbacks
	__UHEAP_MARKEND;
	return err?EFail:EPass;
	}

CIpsecPolTest_2::CIpsecPolTest_2(CTestScheduler* aScheduler) : iScheduler(*aScheduler)
/**
 * Each test step initialises it's own name. Stores the name of this test case.
 * This is the name that is used by the script file
 */
	{
	_LIT(KTestStepName, "IpsecPolicyTest2");
	iTestStepName.Copy(KTestStepName);
	}

CIpsecPolTest_2::~CIpsecPolTest_2()
	{
	}

enum TVerdict CIpsecPolTest_2::doTestStepL()
	{
	__UHEAP_MARK;

	CPolicyTester* tester = CPolicyTester::NewLC((CTestSuiteIpsec*)this->iSuite);
	CPolicyTester* tester2 = CPolicyTester::NewLC((CTestSuiteIpsec*)this->iSuite);
	
	_LIT(KPolicyNum, "Policy1");
	HBufC8* policy = LoadLC(KPolicyNum);

	_LIT(KPolicy2Num, "Policy2");
	HBufC8* policy2 = LoadLC(KPolicy2Num);

	tester->StartPolicyLoadL(policy);	// transfers policy ownership
	tester2->StartPolicyLoadL(policy2);	// transfers policy ownership
	
	CleanupStack::Pop(2);

	// The scheduler starts, and completes when the AOs are done
	iScheduler.Start();

	TInt err = tester->GetResult();
	Log(_L("The policy ID %d was loaded with result %d"), tester->GetId(), err);

	TInt err2 = tester2->GetResult();
	Log(_L("The policy ID %d was loaded with result %d"), tester2->GetId(), err2);

	if (!err)
		tester->StartActivatePolicyL();
	
	if (!err2)
		tester2->StartUnloadPolicyL();
	
	iScheduler.Start();

	err = tester->GetResult();
	Log(_L("The policy ID %d was activated with result %d"), tester->GetId(), err);
	
	err2 = tester2->GetResult();
	Log(_L("The policy ID %d was unloaded with result %d"), tester2->GetId(), err2);

	if (!err)
		tester->StartUnloadPolicyL();

	iScheduler.Start();

	err = tester->GetResult();
	Log(_L("The policy ID %d was unloaded with result %d"), tester->GetId(), err);
	
	// Get rid of the Active objects
	CleanupStack::PopAndDestroy(tester2);
	CleanupStack::PopAndDestroy(tester);
	
	// Find the verdict from the callbacks
	__UHEAP_MARKEND;
	return err?EFail:EPass;
	}
