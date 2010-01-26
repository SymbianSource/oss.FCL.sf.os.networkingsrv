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
// tcpseq.h - TCP sequence numbers
// This module defines a TCP sequence number with mod32 arithmetic.
//



/**
 @file tcpseq.h
 @publishedAll
 @released
*/

#ifndef __TCPSEQ_H__
#define __TCPSEQ_H__

#include <e32std.h>


/**  TCP sequence number with mod-32 arithmetic.
@publishedAll
@released
*/
class TTcpSeqNum
	{
public:
  	inline TTcpSeqNum()					  			{ iSeq = 0; }
  	inline TTcpSeqNum(TUint32 aSeq)			  		{ iSeq = aSeq; }
  	inline TTcpSeqNum(const TTcpSeqNum& aSeq)		{ iSeq = aSeq.iSeq; }

  	inline TTcpSeqNum& operator=(TUint32 aSeq)		  { iSeq = aSeq; return *this; }
  	inline TTcpSeqNum& operator=(const TTcpSeqNum& aSeq)	  { iSeq = aSeq.iSeq; return *this; }
  	inline TTcpSeqNum& operator+=(TInt aOff)		  { iSeq += aOff; return *this; }
  	inline TTcpSeqNum& operator-=(TInt aOff)		  { iSeq -= aOff; return *this; }

  	inline TTcpSeqNum& operator++()			{ ++iSeq; return *this; }
  	inline TTcpSeqNum operator++(TInt)		{ return iSeq++; }
  	inline TTcpSeqNum& operator--()			{ --iSeq; return *this; }
  	inline TTcpSeqNum operator--(TInt)		{ return iSeq--; }

  	inline TBool operator==(const TTcpSeqNum& aSeq) const	{ return iSeq == aSeq.iSeq; }
  	inline TBool operator!=(const TTcpSeqNum& aSeq) const	{ return iSeq != aSeq.iSeq; }
  	inline TBool operator<(const TTcpSeqNum& aSeq) const	{ return (TInt32)(iSeq - aSeq.iSeq) < 0; }
  	inline TBool operator<=(const TTcpSeqNum& aSeq) const	{ return (TInt32)(iSeq - aSeq.iSeq) <= 0; }
  	inline TBool operator>(const TTcpSeqNum& aSeq) const	{ return (TInt32)(iSeq - aSeq.iSeq) > 0; }
  	inline TBool operator>=(const TTcpSeqNum& aSeq) const	{ return (TInt32)(iSeq - aSeq.iSeq) >= 0; }
  	inline TTcpSeqNum operator+(TInt aOff) const		  	{ return iSeq + aOff; }
  	inline TTcpSeqNum operator-(TInt aOff) const		  	{ return iSeq - aOff; }
  	inline TInt32 operator-(const TTcpSeqNum& aSeq) const   { return iSeq - aSeq.iSeq; }

  	//
  	// Automatic typecast can lead to ambiguous expressions and
  	// in the worst case to some very hard-to-track errors.
  	// We use the following explicit typecast instead. -ML
  	//
  	inline TUint32 Uint32() const				  { return iSeq; }
	//inline operator TUint32() const			  { return iSeq; }

  	// Methods for checking whether a sequence number is inside or outside a given window.
  	inline TBool Inside(TTcpSeqNum aSeqLo, TTcpSeqNum aSeqHi)
    	{ return (iSeq - aSeqLo.iSeq) <= (aSeqHi.iSeq - aSeqLo.iSeq); }

  	inline TBool Outside(TTcpSeqNum aSeqLo, TTcpSeqNum aSeqHi)
    	{ return (iSeq - aSeqLo.iSeq) > (aSeqHi.iSeq - aSeqLo.iSeq); }

private:
  	TUint32 iSeq;
	};

#endif
