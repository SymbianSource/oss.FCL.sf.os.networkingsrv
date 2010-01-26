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
* Declares network layer configuration classes supported by NIFMAN 
* 
*
*/



/**
 @file NIFConfigurationControl.h
 @internalComponent
*/

#if !defined (__NIFCONFIGURATIONCONTROL_H__)
#define __NIFCONFIGURATIONCONTROL_H__

#include <e32base.h>
#include <ecom/ecom.h>
#include <comms-infras/nifprvar.h>

#include <comms-infras/rconfigdaemonmess.h>

class MNifIfNotify;
/**
An abstract base for NIFMAN configuration controls covering the concrete implementation as to
how to configure network layer
@internalComponent
@version 0.03
@date	26/05/2004
**/
class CNifConfigurationControl : public CActive
	{
protected:
	CNifConfigurationControl(MNifIfNotify& aNifIfNotify);
		
public:
	static CNifConfigurationControl* NewL(MNifIfNotify& aNifIfNotify);
		
	/**
	ConfigureNetworkL - called when NIFMAN wants to start network configuration process
	@internalComponent
	@version 0.01
	**/
	virtual void ConfigureNetworkL() = 0;
		
	/**
	LinkLayerDown - used to inform the daemon that link layer renegotiation has started.
	@internalComponent
	@version 0.01
	**/
	virtual void LinkLayerDown() = 0;
	
	/**
	LinkLayerUp - used to inform the daemon that link layer renegotiation has completed.
	@internalComponent
	@version 0.01
	**/
	virtual void LinkLayerUp() = 0;
	
	/**
	Deregister - called when NIFMAN needs to deregister/unconfigure the network
	@internalComponent
	**/
	virtual void Deregister(TInt aCause) = 0;
   		
	virtual void SendIoctlMessageL(const RMessage2& aMessage) = 0;

   	virtual void CancelControl() = 0;
   	
   	virtual void AsyncDelete() = 0;

	/**
	Notification - used to inform the daemon of notification events.
	@internalComponent
	@version 0.01
	**/
	virtual void EventNotification(TNetworkAdaptorEventType aEventType, TUint aEvent, const TDesC8& aEventData, TAny* aSource) = 0;

protected:
	/**
    iNifIfNotify - to access comm database and notify NIFMAN.
    Note that this is a pointer to allow it to be zeroed when detaching from
    NIFMAN (during asynchronous destruction) to avoid a dangling pointer.
    @internalComponent
    @version 0.01
	**/
	MNifIfNotify* iNifIfNotify;
	};

inline CNifConfigurationControl::CNifConfigurationControl(MNifIfNotify& aNifIfNotify) :
CActive(EPriorityStandard),
iNifIfNotify(&aNifIfNotify)
/**
CNifConfigurationControl - constructor
@param aNifIfNotify - client of the control
@internalComponent
@version 0.01
**/
	{
	}

/**
 NIF configuration interface to be able create plug-ins using ECOM
 @internalComponent
 @version 0.01
 @date	12/08/2003
**/
class CNifConfigurationIf : public CNifConfigurationControl
	{
  	friend class CNifConfigurationControl; //to access iDtor_ID_Key from ::NewL

public:
   	CNifConfigurationIf(MNifIfNotify& aNifIfNotify);
   	
protected:
	virtual ~CNifConfigurationIf();

private:
   	// Instance identifier key
   	TUid iDtor_ID_Key;
	};

inline CNifConfigurationIf::CNifConfigurationIf(MNifIfNotify& aNifIfNotify) :
   CNifConfigurationControl(aNifIfNotify)
/**
 CNifConfigurationIf - constructor
 @param aNifIfNotify - client of the control
 @internalComponent
 @version 0.01
**/
	{
	}

//keep the destructor inline even though it's virtual so that 
//the plug-in doesn't have to link against us
inline CNifConfigurationIf::~CNifConfigurationIf()
    {
    REComSession::DestroyedImplementation(iDtor_ID_Key);
    }

#endif


