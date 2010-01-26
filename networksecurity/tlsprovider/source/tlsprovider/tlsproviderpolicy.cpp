// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "tlsproviderpolicy.h"
#include <s32file.h>
	
CTlsProviderPolicy* CTlsProviderPolicy::NewL()
	{
	CTlsProviderPolicy* self = new(ELeave)CTlsProviderPolicy();
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CTlsProviderPolicy::CTlsProviderPolicy()
: iClientAuthenticationEnabled(EFalse)
	{
	}
	
CTlsProviderPolicy::~CTlsProviderPolicy()
	{
	iCriticalExtn.ResetAndDestroy();
	}
	
TBool CTlsProviderPolicy::ClientAuthenticationDialogEnabled() const
	{
	return iClientAuthenticationEnabled;
	}

TInt CTlsProviderPolicy::PKICriticalExtensions()
    {
    return iCriticalExtn.Count();
    }

RPointerArray<TDesC>& CTlsProviderPolicy::GetPKICriticalExtensions()
    {
    return iCriticalExtn;
    }

TBool CTlsProviderPolicy::ReadLineL(const TDesC8& aBuffer, TInt& aPos, TPtrC8& aLine) const
	{
	TBool endOfBuffer = EFalse;	
	aLine.Set(NULL, 0);

	TInt bufferLength = aBuffer.Length();	
	__ASSERT_ALWAYS(aPos >=0 && aPos <= bufferLength, User::Leave(KErrArgument));
	
	// skip blank lines
	while (aPos < bufferLength) 
		{
		TChar  c = aBuffer[aPos];
		if (c != '\r' && c != '\n')
			{
			break;
			}
		aPos++;
		}

	// find the position of the next delimter		
	TInt endPos = aPos;	
	while (endPos < bufferLength)
		{
		TChar c = aBuffer[endPos];
		
		if (c == '\n' || c == '\r') 
			{
			break;
			}	
		endPos++;
		}
		
	if (endPos != aPos)	
		{
		TInt tokenLen = endPos - aPos;
		aLine.Set(&aBuffer[aPos], tokenLen);
		}
	else 
		{
		return ETrue; // End of buffer
		}			
		
	aPos = endPos;
	return endOfBuffer;
	}

void CTlsProviderPolicy::ConstructL()
	{
	_LIT8(KFalse, "false");
	
	// Read the policy file into a buffer
	RFs fs;
	User::LeaveIfError(fs.Connect());
	CleanupClosePushL(fs);
	
	RFile policyFile;
	User::LeaveIfError(policyFile.Open(fs, KTlsProviderPolicyFile, EFileRead));
	CleanupClosePushL(policyFile);

	TInt size;
	policyFile.Size(size);
	CleanupStack::PopAndDestroy(&policyFile);

	HBufC8* policyBuffer = HBufC8::NewLC(size);
	TPtr8 p(policyBuffer->Des());
	p.SetLength(size);

	RFileReadStream stream;
	User::LeaveIfError(stream.Open(fs, KTlsProviderPolicyFile, EFileStream));
	CleanupClosePushL(stream);
	stream.ReadL(p, size);
	CleanupStack::PopAndDestroy(&stream);		
			
	TInt readPos = 0;
	TPtrC8 currentLine;
	while (!ReadLineL(*policyBuffer, readPos, currentLine))
		{
		TLex8 lex(currentLine);

		lex.SkipSpaceAndMark() ; 		// move to end of character token
		lex.SkipCharacters();
				
		if (lex.TokenLength() == 0) // if valid potential token
			{
			User::Leave(KErrCorrupt);
			}		
		TPtrC8 key = lex.MarkedToken(); 
		
		lex.SkipSpaceAndMark(); 		// move to end of character token
		lex.SkipCharacters();
		
		if (lex.TokenLength() == 0) // if valid potential token
			{
			User::Leave(KErrCorrupt);
			}		
		/*TPtrC8 separator =*/ lex.MarkedToken(); 

		lex.SkipSpace(); 		// move to end of character token

		// Get the value
		TPtrC8 value = lex.Remainder(); 	
			
		if (key.Compare(KClientAuthenticationDialogEnabled) == 0)
			{
			TBool valueBool = (value.Compare(KFalse) != 0);
			iClientAuthenticationEnabled = valueBool;
			}
		else if (key.Match(KPKICriticalExtn_Pattern)!= KErrNotFound ) 
		    {
            HBufC16* extn_val = HBufC16::NewMaxLC(value.Length());
            extn_val->Des().Copy(value);
            iCriticalExtn.AppendL(extn_val);
            CleanupStack::Pop(extn_val);		
            }
		}
	
	CleanupStack::PopAndDestroy(2, &fs); // policyBuffer, fs
	}

