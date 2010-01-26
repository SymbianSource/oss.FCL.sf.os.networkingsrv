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
// icmp6_hdr.h - ICMPv6 header structure
// This module defines the basic classes for accessing the header
// structures within ICMPv6 packets.
//



/**
 @file icmp6_hdr.h
 @ingroup ip_packet_formats
 @publishedAll
 @released
*/

#ifndef __ICMP6_HDR_H__
#define __ICMP6_HDR_H__

#include <e32def.h>
#include "in_hdr.h"
#include <in_sock.h> // IPv6 enhanced in_sock.h

#ifdef SYMBIAN_TCPIPDHCP_UPDATE
//RFC 5006 definitions
#define RDNSSADDRSIZE 16
#define RDNSSOPTION_HDRLENGTH 8
#define RDNSS_MAX_ADDRESS 4 //4 DNS address shall be processed from RDNSS Option available in RA
#endif //SYMBIAN_TCPIPDHCP_UPDATE

/**
* @addtogroup ip_packet_formats
*/
//@{

// TInet6HeaderICMP
class TInet6HeaderICMP
/**
* ICMPv6 header common part layout.
*
* The basic ICMP header format only covers the common part (4 bytes)
* and 4 bytes of the Message Body (can be accesses as "Parameter")
@verbatim
Extract from RFC-2462: General format of ICMP message

 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|     Type      |     Code      |          Checksum             |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
+                         Message Body                          +
|                                                               |
@endverbatim
* @publishedAll
* @released
*/
	{
public:
	inline TInt HeaderLength() const
		/**
		* Gets the header length.	
		* 
		* @note
		*	This length is not the true length of the
		*	ICMP header. This only covers the fixed part.
		* 
		* @return	Header length.
		*/
		{return 4;}
	inline static TInt MinHeaderLength()
		/**
		* Gets the minimum header length.
		* 
		* @return	Minimum header length
		*/
		{return 4; }
	inline static TInt MaxHeaderLength()
		/**
		* Gets the maximum header length.
		* 
		* @note
		*	This length is not the true length of the
		*	ICMP header. This only covers the fixed part.
		* 
		* @return	Maximum header length
		*/
		{return 4; }
	inline TUint8 *EndPtr() const
		/**
		* Gets a pointer to the byte following the header.
		* 
		* @return
		*	Pointer to the byte following the minimum
		*	fixed header
		*/
		{return (TUint8 *)i + HeaderLength();}
	//
	// Access, get ICMP header field values from the packet
	//
	inline TUint8 Type() const
		/**
		* Gets the ICMPv6 type from the header.
		* @return ICMPv6 type [0..255]
		*/
		{
		return i[0];
		}
	inline TUint8 Code() const
		/**
		* Gets the ICMPv6 code from the header.
		* @return ICMPv6 code [0..255]
		*/
		{
		return i[1];
		}
	inline TInt Checksum() const
		/**
		* Gets the Checksum from the header.
		* @return Header Checksum (TUint16 in NETWORK byte order)
		*/
		{
		// Checksum is used in network byte order
		return *((TUint16 *)&i[2]);
		}
	inline TUint32 Parameter() const
		/**
		* Gets the ICMPv6 Parameter.
		*
		* Accesses the first 4 bytes of ICMP message body, and assumes
		* they form a 32 bit integer in network byte order. Returns
		* this integer in host order.
		*
		* @return ICMPv6 Parameter (as an integer)
		*/
		{
		return (i[4] << 24) | (i[5] << 16) | (i[6] << 8) | i[7];
		}
	//
	// Build, set IP header field values into the packet
	//
	inline void SetType(TUint8 aType)
		/**
		* Sets the ICMPv6 type.
		* @param aType ICMPv6 type [0..255]
		*/
		{
		i[0] = aType;
		}
	inline void SetCode(TUint8 aCode)
		/**
		* Sets the ICMPv6 code.
		* @param aCode ICMPv6 code [0..255]
		*/
		{
		i[1] = aCode;
		}
	inline void SetChecksum(TInt aSum)
		/**
		* Sets the Checksum.
		*
		* @param aSum
		*	The Checksum [0..65535] (16 least significant bits
		*	stored as is (assumed to be in NETWORK byte order).					
		*/
		{
		*((TUint16 *)&i[2]) = (TUint16)aSum;
		}
	inline void SetParameter(TUint32 aValue)
		/**
		* Sets the ICMPv6 Parameter.
		*
		* The value is converted into network byte order and
		* stored as the first 4 bytes of the ICMP message body.
		*
		* @param aValue
		*	The parameter.
		*/
		{
		i[7] = (TUint8)aValue;
		i[6] = (TUint8)(aValue >> 8);
		i[5] = (TUint8)(aValue >> 16);
		i[4] = (TUint8)(aValue >> 24);
		}

protected:
	union
		{
		TUint8 i[8];
		TUint32 iAlign;	// A dummy member to force the 4 byte alignment
		};
	};


//
// TInet6HeaderICMP_Echo
//
class TInet6HeaderICMP_Echo : public TInet6HeaderICMP
/**
* ICMPv6 Echo Request and Echo Reply layout.
*
* Describes the ICMP Echo Request and Replay layout. The space for
* Identifier and Sequence is already covered by the base class.
*
@verbatim
RFC-2463:

 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|     Type      |     Code      |          Checksum             |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|           Identifier          |        Sequence Number        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|     Data ...
+-+-+-+-+-
@endverbatim
@publishedAll
@released
*/
	{
public:
	//
	// General
	//
	inline TInt HeaderLength() const
		/**
		* Gets the header length.
		* @return The length
		*/
		{return 8;}
	inline static TInt MinHeaderLength()
		/**
		* Gets the minimum header length.	
		* @return The length
		*/
		{return 8; }
	inline static TInt MaxHeaderLength()
		/**
		* Gets the maximum header length.	
		* @return The length
		*/
		{return 8; }

	//
	// Access, get ICMP header field values from the packet
	//
	inline TInt Identifier() const
		/**
		* Gets the Idenfifier
		* @return The Identifier
		*/
		{
		return (i[4] << 8) + i[5];
		}
	inline TInt Sequence() const
		/**
		* Gets the Sequence Number
		* @return The number
		*/
		{
		return (i[6] << 8) + i[7];
		}
	//
	// Build, set IP header field values into the packet
	//
	inline void SetIdentifier(TUint16 aIdentifier)
		/**
		* Sets the Idenfifier
		* @param aIdentifier The Identifier
		*/
		{
		i[4] = (TUint8)(aIdentifier >> 8);
		i[5] = (TUint8)aIdentifier;
		}
	inline void SetSequence(TUint16 aSequence)
		/**
		* Sets the Sequence Number
		* @param aSequence The number
		*/
		{
		i[6] = (TUint8)(aSequence >> 8);
		i[7] = (TUint8)aSequence;
		}
private:
	};


class TInet6HeaderICMP_RouterSol: public TInet6HeaderICMP
/**
* ICMPv6 Router Solicitation layout.
*
@verbatim
Router Solicitation Message Format (from RFC-2461)

 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|     Type      |     Code      |          Checksum             |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                            Reserved                           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|   Options ...
+-+-+-+-+-+-+-+-+-+-+-+-
@endverbatim
* Aside from the fields provided by the base class, there is nothing
* else here.
*
* Valid options:
*
* - #KInet6OptionICMP_SourceLink
*
* @publishedAll
* @released
*/
	{
public:
	inline static TInt MinHeaderLength() {return 8; }
	inline static TInt MaxHeaderLength() {return 8; }
	inline TInt HeaderLength() const {return 8;}
	};

// Router Advertisement Message Format from RFC-2461
class TInet6HeaderICMP_RouterAdv : public TInet6HeaderICMP
/**
* ICMPv6 Router Advertisement layout.
*
* (Neighbour Discovery for IP version 6)
* (+ Home Agent flag from draft-ietf-mobileip-ipv6-08)
@verbatim
Type=134, Code=0

 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|     Type      |     Code      |          Checksum             |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
| Cur Hop Limit |M|O|H|Prf|Rsrvd|       Router Lifetime         |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                         Reachable Time                        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                          Retrans Timer                        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|   Options ...
+-+-+-+-+-+-+-+-+-+-+-+-
@endverbatim
*
* @note
*	Above is longer thant what is declared in the base class
*	i-member. The user must verify the sufficient length of
*	the packet, when using this.
*
* Valid options:
*
* - #KInet6OptionICMP_SourceLink
* - #KInet6OptionICMP_Mtu
* - #KInet6OptionICMP_Prefix
* - #KInet6OptionICMP_RouteInformation (draft)
*
* @publishedAll
* @released
*/
	{
public:
	//
	// General
	//
	inline static TInt MinHeaderLength() {return 16; }
	inline static TInt MaxHeaderLength() {return 16; }
	inline TInt HeaderLength() const {return 16;}
	//
	// Access, get ICMP header field values from the packet
	//
	inline TInt CurHopLimit() const
		/**
		* Gets Cur Hop Limit.
		* @return Hop Limit
		*/
		{
		return i[4];
		}
	inline TInt Flags() const
		/**
		* Gets flags byte
		* @return Flags (M, O, H, Prf and Rsrvd)
		*/
		{
		return i[5];		// M + O + Reserved as one unit
		}
	inline TInt M() const
		/** Gets Managed Address Configuration (M) flag */
		{
		return i[5] & 0x80;
		}
	inline TInt O() const
		/** Gets Other Address Configuartion (O) flag */
		{
		return i[5] & 0x40;
		}
	inline TInt H() const
		/** Gets Home Agent Configuration (H) flag */
		{
		return i[5] & 0x20;
		}
#if 1
	inline TInt Prf() const
		/**
		* Gets default route preference.
		*
		* Experimental: draft-draves-ipngwg-router-selection-01.txt
		* Default Router Preferences and More-Specific Routes
		*/
		{
		return (i[5] >> 3) & 0x3;	// should be treated as 2bit signed int
		}
#endif
	inline TInt RouterLifetime() const
		/**
		* Gets the lifetime of the defaul route.
		*
		* If non-zero, specifies how long (in seconds) this
		* router is willing to act as a default router.
		*
		* @return The life time of the default route.
		*
		* @note
		*	This is badly named. The parameter controls
		*	only the default route processing. The value
		*	ZERO does not mean that the sender is not a
		*	router.
		*/
		{
		return (i[6] << 8) + i[7];
		}
	inline TUint32 ReachableTime() const
		/**
		* Gets the value of reachable timer.
		*/
		{
		// coverity[overrun-local]
		return (i[8] << 24) | (i[9] << 16) | (i[10] << 8) | i[11];
		}
	inline TUint32 RetransTimer() const
		/**
		* Gets the value of retransmit timer.
		*/
		{
		// coverity[overrun-local]
		return (i[12] << 24) | (i[13] << 16) | (i[14] << 8) | i[15];
		}
	//
	// Build, set IP header field values into the packet
	//
	inline void SetCurHopLimit(TInt aLimit)
		/**
		* Sets the Cur Hoplimit.
		* @param	aLimit	The Hoplimit [0..255]
		*/
		{
		i[4] = (TUint8)aLimit;
		}
	inline void SetFlags(TInt aFlags)
		/**
		* Sets the flags.
		* @param	aFlags	The flags bits [0..255].
		*/
		{
		i[5] = (TUint8)aFlags;
		}
	inline void SetRouterLifetime(TInt aTime)
		/**
		* Sets the lifetime of the default route.
		* @param aTime The lifetime.
		*/
		{
		i[7] = (TUint8)aTime;
		i[6] = (TUint8)(aTime >> 8);
		}
	inline void SetReachableTime(TUint32 aTime)
		/**
		* Sets the value of reachable timer
		* @param aTime The timer value
		*/
		{
		// coverity[overrun-local]
		i[11] = (TUint8)aTime;
		// coverity[overrun-local]
		i[10] = (TUint8)(aTime >> 8);
		// coverity[overrun-local]
		i[9] = (TUint8)(aTime >> 16);
		// coverity[overrun-local]
		i[8] = (TUint8)(aTime >> 24);
		}
	inline void SetRetransTimer(TUint32 aTimer)
		/**
		* Sets the value of the retransmit timer
		* @param aTimer The timer value
		*/
		{
		// coverity[overrun-local]
		i[15] = (TUint8)aTimer;
		// coverity[overrun-local]
		i[14] = (TUint8)(aTimer >> 8);
		// coverity[overrun-local]
		i[13] = (TUint8)(aTimer >> 16);
		// coverity[overrun-local]
		i[12] = (TUint8)(aTimer >> 24);
		}

private:
	};

class TInet6HeaderICMP_NeighborSol : public TInet6HeaderICMP
/**
* ICMPv6 Neighbour Solicitation layout.
@verbatim
Neigbour Solicitation Message Format from RFC-2461

 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|     Type      |     Code      |          Checksum             |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                           Reserved                            |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
+                                                               +
|                                                               |
+                       Target Address                          +
|                                                               |
+                                                               +
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|   Options ...
+-+-+-+-+-+-+-+-+-+-+-+-
@endverbatim
*
* @note
*	Above is longer thant what is declared in the base class
*	i-member. The user must verify the sufficient length of
*	the packet, when using this.
*
* Valid options:
*
* - #KInet6OptionICMP_SourceLink
*
* @publishedAll
* @released
*/
	{
public:
	//
	// General
	//
	inline static TInt MinHeaderLength() {return 24; }
	inline static TInt MaxHeaderLength() {return 24; }
	inline TInt HeaderLength() const {return 24;}
	inline TIp6Addr &Target() const
		/**
		* Gets the Target Address.
		*
		* @return The target address (reference).
		*/
		{
		return (TIp6Addr &)i[8];
		}
private:
	};


class TInet6HeaderICMP_NeighborAdv : public TInet6HeaderICMP
/**
* ICMPv6 Neighbour Advertisement layout.
@verbatim
Neighbor Advertisement Message Format (from RFC-2461)

 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|     Type      |     Code      |          Checksum             |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|R|S|O|                     Reserved                            |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
+                                                               +
|                                                               |
+                       Target Address                          +
|                                                               |
+                                                               +
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|   Options ...
+-+-+-+-+-+-+-+-+-+-+-+-
@endverbatim
* @note
*	Above is longer thant what is declared in the base class
*	i-member. The user must verify the sufficient length of
*	the packet, when using this.
*
* Valid options:
*
* - #KInet6OptionICMP_TargetLink
*
* @publishedAll
* @released
*/
	{
public:
	//
	// General
	//
	inline static TInt MinHeaderLength() {return 24; }
	inline static TInt MaxHeaderLength() {return 24; }
	inline TInt HeaderLength() const {return 24;}

	//
	// Set and Access the Target Address
	//
	inline TIp6Addr &Target() const
		/**
		* Gets the Target Address.
		*
		* @return The target address (reference).
		*/
		{
		return (TIp6Addr &)i[8];
		}

	inline TInt R()
		{
		return 0x80 & i[4];
		}
	inline TInt S()
		{
		return 0x40 & i[4];
		}
	inline TInt O()
		{
		return 0x20 & i[4];
		}
	inline void SetR(TInt aValue)
		{
		if (aValue) i[4] |= 0x80; else i[4] &= ~0x80;
		}
	inline void SetS(TInt aValue)
		{
		if (aValue) i[4] |= 0x40; else i[4] &= ~0x40;
		}
	inline void SetO(TInt aValue)
		{
		if (aValue) i[4] |= 0x20; else i[4] &= ~0x20;
		}

	};


class TInet6HeaderICMP_Redirect : public TInet6HeaderICMP
/**
* ICMPv6 Redirect layout.
@verbatim
Redirect Message Format (RFC-2461)

 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|     Type      |     Code      |          Checksum             |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                           Reserved                            |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
+                                                               +
|                                                               |
+                       Target Address                          +
|                                                               |
+                                                               +
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
+                                                               +
|                                                               |
+                     Destination Address                       +
|                                                               |
+                                                               +
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|   Options ...
+-+-+-+-+-+-+-+-+-+-+-+-

@endverbatim
* @note
*	Above is longer thant what is declared in the base class
*	i-member. The user must verify the sufficient length of
*	the packet, when using this.
*
* Valid options:
*
* - #KInet6OptionICMP_TargetLink
* - #KInet6OptionICMP_Redirect
*
* @publishedAll
* @released
*/
	{
public:
	//
	// General
	//
	inline static TInt MinHeaderLength() {return 40; }
	inline static TInt MaxHeaderLength() {return 40; }
	inline TInt HeaderLength() const {return 40;}

	inline TIp6Addr &Target() const
		/**
		* Gets the Target Address.
		*
		* @return The target address (reference).
		*/
		{
		return (TIp6Addr &)i[8];
		}
	inline TIp6Addr &Destination() const
		/**
		* Gets the Destination Address.
		*
		* @return The destination address (reference).
		*/
		{
		return (TIp6Addr &)i[24];
		}
	};


class TInet6OptionICMP_LinkLayer
/**
* ICMPv6 Link-layer Address layout.
@verbatim
Source/Target Link-layer Address Option (RFC-2461)

 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|     Type      |    Length     |    Link-Layer Address ...
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
@endverbatim
* @publishedAll
* @released
*/
	{
public:
	inline static TInt MinHeaderLength() {return 8; }
	inline static TInt MaxHeaderLength() {return 8; }	// Not very useful
	inline TInt HeaderLength() const {return Length() * 8; }
	//
	// Access
	//
	inline TInt Type() const
		{
		return i[0];
		}
	inline TInt Length() const
		{
		return i[1];
		}
	//
	// Access and Set
	//
	inline TPtr8 Address() const
		{
		return TPtr8((TUint8 *)&i[2], i[1] * 8 - 2, i[1] * 8 - 2);
		}
	//
	// Construct methods
	//
	inline void SetType(TInt aType)
		{
		i[0] = (TUint8)aType;
		}
	inline void SetLength(TInt aLength)
		{
		i[1] = (TUint8)aLength;
		}
private:
	union
		{
		TUint8 i[8];
		TUint32 iAlign;	// A dummy member to force the 4 byte alignment
		};
	};


class TInet6OptionICMP_Prefix
/**
* ICMPv6 Prefix Infotmation Option.
@verbatim
Prefix Information Option (RFC-2461)
(+ Router Address flag from draft-ietf-mobileip-ipv6-08)
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|     Type      |    Length     | Prefix Length |L|A|R| Rsrvd1  |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                         Valid Lifetime                        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                       Preferred Lifetime                      |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                           Reserved2                           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
+                                                               +
|                                                               |
+                            Prefix                             +
|                                                               |
+                                                               +
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

@endverbatim
* @publishedAll
* @released
*/
	{
public:
	inline static TInt MinHeaderLength() {return 4*8; }
	inline static TInt MaxHeaderLength() {return 4*8; }	// Not very useful
	inline TInt HeaderLength() const {return 4*8; }

	inline TInt Type() const
		{
		return i[0];
		}
	inline TInt Length() const
		{
		return i[1];
		}
	inline TInt PrefixLength() const
		{
		return i[2];	// 0..128
		}
	inline TInt LFlag() const
		{
		return i[3] & 0x80;
		}
	inline TInt AFlag() const
		{
		return i[3] & 0x40;
		}
	inline TInt RFlag() const
		{
		return i[3] & 0x20;
		}
	inline TUint32 ValidLifetime() const
		{
		return (i[4] << 24) | (i[5] << 16) | (i[6] << 8) | i[7];
		}
	inline TUint32 PreferredLifetime() const
		{
		return (i[8] << 24) | (i[9] << 16) | (i[10] << 8) | i[11];
		}
	//
	//
	inline TIp6Addr &Prefix() const
		{
		return (TIp6Addr &)i[16];
		}
	//
	// Construct methods
	//
	inline void SetType(TInt aType)
		{
		i[0] = (TUint8)aType;
		}
	inline void SetLength(TInt aLength)
		{
		i[1] = (TUint8)aLength;
		}
	inline void SetPrefixLength(TInt aLength)
		{
		i[2] = (TUint8)aLength;
		}
	inline void SetFlags(TInt aFlags)
		{
		i[3] = (TUint8)aFlags;
		}
	inline void SetValidLifetime(TUint32 aTime)
		{
		i[7] = (TUint8)aTime;
		i[6] = (TUint8)(aTime >> 8);
		i[5] = (TUint8)(aTime >> 16);
		i[4] = (TUint8)(aTime >> 24);
		}
	inline void SetPreferredLifetime(TUint32 aTime)
		{
		i[11] = (TUint8)aTime;
		i[10] = (TUint8)(aTime >> 8);
		i[9] = (TUint8)(aTime >> 16);
		i[8] = (TUint8)(aTime >> 24);
		}
	inline void SetReserved2(TUint32 aFiller)
		{
		i[15] = (TUint8)aFiller;
		i[14] = (TUint8)(aFiller >> 8);
		i[13] = (TUint8)(aFiller >> 16);
		i[12] = (TUint8)(aFiller >> 24);
		}


private:
	union
		{
		TUint8 i[4*8];
		TUint32 iAlign;	// A dummy member to force the 4 byte alignment
		};
	};


class TInet6OptionICMP_Mtu
/**
* ICMPv6 MTU Option.
@verbatim
MTU Option (RFC-2461)

 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|     Type      |    Length     |           Reserved            |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                              MTU                              |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
@endverbatim
* @publishedAll
* @released
*/
	{
public:
	inline static TInt MinHeaderLength() {return 8; }
	inline static TInt MaxHeaderLength() {return 8; }	// Not very useful
	inline TInt HeaderLength() const {return 8; }

	inline TInt Type() const
		{
		return i[0];
		}
	inline TInt Length() const
		{
		return i[1];
		}
	inline TInt Mtu() const
		{
		return (i[4] << 24) | (i[5] << 16) | (i[6] << 8) | i[7];
		}
	//
	// Construct methods
	//
	inline void SetType(TInt aType)
		{
		i[0] = (TUint8)aType;
		}
	inline void SetLength(TInt aLength)
		{
		i[1] = (TUint8)aLength;
		// Silently ZERO the reserved bits... not too nice --- msa
		i[2] = 0;
		i[3] = 0;
		}
	inline void SetMtu(TUint32 aMtu)
		{
		i[7] = (TUint8)aMtu;
		i[6] = (TUint8)(aMtu >> 8);
		i[5] = (TUint8)(aMtu >> 16);
		i[4] = (TUint8)(aMtu >> 24);
		}
private:
	TUint8 i[8];
	};


#if 1
class TInet6OptionICMP_RouteInformation
// Route Information Option 
// Experimental: draft-draves-ipngwg-router-selection-01.txt
/**
*  ICMPv6 Route Information Option.
@verbatim
Default Router Preferences and More-Specific Routes

 0                   1                   2                   3 
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ 
|     Type      |    Length     | Prefix Length |Resvd|Prf|Resvd| 
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ 
|                        Route Lifetime                         | 
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ 
|                                                               | 
+                                                               + 
|                                                               | 
+                            Prefix                             + 
|                                                               | 
+                                                               + 
|                                                               | 
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ 
@endverbatim
* @publishedAll
* @released
*/
	{
public:
	inline static TInt MinHeaderLength() {return 8; }
	inline static TInt MaxHeaderLength() {return 3*8; }
	inline TInt HeaderLength() const {return Length()*8; }

	inline TInt Type() const
		{
		return i[0];
		}
	inline TInt Length() const
		{
		return i[1];
		}
	inline TInt PrefixLength() const
		{
		return i[2];	// 0..128
		}
	inline TInt Prf() const
		{
		return (i[3] >> 3) & 0x3;	// should be treated as 2bit signed int
		}
	inline TUint32 RouteLifetime() const
		{
		return (i[4] << 24) | (i[5] << 16) | (i[6] << 8) | i[7];
		}
	//
	// *WARNING* The "Prefix" returns a raw reference to the beginning
	// of the prefix field in the option structure. HOWEVER, the option
	// field can be shorter than 128 bits! If used to allocate space,
	// the maximum is allocated and the method is safe, but that is not
	// true if header is mapped directly to the received packet! -- msa
	inline TIp6Addr &Prefix() const
		{
		return (TIp6Addr &)i[8];
		}
	//
	// Construct methods
	//
	inline void SetType(TInt aType)
		{
		i[0] = (TUint8)aType;
		}
	inline void SetLength(TInt aLength)
		{
		i[1] = (TUint8)aLength;
		}
	inline void SetPrefixLength(TInt aLength)
		{
		i[2] = (TUint8)aLength;
		}
	inline void SetPrefixLifetime(TUint32 aTime)
		{
		i[7] = (TUint8)aTime;
		i[6] = (TUint8)(aTime >> 8);
		i[5] = (TUint8)(aTime >> 16);
		i[4] = (TUint8)(aTime >> 24);
		}

private:
	union
		{
		TUint8 i[3*8];	// The space allocated for MAX LENGTH
		TUint32 iAlign;	// A dummy member to force the 4 byte alignment
		};
	};

class TInet6OptionICMP_DnsInformation
/**
*  ICMPv6 Recursive DNS Server Option.
* IPv6 DNS Configuration based on Router Advertisement
*
* Experimental: draft-jeong-dnsop-ipv6-discovery-03.txt
@verbatim
Recursive DNS Server Option

 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|     Type      |     Length    |  Pref |        Reserved       |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                           Lifetime                            |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
:                     IPv6 Address of RDNSS                     :
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     
@endverbatim
* @publishedAll
* @released
*/
	{
public:
	inline static TInt MinHeaderLength() {return 24; }
	inline static TInt MaxHeaderLength() {return 24; }
	inline TInt HeaderLength() const {return Length()*8; }

	inline TInt Type() const
		{
		return i[0];
		}
	inline TInt Length() const
		{
		return i[1];
		}
	inline TInt Pref() const
		{
		return (i[3] >> 4) & 0xF;
		}
	inline TUint32 Lifetime() const
		{
		return (i[4] << 24) | (i[5] << 16) | (i[6] << 8) | i[7];
		}
	inline TIp6Addr &Address() const
		{
		return (TIp6Addr &)i[8];
		}
	//
	// Construct methods
	//
	inline void SetType(TInt aType)
		{
		i[0] = (TUint8)aType;
		}
	inline void SetLength(TInt aLength)
		{
		i[1] = (TUint8)aLength;
		}
	inline void SetPref(TInt aPref)
		{
		i[2] = (TUint8)(((aPref << 4) & 0xF0) | (i[2] & 0xF));
		}
	inline void SetLifetime(TUint32 aTime)
		{
		i[7] = (TUint8)aTime;
		i[6] = (TUint8)(aTime >> 8);
		i[5] = (TUint8)(aTime >> 16);
		i[4] = (TUint8)(aTime >> 24);
		}

private:
	union
		{
		TUint8 i[24];	// The space allocated for MAX LENGTH
		TUint32 iAlign;	// A dummy member to force the 4 byte alignment
		};
	};

#ifdef SYMBIAN_TCPIPDHCP_UPDATE
class TInet6OptionICMP_DnsInformationV1
/**
* ICMPv6 Recursive DNS Server Option(RFC-5006)
* IPv6 DNS Configuration based on Router Advertisement
*
*  
@verbatim
Recursive DNS Server Option

 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|     Type      |     Length    |  		        Reserved        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                           Lifetime                            |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
:                     IPv6 Address of RDNSS                     :
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     
@endverbatim
* @publishedAll
* @released
*/
	{
public:
	inline static TInt MinRdnssOptionLength() {return RDNSSOPTION_HDRLENGTH + RDNSSADDRSIZE;}//24
	
	inline static TInt MaxRdnssOptionLength() {return RDNSSOPTION_HDRLENGTH + (RDNSSADDRSIZE*RDNSS_MAX_ADDRESS); }//72
	
	inline TInt HeaderLength() const {return RDNSSOPTION_HDRLENGTH;}//8
	
	inline TInt Type() const
		{
		return i[0];
		}
	inline TInt Length() const
		{
		return i[1];
		}
	inline TUint32 Lifetime() const
		{
		return (i[4] << 24) | (i[5] << 16) | (i[6] << 8) | i[7];
		}
	inline TIp6Addr &Address() const
		{
		return (TIp6Addr &)i[8];
		}
	inline TIp6Addr &GetNextAddress(TInt aOffset) const
		{
		return (TIp6Addr &)i[aOffset];
		}
private:
	union
		{
		TUint8 i[RDNSSOPTION_HDRLENGTH + (RDNSSADDRSIZE * RDNSS_MAX_ADDRESS)];	// The space allocated for MAX LENGTH of 4 DNS address
		TUint32 iAlign;	// A dummy member to force the 4 byte alignment
		};
	};
#endif //SYMBIAN_TCPIPDHCP_UPDATE
#endif

/**
* @name ICMPv6 Error Message Types (0-127)
*/
//@{
const TUint8 KInet6ICMP_Unreachable		= 1;
const TUint8 KInet6ICMP_PacketTooBig	= 2;
const TUint8 KInet6ICMP_TimeExceeded	= 3;
const TUint8 KInet6ICMP_ParameterProblem= 4;
//@}
/**
* @name ICMPv6 Informational Message Types (128-255)
*/
//@{
/** Echo Request. See TInet6HeaderICMP_Echo. */
const TUint8 KInet6ICMP_EchoRequest		= 128;
/** Echo Reply. See TInet6HeaderICMP_Echo. */
const TUint8 KInet6ICMP_EchoReply		= 129;
/** Not implemented. */
const TUint8 KInet6ICMP_GroupQuery		= 130;
/** Not implemented. */
const TUint8 KInet6ICMP_GroupReport		= 131;
/** Not implemented. */
const TUint8 KInet6ICMP_GroupDone		= 132;
/** Router Solicitation. See TInet6HeaderICMP_RouterSol. */
const TUint8 KInet6ICMP_RouterSol		= 133;
/** Router Advertisement. See TInet6HeaderICMP_RouterAdv. */
const TUint8 KInet6ICMP_RouterAdv		= 134;
/** Neighbor Solicitation. See TInet6HeaderICMP_NeighborSol. */
const TUint8 KInet6ICMP_NeighborSol		= 135;
/** Neighbor Advertisement. See TInet6HeaderICMP_NeighborAdv. */
const TUint8 KInet6ICMP_NeighborAdv		= 136;
/** Redirect. See TInet6HeaderICMP_Redirect. */
const TUint8 KInet6ICMP_Redirect		= 137;
//@}

/**
* @name ICMPv6 Option types.
* The default derivation of the symbol
* is from the name of the header class by replacing 'T' with 'K' (or
* vice versa).
*
*/
//@{
/** Source Link-Layer Address. See TInet6OptionICMP_LinkLayer. */
const TInt KInet6OptionICMP_SourceLink	= 1;
/** Target Link-Layer Address. See TInet6OptionICMP_LinkLayer. */
const TInt KInet6OptionICMP_TargetLink	= 2;
/** Prefix Information. See TInet6OptionICMP_Prefix. */
const TInt KInet6OptionICMP_Prefix		= 3;
/** Redirect Header. (not implemented). */
const TInt KInet6OptionICMP_Redirect	= 4;
/** MTU.  See TInet6OptionICMP_Mtu. */
const TInt KInet6OptionICMP_Mtu			= 5;
#ifdef SYMBIAN_TCPIPDHCP_UPDATE
/** RFC 5006: Recursive DNS Server Option. See TInet6OptionICMP_DnsInformationV1*/
const TInt KInet6OptionICMP_RDNSS		= 25;
#endif //SYMBIAN_TCPIPDHCP_UPDATE
#if 1
	// Experimental: draft-draves-ipngwg-router-selection-01.txt
	// Default Router Preferences and More-Specific Routes
	// *UNOFFICIAL NUMBER ASSIGNMENT (SAME AS MSR STACK)--REAL VALUE TBD*
/** Route Information. See TInet6OptionICMP_RouteInformation. */
const TInt KInet6OptionICMP_RouteInformation = 9;
#endif
//@}

//@}
#endif
