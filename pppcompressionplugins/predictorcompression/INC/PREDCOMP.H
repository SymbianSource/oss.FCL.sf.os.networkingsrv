/**
* Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/



/**
 @file PREDCOMP.H 
 @internalComponent
*/
 
#ifndef __PREDCOMP_H__
#define __PREDCOMP_H__

#include <networking/pppccp.h>
#include "PREDLOG.H"

NONSHARABLE_CLASS(CPredCompFactory) : public CPppCompFactory
/**	
Factory base for creating a concrete instance of a CSSLBase

@internalComponent
*/
	{
public:
	CPredCompFactory();
	void InstallL();
	virtual CPppCompressor* NewPppCompressorL(CPppCcp* aCcp,TInt aMaxFrameLength,const TUint8* aMode = NULL);
	virtual CPppDeCompressor* NewPppDeCompressorL(CPppCcp* aCcp, TInt aMaxFrameLength,const TUint8* aMode = NULL);
	};

class TRFC1978Table
/**
Main RFC 1978 class that contains the guess table and the associated
RFC compress and decompress functions.

@internalComponent
*/
	{
protected:
	void CompressRFC1978(const TDesC8& aSrc,TDes8* aDest);
	void DecompressRFC1978(const TDesC8& aSrc,TDes8& aDest);
	void Reset(){iGuessTable.SetMax();iGuessTable.FillZ();iHash = 0;}
private:
	TBuf8<65536> iGuessTable;
	TUint16      iHash;
	inline void Hash(TUint16 aSrc);
	};
	
inline void TRFC1978Table::Hash(TUint16 aSrc)
	{
	iHash = (TUint16)((iHash << 4) ^aSrc);
	}

NONSHARABLE_CLASS(CPredCompressor) : public CPppCompressor , public TRFC1978Table	
/**
Compressor class contains main Compressor virtual plus support methods
Predictor-1 PPP compressor (RFC 1978)
@internalComponent
*/
	{
public:
	TPppCompressReturnValue Compress(RMBufChain& aPacket, TUint16 aPppId);
	TBool	ResetCompressor(TInt aLength, RMBufChain& aPacket);
	~CPredCompressor();
	CPredCompressor();
	void	ConstructL(CPredCompFactory* aFactory, CPppCcp* aCcp, TInt aMaxFrameLength);

private:
	TUint CopyFrameIntoFlatBuf(TPtr8& aDest,RMBufChain& aPacketQ, TUint16 aPppId);

private:
	HBufC8*			iCompressedBuffer;
	HBufC8*			iFrameBuffer;
	CPppCcp*		iCcp;
	};

NONSHARABLE_CLASS(CPredDeCompressor) : public CPppDeCompressor , public TRFC1978Table
/**
Predictor-1 PPP decompressor (RFC 1978)
@internalComponent
*/
	{
public:
	TBool	Decompress(RMBufQ& aBufferQ);
	void	ResetDecompressor(TInt aLength, RMBufChain& aPacket);
	~CPredDeCompressor();
	CPredDeCompressor();
	void	ConstructL(CPredCompFactory* aFactory, CPppCcp* aCcp, TInt aMaxFrameLength);

private:
	TUint CopyFrameIntoFlatBuf(const TPtr8& aPtr,RMBufQ& aBufferQ);
	TUint FlattenBuf(TPtr8& aPtr, RMBufQ& aBufferQ);
	TBool CopyNewFrameToChain(TDesC8& aSrc,RMBufQ& aBufferQ);
private:
	TBool			iReConfiguring;
	HBufC8*			iCompressedBuffer;
	HBufC8*			iDecompressBuffer;
	CPppCcp*		iCcp;
	};

extern "C"
    {
/**
Generates and returns a CPppCompFactory object.
This is the polymorphic DLL entry point.

@return New CPppCompFactory object
@internalTechnology
*/
	EXPORT_C CPppCompFactory* NewPppCompFactoryL(void);
	}


#endif
