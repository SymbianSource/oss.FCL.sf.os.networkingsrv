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
 @file negativegetsessionstep.h
 @internalTechnology	
*/
#ifndef __NEGATIVEGETSESSIONSTEP_H__
#define __NEGATIVEGETSESSIONSTEP_H__

#include <e32base.h>
#include "verifyGetSessionstep.h"
#include "tlsstepbase.h"


_LIT(KNegativeGetSessionStep, "NegativeGetSessionStep");


class CNegativeGetSessionStep : public CVerifyGetSessionStep
	{
public:
	CNegativeGetSessionStep();
	
	TVerdict doTestStepL();
		
	};


#endif /* __NEGATIVEGETSESSIONSTEP_H__*/
