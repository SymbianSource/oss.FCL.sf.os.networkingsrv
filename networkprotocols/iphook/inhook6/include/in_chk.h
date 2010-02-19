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
// in_chk.h - IPv6/IPv4 checksum module
// Raw Internet Checksum computation from RMBufChain.
//



/**
 @file in_chk.h
 @publishedAll
 @released
*/

#ifndef __IN_CHK_H__
#define __IN_CHK_H__

#include "in_pkt.h"
class RMBufChain;
class RMBufPktInfo;

/** Checksum calculation.

@publishedAll
@released
*/
class TChecksum {
public:
        // TChecksum can be initialized with an old inverted checksum
        TChecksum(TUint16 aSum = ~0) : iSum((TUint16)~aSum) {}
	inline void Init(TUint16 aSum = ~0);

	inline TUint32 Sum();
        inline TUint32 Sum32();
        inline void Fold();
        inline void Reverse();

        // Complex Add methods
        IMPORT_C void Add(RMBufChain &aPacket, TInt aOffset);
        IMPORT_C void Add(RMBufChain &aPacket, const RMBufPktInfo *aInfo, TInt aOffset);

        // Inline Add methods
        inline void Add(const TUint16 *aPtr, TInt aLength);
        inline void AddHi(TUint8 aByte);
        inline void AddLo(TUint8 aByte);
        inline void Add(TUint16 aHalfWord);
        inline void Add(TUint32 aWord);
        inline void AddH(TUint16 aHalfWord);
        inline void AddH(TUint32 aWord);

        // Static methods
        static inline TUint32 Fold(TUint32 aSum);
        static inline TUint16 ComplementedFold(TUint32 aSum);
        IMPORT_C static TUint32 Calculate(const TUint16 *aPtr, TInt aLength);

private:
	TUint32	iSum;   // 32-bit sum in network byte order
};


/**
// TInet6Checksum Template.
//
//		This template class provides utilitlies to compute and check
//		IPv6 Upper Layer Checksums
//		These are not merged with the TInet6Packet class, because that
//		class is intended (and is used) for all headers, not just upper
//		layers.
//
//	The template parameter (Header class) must
//		- have Checksum() method
//		- have SetChecksum() method
//		- the checkum in header must be aligned to 16 bit word
//
// @publishedAll
// @released
*/
template <class T>
class TInet6Checksum : public TInet6Packet<T>
	{
public:
	TInet6Checksum() {}
	TInet6Checksum(RMBufChain &aPacket) : TInet6Packet<T>(aPacket) {}
	TInet6Checksum(RMBufChain &aPacket, TInt aOffset) : TInet6Packet<T>(aPacket, aOffset) {}
	void ComputeChecksum(RMBufChain &aPacket, const RMBufPktInfo *aInfo, TInt aOffset = 0);
	TBool VerifyChecksum(RMBufChain &aPacket, const RMBufPktInfo *aInfo, TInt aOffset = 0);
	void ComputeChecksum();
	TBool VerifyChecksum();
	};

//
// Inline methods for TChecksum
//

inline void TChecksum::Init(TUint16 aSum)
{
        iSum = ~aSum & 0xffff;
}

inline void TChecksum::Add(const TUint16 *aPtr, TInt aLength)
{
	iSum += Calculate(aPtr, aLength);
}

inline void TChecksum::AddHi(TUint8 aByte)
{
        iSum += (aByte << 8);
}

inline void TChecksum::AddLo(TUint8 aByte)
{
        iSum += aByte;
}

// Add halfword in network byte order
inline void TChecksum::Add(TUint16 aHalfWord)
{
        iSum += aHalfWord;
}

// Add word in network byte order
inline void TChecksum::Add(TUint32 aWord)
{
	iSum += aWord >> 16;
        iSum += aWord & 0xffff;
}

// Add halfword in host byte order
inline void TChecksum::AddH(TUint16 aHalfWord)
{
		__DEBUG_ONLY (const TInt one = 1;)
		ASSERT(*(TUint8*)&one); //check that we are little-endian

		iSum += ((aHalfWord << 8) | (aHalfWord >> 8)) & 0xffff;
}

// Add word in host byte order
inline void TChecksum::AddH(TUint32 aWord)
{
		__DEBUG_ONLY (const TInt one = 1;)
		ASSERT(*(TUint8*)&one); //check that we are little-endian

        iSum += (aWord >> 24);          // add octet  3
        iSum += (aWord >> 8) & 0xffff,  // add octets 2-1
        iSum += (aWord << 8) & 0xff00;  // add octet  0
}

// Fold 32-bit sum into 16 bits
inline void TChecksum::Fold()
{
        iSum = Fold(iSum);
}

// Return 16-bit inverted checksum
inline TUint32 TChecksum::Sum()
{
        return Fold(iSum) ^ 0xffff;
}

// Return 32-bit intermediate sum
inline TUint32 TChecksum::Sum32()
{
        return iSum;
}

// Reverse checksum direction. Can be used to back out partial sums.
inline void TChecksum::Reverse()
{
        iSum = Sum();
}

inline TUint32 TChecksum::Fold(TUint32 aSum)
{
	aSum = (aSum >> 16) + (aSum & 0xffff);  // Max: 0x0001fffe
	aSum += (aSum >> 16);                   // Max: 0x0001ffff
        return aSum & 0xffff;                   // Truncate to 16 bits
}

inline TUint16 TChecksum::ComplementedFold(TUint32 aSum)
{
        return (TUint16)~Fold(aSum);
}

template <class T>
void TInet6Checksum<T>::ComputeChecksum(RMBufChain &aPacket, const RMBufPktInfo *aInfo, TInt aOffset)
	{
        TChecksum sum;
	this->iHdr->SetChecksum(0);
        sum.Add(aPacket, aInfo, aOffset);
	this->iHdr->SetChecksum(sum.Sum());
	}

template <class T>
TBool TInet6Checksum<T>::VerifyChecksum(RMBufChain &aPacket, const RMBufPktInfo *aInfo, TInt aOffset)
	{
        TChecksum sum;
        sum.Add(aPacket, aInfo, aOffset);
        return !sum.Sum();
	}
//
// The following are mainly for the IPv4 Header checksum
//
template <class T>
void TInet6Checksum<T>::ComputeChecksum()
	{
        TChecksum sum;
	this->iHdr->SetChecksum(0);
        sum.Add((TUint16 *)(this->iHdr), this->iHdr->HeaderLength());
	this->iHdr->SetChecksum(sum.Sum());
	}

template <class T>
TBool TInet6Checksum<T>::VerifyChecksum()
	{
        TChecksum sum;
        sum.Add((TUint16*)(this->iHdr), this->iHdr->HeaderLength());
        return !sum.Sum();
	}

#endif
