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
// ip6_hdr.h - IPv6 header structure
// This module defines the basic classes for accessing the header
// structures within IPv6 packets.
// Defines the IPv6 header structure.
//



/**
 @file ip6_hdr.h
 @ingroup ip_packet_formats
 @publishedAll
 @released
*/

#ifndef __IP6_HDR_H__
#define __IP6_HDR_H__

#include "in_hdr.h"
#include <in_sock.h> // Only for TIp6Addr !

/**
* @defgroup  ip_packet_formats	IPv4 and IPv6 packet formats and constants
*
* These headers define various constants and special <em>mapping or overlay
* classes</em> to be used in accessing and building the IP packets. The
* mapping classes are not intended to be used in declaring
* variables (although some of them do support that use also).
*
* The normal usage is to typecast a binary buffer
* to a mapping class. For example
* assuming a buf contains a raw IPv6 packet:
*
@code
	TBuf8<2000> buf;

	TInet6HeaderIP &ip = *(TInet6HeaderIP *)buf.Ptr();
	if (ip.MinHeaderLength() > buf.Length() ||
		ip.HeaderLength() + ip.PayloadLength() > buf.Length())
		{
		// Oops. packet too short!
		}
	else if (ip.Version() == 6)
		{
		// Packet seems to be IPv6 packet.
		}
@endcode
*
* Within the TCP/IP stack, the packets are stored in RMBufChain objects.
* The data is not in a contiguous memory area and accessing the headers is
* more complicated. For that purpose there is a template class TInet6Packet,
* which can use any <em>mapping class</em> as a template parameter and
* provides functions to access the header within the chain. Assuming the
* a RMBufChain object buf contains the packet, equivalent code would be:
*
@code
	RMBufChain buf;

	TInet6Packet<TInet6HeaderIP> ip(buf);
	if (ip.iHdr == NULL ||
		ip.iHdr->HeaderLength() + ip.iHdr->PayloadLength() > buf.Length())
		{
		// Opps. packet too short!
		}
	else if (ip.iHdr->Version() == 6)
		{
		// Packet seems to be IPv6 packet.
		}
@endcode
* The TInet6Packet template does automatic MinHeaderLength check. The iHdr
* member variable is set only if the mapping succeeds at least for MinHeaderLength
* octets.
*
* All mapping headers must have the following functions defined:
*
* - static function MinHeaderLength returns the minimum header length in octets.
*
* - static function MaxHeaderLength returns the maximum header length in octets (not used much).
*
* - HeaderLength returns the actual length of a header instance in octets.
*	Note that this may return garbage value, if the mapped header is invalid.
*	The return value can be larger than indicated by MinHeaderLength, and
*	as the mapping is only guaranteed up to that size, user must separately
*	verify the accessibility of a full header in such case. 
* @{
* @}
*/

/**
* @addtogroup  ip_packet_formats
* @{
*/

/**
* The IPv6 minimum value for Maximum Transmission Unit (MTU) as defined in RFC 2460. 
* 
* @since v7.0
*/
const TInt KInet6MinMtu = 1280;

//
// TInet6HeaderIP
// **************
//	Methods of manipulating IPv6 IP header.
//
//	This implementation assumes TUint8 is exactly 8 bits (and not
//	9 or more)
class TInet6HeaderIP
/**
* Encapsulates an IPv6 IP header. 
* @verbatim
 *************************
 Extract from the RFC-2460
 *************************

   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |Version| Traffic Class |           Flow Label                  |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |         Payload Length        |  Next Header  |   Hop Limit   |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                                                               |
   +                                                               +
   |                                                               |
   +                         Source Address                        +
   |                                                               |
   +                                                               +
   |                                                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                                                               |
   +                                                               +
   |                                                               |
   +                      Destination Address                      +
   |                                                               |
   +                                                               +
   |                                                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   Version              4-bit Internet Protocol version number = 6.

   Traffic Class        8-bit traffic class field.  See section 7.

   Flow Label           20-bit flow label.  See section 6.

   Payload Length       16-bit unsigned integer.  Length of the IPv6
                        payload, i.e., the rest of the packet following
                        this IPv6 header, in octets.  (Note that any
                        extension headers [section 4] present are
                        considered part of the payload, i.e., included
                        in the length count.)

   Next Header          8-bit selector.  Identifies the type of header
                        immediately following the IPv6 header.  Uses the
                        same values as the IPv4 Protocol field [RFC-1700
                        et seq.].

   Hop Limit            8-bit unsigned integer.  Decremented by 1 by
                        each node that forwards the packet. The packet
                        is discarded if Hop Limit is decremented to
                        zero.

   Source Address       128-bit address of the originator of the packet.

   Destination Address  128-bit address of the intended recipient of the
                        packet (possibly not the ultimate recipient, if
                        a Routing header is present)
@endverbatim
* @publishedAll
* @released
* @since v7.0
*/
	{
public:
	enum TOffsets
		{
		O_PayloadLength = 4,
		O_NextHeader = 6,
		O_HopLimit = 7,
		O_SrcAddr = 8,
		O_DstAddr = 24
		};

	// Basic functions, common to all header classes
	// *********************************************

	/**
	* Gets the header length.	
	* 
	* Note that the header length is fixed.
	* 
	* @return	Header length.
	*/
	inline TInt HeaderLength() const {return 40;}
	/**
	* Gets the minimum header length.
	* 
	* Note that the header length is fixed.
	* 
	* @return	Minimum header length
	*/
	inline static TInt MinHeaderLength() {return 40; }
	/**
	* Gets the maximum header length.
	* 
	* Note that the header length is fixed.
	* 
	* @return	Maximum header length
	*/
	inline static TInt MaxHeaderLength() {return 40; }
	/**
	* Gets a pointer to the byte following the header.
	* 
	* @return	Pointer to the byte following the header
	*/
	inline TUint8 *EndPtr() {return i + HeaderLength();}

	// IPv6 specific methods, get IP header field values from the packet
	// *****************************************************************

	inline TInt Version() const
		/**
		* Gets the IP version from the header.
		*
		* @return	IP version
		*/
		{
		return (i[0] >> 4) & 0xf;
		}
	inline TInt TrafficClass() const
		/**
		* Gets the traffic class from the header.
		*
		* @return	Traffic class
		*/
		{
		return ((i[0] << 4) & 0xf0) | ((i[1] >> 4) & 0x0f);
		}
	inline TInt FlowLabel() const
		/**
		* Gets the flow label from the header.
		* 
		* @return	Flow label (host byte order)
		*/
		{
		return ((i[1] << 16) | (i[2] << 8) | i[3]) & 0xfffff;
		// Could also use any deterministic permutation of the 20 bits,
		// if Flow label can be treated as opaque 20 bits, and not as
		// integer where host/net byte order comes into play --msa
		}
	inline TInt PayloadLength() const
		/**
		* Gets the payload length from the header.
		*
		* @return	Payload length
		*/
		{
		return (i[4] << 8) | i[5];
		}
	inline TInt NextHeader() const
		/**
		* Gets the next header selector from the header.
		*
		* @return	Next header selector (0..255)
		*/
		{
		return i[6];
		}
	inline TInt HopLimit() const
		{
		/**
		* Gets the hop limit from the header.
		*
		* @return	Hop limit (0..255)
		*/
		return i[7];
		}
	//
	// The following return a modifiable reference, so
	// they can be used both for access and build.
	//
	inline TIp6Addr &SrcAddr() const
		/**
		* Gets the source address from the header.
		*
		* @return	Source address
		*/
		{
		return (TIp6Addr &)i[8];
		}
	inline TIp6Addr &DstAddr() const
		/**
		* Gets the destination address from the header.
		*
		* @return	Destination address
		*/
		{
		return (TIp6Addr &)i[24];
		}

	inline TBool EcnIsCongestion()
		/**
		* Gets ECN congestion status.
		*
		* see RFC-3168 for details.
		*
		* @return	True, if CE bit is set on an ECN capable packet.
		*/
		{
		return ((TrafficClass() & 3) == 3);
		}


	// IPv6 specific methods, set IP header field values into the packet
	// *****************************************************************

	inline void Init()
		/**
		* Initialises the header to basic initial values.
		*
		* @li Version = 6
		* @li Traffic Class = 0
		* @li Flow Label = 0
		* @li Payload Length = 0
		* @li Next Header = 0
		* @li Hop Limit = 0
		*
		* The Source and Destination address fields are not touched.
		*/
		{
		static const union { TUint8 a[4]; TUint32 b;} x = { {6 << 4, 0, 0, 0} };
		(TUint32 &)i[0] = x.b;
		(TUint32 &)i[4] = 0;
		}


	inline void SetVersion(TInt aVersion)
		/**
		* Sets the IP version in the header.
		*
		* @param aVersion	IP version (0..15, = 6 for IPv6)
		*/
		{
		i[0] = (TUint8)((i[0] & 0x0f) | ((aVersion << 4) & 0xf0));
		}
	inline void SetTrafficClass(TInt aClass)
		/**
		* Sets the traffic class in the header.
		* 
		* @param aClass	Traffic class (0..255)
		*/
		{
		i[0] = (TUint8)((i[0] & 0xf0) | (aClass >> 4) & 0x0f);
		i[1] = (TUint8)((i[1] & 0x0f) | (aClass << 4) & 0xf0);
		}
	inline void SetFlowLabel(TInt aFlow)
		/**
		* Sets the flow label in the header.
		* 
		* @param aFlow	Flow label (20 bit integer in host order)
		*/
		{
		i[1] = (TUint8)((i[1] & 0xf0) | ((aFlow >> 16) & 0x0f));
		i[2] = (TUint8)(aFlow >> 8);
		i[3] = (TUint8)aFlow;
		}

	inline void SetPayloadLength(TInt aLength)
		/**
		* Sets the payload length in the header.
		* 
		* @param aLength	Payload length
		*/
		{
		i[4] = (TUint8)(aLength >> 8);
		i[5] = (TUint8)aLength;
		}

	inline void SetNextHeader(TInt aNextHeader)
		/**
		* Sets the next header selector from the header.
		*
		* @param aNextHeader	Next header selector (0..255)
		*/
		{
		i[6] = (TUint8)aNextHeader;
		}
	inline void SetHopLimit(TInt aLimit)
		/**
		* Sets the hop limit in the header.
		*
		* @param aLimit	Hop limit (0..255)
		*/
		{
		i[7] = (TUint8)aLimit;
		}
	inline void SetSrcAddr(const TIp6Addr &anAddr)
		/**
		* Sets the source address in the header.
		* 
		* @param anAddr	Source address
		*/
		{
		(TIp6Addr &)i[8] = anAddr;
		}
	inline void SetDstAddr(const TIp6Addr &anAddr)
		/**
		* Sets the destination address in the header.
		* 
		* @param anAddr	Destination address
		*/
		{
		(TIp6Addr &)i[24] = anAddr;
		}
							
private:
	union
		{
		TUint8 i[40];
		TUint32 iAlign;	// A dummy member to force the 4 byte alignment
		};
	};

/** @} */
#endif
