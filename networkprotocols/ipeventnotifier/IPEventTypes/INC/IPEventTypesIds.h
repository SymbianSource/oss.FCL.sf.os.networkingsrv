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
 @file IPEventTypesIds.h
 @publishedPartner
 @released
*/


#ifndef __IPEVENTTYPES_IDS_H__
#define __IPEVENTTYPES_IDS_H__

#include <e32base.h>
#include <s32std.h>

namespace IPEvent
{

/**
 * The UId / IP event notifier factory ID which is to use when sending message to
 * the factory via RRootSrv::SendMessage (as a CTypeIdQuery::iUid)
 * @see NetMessage::CTypeIdQuery
 */
const TInt KFactoryImplementationUid = { 0x102046A1 };

/**
 * The UId / protocol ID with which to talk to this "event class"
 *  The event class described in this header is "IP stack events"
 * The Id is to use when sending message to
 * the factory via RRootSrv::SendMessage (as a CTypeIdQuery::iTypeId)
 * @see NetMessage::CTypeIdQuery
 */
const TUint KProtocolId		= 0x102045B8;

/**
 * The Uid of the implementation that creates IPEvents family
 *  (currently only used in resource file)
 */
const TInt KEventImplementationUid = 0x102045B5; 


/**
 * Possible option numbers to be requested from CIPEventNotifier::GetOption
 */
typedef enum
	{
	/**
	 * Request a TInt handle that creates an association with the interface.
	 * TDes8& argument is [in/out].
	 *  [in] goes the name of the interface
	 *  [out] comes a TInt (handle/association number)
	 */
	EInterfaceHandle = 5001,

	/**
	 * Tell the notifier that we no longer want to receive events.
	 * TDes8& argument is [in].
	 *  [in] goes the TInt (the handle returned by EInterfaceHandle request)
	 */
	ECloseSession = 5002
	} OptionName;


/**
 * Enumeration of the event types.
 *   Correspond to Type used by CIPEventType to create the message objects.
 *   Also correspond to the pub/sub Uid on which the messages are published
 * @see CIPEventType
 */
typedef enum
	{
	EMFlagReceived = 0x102045B5,
	EIPReady = 0x102045B6,
	ELinklocalAddressKnown = 0x102045B7
	} EventType;

}

#endif // __IPEVENTTYPES_ID_H__




