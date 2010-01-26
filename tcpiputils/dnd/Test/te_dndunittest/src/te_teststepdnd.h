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

#ifndef     __DNDTESTSTEP_H__
#define     __DNDTESTSTEP_H__

#include <in_sock.h>

#include <testexecutestepbase.h>
#include "te_dndserver.h"

//-- all test steps logging will be dumped to the file with this name.
_LIT(KTestStepDND_Report,"TestStepDND_Report");


_LIT(KNewLine, "\n");



/**
*   Basic DND test step functionality    
*/
class CTestStepDND: public CTestStep
    {
    public:
        CTestStepDND(CDndTestServer *apDndTestServer = NULL);
       ~CTestStepDND();
        
    protected:
        
 //     TBool   GetIniFileString(const TDesC& aSectName, const TDesC& aKeyName, TDes& aStr);
 //	TBool   GetIniFileString8(const TDesC& aSectName, const TDesC& aKeyName, TDes8& aStr);
 //     TBool   GetIniFileIpAddr(const TDesC& aSectName, const TDesC& aKeyName, TInetAddr& aInetAddr);
        
    protected:
        
        CDndTestServer *ipTestServer; //< pointer to the server which owns this test step
        
    };





#endif  //__DNDTESTSTEP_H__





