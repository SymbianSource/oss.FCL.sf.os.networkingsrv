// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// @file
// This contains CT_DnsProxyTestServer
//



/**
 @test
 @internalComponent
*/

#ifndef __C_TEF_DNSPROXY_TEST_SERVER__
#define __C_TEF_DNSPROXY_TEST_SERVER__

#include <test/testblockcontroller.h>
#include <test/testserver2.h>
#include "t_dnsproxyconst.h"
#include "t_dnsproxymain.h"



class CT_DnsProxyTestBlock : public CTestBlockController
	{
public:
	CT_DnsProxyTestBlock() : CTestBlockController() {}
	~CT_DnsProxyTestBlock() {}

	CDataWrapper* CreateDataL(const TDesC& aData)
		{
		CDataWrapper* wrapper = NULL;
		if (KDnsProxyMainTestWrapper() == aData)
			{
			wrapper = CT_DnsProxyMainTestWrapper::NewL();
			}
		
		return wrapper;
		}
	};

class CT_DnsProxyTestServer : public CTestServer2
	{
public:
	CT_DnsProxyTestServer() {}
	~CT_DnsProxyTestServer() {}

	static CT_DnsProxyTestServer* NewL();

	CTestBlockController*	CreateTestBlock()
		{
		CTestBlockController* controller = new (ELeave) CT_DnsProxyTestBlock();
		return controller;
		}
	};

#endif // __C_TEF_INTEGRATION_TEST_SERVER__
