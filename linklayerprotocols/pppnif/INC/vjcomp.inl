// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

#if !defined(__VJCOMP_INL_)
#define __VJCOMP_INL_

/**
Tests if the delta value is compressible into the 16 bit VJ delta format.
The aDelta value actually needs a 33 bit signed integer to hold the entire
possible range, but the twos complement math works out if it's just
stored as a 32 bit unsigned integer.

@param aDelta Delta value to test

@return ETrue if the value is compressible
*/
inline TBool CVJCompressor::IsDeltaCompressible(TUint32 aDelta) const
	{
	// This test is a faster equivalent to:
	//    (aDelta >= 0) && (aDelta <= 0xffff)
	// if aDelta were signed
	return !(aDelta & 0xffff0000);
	}


inline TUint8* CVJCompressor::GetTCPOpts(ThdrTCP* aTCPHeader) const
/**
Returns a pointer to the start of the TCP options (if there are any).

@param aTCPHeader TCP header

@return Start of the TCP options
*/
	{
	TUint8* aPtr = (TUint8*)aTCPHeader+KTCPHeaderSize;
	return aPtr;
	}



#endif // __VJCOMP_INL_

