/*
* Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Symbian Foundation License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.symbianfoundation.org/legal/sfl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/

/**
@test
@internalComponent

This contains CT_IPSecTestServer
*/

#ifndef __TEF_IPSEC_TESTSERVER_H__
#define __TEF_IPSEC_TESTSERVER_H__

#include <test/testblockcontroller.h>
#include <test/testserver2.h>
#include "te_ipsecconst.h"
#include "te_loadpolicy_bbd.h"
#include "te_loadpolicy_bdd.h"
#include "te_selectorconflict.h"
#include "te_loadpolicy_uma.h"
#include "te_coverage_test.h"


class CT_LoadPolicyTestBlock : public CTestBlockController
	{
public:
    static CT_LoadPolicyTestBlock* NewL();
    CT_LoadPolicyTestBlock() : CTestBlockController() {}
	~CT_LoadPolicyTestBlock() {}

	CDataWrapper* CreateDataL(const TDesC& aData)
		{	  
		CDataWrapper* wrapper = NULL;
		
		if (KIPSecTestBypass() == aData)
			{
			wrapper = CT_LoadPolicyBBD::NewL();		
			}
		else if (KIPSecTestUMA() == aData)
		    {
		    wrapper = CT_LoadPolicyUMA::NewL();	    
		    }		
		else if (KIPSecTestDrop() == aData)
            {
            wrapper = CT_LoadPolicyBDD::NewL();
            }
		else if (KIPSecCoverageTest() == aData)
		    {
            wrapper = CT_CoverageTest::NewL();
		    }
		else
			{
			wrapper = NULL;
			}
		return wrapper;
		}
	};

class CT_LoadPolicyTestServer : public CTestServer2
	{
public:
    CT_LoadPolicyTestServer() {}
	~CT_LoadPolicyTestServer() {}

	static CT_LoadPolicyTestServer* NewL();

	CT_LoadPolicyTestBlock*	CreateTestBlock()
		{
		CT_LoadPolicyTestBlock* controller = new (ELeave) CT_LoadPolicyTestBlock();
		return controller;
		}
	};

#endif //__TEF_IPSEC_TESTSERVER_H__
