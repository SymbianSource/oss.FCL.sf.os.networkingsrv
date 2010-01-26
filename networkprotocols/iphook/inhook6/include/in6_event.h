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
// in6_event.h - Interface definitions for Event managers and event listeners
// Interface definitions for Event managers and event listeners
//



/**
 @file in6_event.h
 @publishedPartner
 @released
*/

#ifndef __IN6_EVENT_H__
#define __IN6_EVENT_H__

#include <e32base.h>
#include "apibase.h"


/**
* \brief Interace for event listeners to implement.
*
* Event listeners register to receive notifications in selected event classes. If a event belonging
* to the given class occurs, Notify method for all registered listeners is called by the event manager.
*
* @publishedPartner
* @released
*/
class MEventListener : public MInetBase
{
public:
	/**
	* \brief Called when an event occurs (i.e. someone calls Notify in MEventService class)
	*
	* @param aEventClass  Event class code
	* @param aEventType   Event type, specific to the given event class
	* @param aData	      Event type specific data.
	*/
	virtual void Notify(TUint aEventClass, TUint aEventType, const void *aData) = 0;
};


/**
* \brief Interface to the service provided by the event managers.
*
* Event Managers distribute the event notifications to the registered listeners.
*
* @publishedPartner
* @released
*/
class MEventService : public MInetBase
{
public:
	virtual ~MEventService() {}

	/**
	* \brief A factory method for creating a new event manager
	*
	* @param aNumClasses  Number of event classes handled by the new manager.
	*
	* @return Pointer to the created event manager instance.
	*/
	IMPORT_C static MEventService *CreateEventManager(TUint aNumClasses);
	
	/**
	* \brief Registers a listener to get notifications of given event class.
	*
	* If a listener wants to register for several event classes, this method needs to be called
	* a multiple times.
	*
	* @param aListener    Pointer to listener instance
	* @param aEventClass  Event class for which notifications are to be sent.
	*
	* @return Error code or KErrNone if registeration was succesful.
	*/
	virtual TInt RegisterListener(MEventListener *aListener, TUint aEventClass) = 0;
	
	/**
	* \brief Unregisters a listener.
	*
	* @param aListener    Pointer to listener instance to be unregistered
	* @param aEventClass  Event class which is unregistered. If the listener was registered to
	*		      other event classes, it remains registered there. If this parameter is
	*		      omitted or 0 is given, then the listener is unregistered from all event
	*		      classes (useful for cleanup).
	*
	* @return Error code or KErrNone if unregisteration was succesful.
	*/
	virtual TInt RemoveListener(MEventListener *aListener, TUint aEventClass = 0) = 0;
	
	/**
	* \brief Notifies all registered listeners about an event.
	*
	* @param aEventClass  event class, determines which listeners are notified.
	* @param aEventType   event type code, specific for given class.
	* @param aData	      Event type specific data. The data is passed to all notified listeners.
	*/
	virtual void Notify(TUint aEventClass, TUint aEventType, const void *aData) = 0;

	/**
	* \brief Check if there are any listeners for given event class.
	*
	* @param aEventClass  event class to be checked for listeners.
	*
	* @return ETrue if the listeners list is empty. EFalse if there are listeners registered
	*	  for this event class.
	*/
	virtual TBool IsEmpty(TUint aEventClass) = 0;
};

const TUint KApiVer_MEventService = 1;


#endif  // __IN6_EVENT_H__
