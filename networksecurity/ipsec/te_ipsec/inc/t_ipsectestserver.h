/*
* Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
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

#ifndef __C_TEF_INTEGRATION_TEST_SERVER__
#define __C_TEF_INTEGRATION_TEST_SERVER__

#include <test/testblockcontroller.h>
#include <test/testserver2.h>
#include "t_ipsecconst.h"
#include "t_ipsecikev2.h"
#include "t_ipsecmultiplesa.h"

class CT_IPSecTestBlock : public CTestBlockController
	{
public:
	CT_IPSecTestBlock() : CTestBlockController() {}
	~CT_IPSecTestBlock() {}

	CDataWrapper* CreateDataL(const TDesC& aData)
		{
		CDataWrapper* wrapper = NULL;
		if (KIPSecIKEv2TestWrapper() == aData)
			{
			wrapper = CT_IPSecIKEv2TestWrapper::NewL();
			}
		else if (KIPSecMultipleSATestWrapper() == aData)
			{
			wrapper = CT_IPSecMultipleSATestWrapper::NewL();
			}
		
		return wrapper;
		}
	};

class CT_IPSecTestServer : public CTestServer2
	{
public:
	CT_IPSecTestServer() {}
	~CT_IPSecTestServer() {}

	static CT_IPSecTestServer* NewL();

	CTestBlockController*	CreateTestBlock()
		{
		CTestBlockController* controller = new (ELeave) CT_IPSecTestBlock();
		return controller;
		}
	};

#endif // __C_TEF_INTEGRATION_TEST_SERVER__
