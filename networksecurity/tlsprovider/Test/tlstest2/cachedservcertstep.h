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
 @file cachedservcertstep.h
 @internalTechnology	
*/
#ifndef __CACHEDSERVCERTSTEP_H__
#define __CACHEDSERVCERTSTEP_H__

#include <e32base.h>
#include "tlsstepbase.h"
#include "clientkeyexchangestep.h"

_LIT(KCachedServCertStep, "CachedServCertStep");


class CCachedServCertStep : public CClientKeyExchangeStep
	{
public:
	CCachedServCertStep();
	
	TVerdict doTestStepL();
	
	};

#endif /* __CACHEDSERVCERTSTEP_H__ */
