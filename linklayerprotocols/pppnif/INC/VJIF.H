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

#ifndef __VJIF_H__
#define __VJIF_H__

#include <nifman.h>

class CVJCompressorIf : public CObject
/**
Van Jacobson TCP/IP header compressor base class
@internalTechnology
*/
	{
public:
	CVJCompressorIf(){};
	virtual ~CVJCompressorIf(){};
	virtual TInt VJCompressFrame(RMBufChain& aPacket)=0;
protected:
	/** Factory that created this object */
	CNifFactory*	iFactory;
	};

class CVJDeCompressorIf : public CObject
/**
Van Jacobson TCP/IP header decompressor base class
@internalTechnology
*/
	{
public:
	CVJDeCompressorIf(){};
	virtual ~CVJDeCompressorIf(){};
	virtual TBool DecompVJUncomp(RMBufChain& aPacket)=0;
	virtual TBool DecompVJComp(RMBufChain& aPacket)=0;
	virtual void CRCError()=0;
protected:
	/** Factory that created this object */
	CNifFactory*	iFactory;
	};

class CVJCompFactory : public CNifFactory
/**
Factory base for creating a concrete instance of a CVJCompressor
or CVJDeCompressor
@internalTechnology
*/
	{
public:
	virtual void InstallL()=0;
	virtual CVJCompressorIf* NewVJCompressorL( TInt aMaxVJSlots, TInt aCompressConnId)=0;
	virtual CVJDeCompressorIf* NewVJDeCompressorL(TInt aMaxVJSlots)=0;
	};

#endif //__VJIF_H__
