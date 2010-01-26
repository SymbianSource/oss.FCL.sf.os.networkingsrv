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

#ifndef     __LLMNRTESTSTARTSTOP_H__
#define     __LLMNRTESTSTARTSTOP_H__


#include "TestStepLLMNR.h"

/**
*   responsible for first starting-up steps: connecting to socket server, opening host resolver etc.
*   should be the first step.
*/
class CTestStepLLMNR_StartUp: public CTestStepLLMNR
    {
    public:
        CTestStepLLMNR_StartUp(CLlmnrTestServer *apLlmnrTestServer = NULL);
        ~CTestStepLLMNR_StartUp();
        
        virtual TVerdict doTestStepL();	
        
    protected:
    };


/**
*   Shutdown test step. Disconnecting from servers etc. 
*   should be the last step.
*/
class CTestStepLLMNR_ShutDown: public CTestStepLLMNR
    {
    public:
        CTestStepLLMNR_ShutDown(CLlmnrTestServer *apLlmnrTestServer = NULL);
        ~CTestStepLLMNR_ShutDown();
        
        virtual TVerdict doTestStepL();	
        
    protected:
    };

_LIT(KTestStepLLMNR_StartUp,"TestStepLLMNR_StartUp");
_LIT(KTestStepLLMNR_ShutDown,"TestStepLLMNR_ShutDown");


#endif  //__LLMNRTESTSTARTSTOP_H__





