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
 @file multicancelstep.h
 @internalTechnology	
*/
#ifndef __MULTICANCELSTEP_H__
#define __MULTICANCELSTEP_H__

#include <e32base.h>
#include "serverfinishedstep.h"
#include "tlsstepbase.h"


_LIT(KMultiCancelStep, "MultiCancelStep");


class CMultiCancelStep : public CServerFinishedStep
	{
public:
	CMultiCancelStep();
	
	TVerdict doTestStepL();
		
	};


#endif /* __MULTICANCELSTEP_H__*/
