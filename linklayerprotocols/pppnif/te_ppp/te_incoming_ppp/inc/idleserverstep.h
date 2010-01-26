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
// Interface for CIdleServerStep class
// 
//

/**
 @file 
 @internalComponent
*/
#ifndef __IDLESERVERSTEP_H__
#define __IDLESERVERSTEP_H__

#include "loopbackteststepbase.h"


namespace te_ppploopback
{
/**
 Defines a PPP loopback testing step, where two
 connections are created to an idle PPP server instance.
 
 @test
 */
class CIdleServerStep: public CLoopbackTestStepBase
	{
public:
	CIdleServerStep();
	~CIdleServerStep();
	TVerdict doTestStepL();
	};

_LIT(KIdleServerStep,"CIdleServerStep"); 

} // namespace te_ppploopback

#endif
