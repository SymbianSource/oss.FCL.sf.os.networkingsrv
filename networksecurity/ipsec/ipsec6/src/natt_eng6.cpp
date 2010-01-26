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
// natt_eng6.cpp - Nokia VPN specific UDP encapsulating
// natt_eng.cpp - Nokia VPN specific and IETF specified UDP encapsulating of ESP packet
// This class takes care of Nokia VPN specific and IETF specified UDP encapsulating
// ESP UDP capsulating means the following tunnel mode ESP packet encapsulation. 
// | IPV4 HDR  | UDP |  ESP    |  IP HDR  !    DATA    !  EPS   !  ESP  !
// |   New     | Hdr |  Header | Original !            ! Trailer!  AUTH !
// |<---------- encrypted --------->| 
// |<------------- authenticated ------------>|
// where UDP port number is configurable
// For example:
// -- IETF specified UDP encapsulation: Port 4500
// -- The "native" Nokia specified UDP encapsulation: Port 9872
// -- Policy specified "raw" UDP encapsulation: Port xxxx
// This kind of encapsulation is required for handling IPSEC traffic
// in networks with NAT devices.
// OBS: UDP header does not effect to the byte counts in SA object !
// Nokia VPN specific UDP encapsulating of ESP packet
// class
//

#include "ip6_hdr.h"
#include "ext_hdr.h"
#include "in_chk.h"
#include "udp_hdr.h"
#include "sadb.h"
#include <networking/ipsecerr.h>
#include "natt_eng.h"

//
//  GetNextheaderL
//  **********
//  Check that current packet is IPV4 packet and return IP header length
//  and pointer to next_header field
//
//  An outgoing packet MUTS be in the following format when it arrives to
//  NATT_ENG
//    +-----------+---------+----------+------------+--------+-------+
//    | IPV4 HDR  |  ESP    |  IP HDR  !    DATA   
//    |   Outer   |  Header | Original !            ! Trailer!  AUTH !
//    +-----------+---------+----------+------------+--------+-------+
//                          |<---------- encrypted --------->| 
//                |<------------- authenticated ------------>|
//
//
//  The packet is not modified by this
//
static TUint8 *GetNextheaderL(RMBufPacketBase &aPacket, TInt &aLength)
    {
    TPacketPoker pkt(aPacket);
    TInet6HeaderIP4 *ip;
    TUint8 *next_header = NULL;

    //
    // There must be at least the first RMBuf, containing the IPv6 or IPv4
    // header in full! "Autodetect" between IPv4 and IPv6
    //
    ip = (TInet6HeaderIP4 *)pkt.ReferenceL(TInet6HeaderIP4::MinHeaderLength());
    if (ip->Version() == 4) {
       next_header = (TUint8 *)ip + TInet6HeaderIP4::O_Protocol;
       aLength     = ip->HeaderLength();
    }

    return next_header;
    }

//
//  TIpsecNATT::Overhead
//  *******************
//  Return maximum possible overhead caused by this ESP SA
//
TInt TIpsecNATT::Overhead() const
    {
    
    return sizeof(TInet6HeaderUDP); 
    }

//
//  TIpsecNATT::ApplyL
//  *****************
//  NAT traversal processing of the outbound packet (UDP encapsuation)
//
//  This is part of IPv6 outgoing packet hook processing.
//  -   aHead parameter is present in case processing needs
//      some additional information from it (not all hooks
//      have need for it)
//  -   aPacket is the outgoing packet. It is *FULL* packet
//      and already includes the outgoing IPv6 header. If
//      the hook needs to do any modifications to the outgoing
//      packet, it must make them into aPacket.
//  -   aPacket has an info block associated. The info->iLength
//      *MUST* be maintained, if any change affects it.
//
//  Returns (as specified for the hook mechanism)
//  -   Any Leave causes the packet to be dropped (hook must
//      take care not cause a leave, if it frees that packet
//      for < 0 return).
//  -   return = 0, pass on to the next output hook
//  -   return < 0, the hook took "ownership" of the packet (usually
//      just dropped it explicitly), "Send aborted" return!
//  -   return > 0, This return value is not used at this
//      point. Do not return with > 0!
//
TInt TIpsecNATT::ApplyL(CNatTraversal* aNatTraversal, RMBufSendPacket &aPacket, RMBufSendInfo &aInfo)
    {
    RMBufChain payload;         
    TInt result = KErrNone;

    if ( !aNatTraversal ) {
       return result;     // No NAT traversal required  
    }

    TInt ipv4hlen;
    TUint8 *next_header = GetNextheaderL(aPacket, ipv4hlen);
    if ( !next_header ) {
       return KErrNone;   // Not A IPV4 packet. NAT-Traversal not required
    }   
    aPacket.SplitL(ipv4hlen, payload); // <-- Must have "split > 0" due SplitL "feature"!! -- msa
    for (;;)        // Only for easy exits...
        {
        //
        // Compute the required extensions to UDP header
        //
        const TInt udphlen = TInet6HeaderUDP::MinHeaderLength();

        TRAP(result, if (payload.IsEmpty()) payload.AllocL(udphlen); else payload.PrependL(udphlen));
        if (result != KErrNone)
            break;
        aInfo.iLength += udphlen;
        
        TInet6Packet<TInet6HeaderUDP> udp;
        udp.Set(payload, 0, udphlen);
        if (udp.iHdr == NULL) {
           result = EIpsec_RMBUF;
           break;
        }
        //
        // Fill NAT Traversal UDP header fields
        // UDP checksum is NOT used, zero value shall be used
        // 
        udp.iHdr->SetDstPort(aNatTraversal->GetDestPort());
        udp.iHdr->SetSrcPort(aNatTraversal->GetSrcPort());
        udp.iHdr->SetLength(aInfo.iLength - ipv4hlen);          
        udp.iHdr->SetChecksum(0);     /* No UDP checksum used */
        //
        // Update the next header chaining (in IP header)
        //
        *next_header = KProtocolInetUdp;
        
        TInet6Checksum<TInet6HeaderIP4> ip(aPacket);
        if (ip.iHdr == NULL) {
            result = EIpsec_RMBUF;
            break;
        }
    /**--------------------------------------------------------------------------------
     *
     * Change destination IP address in IP header if defined so in NAT traversal object
     * This means that the peer station of SA is behind a NAT device
     *
     *-------------------------------------------------------------------------------*/
        if ( aNatTraversal->UseDestIPAddr() ) {
           TInetAddr* DestIPaddr = aNatTraversal->GetDestIPAddr();
           ip.iHdr->SetDstAddr(DestIPaddr->Address());
           TInetAddr::Cast(aInfo.iDstAddr).SetAddress(DestIPaddr->Address());
        }
        ip.iHdr->SetTotalLength(aInfo.iLength);             
        ip.ComputeChecksum();  //  Compute New IPV4 header checksum     
        
        break;      // -- NO REAL LOOP, BREAK ALWAYS --
        }
    
    aPacket.Append(payload);
    return result;
    }

//
//  TIpsecNATT::ApplyL
//  *****************
//      Unwrap possible NAT traversal UDP header  from a received packet.
//
//  This is part of the IPv6 incoming packet hook system, which means
//  -   updating info is not required (actually should not be done),
//      *UNLESS* the aHead.iPacket itself is modified! (info->iLength
//      must always contain the length of the aHead.iPacket).
//
//  *   TIpsecNATT::ApplyL will change the info->iLength, if UDP is
//  *   removed from the aHead.iPacket!
//
//  -   applied changes are made to the TPacketHead
//  -   the current "scanning point" in the aHead.iPacket is indicated
//      by the aHead.iOffset. The remaining "unscanned" packet length
//      is info->iLength - aHead.iOffset.
//  -   when a hook desides to advance in the aHead.iPacket (or if it
//      changes the IP extension/upper layer header at iOffset), it
//      must return the protocol id of the new header (it must not
//      change the next header field of the aHead.ip6). This is done
//      in the main loop and at entry this contains the protocol id
//      of the current header indicated by the iOffset!
//
//
//  *NOTE*
//      A minor difference to the Hook semantics: < 0 return does not
//      indicate the packet has been freed, but it is indication to
//      the caller that this should be done.
//
//  Returns
//  -   any Leave causes packet to be dropped
//  -   return < 0, cause the packet to be dropped
//  -   return = 0, pass the packet to the next hook of the same protocol
//  -   return > 0, hook changed the iOffset/iPacket, topmost protocol
//      has been changed
//
//
TInt TIpsecNATT::ApplyL(CSecurityAssoc* &aSa, RMBufRecvPacket &aPacket, RMBufRecvInfo &aInfo)
    {
    RMBufChain payload;
    TInt result = EIpsec_NotANATTPacket;  /* default */

    //
    // After SplitL plen MUST always be same as payload.Length()!
    //
    TInt plen = aInfo.iLength - aInfo.iOffset;
    
    //
    // aHead.iOffset points to the beginning of the UDP. UDP needs to
    // deleted from the middle, thus Split is required in any case.
    //
    aPacket.SplitL(aInfo.iOffset, payload); // <-- Must have "aInfo.iOffset > 0" due SplitL "feature"!! -- msa
	if (payload.IsEmpty())
		return result;	// Play safe, and just say "NO NAT"
    
    for (;;)        // Just for simple exits
        {

        TInet6Packet<TInet6HeaderUDP> udp(payload);
        if (udp.iHdr == NULL)
        {
           result = EIpsec_CorruptPacketIn;         
           break;
        }
        //
        // Copy possible SPI data behind UDP header 
        // If packet is a UDP encapsulated ESP packet the header is format UDP+ESP.
        // The SPI value shall be found from the begin of ESP header (SPI length 4 octets)
        //
        //
        const TInt udphlen = udp.iHdr->HeaderLength();
        if ( plen < (TInt)(udphlen + sizeof(TInet6HeaderESP)) )
        {
            break; // NOT A NAT TRAVERSAL UDP PACKET                 
        }            

        TUint32 spi;
        TPtr8 spiptr((TUint8*)&spi, 4, 4);
        payload.CopyOut(spiptr, udphlen);
        if ( spiptr.Length() != 4 )
        {
            break; // NOT A NAT TRAVERSAL UDP PACKET                 
        }
    /**--------------------------------------------------------------------------
     *
     * Check is the UDP packet destinated to the NAT Traversal port:
     *  - Take SPI value from ESP header and find corresponding IPSEC SA
     *  - If SA contains CNatTraversal object instance check that current UDP
     *    destination port corresponds source port defined in there.
     *  - If UDP port corresponds packet is handled as a UDP capsulated ESP packet.
     *    Otherwise UDP packet is a "normal" UDP packet
     * Special rule:
     * SPI value 0x00000000 is a "non ESP marker" which means that is NOT an
     * UDP capsulated ESP packet (it is a IKE message).
     *
     *---------------------------------------------------------------------------*/
        if ( spi == NON_ESP_MARKER ) {  
           break;  // NOT A NAT TRAVERSAL UDP PACKET                 
        }
        aSa = iManager->Lookup(SADB_SATYPE_ESP, spi, TIpAddress(aInfo.iDstAddr));       
        if ( !aSa ) {
           break;  // NOT A NAT TRAVERSAL UDP PACKET 
        }
        
        CNatTraversal* NatTraversal = aSa->NatTraversal();
        if ( !NatTraversal || (NatTraversal->GetSrcPort() != udp.iHdr->DstPort()) ) {
           break;  // NOT A NAT TRAVERSAL UDP PACKET         
        }   
        //
        // The packet is an UDP encapsulated ESP packet.
        // Remove UDP header from packet.
        // (NAT-T processing does not change the iPrevNextHdr offset value!)
        //
        result = KProtocolInetEsp;
        aPacket.CopyIn(TPtrC8((TUint8 *)&result, 1), aInfo.iPrevNextHdr);
        
        payload.TrimStart(udphlen);
        aInfo.iLength -= udphlen;
        //
        // The real packet length has been modified, modify the original outermost
        // payload length in the IPv6 header (only for the case where an ICMP error
        // might be generated from this packet!)
        //
        TInet6Packet<TInet6HeaderIP4> ip(aPacket);
        if (ip.iHdr->Version() == 4)
            ip.iHdr->SetTotalLength(aInfo.iLength);
            // IPv4 header checksum not recomputed... 
        else
            ((TInet6HeaderIP *)ip.iHdr)->SetPayloadLength(aInfo.iLength - sizeof(TInet6HeaderIP));      
        break;      // -- NO REAL LOOP, BREAK ALWAYS --
        
        }
    aPacket.Append(payload);
    
    return result;
    }
