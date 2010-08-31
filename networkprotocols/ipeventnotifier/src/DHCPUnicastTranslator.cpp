// Copyright (c) 2006-2010 Nokia Corporation and/or its subsidiary(-ies).
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
 @file DHCPUnicastTranslator.cpp
 @internalComponent
*/

#include <in_chk.h>
#include <in_sock.h>
#include <in_bind.h>
#include "in6_opt.h"
#include <udp_hdr.h>

#include "DHCPUnicastTranslator.h"
#include "HookLog.h"
#include "ipeventlistener.h"




CDHCPUnicastTranslator::CDHCPUnicastTranslator(CProtocolInet6Binder& aParent):
	iParent(aParent)
	{
	}
		
CDHCPUnicastTranslator::~CDHCPUnicastTranslator()
	{
//	iParent.Unbind(this, 0);
	}

void CDHCPUnicastTranslator::FillIdentification(TServerProtocolDesc& anEntry)
/**
 * Fills in an existing protocol description structure with details of this protocol.
 *
 * @param aProtocolDesc: reference to the structure to be filled in
 */
	{
	anEntry.iName=_S("dhcpunicasttranslator");
	anEntry.iAddrFamily=KAfInet;
	anEntry.iSockType=0;
	anEntry.iProtocol=1; // not in a family so doesn't really matter
	anEntry.iVersion=TVersion(1, 0, 0);
	anEntry.iByteOrder=EBigEndian;
	anEntry.iServiceInfo=0;
	anEntry.iNamingServices=0;
	anEntry.iSecurity=0;
	anEntry.iMessageSize=0;
	anEntry.iServiceTypeInfo=0;
	anEntry.iNumSockets=0;
	}


void CDHCPUnicastTranslator::Identify(TServerProtocolDesc* aProtocolDesc) const
/**
 * Fills in an existing protocol description structure with details of this protocol.
 *
 * @param aProtocolDesc: pointer to the structure to be filled in
 */
	{
	FillIdentification(*aProtocolDesc);
	}



/**
 * Incoming packet
 */
TInt CDHCPUnicastTranslator::ApplyL(RMBufHookPacket& aPacket, RMBufRecvInfo& aInfo)
	{

	LOG	(
		_LIT(KHookNotifyStr,"CDHCPUnicastTranslator::ApplyL hit with protocol: %d");
		HookLog::Printf( KHookNotifyStr , aInfo.iProtocol );
		)
	
	/** Fix for INC079832 - DHCP must be able to receive unicast replies from
	 *   DHCP server (to an address that hasn't been set yet! chicken and egg).
	 *
	 *  Without this, a DHCP reply unicast to an address which isn't yet assigned
	 *   to the inbound interface will be dropped.
	 *
	 *  So we need to detect BOOTP packets which are about to be dropped. This is
	 *   done by installing this object as a "forwarding hook".
	 *
	 *  DHCP packets are detected as follows:
	 *   - packet is UDP
	 *   - UDP destination port is 0x0044
	 *
	 *  Modification is performed by setting dest address to broadcast address
	 *   (and updating checksum)
	 *
	 *  Then we return KIp6Hook_DONE (which signals to the stack to re-inject
	 *   the packet into the incoming data path).
	 *
	 *  Note: this trick isn't needed for IPv6 as link-local unicast is always
	 *   possible before network/global address has been assigned.
	 */
    if (aInfo.iVersion != 4 || aInfo.iProtocol != KProtocolInetUdp)
        return KIp6Hook_PASS;    // Only interested in IPv4 and UDP.

    TInetAddr&  dst = TInetAddr::Cast(aInfo.iDstAddr);

    if (!dst.IsUnicast())
        return KIp6Hook_PASS;    // Only the unicast case really needs fixing

    // Need to examine the UDP header
    TInet6Packet<TInet6HeaderUDP> udp(aPacket, aInfo.iOffset);
    if (udp.iHdr == NULL)
        return KIp6Hook_PASS;    // Oops, short packet, let someone else worry about it.

    // To cause least distruption, one could try to check that this really is
    // DCHP address configuration message with packet destination address
    // matching the offered address inside DCHP message (but this would need
    // more extensive packet snooping here)    
#ifndef _DEBUG
    if (udp.iHdr->DstPort() == 68)
#else
	if (udp.iHdr->DstPort() == 68 || udp.iHdr->DstPort() == 127 || udp.iHdr->DstPort() == 119 || udp.iHdr->DstPort() == 94 || udp.iHdr->DstPort() == 125
	#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
	|| udp.iHdr->DstPort() == 92 || udp.iHdr->DstPort() == 99
	#endif//SYMBIAN_NETWORKING_DHCP_MSG_HEADERS	
	)	// debug mode, src port 127, 119, 125, 91, 98
#endif
        {
        static const TIp6Addr broadcast = {{{0,0,0,0,0,0,0,0,0,0,0xff,0xff,255,255,255,255}}};

        dst.SetAddress(broadcast);
        udp.iHdr->SetChecksum(0);    // Either this or recompute the UDP checksum for broadcast address.
        return KIp6Hook_DONE;
        }
    return KIp6Hook_PASS;
	}


