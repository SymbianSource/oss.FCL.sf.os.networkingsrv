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
// TLSCACHECLIENTSERVER.H
// 
//

/**
 @file 
 @internalTechnology
*/
 
#ifndef __TLSCLIENTSERVER_H__
#define __TLSCLIENTSERVER_H__

_LIT(KCacheServerName, "!TLSCACHESERVER");
_LIT(KCacheServerExe, "TlsCacheServer");

const TUid KCacheServerUid = {0x200008D1};

enum TCacheEntryState
	{
	ENewEntry,
	EEntryAwaitingApproval,
	EEntryDenied,
	EEntryApproved
	};
	
enum TCacheFunction
	{
	EOpen,
	EGetEntryStatus,
	ESetEntryStatus,
	ENotifyChange,
	ECancel
	};

#endif /* __TLSCLIENTSERVER_H__ */
