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

#include "tlscachesegment.h"

#include <s32file.h>
#include <bautils.h>
#include <barsc.h>
#include "tlscacheutils.h"
#include "tlscacheitem.h"

#include <tlscachetimeouts.rsg>
_LIT(KTimeoutsResource, "z:\\Resource\\TlsCacheServer\\TlsCacheTimeouts.rsc");

CTlsCacheSegment::CTlsCacheSegment(TUid aClientSid)
	: iClientSid(aClientSid), iReferenceCount(0)
	{
	}
	
CTlsCacheSegment* CTlsCacheSegment::NewL(TUid aClientSid)
	{
	CTlsCacheSegment* self = CTlsCacheSegment::NewLC(aClientSid);
	CleanupStack::Pop(self);
	return self;
	}
	
CTlsCacheSegment* CTlsCacheSegment::NewLC(TUid aClientSid)
	{
	CTlsCacheSegment* self = new (ELeave) CTlsCacheSegment(aClientSid);
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
	}
	
void CTlsCacheSegment::ConstructL()
	{
	User::LeaveIfError(iFs.Connect());
	
	// Read the cache timeouts
	RResourceFile file;
	file.OpenL(iFs, KTimeoutsResource);
	CleanupClosePushL(file);
	
	HBufC8* buf = file.AllocReadLC(R_TLS_CACHE_TIMEOUTS);

	TResourceReader reader;
	reader.SetBuffer(buf);
	
	iAcceptedTimeout = reader.ReadInt16();
	iRejectedTimeout = reader.ReadInt16();
	
	CleanupStack::PopAndDestroy(2, &file);
	
	}
	
void CTlsCacheSegment::ReferenceL()
	{
	
	if (iReferenceCount++ == 0)
		{
		// This is the first reference, lets see if we
		// can read it in from disk
		
		TFileName name;
		TlsCacheUtil::BuildCacheFileNameL(name, iClientSid, iFs);
		
		RFile file;
		TInt err = file.Open(iFs, name, EFileRead | EFileStream | EFileShareExclusive);
		if (err == KErrNone)
			{
			CleanupClosePushL(file);
			RFileReadStream stream(file, 0);
			stream.PushL();
			
			// TRAP_IGNORE this, if the cache is corrupt this will leave,
			// we'll start only with any certs correctly read and the corrupt 
			// version will be overwritten on the next CommitL()
			
			TRAP_IGNORE(InternalizeL(stream));
			stream.Pop();
			stream.Close();  
			CleanupStack::PopAndDestroy(&file);
			}
		else if (err != KErrNotFound && err != KErrPathNotFound)
			{
			User::Leave(err);
			}
		}
	}
	
void CTlsCacheSegment::Release()
	{
	ASSERT(iReferenceCount > 0);
	
	if (--iReferenceCount == 0)
		{
		// free all the contents of this cache instance
		iItems.ResetAndDestroy();
		}
	}
	
void CTlsCacheSegment::CommitL()
	{
	iFs.CreatePrivatePath(TlsCacheUtil::SystemDrive()); // Ignore error, we'll catch it when we write
		
	TFileName name;
	TlsCacheUtil::BuildCacheFileNameL(name, iClientSid, iFs);
		
	RFile file;
	User::LeaveIfError(file.Replace(iFs, name, EFileWrite | EFileStream | EFileShareExclusive));
	CleanupClosePushL(file);
		
	RFileWriteStream stream;
	stream.Attach(file, 0);
	stream.PushL();
	ExternalizeL(stream);
	stream.Pop();
	stream.Close();
		
	CleanupStack::PopAndDestroy(&file);
	}
	
void CTlsCacheSegment::InternalizeL(RReadStream& aStream)
	{
	TInt itemCount = aStream.ReadInt32L();
	
	// Internalise all the cache items
	while (itemCount--)
		{
		CTlsCacheItem* item = CTlsCacheItem::NewLC(iAcceptedTimeout, iRejectedTimeout);
		item->InternalizeL(aStream);
		if (item->IsExpired())
			{
			CleanupStack::PopAndDestroy(item);
			}
		else
			{
			TCacheEntryState state = item->State();
			// We should only have approved or denied entries written to the
			// cache file.
			if (state != EEntryApproved && state != EEntryDenied)
				{
				User::Leave(KErrCorrupt);
				}
			User::LeaveIfError(iItems.Append(item));
			CleanupStack::Pop(item);
			}
		}
	}
	
void CTlsCacheSegment::ExternalizeL(RWriteStream& aStream)
	{
	TInt itemCount(iItems.Count());
	aStream.WriteInt32L(itemCount);
	while (itemCount--)
		{
		if (!iItems[itemCount]->IsExpired())
			{
			TCacheEntryState state = iItems[itemCount]->State();
			// Only write approved or denied entries to the cache file.
			if (state == EEntryApproved || state == EEntryDenied)
				{
				iItems[itemCount]->ExternalizeL(aStream);
				}
			}
		}
	}
	
CTlsCacheItem* CTlsCacheSegment::EntryL(const CX509Certificate& aCert)	
	{
	TInt count(iItems.Count());
	while (count--)
		{
		CTlsCacheItem* item = iItems[count];
		if (aCert.IsEqualL(item->Certificate()))
			{
			if (item->IsExpired())
				{
				item->Reset();
				}
			return item;
			}
		}
	
	// Does not currently exist, add a new entry
	CTlsCacheItem* item = CTlsCacheItem::NewLC(aCert, iAcceptedTimeout, iRejectedTimeout);
	User::LeaveIfError(iItems.Append(item));
	CleanupStack::Pop(item);
	return item;
	}
	
CTlsCacheSegment::~CTlsCacheSegment()
	{
	iFs.Close();
	iItems.ResetAndDestroy();
	}
