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
 @file getrandomstep.h
 @internalTechnology	
*/
#ifndef __GETRANDOMSTEP_H__
#define __GETRANDOMSTEP_H__

#include <e32base.h>
#include "tlsstepbase.h"

_LIT(KGetRandomStep, "GetRandomStep");
_LIT(KMaxTimestampInterval, "MaxInterval");

class CGetRandomStep : public CTlsStepBase
	{
public:
	CGetRandomStep();
	
	TVerdict doTestStepPreambleL();
	TVerdict doTestStepL();

private:
	TUint GetTimestamp(const TDesC8& aRandom);
	};

#endif /* __GETRANDOMSTEP_H__ */
