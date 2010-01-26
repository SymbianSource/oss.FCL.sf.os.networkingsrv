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


#include "LlmnrTestStartStop.h"
#include <testexecutelog.h>
#include <c32comm.h>

CTestStepLLMNR_StartUp::CTestStepLLMNR_StartUp(CLlmnrTestServer *apLlmnrTestServer /*= NULL*/)
                       :CTestStepLLMNR(apLlmnrTestServer) 
    {
    // Call base class method to set up the human readable name for logging
    SetTestStepName(KTestStepLLMNR_Report);
    }

CTestStepLLMNR_StartUp::~CTestStepLLMNR_StartUp()
    {
    }

//---------------------------------------------------------------------------------------------------------

TVerdict CTestStepLLMNR_StartUp::doTestStepL()
    {
    
    TInt ret=StartC32(); 
    if ((ret != KErrNone) && (ret != KErrAlreadyExists))
        INFO_PRINTF2(_L("error is : %d \n"),ret);
    else	
        INFO_PRINTF1(_L("Started C32\n"));
    
    //-- connect to the socket server and open host resolver.
    
    INFO_PRINTF1(_L("Connecting to Socket Server..."));
    TInt nRes;
    
    nRes = ipTestServer->iSocketServer.Connect();
    if(nRes != KErrNone)
        {
        ERR_PRINTF2(_L("Socket Server connect error: %d"), nRes);
        TESTL(EFalse);
        }
    
    INFO_PRINTF1(_L("Opening host resolver..."));
    nRes = ipTestServer->iHostResolver.Open(ipTestServer->iSocketServer, KAfInet, KProtocolInetUdp);
    if(nRes != KErrNone)
        {
        ERR_PRINTF2(_L("Host Resolver opening error: %d"), nRes);
        return EFail;
        }
    
    INFO_PRINTF1(KNewLine);
    
    return EPass;
    }

//---------------------------------------------------------------------------------------------------------


CTestStepLLMNR_ShutDown::CTestStepLLMNR_ShutDown(CLlmnrTestServer *apLlmnrTestServer /*= NULL*/)
                        :CTestStepLLMNR(apLlmnrTestServer) 
    {
    // Call base class method to set up the human readable name for logging
    SetTestStepName(KTestStepLLMNR_Report);
    }

CTestStepLLMNR_ShutDown::~CTestStepLLMNR_ShutDown()
    {
    }

//---------------------------------------------------------------------------------------------------------

/**
* Shutdown. Disconnect from the socket server and close host resolver.
*/
TVerdict CTestStepLLMNR_ShutDown::doTestStepL()
    {
    TESTL(ETrue); //-- just to comfort leavescan
    INFO_PRINTF1(_L("closing host resolver..."));
    ipTestServer->iHostResolver.Close();
    
    INFO_PRINTF1(_L("disconnecting from Socket Server..."));
    ipTestServer->iSocketServer.Close();
    
    return EPass;
    }









