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

#ifndef     __LLMNRTESTSTEP_H__
#define     __LLMNRTESTSTEP_H__

#include <in_sock.h>

#include <testexecutestepbase.h>
#include "TE_LlmnrServer.h"

//-- all test steps logging will be dumped to the file with this name.
_LIT(KTestStepLLMNR_Report,"TestStepLLMNR_Report");

//---------------------------------------------------------------------------------------------------------

_LIT(KNewLine, "\n");

const TInt KMinNodes  = 2;       //-- minimal number of network nodes for normal work
const TInt KOneSecond = 1000000; //-- number of uS in 1 Second

//---------------------------------------------------------------------------------------------------------


/**
*   Basic LLMNR test step functionality    
*/
class CTestStepLLMNR: public CTestStep
    {
    public:
        CTestStepLLMNR(CLlmnrTestServer *apLlmnrTestServer = NULL);
       ~CTestStepLLMNR();
        
    protected:
        
        TBool   GetIniFileString(const TDesC& aSectName, const TDesC& aKeyName, TDes& aStr);
        TBool   GetIniFileString8(const TDesC& aSectName, const TDesC& aKeyName, TDes8& aStr);
        TBool   GetIniFileIpAddr(const TDesC& aSectName, const TDesC& aKeyName, TInetAddr& aInetAddr);
        
    protected:
        
        CLlmnrTestServer *ipTestServer; //< pointer to the server which owns this test step
        
    };






#endif // __LLMNRTESTSTEP_H__





