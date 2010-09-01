// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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



/**
 @file
 @internalTechnology
*/

#ifndef __TE_DNSSUFFIXTESTSERVER_H__
#define __TE_DNSSUFFIXTESTSERVER_H__

#include <test/testserver2.h>
#include "te_dnssuffixtestblock.h"

/**
Class implements the TEF3.0 specific test server
*/
class CDNSSuffixTestServer : public CTestServer2
	{
public:
    CDNSSuffixTestServer() {}
	~CDNSSuffixTestServer() {}
	CTestBlockController* CreateTestBlock();
	CTestBlockController* CreateTestBlockL();
	static CDNSSuffixTestServer* NewL();
	};

#endif // __TE_DNSSUFFIXTESTSERVER_H__
