/**
* Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* Declares NULL network layer configuration implemented by NIFMAN 
* 
*
*/



/**
 @file NIFNullConfiguration.h
 @internalAll
*/

#if !defined (__NIFNULLCONFIGURATION_H__)
#define __NIFNULLCONFIGURATION_H__

#include <e32base.h>
#include "NIFConfigurationControl.h"

class MNifIfNotify;
/**
 NULL configuration meaning no special steps need to be taken to configure network layer
 @internalAll
 @version 0.03
 @date	26/05/2004
**/
NONSHARABLE_CLASS(CNifNullConfiguration) : public CNifConfigurationControl
{
   friend class CNifConfigurationControl;

protected:
   CNifNullConfiguration( MNifIfNotify& aNifIfNotify );

public:
   virtual void ConfigureNetworkL();
   virtual void LinkLayerDown();
   virtual void LinkLayerUp();
   virtual void Deregister(TInt aCause);
   virtual void SendIoctlMessageL(const RMessage2& aMessage);
   virtual void CancelControl();
   virtual void AsyncDelete();
	virtual void EventNotification(TNetworkAdaptorEventType aEventType, TUint aEvent, const TDesC8& aEventData, TAny* aSource);

protected:
   virtual void RunL();
   virtual void DoCancel();
};

inline CNifNullConfiguration::CNifNullConfiguration( MNifIfNotify& aNifIfNotify ) :
   CNifConfigurationControl( aNifIfNotify )
/**
 CNifNullConfiguration - constructor
 @internalAll
 @param aNifIfNotify - client of the control
 @version 0.01
**/
{
}

#endif


