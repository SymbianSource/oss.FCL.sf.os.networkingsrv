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
// ah_eng6.cpp - IPv6/IPv4 IPSEC authentication header
// The IPSEC Authentication Header (AH) packet processing class
//



/**
 @file ah_eng6.cpp
*/

#include "ip6_hdr.h"
#include "in_chk.h"
#include "ext_hdr.h"
#include "sadb.h"
#include "sa_crypt.h"
#include <networking/ipsecerr.h>
#include "ah_eng.h"


static void FeedZeroes(CAuthenticationBase *aEngine, TInt aLength)
	/**
	* Feed a sequence zeroes to authentication engine.
	*
	* @param aEngine The authentication engine
	* @param aLength Number of zero bytes to feed.
	*/
	{
	const static TUint8 ZeroFiller[16] = {0,};

	while (STATIC_CAST(TUint, aLength) > sizeof(ZeroFiller))
		{
		aEngine->Update(TPtrC8(ZeroFiller, 16));
		aLength -= sizeof(ZeroFiller);
		}
	if (aLength > 0)
		aEngine->Update(TPtrC8(ZeroFiller, aLength));
	}

static void ZeroMutableOptions(const TDes8 &aOptions)
	/**
	* Zero mutable IPv4 options.
	*
	* Only few IPv4 optios are preserved unchanged on route. Integrity
	* computations for others must be disabled by assuming zero values.
	*
	* The hard coded option numbers are based on RFC-2402, Appendix A.
	*
	* @param aOptions The IPv4 options buffer.
	*/
	{
	TUint8 *opt = (TUint8 *)aOptions.Ptr();
	TInt count = aOptions.Length();

	while (count > 0)
		{
		// Option numbers from RFC1700

		if (*opt < 2)
			{
			++opt;	// END/PAD bytes, immutable, pass
			count -= 1;
			}
		else
			{
			int optlen = opt[1];
			if (optlen > count || optlen < 2)
				// An illegal option length, just bail out by treating
				// the rest of the options as single value. (The packet
				// is broken anyway, and if received packet, this may
				// be someone trying to trick us into a loop with zero
				// as option length!!)
				optlen = count;
			count -= optlen;
			switch (*opt & 0x1f)
				{
			case 2:		// Security
			case 5:		// Extended Security
			case 6:		// Commercial Security
			case 20:	// Router Alert
			case 21:	// Sender directed multidestination delivery
						// ... pass above as is.
				opt += optlen;
				break;
			default:	// ...and all other options are totally zeroed
						// (including the type and length fields!!)
				while (--optlen >= 0)
					*opt++ = 0;
				break;
				}
			}
		}
	}

static TUint8 *Pass1ScanL(RMBufPacketBase &aPacket, TIp6Addr **aFinal, TInt &aLength, TInt aLookFor = -1)
	/**
	* Peek information from the extension headers.
	*
	* @li	locate the last extension header (or the next header in IPv6
	*		header, if no extension headers), and return a pointer to it,
	*		and the length including this header.
	*	
	* @li	if a routing header (type 0) is present return a reference
	*		to the last address in it.
	*
	* This code skips over all known extension headers (IsExtensionHeader),
	* unless told to stop at some specific header (aLookFor).
	*
	* The packet is not modified by this.
	*
	* @param aPacket	The packet to peek
	* @retval aFinal	The last address in RTH, if present, or NULL
	* @retval aLength	Offset of the first header after extension headers.
	@ @param aLookFor	Stop peek if this extension header found.
	* @return The pointer to next Header/Protocol field in the packet.
	*/
	{
	TPacketPoker pkt(aPacket);
	TInet6HeaderIP4 *ip;
	TUint8 *next_header;

	// There must be at least the first RMBuf, containing the IPv6 or IPv4
	// header in full! "Autodetect" between IPv4 and IPv6
	ip = (TInet6HeaderIP4 *)pkt.ReferenceL(TInet6HeaderIP4::MinHeaderLength());
	if (ip->Version() == 4)
		{
		next_header = (TUint8 *)ip + TInet6HeaderIP4::O_Protocol;
		aLength = ip->HeaderLength();
		// Strictly, for IPv4 only header needs to be skipped, but
		// for now drop through to the normal IPv6 lookup, and
		// implement a "feature" for someone who is crazy enough
		// to use IPv6 extension headers under IPv4.  -- msa
#if 0	// <-- Change to 1 to disable the "feature"
		return next_header
#endif
		}
	else
		{
		next_header = (TUint8 *)ip + TInet6HeaderIP::O_NextHeader;	// assumes O_NextHeader < TInet6HeaderIP4::MinHeaderLength()
		aLength = sizeof(TInet6HeaderIP);
		}
	pkt.SkipL(aLength);

	while (pkt.IsExtensionHeader(*next_header) && pkt.More())
		{
		union
			{
			TInet6HeaderExtension *ext;
			TInet6HeaderRouting *rth;
			TUint8 *dat;
			};
		ext = (TInet6HeaderExtension *)pkt.ReferenceL(sizeof(TInet6HeaderExtension));

		// TInet6HeaderExt already guarantees 8 octets, so
		// the base part of the Routing Header is also
		// directly accessible, just use the rth variant
		if (*next_header == KProtocolInet6RoutingHeader && rth->RoutingType() == 0)
			{
			// Return a ptr to the last component of the
			// routing header.
			TInt extlen = rth->HdrExtLen();
			extlen = (extlen << 3) + TInet6HeaderRouting::O_Address;	// = offset to the address.
			pkt.SkipL(extlen);
			*aFinal = (TIp6Addr *)pkt.ReferenceAndSkipL(sizeof(TIp6Addr));
			}
		else
			pkt.SkipL(ext->HeaderLength());
		if (*next_header == aLookFor)
			break;
		aLength += ext->HeaderLength();
		next_header = dat;
		}
	return next_header;
	}


static void Pass2ScanL(
	CAuthenticationBase *aEngine,
	RMBufPacketBase &aPacket,
	TIp6Addr *aDst,
	RMBufChain &aPayload)
	/**
	* Compute the checksum (ICV).
	*
	* Do the actual ICV computation:
	*
	* @li	Process HopByHop and Destination options as described in the
	*		IPsec AH documentation
	* @li	If a routing header (type 0) is present, process it and the
	*		addresses, simulating the state of the header as it will be
	*		at the receiving end.
	*
	* @param aEngine The authentication engine
	* @param aPacket The packet
	* @param aDst If non-NULL, there is RTH and this points to the last address.
	* @param aPayload The payload part of the packet (after AH header).
	*/
	{
	TPacketPoker pkt(aPacket);
	TUint8 next_header;
	TInt len;
	TInet6HeaderIP4 *ip;
	//Variable ip6 is intialised but never used-this needs investigation
	TInet6HeaderIP *ip6 = NULL;

	aEngine->Init();
	ip = (TInet6HeaderIP4 *)pkt.ReferenceL(TInet6HeaderIP4::MinHeaderLength());
	if (ip->Version() == 4)
		{
		TInt hlen = ip->HeaderLength();
		TInet6HeaderIP4 ipo;
		(void)Mem::Copy(&ipo, ip, hlen);
		next_header = (TUint8)ipo.Protocol();
		pkt.SkipL(hlen);

		// Clean out mutable fields
		ipo.SetTOS(0);
		ipo.SetFlags(0);
		ipo.SetChecksum(0);
		ipo.SetTtl(0);
		ZeroMutableOptions(ipo.Options());
		// What should be done in the weird case if aDst is non-NULL (would mean
		// that there was IPv6 routing header inside IPv4 packet!)? Assúme IPv4 mapped
		// address and place into ipo? Not done at this point... -- msa
		aEngine->Update(TPtrC8((TUint8 *)&ipo, ipo.HeaderLength()));
		}
	else
		{
		ip6 = (TInet6HeaderIP *)pkt.ReferenceAndSkipL(sizeof(TInet6HeaderIP));
		TInet6HeaderIP ipo = *ip6; 
		
		next_header = (TUint8)ipo.NextHeader();
		ipo.SetTrafficClass(0);
		ipo.SetFlowLabel(0);
		ipo.SetHopLimit(0);
		if (aDst)	// Mutable and predictable destination address?
			{
			aEngine->Update(TPtrC8((TUint8 *)&ipo, sizeof(TInet6HeaderIP) - sizeof(TIp6Addr)));
			aEngine->Update(TPtrC8((TUint8 *)aDst, sizeof(TIp6Addr)));
			}
		else
			aEngine->Update(TPtrC8((TUint8 *)&ipo, sizeof(TInet6HeaderIP)));
		}
	while (pkt.IsExtensionHeader(next_header) && pkt.More())
		{
		union
			{
			TInet6HeaderExtension *ext;
			TInet6HeaderRouting *rth;
			TInet6Options *oh;
			TUint8 *dat;
			};
		ext = (TInet6HeaderExtension *)pkt.ReferenceL(sizeof(TInet6HeaderExtension));
		if (next_header == KProtocolInet6HopOptions ||
			next_header == KProtocolInet6DestinationOptions)
			{
			// The somewhat "tricky" code below attemps to avoid
			// many calls to the digest update method by combining
			// as many contigous immutable sections as possible
			// into a single "chunk". The first "immutable" section
			// is the fixed part of the option header itself.
			TInt chunk = TInet6Options::O_Options;
			TUint8 *start = dat;
			TInt count = oh->HeaderLength() - chunk;
			pkt.SkipL(chunk);
			while (count > 0)
				{
				dat = pkt.ReferenceL();
				if (*dat == KDstOptionPad1)
						len = 1;
				else
					{
						dat = pkt.ReferenceL(2);
						len = 2 + dat[1];
					}
				count -= len;
				if (chunk == 0)
					start = dat;	// see NOTE 1 below...
				else if (pkt.AtBegin())
					{
					// MBuf changed, flush out previous segment
					aEngine->Update(TPtrC8(start, chunk));
					start = dat;
					chunk = 0;
					}
				// The Data portion of the mutable options must
				// must be treated as ZEROES. However, the Type
				// and length of such uptions are still "immutable"!
				if ((*dat & 0x20) != 0)
					{
					// Mutable Option: flush out previous
					// immutable chunk and type/length of the
					// current option (+2).
					aEngine->Update(TPtrC8(start, chunk + 2));

					// Feed equivalent of "Option Data" as zeroes.
					FeedZeroes(aEngine, len - 2);
					pkt.SkipL(len);
					chunk = 0;
					// *Note 1*
					// Cannot initialize start with ReferenceL() here, because
					// this could have been the last option and above SkipL
					// consumed it all (start would at worst point beyond the
					// end of the buffer). If more options are coming, rely on
					// "chunk==0" test in above code to initialize the start!
					}
				else
					{
					// Immutable Option. As option may be extending
					// over multiple MBufs. loop over and feed the
					// digest until the last remaining segment of
					// the option is in single mbuf --> chunk
					while (pkt.Remainder() < len)
						{
						// Option extends to the next MBuf, at least by one byte!
						// Flush out the current buffer and go to the next.
						aEngine->Update(TPtrC8(start, chunk + pkt.Remainder()));
						len -= pkt.Remainder();		// len > 0! Always!
						pkt.SkipL(pkt.Remainder());
						// Need to start a new chunk from the beginning of the
						// next MBuf (must initialize start too, because chunk
						// *will* be non-zero (len > 0!)
						start = pkt.ReferenceL();
						chunk = 0;
						}
					// The remaining 'len' bytes (> 0) are guaranteed to be a
					// contiguous with the previous chunk. Just add to chunk.
					chunk += len;
					pkt.SkipL(len);
					}
				}
			ASSERT(count == 0);
			if (chunk > 0)
				aEngine->Update(TPtrC8(start, chunk));	// Flush out tail if any!
			}
		else if (aDst && next_header == KProtocolInet6RoutingHeader && rth->RoutingType() == 0)
			{
			// *NOTE* Routing header needs special treatment only on send side (aDst != NULL)
			//
			// *NOTE* Works correctly only with one routing header (does not
			// crash if multiple present).
			TInt left = rth->SegmentsLeft();
			rth->SetSegmentsLeft(0);
			aEngine->Update(TPtrC8(pkt.Ptr(), sizeof(*rth)));	// Base part.
			rth->SetSegmentsLeft(left);

			// For now, make this work only for IPv6 (as it should). Let the dubious
			// case of Routing header inside IPv4 fail for now. To make *that* work,
			// would perhaps need to assume IPv4 mapped address in routing header and
			// convert the current IPv4 destionation into mapped address for the
			// computation...
			// Variable Update is never be called as ip6 never intialised-this needs investigation
			if (ip6)
				aEngine->Update(TPtrC8((TUint8 *)&ip6->DstAddr(), sizeof(TIp6Addr)));

			// Do the addresses from the routing Header, except the last
			TInt length = rth->HeaderLength() - TInet6HeaderRouting::O_Address - sizeof(TIp6Addr);	
			pkt.SkipL(TInet6HeaderRouting::O_Address);
			while (length > 0)
				{
				TInt chunk = pkt.Remainder();
				if (length < chunk)
					chunk = length;
				aEngine->Update(TPtrC8(pkt.Ptr(), chunk));
				length -= chunk;
				pkt.SkipL(chunk);
				}
			pkt.SkipL(sizeof(TIp6Addr));	// skip last segment! Already
											// done as a part of IPv6 hdr
											// processing.
			}
		else
			{
			// Unknown extension header, assume immutable and
			// feed to the authentication engine as is.
			TInt length = ext->HeaderLength();
			while (length > 0)
				{
				TInt chunk = pkt.Remainder();
				if (length < chunk)
					chunk = length;
				aEngine->Update(TPtrC8(pkt.Ptr(), chunk));
				length -= chunk;
				pkt.SkipL(chunk);
				}
			}
		next_header = (TUint8)ext->NextHeader();
		}

	// Feed rest of the packet into iAeng!!
	TMBufIter m(aPayload);
	RMBuf *p = m++;

	// The payload starts with Authentication header, of which the
	// base part is guaranteed to be accessible. (p is guaranteed
	// to be non-null initially). The fixed part of AH is fed into
	// the digest as is. The authentication field is fed as ZEROES.
	TInet6HeaderAH *ah = (TInet6HeaderAH *)p->Ptr();
	aEngine->Update(TPtrC8((TUint8 *)ah, TInet6HeaderAH::MinHeaderLength()));
	len = ah->DataLength();
	FeedZeroes(aEngine, len);	// ICV is zero.

	// Skip AH from the payload
	len += TInet6HeaderAH::MinHeaderLength();
	while (p->Length() < len)
		{
		//
		// The ICV continues in the next buffer
		// (Note it is assumed that caller has assured that
		// the packet at least contains complete AH).
		len -= p->Length();
		if ((p = m++) == NULL)
			User::Leave(EIpsec_RMBUF);	// Is really a program error!

		}
	if (len > 0)
		{
		// The remaining 'len' AH octets are on this buffer, need to
		// start digest compute after them.
		aEngine->Update(TPtrC8(p->Ptr() + len, p->Length() - len));
		p = m++;
		}
	while (p)
		{
		aEngine->Update(TPtrC8(p->Ptr(), p->Length()));
		p = m++;
		}
	}

TInt TIpsecAH::Overhead(const CSecurityAssoc &aSa) const
	/**
	* Return maximum possible overhead caused by this AH SA.
	*
	* @param aSa The Security Association
	* @return The overhead (bytes).
	*/
	{
	if (!aSa.iAeng)
		return 0;
	return ((aSa.iAeng->DigestSize()) + sizeof(TInet6HeaderAH) + 7) & ~0x7;
	}


TInt TIpsecAH::ApplyL(CSecurityAssoc &aSa, RMBufSendPacket &aPacket, RMBufSendInfo &aInfo)
	/**
	* Apply IPsec AH processing to outbound packet.
	*
	* @li	aPacket is the outgoing packet. It is *FULL* packet
	*		and already includes the outgoing IP header.
	* @li	aInfo is the associated info block. The info->iLength
	*		*MUST* be maintained, if any change affects it.
	*
	* @param aSa The security association (AH type)
	* @param aPacket The packet to process
	* @param aInfo The packet info
	* @return
	*	@li	Normally leaves on any error condition, which causes
	*		the packet to be dropped,
	*	@li	KErrNone, the AH header has been added.
	* 
	*	@li < 0, some other unexpected error.
	*/
	{
	RMBufChain payload;
	TInt split;
	TInt result = 0;
	TIp6Addr *final = NULL;
	TUint8 *next_header;

	// Extend the buffer by AH requirements
	// (and make sure both IP and AH headers are
	// in the same contigous memory).
	if (!aSa.iAeng)
		User::Leave(EIpsec_AhAuthAlg);

	// Need to go through the aPacket and process existing destination header
	// and find out what the true IPv6 dest would be at the end node.
	// Find the point to insert the AH header. Currently placed before any
	// destionation options, if present.
	// Also, need to extract the last Next Header field and place it into
	// ah Next header
	TInet6Checksum<TInet6HeaderIP4> ip(aPacket);
	next_header = Pass1ScanL(aPacket, &final, split /*, KProtocolInet6DestinationOptions*/);
	aPacket.SplitL(split, payload);	// <-- Must have "split > 0" due SplitL "feature"!! -- msa
	for (;;)	// Only for easy exits...
		{
		// Compute the AH header size (must be multiple of 8 octets)
		// make allocate room for the AH in the payload part.
		//
		// *NOTE*	May need to zero out the allocated space, because if padding is
		//			present, it must not be left as random content of memory, this
		//			might be a security leak... -- msa
		//			[Currently dealt, beacuse ah.iHdr->ICV() returns a descriptor
		//			which includes the padding, and FillZ is applied to it below]
		const TInt ahlen = ((aSa.iAeng->DigestSize()) + sizeof(TInet6HeaderAH) + 7) & ~0x7;
		TRAP(result, if (payload.IsEmpty()) payload.AllocL(ahlen); else payload.PrependL(ahlen));
		if (result != KErrNone)
			break;
		aInfo.iLength += ahlen;

		TInet6Packet<TInet6HeaderAH> ah(payload);
		if (ip.iHdr == NULL || ah.iHdr == NULL)
			{
			result = EIpsec_AhRMBufSplit;
			break;
			}
		// One more place to return for cleanup... a repeat test for IPv4/6 -- msa

		ah.iHdr->SetNextHeader(*next_header);
		*next_header = KProtocolInetAh;
		ah.iHdr->SetReserved(0);
		ah.iHdr->SetHeaderLength(ahlen);
		ah.iHdr->SetSPI(aSa.iSPI);
		ah.iHdr->SetSequence(++aSa.iSendSeq);

		if (ip.iHdr->Version() == 4)
			{
			TInt plen = ip.iHdr->TotalLength() + ahlen;
			if (plen >= (1 << 16))
				{
				result = EIpsec_AhPacketTooLong;
				break;
				}
			ip.iHdr->SetTotalLength(plen);
			// Needs to be looked into. In ip6.cpp a checksum is computed
			// unnecessarily when IPSEC is applied.Should delay checksum
			// after hooks...
			ip.ComputeChecksum();

			}
		else
			{
			TInt plen = ((TInet6HeaderIP *)ip.iHdr)->PayloadLength() + ahlen;
			if (plen >= (1 << 16))
				{
				result = EIpsec_AhPacketTooLong;
				break;
				}
			((TInet6HeaderIP *)ip.iHdr)->SetPayloadLength(plen);
			}
		if (aSa.iReplayCheck && aSa.iSendSeq == 0)
			{
			// Unless extended sequence numbering is enabled, low order overflow
			// is sufficient, otherwise test the high bits too.
			if ((aSa.iFlags & SABD_SAFLAGS_ESN) == 0 || aSa.iSendSeq.High() == 0)
				{
				// Sequence number is not allowed to wrap around
				// SA needs to be deleted
				// This does not generate PFKEY expired message! Should it? -- msa
				iManager->Delete(&aSa);
				result = EIpsec_AhSequenceWrap;
				break;
				}
			}
		TPtr8 icv(ah.iHdr->ICV());
		icv.FillZ();

		TRAP(result, Pass2ScanL(aSa.iAeng, aPacket, final, payload));
		if (result != KErrNone)
			break;
		// ** If ESN: Add high order bits to digest
		if (aSa.iFlags & SABD_SAFLAGS_ESN)
			{
			TUint8 buf[4];
			const TUint32 seq = aSa.iSendSeq.High();
			buf[3] = (TUint8)seq;
			buf[2] = (TUint8)(seq >> 8);
			buf[1] = (TUint8)(seq >> 16);
			buf[0] = (TUint8)(seq >> 24);
			aSa.iAeng->Update(TPtrC8(buf, 4));
			aSa.iCurrent.iBytes += 4;	// Algorithm applied to 4 additional bytes.
			}
		icv = aSa.iAeng->Final(icv.Length());
		aSa.iCurrent.iBytes += aInfo.iLength;
		result = aSa.MarkUsed(*iManager); // + Expire test for counts
		break;		// NOT really a loop!
		}
	aPacket.Append(payload);	// Join the split.
	ASSERT(aInfo.iLength == aPacket.Length());
	return result;
	}

TInt TIpsecAH::ApplyL(CSecurityAssoc* &aSa, RMBufRecvPacket &aPacket, RMBufRecvInfo &aInfo)
	/**
	* Unwrap IPSEC AH from a received packet.
	*
	* TIpsecAH::ApplyL changes the info.iLength, because it removes
	* the AH from the packet.
	*	
	* -	the current "scanning point" in the aPacket is indicated
	* 	by the aInfo.iOffset. The remaining "unscanned" packet length
	*	is aInfo.iLength - aInfo.iOffset.
	*
	* @param aSa The Security Association (AH type)
	* @param aPacket The packet
	* @param aInfo The packet infomrmation
	*
	* @return
	*	@li	Signals some error conditions with leaving,
	*	@li < 0, Error detected.
	*	@li > 0, Next Protocol (AH passed checks and has been removed)
	*
	*	TIpsecAH::ApplyL will never return 0, because it always either
	*	fails or successfully unwraps the AH and returns the next
	*	protocol layer underneath!
	*/
	{
	TInt ahlen, result = KErrNone;
	RMBufChain payload;			//

	// aHead.iOffset points to the beginning of the AH. AH needs to
	// deleted from the middle, thus Split is required in any case.
	aPacket.SplitL(aInfo.iOffset, payload);	// <-- Must have "aInfo.iOffset > 0" due SplitL "feature"!! -- msa
	if (payload.IsEmpty())
		return EIpsec_CorruptPacketIn;

	// Unfortunately, after this need to catch all LEAVES for
	// not to leave 'payload' hanging around... -- msa
	for (;;)	// for easy exits
		{
		TInet6Packet<TInet6HeaderIP4> ip(aPacket);
		TInet6Packet<TInet6HeaderAH> ah(payload);

		if (ip.iHdr == NULL || ah.iHdr == NULL)
			{
			result = EIpsec_CorruptPacketIn;
			break;
			}
		// It is assumed that aInfo.iSrcAddr and aInfo.iDstAddr actually contain the current
		// src and dst addresses of the aPacket in IPv6 format (IPv4 addresses in mapped form)!
		aSa = iManager->Lookup(SADB_SATYPE_AH, ah.iHdr->SPI(), TIpAddress(aInfo.iDstAddr));
		if (!aSa)
			{
			result = EIpsec_AhInboundSA;	// --> No usable SA, drop packet!
			break;
			}
		ahlen = ah.iHdr->DataLength();		// The length of the bare Authentication data (+ padding)
		if (aInfo.iOffset + TInet6HeaderAH::MinHeaderLength() + ahlen > aInfo.iLength)
			{
			result = EIpsec_PacketLength;	// --> Truncated packet (not long enough for AH)
			break;
			}
		if (ahlen != aSa->iAeng->DigestSize())
			{
			result = EIpsec_AhIcvLength;	// --> Bad ICV length 
			break;
			}
		if (!aSa->ReplayCheck(ah.iHdr->Sequence()))
			{
			result = EIpsec_ReplayDuplicate;// --> Duplicate packet
			break;
			}

		// Compute the ICV
		TRAP(result, Pass2ScanL(aSa->iAeng, aPacket, NULL, payload));
		if (result != KErrNone)
			break;
		TInt bytes = aInfo.iLength;
		// ** If ESN: Add high order bits to digest
		if (aSa->iFlags & SABD_SAFLAGS_ESN)
			{
			TUint8 buf[4];
			const TUint32 seq = aSa->iTestSeq.High();
			buf[3] = (TUint8)seq;
			buf[2] = (TUint8)(seq >> 8);
			buf[1] = (TUint8)(seq >> 16);
			buf[0] = (TUint8)(seq >> 24);
			aSa->iAeng->Update(TPtrC8(buf, 4));
			bytes += 4;
			}
		// Access may return a descriptor with length != ahlen!
		TPtr8 digest(((RMBufPacketPeek &)payload).Access(ahlen, TInet6HeaderAH::MinHeaderLength()));
		if (digest.Length() < ahlen)
			{
			result = EIpsec_CorruptPacketIn;
			break;
			}
		digest.SetLength(ahlen);		// Make sure the length is exactly right
		if (aSa->iAeng->Compare(digest) != 0)
			{
			result = EIpsec_AhAuthentication;	// --> Authentication failed
			break;
			}
		// Authenticated, update bytes and sequence number.
		aSa->iCurrent.iBytes += bytes;
		aSa->ReplayUpdate(ah.iHdr->Sequence());
		result = aSa->MarkUsed(*iManager);
		if (result != KErrNone)
			{
			aSa = NULL;
			break;								// -- SA has expired
			}

		// Fixup the next header chain
		// (AH processing does not change the iPrevNextHdr offset value!)
		result = ah.iHdr->NextHeader();
		aPacket.CopyIn(TPtrC8((TUint8 *)&result, 1), aInfo.iPrevNextHdr);

		// Remove AH space and adjust IP header
		//	(need also adjust payload length of the IPv6 headers
		//	 just to be clean for a potential ICMP error)
		ahlen += TInet6HeaderAH::MinHeaderLength();
		payload.TrimStart(ahlen);
		aInfo.iLength -= ahlen;

		// The real packet length has been modified, modify the original outermost
		// payload length in the IPv6 header (only for the case where an ICMP error
		// might be generated from this packet!)
		if (ip.iHdr->Version() == 4)
			{
			ip.iHdr->SetTotalLength(aInfo.iLength);
			// Note: IPv4 header checksum is not recomputed! Should it? -- msa
			}
		else
			{
			// *NOTE* 'ip.iHdr' is mapped as IPv4 header. Here it is assumed (correctly)
			// that IPv6 payload field offset is less than minimum IPv4 header size (which
			// guarantees that it is accessible). -- msa
			((TInet6HeaderIP *)ip.iHdr)->SetPayloadLength(aInfo.iLength - sizeof(TInet6HeaderIP));
			}
		break;	// -- NOT REALLY LOOP, BREAK ALWAYS --
		}
	aPacket.Append(payload);
	ASSERT(aInfo.iLength == aPacket.Length());
	return result;
	}
