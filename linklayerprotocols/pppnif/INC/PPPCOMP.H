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
 @internalTechnology 
*/

#ifndef __PPPCOMP_H__
#define __PPPCOMP_H__

#include <es_mbuf.h>

// Return value from call to compress module
enum TPppCompressReturnValue
	{ EPppCompressedOK, EPppCompressedNotOK, EPppCompressedFrameExpanded };

class CNifFactory;
class CPppCompressor : public CObject
	{
public:
	CPppCompressor(){};
	virtual ~CPppCompressor(){};

	virtual TPppCompressReturnValue Compress(RMBufChain& aPacket, TUint16 aPppId)=0;
	virtual TBool ResetCompressor(TInt aLength, RMBufChain& aPacket)=0;
protected:
	CNifFactory*	iFactory;
	};

class CPppDeCompressor : public CObject
	{
public:
	virtual TBool Decompress(RMBufQ& aBufferQ)=0;
	virtual void ResetDecompressor(TInt aLength, RMBufChain& aPacket)=0;

	CPppDeCompressor(){};
	virtual ~CPppDeCompressor(){};
protected:
	CNifFactory*	iFactory;
	};

#endif //__PPPCOMP_H__
