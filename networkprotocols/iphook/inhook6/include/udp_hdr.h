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
// udp_hdr.h - UDP header structure
// Defines the basic classes for accessing the header
// structures within UDP packets.
//



/**
 @file udp_hdr.h
 @ingroup ip_packet_formats
 @publishedAll
 @released
*/

#ifndef __UDP_HDR_H__
#define __UDP_HDR_H__

#include <e32def.h>
#include "in_hdr.h"

/**
* @addtogroup ip_packet_formats
* @{
*/

// TInet6HeaderUPD

class TInet6HeaderUDP
	/** UDP Header format.
    * User Datagram Header Format
	* @verbatim
Extract from RFC-768
                                    
     0      7 8     15 16    23 24    31  
    +--------+--------+--------+--------+ 
    |     Source      |   Destination   | 
    |      Port       |      Port       | 
    +--------+--------+--------+--------+ 
    |                 |                 | 
    |     Length      |    Checksum     | 
    +--------+--------+--------+--------+ 
    |                                     
    |          data octets ...            
    +---------------- ...                 
@endverbatim
	* @publishedAll
	* @released
	*/
	{
public:
	inline TInt HeaderLength() const {return 8;}
	inline static TInt MinHeaderLength() {return 8; }
	inline static TInt MaxHeaderLength() {return 8; }
	inline TUint8 *EndPtr() {return i + HeaderLength();}
	//
	// Access, Get UDP field values from the packet
	//
	inline TUint SrcPort() const
		{
		return (i[0] << 8) + i[1];
		}
	inline TUint DstPort() const
		{
		return (i[2] << 8) + i[3];
		}
	inline TInt Length() const
		{
		return (i[4] << 8) + i[5];
		}
	inline TInt Checksum() const
		{
		// Checksum is used in network byte order
		return *((TUint16 *)&i[6]);
		}
	//
	// Build, Set UDP field value to the packet
	//
	inline void SetSrcPort(TUint aPort)
		{
		i[0] = (TUint8)(aPort >> 8);
		i[1] = (TUint8)aPort;
		}
	inline void SetDstPort(TUint aPort)
		{
		i[2] = (TUint8)(aPort >> 8);
		i[3] = (TUint8)aPort;
		}
	inline void SetLength(TInt aLength)
		{
		i[4] = (TUint8)(aLength / 256);
		i[5] = (TUint8) aLength;
		}
	inline void SetChecksum(TInt aSum)
		{
		// Checksum is used in network byte order
		*((TUint16 *)&i[6]) = (TUint16)aSum;
		}
private:
	TUint8 i[8];
	};
/** @} */
#endif
