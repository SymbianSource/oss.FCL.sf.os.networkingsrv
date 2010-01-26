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
// ipip_eng.cpp - IPv6/IPv4 in IPv6/IPv4 for IPSEC
// The IPSEC IP-in-IP engine (not really bound to IPSEC very tightly)
//



/**
 @file ipip_eng6.cpp
*/

#include "ip6_hdr.h"
#include "ext_hdr.h"
#include "sadb.h"
#include "ipip_eng.h"
#include <networking/ipsecerr.h>
#include "in_chk.h"


TInt TIpsecIPIP::Overhead(const TIpAddress &aTunnel) const
	/**
	* Return maximum possible overhead caused by this tunnel,
	*
	* @param aTunnel The tunnel end point address.
	* @return The overhead (bytes).
	*/
	{
	if (aTunnel.Address().IsV4Mapped())
		return TInet6HeaderIP4::MinHeaderLength();	// No options!
	else
		return TInet6HeaderIP::MinHeaderLength();	// No extension headers
	}

TInt TIpsecIPIP::ApplyL(const TIpAddress &aTunnel, RMBufSendPacket &aPacket, RMBufSendInfo &aInfo)
	/**
	* Apply Tunnel to outbound packet.
	*
	* @li	aPacket is the outgoing packet. It is *FULL* packet
	*		and already includes the outgoing IP header.
	* @li	aInfo is the associated info block. The info->iLength
	*		*MUST* be maintained, if any change affects it.
	*
	* @param aTunnel The tunnel end point address
	* @param aPacket The packet to process
	* @param aInfo The packet info
	* @return
	*	@li	Normally leaves on any error condition, which causes
	*		the packet to be dropped,
	*	@li	KErrNone, the tunnel has been added.
	* 
	*	@li < 0, some other unexpected error.
	*/
	{
	TIpHeader *ip = ((RMBufPacketPeek &)aPacket).GetIpHeader();
	if (!ip)
		return EIpsec_RMBUF;

	const TInt inner_ip4 = (ip->ip4.Version() == 4);

	// Determine the outer Time To Live value from the real interface or the
	// TCP/IP 6 stack default.
	TPckgBuf<TInt> outerTtl;
	TInt err = aInfo.iFlow.FlowContext()->GetOption( KSolInetIp, KSoIp6InterfaceUnicastHops, outerTtl );
	if( err != KErrNone )
		{
		return err;
		}

	if (aTunnel.Address().IsV4Mapped())
		{
		// Adding outer IPv4 layer
		const TInt hlen = TInet6HeaderIP4::MinHeaderLength();

		aPacket.PrependL(hlen);
		TInet6Checksum<TInet6HeaderIP4> outer(aPacket);
		if (outer.iHdr == NULL)
			return EIpsec_RMBUF;
		aInfo.iLength += hlen;
		aInfo.iProtocol = KProtocolInetIp;		// Outer is IPv4
		outer.iHdr->Init();
		outer.iHdr->SetHeaderLength(hlen);
		outer.iHdr->SetTotalLength(aInfo.iLength);
		outer.iHdr->SetIdentification(AllocId());
		outer.iHdr->SetDstAddr(aTunnel.Ip4Address());
		outer.iHdr->SetSrcAddr(TInetAddr::Cast(aInfo.iSrcAddr).Address());
		outer.iHdr->SetTtl(outerTtl());
		if (inner_ip4)
			{
			// IPv4 in IPv4

			// (the inner IPv4 is assumed to be complete, including the checksum)
			outer.iHdr->SetTOS(ip->ip4.TOS());
			// What should be the value of the DF bit? For now
			// just copy from the inner header! Other flags are
			// set to ZERO! There should be some way of configuring
			// the following choices:
			// - copy outer DF from inner (as done now)
			// - set outer DF always
			// - clear outer DF always
			//-- msa
			outer.iHdr->SetFlags((TUint8)(ip->ip4.Flags() & KInet4IP_DF));
			outer.iHdr->SetProtocol(KProtocolInetIpip);
			}
		else
			{
			// IPv6 in IPv4

			outer.iHdr->SetTOS(ip->ip6.TrafficClass());
			outer.iHdr->SetProtocol(KProtocolInet6Ipip);
			}
		// Fix outer IPv4 header checksum
		outer.ComputeChecksum();
		}
	else
		{
		// Adding outer IPv6 layer

		aPacket.PrependL(sizeof(TInet6HeaderIP));
		TInet6Packet<TInet6HeaderIP> outer(aPacket);
		if (outer.iHdr == NULL)
			return EIpsec_RMBUF;
		outer.iHdr->Init();								// For now, class & flow id are ZERO.
		outer.iHdr->SetPayloadLength(aInfo.iLength);	// [PrependL didn't touch iLength]
		outer.iHdr->SetDstAddr(aTunnel);				//
		outer.iHdr->SetSrcAddr(TInetAddr::Cast(aInfo.iSrcAddr).Ip6Address());
		outer.iHdr->SetHopLimit(outerTtl());
		if (inner_ip4)
			{
			// IPv4 in IPv6
			//
			// (the inner IPv4 is assumed to be complete, including the checksum)
			outer.iHdr->SetTrafficClass(ip->ip4.TOS());
			outer.iHdr->SetNextHeader(KProtocolInetIpip);
			}
		else
			{
			// IPv6 in IPv6
			outer.iHdr->SetTrafficClass(ip->ip6.TrafficClass());
			outer.iHdr->SetNextHeader(KProtocolInet6Ipip);
			}
		aInfo.iLength += sizeof(TInet6HeaderIP);		// [as PrependL didn't do this]
		aInfo.iProtocol = KProtocolInet6Ip;				// Outer is IPv6
		}
	return KErrNone;
	}

TInt TIpsecIPIP::ApplyL(TIpAddress &aTunnel, RMBufRecvPacket &aPacket, RMBufRecvInfo &aInfo)
	/**
	* Process inbound tunnel header.
	*
	* For easy handling of tunneled fragments, don't do actual detunneling
	* in the IPsec hook. Let it happen implicitly at IP layer. However,
	* pick up the outer source address of the tunnel.
	*	
	* The source address is the other end of the tunnel. When this
	* is called, it has already been determined that the current
	* host is the other end point. Thus, the outer destination (= me)
	* is just thrown away.
	*
	* @retval aTunnel The outer source address.
	* @param aPacket The received packet
	* @param aInfo The information block.
	* @return Next Protocol (KProtocolInetIpip or KProtocolInet6Ipip).
	*/
	{
	(void)aPacket;
	aTunnel.SetAddress(aInfo.iSrcAddr);
	return aInfo.iProtocol;
	}
