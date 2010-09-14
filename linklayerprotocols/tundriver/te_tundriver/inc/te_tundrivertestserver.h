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
 @file te_tundrivertestserver.h
 @internalTechnology
*/

#ifndef TE_TUNDRIVERTESTSERVER_H
#define TE_TUNDRIVERTESTSERVER_H

#include <test/testserver2.h>
#include "te_tundrivertestblock.h"

/**
Class implements the TEF3.0 specific test server
*/
class CTunDriverTestServer : public CTestServer2
	{
public:
    CTunDriverTestServer() {}
	~CTunDriverTestServer() {}
	CTestBlockController* CreateTestBlock();
	CTestBlockController* CreateTestBlockL();
	static CTunDriverTestServer* NewL();
	};

#endif // TE_TUNDRIVERTESTSERVER_H
