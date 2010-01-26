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
// Interface for CPppLink class 
// 
//

/**
 @file 
 @internalComponent
*/
 
#ifndef __PPPLINKIMPL_H__
#define __PPPLINKIMPL_H__

#include <e32std.h>
#include <e32base.h>
#include <es_sock.h>
#include <in_sock.h>
#include <commdbconnpref.h>

namespace te_ppploopback
{

/**
 Interface for implementation by observers of the PPP Link,
 in order to receieve notifications of link event (LinkUp and LinkDown)
 
 @internalComponent
 @test
 */
class MPppLinkObserver
	{
	public:
		/**
		Called when a request to open ppp link is completed, regardless whether is 
		succesful or not. 
		
		@param aError status of PPP link up request completion. KErrNone if link is up.
		*/
		virtual void NotifyLinkUp(TInt aError)   = 0;
		
		/**
		Called when PPP link down request is completed regardless whether is 
		succesful or not. 
		
		@param aError status of PPP link is down. KErrNone if no errors.
		*/
		virtual void NotifyLinkDown(TInt aError) = 0;
	};

/**
 Defines a PPP Link endpoint.
 
 @internalComponent
 @test
 */
class CPppLinkImpl: public CActive
	{	
public:
	CPppLinkImpl(MPppLinkObserver* aObserver, RConnection* aRconn);
	~CPppLinkImpl();
	
	void OpenPppLinkL(TCommDbConnPref* prefs);
	void ClosePppLinkL();
	
	// CActive
	void RunL();
	void DoCancel();
	TInt RunError();

private:

	/** The current stage of the PPP link:
	EUp: link is Up
	UDown: link is Down
	*/
	enum TPppStage {EUp, EDown};
	
	/** The observer of events on the link */
	MPppLinkObserver* iObserver;
	
	/** Encapsulates PPP endpoint*/	
	RConnection* iRConn;	
		
	/** The current stage of the link */
	TPppStage iPppStage;	
	
	/** Buffer for NifProgress object.
	Used to quiery NIF progress. 
	 */
	TNifProgressBuf iProgress;
	};
} // namespace te_ppploopback	
#endif
