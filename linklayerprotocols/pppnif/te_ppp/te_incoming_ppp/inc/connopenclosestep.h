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
// Interface for CConnOpenCloseStep class
// 
//

/**
 @file 
 @internalComponent
*/
#ifndef __CONNOPENCLOSESTEP_H__
#define __CONNOPENCLOSESTEP_H__

#include "loopbackteststepbase.h"


namespace te_ppploopback
{
/**
 Defines a PPP loopback test step where a connection is opened and closed, and
 after that a second connection is created
 
 @internalComponent
 @test
 */
class CConnOpenCloseStep: public CLoopbackTestStepBase
	{
public:
	CConnOpenCloseStep();
	~CConnOpenCloseStep();
	TVerdict doTestStepL();
	void OnTimerEvent(TInt/* aErrorCode */);
	};

_LIT(KConnOpenCloseStep,"CConnOpenCloseStep"); 

} // namespace te_ppploopback

#endif
