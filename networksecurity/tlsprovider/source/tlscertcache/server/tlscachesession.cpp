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

#include "tlscachesession.h"

#include <x509cert.h>
#include "tlscachesegment.h"
#include "tlsclientserver.h"

CTlsCacheSession* CTlsCacheSession::NewL(CTlsCacheSegment& aSegment)
	{
	return new (ELeave) CTlsCacheSession(aSegment);
	}
	
CTlsCacheSession::CTlsCacheSession(CTlsCacheSegment& aSegment)
	: iSegment(aSegment)
	{
	}
	
void CTlsCacheSession::ServiceL(const RMessage2& aMessage)
	{
	switch (aMessage.Function())
		{
	case EOpen:
		{
		TInt len = aMessage.GetDesLengthL(0);
		HBufC8* buf = HBufC8::NewLC(len);
		TPtr8 ptr(buf->Des());
		aMessage.ReadL(0, ptr);
		
		iCert = CX509Certificate::NewL(*buf);
		iWatcher = CTlsCacheItemStatusWatcher::NewL(iSegment, *iCert);
		CleanupStack::PopAndDestroy(buf);
		Server().AddSession();
		aMessage.Complete(KErrNone);
		}
		break;
		
	case ENotifyChange:
		{
		if (!iWatcher)
			{
			User::Leave(KErrArgument);
			}
			
		if (iWatcher->IsActive())
			{
			User::Leave(KErrInUse);
			}
		if (iSessionResponsibleForApproval)
			{
			// The session responsible for approval should not be waiting
			// for notification of approval.
			User::Leave(KErrNotReady);
			}
		iWatcher->StartL(aMessage);
		}
		break;
		
	case EGetEntryStatus:
		{
		if (!iCert)
			{
			User::Leave(KErrArgument);
			}
			
		CTlsCacheItem* item = iSegment.EntryL(*iCert);
		TCacheEntryState state = item->State();
		if (state == ENewEntry)
			{
			// This session's client will be responsible for setting the
			// approval state; others will wait for the state to be set.
			item->SetState(EEntryAwaitingApproval);
			iSessionResponsibleForApproval = ETrue;
			}
		TPckgC<TCacheEntryState> pkg(state);
		aMessage.WriteL(0, pkg);
		aMessage.Complete(KErrNone);
		}
		break;
		
	case ESetEntryStatus:
		{
		if (!iCert)
			{
			User::Leave(KErrArgument);
			}
			
		CTlsCacheItem* item = iSegment.EntryL(*iCert);
		TCacheEntryState state;
		TPckg<TCacheEntryState> pkg(state);
		aMessage.ReadL(0, pkg);
		
		item->SetState(state);
		iSegment.CommitL();
		aMessage.Complete(KErrNone);
		}
		break;
		
	case ECancel:
		{
		if (!iWatcher)
			{
			User::Leave(KErrArgument);
			}
			
		iWatcher->Cancel();
		if (iSessionResponsibleForApproval)
			{
			iWatcher->NotifyReset();
			iSessionResponsibleForApproval = EFalse;
			}
		aMessage.Complete(KErrNone);
		}
		break;
			
	default:
		User::Leave(KErrNotSupported);
		break;
		}
	}
	
void CTlsCacheSession::ServiceError(const RMessage2& aMessage, TInt aError)
	{
	aMessage.Complete(aError);
	}
	
CTlsCacheSession::~CTlsCacheSession()
	{
	if (iWatcher)
		{
		iWatcher->Cancel();
		if (iSessionResponsibleForApproval)
			{
			iWatcher->NotifyReset();
			}
		}
	delete iWatcher;
	delete iCert;
	iSegment.Release();
	Server().DropSession();
	}
	
CTlsCacheItemStatusWatcher* CTlsCacheItemStatusWatcher::NewL(CTlsCacheSegment& aSegment, const CX509Certificate& aCertificate)
	{
	CTlsCacheItemStatusWatcher* self = CTlsCacheItemStatusWatcher::NewLC(aSegment, aCertificate);
	CleanupStack::Pop(self);
	return self;
	}
	
CTlsCacheItemStatusWatcher* CTlsCacheItemStatusWatcher::NewLC(CTlsCacheSegment& aSegment, const CX509Certificate& aCertificate)
	{
	CTlsCacheItemStatusWatcher* self = new (ELeave) CTlsCacheItemStatusWatcher;
	CleanupStack::PushL(self);
	self->ConstructL(aSegment, aCertificate);
	return self;
	}
	
CTlsCacheItemStatusWatcher::CTlsCacheItemStatusWatcher()
	: CActive(EPriorityNormal)
	{
	CActiveScheduler::Add(this);
	}
	
void CTlsCacheItemStatusWatcher::ConstructL(CTlsCacheSegment& aSegment, const CX509Certificate& aCertificate)
	{
	iItem = aSegment.EntryL(aCertificate);
	}
	
void CTlsCacheItemStatusWatcher::StartL(const RMessage2& aMessage)
	{
	// Only wait if the entry is awaiting approval, otherwise the state has
	// probably changed between the client checking the state and requesting
	// notification, and the message can be completed immediately.
	if (iItem->State() == EEntryAwaitingApproval)
		{
		iMessage = aMessage;
		iItem->RequestNotificationL(iStatus);
		SetActive();
		}
	else
		{
		aMessage.Complete(KErrNone);
		}
	}
	
void CTlsCacheItemStatusWatcher::RunL()
	{
	User::LeaveIfError(iStatus.Int());
	iMessage.Complete(KErrNone);
	}
	
TInt CTlsCacheItemStatusWatcher::RunError(TInt aError)
	{
	iMessage.Complete(aError);
	return KErrNone;
	}
	
void CTlsCacheItemStatusWatcher::DoCancel()
	{
	iItem->CancelNotification(iStatus);
	iMessage.Complete(KErrCancel);
	}

void CTlsCacheItemStatusWatcher::NotifyReset()
	{
	// If approval is pending, set the item back to ENewEntry; this will
	// complete other outstanding notification requests.  This needs to be
	// done for premature closure of the session responsible for setting the
	// approval state to prevent other sessions hanging.
	if (iItem->State() == EEntryAwaitingApproval)
		{
		iItem->SetState(ENewEntry);
		}
	}
