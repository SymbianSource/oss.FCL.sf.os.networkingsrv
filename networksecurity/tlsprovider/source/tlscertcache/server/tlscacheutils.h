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
// Interface for the TlsCacheUtil class
// 
//

/**
 @file 
 @internalTechnology
*/
 
#ifndef __TLSCACHEUTILS_H__
#define __TLSCACHEUTILS_H__

#include <e32base.h>
#include <f32file.h>

class TlsCacheUtil
	{
public:
	static inline TInt SystemDrive();
	static void BuildCacheFileNameL(TDes& aFileName, TUid aSid, RFs& aFs);
	
	};

inline TInt TlsCacheUtil::SystemDrive()
	{
	return RFs::GetSystemDrive();
	}

#endif /* __TLSCACHEUTILS_H__ */
