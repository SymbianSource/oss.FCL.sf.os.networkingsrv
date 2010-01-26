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
// timeout.h - timer manager
// Timer Manager.
//



/**
 @file timeout.h
 @publishedPartner
 @released
*/

#ifndef	__TIMEOUT_H__
#define	__TIMEOUT_H__

#include <e32base.h>
#include "apibase.h"

class RTimeout;
class MTimeoutManager;

//	TimeoutFactory
//	**************

class TimeoutFactory
	/**
	* Factory for creating MTimeoutManager instance(s).
	*
	* TimeoutFactory is not a class that can be instantiated! It only declares
	* static methods, and only publicly available method (NewL) is the one that
	* creates an instance of a timeout manager and returns a reference to the
	* interface class (MTimeoutManager).
	*
	* @publishedPartner
	* @released
	*/
	{
	friend class RTimeout;
public:
	IMPORT_C static MTimeoutManager *NewL(TUint aUnit = 1, TAny *aPtr = 0, TInt aPriority = 0);
private:
	IMPORT_C static void Cancel(RTimeout &aLink);
	IMPORT_C static TBool IsActive(const RTimeout &aLink);
	};

//	MTimeoutManager
//	***************

class MTimeoutManager : public MInetBase
	/**
	* Implementation of "delta queue" timeout manager.
	*
	* The timeout manager provides a timeout callback service
	* for any object. An object using the timeout service only
	* needs to have member variable of RTimeout and define the
	* timeout linkage (TimeoutLinkage) for redirecting the
	* timeout callback to a real function of some object.
	*
	* The same time manager can be used for different object
	* classes. A component can use single timeout manager
	* instance for all of its objects:
	*
	* -# Create timeout manager using TimeoutFactory::NewL
	* -# Objects, which need the timeout callback, declare
	*    the RTimeout member variable and the appropriate
	*    linkage (TimeoutLinkage).
	* -# The timeout callback for a object is requested either
	*	 by RTimeout::Set or MTimeoutManager::Set. The set
	*	 implicitly cancels previous timeout, if active.
	* -# RTimeout::Cancel cancels pending timeout, if any
	*	 is active.
	*
	* Destructions:
	* -# When an object with RTimeout member(s) is destroyed,
	*	 the destructor of this object should include a
	*	 RTimeout::Cancel() for the member variable(s).
	* -# The timeout manager must be deleted, as any normal
	*	 allocated object. All pending timeouts are silently
	*	 cancelled (no expire call will happen for them).
	*
	* The timeout manager maintains an ordered queue of pending timeouts.
	* The next object to expire is always the first in the list, and
	* the timeout manager sets up an internal timer event (RTimer::After)
	* to happen based on the first object.
	*
	* To create an instance of a timeout manager, use TimeoutFactory::NewL.
	*
	* @publishedPartner
	* @released
	*/
	{
public:
	/**
	* Destructor.
	*
	* Even though this is a "mixin" class, the entity that creates
	* it through the TimeoutFactory::NewL() is expected to delete
	* this manager using the returned pointer.
	*/
	virtual ~MTimeoutManager() {};
	/**
	* Activate timeout callback after the specified time.
	*
	* Set a timeout for an object (any previous timeout setting for
	* this object is silently removed, no Expired callback will
	* occur for that). The timeout (aTime) value interpretation is
	* dependent on the aUnit parameter specified for the manager,
	* when it was created (see TimeoutFactory::NewL).
	*
	* @param	aLink	the timeout handle within the object
	* @param	aTime	the time (in fraction of second units).
	*/
	virtual void Set(RTimeout &aLink, TUint aTime) = 0;
	};

//	TimeoutCallback
//	***************
/**
* The callback template.
*
* If the timer expires, the timeout callback is called.
*
* @param	aLink
*		The timeout handle that expired,
* @param	aNow
*		The current time (to which the expiry is based on.
* @param	aPtr
*		The aPtr paramater given in timeout manager instantiation.
*
* @publishedPartner
* @released
*/
typedef void (*TimeoutCallback)(RTimeout &aLink, const TTime &aNow, TAny *aPtr);


//	RTimeout
//	********
class RTimeout
	/**
	* The timeout handle.
	*
	* This can be a member of any object that needs timeout events.
	*
	* The RTimeout can be used as a base for derived classes.
	* 
	* @publishedPartner
	* @released
	*/
	{
	friend class CTimeoutManager;
	friend class TimeoutFactory;
public:
	RTimeout(TimeoutCallback aCallback) : iDelta(0), iExpired(aCallback)
		/**
		* Contructor.
		*
		* @param	aCallback	the expire callback function
		*/
		{
		// Can't use 'this' as initializer, so following is required.
		iNext = this;
		iPrev = this;
		}

	inline void Set(MTimeoutManager *aMgr, TUint aTime)
		/**
		* Activate timeout callback after the specified time.
		*
		* Just an alternate way of calling MTimeoutManager::Set.
		*
		* @param	aMgr	The timeout manager.
		* @param	aTime	The time (in fraction of second units).
		*/
		{
		aMgr->Set(*this, aTime);
		}
	inline TBool IsActive() const
		/**
		* Tests if the timeout is active.
		* @return
		* @li	ETrue, if timeout is active
		* @li	EFalse, if timeout is not active
		*
		* @note
		*	It is always safe to just add the link to a new time manager.
		*	Add will implicitly cancel any previous setting, if such exists.
		*/
		{
		return TimeoutFactory::IsActive(*this);
		}
	inline void Cancel()
		/**
		* Cancel timeout.
		*
		* Cancel removes the timeout (if any) from this link.
		* No Expired() call will happen as a result of this.
		*/
		{
		TimeoutFactory::Cancel(*this);
		}
private:
	RTimeout *iPrev;
	RTimeout *iNext;
	TUint iDelta;
	const TimeoutCallback iExpired;
	};


//	**************
//	TimeoutLinkage
//	**************
template <class T, int Offset>
class TimeoutLinkage
	/**
	* Access "parent" class based on the RTimeout member offset.
	*
	* This is a base template class to convert the link reference into
	* "parent" class refeference. This is just a convenience
	* template and other solutions for defining the the
	* "expired" callback for RTimeout can also be used.
	*
	* A simple use example:
@code
	class CSome
		{
		
		CSome(aCallback) : iTimeout(aCallback) {};
		~CSome() { iTimeout.Cancel(); }
		Timeout();			// A method to handle the timeout of CSome
	public:
		RTimeout iTimeout;
		};

	class SomeLinkage : public TimeoutLinkage<CSome, _FOFF(CSome, iTimeout)>
		{
	public:
		static void Expired(RTimeout &aLink, const TTime & aNow, TAny *aPtr)
			{
			Object(aLink)->Timeout();
			}
		};
@endcode
	* All of the above can be in the CSome header file, and instances of CSome can now just
	* be created as
@code
	MTimeoutManager *tMgr = TimeoutFactory::NewL(1); // unit = 1s
	...
	CSome x = new CSome(SomeLinkage:Expired);
	...
	x->iTimeout.Set(tMgr, 10); // request timeout after 10s.
	...
	// when not needed, just delete
	delete x;
	delete tMgr;
@endcode
	* and the CSome Timeout() method will be called after 10s, unless
	* cancelled.
	*
	* Instead of using the Timeout in the objects directly, the aPtr parameter
	* of Expired could be an address of a manager type instance,
	* which handles objects of this type. The object itself might not have
	* any use of the Expired call and it should be passed to the manager. This can
	* be realised by defining the linkage Expired as follows (for example)
@code
	static void Expired(RTimeoutLink &aLink, const TTime &aNow, TAny *aPtr)
		{
		((CSomeManager *)aPtr)->Timeout(Object(aLink), aNow);
		}
@endcode
	* @publishedPartner
	* @released
	*/
	{
protected:
	static inline T *Object(RTimeout &aLink)
		/**
		* Gets the "parent object" of RTimeout member.
		*
		* Based on the offset of the RTimeout member, typecast the
		* adjusted pointer to the "parent type", and return the
		* pointer.
		*
		* @param aLink The timeout handle.
		* @return The object.
		*/
		{
		return ((T *)((char *)(&aLink) - Offset));
		}
	};

#endif
