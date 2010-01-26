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

#include "tlscacheserver.h"

#include "tlscache.h"
#include "tlsclientserver.h"
#include "tlscachesession.h"

CTlsCacheServer* CTlsCacheServer::NewLC()
	{
	CTlsCacheServer* server = new (ELeave) CTlsCacheServer;
	CleanupStack::PushL(server);
	server->ConstructL();
	return server;
	}
	
CTlsCacheServer::CTlsCacheServer()
	: CServer2(EPriorityStandard, EUnsharableSessions)
	{
	}
	
void CTlsCacheServer::ConstructL()
	{
	iCache = CTlsCache::NewL();
	
	// create and start shutdown infrastructure
	iShutdown = new (ELeave) CShutdown;
	iShutdown->ConstructL();
	iShutdown->Start();
	
	StartL(KCacheServerName);
	}
	
void CTlsCacheServer::AddSession()
	{
	iSessionCount++;
	iShutdown->Cancel();
	}
	
void CTlsCacheServer::DropSession()
	{
	if (--iSessionCount == 0)
		{
		iShutdown->Start();
		}
	}
	
	
CSession2* CTlsCacheServer::NewSessionL(const TVersion& /*aVersion*/, const RMessage2& aMessage) const
	{
	return CTlsCacheSession::NewL(iCache->SegmentL(aMessage.SecureId()));
	}
	
CTlsCacheServer::~CTlsCacheServer()	
	{
	delete iCache;
	delete iShutdown;
	}

void CShutdown::RunL()
	{
	CActiveScheduler::Stop();
	}
