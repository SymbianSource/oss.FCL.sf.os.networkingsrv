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
// Interface for the CTlsCacheSession class
// 
//

/**
 @file 
 @internalTechnology
*/
 
#ifndef __TLSCACHESESSION_H__
#define __TLSCACHESESSION_H__

#include <e32base.h>
#include <x509cert.h>
#include "tlscacheitem.h"
#include "tlscacheserver.h"

class CTlsCacheSegment;
class CTlsCacheItemStatusWatcher;

class CTlsCacheSession : public CSession2
	{
public:
	static CTlsCacheSession* NewL(CTlsCacheSegment& aSegment);
	
	void ServiceL(const RMessage2& aMessage);
	void ServiceError(const RMessage2& aMessage, TInt aError);
	
private:
	CTlsCacheSession(CTlsCacheSegment& aSegment);
	~CTlsCacheSession();
	
	CTlsCacheServer& Server();
	
private:
	CTlsCacheSegment& iSegment;
	CTlsCacheItemStatusWatcher* iWatcher;
	CX509Certificate* iCert;
	TBool iSessionResponsibleForApproval;	
	};

class CTlsCacheItemStatusWatcher : public CActive
	{
public:
	static CTlsCacheItemStatusWatcher* NewL(CTlsCacheSegment& aSegment, const CX509Certificate& aCertificate);
	static CTlsCacheItemStatusWatcher* NewLC(CTlsCacheSegment& aSegment, const CX509Certificate& aCertificate);
	
	void StartL(const RMessage2& aMessage);
	
	void RunL();
	void DoCancel();
	void NotifyReset();
	
	TInt RunError(TInt aError);
	
private:
	CTlsCacheItemStatusWatcher();
	void ConstructL(CTlsCacheSegment& aSegment, const CX509Certificate& aCertificate);
	
private:
	CTlsCacheItem* iItem; // owned by the cache segment
	RMessage2 iMessage;
	
	};

inline CTlsCacheServer& CTlsCacheSession::Server()
	{
	return *static_cast<CTlsCacheServer*>(const_cast<CServer2*>(CSession2::Server())); 
	}
	
#endif /* __TLSCACHESESSION_H__ */
