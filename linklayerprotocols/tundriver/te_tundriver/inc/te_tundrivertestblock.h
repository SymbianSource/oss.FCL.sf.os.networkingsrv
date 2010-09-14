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
 @file te_tundrivertestblock.h
 @internalTechnology
*/

#ifndef TE_TUNDRIVERTESTBLOCK_H
#define TE_TUNDRIVERTESTBLOCK_H

#include <test/testblockcontroller.h>
#include "te_tundrivertestwrapper.h"

/**
Wrapper name
*/
_LIT(KTunDriverTestWrapper, "TunDriverTestWrapper");

/**
Class implements the TEF3.0 specific test block controller.
*/
class CTunDriverTestBlock : public CTestBlockController
	{
public:
    CTunDriverTestBlock() : CTestBlockController() {}
	~CTunDriverTestBlock() {}
	CDataWrapper* CreateDataL(const TDesC& aData);
	};
	
#endif //TE_TUNDRIVERTESTBLOCK_H
