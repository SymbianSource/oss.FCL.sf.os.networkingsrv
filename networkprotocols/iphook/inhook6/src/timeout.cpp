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
// timeout.cpp - timer manager
//

#include <e32std.h>
#include "timeout.h"
#include "inet6log.h"

// Turn off logging for this module
#undef _LOG
#undef LOG
#define LOG(s)

#ifdef NONSHARABLE_CLASS
	NONSHARABLE_CLASS(CTimeoutManager);
#endif

class CTimeoutManager : public CActive, public MTimeoutManager
	{
	friend MTimeoutManager *TimeoutFactory::NewL(TUint,TAny *, TInt);
	CTimeoutManager(TUint aUnit, TAny *aPtr, TInt aPriority);
public:
	~CTimeoutManager();
	void Set(RTimeout &aLink, TUint aTime);
private:
	void DoCancel();
	void RunL();
	TUint Elapsed(const TTime &aStamp);
	inline TTimeIntervalMicroSeconds32 AfterTime(TUint aDelta) const
		{
		return TTimeIntervalMicroSeconds32((aDelta > iMaxDelta) ? KMaxTInt : aDelta * iMultiplier);
		}
	TAny *const iPtr;
	const TUint iMultiplier;						//< Units to microseconds multiplier.
	const TUint iMaxDelta;							//< KMaxTInt microseconds in units.
	const TTimeIntervalMicroSeconds iMaxInterval;	//< KMaxTUint units in microseconds
	RTimer iTimer;
	RTimeout iHead;			//< Use the RTimeout as head, queue is empty , when iNext == iPrev == 0
	TTime iTimeStamp;		//< The base time
	TInt iRun;				//< > 0, when inside RunL, < 0, iTimer is not open
	};

//
// CTimeoutManager::CTimeoutManager()
//
CTimeoutManager::CTimeoutManager(TUint aUnit, TAny *aPtr, TInt aPriority) : CActive(aPriority),
	iPtr(aPtr),
	iMultiplier(1000000 / aUnit),
	iMaxDelta(KMaxTInt / iMultiplier),
	iMaxInterval(TInt64(KMaxTUint) * iMultiplier),
	iHead(NULL)
	{
	LOG(Log::Printf(_L("CTimeoutManager[%u]::CTimeoutManager() aUnit=%u, iMultiplier=%u, iMaxDelta=%u\r\n"),
		this, aUnit, iMultiplier, iMaxDelta));
	// If CreateLocal fails, the iRun will be negative. This
	// will silently disable the timeout activation,
	// there will be no timeout callbacks.
	iRun = iTimer.CreateLocal();
	ASSERT(iRun <= 0);		// should never be > 0!
	CActiveScheduler::Add(this);
	}

//
//	CTimeoutManager::~CTimeoutManager()
//
CTimeoutManager::~CTimeoutManager()
	{
	if (IsActive())
		Cancel();
	if (iRun >= 0)	// Has iTimer been successfully opened?
		iTimer.Close();
	//
	// *NOTE*
	//		Leaves iHead in incorrect state for iPrev,
	//		but as this is a desctuctor, ignore this
	//		for now (beware of using this loop as a
	//		model in elsewhere! -- msa)
	//
	RTimeout *p;
	while ((p = iHead.iNext) != &iHead)
		{
		iHead.iNext = p->iNext;
		p->iNext = p;
		p->iPrev = p;
		}
	}

TUint CTimeoutManager::Elapsed(const TTime &aStamp)
	{
	// If the clock does not behave normally (i.e turned backward or forward),
	// the base time will be set to current time without adjusting any timers
	// This will make all timers in the queue be scheduled longer in actual time
	// by the delta time of the first timer in the queue.
	const TTimeIntervalMicroSeconds delta = aStamp.MicroSecondsFrom(iTimeStamp);
	
#ifdef _DEBUG
	if (delta < TTimeIntervalMicroSeconds(0))
		{
		// Someone has turned the clock backwards!
		LOG(Log::Printf(_L("CTimeoutManager[%u]::Elapsed() *** Clock has been turned back! ***"), this));
		}
	else if (delta > iMaxInterval)
		{
		// Too much time has elapsed
		LOG(Log::Printf(_L("CTimeoutManager[%u]::Elapsed() *** Too much time has elapsed! ***"), this));
		}
#endif

	if (delta < TTimeIntervalMicroSeconds(0) || delta > iMaxInterval)
		{
		iTimeStamp = aStamp;	// Reset base time 
		return 0;
		}

#ifdef I64LOW
	return I64LOW(delta.Int64() / iMultiplier); 
#else
	return (delta.Int64() / iMultiplier).Low(); 
#endif
	}


//
// CTimeoutManager::Set
//	Add an object to the timer queue
//
void CTimeoutManager::Set(RTimeout &aHandle, TUint aTime)
	{
	if (aHandle.IsActive())		// Already Queued?
		aHandle.Cancel();

	TTime stamp;
	stamp.UniversalTime();
	//
	// Initial iTimer.iDelta is the expiration time from the
	// last time stamp. Should always be > 0.
	//
	aHandle.iDelta = aTime;
	if (iHead.iNext != &iHead)
		{
		// Non-empty queue has a valid time stamp.
		// Compute elapsed time from the last time stamp
		// This is always >= 0!
		//
		TUint elapsed = Elapsed(stamp); 
		if (KMaxTUint - aHandle.iDelta < elapsed)
			aHandle.iDelta = KMaxTUint;
		else
			aHandle.iDelta += elapsed;
		}
	else
		{
		//
		// Empty queue does not have a valid time stamp,
		// initialize it to the current time.
		iTimeStamp = stamp;
		}

	LOG(TInt pos = 0);
	RTimeout *p, *prev;
	for (prev = &iHead; ;prev = p)
		{
		LOG(pos++);
		if ((p = prev->iNext) == &iHead)
			{
			//
			// Add to last (and possibly first)
			// Note: prev == some node or &iHead
			//
			aHandle.iPrev = prev;
			aHandle.iNext = &iHead;
			prev->iNext = &aHandle;
			iHead.iPrev = &aHandle;
			LOG(Log::Printf(_L("\tCTimeoutManager[%u]::Set(RTimeout[%u], %u) iDelta=%u, pos=%u. (last)\r\n"),
				this, &aHandle, aTime, aHandle.iDelta, pos));
			break;
			}
		else if (aHandle.iDelta < p->iDelta)
			{
			//
			//	Need to add before this element (p)
			//	(note, p could be == &iHead!)
			//
			p->iDelta -= aHandle.iDelta;
			aHandle.iNext = p;
			aHandle.iPrev = prev;
			p->iPrev = &aHandle;
			prev->iNext = &aHandle;
			LOG(Log::Printf(_L("\tCTimeoutManager[%u]::Set(RTimeout[%u], %u) iDelta=%u, pos=%u. (not last)\r\n"),
				this, &aHandle, aTime, aHandle.iDelta, pos));
			break;
			}
		else
			aHandle.iDelta -= p->iDelta;
		}

	if (iHead.iNext == &aHandle && !iRun)
		{
		// Inserted to the beginning, need to shorten the
		// timer event! (If the insert was not to the beginning,
		// the timer must already be active or this is called
		// from RunL).
		//
		if (IsActive())
			{
			if (iStatus.Int() != KRequestPending)
				{
				LOG(Log::Printf(_L("\tCTimeoutManager[%u] RunL() pending (%d)\r\n"), this, iStatus.Int()));
				return;	// RunL will be called, no need to do anything here.
				}
			Cancel();
			}
		//
		// Activate timer, Set NEVER expires directly -- assume After(0)
		// works and calls RunL() as soon as possible.
		//
		LOG(Log::Printf(_L("\tCTimeoutManager[%u] event after = %u units [%uus]\r\n"), this, aTime, AfterTime(aTime).Int()));
		iTimer.After(iStatus, AfterTime(aTime));
		SetActive();
		}
	}


//
// CTimeoutManager::RunL
//	Called when the timer expires, but it makes no assumptions about
//	the time elapsed, instead it refers to the current real time.
//
void CTimeoutManager::RunL()
	{
	LOG(Log::Printf(_L("-->\tCTimeoutManager[%u]::RunL()\r\n"), this));

	// If iRun < 0, then iTimer.CreateLocal() has failed
	// and RunL should never get called, because timer
	// has never been activated. However, just return
	// if this happens for some reason...
	if (iRun < 0)
		return;

	ASSERT(iRun == 0);

	iRun++;

	for (;;)
		{
		RTimeout *const p = iHead.iNext;
		if (p == &iHead)
			{
			LOG(Log::Printf(_L("<--\tCTimeoutManager[%u] queue is empty\r\n"), this));
			break;
			}
		TTime stamp;
		stamp.UniversalTime();
		// Number of units elapsed since the last time stamp. Always >= 0.
		const TUint elapsed = Elapsed(stamp);

		if (p->iDelta > elapsed)
			{
			iTimeStamp = stamp;
			p->iDelta -= elapsed;			// iDelta > 0, always!
			LOG(Log::Printf(_L("<--\tCTimeoutManager[%u] elapsed=%u next event=%u [%uus]\r\n"),
				this, elapsed, p->iDelta, AfterTime(p->iDelta).Int()));
			iTimer.After(iStatus, AfterTime(p->iDelta));
			SetActive();
			break;
			}
		LOG(Log::Printf(_L("\tCTimeoutManager[%u] elapsed=%u, expire %u [overdue=%u units] RTimeout[%u]\r\n"),
			this, elapsed, p->iDelta, elapsed - p->iDelta, p));
		// Pass the delay to next in queue (as
		// iTimeStamp is not yet changed)
		p->iNext->iDelta += p->iDelta;
		// The time has passed for this TTimeout. Remove
		// from the queue and call the Expired callback
		// (if last, iHead will end up pointing to itself)
		iHead.iNext = p->iNext;
		iHead.iNext->iPrev = &iHead;
		// Mark as inactive (link element to self)
		p->iPrev = p;
		p->iNext = p;
		(p->iExpired)(*p, stamp, iPtr);
		// After above, the iQueue content may have changed
		// totally, one needs to examine only the first entry
		// on each loop.
		}
	--iRun;
	}

void CTimeoutManager::DoCancel()
	{
	iTimer.Cancel();
	}



EXPORT_C MTimeoutManager *TimeoutFactory::NewL(TUint aUnit, TAny *aPtr, TInt aPriority)
	/**
	* Create a new instance of timeout manager.
	*
	* aUnit specifies the unit of the aTime parameter for the timeout
	* setting as fractions of a second. Valid range is [1..1000000].
	* aPtr is additional parameter  which is passed to each Expired()
	* call. aPriority is used for the CActive instantiation.
	*
	* The chosen unit also contrains the maximum time that can
	* be specified with the manager. The time corresponding the
	* KMaxTUint units in seconds is:
@verbatim
maxtime = KMaxTUint / unit
@endverbatim
	*
	* @param	aUnit
	*		The unit in fractions of a second (1/aUnit sec).
	* @param	aPtr
	*		The ptr parameter for every callback (TimeoutCallback)
	*		generated by this manager.
	* @param	aPriority
	*		The CActive priority value for the timeout manager.
	* @return
	*		The MTimeoutManager instance.
	*
	* @leave KErrArgument, if aUnit is invalid
	* @leave Other system wide reasons, if creation fails.
	*/
	{
	//
	// Sanity check and constrain parameters
	//
	if (aUnit < 1 || aUnit > 1000000)
		User::Leave(KErrArgument);
	return new (ELeave) CTimeoutManager(aUnit, aPtr, aPriority);
	}

//
//	CTimeoutFactory::IsActive
//
EXPORT_C TBool TimeoutFactory::IsActive(const RTimeout &aHandle)
	/**
	* Tests if a timeout is active on specified handle.
	*
	* @param	aHandle	The timeout handle
	* @return	ETrue, if active, and EFalse otherwise.
	*/
	{
	return (aHandle.iNext != &aHandle);
	}
//
//	CTimeoutFactory::Cancel
//
EXPORT_C void TimeoutFactory::Cancel(RTimeout &aHandle)
	/**
	* Cancels timeout, if active.
	*
	* This is safe to call, even if handle is inactive.
	*
	* @param	aHandle	The timeout handle
	*/
	{
	LOG(if (aHandle.IsActive()) Log::Printf(_L("\tRTimeout[%u] canceled iDelta=%u\r\n"), &aHandle, aHandle.iDelta));

	//
	// (no need to check for IsActive(), code does not
	// crash even if called for unlinked handle! -- msa)
	//
	// Pass the delay to the next in queue
	aHandle.iNext->iDelta += aHandle.iDelta;
	// Remove element from the old queue
	aHandle.iPrev->iNext = aHandle.iNext;
	aHandle.iNext->iPrev = aHandle.iPrev;
	// Link to self, (== InActive status)
	aHandle.iNext = &aHandle;
	aHandle.iPrev = &aHandle;

	// ** Don't have access to the actual manager, cannot
	// ** cancel timer. Timer event must deal with queue
	// ** being empty!
	}

