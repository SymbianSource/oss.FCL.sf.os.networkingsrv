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
// arp_hdr.h - ARP header structure
// ARP header structure.
//



/**
 @file arp_hdr.h
 @ingroup ip_packet_formats
 @publishedAll
 @released
*/

#ifndef __ARP_HDR_H__
#define __ARP_HDR_H__

#include "in_hdr.h"

/**
* @addtogroup  ip_packet_formats
* @{
*/

/**
* "Fake" protocol number for ARP.
*
* This protocol value is only used to recognize the ARP packets
* coming from the interface. There is no "real" ARP.PRT.
*/
const TUint KProtocolArp		= 0xFAD;	// (get some better value)

const TUint16 KArpProtocolType_IP = 0x0800;	// Protocol Type value for IPv4
//
// Hardware types listed only as example, it is assumed that the
// driver knows its own type...
//
const TUint16 KArpHarwareType_ETHERNET = 1;
const TUint16 KArpHarwareType_IEEE_802 = 6;

/**
@publishedAll
@released
*/
enum TArpOperation	// from RFC-1700
	{
	EArpOperation_REQUEST = 1,			// RFC-826
	EArpOperation_REPLY = 2,			// RFC-826
	EArpOperation_REQUEST_REVERSE = 3,	// RFC-903
	EArpOperation_REPLY_REVERSE = 4,	// RFC-903
	EArpOperation_DRARP_REQUEST = 5,	//
	EArpOperation_DRARP_REPLY = 6,
	EArpOperation_DRARP_ERROR = 7,
	EArpOperation_INARP_REQUEST = 8,	// RFC-1293
	EArpOperation_INARP_REPLY = 9,		// RFC-1293
	EArpOperation_ARP_NAK = 10
	};


class TInet6HeaderArp
	/**
	* ARP Header.
	*
@verbatim
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |        Hardware Type          |         Protocol Type         |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |  HwAddrLen    |   PrAddrLen   |         ARP Operation         |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |         Sender's physical hardware address                    |
   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 |         Sender's protocol address                             |
   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 |         Target's physical hardware address                    |
   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 |         Target's protocol address                             |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
@endverbatim
	* @note
	* This definition covers only the fixed portion of the
	* message. DO NOT DECLARE A VARIABLE WITH THIS CLASS!
	* @publishedAll
	* @released
	*/
	{
public:
	inline static TInt MinHeaderLength() {return 8; }
	inline static TInt MaxHeaderLength() {return 128; }	

	inline TUint8* EndPtr() { return i + HeaderLength(); }
	inline TInt HeaderLength() const { return 8 + 2 * HwAddrLen() + 2 * PrAddrLen(); } // Return true byte length

	// Access methods

	inline TInt HardwareType() const	// Return hardware type value in host order
		{
		return i[0] << 8 | i[1];
		}

	inline TInt ProtocolType() const	// Return protocol type value in host order
		{
		return i[2] << 8 | i[3];
		}

	inline TInt HwAddrLen()	const		// Return hardware address length (in bytes)
		{
		return i[4];
		}

	inline TInt PrAddrLen()	const		// Return protocol address length (in bytes)
		{
		return i[5];
		}

	inline TInt Operation() const
		{
		return i[6] << 8 | i[7];
		}

	// Access/Modify

	inline TPtr8 SenderHwAddr()
		{
		return TPtr8(&i[8], HwAddrLen(), HwAddrLen());
		}
	inline TPtr8 SenderPrAddr()
		{
		return TPtr8(&i[8] + HwAddrLen(), PrAddrLen(), PrAddrLen());
		}
	inline TPtr8 TargetHwAddr()
		{
		return TPtr8(&i[8] + HwAddrLen() + PrAddrLen(), HwAddrLen(), HwAddrLen());
		}
	inline TPtr8 TargetPrAddr()
		{
		return TPtr8(&i[8] + 2*HwAddrLen() + PrAddrLen(), PrAddrLen(), PrAddrLen());
		}

	// Modify

	inline void SetHardwareType(TInt aType)	// Set hardware type value
		{
		i[1] = (TUint8)aType;
		i[0] = (TUint8)(aType >> 8);
		}

	inline void SetProtocolType(TInt aType)	// Set protocol type value
		{
		i[3] = (TUint8)aType;
		i[2] = (TUint8)(aType >> 8);
		}

	inline void SetHwAddrLen(TInt aLength)	// Set hardware address length (in bytes)
		{
		i[4] = (TUint8)aLength;
		}

	inline void SetPrAddrLen(TInt aLength)	// Set protocol address length (in bytes)
		{
		i[5] = (TUint8)aLength;
		}

	inline void SetOperation(TInt aOperation)
		{
		i[7] = (TUint8)aOperation;
		i[6] = (TUint8)(aOperation >> 8);
		}

private:
	union
		{
		TUint8 i[8];
		TUint32 iAlign;	// A dummy member to force the 4 byte alignment
		};
	};

/** @} */
#endif
