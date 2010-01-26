/*
* Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* Name        : 6to4_listener.h
* Part of     : 6to4 plugin / 6to4.prt
* Implements 6to4 automatic and configured tunnels, see
* RFC 3056 & RFC 2893
* Version     : 0.2
*
*/




/**
 @internalComponent
*/


#ifndef __6TO4_LISTENER_H
#define __6TO4_LISTENER_H

//  INCLUDES
#include <e32base.h>
#include <in6_event.h>

#include "6to4.h"

// CONSTANTS
// MACROS
// DATA TYPES
// FUNCTION PROTOTYPES
// FORWARD DECLARATIONS
// CLASS DECLARATION

/**
*  Listener class implementing MEventListener
*  Gets notifications on new and removed IP addresses on interfaces
*
*  @lib 
*  @since 
*/
class C6to4Listener : public CBase, public MEventListener
	{
	public:  // Constructors and destructor
		
	C6to4Listener (MNetworkService *const aNetwork, MEventService &aService);
	/**
	 * Destructor.
	 */
	virtual ~C6to4Listener ();
	
	inline MNetworkService *NetworkService() const { return iNetwork; }

	public: // Functions from base classes

	/**
	 * From MEventListener Handle a notification.
	 * @since 
	 * @param aEventClass Class of event
	 * @param aEventType Type of event
	 * @param aData Event data
	 * @return void
	 */
	virtual void Notify (TUint aEventClass, TUint aEventType,
						 const void * aData);
		
	private:    // Data

	// 6to4
	MNetworkService *const iNetwork;
	MEventService &iService;
	};

#endif      // __6TO4_LISTENER_H   
			
// End of File
