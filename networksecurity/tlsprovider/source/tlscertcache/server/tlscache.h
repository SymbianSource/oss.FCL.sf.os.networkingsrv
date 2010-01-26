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
// Interface for the CTlsCache class
// 
//

/**
 @file 
 @internalTechnology
*/
 
#ifndef __TLSCACHE_H__
#define __TLSCACHE_H__

#include <e32base.h>

class CTlsCacheSegment;

class CTlsCache : public CBase
	{
public:
	static CTlsCache* NewL();
	
	CTlsCacheSegment& SegmentL(TUid aSid);
	
	~CTlsCache();	
private:
	RPointerArray<CTlsCacheSegment> iSegments;

	};

#endif /* __TLSCACHE_H__ */
