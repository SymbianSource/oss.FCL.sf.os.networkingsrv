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
 @file testpadding.h
 @internalTechnology	
*/
#ifndef __TESTPADDING_H__
#define __TESTPADDING_H__

#include <padding.h>

/*
 * This class implements SSLv3 padding, but offers more error checking
 * on UnPad than the Symbian class does, and can corrupt the padding
 * on DoPad.
 *
 */

class CTlsTestPadding : public CPadding
	{
public:
	static CTlsTestPadding* NewLC(TInt aBlockBytes, TInt aPaddingMod);
	
	void DoPadL(const TDesC8& aInput,TDes8& aOutput);
	void UnPadL(const TDesC8& aInput,TDes8& aOutput);
	
	TInt MinPaddingLength(void) const;
	TInt MaxPaddedLength(TInt aInputBytes) const;
	
private:
	CTlsTestPadding(TInt aBlockBytes, TInt aPaddingMod);
	
private:
	TInt iPaddingMod;
	};

#endif /* __TESTPADDING_H__ */
