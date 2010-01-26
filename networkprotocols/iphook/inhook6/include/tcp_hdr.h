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
// tcp_hdr.h - TCP protocol header structure
// Defines the basic classes for accessing the header structures within TCP packets.
//



/**
 @file tcp_hdr.h 
 @ingroup ip_packet_formats
 @publishedAll
 @released
*/

#ifndef __TCP_HDR_H__
#define __TCP_HDR_H__

#include <e32std.h>
#include "in_hdr.h"
#include "sbque.h"

/**
* @addtogroup ip_packet_formats
* @{
*/

//
// TInet6HeaderTCP
//
//	*NOTE*
//		TInet6HeaderTCP declares for maximum length TCP header
//		(64	bytes). A valid TCP packet can be shorter and failing
//		to map full 64 bytes from a packet is not an error condition.
//

/** @name TCP Header length constants
* @{
*/
const TInt KTcpMinHeaderLength = 20;
const TInt KTcpMaxHeaderLength = 60;
const TInt KTcpMaxOptionLength = 40;
/** @} */

/** @name TCP control flags
* @{
*/
const TUint8 KTcpCtlFIN = 1;
const TUint8 KTcpCtlSYN = 2;
const TUint8 KTcpCtlRST = 4;
const TUint8 KTcpCtlPSH = 8;
const TUint8 KTcpCtlACK = 16;
const TUint8 KTcpCtlURG = 32;
const TUint8 KTcpCtlECE = 0x40;
const TUint8 KTcpCtlCWR = 0x80;
/** @} */

/** @name TCP options
* @{
*/
const TUint8 KTcpOptEnd		=  0;
const TUint8 KTcpOptNop		=  1;
const TUint8 KTcpOptMSS		=  2;
const TUint8 KTcpOptWScale	=  3;
const TUint8 KTcpOptSackOk	=  4;
const TUint8 KTcpOptSack	=  5;
const TUint8 KTcpOptTimeStamps  =  8;
const TUint8 KTcpOptCount	=  9;

const TUint8 KTcpOptMinLen[] =  {1,  1,  4,  3,  2,  6,  2,  2, 10};
const TUint8 KTcpOptMaxLen[] =  {1,  1,  4,  3,  2, 36, 40, 40, 10};
const TUint32 KTcpOptAlignFlag = 0x00000001;
/** @} */


class TTcpOptions
/** TCP Options Header.
@publishedAll
@released
*/
	{
public:
  	inline TTcpOptions()			      { Init(); }
  	IMPORT_C void Init();
  	IMPORT_C TInt Length() const;
  	inline TBool Error() const		  { return iError; }
  	inline void ClearError()		      { iError = EFalse; }

  	inline TInt MSS() const		      { return iMSS; }
  	inline void SetMSS(TInt aMSS)		  { iMSS = aMSS; }
  	inline void ClearMSS()		      { iMSS = -1; }

  	inline TBool TimeStamps() const	  { return iTimeStamps; }
  	inline TBool TimeStamps(TUint32& aTsVal, TUint32& aTsEcr) const
    	{
      	aTsVal = iTsVal;
      	aTsEcr = iTsEcr;
      	return iTimeStamps;
    	}
  	inline void SetTimeStamps(TUint32 aTsVal, TUint32 aTsEcr)
    	{
      	iTsVal = aTsVal;
      	iTsEcr = aTsEcr;
      	iTimeStamps = ETrue;
    	}
  	inline void ClearTimeStamps()		  { iTimeStamps = EFalse; }

  	inline TBool SackOk() const		  { return iSackOk; }
  	inline void SetSackOk()		      { iSackOk = ETrue; }
  	inline void ClearSackOk()		      { iSackOk = EFalse; }
  	inline void SuppressSack(TBool aBool=ETrue) { iSuppressSack = aBool; }
  	inline SequenceBlockQueue& SackBlocks()     { return iBlocks; }

  	inline TInt Unknown() const		  { return iUnknown; }
  	inline void ClearUnknown()		  { iUnknown = 0; }
  
  	// Query window scale option from TCP header options.
  	// Wscale == 1 means scale factor 1, i.e. shift count of 0 is used in TCP option.
  	// Wscale == 0 means that wscale option is not used in TCP header.
  	inline TUint WindowScale() const		{ return iWscale; }

  	inline void SetWindowScale(TUint8 aWscale)	{ iWscale = aWscale; }
  	
  	/**
  	If set, each option will be aligned to 32-bit longword boundaries with Nop padding.
  	By default the Nop padding is not applied.
  	
  	@param aAlignNop	ETrue if option alignment should be applied. 
	*/
  	inline void SetAlignOpt(TBool aAlignOpt) 
  		{ 
  		if(aAlignOpt) 
  			iFlags |= KTcpOptAlignFlag; 
  		else 
  			iFlags &= ~KTcpOptAlignFlag;
  		}

private:
  	friend class TInet6HeaderTCP;

  	IMPORT_C TBool ProcessOptions(const TUint8 *aPtr, TUint aLen);
  	IMPORT_C TUint OutputOptions(TUint8 *aPtr, TUint aMaxLen);
  	
  	void CheckOptAlignment(TUint8* aPtr, TUint& aI, TUint aNumBytes);
  	
  	inline TInt AlignedLength(TInt aLength) const
  	/**
  	Calculate the actual space requirement for option with given length.
  	
  	Takes optional 32-bit alignment into account.
  	*/
  		{ if (iFlags & KTcpOptAlignFlag) return (aLength + 3) & ~3; else return aLength; }

  	TInt	              iMSS;
  	TInt	              iUnknown;
  	TUint32             iTsVal;
  	TUint32             iTsEcr;
  	SequenceBlockQueue  iBlocks;
  	TBool	              iError;
  	TBool	              iTimeStamps;
  	TBool	              iSackOk;
  	TBool               iSuppressSack;
  	TUint				iWscale;	//< Window scale option [RFC 1323].
  	TUint32				iFlags;	//< ETrue if options are to be aligned.
	};



class TInet6HeaderTCP
/**  TCP segment header.
@verbatim
Extract from RFC-793

    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |          Source Port          |       Destination Port        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                        Sequence Number                        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    Acknowledgment Number                      |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Data |           |U|A|P|R|S|F|                               |
   | Offset| Reserved  |R|C|S|S|Y|I|            Window             |
   |       |           |G|K|H|T|N|N|                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |           Checksum            |         Urgent Pointer        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    Options                    |    Padding    |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                             data                              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
@endverbatim
@publishedAll
@released
*/
	{
public:
	//
	// Basic
	//
	inline static TInt MinHeaderLength() {return KTcpMinHeaderLength; }
	inline static TInt MaxHeaderLength() {return KTcpMaxHeaderLength; }
	inline TUint8 *EndPtr() {return i + HeaderLength();}
	//
	// Access, Get TCP field values from the packet
	//
	inline TUint SrcPort() const
		{
		return (i[0] << 8) + i[1];
		}
	inline TUint DstPort() const
		{
		return (i[2] << 8) + i[3];
		}
	inline TTcpSeqNum Sequence() const
		{
		return (i[4] << 24) | (i[5] << 16) | (i[6] << 8) | i[7];
		}
	inline TTcpSeqNum Acknowledgment() const
		{
		return (i[8] << 24) | (i[9] << 16) | (i[10] << 8) | i[11];
		}
	inline TInt HeaderLength() const
		{
		// Return TCP Header Length in bytes (including options and padding)
		return (i[12] >> 2) & (0xF << 2);
		}
	//
	// A testing method for each individual Control Bit is provided
	// (It remains to be seen whether this is useful or not). Note
	// also that the result of the AND is returned, not 0 and 1.
	//
	inline TInt FIN() const
		{
		return i[13] & KTcpCtlFIN;
		}
	inline TInt SYN() const
		{
		return i[13] & KTcpCtlSYN;
		}
	inline TInt RST() const
		{
		return i[13] & KTcpCtlRST;
		}
	inline TInt PSH() const
		{
		return i[13] & KTcpCtlPSH;
		}
	inline TInt ACK() const
		{
		return i[13] & KTcpCtlACK;
		}
	inline TInt URG() const
		{
		return i[13] & KTcpCtlURG;
		}

	// ECN Echo flag [RFC 3168].
	inline TInt ECE() const
		{
		return i[13] & KTcpCtlECE;
		}

	// ECN: Congestion Window Reduced [RFC 3168].
	inline TInt CWR() const
		{
		return i[13] & KTcpCtlCWR;
		}	
		
	// A method to access all of the above as is. Note that this
	// also returns the reserved unspecified bits. Value can be
	// non-zero, even if none of the above is set. However, it only
	// returns unspecified bits from the 13th byte, not any from 12th!
	inline TUint8 Control() const
		{
		return i[13];
		}
	//
	inline TUint Window() const
		{
		return (i[14] << 8) + i[15];
		}
	inline TUint Checksum() const
		{
		// Checksum is used in network byte order
		return *((TUint16 *)&i[16]);
		}
	inline TUint Urgent() const
		{
		return (i[18] << 8) + i[19];
		}
	inline TBool Options(TTcpOptions& aOptions) const
		{
		return aOptions.ProcessOptions(i + KTcpMinHeaderLength, HeaderLength() - KTcpMinHeaderLength);
		}
	//Backwards compatibility, mainly for IPRotor.
	inline TPtr8 Options() const
		{
		TInt len = HeaderLength() - 20;
		return TPtr8((TUint8 *)&i[20], len, len);
		}

	//
	// Build, Set TCP field value to the packet
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
	inline void SetSequence(TTcpSeqNum aSeq)
		{
		i[7] = (TUint8)aSeq.Uint32();
		i[6] = (TUint8)(aSeq.Uint32() >> 8);
		i[5] = (TUint8)(aSeq.Uint32() >> 16);
		i[4] = (TUint8)(aSeq.Uint32() >> 24);
		}
	inline void SetAcknowledgment(TTcpSeqNum aAck)
		{
		i[11] = (TUint8)aAck.Uint32();
		i[10] = (TUint8)(aAck.Uint32() >> 8);
		i[9] = (TUint8)(aAck.Uint32() >> 16);
		i[8] = (TUint8)(aAck.Uint32() >> 24);
		}
	inline void SetHeaderLength(TUint aLength)
		{
		// *NOTE* A very low level function, no sanity
		// checks on value, no rounding up. Value is just
		// truncated and shifted to the proper position
		// (all other bits of this byte should always
		// be zero!)
		i[12] = (TUint8)((aLength >> 2) <<4);
		}
	//
	// A set method for each individual Control Bit is provided
	// (It remains to be seen whether this is sensible or not).
	//
	inline void SetFIN()
		{
		i[13] |= KTcpCtlFIN;
		}
	inline void SetSYN()
		{
		i[13] |= KTcpCtlSYN;
		}
	inline void SetRST()
		{
		i[13] |= KTcpCtlRST;
		}
	inline void SetPSH()
		{
		i[13] |= KTcpCtlPSH;
		}
	inline void SetACK()
		{
		i[13] |= KTcpCtlACK;
		}
	inline void SetURG()
		{
		i[13] |= KTcpCtlURG;
		}
		
	// Set ECN Echo flag [RFC 3168].
	inline void SetECE()
		{
		i[13] |= KTcpCtlECE;
		}
		
	// Set ECN Congestion Window Reduced [RFC 3168].
	inline void SetCWR()
		{
		i[13] |= KTcpCtlCWR;
		}
		
	//
	// Note: does not touch the unused control bits at 12th byte!!!
	//
	inline void SetControl(TUint8 aFlags)
		{
		i[13] = aFlags;
		}
	//
	inline void SetWindow(TUint aWin)
		{
		i[15] = (TUint8)aWin;
		i[14] = (TUint8)(aWin >> 8);
		}
	inline void SetChecksum(TUint aSum)
		{
		// Checksum is used in network byte order
		*((TUint16 *)&i[16]) = (TUint16)aSum;
		}
	inline void SetUrgent(TUint aOff)
		{
		i[19] = (TUint8)aOff;
		i[18] = (TUint8)(aOff >> 8);
		}
	inline TInt SetOptions(TTcpOptions& aOptions)
		{
		TInt optLen = aOptions.OutputOptions(i + KTcpMinHeaderLength, KTcpMaxOptionLength);
		SetHeaderLength(KTcpMinHeaderLength + optLen);
		return optLen;
		}

protected:
	TUint8 i[KTcpMaxHeaderLength];
	};

/** @} */
#endif
