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
// ip4_hdr.h - IPv4 header structure
// Defines the basic classes for accessing the header
// structures within IPv4 packets.
//



/**
 @file ip4_hdr.h
 @ingroup ip_packet_formats
 @publishedAll
 @released
*/

#ifndef __IP4_HDR_H__
#define __IP4_HDR_H__

#include "in_hdr.h"
#include "es_sock.h"		// for ByteOrder only!!!

/**
* @addtogroup ip_packet_formats
* @{
*/

/**
* @name IP v4 constants 
* @since v7.0
* @publishedAll
* @released
*/
//@{
const TUint8 KInet4IP_DF = 0x40;	//< Don't Fragment flag
const TUint8 KInet4IP_MF = 0x20;	//< More Fragments flag
const TInt KInetMinMtu = 68;		//< Minimum MTU as defined in RFC-791 
//@}

class TInet6HeaderIP4
/**
*	Encapsulates an IPv4 IP header. 
* 
@verbatim
  ************************
  Extract from the RFC-791
  ************************
  0                   1                   2                   3
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |Version|  IHL  |Type of Service|          Total Length         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |         Identification        |Flags|      Fragment Offset    |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |  Time to Live |    Protocol   |         Header Checksum       |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                       Source Address                          |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                    Destination Address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                    Options                    |    Padding    |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  
  Version:  4 bits = 4
  IHL:  4 bits

    Internet Header Length is the length of the internet header in 32
    bit words, and thus points to the beginning of the data.  Note that
    the minimum value for a correct header is 5

  Type of Service:  8 bits
      Bits 0-2:  Precedence.
      Bit    3:  0 = Normal Delay,      1 = Low Delay.
      Bits   4:  0 = Normal Throughput, 1 = High Throughput.
      Bits   5:  0 = Normal Relibility, 1 = High Relibility.
      Bit  6-7:  Reserved for Future Use.

	  Precedence

          111 - Network Control
          110 - Internetwork Control
          101 - CRITIC/ECP
          100 - Flash Override
          011 - Flash
          010 - Immediate
          001 - Priority
          000 - Routine
  Total Length:  16 bits
    Total Length is the length of the datagram, measured in octets,
    including internet header and data.

  Identification:  16 bits
    An identifying value assigned by the sender to aid in assembling the
    fragments of a datagram.

  Flags:  3 bits
    Various Control Flags.

      Bit 0: reserved, must be zero
      Bit 1: (DF) 0 = May Fragment,  1 = Don't Fragment.
      Bit 2: (MF) 0 = Last Fragment, 1 = More Fragments.

  Fragment Offset:  13 bits

    This field indicates where in the datagram this fragment belongs.
    The fragment offset is measured in units of 8 octets (64 bits).  The
    first fragment has offset zero.

  Time to Live:  8 bits
  Protocol:  8 bits
  Header Checksum:  16 bits
  Source Address:  32 bits
  Destination Address:  32 bits
@endverbatim

@publishedAll
@released
@since v7.0
*/
	{
public:
	//
	// Basic
	//
	/**
	* Gets the minimum header length.	
	* @return Minimum header length (= 20) 
	* @since v7.0
	*/
	inline static TInt MinHeaderLength() {return 4*5; }
	/**
	* Gets the maximum header length.
	* @return Maximum header length (= 60)
	* @since v7.0
	*/
	inline static TInt MaxHeaderLength() {return 4*15; }
	/**
	* Gets a pointer to the byte following the header.
	* @return Pointer to the byte following the header
	* @since v7.0
	*/
	inline TUint8 *EndPtr() {return i + HeaderLength();}

	enum TOffsets
		{
		O_TotalLength = 2,
		O_FragmentOffset = 6,
		O_TTL = 8,
		O_Protocol = 9
		};
	inline TInt Version() const
		/**
		* Gets the IP version from the header.
		* @return IP version (should be 4 for IPv4)
		*/ 
		{
		return (i[0] >> 4) & 0xf;
		}
	inline TInt HeaderLength() const
		/**
		* Gets the header length.
		* @return Header length in bytes (based on IHL field)
		*/
		{
		return (i[0] & 0x0f) * 4;	// Note: Returns bytes length!
		}
	inline TInt TOS() const
		/**
		* Gets the TOS from the header.
		* @return TOS
		*/
		{
		return i[1];
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
		return ((TOS() & 3) == 3);
		}
	inline TInt TotalLength() const
		/**
		* Gets the Total Length from the header.
		* @return Total Length (includes IP header and payload)
		*/
		{
		return (i[2] << 8) | i[3];
		}
	inline TInt Identification() const
		/**
		* Gets the Identification from the header.
		* @return Identification [0..65535]
		*/
		{
		return (i[4] << 8) | i[5];
		}
	inline TInt Flags() const
		/**
		* Gets the Flags from the header.
		* @note returns flags byte as is, may include bits of fragment offset!
		* @return Flags
		*/
		{
		return i[6];
		}
	inline TInt DF() const
		/**
		* Gets the DF flag from the header.
		* @return DF flag (= KInet4IP_DF, if set and zero otherwise)
		*/
		{
		return i[6] & KInet4IP_DF;
		}
	inline TInt MF() const
		/**
		* Gets the MF flag from the header.
		* @return MF flag (= KInet4IP_MF, if set and zero otherwise)
		*/
		{
		return i[6] & KInet4IP_MF;
		}
	inline TInt FragmentOffset() const
		/**
		* Gets the Fragment Offset from the header.
		* @return Fragment Offset (raw 8 octet units, not a bytes offset!)
		*/
		{
		return ((i[6] & 0x1f) << 8) | i[7];
		}
	inline TInt Ttl() const
		/**
		* Gets the Time to Live from the header.
		* @return Time to Live [0..255]
		*/
		{
		return i[8];
		}
	inline TInt Protocol() const
		/**
		* Gets the Protocol from the header.
		* @return Protocol [0..255]
		*/
		{
		return i[9];
		}
	inline TInt Checksum() const
		/**
		* Gets the Header Checksum from the header.
		* @return Header Checksum (in NETWORK byte order)
		*/
		{
		// Checksum is used in network byte order
		return *((TUint16 *)&i[10]);
		}
	inline TUint32 SrcAddr() const
		/**
		* Gets the source address from the header.
		* @return Source address (in host byte order)
		*/
		{
		return (i[12] << 24) | (i[13] << 16) | (i[14] << 8) | i[15];
		}
	inline TUint32 DstAddr() const
		/**
		* Gets the destination address from the header.
		* @return Destination address (in host byte order)
		*/
		{
		return (i[16] << 24) | (i[17] << 16) | (i[18] << 8) | i[19];
		}
	inline TUint32 &SrcAddrRef() const
		/**
		* Gets a raw reference to the source address in network byte order.
		* @return Raw reference to the source address
		*/
		{
		return (TUint32 &)i[12];
		}
	inline TUint32 &DstAddrRef() const
		/**
		* Gets a raw reference to the destination address in network byte order.
		* @return Raw reference to the destination address
		*/
		{
		return (TUint32 &)i[16];
		}
	inline TPtrC8 Options() const
		/**
		* Gets the Options from the header (const overload).
		* @return Options
		*
		* @note
		*	This relies on correct value of IHL! Must not be used with
		*	corrupt headers (will panic if IHL < 5!).
		*/
		{
		// *NOTE* This includes the padding bytes, or can be empty!
		return TPtrC8((TUint8 *)&i[20], HeaderLength() - 20);
		}
	inline TPtr8 Options()
		/**
		* Gets the Options from the header.
		* @return Options
		*
		* @note
		*	This relies on correct value of IHL! Must not be used with
		*	corrupt headers (will panic if IHL < 5!).
		*/
		{
		// It is yet unclear what will be the best way to build
		// the options into the header. For the time being this
		// method returns a modifiable Ptr8 to the the available
		// option space. For this to work, the application must
		// have used the SetHeaderLength() to fix the current
		// available length.
		return TPtr8((TUint8 *)&i[20], HeaderLength() - 20);
		}

	inline void Init(TInt aTOS = 0)
		/**
		* Initialises the IPv4 header to basic initial values.
		*
		* @li	Version = 4
		* @li	IHL = 5
		* @li	Total Length = 20
		* @li	Identification = 0
		* @li	Fragment Offset = 0
		* @li	Flags = 0
		* @li	TTL = 0
		* @li	Checksum = 0
		* @li	TOS = aTOS (optional parameter, default = 0)
		*
		* However, address fields are not touched, because
		* they are most often set separately in any case.
		*
		* @param	aTOS	initial value for TOS (= 0)
		*/
		{
		i[0] = 0x45;			// Version=4, IHL=5 (= 20 bytes)
		i[1] = (TUint8)aTOS;	// TOS
		i[2] = 0;
		i[3] = 20;				// Total length = 20
		*((TInt32 *)&i[4]) = 0;	// Identification = 0, flags=0, Fragment offset = 0;
		*((TInt32 *)&i[8]) = 0;	// TTL=0,Protocol=0,Checksum=0
		}
	//
	// Build, set IP header field values into the packet
	//

	inline void SetVersion(TInt aVersion)
		/**
		* Sets the IP version in the header.
		* @param	aVersion the value to be set [0..15]
		*/
		{
		i[0] = (TUint8)((i[0] & 0x0f) | ((aVersion << 4) & 0xf0));
		}
	inline void SetHeaderLength(TInt aLength)
		/**
		* Sets the header length (IHL).
		*
		* @param aLength
		*	the length of the IPv4 header in BYTES. The
		*	IHL is computed from this, without any sanity
		*	checks. The valid range is [20..60].
		*/
		{
		i[0] =  (TUint8)((i[0] & 0xf0) | ((aLength >> 2) & 0x0f));
		}
	inline void SetTOS(TInt aTos)
		/**
		* Sets the TOS in the header.
		* @param	aTos	The TOS value to set [0..255]
		*/
		{
		i[1] = (TUint8)aTos;
		}
	inline void SetTotalLength(TInt aLength)
		/**
		* Sets the Total Length in the header.
		*
		* @param	aLength the length of combined header and
		*			payload in bytes (no sanity checks, but the
		*			value should be in range [20..65535]). Only
		*			16 least significant bits used.
		*/
		{
		i[3] = (TUint8)aLength;
		i[2] = (TUint8)(aLength >> 8);
		}
	inline void SetIdentification(TInt aId)
		/**
		* Sets the Identification in the header.
		*
		* @param	aId	the value to be set (only 16 least significant
		*			bits are used, rest is ignored).
		*/
		{
		i[5] = (TUint8)aId;
		i[4] = (TUint8)(aId >> 8);
		}

	inline void SetFlags(TUint8 aFlags)
		/**
		* Sets the Flags in the header.
		*
		* Flags are assumed to be in the three most significant bits
		* of aFlags, in their proper positions.
		* (No individual settings provided, if you need to set a flag
		* without affecting others, use Flags() to get old values,
		* update and store the result with SetFlags()).
		*
		* @param	aFlags	contains the new flags
		*/
		{
		i[6] = (TUint8)((i[6] & 0x1f) | (aFlags & 0xe0));
		}
	inline void SetFragmentOffset(TUint16 aOffset)
		/**
		* Sets the Fragment Offset in the header.
		* @param aOffset Fragment Offset (8 octet units, not in bytes)
		*/
		{
		i[6] = (TUint8)((i[6] & 0xe0) | ((aOffset >> 8) & 0x1f));
		i[7] = (TUint8)aOffset;
		}
	inline void SetTtl(TInt aTTL)
		/**
		* Sets the Time to Live in the header.
		* @param aTTL Time to Live [0..255]
		*/
		{
		i[8] = (TUint8)aTTL;
		}
	inline void SetProtocol(TInt aProtocol)
		/**
		* Sets the Protocol in the header.
		* @param aProtocol Protocol [0..255]
		*/
		{
		i[9] = (TUint8)aProtocol;
		}
	inline void SetChecksum(TInt aSum)
		/**
		* Sets the Header Checksum in the header.
		* @param aSum Header Checksum [0..65535]
		*	(16 least significant bits stored
		*	as is (assumed to be in NETWORK byte order).					
		*/
		{
		// Checksum is used in network byte order
		*((TUint16 *)&i[10]) = (TUint16)aSum;
		}

	inline void SetSrcAddr(TUint32 aAddr)
		/**
		* Sets the source address in the header.
		* @param aAddr Source address (IPv4, 32 bit integer in host byte order)
		*/
		{
		i[15] = (TUint8)aAddr;
		i[14] = (TUint8)(aAddr >> 8);
		i[13] = (TUint8)(aAddr >> 16);
		i[12] = (TUint8)(aAddr >> 24);
		}

	inline void SetDstAddr(TUint32 aAddr)
		/**
		* Sets the destination address in the header.
		* @param aAddr Destination IPv4 address (32 bit integer) in host byte order
		*/
		{
		i[19] = (TUint8)aAddr;
		i[18] = (TUint8)(aAddr >> 8);
		i[17] = (TUint8)(aAddr >> 16);
		i[16] = (TUint8)(aAddr >> 24);
		}

	//
	// The old IPv4 stack leaves IP header in packet when passing it upwards,
	// but this header is swapped into host order. As upper layers really don't
	// need this stuff much, only few "compatibility" methods is defined here
	//

	inline TInt HostHeaderLength() const
		{
		/**
		* Gets the Header Length from a header that is in Host byte order.
		* @return Header Length
		*
		* @deprecated There is no reason to use swapped headers
		*/
		return (i[3] & 0x0f) * 4;
		}
	inline TInt HostProtocol() const
		/**
		* Gets the Protocol from a header that is in Host byte order.
		* @return Protocol
		* @deprecated There is no reason to use swapped headers
		*/
		{
		return i[10];
		}

	inline void Swap()
		/**
		* Swaps the byte order in the header.
		* @deprecated There is no reason to use swapped headers
		*/
		{
		*(TUint32 *)(&i[0]) = ByteOrder::Swap32(*(TUint32 *)(&i[0]));
		*(TUint32 *)(&i[4]) = ByteOrder::Swap32(*(TUint32 *)(&i[4]));
		*(TUint32 *)(&i[8]) = ByteOrder::Swap32(*(TUint32 *)(&i[8]));
		*(TUint32 *)(&i[12]) = ByteOrder::Swap32(*(TUint32 *)(&i[12]));
		*(TUint32 *)(&i[16]) = ByteOrder::Swap32(*(TUint32 *)(&i[16]));
		}
				
private:
	union
		{
		TUint8 i[4*15];	//< This allocates maximum length (60 bytes)
		TUint32 iAlign;	//< A dummy member to force the 4 byte alignment
		};
	};

/**
* @name IP v4 Option constants
* @since v7.0
* @publishedAll
* @released
*/
//@{
const TUint8 KInet4Option_End			= 0x00;
const TUint8 KInet4Option_Nop			= 0x01;

const TUint8 KInet4OptionFlag_Copy		= 0x80;
//@}

//
//	ICMP v4 constants
//	=================
//
/**
* @name ICMP v4 constants 
* @since v7.0
* @publishedAll
* @released
*/
//@{
/** Echo Reply. See TInet6HeaderICMP_Echo (IPv4 and IPv6 use the same format). */
const TUint8 KInet4ICMP_EchoReply		=  0;
const TUint8 KInet4ICMP_Unreachable		=  3;
const TUint8 KInet4ICMP_SourceQuench	=  4;
const TUint8 KInet4ICMP_Redirect		=  5;
/** Echo Request. See TInet6HeaderICMP_Echo (IPv4 and IPv6 use the same format). */
const TUint8 KInet4ICMP_Echo			=  8;
const TUint8 KInet4ICMP_TimeExceeded	= 11;
const TUint8 KInet4ICMP_ParameterProblem = 12;
const TUint8 KInet4ICMP_TimeStamp		= 13;
const TUint8 KInet4ICMP_TimeStampReply	= 14;
//@}

//@}
#endif
