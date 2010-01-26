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
// Interface for the CTlsCacheItem class
// 
//

/**
 @file 
 @internalTechnology
*/
 
#ifndef __TLSCACHEITEM_H__
#define __TLSCACHEITEM_H__

#include <e32base.h>
#include <x509cert.h>

#include "tlsclientserver.h"

class CTlsCacheItem : public CBase
	{
public:
	static CTlsCacheItem* NewL(const CX509Certificate& aCertificate, 
		TInt aAccepted, TInt aRejected);
	static CTlsCacheItem* NewLC(const CX509Certificate& aCertificate, 
		TInt aAccepted, TInt aRejected);
	
	static CTlsCacheItem* NewLC(TInt aAccepted, TInt aRejected);
	
	inline const CX509Certificate& Certificate();
	inline const TCacheEntryState& State();
	
	void SetState(TCacheEntryState aState);
	TBool IsExpired();
	void Reset();
	
	void ExternalizeL(RWriteStream& aStream);
	void InternalizeL(RReadStream& aStream);
	
	void RequestNotificationL(TRequestStatus& aStatus);
	void CancelNotification(TRequestStatus& aStatus);
	
	~CTlsCacheItem();
	
private:
	CTlsCacheItem(TInt aAccepted, TInt aRejected);
	void ConstructL(const CX509Certificate& aCertificate);
	
	void DoChangeNotify(TInt aError);
	
private: 
	CX509Certificate* iCert;
	TCacheEntryState iState;
	
	RPointerArray<TRequestStatus> iNotificationRequests; // Do not delete pointers!
	
	TInt iAcceptedTimeout;
	TInt iRejectedTimeout;
	TTime iApprovalTime;
	
	};
	
inline const CX509Certificate& CTlsCacheItem::Certificate()
	{
	return *iCert;
	}
	
inline const TCacheEntryState& CTlsCacheItem::State()
	{
	return iState;
	}

#endif /* __TLSCACHEITEM_H__ */
