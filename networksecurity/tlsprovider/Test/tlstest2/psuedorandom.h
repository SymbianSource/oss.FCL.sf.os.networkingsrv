// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
 @file psuedorandom.h
 @internalTechnology	
*/
#ifndef __PSUEDORANDOM_H__
#define __PSUEDORANDOM_H__

#include <e32base.h>
#include <hash.h>

class CTls10PsuedoRandom : public CBase
	{
public:
	static HBufC8* PseudoRandomL(const TDesC8& aSecret, const TDesC8& aLabel, 
		const TDesC8& aSeed, TInt aLength);
	};
	
class CSsl30PsuedoRandom : public CBase
	{
public:
	static HBufC8* PseudoRandomL(const TDesC8& aSecret, const TDesC8& aSeed, TInt aLength);
	};

#endif /* __PSUEDORANDOM_H__ */
