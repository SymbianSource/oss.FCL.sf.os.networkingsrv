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
// ext_hdr.h - IPv6 extension headers
// Defines the basic classes for accessing the extension
// header structures within IPv6 packets.
// All return types codified as TInt (Native form and signed)
//



/**
 @file ext_hdr.h
 @ingroup ip_packet_formats
 @publishedAll
 @released
*/

#ifndef __EXT_HDR_H__
#define __EXT_HDR_H__

#include <e32def.h>
#include "in_hdr.h"

/**
* @addtogroup  ip_packet_formats
* @{
*/

/**
* @name Additional protocol numbers
*
* @{
*/
/** Hop-by-Hop Extension Header. See TInet6HeaderHopByHop. */
const TUint KProtocolInet6HopOptions = 0;
/** Tunneled IPv4. The next header is TInet6HeaderIP4. */
const TUint KProtocolInetIpip = 4;
/** Routing Header. See TInet6HeaderRouting. */
const TUint KProtocolInet6RoutingHeader = 43;
/** IPv6 Fragment Header. See TInet6HeaderFragment. */
const TUint KProtocolInet6Fragment = 44;
/** IPsec ESP header. See TInet6HeaderESP. */
const TUint KProtocolInetEsp	= 50;
/** IPsec AH header. See TInet6HeaderAH. */
const TUint KProtocolInetAh = 51;
/** No Next Header. */
const TUint KProtocolInet6NoNextHeader = 59;
/** Destination Options Extension Header. See TInet6Options. */
const TUint KProtocolInet6DestinationOptions = 60;
/* @} */

//
// TInet6HeaderExtension
// *********************
class TInet6HeaderExtension
	/**
	* Basic part of normal IPv6 extension headers.
	*
	* This is simple class to be used in scanning the extension headers
	* which are known to follow the following format:
	*
	* - 1st octet = Next Header field
	*
	* - 2nd octet = Length of the extension header (= (x+1)*8 octets)
	*
	* @note
	*	Extension headers do not need to follow this format. Unknown
	*	headers cannot be skipped over by just assuming this format!
	*
	* @publishedAll
	* @released
	*/
	{
public:
	inline static TInt MinHeaderLength() {return 8; }
	inline static TInt MaxHeaderLength() {return 8; }	

	inline TUint8* EndPtr() { return i + HeaderLength(); }
	inline TInt NextHeader() const { return i[0]; }
	inline TInt HdrExtLen() const { return i[1]; }	// Return raw value
	inline TInt HeaderLength() const { return (i[1]+1) << 3; } // Return true byte length
private:
	union
		{
		TUint8 i[8];
		TUint32 iAlign;	// A dummy member to force the 4 byte alignment
		};
	};


class TInet6HeaderHBH
	/**
	* IPv6 Hop-by-hop options header.
@verbatim
	From RFC 2460

    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |  Next Header  |  Hdr Ext Len  |                               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               +
    |                                                               |
    .                                                               .
    .                            Options                            .
    .                                                               .
    |                                                               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   Next Header          8-bit selector.  Identifies the type of header
                        immediately following the Hop-by-Hop Options
                        header.  Uses the same values as the IPv4
                        Protocol field [RFC-1700 et seq.].

   Hdr Ext Len          8-bit unsigned integer.  Length of the Hop-by-
                        Hop Options header in 8-octet units, not
                        including the first 8 octets.

   Options              Variable-length field, of length such that the
                        complete Hop-by-Hop Options header is an integer
                        multiple of 8 octets long.  Contains one or more
                        TLV-encoded options, as described in section
                        4.2.

  And Options is:

	  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- - - - - - - - -
      |  Option Type  |  Opt Data Len |  Option Data
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- - - - - - - - -

      Option Type          8-bit identifier of the type of option.

      Opt Data Len         8-bit unsigned integer.  Length of the Option
                           Data field of this option, in octets.

      Option Data          Variable-length field.  Option-Type-specific
                           data.
@endverbatim
	* @publishedAll
	* @released
	*/
	{
public:
	inline TInt HeaderLength() const
		{
		return (i[1]+1) << 3;	// Return length in octets.
		}

	inline TUint8* EndPtr() { return i + HeaderLength(); }

	inline static TInt MinHeaderLength() {return 8; }	
	inline static TInt MaxHeaderLength() {return 8; }
	
	//
	// Access, Get Hop By Hop header values from the packet
	//
	inline TInt NextHeader() const
		{
		return i[0];
		}

	//From Options
	inline TInt OptionType() const
		/** @return The type of the first option */
		{
		return i[2];
		}
	inline TInt OptionDataLen() const
		/** @return The data length of the first option */
		{
		return i[3];
		}

	//
	// Access, SET Hop By Hop header values
	//
	inline void SetHdrExtLen(TInt aLength)
		{
		i[1]=(TUint8)aLength;
		}
	
	inline void SetNextHeader(TInt aNext)
		{
		i[0]=(TUint8)aNext;
		}

	//From Options
	inline void SetOptionType(TInt aType)
		/** Sets type of the first option.*/
		{
		i[2]=(TUint8)aType;
		}

	inline void SetOptionDataLen(TInt aLength)
		/** Sets data length of the first option.*/
		{
		i[3]=(TUint8)aLength;
		}

private:
	union
		{
		TUint8 i[8];
		TUint32 iAlign;	// A dummy member to force the 4 byte alignment
		};
	};


class TInet6HeaderHopByHop
	/**
	* IPv6 Hop-by-hop options header.
	* @publishedAll
	* @deprecated
	*	Because of the non-standard method naming and
	*	semantics. Use TInet6HeaderHBH instead.
	*/
	{
public:
	inline TInt HeaderLength() const
		{
		return i[1];
		}

	inline TUint8* EndPtr() { return i + HeaderLength() * 8 + MinHeaderLength(); }

	inline static TInt MinHeaderLength() {return 8; }	
	inline static TInt MaxHeaderLength() {return 8; }
	
	//
	// Access, Get Hop By Hop header values from the packet
	//
	inline TInt NextHeader() const
		/** @return The length in 8-byte units! (non-standard, should be bytes!) */
		{
		return i[0];
		}

	//From Options
	inline TInt OptionType() const
		/** @return The type of the first option */
		{
		return i[2];
		}
	inline TInt OptionDataLen() const
		/** @return The data length of the first option */
		{
		return i[3];
		}

	//
	// Access, SET Hop By Hop header values
	//
	inline void SetHeaderLength(TInt aLength)
		{
		i[1]=(TUint8)aLength;
		}
	
	inline void SetNextHeader(TInt aNext)
		{
		i[0]=(TUint8)aNext;
		}

	//From Options
	inline void SetOptionType(TInt aType)
		/** Sets type of the first option.*/
		{
		i[2]=(TUint8)aType;
		}

	inline void SetOptionDataLen(TInt aLength)
		/** Sets data length of the first option.*/
		{
		i[3]=(TUint8)aLength;
		}

private:
	union
		{
		TUint8 i[8];
		TUint32 iAlign;	// A dummy member to force the 4 byte alignment
		};
	};

class TInet6HeaderRouting
	/**
	* IPv6 Routing Header.
	* The Type 0 Routing header has the following format:
@verbatim

    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |  Next Header  |  Hdr Ext Len  | Routing Type=0| Segments Left |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                            Reserved                           |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                                                               |
    +                                                               +
    |                                                               |
    +                           Address[1]                          +
    |                                                               |
    +                                                               +
    |                                                               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                                                               |
    +                                                               +
    |                                                               |
    +                           Address[2]                          +
    |                                                               |
    +                                                               +
    |                                                               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    .                               .                               .
    .                               .                               .
    .                               .                               .
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                                                               |
    +                                                               +
    |                                                               |
    +                           Address[n]                          +
    |                                                               |
    +                                                               +
    |                                                               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   Next Header          8-bit selector.  Identifies the type of header
                        immediately following the Routing header.  Uses
                        the same values as the IPv4 Protocol field
                        [RFC-1700 et seq.].

   Hdr Ext Len          8-bit unsigned integer.  Length of the Routing
                        header in 8-octet units, not including the first
                        8 octets.  For the Type 0 Routing header, Hdr
                        Ext Len is equal to two times the number of
                        addresses in the header.

   Routing Type         0.


   Segments Left        8-bit unsigned integer.  Number of route
                        segments remaining, i.e., number of explicitly
                        listed intermediate nodes still to be visited
                        before reaching the final destination.

   Reserved             32-bit reserved field.  Initialized to zero for
                        transmission; ignored on reception.

   Address[1..n]        Vector of 128-bit addresses, numbered 1 to n.

@endverbatim
	* This header mapping describes only the fixed part, without
	* any addresses.
	* @publishedAll
	* @released
	*/
	{
public:
	inline static TInt MinHeaderLength() {return 8; }
	inline static TInt MaxHeaderLength() {return 8; }	

	inline TUint8* EndPtr() { return i + HeaderLength(); }

	enum TOffsets
		{
		O_NextHeader,
		O_HdrExtLen,
		O_RoutingType,
		O_SegmentsLeft,
		O_Address = 8
		};

	inline TInt NextHeader() const { return i[0]; }
	inline TInt HdrExtLen() const { return i[1]; }	// Return raw value
	inline TInt HeaderLength() const { return (i[1]+1) << 3; } // Return true byte length
	inline TInt RoutingType() const { return i[2]; }
	inline TInt SegmentsLeft() const { return i[3]; }
	
	//SET
	inline void SetNextHeader(TInt aNext) { i[0] = (TInt8)aNext; }
	inline void SetHdrExtLen(TInt aLen) { i[1] = (TUint8)aLen; }	
	inline void SetRoutingType(TInt aType) { i[2] = (TUint8)aType; }
	inline void SetSegmentsLeft(TInt aValue) { i[3] = (TUint8)aValue; }

private:
	union
		{
		TUint8 i[8];
		TUint32 iAlign;	// A dummy member to force the 4 byte alignment
		};
	};



class TInet6Options
	/**
	* IPv6 Option extension headers (Hop-by-Hop, Destination Options).
	*
@verbatim
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Next Header  |  Hdr Ext Len  |                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               +
   |                                                               |
   .                                                               .
   .                            Options                            .
   .                                                               .
   |                                                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  Next Header          8-bit selector.  Identifies the type of header
                       immediately following the Destination Options
                       header.  Uses the same values as the IPv4
                       Protocol field [RFC-1700 et seq.].

  Hdr Ext Len          8-bit unsigned integer.  Length of the
                       Destination Options header in 8-octet units, not
                       including the first 8 octets.

  Options              Variable-length field, of length such that the
                       complete Destination Options header is an
                       integer multiple of 8 octets long.  Contains one
                       or  more TLV-encoded options, as described in
                       section 4.2.
@endverbatim
	* This mapping describes only the minimal 8 bytes.
	* @publishedAll
	* @released
	*/
	{
public:
	inline TInt HeaderLength() const { return (i[1]+1) << 3; } // Return true byte length

	inline static TInt MinHeaderLength() {return 8; }
	inline static TInt MaxHeaderLength() {return 8; }	

	inline TUint8* EndPtr() { return i + HeaderLength(); }

	enum TOffsets
		{
		O_NextHeader,
		O_HdrExtLen,
		O_Options
		};

	inline TInt NextHeader() const { return i[0]; }
	inline TInt HdrExtLen() const { return i[1]; }	// Return raw value
	
	inline void SetNextHeader(TInt aNext) { i[0] = (TInt8)aNext; }
	inline void SetHdrExtLen(TInt aLen) { i[1] = (TUint8)aLen; }
	
private:
	TUint8 i[8];
	};


/**
* @name Destination option types.
*
* @{
*/
/** One octet padding. */
const TUint8 KDstOptionPad1				= 0;
/** N octets padding */
const TUint8 KDstOptionPadN				= 1;
/** not used? (MIP6) */
const TUint8 KDstOptionBindingAck		= 7;
/** not used? (MIP6) */
const TUint8 KDstOptionBindingRequest	= 8;
/** not used? (MIP6) */
const TUint8 KDstOptionBindingUpdate	= 0xC6;
/** Home Address option (MIP6) */
const TUint8 KDstOptionHomeAddress		= 0xC9;
/** @} */

class TInet6OptionBase
	/**
	* IPv6 Option value header.
	*
	* A basic class for handling Type-Length-Value (TLV) options.
	*
@verbatim
	  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- - - - - - - - -
      |  Option Type  |  Opt Data Len |  Option Data
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- - - - - - - - -

      Option Type          8-bit identifier of the type of option.

      Opt Data Len         8-bit unsigned integer.  Length of the Option
                           Data field of this option, in octets.

      Option Data          Variable-length field.  Option-Type-specific
                           data.
@endverbatim
	* Option values are used inside option headers (for example:
	* Hop-by-Hop options and Destination options).
	*
	* @publishedAll
	* @released
	*/
	{
public:
	inline static TInt MinHeaderLength()	{ return 2; }
	inline static TInt MaxHeaderLength()	{ return 2; }

	inline TInt Type() const				{ return i[0]; }
	inline TInt HeaderLength() const { return i[1] + 2; }
	
	inline void SetType(TInt aType)			{ i[0] = (TUint8)aType; }
	inline void SetDataLen(TInt aLen)		{ i[1] = (TUint8)aLen; }

	inline TUint8* EndPtr() { return i + HeaderLength(); }

private:
	TUint8	i[2];
	};

class TInet6DstOptionBase
	/**
	* IPv6 Option value header.
	* @publishedAll
	* @deprecated
	*	Because of the non-standard method naming and
	*	semantics. Use TInet6OptionBase instead.
	*/
	{
public:
	inline static TInt MinHeaderLength()	{ return 2; }
	inline static TInt MaxHeaderLength()	{ return 2; }

	inline TInt Type() const				{ return i[0]; }
	inline TInt HeaderLength() const { return i[1]; }
	
	inline void SetType(TInt aType)			{ i[0] = (TUint8)aType; }
	inline void SetHeaderLength(TInt aLen)	{ i[1] = (TUint8)aLen; }

	inline TUint8* EndPtr() { return i + 2 + HeaderLength(); }

private:
	TUint8	i[2];
	};


class TInet6HeaderFragment
	/**
	* IPv6 Fragment Header.
@verbatim
RFC2460

   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Next Header  |   Reserved    |      Fragment Offset    |Res|M|
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                         Identification                        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   Next Header          8-bit selector.  Identifies the initial header
                        type of the Fragmentable Part of the original
                        packet (defined below).  Uses the same values as
                        the IPv4 Protocol field [RFC-1700 et seq.].

   Reserved             8-bit reserved field.  Initialized to zero for
                        transmission; ignored on reception.

   Fragment Offset      13-bit unsigned integer.  The offset, in 8-octet
                        units, of the data following this header,
                        relative to the start of the Fragmentable Part
                        of the original packet.

   Res                  2-bit reserved field.  Initialized to zero for
                        transmission; ignored on reception.

   M flag               1 = more fragments; 0 = last fragment.

   Identification       32 bits.  See description below.
@endverbatim
	* @publishedAll
	* @released
	*/
	{
public:
	enum TOffsets
		{
		O_FragmentOffset = 2
		};

	inline TInt HeaderLength() const
		{
		return 8;
		}

	inline TUint8* EndPtr() { return i + HeaderLength(); }

	inline static TInt MinHeaderLength() {return 8; }
	inline static TInt MaxHeaderLength() {return 8; }
	
	//
	// Access, Get Fragmentation header values from the packet
	//
	inline TInt NextHeader() const { return i[0]; }

	inline TInt FragmentOffset() const
		{
		//
		//	The Offset is returned as octet count (3 righmost bits are zero)
		//
		return ((i[2] << 8) + i[3]) & 0xfff8;
		}

	inline TInt MFlag() const
		{
		return i[3] & 0x01;
		}

	inline TInt32 Id() const
		{
		return *(TInt32 *)(&i[4]);	// *ASSUMES* proper aligment!!!
		}
	//
	// Building methods
	//
	inline void ZeroAll()
		{
		*((TInt32 *)&i[0]) = 0;		// *ASSUMES* proper aligment!!!
		*((TInt32 *)&i[4]) = 0;		// *ASSUMES* proper aligment!!!
		}
	inline void SetNextHeader(TInt aNext)
		{
		i[0] = (TUint8)aNext;
		}
	inline void SetFragmentOffset(TInt aOffset)
		{
		//
		// The aOffset is assumed to be given in octets. The least significant
		// 3 bits should be ZERO (bits are just masked away).
		//
		i[2]=(TUint8)(aOffset >> 8);
		i[3]=(TUint8)((i[3] & 0x7) | (aOffset & ~0x7));
		}

	inline void SetMFlag(TInt aFlag)
		{
		i[3]= (TUint8)((i[3] & ~0x1) | (aFlag & 0x1));
		}

	inline void SetId(TInt32 aId)
		{
		*((TInt32 *)&i[4]) = aId;	// *ASSUMES* proper aligment!!!
		}
private:
	union
		{
		TUint8 i[8];
		TUint32 iAlign;	// A dummy member to force the 4 byte alignment
		};
	};

class TInet6HeaderAH
	/**
	* IPsec Authentication Header.
	*
	* The function parameters and return values are in host order,
	* except SPI, which is always in network byte order.
@verbatim
Extract from RFC-2402

    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   | Next Header   |  Payload Len  |          RESERVED             |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                 Security Parameters Index (SPI)               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    Sequence Number Field                      |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                                                               |
   +                Authentication Data (variable)                 |
   |                                                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
@endverbatim
	* @publishedAll
	* @released
	*/
	{
public:
	//
	// Basic
	//
	inline static TInt MinHeaderLength() {return 3*4; }
	inline static TInt MaxHeaderLength() {return 3*4; }
	inline TUint8 *EndPtr() {return i + HeaderLength();}
	//
	// Access, get values
	//
	inline TInt NextHeader() const
		{
		return i[0];
		}
	//
	// PayloadLength returns the raw value
	//
	inline TInt PayloadLength() const
		{
		return i[1];
		}
	//
	// *NOTE* AH is called IPv6 extension header, but its
	// length field semantics does not follow the normal
	// IPv6 extension header logic (it follows the IPv4)
	//
	inline TInt HeaderLength() const
		{
		return (i[1]+2) << 2;	// IPv4 and IPv6
		}
	//
	// SPI is returned in network byte order
	//
	inline TUint32 SPI() const
		{
		return *((TUint32 *)(i + 4));
		}
	inline TUint32 Sequence() const
		{
		return (i[8] << 24) | (i[9] << 16) | (i[10] << 8) | i[11];
		}

	// The length of the Authentication Data (in octets).
	// *NOTE* This will include the potential padding! -- msa
	inline TInt DataLength() const		{
		return HeaderLength() - 12;
		}
	inline TPtr8 ICV()
		{
		return TPtr8((TUint8 *)&i[12], DataLength(), DataLength());
		}
	//
	// Build
	//
	inline void SetNextHeader(TInt aNext)
		{
		i[0] = (TUint8)aNext;
		}
	inline void SetPayloadLength(TInt aByte)
		{
		i[1] = (TUint8)aByte;
		}
	//
	// *NOTE* AH is called IPv6 extension header, but its
	// length field semantics does not follow the normal
	// IPv6 extension header logic (it follows the IPv4)
	// As this is bit tricky, a "cooked version" of PayloadLength
	// setting is also provided (e.g. take in bytes, and compute
	// the real payload length value) -- msa
	inline void SetHeaderLength(TInt aLength)
		{
		i[1] = (TUint8)((aLength >> 2) - 2);
		}
	inline void SetSPI(TUint32 aSPI)
		{
		*((TUint32 *)(i + 4)) = aSPI;
		}
	inline void SetReserved(TInt aValue)
		{
		i[3] = (TUint8)aValue;
		i[2] = (TUint8)(aValue >> 8);
		}
	inline void SetSequence(TUint32 aSeq)
		{
		i[11] = (TUint8)aSeq;
		i[10] = (TUint8)(aSeq >> 8);
		i[9] = (TUint8)(aSeq >> 16);
		i[8] = (TUint8)(aSeq >> 24);
		}
protected:
	union
		{
		TUint8 i[3*4];
		TUint32 iAlign;	// A dummy member to force the 4 byte alignment
		};
	};



class TInet6HeaderESP
	/**
	* IPsec Encapsulating Security Payload Packet Format.
	*
	* The function parameters and return values are in host
	* order (except SPI, which is always in network byte order)
	*
@verbatim
RFC-2406
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ ----
|               Security Parameters Index (SPI)                 | ^Auth.
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ |Cov-
|                      Sequence Number                          | |erage
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ | ----
|                    Payload Data* (variable)                   | |   ^
~                                                               ~ |   |
|                                                               | |Conf.
+               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ |Cov-
|               |     Padding (0-255 bytes)                     | |erage*
+-+-+-+-+-+-+-+-+               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ |   |
|                               |  Pad Length   | Next Header   | v   v
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ ------
|                 Authentication Data (variable)                |
~                                                               ~
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
@endverbatim
	* This only defines the fixed portion of the ESP, 8 bytes).
	* @publishedAll
	* @released
	*/
	{
public:
	//
	// Basic
	//
	inline static TInt MinHeaderLength() {return 2*4; }
	inline static TInt MaxHeaderLength() {return 2*4; }
	inline TInt HeaderLength() const {return 2*4; }
	inline TUint8 *EndPtr() {return i + HeaderLength();}

	//
	// Access, get values
	//
	//
	// SPI is returned in network byte order
	//
	inline TUint32 SPI() const
		{
		return *((TUint32 *)(i + 0));
		}
	inline TUint32 Sequence() const
		{
		return (i[4] << 24) | (i[5] << 16) | (i[6] << 8) | i[7];
		}
	//
	// IV is not exactly part of the header, but provide
	// a method that returns a Ptr to it (assuming the
	// IV is accessible directly after the fixed part).
	//
	inline TPtr8 IV(TInt aLen)
		{
		return TPtr8((TUint8 *)&i[sizeof(i)], aLen, aLen);
		}

	//
	// Build
	//
	inline void SetSPI(TUint32 aSPI)
		{
		*((TUint32 *)(i + 0)) = aSPI;
		}
	inline void SetSequence(TUint32 aSeq)
		{
		i[7] = (TUint8)aSeq;
		i[6] = (TUint8)(aSeq >> 8);
		i[5] = (TUint8)(aSeq >> 16);
		i[4] = (TUint8)(aSeq >> 24);
		}
protected:
	union
		{
		TUint8 i[2*4];
		TUint32 iAlign;	// A dummy member to force the 4 byte alignment
		};
	};

/** @} */
#endif
