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
// Name        : 6to4_flow.cpp
// Part of     : 6to4 plugin / 6to4.prt
// Implements 6to4 automatic and configured tunnels, see
// RFC 3056 & RFC 2893
// Version     : 0.2
//




// INCLUDE FILES
#include <in_chk.h>
#include "6to4_flow.h"

// EXTERNAL DATA STRUCTURES
// EXTERNAL FUNCTION PROTOTYPES  
// CONSTANTS
// MACROS
// LOCAL CONSTANTS AND MACROS

// MODULE DATA STRUCTURES
// LOCAL FUNCTION PROTOTYPES
// FORWARD DECLARATIONS

// ============================= LOCAL FUNCTIONS ==============================

// ============================ MEMBER FUNCTIONS ==============================


// ----------------------------------------------------------------------------
// C6to4FlowInfo::C6to4FlowInfo
// ----------------------------------------------------------------------------
C6to4FlowInfo::C6to4FlowInfo(const TPacketHead &aHead)
	: iInterfaceIndex(aHead.iInterfaceIndex)
	{
	// Need to save the inner selector information which is
	// restored at ReadyL.
	iSrcPort = aHead.iSrcPort;
	iDstPort = aHead.iDstPort;
	iIcmpType = aHead.iIcmpType;
	iIcmpCode = aHead.iIcmpCode;
	iInnerIp = aHead.ip6;
	iDstId = aHead.iDstId;
	iSrcId = aHead.iSrcId;
	}
	
// ----------------------------------------------------------------------------
// C6to4FlowInfo::~C6to4FlowInfo
// ----------------------------------------------------------------------------
C6to4FlowInfo::~C6to4FlowInfo ()
	{
	iPacket.Free();
	}

// ----------------------------------------------------------------------------
// 6to4FlowInfo::ReadyL
// The stack asks if the flow is ready to send. In the same time, the plugin
// configures the source IP address from the flow, including configuring it to
// the virtual interface. Set the packet source and destination IP addresses
// for the inner (IPv6) header. Set the packet interface index. Set the packet 
// next header to be the inner header next header and set the packet IP version
// to 6.
// ----------------------------------------------------------------------------
//
TInt C6to4FlowInfo::ReadyL (TPacketHead & aHead)
	{
	// Save the fixed packet content from the TPacketHead
	iOffset = aHead.iOffset;
	iPacket.Free();
	if (iOffset > 0)
		{
		iPacket.Assign(aHead.iPacket);
		aHead.iOffset = 0;
		}

	// Save the outer IP information
	iOuterIp = aHead.ip6;

	// Set inner header source and dest addresses
	aHead.ip6 = iInnerIp;
	// Inner Version() is not yet set correctly, make it correct.
	aHead.ip6.SetVersion(iInnerIp.DstAddr().IsV4Mapped() ? 4 : 6);
	aHead.iSrcId = iSrcId;
	aHead.iDstId = iDstId;
	aHead.iInterfaceIndex = iInterfaceIndex;
	// Both next header and iProtocol must have the same value
	// as the iPacket is now empty.
	aHead.iProtocol = (TUint8)iInnerIp.NextHeader();
	aHead.iSrcPort = iSrcPort;
	aHead.iDstPort = iDstPort;
	aHead.iIcmpType = iIcmpType;
	aHead.iIcmpCode = iIcmpCode;
	return EFlow_READY;
	}

// ----------------------------------------------------------------------------
// 6to4FlowInfo::ApplyL
// Final stage where the 6to4 touches the outgoing packet. Space is made for
// the IP header and the IP header is initialized.
// ----------------------------------------------------------------------------
TInt C6to4FlowInfo::ApplyL (RMBufSendPacket & aPacket, RMBufSendInfo & aInfo)
	{
	// Apply the tunnelling

	// Prepend the fixed content (if any) below the IP header
	if (iOffset > 0)
		{
		RMBufChain work;
		// Note: assumed that if CopyL leaves, then work is empty
		iPacket.CopyL(work);
		aPacket.Prepend(work);
		aInfo.iLength += iOffset;
		}
	switch(iOuterIp.Version())
		{
		case 6:
			{
			aPacket.PrependL(sizeof(TInet6HeaderIP));
			aInfo.iLength += sizeof(TInet6HeaderIP);
			aInfo.iProtocol = KProtocolInet6Ip;

			TInet6Packet<TInet6HeaderIP> ip(aPacket);
			if (ip.iHdr == NULL)
				User::Leave(KErrGeneral);

			// Build outer IPv6 header (already in iOuterIp, just copy)
			*ip.iHdr = iOuterIp;
			// Note: Certain packets must not have ECN ECT bits set. tcp_sap.cpp sets
			// KIpNoEcnEct for those packets.
			if (aInfo.iFlags & KIpNoEcnEct)
				{
				ip.iHdr->SetTrafficClass(ip.iHdr->TrafficClass() & 0xfc);
				}
			ip.iHdr->SetPayloadLength(aInfo.iLength);
			break;
			}
		case 4:
			{
			// Make space for IPv4 header
			aPacket.PrependL (20);
			aInfo.iLength += 20;
			aInfo.iProtocol = KProtocolInetIp;

			TInet6Checksum < TInet6HeaderIP4 > ip (aPacket);
			if (ip.iHdr == NULL)
				User::Leave (KErrGeneral);

			// Build the outer IPv4 header
			ip.iHdr->Init(iOuterIp.TrafficClass() & ((aInfo.iFlags & KIpNoEcnEct) ? 0xfc : 0xff));
			// Raw Address load. Someone else must have already checked that
			// both src and dst are IPv4 mapped addresses at the end of the
			// ReadyL phase! This just does a blind copy.
			ip.iHdr->DstAddrRef() = iOuterIp.DstAddr().u.iAddr32[3];
			ip.iHdr->SrcAddrRef() = iOuterIp.SrcAddr().u.iAddr32[3];

			// Somewhat ad hoc thing: if DontFragment flag is set, then
			// set the DF bit to the IPv4 header...
			if (aInfo.iFlags & KIpDontFragment)
				ip.iHdr->SetFlags((TUint8)(ip.iHdr->Flags() | KInet4IP_DF));
	
			ip.iHdr->SetHeaderLength (20);
			ip.iHdr->SetTotalLength (aInfo.iLength);
			ip.iHdr->SetTtl(iOuterIp.HopLimit());

			// NOTE!!! Something might be done for this. Now it is theoretically
			// possible that a same host receives protocol 41 packets from us and
			// we have two or more flows with protocol 41 AND two packets get 
			// fragmented and are reassembled in the same time by the remote host
			// AND the IPv4 identifier happens to be the same. That would break the
			// reassembly process, but since this is practically "impossible", it is
			// left now without a fix.
			ip.iHdr->SetIdentification (iPacketID++);                                                                
			ip.iHdr->SetProtocol (iOuterIp.NextHeader());

			// Checksum of the outer header.
			ip.ComputeChecksum ();
			break;
			}
		default:
			User::Leave(KErrGeneral);
		}

	// Pass the packet
	return 0;
	}

// ----------------------------------------------------------------------------
// 6to4FlowInfo::Open
// Open the flow, keep reference count.
// ----------------------------------------------------------------------------
//

void C6to4FlowInfo::Open ()
	{
	iRefs++;
	}

// ----------------------------------------------------------------------------
// 6to4FlowInfo::Close
// Close the flow if the reference count has reached zero. Remove the flow 
// from any list it is stored.
// ----------------------------------------------------------------------------
//
void C6to4FlowInfo::Close ()
	{
	if (--iRefs < 0)
		{
		delete this;
		}
	}

// ========================== OTHER EXPORTED FUNCTIONS ========================

//  End of File  
