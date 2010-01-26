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
 @file verifyserversertstep.h
 @internalTechnology	
*/
#ifndef __VERIFYSERVERCERTSTEP_H__
#define __VERIFYSERVERCERTSTEP_H__

#include <e32base.h>
#include "tlsstepbase.h"

_LIT(KServerCertStep, "VerifyServerCertStep");

class CVerifyServerCertStep : public CTlsStepBase
	{
public:
	CVerifyServerCertStep();
	
	TVerdict doTestStepPreambleL();
	TVerdict doTestStepL();
	
	};


#endif /* __VERIFYSERVERCERTSTEP_H__ */
