// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// key_sap.cpp - IPv6/IPv4 IPSEC PFKEY service access point
// The PFKEY Service Access Point (SAP) -- the socket.
//



/**
 @file key_sap.cpp
*/

#include "ipsec.h"
#include "pfkey.h"
#include "pfkeymsg.h"

//
CProviderKey::CProviderKey(CProtocolKey& aProtocol) : iProtocol(aProtocol)
	/**
	* Constructor.
	*
	* @param aProtocol The PFKEY protocol object reference
	*/
	{
	}


void CProviderKey::Start()
	{
	iListening = 1;
	}

TUint CProviderKey::Write(const TDesC8 &aDesc, TUint /*aOptions*/, TSockAddr* /*aAddr =NULL*/)
	/**
	* Receive PF_KEY v2 messages from the Key Manager application.
	*
	* The key manager controls Security Associations in SAD through the PFKEY socket.
	* Each (datagram) write is a complete PFKEYv2 message. Calls CProtocolKey::Exec()
	* to process the message.
	*
	* @param aDesc The PFKEYv2 message from the application.
	* @param aOptions Not used.
	* @param aAddr Not used.
	* @return Non-zero (datagram accepted).
	*/
	{
	const TInt result = iProtocol.Exec(aDesc, this);
	if (result != KErrNone)
		iSocket->Error(result, MSocketNotify::EErrorSend);
	return 1;	// Datagram sockets return 1 or 0
	}

TInt CProviderKey::SetOption(TUint aLevel, TUint aName, const TDesC8& aOption)
	/**
	* PFKEY SetOpt implementation.
	*
	* No options on SAP level, just pass the call to
	* CProtocolKey::SetOption().
	*
	* @param aLevel The option level
	* @param aName The option name
	* @param aOption The option parameter
	* @return Result code.
	*/
	{
	// Note: capability check not needed here, because only
	// applications with sufficient capability can open this socket.
	return iProtocol.SetOption(aLevel, aName, aOption);
	}


TInt CProviderKey::GetOption(TUint aLevel, TUint aName, TDes8& aOption) const
	/**
	* PFKEY GetOpt implementation.
	*
	* No options on SAP level, just pass the call to
	* CProtocolKey::GetOption().
	*
	* @param aLevel The option level
	* @param aName The option name
	* @retval aOption The option parameter
	* @return Result code.
	*/
	{
	// Note: capability check not needed here, because only
	// applications with sufficient capability can open this socket.
	return iProtocol.GetOption(aLevel, aName, aOption);
	}

void CProviderKey::Deliver(const TPfkeyMessage &aMsg)
	/**
	* Queue PFKEY v2 messages for delivery to application.
	*
	* Convert internal format of PFKEY message into byte stream and
	* append it into the socket receive queue (it has already been
	* decided elsewhere that this socket wants this information)
	*
	* @param aMsg The PFKEY message.
	*/
	{
	if (iListening)
		{
		RMBufRecvPacket packet;

		// Allocate info and fill with dummy values, none of which
		// make any sense really, nobody is using them anyways for
		// these packets...
		TRAP_IGNORE(
			RMBufRecvInfo *info = packet.NewInfoL();
			packet.SetInfo(info);
			info->iProtocol = KProtocolKey;
			info->iLength = 0;
			info->iInterfaceIndex = 0;
			aMsg.ByteStreamL(packet);
			packet.Pack();
			CProviderIpsecBase::Deliver(packet);
			);
		// Just release any partially constructed packet,
		// if not properly delivered.
		packet.Free();
		}
	}
