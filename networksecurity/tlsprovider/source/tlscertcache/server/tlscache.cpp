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
//

#include "tlscache.h"

#include "tlscachesegment.h"

CTlsCache* CTlsCache::NewL()
	{
	return new (ELeave) CTlsCache;
	}
	
CTlsCache::~CTlsCache()
	{
	iSegments.ResetAndDestroy();
	}
	
CTlsCacheSegment& CTlsCache::SegmentL(TUid aSid)
	{
	TInt count(iSegments.Count());
	CTlsCacheSegment* seg;
	while (count--)
		{
		seg = iSegments[count];
		if (seg->Sid() == aSid)
			{
			seg->ReferenceL();
			return *seg;
			}
		}
	
	// It isn't currently in our list, create an instance of it
	seg = CTlsCacheSegment::NewLC(aSid);
	seg->ReferenceL();
	User::LeaveIfError(iSegments.Append(seg));
	CleanupStack::Pop(seg);
	return *seg;
	}
