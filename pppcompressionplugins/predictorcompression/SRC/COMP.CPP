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
 @file COMP.CPP
*/

#include "PREDCOMP.H"


// Force export of non-mangled name
extern "C"
    {
/**
@internalComponent
@return an object of CPredCompFactory.
*/
EXPORT_C CPppCompFactory* NewPppCompFactoryL(void)
	{
	LOG(PredLog::Printf(_L("Creating a compressor\r\n")));
	return new (ELeave) CPredCompFactory;
	}

	}

CPredCompFactory::CPredCompFactory()
/**
Constructor
*/
	{
	__DECLARE_NAME(_S("CPredCompFactory"));
	}

void CPredCompFactory::InstallL()
/**
Performs a Compressor initialization.
*/
	{
	}

CPppCompressor* CPredCompFactory::NewPppCompressorL(CPppCcp* aCcp,TInt aMaxFrameLength,const TUint8* /*aMode*/)
/**
@return a poiner to class CPredCompressor.
*/
	{
	CPredCompressor* Comp = new (ELeave) CPredCompressor();
	CleanupStack::PushL(TCleanupItem(CNifFactory::Cleanup,Comp));
	Comp->ConstructL(this, aCcp, aMaxFrameLength);
	CleanupStack::Pop();
//	__LOGTEXT_DEBUG(_L8("New Predictor Compressor\n"));
	return Comp;
	}

void CPredCompressor::ConstructL(CPredCompFactory* aFactory, CPppCcp* aCcp, TInt aMaxFrameLength)
/**
2nd Phase Construction
*/
	{
	// Initializes the guess table
	Reset();
//	__LOGTEXT_DEBUG(_L8("CPredCompressor::ConstructL\r\n"));
	//
	// Set up a contiguous block for data copied from an input chain of buffers
	// + 2 bytes for protocol ID which forms part of the compressed data
	iFrameBuffer = HBufC8::NewL(aMaxFrameLength+2);
	// Set up a contiguous block for output from the compressor method
	// Allow space for :- 2 Byte Length + 2 Byte protocol ID + CRC + possible expansion after compression
	// Compressed data can be bigger than the original frame ie Flag byte
	// every 8 bytes and little or no compression possible.
	iCompressedBuffer = HBufC8::NewL(aMaxFrameLength + 6 + ((aMaxFrameLength+2)/8) + 1);

	iCcp = aCcp;
	iFactory = aFactory;
	iFactory->Open();
	return;
	}


TBool CPredCompressor::ResetCompressor(TInt /*aLength*/, RMBufChain& /*aPacket*/)
	{
	LOG(PredLog::Printf(_L("ResetCompressor\n"));)
	// Initializes the guess table
	Reset();
	return TRUE;
	}

TPppCompressReturnValue CPredCompressor::Compress(RMBufChain& aPacket, TUint16 aPppId)
	{
	TPppCompressReturnValue retCode=EPppCompressedNotOK;
	TInt16	originalLength;

	// Set some convenient references to the class buffers
	TPtr8 dest = iCompressedBuffer->Des();
	TPtr8 src = iFrameBuffer->Des();

	// The copy from packet chain method requires the descriptor to be at the maximum length
	src.SetMax();
	// Copy the packet chain to the contiguous buffer, also prepends the protocol id (aPppId)
	// to the beginning of the packet. 0x0021 for IP
	originalLength = (TInt16) CopyFrameIntoFlatBuf(src,aPacket,aPppId);
	// Set the length of the copied data for the compressor
	src.SetLength(originalLength);
	// Set the destination buffer size to allow for Uncompressed length	
	dest.SetLength(2);
	// Perform the RFC algorithm
	// method sets the size of dest length
	CompressRFC1978(src,&dest);
	// Check to see if it's worth sending compressed data
	TBool setTopBit = TRUE;
	if((dest.Length() - 2) > originalLength)
		{
		// Compressed larger than the original data
		// Reset the length and append the original source data
		dest.SetLength(2);
		dest.Append(src);
		setTopBit = FALSE;
		}

	BigEndian::Put16(const_cast<TUint8*>(dest.Ptr()),originalLength);
	// Calculate the 16bit CRC
	TPppFcs16 fcs;
	// CRC the length
	fcs.Calc(dest.Ptr(),dest.Ptr()+2);
	// We set the top bit if we've compressed the frame
	if(setTopBit)
		dest[0] |= 0x80;
	// CRC the original frame
	fcs.Calc(src.Ptr(),src.Ptr()+originalLength);
	// Make room for the CRC at the end of the buffer
	dest.SetLength(dest.Length()+2);
	// Write the network byte order converted CRC
	dest[dest.Length()-2] = (TUint8)(fcs.Fcs() & 0xff);
	dest[dest.Length()-1] = (TUint8)(fcs.Fcs() >> 8);

	// Formatted a contiguous buffer
	// Now re-make it into a packet chain
	TRAPD(ret, aPacket.AppendL(dest.Length()));
	if(ret == KErrNone)
		{
		aPacket.CopyIn(dest);
		retCode = EPppCompressedOK;
		}
	else
		retCode = EPppCompressedNotOK;

	return retCode;
	}

CPredCompressor::CPredCompressor()
/**
Constructor
*/
	{
	}

CPredCompressor::~CPredCompressor()
/**
Destructor
*/
	{

	delete iCompressedBuffer;
	delete iFrameBuffer;
	//
	// Close the factory and delete the container??
	//
	if(iFactory)
		iFactory->Close();
	}

/**
Copies the frame into a contiguous buffer.

@param aDest Output descriptor
@param aPacketQ Data to copy
@param aPppId PPP protocol ID to prepend to the output
	
@return Length of output data or 0 on an error
*/
TUint CPredCompressor::CopyFrameIntoFlatBuf(TPtr8& aDest, RMBufChain& aPacketQ, TUint16 aPppId)
	{
	const TUint	maxLength = aDest.Length();
	TUint8*		ptr = const_cast<TUint8*>(aDest.Ptr());
	
	// Store the protocol ID at the beginning of the uncompressed buffer
	BigEndian::Put16(ptr, aPppId);
	TUint Offset = 2;

	RMBuf* Temp;
	while((Temp=aPacketQ.Remove()) != NULL)
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
	aPacketQ.Free();
	return Offset;
	}


void TRFC1978Table::CompressRFC1978(const TDesC8& aSrc,TDes8* aDest)
/**
Compression logic from RFC 1978
Modified to use descriptors

@param aSrc Data to compress
@param aDest Compressed output buffer, or NULL to synchronise the guess table
without writing the output
*/
	{
	TInt len = aSrc.Length();
	TInt srcIndex = 0;
	TInt flagIndex = 0;
	while(len)
		{
		if(aDest)
			flagIndex = aDest->Length();
		TUint8 flags = 0;
		if(aDest)
			aDest->Append(flags);
		TUint8 bitMask;
		TInt j;
		for(bitMask = 1,j = 0;j < 8 && len ;j++,bitMask <<= 1)
			{
			TUint8 src = aSrc[srcIndex];

			if(iGuessTable[iHash] == src)
				{
				flags |= bitMask;
				}
			else
				{
				iGuessTable[iHash] = src;
				if(aDest)
					aDest->Append(src);
				}
			Hash(src);
			srcIndex++;
			len--;
			}
			if(aDest)
				{
				TUint8* ptr = const_cast<TUint8*>(aDest->Ptr() + flagIndex);
				*ptr = flags;
				}
		}
	}
