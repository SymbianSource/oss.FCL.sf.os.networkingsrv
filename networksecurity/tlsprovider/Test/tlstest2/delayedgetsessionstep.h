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
//

/**
 @file delayedgetsessionstep.h
 @internalTechnology	
*/
#ifndef __DELAYEDGETSESSIONSTEP_H__
#define __DELAYEDGETSESSIONSTEP_H__

#include <e32base.h>
#include "verifyGetSessionstep.h"
#include "tlsstepbase.h"


_LIT(KDelayedGetSessionStep, "DelayedGetSessionStep");


class CDelayedGetSessionStep : public CVerifyGetSessionStep
	{
public:
	CDelayedGetSessionStep();
	
	TVerdict doTestStepL();
		
	};


#endif /* __DELAYEDGETSESSIONSTEP_H__*/
