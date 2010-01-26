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
// PPP Loopback testing implementation
// 
//

/**
 @file 
 @internalComponent
*/

#include "ppplinkimpl.h"
using namespace te_ppploopback;
/**
 C++ Constructor
 
 @param aObserver the observer, for events on the link.
 @param aRConn the RConnection used to create the PPP link.
 @post Object is constructed.
 */	
CPppLinkImpl::CPppLinkImpl(MPppLinkObserver* aObserver, RConnection* aRConn):
	CActive(EPriorityNormal),
	iObserver(aObserver),
	iRConn(aRConn),	
	iPppStage(EDown)
	{
	CActiveScheduler::Add(this);
	}

/**
 C++ Destructor
 */
CPppLinkImpl::~CPppLinkImpl()
	{
	Cancel();
	iObserver = 0;
	iRConn  = 0;
	}


/**
 Asynchronously requests PPP link open
 
 @param aConnPrefs Connection Preferences, (specifying the IAP)
 @post Request is issued
 */ 
void CPppLinkImpl::OpenPppLinkL(TCommDbConnPref* aConnPrefs)
	{	
	iRConn->Start(*aConnPrefs, iStatus);
	SetActive();
	}

/**
 Requests to close PPP link 
 
 @pre Link is Up
 @post Link is Down
 @leave if the link is down already, or a link related error occurs.
 */
void CPppLinkImpl::ClosePppLinkL()
	{	
	User::LeaveIfError(iRConn->Stop());
	}

/**
Implementation of CActiveObject::RunL 
*/	
void CPppLinkImpl::RunL()
	{
	switch(iPppStage)
		{
		case EDown:
			if(iStatus.Int() == KErrNone)
				{
				iPppStage = EUp;
				}				
			iObserver->NotifyLinkUp(iStatus.Int());	
			iRConn->ProgressNotification(iProgress, iStatus, KLinkLayerClosed);				
			SetActive();		
			break;
			
		case EUp:
			iPppStage = EDown; // even if error.		
			iObserver->NotifyLinkDown(iStatus.Int());
			break;
		}				
	}
/**
 Implementation of CActiveObject::DoCancel()
 */	
void CPppLinkImpl::DoCancel()
	{
	if(iRConn->SubSessionHandle() != 0)
		{
		iRConn->CancelProgressNotification();
		}
	}
/**
 Implementation of ActiveObject::RunError()
 */	
TInt CPppLinkImpl::RunError()
	{
	return KErrNone; 
	}


 
 




