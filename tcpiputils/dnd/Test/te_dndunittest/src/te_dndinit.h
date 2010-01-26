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

#ifndef     __TE_DNDINIT_H__
#define     __TE_DNDINIT_H__

#include "te_teststepdnd.h"


/**
*   class responsible for DND initialization and collecting information about 
*   network configuration.
*/
class CTestStepDND_Init: public CTestStepDND
    {
    public:
        CTestStepDND_Init(CDndTestServer *apDndTestServer = NULL);
        ~CTestStepDND_Init();
        
        virtual TVerdict doTestStepL();	
        
        
    private:
        TBool   StartUpDND();
              
    };

_LIT(KTestStepDND_Init,"TestStepDND_Init");


#endif  //__TE_DNDINIT_H__
