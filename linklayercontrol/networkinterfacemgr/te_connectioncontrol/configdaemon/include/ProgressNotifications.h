/**
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
* Defines some progress notifications specific to ConfigDaemon.
* 
*
*/



/**
 @file
 @internalComponent
*/

#ifndef __PROGRESSNOTIFICATIONS_H__
#define __PROGRESSNOTIFICATIONS_H__

#include <nifvar.h>

/** Implementation specific progress notification posted by ConfigDaemon when it completes the simulated Mobile IP registration. */
const TUint KConfigDaemonCompletedRegistration = KConfigDaemonStartingRegistration + 25; // 25 is used to reduce the chance of a conflict.

/** Implementation specific progress notification posted by ConfigDaemon when it enters fast dormant mode. */
const TUint KConfigDaemonEnteringFastDormantMode = KConfigDaemonFinishedDeregistrationStop + 25; // 25 is used to reduce the chance of a conflict.

/** Implementation specific progress notification posted by ConfigDaemon when it receives a LinkLayerDown notification. */
const TUint KLinkLayerDownNotificationReceivedByConfigDaemon = KLinkLayerClosed + 25; // 25 is used to reduce the chance of a conflict.

/** Implementation specific progress notification posted by ConfigDaemon when it receives a LinkLayerUp notification. */
const TUint KLinkLayerUpNotificationReceivedByConfigDaemon = KLinkLayerOpen + 25; // 25 is used to reduce the chance of a conflict.

//
// Support for determination of exact deregistration cause
//

/** Progress notification optionally posted by ConfigDaemon when it receives Deregister notification due to Stop */
const TUint KConfigDaemonStartingDeregistrationStop = KConfigDaemonStartingDeregistration + 25; // 25 is used to reduce the chance of a conflict.

/** Progress notification optionally posted by ConfigDaemon when it receives Deregister notification due to Short Idle Timer expiry */
const TUint KConfigDaemonStartingDeregistrationTimerShort = KConfigDaemonStartingDeregistrationStop + 1;

/** Progress notification optionally posted by ConfigDaemon when it receives Deregister notification due to Medium Idle Timer expiry */
const TUint KConfigDaemonStartingDeregistrationTimerMedium = KConfigDaemonStartingDeregistrationTimerShort + 1; 

/** Progress notification optionally posted by ConfigDaemon when it receives Deregister notification due to Long Idle Timer expiry */
const TUint KConfigDaemonStartingDeregistrationTimerLong = KConfigDaemonStartingDeregistrationTimerMedium + 1; 


#endif


