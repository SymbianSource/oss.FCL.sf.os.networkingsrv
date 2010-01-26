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
// Expire Timer class definition file
// 
//

/**
 @file
 @internalComponent
*/


#include "ExpireTimer.h"


#ifdef _DEBUG
// Panic category for "absolutely impossible!" vanilla ASSERT()-type panics from this module
// (if it could happen through user error then you should give it an explicit, documented, category + code)
_LIT(KSpecAssert_NifManExprTimer, "NifManExprTimer.");
#endif


CExpireTimer* CExpireTimer::NewL()
/**
 * Create an instance of the timer.
 *
 * @internalComponent
 */
	{
	CExpireTimer* timer = new(ELeave)CExpireTimer();
	CleanupStack::PushL(timer);
	timer->ConstructL();
	CleanupStack::Pop(timer);
	return timer;
	}

void CExpireTimer::After(TTimeIntervalMicroSeconds32 aMcSecs, MExpireTimer& aExpireTimer)
/**
 * Sets-up the timer to notify the target after the specified number of micro seconds.
 *
 * @internalComponent
 *
 * @param aMcSecs Number of microseconds to wait before invoking TimerExpired on aExpireTimer.
 * @param aExpireTimer The object on which to invoke TimerExpired.
 */
	{
	__ASSERT_DEBUG(!IsAdded(), User::Panic(KSpecAssert_NifManExprTimer, 1));
	CActiveScheduler::Add(this);
	iExpireTimer = &aExpireTimer;
	CTimer::After(aMcSecs);
	}

void CExpireTimer::Cancel()
/**
 * Cancel the timer.
 *
 * @internalComponent
 */
	{
	CTimer::Cancel();
	if (IsAdded())
		{
		Deque();
		}
	}

CExpireTimer::~CExpireTimer()
/**
 * Timer destructor.
 *
 * @internalComponent
 */
	{
	Cancel();
	}

void CExpireTimer::RunL()
/**
 * RunL function alerts the notification object by 
 * calling its TimerExpired interface function.
 *
 * @internalComponent
 *
 * @leave Doesn't leave.
 */
	{
	__ASSERT_DEBUG(iExpireTimer, User::Panic(KSpecAssert_NifManExprTimer, 2));
	Deque();
	iExpireTimer->TimerExpired();
	}



