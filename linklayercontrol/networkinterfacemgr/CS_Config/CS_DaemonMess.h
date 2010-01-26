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
* Client side header listing generic message set supported by NIFMAN.
* Each particular daemon lists its command & controls in a separate .h file
* 
*
*/



/**
 @file CS_DaemonMess.h
 @publishedPartner
 @released
*/

#ifndef __CS_DAEMONMESS_H__
#define __CS_DAEMONMESS_H__

/**
 EConfigDaemon daemon functions
 @publishedPartner
 @released
 @version 0.03
 @date	26/05/2004
*/
enum EConfigDaemonMess
{
   /** Request to the daemon to configure the interface, e.g., to 
       get an IP address for the interface and to set its IP address. 
       Sent to daemon when link layer goes up. */
   EConfigDaemonConfigure,
   /** Generic Ioctl request to daemon. */
   EConfigDaemonIoctl,
   /** Cancels any outstanding request. */
   EConfigDaemonCancel,
   /** Request to deregister. Sent to the daemon when the 
       link layer is about to be torn down. E.g., allows 
       for Mobile IP deregistration. */
   EConfigDaemonDeregister,
   /** Request for a progress notification. In general, there is always
       an oustanding request for a progress notification on the 
       configuration daemon. */
   EConfigDaemonProgress,
   /** Cancels the request specified in the provided mask parameter. */
   EConfigDaemonCancelMask,
   /** Notifies the daemon that the link layer renegotiation has started. */
   EConfigDaemonLinkLayerDown,
   /** Notifies the daemon that the link layer renegotiation has completed. */
   EConfigDaemonLinkLayerUp,
   /** Control command */
   EConfigDaemonControl
};

/**
 Cause codes passed to RConfigDaemon::Deregister
 @publishedPartner
 @released
 @version 0.02
*/
enum EConfigDaemonDeregisterCause
{
   	/** @deprecated 
   	Nifman no longer sends this value.
   	An identity of the timer is sent istead: 
   	LastSessionClosed, LastSocketClosed, LastSocketActivity  */
	EConfigDaemonDeregisterCauseTimer, 
   	/** the deregistration was caused by RConnection::Stop. */
   	EConfigDaemonDeregisterCauseStop,

	/** The deregistration was caused by expiry of the LastSessionClosed idle timer in Nifman:
	The timeout was initiated by absence of ESock RConnection objects and Protocol SAPs */
	EConfigDaemonDeregisterCauseTimerLastSessionClosed, 
   	
   	/** The deregistration was caused by expiry of the LastSocketClosed idle timer in Nifman:
	The timeout was initiated by absence of Protocol SAPs */
	EConfigDaemonDeregisterCauseTimerLastSocketClosed, 
   	
   	/** The deregistration was caused by expiry of the LastSocketActivity idle timer in Nifman:
	The timeout was initiated by the absence of activity on Protocol SAPs */
	EConfigDaemonDeregisterCauseTimerLastSocketActivity,
	
	/** Identity of the expired idle timer not known.
	This value indicates an internal logic error in Nifman */
	EConfigDaemonDeregisterCauseTimerUnknown,
	
	/** The identity of the expired idle timer is other than any of the above identities.
	This value indicates an internal logic error in Nifman */	
	EConfigDaemonDeregisterCauseTimerMax		
};

/**
 Action codes returned by RConfigDaemon::Deregister
 @publishedPartner
 @released
 @version 0.01
*/
enum EConfigDaemonDeregisterAction
{
   	/** On successful deregistration stop the interface. */
	EConfigDaemonDeregisterActionStop,
   	/** On successful deregistration preserve the interface state. */
   	EConfigDaemonDeregisterActionPreserve
};

class TDaemonProgress
/**
 Encapsulates the daemon progress notification information.
 @publishedPartner
 @released
 @version 0.01
*/
	{
public:
	/** The stage. */
	TUint iStage;
	/** The error code associated with the progress notification. */
	TInt iError;
	};

/** The package type for the progress notification parameter. */
typedef TPckgBuf<TDaemonProgress> TDaemonProgressBuf;

/**
 Asynchronous operation masks used by EConfigDaemonCancelMask.
 @publishedPartner
 @released
 @version 0.01
*/
/** Mask to cancel EConfigDaemonConfigure, EConfigDaemonIoctl and EConfigDaemonDeregister. */
const TUint KConfigDaemonOpMaskGeneral	= 0x01; 
/** Mask to cancel EConfigDaemonProgress. */
const TUint KConfigDaemonOpMaskProgress	= 0x02;

#endif

