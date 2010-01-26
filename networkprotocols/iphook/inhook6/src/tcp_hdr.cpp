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
// tcp_hdr.cpp - TCP protocol header structure
// TCP Options Header Processing.
//



/**
 @file tcp_hdr.cpp
*/

#include "inet6log.h"
#include "tcp_hdr.h"


EXPORT_C void TTcpOptions::Init()
	{
	iMSS          = -1;
	iUnknown      = 0;
	iError        = EFalse;
	iTimeStamps   = EFalse;
	iSackOk       = EFalse;
	iSuppressSack = EFalse;
	iFlags        = 0;
	iWscale	= 0;
	}


//
// Calculates the output length of TCP options.
//
// Note: When processing a received packet, this may differ
//       from the option block length in the received packet!
//
EXPORT_C TInt TTcpOptions::Length() const
	{
  	TInt len = 0;

  	// MSS option
  	if (iMSS >= 0)
    	len += 4;

  	// SACK permitted option
  	if (iSackOk)
    	len += AlignedLength(2);

  	// TimeStamps option
  	if (iTimeStamps)
    	len += AlignedLength(10);

  	// SACK option
  	if (!iSuppressSack && iBlocks.Count())
    	{
      	TInt blocks = (KTcpMaxOptionLength - len - 2) >> 3;
      	if (blocks > iBlocks.Count())
        	blocks = iBlocks.Count();
      	if (blocks)
        	len += AlignedLength(2 + (blocks << 3));
    	}
    
  	if (iWscale)
  		{
  		len += AlignedLength(3);
  		}

  	// Align to longword boundary
  	return ((len + 3) & ~3);
	}


//
// Process TCP options
//
EXPORT_C TBool TTcpOptions::ProcessOptions(const TUint8 *aPtr, TUint aLen)
	{
	const TUint8 *endp = aPtr + aLen;

  	while (aPtr < endp)
    	{
      	TUint8 kind = *aPtr++;

      	if (kind == KTcpOptEnd)
			return ETrue;

      	if (kind == KTcpOptNop)
			continue;

      	if (aPtr >= endp || *aPtr < 2 || aPtr + *aPtr - 2 > endp ||
           (kind < KTcpOptCount && (*aPtr < KTcpOptMinLen[kind] || *aPtr > KTcpOptMaxLen[kind])))
			{
	  		LOG(Log::Printf(_L("CProviderTCP6::ProcessOptions(): Invalid option length.\r\n"));)
	  		iError = ETrue;
	  		return EFalse;
			}

      	TUint8 len = *aPtr++;
      	const TUint8 *p;

      	switch (kind)
			{
		case KTcpOptMSS:
	  		iMSS  = *aPtr++ << 8;
	  		iMSS |= *aPtr++;
	  		break;

		case KTcpOptTimeStamps:
	  		if ((len - 2) & 7)
	    		{
	      		LOG(Log::Printf(_L("CProviderTCP6::ProcessOptions(): Invalid TimeStamps option length %d.\r\n"), len);)
	      		iError = ETrue;
	      		return EFalse;
	    		}
	  		iTsVal  = *aPtr++ << 24;
		  	iTsVal |= *aPtr++ << 16;
		  	iTsVal |= *aPtr++ <<  8;
		  	iTsVal |= *aPtr++;
		  	iTsEcr  = *aPtr++ << 24;
		  	iTsEcr |= *aPtr++ << 16;
		  	iTsEcr |= *aPtr++ <<  8;
		  	iTsEcr |= *aPtr++;
		  	iTimeStamps = ETrue;
		  	break;

		case KTcpOptSackOk:
	  		iSackOk = ETrue;
	  		break;

		case KTcpOptSack:
         	len -= 2;
	  		if (len & 7)
	    		{
	      		LOG(Log::Printf(_L("CProviderTCP6::ProcessOptions(): Invalid SACK option length %d.\r\n"), len);)
	      		iError = ETrue;
	      		return EFalse;
	    		}
          	for (p = aPtr + len - 8; p >= aPtr; p -= 16)
            	{
              	TTcpSeqNum left, right;
				left   = *p++ << 24;
				left  += *p++ << 16;
				left  += *p++ <<  8;
				left  += *p++;
				right  = *p++ << 24;
				right += *p++ << 16;
				right += *p++ <<  8;
				right += *p++;
				iBlocks.AddUnordered(left, right);
	    		}
          	aPtr += len;
	  		break;

		case KTcpOptWScale:
	  		iWscale = (*aPtr++) + 1;
	  		break;
	  
		default:
	  		LOG(Log::Printf(_L("CProviderTCP6::ProcessOptions(): Unknown TCP option %d, length = %d.\r\n"), kind, len);)
	  		aPtr += (len - 2);
	  		iUnknown += len;
	  		break;
			}
    	}
  	return ETrue;
	}

//
// Output TCP options
//
// Note: We currently do not align the options. Byte efficiency is
// of more import in this implementation than quick memory accesses.
//
EXPORT_C TUint TTcpOptions::OutputOptions(TUint8 *aPtr, TUint aMaxLen)
	{
	TUint i = 0;

  	// MSS option
  	if (iMSS >= 0)
    	{
      	ASSERT(i + 4 <= aMaxLen);
      	aPtr[0] = KTcpOptMSS;
      	aPtr[1] = 4;
      	aPtr[2] = (TUint8)(iMSS >> 8);
      	aPtr[3] = (TUint8) iMSS;
      	i += 4;
    	}

  	// Timestamps option
  	if (iTimeStamps)
    	{
      	ASSERT(i + 10 <= aMaxLen);
		CheckOptAlignment(aPtr, i, 2);
		aPtr[i++] = KTcpOptTimeStamps;
		aPtr[i++] = 10;
		aPtr[i++] = (TUint8)(iTsVal >> 24);
		aPtr[i++] = (TUint8)(iTsVal >> 16);
		aPtr[i++] = (TUint8)(iTsVal >>  8);
		aPtr[i++] = (TUint8) iTsVal;
		aPtr[i++] = (TUint8)(iTsEcr >> 24);
		aPtr[i++] = (TUint8)(iTsEcr >> 16);
		aPtr[i++] = (TUint8)(iTsEcr >>  8);
		aPtr[i++] = (TUint8) iTsEcr;
    	}

	// Window scale option
	if (iWscale)
  		{
		ASSERT(i + 3 <= aMaxLen);
		CheckOptAlignment(aPtr, i, 1);
		aPtr[i++] = KTcpOptWScale;
		aPtr[i++] = 3;
		aPtr[i++] = (TUint8)(iWscale - 1);
    	}

  	// SACK permitted option
  	if (iSackOk)
    	{
      	ASSERT(i + 2 <= aMaxLen);
      	CheckOptAlignment(aPtr, i, 2);
      	aPtr[i++] = KTcpOptSackOk;
      	aPtr[i++] = 2;
    	}

  	// SACK option
  	if (!iSuppressSack && iBlocks.Count())
    	{
      	ASSERT(i + 10 <= aMaxLen);

		CheckOptAlignment(aPtr, i, 2);
      	TInt optp = i;
      	SequenceBlockQueueIter iter(iBlocks);
      	SequenceBlock *block;

      	aPtr[i] = KTcpOptSack;
      	i += 2;

      	//
      	// iLastBlock points to the contiguous block
      	// most recently added to the queue. This must
      	// be the first SACK block in the option data.
      	//
      	while (block = iter++, block != NULL && i + 8 <= aMaxLen)
        	{
	  		aPtr[i++] = (TUint8)(block->iLeft.Uint32()  >> 24);
	  		aPtr[i++] = (TUint8)(block->iLeft.Uint32()  >> 16);
	  		aPtr[i++] = (TUint8)(block->iLeft.Uint32()  >>  8);
	  		aPtr[i++] = (TUint8) block->iLeft.Uint32();
	  		aPtr[i++] = (TUint8)(block->iRight.Uint32() >> 24);
	  		aPtr[i++] = (TUint8)(block->iRight.Uint32() >> 16);
	  		aPtr[i++] = (TUint8)(block->iRight.Uint32() >>  8);
	  		aPtr[i++] = (TUint8) block->iRight.Uint32();
        	}

      	aPtr[optp + 1] = (TUint8)(i - optp);
    	}

  	// Padding
  	while ((i & 3))
    	aPtr[i++] = KTcpOptEnd;

  	//LOG(Log::Printf(_L("TTcpOptions::OutputOptions(): Options length = %d\r\n"), i));

  	return i;
	}


/**
Align options with NOP if option alignment is enabled.

@param aPtr			option data area.
@param aI			index to the location being processed.
@param aNumBytes	Number of bytes that need to be used for padding.
*/
void TTcpOptions::CheckOptAlignment(TUint8* aPtr, TUint& aI, TUint aNumBytes)
	{
	if (iFlags & KTcpOptAlignFlag)
		{
		while (aNumBytes--)
			aPtr[aI++] = KTcpOptNop;
		}
	}
