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
//

/**
 @file ipeventlistener.h
 @internalComponent
*/


#ifndef __IPEVENTLISTENER_H__
#define __IPEVENTLISTENER_H__

#include <e32base.h>
#include <in6_event.h>


class CIPEventNotifier;

class CIPEventListener : public CBase, public MEventListener
//
// Listener class implementing MEventListener
// Gets notifications of the state of IP addresses on interfaces
//
	{
public:  // Constructors and destructor

	explicit CIPEventListener(CIPEventNotifier& aNotifier);

	/**
	 * Destructor.
	 */
	virtual ~CIPEventListener() {};

public: // Functions from base classes

	/**
	 * From MEventListener Handle a notification.
	 * @param aEventClass Class of event
	 * @param aEventType Type of event
	 * @param aData Event data
	 * @return void
	 */
	virtual void Notify(TUint aEventClass, TUint aEventType, const void* aData);

private:    // Data

	CIPEventNotifier& iNotifier;
	};

#endif      // __IPEVENTLISTENER_H__



