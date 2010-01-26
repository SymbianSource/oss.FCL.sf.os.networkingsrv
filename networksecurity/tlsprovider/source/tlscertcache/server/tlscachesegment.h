// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Interface for the CTlsCacheSegment class
// 
//

/**
 @file 
 @internalTechnology
*/
 
#ifndef __TLSCACHESEGMENT_H__
#define __TLSCACHESEGMENT_H__

#include <e32base.h>
#include <s32strm.h>
#include <f32file.h>

#include "tlscacheitem.h"

class CTlsCacheSegment : public CBase
	{

public:
	static CTlsCacheSegment* NewL(TUid aClientSid);
	static CTlsCacheSegment* NewLC(TUid aClientSid);
	
	void ReferenceL();
	void Release();
	void CommitL();
	
	inline const TUid Sid();
	CTlsCacheItem* EntryL(const CX509Certificate& aCert);
	
	~CTlsCacheSegment();
	
private:
	CTlsCacheSegment(TUid aClientSid);
	void ConstructL();
	
	void ExternalizeL(RWriteStream& aStream);
	void InternalizeL(RReadStream& aStream);
	
private:
	TUid iClientSid;
	TInt iReferenceCount;
	
	RFs iFs;
	
	RPointerArray<CTlsCacheItem> iItems;
	TInt iAcceptedTimeout;
	TInt iRejectedTimeout;
	
	};

inline const TUid CTlsCacheSegment::Sid()
	{
	return iClientSid;
	}

#endif /* __TLSCACHESEGMENT_H__ */
