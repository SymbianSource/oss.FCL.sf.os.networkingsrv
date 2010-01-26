// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Decodes domain names within protocol messages as specified in:
// "RFC 1035 - Domain names - implementation and specification"
// 
//

/**
 @file DomainNameDecoder.cpp
 @internalTechnology
*/

#include "DomainNameDecoder.h"

void CDomainNameCodec::EncodeL(TDomainNameArray& aNames, RBuf8& aBuf8)
	{
	TUint requiredLength = 0;
	TUint8 nameIdx = 0;
	
	for (nameIdx=0;nameIdx<aNames.Count();nameIdx++)
		{
		// The total length required for the labels that comprise an
		// individual domain name needs to take into the length octet
		// for the initial label and the null-termination character.
		// Hence the '+ 2' below.
		requiredLength += (aNames[nameIdx].Length() + 2);		
		
		// A further length check is performed on each domain name to
		// ensure it does not exceed the maximum length permitted according
		// to RFC 1035.
		if(aNames[nameIdx].Length() > KMaxDomainNameLength)
			{
			User::Leave(KErrArgument);
			}
		}		
	
	aBuf8.Zero();
	aBuf8.ReAllocL(requiredLength);
	
	TLex8 domainName;
	TPtrC8 currentLabel;
	
	for (nameIdx=0;nameIdx<aNames.Count();nameIdx++)
		{
		domainName.Assign(aNames[nameIdx]);		
		domainName.Mark();
		
		while (!domainName.Eos())
			{
			TChar ch;
			do
				{
				ch = domainName.Get();
				}
			while ( ch != TChar('.') && !domainName.Eos() );
			
			// if not the end of the string, unget the previous char to skip the trailing
			//  dot in our marked token
			//
			if( !domainName.Eos() )
				{
				domainName.UnGet();
				}
			
			currentLabel.Set(domainName.MarkedToken());
			
			// move past the dot again, or do nothing in particular at EOS
			//
			domainName.Get();
			
			User::LeaveIfError(currentLabel.Length() > KMaxDnsLabelLength ? 
				KErrArgument : KErrNone);
			
			aBuf8.Append(TChar(currentLabel.Length()));
			aBuf8.Append(currentLabel);
			
			domainName.Mark();
			}
		
		aBuf8.Append(TChar(0));
		}
	}

void CDomainNameCodec::DecodeL(TPtr8& aInDes)
	{
	TDomainName domainName;

	TUint8* pChar = const_cast<TUint8*>(aInDes.Ptr());
	TUint listLength = aInDes.Length();
	TUint8 labelLength = 0;
	
	// Walk the list of domain names
	while(pChar < aInDes.Ptr() + listLength)
		{
		domainName.Zero();
		
		while(*pChar++ != NULL)
			{
			if(domainName.Length() > 0)
				{
				domainName.Append('.');
				}
				
			labelLength = *(pChar - 1);
			
			// The two highest order bits must be clear
			User::LeaveIfError(labelLength & 0xC0 ? KErrArgument : KErrNone);
					
			// Add in the label data
			domainName.Append(pChar, labelLength);
			
			// Advance the pointer to the next length value
			pChar += labelLength;
			}
			
		iDomainList.Append(domainName);
		}
	}
	
CDomainNameCodec::~CDomainNameCodec()
	{
	iDomainList.Close();
	}
	
	
	
	
	
	

