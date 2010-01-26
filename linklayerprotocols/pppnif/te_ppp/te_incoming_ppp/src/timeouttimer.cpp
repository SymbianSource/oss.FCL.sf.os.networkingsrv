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
// Implementation of the timeout timer.
// 
//

/**
 @file 
*/


#include "timeouttimer.h"


using te_ppploopback::CTimeoutTimer;
using te_ppploopback::MPppEndpointListener;

/**
 * Creates New Timer
 *

 
 @post Object partially constructed
 */
CTimeoutTimer::CTimeoutTimer()
	: CTimer(CActive::EPriorityStandard)
	{};


/**
 * Factory method for use on Stack
 *

 @return new CTimeoutTimer
 */
CTimeoutTimer* CTimeoutTimer::NewLC( )
	{
	CTimeoutTimer* self=new (ELeave) CTimeoutTimer;
	CleanupStack::PushL(self);
	self->ConstructL();
	CActiveScheduler::Add(self);
	return self;
	}
	
/**
 * Factory method for use on Heap
 *

 @return new CTimeoutTimer
 */
CTimeoutTimer* CTimeoutTimer::NewL()
	{
	CTimeoutTimer* self = NewLC();
	CleanupStack::Pop();
	return self;
	}

/**
 * Second phase construction
 

 */
void CTimeoutTimer::ConstructL()
	{
	CTimer::ConstructL();
	}
	

/**
 * Sets a listener for timeout events.
 *

 @param aListener the listener object
 @param aId the Id for this TimeoutTimer. 
 
 @post listener and id have been set.
 */
void CTimeoutTimer::SetListener(MPppEndpointListener* aListener, MPppEndpointListener::EEndpointId aId)
	{
	iListener = aListener;
	iOurId = aId;
	}

/**
 * Destructor
 *

 * 
 * @post All outstanding requests have been canceled.
 */
CTimeoutTimer::~CTimeoutTimer()
	{
	Cancel();
	iListener = NULL;
	}

/**
 * Cancels an outstanding request, if any.
 *

 * 
 * @post All outstanding requests have been canceled.
 */
void CTimeoutTimer::DoCancel()
	{
	CTimer::DoCancel(); 
	}

/**
 * Requests a timeout.
 *

 @param aTimeout the timeout in microseconds. 
 @post The MpppEndpointListener timeout method is called after aTimeout
 */
void CTimeoutTimer::RequestTimeoutL(TInt aTimeout)
	{
	CTimer::After(aTimeout);
	}
	
	
/**
 Implements the ActiveObject::RunL()
 Services the timeout. Called by ActiveScheduler
 *
 
 @post Timeout request was serviced.
 */
void CTimeoutTimer::RunL()
	{	
	iListener->OnEvent(iOurId, MPppEndpointListener::ETimeout, KErrNone);			
	}
	
	






