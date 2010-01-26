// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// circular.h - Basic circluar list for IPSEC
// @internalComponent	for IPSEC
//



/**
 @internalComponent
*/
#ifndef __CIRCULAR_H__
#define __CIRCULAR_H__

#include <e32def.h>

class RCircularList
	/**
	* Circular list.
	*
	* This is a double linked list without a specific "head".
	* An element may be alone in it's list (both links point
	* to self). The list is never empty, it always includes
	* the object itself.
	*/
	{
public:
	RCircularList();
	~RCircularList();
	RCircularList(RCircularList &aList);
	void Attach(RCircularList &aList);
	void Detach();
	inline TBool IsDetached() const
		/**
		* Return ETrue if this is the only item in the list.
		*/
		{
		return iNext == this;
		}
	RCircularList *iPrev;	//< Previous in the ring (never NULL)
	RCircularList *iNext;	//< Next in the rign (never NULL)
	};

class TCircularListIter
	/**
	* Iterate over the objects in the list.
	*
	* The initial element in construction of the iterator is treated
	* as a traditional list head. The iterator gives the other objects
	* in the list (if any).
	*
	* If the initial element is alone in the ciruclar list, the
	* first application of ++ returns NULL (e.g. the list contains
	* the only the "head", which is never returned by this iterator).
	*/
	{
public:
	inline TCircularListIter(const RCircularList &aList) : iMark(aList), iNext(aList.iNext)
		/**
		* Construct iterator.
		*
		* @param aList	The head element.
		*/
		{}
	RCircularList* operator++(TInt);
private:
	const RCircularList &iMark;	//< The designated list "head" element.
	RCircularList *iNext; 		//< The next element to return with ++ (or "head", if at end of list).
	};

#endif

