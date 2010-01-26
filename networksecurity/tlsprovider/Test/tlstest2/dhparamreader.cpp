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
 @file dhparamreader.cpp
 @internalTechnology
*/
#include "dhparamreader.h"

#include <f32file.h>
#include <asn1dec.h>

void CDHParamReader::DecodeDERL(const TDesC& aDerFile,
	RInteger& aPrimeOut, RInteger& aGeneratorOut)
	{
	RFs fs;
	User::LeaveIfError(fs.Connect());
	CleanupClosePushL(fs);
	
	RFile file;
	User::LeaveIfError(file.Open(fs, aDerFile, EFileRead));
	CleanupClosePushL(file);
	
	TInt fileSize(0);
	User::LeaveIfError(file.Size(fileSize));
	
	HBufC8* derData = HBufC8::NewLC(fileSize);
	TPtr8 derPtr = derData->Des();
	User::LeaveIfError(file.Read(derPtr));
	
	// we know this should just be an ASN.1 sequence, consisting of two integers....
	TASN1DecGeneric genDec(*derData);
	genDec.InitL();
	
	if (genDec.Tag() != EASN1Sequence)
		{
		// not a sequence...
		User::Leave(KErrNotSupported);
		}
	
	TASN1DecSequence seq;
	CArrayPtrFlat<TASN1DecGeneric>* ints = seq.DecodeDERLC(genDec);
	
	// validate the sequence data
	if (ints->Count() != 2 || 
		ints->At(0)->Tag() != EASN1Integer ||
		ints->At(1)->Tag() != EASN1Integer)
		{
		// This isn't a DH parameter file we can recognise...
		User::Leave(KErrNotSupported);
		}
		
	// Read the integers from the sequence....
	TASN1DecInteger decInt;
	aPrimeOut = decInt.DecodeDERLongL(*ints->At(0));
	aGeneratorOut = decInt.DecodeDERLongL(*ints->At(1));
	
	CleanupStack::PopAndDestroy(4, &fs); // file, derData, ints
	}
