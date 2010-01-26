// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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

/**
 @file
 @brief Source file for the implementation of the MD4 Message-Digest Algorithm - RFC 1320.
 @internalComponent 
*/

#include "MD4.H"

// This implementation is fully derived (copied) from the former MD4
// implementation provided by Symbian PPP in the release 7.0s of
// Symbian OS.  This is a provisional solution until the Symbian
// Security subsystem components will provide a MD4 implementation.

#define MD4_S11 3
#define MD4_S12 7
#define MD4_S13 11
#define MD4_S14 19
#define MD4_S21 3
#define MD4_S22 5
#define MD4_S23 9
#define MD4_S24 13
#define MD4_S31 3
#define MD4_S32 9
#define MD4_S33 11
#define MD4_S34 15

static const TUint8 Padding[64] =
	{
	0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

// F, G, H and I are basic MD4 functions.
#define MD4_F(x, y, z) ((x & y) | ((~x) & z))
#define MD4_G(x, y, z) ((x & y) | (x & z) | (y & z))
#define MD4_H(x, y, z) (x ^ y ^ z)

// _ROL rotates x left n bits.
#define _ROL(x, n) (((x) << (n)) | ((x) >> (32-(n))))

// FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4.
// Rotation is separate from addition to prevent recomputation.
#define MD4_FF(a, b, c, d, x, s ) { \
 a += MD4_F(b, c, d) + x; \
 a = _ROL (a, s); \
 }
#define MD4_GG(a, b, c, d, x, s) { \
 a += MD4_G(b, c, d) + x + 0x5a827999; \
 a = _ROL (a, s); \
 }
#define MD4_HH(a, b, c, d, x, s) { \
 a += MD4_H(b, c, d) + x + 0x6ed9eba1; \
 a = _ROL (a, s); \
  }

const TInt KMdOutputSize = 16;

CMd4::CMd4()
	{
	iCount[0] = 0;
	iCount[1] = 0;
	// Load magic initialization constants.
	iState[0] = 0x67452301;
	iState[1] = 0xefcdab89;
	iState[2] = 0x98badcfe;
	iState[3] = 0x10325476;
	}

CMd4* CMd4::NewL()
	{
	return new(ELeave) CMd4;
	}

// moved from the former CDigestBase
TInt CMd4::Input(const TDesC8& aDes)
	{
	Update(aDes.Ptr(), aDes.Length());
	return KErrNone;
	}

// moved from the former CDigestBase
TInt CMd4::Output(TDes8& aDes)
	{
	TUint8* digest = (TUint8*)aDes.Ptr();
	if (aDes.MaxLength()<16)
		return KErrOverflow;
	
	Final(digest);

	aDes.SetLength(16);
	return KErrNone;
	}

// moved from the former CDigestBase
TInt CMd4::OutputSize() const
//
// Return size of output
//
	{
	return KMdOutputSize;
	}

// moved from the former CDigestBase
void CMd4::Update(const TUint8* aInput, TInt aLen)
//
// MD5 block update operation. Continues an MD5 message-digest
// operation, processing another message block, and updating the
// context.
//
	{
	TInt i;
	TInt index;
	TInt partlen;

	/* Compute number of bytes mod 64 */
	index = (TUint)((iCount[0] >> 3) & 0x3F);

	/* Update number of bits */
	iCount[0] += (aLen<<3);
	
	if (iCount[0] < (TUint)(aLen<<3))
		iCount[1]++;
	iCount[1] += (aLen>>29);
	partlen = 64 - index;

	// Transform as many times as possible.
	if (aLen >= partlen)
		{
		Mem::Copy(&iBuffer[index], aInput, partlen);
		Transform(iBuffer);
		for (i = partlen; i+63 < aLen; i += 64)
		Transform (&aInput[i]);
		index = 0;
		}
	else
		i = 0;

	/* Buffer remaining input */
	Mem::Copy(&iBuffer[index], &aInput[i], aLen-i);
}

// moved from the former CDigestBase
void CMd4::Final(TUint8* aDigest /*[16]*/)
//
// MD5 finalization. Ends an MD5 message-digest operation, writing the
// the message digest and zeroizing the context.
//
	{
	TUint8 bits[8];
	TInt index, padlen;

	/* Save number of bits */
	Encode(bits, (TUint32*)&iCount[0], 8);

	// Pad out to 56 mod 64.
	index = (TUint)((iCount[0] >> 3) & 0x3f);
	padlen = (index < 56) ? (56 - index) : (120 - index);
	Update(Padding, padlen);

	// Append length (before padding) */
	Update(bits, 8);
	// Store state in digest */
	Encode(aDigest, iState, 16);

	// Zero sensitive information.
	Mem::FillZ(iCount, sizeof(iCount));
	Mem::FillZ(iState, sizeof(iState));
	Mem::FillZ(iBuffer, sizeof(iBuffer));
}

// moved from the former CDigestBase
void CMd4::Encode(TUint8* aOutput, const TUint32* aInput, TInt aLen)
//
// Encodes input (UINT4) into output (unsigned char).
// Assumes len is a multiple of 4.
//
	{
	TInt i;
	TInt j;

	for (i=0, j=0; j<aLen; i++, j+=4)
		{
		aOutput[j] = (TUint8)(aInput[i] & 0xff);
		aOutput[j+1] = (TUint8)((aInput[i] >> 8) & 0xff);
		aOutput[j+2] = (TUint8)((aInput[i] >> 16) & 0xff);
		aOutput[j+3] = (TUint8)((aInput[i] >> 24) & 0xff);
		}
	}

// moved from the former CDigestBase
void CMd4::Decode(TUint32* aOutput, const TUint8* aInput, TInt aLen)
//
// Decodes input (unsigned char) into output (UINT4).
// Assumes len is a multiple of 4.
//
	{
	TInt i;
	TInt j;

	for (i=0, j=0; j<aLen; i++, j+=4)
		{
		aOutput[i] = ((TUint)aInput[j]) 
			| (((TUint)aInput[j+1]) << 8) 
			| (((TUint)aInput[j+2]) << 16) 
			| (((TUint)aInput[j+3]) << 24);
		}
	}

void CMd4::Transform (const TUint8* aBlock /*[64]*/)
//
// MD4 basic transformation.
// Transforms state based on block.
//	
	{
	TUint a = iState[0];
	TUint b = iState[1];
	TUint c = iState[2];
	TUint d = iState[3];
	TUint x[16] = {0,};

	Decode((TUint32*)&x[0], aBlock, 64);

	/* Round 1 */
	MD4_FF (a, b, c, d, x[ 0], MD4_S11); /* 1 */
	MD4_FF (d, a, b, c, x[ 1], MD4_S12); /* 2 */
	MD4_FF (c, d, a, b, x[ 2], MD4_S13); /* 3 */
	MD4_FF (b, c, d, a, x[ 3], MD4_S14); /* 4 */
	MD4_FF (a, b, c, d, x[ 4], MD4_S11); /* 5 */
	MD4_FF (d, a, b, c, x[ 5], MD4_S12); /* 6 */
	MD4_FF (c, d, a, b, x[ 6], MD4_S13); /* 7 */
	MD4_FF (b, c, d, a, x[ 7], MD4_S14); /* 8 */
	MD4_FF (a, b, c, d, x[ 8], MD4_S11); /* 9 */
	MD4_FF (d, a, b, c, x[ 9], MD4_S12); /* 10 */
	MD4_FF (c, d, a, b, x[10], MD4_S13); /* 11 */
	MD4_FF (b, c, d, a, x[11], MD4_S14); /* 12 */
	MD4_FF (a, b, c, d, x[12], MD4_S11); /* 13 */
	MD4_FF (d, a, b, c, x[13], MD4_S12); /* 14 */
	MD4_FF (c, d, a, b, x[14], MD4_S13); /* 15 */
	MD4_FF (b, c, d, a, x[15], MD4_S14); /* 16 */

	/* Round 2 */
	MD4_GG (a, b, c, d, x[ 0], MD4_S21); /* 17 */
	MD4_GG (d, a, b, c, x[ 4], MD4_S22); /* 18 */
	MD4_GG (c, d, a, b, x[ 8], MD4_S23); /* 19 */
	MD4_GG (b, c, d, a, x[12], MD4_S24); /* 20 */
	MD4_GG (a, b, c, d, x[ 1], MD4_S21); /* 21 */
	MD4_GG (d, a, b, c, x[ 5], MD4_S22); /* 22 */
	MD4_GG (c, d, a, b, x[ 9], MD4_S23); /* 23 */
	MD4_GG (b, c, d, a, x[13], MD4_S24); /* 24 */
	MD4_GG (a, b, c, d, x[ 2], MD4_S21); /* 25 */
	MD4_GG (d, a, b, c, x[ 6], MD4_S22); /* 26 */
	MD4_GG (c, d, a, b, x[10], MD4_S23); /* 27 */
	MD4_GG (b, c, d, a, x[14], MD4_S24); /* 28 */
	MD4_GG (a, b, c, d, x[ 3], MD4_S21); /* 29 */
	MD4_GG (d, a, b, c, x[ 7], MD4_S22); /* 30 */
	MD4_GG (c, d, a, b, x[11], MD4_S23); /* 31 */
	MD4_GG (b, c, d, a, x[15], MD4_S24); /* 32 */

	/* Round 3 */
	MD4_HH (a, b, c, d, x[ 0], MD4_S31); /* 33 */
	MD4_HH (d, a, b, c, x[ 8], MD4_S32); /* 34 */
	MD4_HH (c, d, a, b, x[ 4], MD4_S33); /* 35 */
	MD4_HH (b, c, d, a, x[12], MD4_S34); /* 36 */
	MD4_HH (a, b, c, d, x[ 2], MD4_S31); /* 37 */
	MD4_HH (d, a, b, c, x[10], MD4_S32); /* 38 */
	MD4_HH (c, d, a, b, x[ 6], MD4_S33); /* 39 */
	MD4_HH (b, c, d, a, x[14], MD4_S34); /* 40 */
	MD4_HH (a, b, c, d, x[ 1], MD4_S31); /* 41 */
	MD4_HH (d, a, b, c, x[ 9], MD4_S32); /* 42 */
	MD4_HH (c, d, a, b, x[ 5], MD4_S33); /* 43 */
	MD4_HH (b, c, d, a, x[13], MD4_S34); /* 44 */
	MD4_HH (a, b, c, d, x[ 3], MD4_S31); /* 45 */
	MD4_HH (d, a, b, c, x[11], MD4_S32); /* 46 */
	MD4_HH (c, d, a, b, x[ 7], MD4_S33); /* 47 */
	MD4_HH (b, c, d, a, x[15], MD4_S34); /* 48 */


	iState[0] += a;
	iState[1] += b;
	iState[2] += c;
	iState[3] += d;

	// Zeroize sensitive information.
	Mem::FillZ(x, sizeof(x));
	}
