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
// event.cpp - Event service implementation
// Implementation of a event manager
//



/**
 @file event.cpp
*/

#include <in6_event.h>


class TListenerNode
{
public:
    MEventListener *iListener;
    TListenerNode *iNext;
};

#ifdef NONSHARABLE_CLASS
	NONSHARABLE_CLASS(CEventManager);
#endif

class CEventManager : public MEventService, public CBase
{
public:
    CEventManager(TUint aNumClasses);
    virtual ~CEventManager();
	void ConstructL();
    virtual TInt RegisterListener(MEventListener *aListener, TUint aEventClass);
    virtual TInt RemoveListener(MEventListener *aListener, TUint aEventClass = 0);
    virtual void Notify(TUint aEventClass, TUint aEventType, const void *aData);
    virtual TBool IsEmpty(TUint aEventClass);

private:
    TUint iNumClasses;		//< Number of event classes handled by this event manager
    TListenerNode *iListeners;	//< Array of registered event listeners for each event class
};


CEventManager::CEventManager(TUint aNumClasses) : CBase(), iNumClasses(aNumClasses)
	{
	}


CEventManager::~CEventManager()
	{
	TListenerNode *node = NULL;
	TListenerNode *prev = NULL;
    for (TUint i = 0; i < iNumClasses; i++)
    	{
		node = iListeners[i].iNext;
		while (node)
	  		{
	    	prev = node;
	    	node = node->iNext;
	    	delete prev;
	  		}
      	}
  
    delete[] iListeners;
	}


void CEventManager::ConstructL()
	{
    iListeners = new TListenerNode[iNumClasses];
    if (!iListeners)
    	{
    	User::Leave(KErrNoMemory);
    	}

    for (TUint i = 0; i < iNumClasses; i++)
    	{
		iListeners[i].iListener = 0;
		iListeners[i].iNext = 0;
      	}
	}


TInt CEventManager::RegisterListener(MEventListener *aListener, TUint aEventClass)
	{
    if (aEventClass > iNumClasses)
    	{
		return KErrArgument;
		}

    TListenerNode *iter = &iListeners[aEventClass-1];
    while (iter->iNext)
    	{
		iter = iter->iNext;
		}

    TListenerNode *node = new TListenerNode;
    if (!node)
    	{
    	return KErrNoMemory;
    	}
    
    iter->iNext = node;
    node->iListener = aListener;
    node->iNext = NULL;
    
	/* Dynamically allocated memory node has been assigned to iter->iNext and managed through the list.
	So CleanupStack::PopAndDestroy() will deallocate that memory. But, Coverity has misinterpreted it an issue.*/
	// coverity [SYMBIAN.CLEANUP STACK]
	// coverity [memory_leak]
    return KErrNone;
	}


TInt CEventManager::RemoveListener(MEventListener *aListener, TUint aEventClass)
	{
    if (aEventClass > iNumClasses)
    	{
		return KErrArgument;
		}

    TListenerNode **iter = NULL;
    TBool found = EFalse;
    TListenerNode *n;
    // Start as an iterator, but if aEventClass was given, exit after first run
    for (TUint i = (aEventClass ? aEventClass-1 : 0); i < iNumClasses; i++)
    	{
		found = EFalse;
		
		iter = &iListeners[i].iNext;
		while (*iter)
	  		{
			n = *iter;
	    	
	    	if (n->iListener == aListener)
	      		{
				*iter = n->iNext;
				delete n;
				found = ETrue;
				continue;
	      		}

    		iter = &n->iNext;
	  		}

		if (aEventClass)
	  		{
	    	// If event class is given explicitly, we don't iterate through all event classes
	    	if(found)
				return KErrNone;  // Something was deleted, function exits succesfully
	    	else
				return KErrNotFound;  // Given listener was not found
	  		}
		}

    // If aEventClass was not given, this call always succeeds
	return KErrNone;
	}


void CEventManager::Notify(TUint aEventClass, TUint aEventType, const void *aData)
	{
    if (aEventClass > iNumClasses)
    	{
		return;
		}

    TListenerNode *iter = iListeners[aEventClass-1].iNext;
    while (iter)
		{
		iter->iListener->Notify(aEventClass, aEventType, aData);
		iter = iter->iNext;
      	}
	}


TBool CEventManager::IsEmpty(TUint aEventClass)
	{
    if (aEventClass > iNumClasses)
    	{
		return ETrue;  // Inexisting class is always empty
		}

    if (iListeners[aEventClass-1].iNext)
    	{
		return EFalse;
		}
    else
    	{
		return ETrue;
		}
	}


EXPORT_C MEventService *MEventService::CreateEventManager(TUint aNumClasses)
	{
    CEventManager *es = new CEventManager(aNumClasses);
    if (!es)
    	{
    	return NULL;
    	}
    	
	TInt err = KErrNone;
	// coverity[leave_without_push]
	// if ConstructL() leaves, the leave code is trapped in "err" and proper cleanup is done by freeing the memory alocated to "es"
    TRAP(err, es->ConstructL());
    if (err != KErrNone)
    	{
    	delete es;
    	return NULL;
    	}
    	
    return es;
	}
