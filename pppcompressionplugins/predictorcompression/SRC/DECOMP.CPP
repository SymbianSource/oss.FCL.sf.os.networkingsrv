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
// Predictor-1 PPP decompressor (RFC 1978)
// 
//

/**
 @file DECOMP.CPP
*/

#include "PREDCOMP.H"

CPppDeCompressor* CPredCompFactory::NewPppDeCompressorL(CPppCcp* aCcp, TInt aMaxFrameLength,const TUint8* aMode)
/**
Constructor, Allocating memory to class CPredDeCompressor.

@return DeComp, a pointer to class CPredDeCompressor.
*/
	{
	aMode = aMode;
	CPredDeCompressor* DeComp = new (ELeave) CPredDeCompressor();
	CleanupStack::PushL(TCleanupItem(CNifFactory::Cleanup,DeComp));
	DeComp->ConstructL(this, aCcp, aMaxFrameLength);
	CleanupStack::Pop();

	return DeComp;
	}

void CPredDeCompressor::ResetDecompressor(TInt /*aLength*/, RMBufChain& /*aPacket*/)
/**
This function is used to reset the Decompressor.
*/
	{
	// Initializes the guess table
	Reset();
	iReConfiguring = FALSE;
//	__LOGTEXT_DEBUG(_L8("ResetDecompressor\r\n"));
	}


void CPredDeCompressor::ConstructL(CPredCompFactory* aFactory, CPppCcp* aCcp, TInt aMaxFrameLength)
/**
2nd Phase Construction
*/
	{
	// Initializes the guess table
	Reset();
//	__LOGTEXT_DEBUG(_L8("CPredDeCompressor::ConstructL\r\n"));
	//
	// Allow space for Protocol header (up to 2 bytes)
	//
	iCompressedBuffer = HBufC8::NewMaxL(aMaxFrameLength + 2);
	iDecompressBuffer = HBufC8::NewMaxL(aMaxFrameLength + 2);
	iReConfiguring = FALSE;
	
	iCcp = aCcp;
	iFactory = aFactory;
	iFactory->Open();
	}

TBool CPredDeCompressor::Decompress(RMBufQ& aBufferQ)
/**
@return 0 if we have not been destructed whilst frames are still arriving else Reconstituted buffer into a chain
*/
	{
	// In case we have not been destructed whilst frames are still arriving
	if(iReConfiguring)
		return(FALSE);
	// Remove the first buffer in the chain so we can extract the
	// uncompressed length see RFC 1978 3.2
	RMBuf *buf = aBufferQ.Remove();
	// Check the pointer is valid, because there is small probability to be it NULL
	// And make sure the RFC 1978 header bytes are there
	if (buf == NULL || buf->Length() < 5)
		{
		aBufferQ.Free();
		return(FALSE);
		}
	// Get the uncompressed length & compressed/uncompressed bit
	TUint16 uncompressedLength = BigEndian::Get16(buf->Ptr());	
	// Make sure the top bit is cleared before we do the CRC
	*buf->Ptr() &= ~0x80;
	// Calculate the 16bit CRC
	TPppFcs16 fcs;
	// CRC the 2 length bytes
	fcs.Calc(buf->Ptr(),buf->Ptr()+2);
	// effectively remove them by adjusting the offset of the start of the buffer
	buf->AdjustStart(2);
	aBufferQ.Prepend(buf);

	// Set some convenient references to the class buffers
	TPtr8 src = iCompressedBuffer->Des();
	TPtr8 dest = iDecompressBuffer->Des();
	// Copy method needs max buffer size
	src.SetMax();
	// Remember, src might be changed to point directly into the MBuf on return
	// Overall length should include the 16 bit CRC
	TInt overallLength = FlattenBuf(src,aBufferQ);
	// Set the length
	src.SetLength(overallLength);
	// Remove the CRC from the end and deduct 2 from the buffer
	TUint16 crc = (TUint16) ((src[src.Length()-1] << 8) | src[src.Length()-2]);
	src.SetLength(src.Length()-2);
	// Prepare for decompression
	dest.SetLength(0);
	// Check the top bit to see if this frame is compressed
	if(uncompressedLength & 0x8000)
		{
		// Decompress using RFC 1878 algorithm
		DecompressRFC1978(src,dest);
		}
	else
		{
		// not compressed
		dest = src;
		// Run compressor over the source to keep the guess table in synch
		// with the server.
		// NULL parameter as we aren't interested in the output
		CompressRFC1978(src,NULL);
		}
	// The length set by the decompressor should match the header length
	if(dest.Length() != (uncompressedLength & ~0x8000))
		{
		// Reset the guess table
//		__LOGTEXT_ALWAYS(_L8("Length Mismatch\r\n"));
		// Call into PPP ccp
		// Causes Configure Request to be sent and we get destructed
		iCcp->ReConfigLink();
		// Set this flag so we throw frames until we are destructed
		iReConfiguring = TRUE;
		// Make sure to free any MBuf still in aBufferQ. This can occur
		// if FlattenBuf avoided copying it into the descriptor.
		aBufferQ.Free();
		return(FALSE);
		}
	// calculate the fcs on the uncompressed data
	fcs.Calc(dest.Ptr(),dest.Ptr()+dest.Length());
	if(fcs.Fcs() != crc)
		{
		// Call into PPP ccp
		// Causes Configure Request to be sent and we get destructed
		iCcp->ReConfigLink();
		iReConfiguring = TRUE;
//		__LOGTEXT3_ALWAYS(_L8("Fcs Fail calcFcs = %X crc = %X\r\n"),calcFcs,crc);
//		__LOGTEXT3_ALWAYS(_L8("Fcs Fail orig length = %d overall length = %d\r\n"),(uncompressedLength & ~0x8000),overallLength);
		// Make sure to free any MBuf still in aBufferQ. This can occur
		// if FlattenBuf avoided copying it into the descriptor.
		aBufferQ.Free();
		return(FALSE);
		}
	// Re constitute the buffer into a chain
	return(CopyNewFrameToChain(dest,aBufferQ));
	}

CPredDeCompressor::CPredDeCompressor()
/**
Constructor
*/
	{
	return;
	}

CPredDeCompressor::~CPredDeCompressor()
/**
Destructor
*/
	{
	if(iFactory)
		iFactory->Close();
	return;
	}

/**
Copies the contents of aBufferQ into the given descriptor. If the entire packet
fits into a single MBuf, aPtr is changed to point directly into the MBuf and
it is not freed from aBufferQ. Otherwise, all MBufs are freed and aBufferQ
returns empty.

@param aPtr Descriptor pointing to a large enough descriptor to hold the data.
  On return, the descriptor may be changed to point directly into the MBuf.
@param aBufferQ Input data

@return Length of data in the buffer
*/
TUint CPredDeCompressor::FlattenBuf(TPtr8& aPtr, RMBufQ& aBufferQ)
	{
	if (aBufferQ.First() == aBufferQ.Last())
		{
		// The BufferQ only holds a single MBuf; just point the descriptor
		// directly into that MBuf, bypassing the copy step.  Remember to free
		// that MBuf before finishing the decompress.
		aPtr.Set(aBufferQ.First()->Ptr(), aBufferQ.First()->Length(), aBufferQ.First()->Length());
		return aPtr.Length();
	}
	return CopyFrameIntoFlatBuf(aPtr, aBufferQ);
	}

/**
Copies the data in the MBuf into the given descriptor.
	
@param aPtr Descriptor large enough to hold the data
@param aBufferQ Input data
	
@return Length of data in the output buffer, or 0 on error
*/
TUint CPredDeCompressor::CopyFrameIntoFlatBuf(const TPtr8& aPtr, RMBufQ& aBufferQ)
	{
	const TUint	maxLength = aPtr.MaxLength();
	TUint8*		ptr = const_cast<TUint8*>(aPtr.Ptr());
	TUint		Offset = 0;

	RMBuf* Temp;
	while ((Temp = aBufferQ.Remove()) != NULL)
		{
		// Make sure we don't copy past the end of the buffer
		if ((Offset + Temp->Length()) > maxLength)
			{
			LOG(PredLog::Printf(_L("Input packet too long\r\n")));
			Offset = 0;	// This is the only indication of an error
			break;
			}
		Mem::Copy(ptr+Offset, Temp->Ptr(), Temp->Length());
		Offset += Temp->Length();
		Temp->Free();
		}

	// Make sure to free the buffers in case we exited the loop early
	aBufferQ.Free();
	return Offset;
	}


/**
Allocate a new chain and copy the decompressed frame into it

@param aSrc Output descriptor
@param aBufferQ Input data

@return TRUE on an error
*/
TBool CPredDeCompressor::CopyNewFrameToChain(TDesC8& aSrc,RMBufQ& aBufferQ)
	{
	TBool RetCode=FALSE;
	RMBufChain	NewChain;

	TRAPD(Ret, NewChain.AllocL(aSrc.Length()));
	if (Ret == KErrNone)
		{
		RetCode = TRUE;
		NewChain.CopyIn(aSrc);

		// Make sure to free any MBuf still in aBufferQ. This can occur
		// if FlattenBuf avoided copying it into the descriptor.
		aBufferQ.Free();
		aBufferQ.Assign(NewChain);
		}

	return RetCode;
	}


void TRFC1978Table::DecompressRFC1978(const TDesC8& aSrc,TDes8& aDest)
/**
Decompression logic from RFC 1978
Modified to use descriptors
We only support predictor type 1 so the "final" code has been removed as suggested
in the RFC

@param aSrc Data to compress
@param aDest Compressed output buffer
*/
	{
	TInt srcIndex = 0;
	TInt len = aSrc.Length();
	while(len >= 9)
		{
		TUint8 flags = aSrc[srcIndex++];
		TUint8 bitMask;
		TInt j;
		for(bitMask = 1,j = 0;j < 8 ;j++,bitMask <<= 1)
			{
			TUint8 dest;
			if(flags & bitMask)
				{
				dest = iGuessTable[iHash];
				}
			else
				{
				dest = aSrc[srcIndex++];
				iGuessTable[iHash] = dest;
				len--;
				}
			aDest.Append(dest);
			Hash(dest);
			}
		len--;
		}

	while(len)
		{
		TUint8 flags = aSrc[srcIndex++];
		TUint8 bitMask;
		TInt j;

		len--;
		for(bitMask = 1,j = 0;j < 8 ;j++,bitMask <<= 1)
			{
			TUint8 dest;
			if(flags & bitMask)
				{
				dest = iGuessTable[iHash];
				}
			else
				{
				if(!len)
					break;

				dest = aSrc[srcIndex++];
				iGuessTable[iHash] = dest;
				len--;
				}
			aDest.Append(dest);
			Hash(dest);
			}
		}
	}
