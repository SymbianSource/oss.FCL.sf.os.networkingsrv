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

#ifndef     __LLMNRINIT_H__
#define     __LLMNRINIT_H__

#include "TestStepLLMNR.h"


/**
*   class responsible for LLMNR initialization and collecting information about 
*   network configuration.
*/
class CTestStepLLMNR_Init: public CTestStepLLMNR
    {
    public:
        CTestStepLLMNR_Init(CLlmnrTestServer *apLlmnrTestServer = NULL);
        ~CTestStepLLMNR_Init();
        
        virtual TVerdict doTestStepL();	
        
        
    private:
        TBool   StartUpLLMNR();
        HBufC8* GetBuffer(HBufC8* apBuf, TInt aBufLenRequired);
        
        void    ListInterfacesL();
        void    LoadNetworkConfigFromIniL(TNetworkInfo&  aNetworkInfo);
        void    ProbeNodes(TNetworkInfo&  aNetworkInfo);
        
    };

_LIT(KTestStepLLMNR_Init,"TestStepLLMNR_Init");



#endif  //__LLMNRINIT_H__





