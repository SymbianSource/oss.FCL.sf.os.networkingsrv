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
 @file keyderivationstep.h
 @internalTechnology	
*/
#ifndef __KEYDERIVATIONSTEP_H__
#define __KEYDERIVATIONSTEP_H__

#include "tlsstepbase.h"

_LIT(KKeyDerivationStep, "KeyDerivationStep");

class CKeyDerivationStep : public CTlsStepBase
	{
public:
	CKeyDerivationStep();
	
	TVerdict doTestStepPreambleL();
	TVerdict doTestStepL();
	};

#endif /* __KEYDERIVATIONSTEP_H__ */
