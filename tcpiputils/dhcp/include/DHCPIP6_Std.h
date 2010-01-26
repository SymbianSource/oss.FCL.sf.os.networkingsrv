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
// Provides constants common across DHCPIv6 implementation
// 
//

/**
 @file DHCPIP6_Std.h
*/

#ifndef DHCPIP6_STD_H
#define DHCPIP6_STD_H

/*The RFC 3315 says
	Because of the risk of denial of service attacks against DHCP
   clients, the use of a security mechanism is mandated in Reconfigure
   messages.  The server MUST use DHCP authentication in the Reconfigure
   message.

   The following macro enables DHCP client to accept Reconfigure even though
	we don't support authentication yet
	The macro should be removed once we have authentication sorted out
*/
#define DHCP_RECONFIGURE_NO_AUTHENTICATION

#include <e32def.h>

#include <es_sock.h>
#include <es_enum.h>
#include <in_sock.h>
#include <cflog.h>

#ifdef _DEBUG
const TUint	KDhcpv6WrongSrcPort = 34;
const TUint	KDhcpv6WrongDestPort = 33;
#else
const TUint	KDhcpv6SrcPort = 546;
const TUint	KDhcpv6DestPort = 547;
#endif
const TUint	KDhcpInitMsgSizeIP6 = 576;

/**
 * constants used to generated Interface Association IDs (IA_NA IAID & IA_TA IAID)
 */
const TUint32 KDHCPv6IA_NANumberSpaceMin = 0x0001;
const TUint32 KDHCPv6IA_NANumberSpaceMax = 0xFFFF;
const TUint32 KDHCPv6IA_TANumberSpaceMin = 0x1FFFF;
const TUint32 KDHCPv6IA_TANumberSpaceMax = 0xFFFFFFFF;

const TInt KIp6AddressLength = 16; //bytes

/** IPv6 address constants
All_DHCP_Relay_Agents_and_Servers address:   FF02::1:2
All_DHCP_Servers address:                    FF05::1:3
*/
const TIp6Addr KDHCPv6Servers = {{{0xff,0x05,0,0,0,0,0,0,0,0,0,0,0,1,0,3}}};
const TIp6Addr KDHCPv6RelayAgentsNServers = {{{0xff,0x02,0,0,0,0,0,0,0,0,0,0,0,1,0,2}}};

#endif // __DHCPIP6_STD_H__

