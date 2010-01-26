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
 @file psuedorandom.cpp
 @internalTechnology
*/
#include "psuedorandom.h"

HBufC8* CTls10PsuedoRandom::PseudoRandomL(const TDesC8& aSecret, const TDesC8& aLabel,
	const TDesC8& aSeed, TInt aLength)
	{
	TInt keylen = (aSecret.Length() / 2) + (aSecret.Length() % 2);
	HBufC8* output = HBufC8::NewLC(aLength);
	TPtr8 outputDes = output->Des();
	
	// Create scratchpads for calculating the A(...) function
	TInt scratchLen = aSeed.Length() + aLabel.Length();
	if (scratchLen < 20)
		{
		scratchLen = 20;
		}
	HBufC8* scratchpad = HBufC8::NewLC(scratchLen);
	TPtr8 scratchpadDes = scratchpad->Des();
	
	TPtrC8 sha1key(aSecret.Right(keylen));
	CMessageDigest* sha1mac = CMessageDigestFactory::NewHMACL(CMessageDigest::ESHA1, sha1key);
	
	// set A0
	scratchpadDes.Append(aLabel);
	scratchpadDes.Append(aSeed);
	
	// Now calculate the SHA1 half of the function
	TInt outputLen = aLength;
	while (outputLen > 0)
		{
		// calculate a(n+1)
		sha1mac->Reset();
		sha1mac->Update(*scratchpad);
		scratchpadDes.Zero();
		scratchpadDes.Append(sha1mac->Final());
		
		// calculate this iteration of p_Hash
		
		sha1mac->Reset();
		sha1mac->Update(*scratchpad); // a(n)
		sha1mac->Update(aLabel);
		sha1mac->Update(aSeed);
		outputDes.Append(sha1mac->Final().Left(outputLen));
		outputLen -= sha1mac->HashSize();
		}
		
	delete sha1mac;
	
	TPtrC8 md5key(aSecret.Left(keylen));
	CMessageDigest* md5mac = CMessageDigestFactory::NewHMACL(CMessageDigest::EMD5, md5key);
	
	// set A0, again
	scratchpadDes.Zero();
	scratchpadDes.Append(aLabel);
	scratchpadDes.Append(aSeed);
	
	// now, XOR in the MD5 version
	outputLen = aLength;
	
	for (TInt i = 0; i < outputLen;)
		{
		// calculate a(n+1)
		md5mac->Reset();
		md5mac->Update(*scratchpad);
		scratchpadDes.Zero();
		scratchpadDes.Append(md5mac->Final());

		// calculate this iteration of p_Hash
		
		md5mac->Reset();
		md5mac->Update(*scratchpad); // a(n)
		md5mac->Update(aLabel);
		TPtrC8 mac = md5mac->Hash(aSeed);

		TInt finalByte = ((i+md5mac->HashSize()) > outputLen) ? outputLen : (i+md5mac->HashSize());
		TInt pos = 0;
		while (i < finalByte)
			{
			outputDes[i++] ^= mac[pos++]; 
			}
		}
	
	delete md5mac;
	CleanupStack::PopAndDestroy(scratchpad);
	CleanupStack::Pop(output);
	
	return output;
	}
	
HBufC8* CSsl30PsuedoRandom::PseudoRandomL(const TDesC8& aSecret, const TDesC8& aSeed, TInt aLength)
	{
	HBufC8* ret = HBufC8::NewLC(aLength);
	TPtr8 retBuf = ret->Des();
	
	CMessageDigest* sha1dig = CMessageDigestFactory::NewDigestLC(CMessageDigest::ESHA1);
	CMessageDigest* md5dig = CMessageDigestFactory::NewDigestLC(CMessageDigest::EMD5);
	
	TUint8 labelChar = 'A';
	TInt round = 1;
	while (aLength > 0)
		{
		sha1dig->Reset();
		md5dig->Reset();
		
		// compute the inner (SHA1) hash
		TPtrC8 labelPtr(&labelChar, 1);
		for (TInt i = 0; i < round; ++i)
			{
			sha1dig->Update(labelPtr);
			}
		sha1dig->Update(aSecret);
		sha1dig->Update(aSeed);
		
		// compute the outer hash.
		md5dig->Update(aSecret);
		md5dig->Update(sha1dig->Final());
		
		// finally, append the required amound of data to the output
		TPtrC8 roundOutput = md5dig->Final().Left(aLength);
		retBuf.Append(roundOutput);
		
		aLength -= roundOutput.Length();
		++round;
		++labelChar;
		}
		
	CleanupStack::PopAndDestroy(2, sha1dig); // md5dig
	CleanupStack::Pop(ret);
	return ret;	
	}
