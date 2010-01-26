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
// The Expire Timer header file
// 
//

/**
 @file ExpireTimer.h
*/

#ifndef __EXPIRETIMER_H__
#define __EXPIRETIMER_H__

#include <e32base.h>

class MExpireTimer
/**
  * Just a notification interface
  * to alert an object that the timer
  * has popped.
  *
  * @internalTechnology
  */
   {
public:
   virtual void TimerExpired() = 0;
   };

class CExpireTimer : public CTimer
/**
 * The CExpireTimer class
 *
 * Implements a simple timer object for the server
 */
	{
public:
	static CExpireTimer* NewL();
	~CExpireTimer();

	void After(TTimeIntervalSeconds aSeconds,MExpireTimer& aExpireTimer);
	void After(TTimeIntervalMicroSeconds32 aSeconds,MExpireTimer& aExpireTimer);
	void At(TTime aTime, MExpireTimer& aExpireTimer);
	void Cancel();

private:
	CExpireTimer();
	void RunL();

private:
	MExpireTimer* iExpireTimer;
	TTime iExpirationTime;
	};

inline CExpireTimer::CExpireTimer() : CTimer(EPriorityStandard)
	{
	}
#endif

