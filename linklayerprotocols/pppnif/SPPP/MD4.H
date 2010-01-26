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
 @brief Header file for the implementation of the MD4 Message-Digest Algorithm - RFC 1320.
 @internalComponent 
*/

#ifndef __MD4_H__
#define __MD4_H__

#include <e32base.h>

// This implementation is fully derived (copied) from the former MD4
// implementation provided by Symbian PPP in the release 7.0s of
// Symbian OS.  This is a provisional solution until the Symbian
// Security subsystem components will provide a MD4 implementation.

/*final*/ class CMd4 : public CBase
/**
   Class that implements the MD4 Message-Digest Algorithm - RFC 1320.
   @internalComponent
   @note This implementation is fully derived from the former MD4 implementation provided by Symbian PPP in the release 7.0s of Symbian OS.  This is a provisional solution until the Symbian Security components will provide a MD4 implementation.
*/
	{
public:
	static CMd4* NewL();

//functions moved from the former CDigestBase
	TInt Input(const TDesC8& aInput);
	TInt Output(TDes8& aOutput);
	TInt OutputSize() const;

private:
	CMd4();
	void Transform(const TUint8*);
	void Update(const TUint8*, TInt);
	void Final(TUint8*);
	void Encode(TUint8*, const TUint32*, TInt);
	void Decode(TUint32*, const TUint8*, TInt);

private:
	TUint32 iState[4];
	TUint iCount[2];
	TUint8 iBuffer[64];
	};

#endif
