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
// ip6_rth.cpp - default hook for IPv6 routing header
//

#include <ext_hdr.h>
#include "ip6_rth.h"
#include <in_pkt.h>
#include <icmp6_hdr.h>
#include "addr46.h"
//
// CRoutingHeaderHook::InitL
//
void CRoutingHeaderHook::ConstructL()
	{
	iProtocol->BindL(this, BindHookFor(KProtocolInet6RoutingHeader));
	}

//
// CRoutingHeaderHook::ApplyL
//
//
TInt CRoutingHeaderHook::ApplyL(RMBufHookPacket &aPacket, RMBufRecvInfo &aInfo)
	{
	if (aInfo.iVersion == 4)
		{
		//
		// Do not process RTH for IPv4. Returning PASS
		// will cause the main loop eventually to treat this
		// as unknown header, if nothing else handles it.
		//
		return KIp6Hook_PASS;
		}

	for (;;)	// .. just for easy error exits with "break"!
		{
		TInet6Packet<TInet6HeaderRouting> rt(aPacket, aInfo.iOffset);
		if (rt.iHdr == NULL)
			break;			// Not enough for even the basic part, drop packet!
		//
		const TInt hdrlen = aInfo.CheckL(rt.iHdr->HeaderLength());
		const TInt next_header = rt.iHdr->NextHeader();
		//
		// The iIcmp decides whether this is a normal Routing Header or
		// some error report relating to such...
		//
		if (aInfo.iIcmp == 0)
			{
			TInt left = rt.iHdr->SegmentsLeft(); // Always >= 0!
			if (left == 0)
				{
				//
				// Remove routing header
				//
				aInfo.iProtocol = next_header;
				aInfo.iPrevNextHdr = (TUint16)aInfo.iOffset;	// Next Header is at +0 in RTH.
				aInfo.iOffset += hdrlen;
				return KIp6Hook_DONE;
				}

			if (rt.iHdr->RoutingType() != 0)
				{
				// Unknown routing type, with segments left > 0! Must send
				// parameter problem, Code = 0 and point to type.
				//
				iProtocol->Icmp6Send(aPacket, KInet6ICMP_ParameterProblem, 0,
					aInfo.iOffset + TInet6HeaderRouting::O_RoutingType);
				return -1;
				}

			TInt extlen = rt.iHdr->HdrExtLen();	// Always >= 0!
			if (extlen & 1)
				{
				//
				// Odd length, need to send ICMP parameter problem, Code = 0
				// and point to Hdr Ext Len.
				// --- not correct, if we allowed RTH within IPv4! -- msa
				iProtocol->Icmp6Send(aPacket, KInet6ICMP_ParameterProblem, 0,
					aInfo.iOffset + TInet6HeaderRouting::O_HdrExtLen);
				return -1;
				}
			extlen >>= 1;	// = number of addresses
			extlen -= left;	// index of the next address
			if (extlen < 0)
				{
				// --- not correct, if we allowed RTH within IPv4! -- msa
				iProtocol->Icmp6Send(aPacket, KInet6ICMP_ParameterProblem, 0,
					aInfo.iOffset + TInet6HeaderRouting::O_SegmentsLeft);
				return -1;
				}
			rt.iHdr->SetSegmentsLeft(left-1);
			extlen = (extlen << 3) + TInet6HeaderRouting::O_Address;	// = offset to the address.

			TIp6Addr address;
			TPtr8 ptr((TUint8 *)(&address), sizeof(TIp6Addr), sizeof(TIp6Addr));
			aPacket.CopyOut(ptr, aInfo.iOffset + extlen);
			if (ptr.Length() != sizeof(TIp6Addr))
				break;	// Bad Packet
			if (TIp46Addr::Cast(address).IsMulticast() ||
				TIp46Addr::Cast(TInetAddr::Cast(aInfo.iDstAddr).Ip6Address()).IsMulticast())
				break;	// Bad RTH! Send ICMP ?
			//
			// Do the address swap
			// -- update IPv6 header (destination address)
			aPacket.CopyIn(ptr, aInfo.iOffsetIp + TInet6HeaderIP::O_DstAddr);
			// -- swap info dest and routing header
			ptr.Set((TUint8 *)&(TInetAddr::Cast(aInfo.iDstAddr).Ip6Address()), sizeof(TIp6Addr), sizeof(TIp6Addr));
			aPacket.CopyIn(ptr, aInfo.iOffset + extlen);
			TInetAddr::Cast(aInfo.iDstAddr).SetAddress(address);

			// HopLimit decrements from here, because it is automaticly
			// decreased by the forwarding action. However, if the address from
			// the routing header is still for this host, then hoplimit does not
			// affect it. Should it? Does this break any RFC? -- msa
	
			//
			// All done, resubmit by leaving the header in
			//
			aInfo.iFlags &= ~KIpAddressVerified;	// Force destination address check!
			return KIp6Hook_DONE;
			}
		else if (aInfo.iIcmp == KProtocolInet6Icmp)
			{
			//
			// ICMP6 Error report
			//
			const TInt offset = aInfo.iOffset - aInfo.iOffsetIp;// Relative offset within problem packet

			if (aInfo.iType == KInet6ICMP_ParameterProblem &&	// A parameter problem...
				offset <= (TInt)aInfo.iParameter &&		// after start of this header?
				offset + hdrlen > (TInt)aInfo.iParameter)	// and before end of this header?
				break;	// ICMP Paremeter error in routing header, just drop for now.
			//
			// Remove routing header, pass error processing to the next header
			//
			aInfo.iPrevNextHdr = (TUint16)aInfo.iOffset;	// next_header is at +0 in RTH!
			aInfo.iProtocol = next_header;
			aInfo.iOffset += hdrlen;
			return KIp6Hook_DONE;
			}
		//
		// ICMP4 Error report (or something weird)
		//
		// * just drop it*
		//
		break;
		}
	//
	// On any problem, get here and do the drop packet return
	//
	aPacket.Free();
	return -1;
	};
