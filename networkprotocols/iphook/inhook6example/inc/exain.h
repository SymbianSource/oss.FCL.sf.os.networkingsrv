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
// exain.h - inbound plugin example protocol module (dummy)
//

#ifndef __EXAIN_H
#define __EXAIN_H
/**
* @file exain.h
* Inbound plugin example protocol module (dummy).
* @internalComponent
*/

#include <posthook.h>

/**
* @name The exain identification
*
* The protocol is identified by (address family, protocol number). This
* should be unique among all protocols known to the socket server.
*
* Unfortunately there are no rules or registration for these values, and
* the protocol writer just has to pick a combination that is supposed to
* be unique.
*
* In this example, neither of these values have any significance to the
* implementation. Any values will work.
* @{
*/
/** The address family constant. Use the UID value of this protocol module. */
const TUint KAfExain			= 0x10000943;
/** The protocol number. Because the family is unique, 1000 should not confuse anyone. */
const TUint KProtocolExain	= 1000;
/** @} */


class CProtocolExain : public CProtocolPosthook
	/**
	* A protocol plugin for inbound packets.
	*
	* This is a minimal definition for a protocol plugin class
	* (hook), which attaches to the inbound packet path to examine
	* packets for a specific protocol or just before they
	* are passed to the upper layer (choice depends on the nature
	* of the BindL which is done in the NetworkAttachedL function).
	*/
	{
public:
	CProtocolExain();
	virtual ~CProtocolExain();

	// CProtocolBase
	virtual void Identify(TServerProtocolDesc *aDesc) const;

	// CProtocolPosthook
	virtual void NetworkAttachedL();

	// CIp6Hook::MIp6Hook 
	virtual TInt ApplyL(RMBufHookPacket &aPacket, RMBufRecvInfo &aInfo);

	// ProtocolModule glue
	static void Describe(TServerProtocolDesc& anEntry);

	};
#endif
