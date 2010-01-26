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
#include "te_dndinit.h"

CTestStepDND_Init::CTestStepDND_Init(CDndTestServer *apDndTestServer /*= NULL*/)
                    :CTestStepDND(apDndTestServer)
    {
    // Call base class method to set up the human readable name for INFO_PRINTF1ging
    SetTestStepName(KTestStepDND_Report);
    }

CTestStepDND_Init::~CTestStepDND_Init()
    {
    }



TVerdict CTestStepDND_Init::doTestStepL()
    {
    SetTestStepResult(EFail);
    INFO_PRINTF1(KNewLine);
    INFO_PRINTF1(_L("Initializing DND test engine..."));
     
    TESTL(StartUpDND());
    //StartUpDND();
    SetTestStepResult(EPass);
    return TestStepResult();
}

/**
*   Start up the DND.
*   @return ETrue on success.
*/
TBool   CTestStepDND_Init::StartUpDND()
    {
    
    TRequestStatus status = KErrNone;
    //-- magic hostname, shall always be in tcp.ini file
    //-- we will try to resolve this name to start DND engine
        
    INFO_PRINTF1(_L("Starting the DND engine..."));
    INFO_PRINTF1(_L("Startind RSocket..."));
	ipTestServer->iSocketServ.Connect();
	INFO_PRINTF1(_L("RSockServ Connect done\n"));
     	_LIT(KTestName1,"nokia.com");
	//_LIT(KTestName2,"xyz.com");
	TName myHostName1 = KTestName1();
	TNameEntry myResolvedName1;
	
    ipTestServer->iHostResolver.Open(ipTestServer->iSocketServ,KAfInet,KProtocolInetUdp);
    INFO_PRINTF1(_L("RHostResolver Opened ..."));
    INFO_PRINTF1(_L(" GetByName Called "));
    ipTestServer->iHostResolver.GetByName(myHostName1,myResolvedName1,status);
    User::WaitForRequest(status);

	if(status.Int() == KErrNone)
	{
	INFO_PRINTF1(_L("RHostResolver:: GetByName successful for nokia.com"));
	}
	else
	{
		INFO_PRINTF1(_L("RHostResolver:: GetByName failed for nokia.com "));
		INFO_PRINTF1(_L("Unable to start !"));
    
                //return EFalse;
	}

     _LIT(KTestName2,"localhost");
	TName myHostName2 = KTestName2();
	TNameEntry myResolvedName2;
	
    ipTestServer->iHostResolver.Open(ipTestServer->iSocketServ,KAfInet,KProtocolInetUdp);
    INFO_PRINTF1(_L("RHostResolver Opened ..."));
    INFO_PRINTF1(_L(" GetByName Called "));
    ipTestServer->iHostResolver.GetByName(myHostName2,myResolvedName2,status);
    User::WaitForRequest(status);

	if(status.Int() == KErrNone)
	{
	INFO_PRINTF1(_L("RHostResolver:: GetByName successful for localhost "));
	}
	else
	{
		INFO_PRINTF1(_L("RHostResolver:: GetByName failed for localhost "));
		INFO_PRINTF1(_L("Unable to start !"));
    
                //return EFalse;
	}


	ipTestServer->iHostResolver.Close();
	ipTestServer->iSocketServ.Close();
	return ETrue;
    }
