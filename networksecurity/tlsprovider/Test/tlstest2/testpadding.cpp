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
 @file testpadding.cpp
 @internalTechnology
*/
#include "testpadding.h"

#include <securityerr.h>

CTlsTestPadding* CTlsTestPadding::NewLC(TInt aBlockBytes, TInt aPaddingMod)
	{
	return new (ELeave) CTlsTestPadding(aBlockBytes, aPaddingMod);
	}
	
void CTlsTestPadding::DoPadL(const TDesC8& aInput,TDes8& aOutput)
	{
	TInt paddingBytes=BlockSize()-(aInput.Length()%BlockSize());
	aOutput.Append(aInput);
	aOutput.SetLength(aOutput.Length()+paddingBytes);
	for (TInt i=1;i<=paddingBytes;i++)
		{
		// apply the padding modifier to every byte in the padding
		aOutput[aOutput.Length()-i]=(TUint8)(paddingBytes+iPaddingMod-1);
		}
	}
	
void CTlsTestPadding::UnPadL(const TDesC8& aInput,TDes8& aOutput)
	{
	TInt paddingLen = aInput[aInput.Length()-1] + 1;

	if (paddingLen > aInput.Length())
		{
		User::Leave(KErrInvalidPadding);
		}
	
	// check the whole length of the padding is the same byte
	TPtrC8 padding = aInput.Right(paddingLen);
	for (TInt i = 0; i < paddingLen; ++i)
		{
		if (padding[i] != paddingLen)
			{
			User::Leave(KErrInvalidPadding);
			}
		}	

	TInt outlen = aInput.Length() - paddingLen;
	aOutput.Append(aInput.Left(outlen));
	}
	
TInt CTlsTestPadding::MinPaddingLength(void) const
	{
	return 1;
	}

TInt CTlsTestPadding::MaxPaddedLength(TInt aInputBytes) const
	{
	TUint padBytes = BlockSize() - (aInputBytes % BlockSize());
	return padBytes + aInputBytes;
	}
	
CTlsTestPadding::CTlsTestPadding(TInt aBlockBytes, TInt aPaddingMod)
	: CPadding(aBlockBytes), iPaddingMod(aPaddingMod)
	{
	}
