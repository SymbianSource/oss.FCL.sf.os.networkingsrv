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
// in_pkt.h - packet handling routines
// Generic packet handling utility for mapping packet handling to the RMBufChain.
//



/**
 @file in_pkt.h
 @publishedAll
 @released
*/

#ifndef __IN_PKT_H__
#define __IN_PKT_H__

#include "ip6_hdr.h"	// ..should eventually be <inet/ip6_hdr.h>? -- msa
#include "ip4_hdr.h"
class RMBufChain;


#define TPACKETHEAD_FRAGMENT	1	//< Enable iFragment in TPacketHead

/**
 TScopeType is only provided so that "magic" constants can be
 avoided in the source code. However, the max value cannot be changed
 to anything from 0xF. The scope type is assumed to be 4 bits long
 in many occasions.

 The value of the scope type is directly bound the the IPv6 Scope
 level - 1. This can be done, as IPv6 Scope level 0 is not legal
 (or usable) in any context within the stack.
 This allows our non-standard network scope (= 0x10) to
 be coded internally in 4 bits (as 0xF).

 @publishedAll
 @released
 @since v7.0s
*/
enum TScopeType
	{
	EScopeType_IF	= 0x0,	//< (= #KIp6AddrScopeNodeLocal - 1), id is interface index
	EScopeType_IAP	= 0x1,	//< (= #KIp6AddrScopeLinkLocal - 1). id is IAP number
	EScopeType_GLOBAL = 0xD,//< (= #KIp6AddrScopeGlobal - 1). id is global scope id
	//
	// no symbols defined for types 2..14 (they are also valid)
	//
	EScopeType_NET	= 0xF	//< (= #KIp6AddrScopeNetwork - 1), id is network number (must be the last entry)
	};

//
//	TIpHeader
//	*********
class TIpHeader
	/**
	A simple help class that uses a union to merge handling of either an IPv4 or 
	an IPv6 header. 
	@since v7.0
	@publishedAll
	@released
	*/
	{
public:
	/**
	Gets the minimum header length.

	IPv6 header is longer than minimum IPv4 header, thus
	returned value is for IPv4. This function only defined
	because it is required when this class is used as template
	parameter in TInet6Packet.
	
	@return Minimum IPv4 header length
	*/
	inline static TInt MinHeaderLength() {return TInet6HeaderIP4::MinHeaderLength(); }
	/**
	Gets the maximum header length.

	IPv6 header always shorter than maximum IPv4 header, thus
	returned value is for IPv4. This function is only defined
	because "header mapping" classes are expected to have it.
	
	@return Maximum IPv4 header length
	*/
	inline static TInt MaxHeaderLength() {return TInet6HeaderIP4::MaxHeaderLength(); }

	union
		{
		TInet6HeaderIP4 ip4;
		TInet6HeaderIP ip6;
		};
	};



class TInet6PacketBase
	/**
	* Thin base class for the TInet6Packet.
	*/
	{
public:
	enum TAlign
		{
		EAlign1 = 0,	//< Align to byte (no align requirement)
		EAlign2 = 1,	//< Align to 2 byte unit (even address)
		EAlign4 = 3,	//< Align to 4 byte unit
		EAlign8 = 7		//< Align to 8 byte unit
		};

	/**
	Constructor.

	@param aAlign	The align requirement.
	*/
	TInet6PacketBase(TAlign aAlign) : iLength(0), iAlign(aAlign) {}

	/**
	Length of the mapped region.

	The real mapped length as computed by the Access function.
	If access returned non-NULL, the following is always TRUE:

	@li	aMin <= iLength
	*/
	TInt iLength;

	IMPORT_C TUint8 *Access(RMBufChain &aPacket, TInt aOffset, TInt aSize, TInt aMin);

	inline void SetAlign(TAlign aAlign)
		/**
		* Changes the align requirement.
		*
		* @param aAlign The new align requirement.
		*/
		{
		iAlign = aAlign;
		}
protected:
	/**
	The align requirement.
	*/
	TAlign iAlign;
	};

// TInet6Packet template
// *********************
template <class T>
class TInet6Packet : public TInet6PacketBase
	/**
	Encapsulates an IPv6 packet header as a section of an RMBufChain.

	The T template parameter should represent a packet header type. It should 
	support static functions MaxHeaderLength() and MinHeaderLength() that return 
	TInt values for maximum and minimum header lengths respectively.

	@publishedAll
	@released
	@since v7.0
	*/
	{
public:
	TInet6Packet(TAlign aAlign = EAlign4) : TInet6PacketBase(aAlign), iHdr(NULL)
		/**
		Default constructor.

		Construct an empty mapping. To be usable, the Set() function
		must be used.
		*/
		{}
	TInet6Packet(RMBufChain &aPacket) : TInet6PacketBase(EAlign4)
		/**
		Constructor specifying a RMBufChain object.

		Verify and arrange it so that a class T can be mapped
		to a contiguous octets from the beginning of the RMBufChain
		content, and set iHdr to point this area.

		If this is not possible, iHdr is initialized to NULL.

		@param	aPacket
			Packet containing the header T at offset = 0
		*/
		{
		iHdr = (T *)Access(aPacket, 0, T::MaxHeaderLength(), T::MinHeaderLength());
		}

	TInet6Packet(RMBufChain &aPacket, TInt aOffset, TAlign aAlign = EAlign4) : TInet6PacketBase(aAlign)
		/**
		Constructor specifying a RMBufChain object and an offset.

		Verify and arrange it so that a class T can be mapped
		to a contiguous octets starting at specified offset of
		the RMBufChain content, and set iHdr to point this area.

		If this is not possible, iHdr is initialized to NULL.

		@param aPacket
			Packet containing the header T at aOffset
		@param aOffset
			Offset of the header to be mapped.
		@param aAlign
			The alignement requirement.
		*/
		{
		iHdr = (T *)Access(aPacket, aOffset, T::MaxHeaderLength(), T::MinHeaderLength());
		}

	void Set(RMBufChain &aPacket, TInt aOffset, TInt aSize)
		/**
		Sets the packet header from a specified RMBufChain object.

		Verify and arrange it so that a aSize octets can be mapped
		to a contiguous octets starting at specified offset of
		the RMBufChain content, and set iHdr to point this area.

		If this is not possible, iHdr is initialized to NULL.

		Note that this differs from the contructors: the required
		size is a parameter, and not determined by the T::MinHeaderLength().
		However, the "T* iHdr" is set to point the start of the requested
		area. It's a responsibility of the user of this method to know
		whether using this pointer is safe with the specified size parameter.

		@param	aPacket
			Packet containing the header T at aOffset
		@param	aOffset
			Offset (position) of the header to be mapped
		@param	aSize
			Length of required contiguous memory
		*/
		{
		iHdr = (T *)Access(aPacket, aOffset, aSize, aSize);
		}

	inline T& operator()()
		{
		return *iHdr;
		}
	/**
	The pointer to the mapped region (if non-NULL). If NULL,
	then there is no mapping, and iLength == 0.
	*/
	T *iHdr;
	};


#endif
