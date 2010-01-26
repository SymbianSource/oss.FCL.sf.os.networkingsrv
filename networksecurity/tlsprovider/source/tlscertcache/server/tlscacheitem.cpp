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

#include "tlscacheitem.h"

CTlsCacheItem* CTlsCacheItem::NewL(const CX509Certificate& aCertificate,
	TInt aAccepted, TInt aRejected)
	{
	CTlsCacheItem* self = CTlsCacheItem::NewLC(aCertificate, aAccepted, aRejected);
	CleanupStack::Pop(self);
	return self;
	}
	
CTlsCacheItem* CTlsCacheItem::NewLC(const CX509Certificate& aCertificate,
	TInt aAccepted, TInt aRejected)
	{
	CTlsCacheItem* self = new (ELeave) CTlsCacheItem(aAccepted, aRejected);
	CleanupStack::PushL(self);
	self->ConstructL(aCertificate);
	return self;
	}
	
CTlsCacheItem* CTlsCacheItem::NewLC(TInt aAccepted, TInt aRejected)
	{
	CTlsCacheItem* self = new (ELeave) CTlsCacheItem(aAccepted, aRejected);
	CleanupStack::PushL(self);
	return self;
	}
	
CTlsCacheItem::CTlsCacheItem(TInt aAccepted, TInt aRejected)
	: iState(ENewEntry), iAcceptedTimeout(aAccepted), iRejectedTimeout(aRejected),
	  iApprovalTime(0)
	{
	}
	
void CTlsCacheItem::ConstructL(const CX509Certificate& aCertificate)
	{
	iCert = CX509Certificate::NewL(aCertificate);
	}
	
CTlsCacheItem::~CTlsCacheItem()
	{
	DoChangeNotify(KErrCancel);
	delete iCert;
	}
	
TBool CTlsCacheItem::IsExpired()
	{
	TTime now;
	now.UniversalTime();
	
	if (iState == EEntryApproved)
		{
		TTimeIntervalSeconds interval(iAcceptedTimeout);
		TTime expirytime(iApprovalTime + interval);
		return (now > expirytime);
		}
	else if (iState == EEntryDenied)
		{
		TTimeIntervalSeconds interval(iRejectedTimeout);
		TTime expirytime(iApprovalTime + interval);
		return (now > expirytime);
		}
	else
		{
		return EFalse;
		}
	}
	
void CTlsCacheItem::Reset()
	{
	iApprovalTime = TTime(0);
	iState = ENewEntry;
	}
	
void CTlsCacheItem::RequestNotificationL(TRequestStatus& aStatus)
	{
	aStatus = KRequestPending;
	TRequestStatus* req = &aStatus;
	User::LeaveIfError(iNotificationRequests.Append(req));
	}
	
void CTlsCacheItem::CancelNotification(TRequestStatus& aStatus)
	{
	TRequestStatus* req = &aStatus;
	TInt index = iNotificationRequests.Find(req);
	if (index >= 0)
		{
		iNotificationRequests.Remove(index);
 		User::RequestComplete(req, KErrCancel);
		}
	}
	
void CTlsCacheItem::DoChangeNotify(TInt aError)	
	{
	TInt count(iNotificationRequests.Count());
	while (count--)
		{
		TRequestStatus* req = iNotificationRequests[count];
		User::RequestComplete(req, aError);
		}
	iNotificationRequests.Reset();
	}
	
void CTlsCacheItem::ExternalizeL(RWriteStream& aStream)
	{
	aStream << *iCert;
	aStream.WriteInt32L(iState);
	TPckgC<TTime> time(iApprovalTime);
	aStream << time;
	}
	
void CTlsCacheItem::InternalizeL(RReadStream& aStream)
	{
	iCert = CX509Certificate::NewL(aStream);
	iState = static_cast<TCacheEntryState>(aStream.ReadInt32L());
	TPckg<TTime> time(iApprovalTime);
	aStream >> time;
	}
	
void CTlsCacheItem::SetState(TCacheEntryState aState)
	{
	if (aState == EEntryApproved || aState == EEntryDenied)
		{
		iApprovalTime.UniversalTime();
		}
	iState = aState;
	DoChangeNotify(KErrNone);
	}
