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
// The Domain Name Decoder header file
// 
//

/**
 @file DomainNameDecoder.h
*/
 
#ifndef DOMAINNAMEDECODER_H
#define DOMAINNAMEDECODER_H

#include <e32base.h>

typedef TBuf8<0xFF> TDomainName;
typedef RArray<TDomainName> TDomainNameArray;

const TUint8 KMaxDnsLabelLength = 63;
const TUint8 KMaxDomainNameLength = 255;
 
class CDomainNameCodec : public CBase
	{
 	public:
 		virtual ~CDomainNameCodec();
 		
 		TUint NumDomainNames() const;
 		TDomainName& operator[](TUint aIndex);
 		
 		void DecodeL(TPtr8& aInDes);
 		void EncodeL(TDomainNameArray& aNames, RBuf8& aOutDes);
 		
 	protected:
 		TDomainNameArray iDomainList;
 	};
 	
inline TDomainName& CDomainNameCodec::operator[](TUint aIndex)
	{	
 	return iDomainList[aIndex];
 	}

inline TUint CDomainNameCodec::NumDomainNames() const
	{	
 	return iDomainList.Count();
 	}

#endif
