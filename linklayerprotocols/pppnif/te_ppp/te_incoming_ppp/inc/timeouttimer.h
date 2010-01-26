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
// Implementation of CTimeoutTimer class
// 
//

/**
 @file 
 @internalComponent
*/

#ifndef __TIMEOUT_TIMER_H__
#define __TIMEOUT_TIMER_H__

#include <e32base.h> 

#include "pppendpoint.h" 


namespace te_ppploopback 
{

/**
 * Timer to implements timeout for the PPP state machine.
 * Not used in ppp loopback testing directly.
 
 * @internalComponent
 * @test
 */
class CTimeoutTimer : public CTimer
  	{
public:	
	~CTimeoutTimer();

public:
	static CTimeoutTimer* NewLC();								 
	static CTimeoutTimer* NewL();

	void SetListener(MPppEndpointListener* aListener, MPppEndpointListener::EEndpointId aId);
	void ConstructL();
	void RequestTimeoutL(TInt aTimeout); 
	void DoCancel();
	void RunL();
	
private:
	CTimeoutTimer();

private:
	MPppEndpointListener* iListener;
	MPppEndpointListener::EEndpointId iOurId;

};

} // namespace te_ppploopback

#endif
