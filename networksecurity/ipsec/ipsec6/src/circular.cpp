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
// circular.cpp - Basic circluar list for IPSEC
// @internalComponent	for IPSEC
//

#include "circular.h"

RCircularList::RCircularList()
	/**
	* Constructor.
	*
	* Ensure that the element is always a member of list,
	* and link to self.
	*/
	{
	iPrev = this;
	iNext = this;
	}
	
RCircularList::~RCircularList()
	/**
	* Destructor.
	*
	* Implicit detach.
	*/
	{
	Detach();
	}

RCircularList::RCircularList(RCircularList &aList)
	/**
	* Constructor.
	*
	* Construct a circular list that includes this element
	* and all elements, except the "head", from another
	* circular list.
	*
	* @param aList The head of the other list.
	*/
	{
	if (aList.iNext == &aList)
		{
		// The aList only contains self, nothing to steal.
		iPrev = this;
		iNext = this;
		}
	else
		{
		// Steal the list from aList (except aList self)
		iPrev = aList.iPrev;
		iNext = aList.iNext;
		iNext->iPrev = this;
		iPrev->iNext = this;

		// Leave the aList alone in its own list.
		aList.iNext = &aList;
		aList.iPrev = &aList;
		}
	}

	
void RCircularList::Attach(RCircularList &aList)
	/**
	* Move an element from another list to this list.
	*
	* Place the new element after this.
	*
	* @param aList The element to move	
	*/
	{
	aList.Detach();
	aList.iPrev = this;
	aList.iNext = iNext;
	iNext->iPrev = &aList;
	iNext = &aList;
	}

void RCircularList::Detach()
	/**
	* Remove this from whatever list it is.
	*
	* This element will be alone in it's own circular list.
	*/
	{
	// Remove element from the old list
	iPrev->iNext = iNext;
	iNext->iPrev = iPrev;
	// Link to self
	iNext = this;
	iPrev = this;
	}
	


RCircularList* TCircularListIter::operator++(TInt)
	/**
	* Return the next element in the list or NULL.
	*
	* @return The next element or NULL.
	*/
	{
	RCircularList *ptr = iNext;
	if (ptr == &iMark)
		return NULL;
	
	iNext = ptr->iNext;
	return ptr;
	}

