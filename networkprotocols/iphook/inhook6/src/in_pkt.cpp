// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// in_pkt.cpp - packet handling routines
// Mainly the RMBufPacketPeek implementation
// + extension header locations
// + method for adding a destination option
//

#include "in_pkt.h"
#include "in_pkt_platform.h"
#include "ext_hdr.h"
#include <nifmbuf.h>

#define OPTIMIZE_PADDING


// RMBufPacketPeek
// ***************

EXPORT_C TUint8 *TInet6PacketBase::Access(RMBufChain &aPacket, TInt aOffset, TInt aSize, TInt aMin)
	/**
	* Accesses a contiguous block of memory within RMBufChain starting
	* at specified offset.
	*
	* Attempts to map aSize amount of contiguous RMBuf space at
	* the specified offset, by rearranging the RMBufs,
	* if necessary.
	*
	* If aSize amount is not available, the returned pointer maps
	* the maximum available space (< aSize).
	*
	* @param	aSize
	*	The size of the requested contiguous mapping (in octets).
	*	The value cannot be larger than KMBufSmallSize. If aSize <= 1,
	*	the method will return availabe contiguous mapping at the
	*	indicated offset without rearranging the RMBuf chain. Values
	*	larger than 1 may require rearrangement to get the requested
	*	area into single RMBuf.
	* @param	aOffset
	*	The offset to the beginning of the area. The value MUST
	*	be in range: 0 <= aOffset <= Length().
	* @param	aMin
	*	The minimum accepted iLength. If iLength is less that aMin,
	*	then returns NULL.
	* @return
	*	A pointer to the data starting at indicated
	*	offset. The iLength is set to the
	*	maximum available contiguous data starting from the
	*	requested offset. iLength is always <= KMBufSmallSize.
	* @note
	*	The length can be either less or
	*	larger than aSize. The caller must verify the returned
	*	length to detect whether Access succeeded.
	*/
	{

	//
	// Can only map at most KMBufSmallSize bytes
	// (truncate aSize to avoid Panics from the Align)
	//
	if (aSize > KMBufSmallSize)
		aSize = KMBufSmallSize;

	for (int already_tried = 0; ;already_tried = 1)
		{
		RMBuf *p;
		TInt offset, len;
		iLength = 0;

		if (aPacket.IsEmpty() || !aPacket.Goto(aOffset, p, offset, len))
			return NULL;
		else if ((aSize <= len && (offset & iAlign) == 0) || already_tried)
			{
			if (len < aMin)
				return NULL;
			iLength = len;
			return p->Buffer() ? (p->Buffer() + offset) : NULL;
			}
		// The requested alignment value is not
		// satisfied!
		//
		// Brute force buffer mangling here: just
		// Split the chain at requested point, apply
		// align to the tail part, and then join the
		// pieces together (assuming this join operation
		// does not undo the align...)
		// (If this is going to occur frequently
		// some optimizations is in order... -- msa)
		//
		//
		// *NOTE*
		//		If aOffset == 0, SplitL works weird (IMHO): it does
		//		nothing (when it logically should move all to the
		//		tail part!). Thus, zero offset needs to be handled
		//		specially (sort of optimizing, so it's not too bad)
		//		-- msa
		//
		if (aOffset > 0)
			{
			RMBufChain tail;
			TInt err = aPacket.Split(aOffset, tail);
			if (err == KErrNone && !tail.IsEmpty())
				{
				tail.Align(aSize);
				aPacket.Append(tail);
				}
			}
		else
			aPacket.Align(aSize);
		// And now just retry the Goto and use that
		// result as is (either we succeeded or not)
		}
	}

EXPORT_C TPtr8 RMBufPacketPeek::Access(TInt aSize, TUint aOffset)
	/**
	* Accesses a contiguous block of memory within RMBufChain starting
	* at specified offset.
	*
	* Attempts to map aSize amount of contiguous RMBuf space at
	* the specified offset into a TPtr8, by rearranging the RMBufs,
	* if necessary.
	*
	* If aSize amount is not available, the returned pointer maps
	* the maximum available space (< aSize).
	*
	* @param	aSize
	*	The size of the requested contiguous mapping (in octets).
	*	The value cannot be larger than KMBufSmallSize. If aSize <= 1,
	*	the method will return availabe contiguous mapping at the
	*	indicated offset without rearranging the RMBuf chain. Values
	*	larger than 1 may require rearrangement to get the requested
	*	area into single RMBuf.
	* @param	aOffset
	*	The offset to the beginning of the area. The value MUST
	*	be in range: 0 <= aOffset <= Length().
	* @return
	*	A pointer descriptor for the data starting at indicated
	*	offset. The length of the descriptor is set to the
	*	maximum available contiguous data starting from the
	*	requested offset. Length is always <= KMBufSmallSize.
	* @note
	*	The length can be either less or
	*	larger than aSize. The caller must verify the returned
	*	length to detect whether Access succeeded.
	*/
	{
	TInet6PacketBase map(TInet6PacketBase::EAlign1);
	TUint8 *const ptr = map.Access(*this, aOffset, aSize, 1);
	return TPtr8(ptr, map.iLength, map.iLength);
	}


EXPORT_C TIpHeader *RMBufPacketPeek::GetIpHeader()
	/**
	* Access IP header (v4 or v6) from the packet at offset 0.
	*
	* @return IP header (or NULL).
	*	A non-NULL return implies that full header, either IPv4 or
	*	IPv6 is accessible through the pointer. This method is
	*	intended to save code space, and not necessarily for
	*	speed...
	*/
	{
	TInet6Packet<TIpHeader> ip(*this);
	if (ip.iHdr)
		{
		if (ip.iHdr->ip4.Version() == 4)
			{
			// Need to do sanity check on header length (because it can be garbage)
			const TInt hlen = ip.iHdr->ip4.HeaderLength();
			if (hlen >= TInet6HeaderIP4::MinHeaderLength() && ip.iLength >= hlen)
				return (TIpHeader *)ip.iHdr;
			}
		else if (ip.iHdr->ip4.Version() == 6)
			{
			if (ip.iLength >= TInet6HeaderIP::MinHeaderLength())
				return (TIpHeader *)ip.iHdr;
			}
		}
	return NULL;
	}

// TPacketPocker
// *************

EXPORT_C TPacketPoker::TPacketPoker(RMBufChain &aChain) : iCurrent(aChain.First()), iOffset(0), iTail(0)
	/**
	* Constructor.
	*
	* @param aChain	The RMBuf chain to be poked.
	*/
	{
	// Find initial value for iTail.
	while (iCurrent)
		{
		iTail = iCurrent->Length();
		if (iTail > 0)
			break;
		// Ugh. There have been RMBuf's with ZERO length!
		iCurrent = iCurrent->Next();
		}
	}

EXPORT_C void TPacketPoker::OverL(TInt aSize)
	/**
	* Skip over bytes.
	*
	* @param aSize	The number of bytes to skip
	* @leave KErrEof, if skipped past end of chain.
	*/
	{
	for (;;)
		{
		if (aSize < iTail)
			{
			iTail -= aSize;		// Results always: iTail > 0!
			iOffset += aSize;
			return;
			}
		aSize -= iTail;
		if ((iCurrent = iCurrent->Next()) == NULL)
			{
			iTail = 0;			// All scanned, at end of chain
			if (aSize > 0)		// --> Error, if request was more
				User::Leave(KErrEof);	// KErrEof used (for lack of any better)
			return;
			}
		iOffset = 0;
		iTail = iCurrent->Length();
		}
	}

EXPORT_C TUint8 *TPacketPoker::AdjustL(TInt aSize)
	/**
	* Arrange contiguous run of bytes.
	*
	* Arragen content of RMBuf chain so that starting from
	* the current offset, a number of bytes is accessible
	* in contiguous memory.
	*
	* @param aSize The requested length,
	* @return The pointer to be beginning of area.
	* @leave KErrEof, if request cannot be satisfied.
	*/
	{
	RMBufChain chain(iCurrent);
	if (iOffset > 0)
		{
		RMBufChain tail;
		chain.SplitL(iOffset, tail);
		tail.Align(aSize);
		chain.Append(tail);
		}
	else
		chain.Align(aSize);
	ASSERT(iCurrent == chain.First());
	iTail = iCurrent->Length() - iOffset;
	if (iTail < aSize)			// Align failed?
		User::Leave(KErrEof);	// KErrEof used (for lack of any better)
	return Ptr();
	}

//

EXPORT_C TBool TPacketPoker::IsExtensionHeader(TInt aProtocolId)
	/**
	* Tests whether a protocol is a known IPv6 extension header using the standard format.
	* 
	* @param aProtocolId	Protocol ID to test.
	* @return ETrue,
	* 	if the protocol header follows the generic IPv6 extension
	*	header format (TInet6HeaderExtension).
	*/
	{
	// If more of these are known, could perhaps implement a
	// static boolean table indexed by protocol -- msa
	return
		(
		aProtocolId == STATIC_CAST(TInt, KProtocolInet6HopOptions) ||
		aProtocolId == STATIC_CAST(TInt, KProtocolInet6DestinationOptions) ||
		aProtocolId == STATIC_CAST(TInt, KProtocolInet6RoutingHeader)
		);
	}

// TPacketHead
// ***********

EXPORT_C TBool TPacketHead::ExtHdrGet(TInt aType, TInt& aOfs, TInt& aLen)
	/**
	* Gets the offset and length of an extension header.
	*
	* @param aType Extension header type
	* @retval aOfs On return, the header offset
	* @retval aLen On return, the header length
	* @return ETrue if the header was found, otherwise EFalse 
	*/
	{
	if (iPacket.IsEmpty())
		return EFalse;

	TInt type = ip6.NextHeader();
	TInt ofs = 0;

	while (TPacketPoker::IsExtensionHeader(type) && ofs<iOffset)
		{
		//TInet6Options is about as generic Extension Header class as possible.
		//Thus no need to create another one.
		TInet6Packet<TInet6Options> hdr(iPacket, ofs);
		if (hdr.iHdr == NULL) return EFalse;
		
		if (type == aType)
			{
			aOfs = ofs;
			aLen = hdr.iHdr->HeaderLength();
			return ETrue;
			}
		type = hdr.iHdr->NextHeader();
		ofs += hdr.iHdr->HeaderLength();
		}
	return EFalse;
	}

EXPORT_C TBool TPacketHead::ExtHdrGetOrPrependL(TInt aType, TInt& aOfs, TInt& aLen)
	/**
	* Gets the offset and length of an extension header, or if it doesn't already 
	* exist, creates the header before all other extension headers.
	*
	* @param aType
	*	Extension header type
	* @retval aOfs
	*	If the header is created, this specifies the header offset. If
	*	the header already exists, on return: the actual header offset.
	* @retval aLen
	*	If the header is created, this specifies the header length. If
	*	the header already exists, on return: the actual header length.
	* @return
	*	ETrue if the header was found, EFalse if the header was created.
	*/
	{
	if (ExtHdrGet(aType, aOfs, aLen))
		return ETrue;
	ASSERT(aLen && ((aLen & 7) == 0));

	if (iPacket.IsEmpty())
		{
		iPacket.AllocL(aLen);
		}
	else
		{
		iPacket.PrependL(aLen);
		}
	
	//Zero out the header
	iPacket.FillZ(aLen);
	TInet6Packet<TInet6Options> hdr(iPacket, 0);
	if (hdr.iHdr == NULL)
		{
		iPacket.TrimStart(aLen);
		User::Leave(KErrGeneral);
		}
	
	hdr.iHdr->SetNextHeader(ip6.NextHeader());
	ip6.SetNextHeader(aType);
	hdr.iHdr->SetHdrExtLen((aLen >> 3) - 1);

	aOfs = 0;
	iOffset += aLen;
	return EFalse;
	}

EXPORT_C TBool TPacketHead::ExtHdrGetOrAppendL(TInt aType, TInt& aOfs, TInt& aLen)
	/**
	* Gets the offset and length of an extension header, or if it doesn't already 
	* exist, creates the header after all other extension headers.
	*
	* @param aType	Extension header type
	* @retval aOfs
	*	If the header is created, this specifies the header offset. If
	*	the header already exists, on return, the actual header offset.
	* @retval aLen
	*	If the header is created, this specifies the header length. If
	*	the header already exists, on return, the actual header length.
	* @return ETrue if the header was found, EFalse if the header was created 
	*/
	{
	if (ExtHdrGet(aType, aOfs, aLen)) return ETrue;
	if (iPacket.IsEmpty()) return ExtHdrGetOrPrependL(aType, aOfs, aLen);
	ASSERT(aLen && ((aLen & 7) == 0));
	
	TInt nxt = ip6.NextHeader();
	TInt ofs = 0;
	FOREVER
		{
		TInet6Packet<TInet6Options> hdr(iPacket, ofs);
		if (hdr.iHdr == NULL)
			User::Leave(KErrGeneral);
		
		nxt = hdr.iHdr->NextHeader();
		ofs += hdr.iHdr->HeaderLength();
		if (ofs >= iOffset)
			{
			hdr.iHdr->SetNextHeader(aType);
			break;
			}
		}
	
	RMBufChain tail;
	tail.AllocL(aLen);
	tail.FillZ();
	iPacket.Append(tail);

	TInet6Packet<TInet6Options> hdr(iPacket, iOffset);
	if (hdr.iHdr == NULL)
		{
		iPacket.TrimEnd(iOffset);
		User::Leave(KErrGeneral);
		}
	hdr.iHdr->SetNextHeader(nxt);
	hdr.iHdr->SetHdrExtLen((aLen >> 3) - 1);
	
	aOfs = iOffset;
	iOffset += aLen;
	return EFalse;
	}

//
// TODO: Possibility to replace existing padding with an option which
//       would preserve the alignment. (eg. Home Address is kind of Pad2)
//
EXPORT_C void TPacketHead::AddDestinationOptionL(const TUint8* aOption, TUint8 aLen, TUint8 aAlign/*=0*/, TUint8 aModulo/*=4*/)
	{
	/**
	* Adds a Destination Option extension header option.
	*
	* If no Destination Option extension header already exists, one is created
	* as the first extension header. The option is aligned according to the
	* specified aAlign and aModulo parameters.
	*
	* @param aOption
	*	Option Data
	* @param aLen
	*	Option Data length
	* @param aAlign
	*	Option alignment requirement. The default is no alignment.
	* @param aModulo
	*	Option must start at an octect position which is a multiple 
	*	of the specified value
	*/
	TPtrC8 ptr(aOption, aLen);
	AddDestinationOptionL(ptr, aAlign, aModulo);
	}

EXPORT_C void TPacketHead::AddDestinationOptionL(const TPtrC8& aOption, TUint8 aAlign/*=0*/, TUint8 aModulo/*=4*/)
	/**
	* Adds a Destination Option extension header option.
	*
	* If no Destination Option extension header already exists, one is
	* created as the first extension header. The option is aligned
	* according to the specified aAlign and aModulo parameters.
	*
	* @param aOption
	*	Pointer to the start of the Option Data
	* @param aAlign
	*	Option alignment requirement. The default is no alignment.
	* @param aModulo
	*	Option must start at an octect position which is a multiple
	*	of the specified value
	*/
	{
	TUint padlen = aAlign?(TUint(aAlign - 2) % aModulo):0;  //Leading pad
	TInt len = 2 + padlen + aOption.Length();
	TInt trailpad = (8 - (len & 7)) & 7;
	len += trailpad;

	TInt ofs;
	if (! ExtHdrGetOrAppendL(KProtocolInet6DestinationOptions, ofs, len))
		{
		//Simple case, new header was allocated and initialized with required length.
		//Just put the option in.
		ofs += 2;
#ifdef OPTIMIZE_PADDING
		if (padlen > 1)
			{
			TInet6Packet<TInet6OptionBase> pad(iPacket, ofs);
			if (pad.iHdr == NULL)
				User::Leave(KErrGeneral);
			pad.iHdr->SetType(KDstOptionPadN);
			pad.iHdr->SetDataLen(padlen - 2);
			}
#endif
		ofs += padlen;
		iPacket.CopyIn(aOption, ofs);
#ifdef OPTIMIZE_PADDING
		ofs += aOption.Length();
		if (trailpad > 1)
			{
			TInet6Packet<TInet6OptionBase> pad(iPacket, ofs);
			if (pad.iHdr == NULL)
				User::Leave(KErrGeneral);
			pad.iHdr->SetType(KDstOptionPadN);
			pad.iHdr->SetDataLen(trailpad - 2);
			}
#endif
		return;
		}

	//Now we have the position and size of the header
	TInet6Packet<TInet6Options> opt(iPacket, ofs);
	if (opt.iHdr == NULL)
		User::Leave(KErrGeneral);

	//padlen = leading pad
	ofs += len;
	padlen = aAlign?(TUint(aAlign - ofs) % aModulo):0;
	TInt size = padlen + aOption.Length();
	trailpad = (8 - (size & 7)) & 7;
	size += trailpad;
	
	RMBufChain pkt;
	pkt.AllocL(size);
	pkt.FillZ();
#ifdef OPTIMIZE_PADDING
	//Optimize leading pad
	if (padlen > 1)
		{
		TInet6Packet<TInet6OptionBase> pad(pkt, 0);
		if (pad.iHdr == NULL)
			User::Leave(KErrGeneral);
		pad.iHdr->SetType(KDstOptionPadN);
		pad.iHdr->SetDataLen(padlen - 2);
		}
	//Optimize trailing pad
	if (trailpad > 1)
		{
		TInet6Packet<TInet6OptionBase> pad(pkt, padlen+aOption.Length());
		if (pad.iHdr == NULL)
			User::Leave(KErrGeneral);
		pad.iHdr->SetType(KDstOptionPadN);
		pad.iHdr->SetDataLen(trailpad - 2);
		}
#endif
	pkt.CopyIn(aOption, padlen);
	
	iPacket.Append(pkt);
	
	iOffset += size;
	opt.iHdr->SetHdrExtLen(opt.iHdr->HdrExtLen() + (size >> 3));
	}
