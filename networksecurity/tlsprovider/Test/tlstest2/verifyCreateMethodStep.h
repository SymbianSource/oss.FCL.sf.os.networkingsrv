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
 @file verifyCreateMethodStep.h
 @internalTechnology	
*/
#ifndef __VERIFYCREATEMETHODSTEP_H__
#define __VERIFYCREATEMETHODSTEP_H__

#include <e32base.h>
#include "tlsstepbase.h"

//_LIT(KGetRandomStep, "GetRandomStep");
_LIT(KCreateMethodStep, "CreateMethodStep");
//_LIT(KMaxTimestampInterval, "MaxInterval");

class CCreateMethodStep : public CTlsStepBase
{
public:
	CCreateMethodStep();
	
	TVerdict doTestStepPreambleL();
	TVerdict doTestStepL();
private:
	//TUint GetTimestamp(const TDesC8& aRandom);
	};

#endif /* __VERIFYCREATEMETHODSTEP_H__ */
