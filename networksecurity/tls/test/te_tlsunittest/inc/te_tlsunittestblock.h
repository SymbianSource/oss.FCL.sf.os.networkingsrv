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

#ifndef TE_TLSUNITTESTBLOCK_H
#define TE_TLSUNITTESTBLOCK_H

#include <test/testblockcontroller.h>
#include "te_tlsunittestwrapper.h"

/**
Wrapper name
*/
_LIT(KTlsUnitTestWrapper, "TlsUnitTestWrapper");

/**
Class implements the TEF3.0 specific test block controller.
*/
class CTlsUnitTestBlock : public CTestBlockController
	{
public:
	CTlsUnitTestBlock() : CTestBlockController() {}
	~CTlsUnitTestBlock() {}
	CDataWrapper* CreateDataL(const TDesC& aData);
	};
	
#endif //TE_TLSUNITTESTBLOCK_H
