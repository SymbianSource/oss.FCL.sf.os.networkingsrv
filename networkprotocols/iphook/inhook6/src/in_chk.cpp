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
// in_chk.cpp - IPv6/IPv4 checksum module
//

#include <in_sock.h> // IPv6 enhanced in_sock.h
#include "in_chk.h"
#include "in_pkt_platform.h"


EXPORT_C void TChecksum::Add(RMBufChain &aPacket, TInt aOffset)
	{
	TMBufIter m(aPacket);
	const RMBuf *mbuf;
    TUint32 count = 0;

	while ((mbuf = m++) != NULL)
		{
	    const TUint16 *p, *end;
		const TUint8 *ptr = mbuf->Ptr(), *boundary;
		TInt len = mbuf->Length();
	    union { TUint16 i16; TUint8 i8[2]; } u;

		if (len <= 0)
			continue;
		if (aOffset > 0)
			{
			aOffset -= len;
			if (aOffset >= 0)
				continue;
			//
			// Setup for computing the checksum, starting
			// from middle of the buffer. aOffset is left
			// with negative value.
			//
			ptr += (aOffset + len);
			len = -aOffset;
	        }

		//
	    // Collect any solitary leading and trailing octets
	    // into a single half word and add that first.
	    //
	    u.i16 = 0;

	    // Leading octet
	    if ((TUint32)ptr & 1)
	    	{
		    u.i8[1] = *ptr++;
            count++;
            len--;
	        }

		// Trailing octet
	    boundary = ptr + len;
	    if (len & 1)
			u.i8[0] = *--boundary;
	    end = (TUint16*)boundary;

        //
        // Explicit loop unrolling for checksum calculation.
            //
        // Duff's device, while not pretty, is perfectly legal
        // C++ as well as being the most efficient way to do it.
        // The switch statement is used to enter the do-while
        // loop at the correct offset. As a result, we get one
        // partial iteration of the loop followed by zero or
        // more full iterations. Exacly four iterations are
        // required to process a completely filled RMBuf.
        //
        // The gcc used in the current SDK tool chain doesnt't
        // seem to be very good at optimizing ARM code, so we
        // try to make things simple for it.
        //
	    register TUint32 sum = u.i16;
        const TInt loops = (len + 30) >> 5;
        if (loops)
        	{
            p = end - (loops << 4);
		    switch ((len >> 1) & 15)
		    	{
			default:
				do {			//lint -fallthrough
		                case  0:		sum += p[ 0];	//lint -fallthrough
		                case 15:		sum += p[ 1];	//lint -fallthrough
		                case 14:		sum += p[ 2];	//lint -fallthrough
		                case 13:		sum += p[ 3];	//lint -fallthrough
		                case 12:		sum += p[ 4];	//lint -fallthrough
		                case 11:		sum += p[ 5];	//lint -fallthrough
		                case 10:		sum += p[ 6];	//lint -fallthrough
		                case  9:		sum += p[ 7];	//lint -fallthrough
		                case  8:		sum += p[ 8];	//lint -fallthrough
		                case  7:		sum += p[ 9];	//lint -fallthrough
		                case  6:		sum += p[10];	//lint -fallthrough
		                case  5:		sum += p[11];	//lint -fallthrough
		                case  4:		sum += p[12];	//lint -fallthrough
		                case  3:		sum += p[13];	//lint -fallthrough
		                case  2:		sum += p[14];	//lint -fallthrough
		                case  1:		sum += p[15];
					} while ((p += 16) < end);
		        }
			}

        //
        // Swap byte order if there is an alignment mismatch
        // between this buffer and the previous ones. We fold
        // the partial sum into the lower half word in order
        // to avoid the possibility of overflow.
        //
        if (count & 1)
        	{
	        iSum += (sum >> 24);          // add octet  3
	        iSum += (sum >> 8) & 0xffff,  // add octets 2-1
	        iSum += (sum << 8) & 0xff00;  // add octet  0
        	}
        else
        	{
	        iSum += sum;
        	}

        // Update number of bytes summed
        count += len;
    	}
	}

EXPORT_C void TChecksum::Add(RMBufChain &aPacket, const RMBufPktInfo *aInfo, TInt aOffset)
	{
	if (aInfo)
		{
		//
		// Compute the pseudo header part
		//
		// It is assumed that the aInfo->iLength is the length
		// of the full IPv6 packet, and the aOffset gives the
		// beginning of the upper layer header. For the pseudo
		// header computation, the upper layer length must be
		// used and thus the aOffset is subtracted!
		//
        AddH((TUint32)aInfo->iLength - aOffset);
        AddH((TUint16)aInfo->iProtocol);
		Add((TUint16 *)&TInetAddr::Cast(aInfo->iSrcAddr).Ip6Address(), 16);
		Add((TUint16 *)&TInetAddr::Cast(aInfo->iDstAddr).Ip6Address(), 16);
        }
	//
	// Add the payload part
	//
	Add(aPacket, aOffset);
	}

EXPORT_C TUint32 TChecksum::Calculate(const TUint16 *aPtr, TInt aLen)
	{
	register TUint32 sum = 0;

    if (aLen >>= 1)
    	{
        //
        // Explicit loop unrolling for checksum calculation.
            //
        // Duff's device, while not pretty, is perfectly legal
        // C++ as well as being the most efficient way to do it.
        // The switch statement is used to enter the do-while
        // loop at the correct offset. As a result, we get one
        // partial iteration of the loop followed by zero or
        // more full iterations. Exacly four iterations are
        // required to process a completely filled RMBuf.
        //
        // The gcc used in the current SDK tool chains doesnt't
        // seem to be very good at optimizing ARM code, so we
        // try to make things simple for it.
        //
    	const TUint16 *end = aPtr + aLen;
        const TInt loops = (aLen + 15) >> 4;
        register const TUint16 *p = end - (loops << 4);
		switch (aLen & 15)
			{
		default:
			do {			//lint -fallthrough
		        case  0:		sum += p[ 0];	//lint -fallthrough
		        case 15:		sum += p[ 1];	//lint -fallthrough
		        case 14:		sum += p[ 2];	//lint -fallthrough
		        case 13:		sum += p[ 3];	//lint -fallthrough
		        case 12:		sum += p[ 4];	//lint -fallthrough
		        case 11:		sum += p[ 5];	//lint -fallthrough
		        case 10:		sum += p[ 6];	//lint -fallthrough
		        case  9:		sum += p[ 7];	//lint -fallthrough
		        case  8:		sum += p[ 8];	//lint -fallthrough
		        case  7:		sum += p[ 9];	//lint -fallthrough
		        case  6:		sum += p[10];	//lint -fallthrough
		        case  5:		sum += p[11];	//lint -fallthrough
		        case  4:		sum += p[12];	//lint -fallthrough
		        case  3:		sum += p[13];	//lint -fallthrough
		        case  2:		sum += p[14];	//lint -fallthrough
		        case  1:		sum += p[15];
				} while ((p += 16) < end);
			}
        }

	return sum;
	}
