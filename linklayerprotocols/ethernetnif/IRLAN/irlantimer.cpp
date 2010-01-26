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
//

#include <e32std.h>
#include "irlantimer.h"
#include <ss_pman.h>

/**
Queue a timer on the global timer
*/
void IrlanTimer::Queue(TTimeIntervalMicroSeconds32 aTimeInMicroSeconds, TDeltaTimerEntry& aTimer)
{
	_LIT(irlanTimer,"IrlanTimer");
	CDeltaTimer* timer = (CDeltaTimer*) Dll::Tls();
	__ASSERT_ALWAYS(aTimeInMicroSeconds.Int()>0, User::Panic(irlanTimer, 0));
	__ASSERT_ALWAYS(timer!=NULL, User::Panic(irlanTimer, 1));

#ifdef __MARM__
	if(aTimeInMicroSeconds.Int()<100000)
		aTimeInMicroSeconds=aTimeInMicroSeconds.Int()+KTimerGranularity+(KTimerGranularity>>2);
#endif
	timer->Queue(aTimeInMicroSeconds, aTimer);
}

/**
Call cancel on the global timer
*/
void IrlanTimer::Remove(TDeltaTimerEntry& aTimer)
{
	_LIT(irlanTimer,"IrlanTimer");
	CDeltaTimer* timer = (CDeltaTimer*) Dll::Tls();
	__ASSERT_ALWAYS(timer!=NULL, User::Panic(irlanTimer, 2));
	timer->Remove(aTimer);
}

