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
 @file ExpireTimer.cpp
 @internalTechnology
*/

#include "ExpireTimer.h"

CExpireTimer* CExpireTimer::NewL()
/**
  * Create an instance of the timer
  *
  * @internalTechnology
  */
	{
	CExpireTimer* timer = new(ELeave)CExpireTimer();
	CleanupStack::PushL(timer);
	timer->ConstructL();
	CleanupStack::Pop(timer);
	return timer;
	}

void CExpireTimer::After(TTimeIntervalMicroSeconds32 aSeconds,MExpireTimer& aExpireTimer)
	{
	ASSERT(!IsAdded());
	CActiveScheduler::Add(this);
	iExpireTimer = &aExpireTimer;
	iExpirationTime.HomeTime();
 	iExpirationTime += aSeconds;
   	CTimer::After( aSeconds );
   	}

void CExpireTimer::After(TTimeIntervalSeconds aSeconds, MExpireTimer& aExpireTimer)
	{
	ASSERT(!IsAdded());
	CActiveScheduler::Add(this);
	iExpireTimer = &aExpireTimer;
	TTime time;
	time.HomeTime();
	time+=aSeconds;
	iExpirationTime = time;
	CTimer::At(time);
	}

void CExpireTimer::At(TTime aTime, MExpireTimer& aExpireTimer)
	{
	ASSERT(!IsAdded());
	CActiveScheduler::Add(this);
	iExpireTimer = &aExpireTimer;
	TTime now;
	now.HomeTime();
	if (now > aTime)
		aTime = now + static_cast<TTimeIntervalSeconds>(5);
	iExpirationTime = aTime;
	CTimer::At(aTime);
	}


void CExpireTimer::Cancel()
/**
  * Cancel the timer
  *
  * @internalTechnology
  */
	{
	CTimer::DoCancel();
	if (IsAdded())
		{
		Deque();
		}
	}

CExpireTimer::~CExpireTimer()
/**
  * Timer destructor
  *
  * @internalTechnology
  */
	{
	Cancel();
	}

void CExpireTimer::RunL()
/**
  * RunL function alerts the notification
  * object by calling its TimerExpired interface function
  *
  * @internalTechnology
  */
	{
	ASSERT(iExpireTimer);
	if (iStatus == KErrAbort)
 		{
 		//Deal with system time change.	
 		TTime now;
 		now.HomeTime();
 
 		//If its not going to expire in the next five seconds reset it
 		if (now < iExpirationTime + static_cast<TTimeIntervalSeconds>(5))
 			{
 			CTimer::At(iExpirationTime);
 			return;
 			}
 		}

	Deque();
	iExpireTimer->TimerExpired();
	}


