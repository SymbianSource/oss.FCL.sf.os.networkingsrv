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
// exain.cpp - Inbound plugin example module
//

#include "exain.h"

#pragma warning (disable:4100)

//
// Applies all the modifications needed for every incoming ICMP packet
//
TInt CProtocolExain::ApplyL(RMBufHookPacket& /*aPacket*/, RMBufRecvInfo& aInfo)
	/**
	* An IPv4 ICMP header detected.
	*
	* This example returns always KIp6Hook_PASS without modifying
	* the packet in any way. There is some template if statements
	* which demonstrate the major paths that would need to be taken,
	* if any real processing was to happen.
	*/
	{
/** @code */
	if (aInfo.iProtocol != (TInt)KProtocolInetIcmp)
		{
		// If the CProtocolExain::NetworkAttached only bound the hook for
		// IPv4 ICMP, we should never get here...
		return KIp6Hook_PASS;
		}

	if (aInfo.iIcmp != 0)
		{
		// The ICMPv4 header is in the returned packet of some ICMP error  packet.
		//
		// aInfo.iOffset
		//	offset to the relevant ICMPv4 header
		// aInfo.iOffsetIp
		//	indicates position of the IP header of the returned packet.
		// aInfo.iSrcAddr
		//	is the source address of the returned packet (should be my own address)
		// aInfo.iDstAddr
		//	is the destination address of the returned packet
		return KIp6Hook_PASS;
		}

	// The incoming packet is ICMPv4 addressed to this node.
	//
	// aInfo.iOffset
	//	offset to the relevant ICMPv4 header
	// aInfo.iOffsetIp
	//	indicates position of the IP header before this ICMPv4 header
	//	(usually zero).
	// aInfo.iSrcAddr
	//	is the source address of the packet
	// aInfo.iDstAddr
	//	is the destination address of the packet (should be my own
	//	address, or some type of multicast/broadcast address).
	return KIp6Hook_PASS;
/** @endcode */
	}

#pragma warning (default:4100)
