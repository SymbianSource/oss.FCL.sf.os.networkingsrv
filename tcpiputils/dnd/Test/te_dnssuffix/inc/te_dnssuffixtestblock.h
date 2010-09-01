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

#ifndef __TE_DNSSUFFIXTESTBLOCK_H__
#define __TE_DNSSUFFIXTESTBLOCK_H__

#include <test/testblockcontroller.h>
#include "te_dnssuffixtestblock.h"

/**
Wrapper name
*/
_LIT(KDNSSuffixTestWrapper, "DNSSuffixTestWrapper");

/**
Class implements the TEF3.0 specific test block controller.
*/
class CVirtualTunnelTestBlock : public CTestBlockController
	{
public:
    CVirtualTunnelTestBlock() : CTestBlockController() {}
	~CVirtualTunnelTestBlock() {}
	CDataWrapper* CreateDataL(const TDesC& aData);
	};
	
#endif //__TE_DNSSUFFIXTESTBLOCK_H__
