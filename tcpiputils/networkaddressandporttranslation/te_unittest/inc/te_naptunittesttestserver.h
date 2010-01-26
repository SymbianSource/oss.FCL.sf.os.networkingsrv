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

#ifndef TE_NAPTUNITTESTTESTSERVER_H
#define TE_NAPTUNITTESTTESTSERVER_H

#include <test/testserver2.h>
#include "te_naptunittesttestblock.h"

/**
Class implements the TEF3.0 specific test server
*/
class CNaptUnitTestTestServer : public CTestServer2
	{
public:
	CNaptUnitTestTestServer() {}
	~CNaptUnitTestTestServer() {}
	CTestBlockController* CreateTestBlock();
	CTestBlockController* CreateTestBlockL();
	static CNaptUnitTestTestServer* NewL();
	};

#endif // TE_NAPTUNITTESTTESTSERVER_H
