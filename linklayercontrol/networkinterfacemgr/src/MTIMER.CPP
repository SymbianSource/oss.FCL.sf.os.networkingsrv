// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// One-shot timer mixin
// 
//

/**
 @file
*/


#include <nifutl.h>
#include "NI_STD.H"

NONSHARABLE_CLASS(COneShotTimer) : public CTimer
/**
@internalComponent
*/
	{
public:
	COneShotTimer(MTimer* aTimer, TInt aPriority);
	void ConstructL();
	virtual void RunL();
private:
	MTimer *iTimer;
	};

#if !defined(__EABI__) && !defined(__X86GCC__)
EXPORT_C MTimer::MTimer()
	: iTimer(NULL)
/**
Constructor
*/
	{
	}
#endif

EXPORT_C void MTimer::TimerDelete()
	{
	delete iTimer;
	iTimer=NULL;				
	}

EXPORT_C void MTimer::TimerConstructL(TInt aPriority)
	{
	iTimer = new (ELeave) COneShotTimer(this, aPriority);
	((COneShotTimer*)iTimer)->ConstructL();
	}

EXPORT_C void MTimer::TimerAt(const TTime& aTime)
	{
	__ASSERT_DEBUG(iTimer!=NULL, NetUtilPanic(ENuPanic_NotConstructed));
	iTimer->At(aTime);
	}

EXPORT_C void MTimer::TimerAfter(TTimeIntervalMicroSeconds32 aInterval)
	{
	__ASSERT_DEBUG(iTimer!=NULL, NetUtilPanic(ENuPanic_NotConstructed));
	iTimer->After(aInterval);
	}

EXPORT_C void MTimer::TimerLock(TTimerLockSpec aLock)
	{
	__ASSERT_DEBUG(iTimer!=NULL, NetUtilPanic(ENuPanic_NotConstructed));
	iTimer->Lock(aLock);
	}

EXPORT_C void MTimer::TimerCancel()
	{
	if (iTimer!=NULL)
		iTimer->Cancel();
	}

COneShotTimer::COneShotTimer(MTimer* aTimer, TInt aPriority)
	: CTimer(aPriority), iTimer(aTimer)
/**
Constructor
*/
	{
	CActiveScheduler::Add(this);
	}

void COneShotTimer::RunL()
	{
	iTimer->TimerComplete(iStatus.Int());
	}

void COneShotTimer::ConstructL()
	{
	CTimer::ConstructL();
	}
