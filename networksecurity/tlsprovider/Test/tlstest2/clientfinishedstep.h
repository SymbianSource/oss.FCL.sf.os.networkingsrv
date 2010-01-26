// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
 @file clientfinishedstep.h
 @internalTechnology	
*/

#ifndef __CLIENTFINISHEDSTEP_H__
#define __CLIENTFINISHEDSTEP_H__

#include <e32base.h>
#include "tlsstepbase.h"

_LIT(KClientFinishedStep, "ClientFinishedStep");

class CClientFinishedStep : public CTlsStepBase
	{
public:
	CClientFinishedStep();
	
	TVerdict doTestStepPreambleL();
	TVerdict doTestStepL();
	};

#endif /* __CLIENTFINISHEDSTEP_H__ */
