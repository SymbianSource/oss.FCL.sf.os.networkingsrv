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
// MPppEndpoinListener interface definition 
// 
//

/**
 @file 
 @internalComponent
*/
#ifndef __PPPENDPOINT_H__
#define __PPPENDPOINT_H__

#include <e32base.h> 


namespace te_ppploopback 
{

/**
 * Notification interface for PPP endpoint observers.
 * To be implemented by classes defining PPP loopback test cases.
 * @internalComponent
 * @test
 */
class MPppEndpointListener
{
public:
	/** Used to distinguish between endpoints */
	enum EEndpointId 
		{
		EServer = 0, 
		EClient, 
		ETimeoutTimer, 
		EMinEndpointId = EServer,
		EMaxEndpointId = ETimeoutTimer
		};

	/** Events produced by endpoints. May need to add more */
	enum EEventId
		{		
		ESend,
		ERecv,
		ELinkUp,
		ELinkDown,
		ETimeout,
		EMinEventId = ESend,
		EMaxEventId = ETimeout
		};
	
public:
	/**
	Called on any PPP endpoint event
	@param aId  endpoint ID (either EDaemon or EPeer).
	@param aEvent cause of notification
	@param aError error code associated with event. 
	@pre  none
	@post Listener was notified.
	*/
	virtual void OnEvent(EEndpointId aId, EEventId aEvent, TInt aError) = 0;
};
} // namespace  te_ppploopback

#endif
