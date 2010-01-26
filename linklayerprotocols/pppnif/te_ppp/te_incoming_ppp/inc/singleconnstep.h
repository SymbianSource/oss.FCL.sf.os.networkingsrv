// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Interface for CSingleConnStep class 
// 
//

/**
 @file 
 @internalComponent
*/
#ifndef __SINGLECONNSTEP_H__
#define __SINGLECONNSTEP_H__

#include "loopbackteststepbase.h"


namespace te_ppploopback
{
/**
 Defines a PPP loopback test step where a single connection is created
 under different circumstances, as determined by the test configuration.
 
 @internalComponent
 @test
 */
class CSingleConnStep: public CLoopbackTestStepBase
	{
public:
	CSingleConnStep();
	~CSingleConnStep();
	TVerdict doTestStepL();
	};
	
/** Test Step name */
_LIT(KSingleConnStep,"CSingleConnStep"); 
} // namespace te_ppploopback

#endif
