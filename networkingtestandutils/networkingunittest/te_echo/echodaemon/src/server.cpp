// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <e32cmn.h>
#include <e32std.h>
#include <e32svr.h>

#include <networking/echodaemon.h>
#include "server.h"
#include "session.h"


TInt CEchoDaemonServer::ThreadFunction()
	{
	__UHEAP_MARK;
	CTrapCleanup* cleanupStack = CTrapCleanup::New();
	
	TInt ret = KErrNoMemory;
	if	(cleanupStack)
		{
		TRAP(ret, ThreadFunctionL());
		delete cleanupStack;
		}

	__UHEAP_MARKEND;
	return KErrNone;
	}


void CEchoDaemonServer::ThreadFunctionL()
	{
	CActiveScheduler* scheduler = new (ELeave) CActiveScheduler;
	CleanupStack::PushL(scheduler);
	CActiveScheduler::Install(scheduler);

	CEchoDaemonServer::NewLC();
	
	RProcess::Rendezvous(KErrNone);

	CActiveScheduler::Start();

    CleanupStack::PopAndDestroy(2, scheduler);
	}

void CEchoDaemonServer::PanicServer(TEchoDaemonSrvPanic aPanic)
	{
	User::Panic(KEchoDaemonServerName, aPanic);
	}
	
CEchoDaemonServer* CEchoDaemonServer::NewLC()
	{
	CEchoDaemonServer* self = new (ELeave) CEchoDaemonServer(EPriorityNormal);
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
	}

CEchoDaemonServer::~CEchoDaemonServer()
	{
	iDaemons.Close();
	}

CSession2* CEchoDaemonServer::NewSessionL(const TVersion& aVersion, const RMessage2& /*aMessage*/) const
	{
	TVersion serverVersion(	KEchoDaemonServMajorVersionNumber,
							KEchoDaemonServMinorVersionNumber,
							KEchoDaemonServBuildVersionNumber);
	if(!User::QueryVersionSupported(serverVersion, aVersion))
		{
		User::Leave(KErrNotSupported);
		}

	return CEchoDaemonSession::NewL(*(const_cast<CEchoDaemonServer*>(this)));
	}

void CEchoDaemonServer::IncreaseNumSessions()
	{
	iNumSessions++;
	}


void CEchoDaemonServer::DecreaseNumSessions()
	{
	iNumSessions--;
	if(iNumSessions <= 0 && iDaemons.Count() == 0)
		{
		CActiveScheduler::Stop();
		}
	}

void CEchoDaemonServer::StartDaemonL(TInt aIapId, TInt aProtocol, TInt aPort)
	{
	CEchoDaemon* daemon = CEchoDaemon::NewL(aIapId, aProtocol, aPort);
	iDaemons.AppendL(daemon);
	daemon->StartL();
	}

void CEchoDaemonServer::StopDaemonL(TInt aIapId, TInt aProtocol,TInt aPort)
	{
	for (TInt i = 0; i < iDaemons.Count(); i++)
		{
		if (iDaemons[i]->IapId() == aIapId
			&& iDaemons[i]->Protocol() == aProtocol 
			&& iDaemons[i]->Port() == aPort)
			{
			CEchoDaemon *daemon = iDaemons[i];
			// check whether there are others to use the same IAP
			// if it is, it does not clase RConnection.
			TBool moreThanOne = EFalse;
			IsIapShared(aIapId,moreThanOne);
			daemon->StopL(moreThanOne);
			iDaemons.Remove(i);
			delete daemon;
			return;
			}
		}
	User::Leave(KErrNotFound);
	}

void CEchoDaemonServer::StopAllDaemonsL()
	{
	for (TInt i = 0; i < iDaemons.Count(); i++)
		{
		CEchoDaemon *daemon = iDaemons[i];
		// check whether there are others to use the same IAP
		// if it is, it does not clase RConnection.
		TBool moreThanOne = EFalse;
		IsIapShared(iDaemons[i]->IapId(),moreThanOne);
		daemon->StopL(moreThanOne);
		iDaemons.Remove(i);
		delete daemon;
		}
	}

TInt CEchoDaemonServer::RunError(TInt aError)
	{
	Message().Complete(aError);
    
	ReStart();

	return KErrNone;
	}


CEchoDaemonServer::CEchoDaemonServer(TInt aPriority)
	: CServer2(aPriority)
	{
	}
	

void CEchoDaemonServer::ConstructL()
	{
	Start(KEchoDaemonServerName);
	}

void CEchoDaemonServer::IsIapShared(TInt aIapId,TBool& moreThanOne)
	{  //return true when there is one to share the same IAP
	TInt num=0;
	for (TInt i = 0; i < iDaemons.Count(); i++)
		{
			if (iDaemons[i]->IapId() == aIapId)
				{
				num++;
				if(num>1) // more than one 
					{
					moreThanOne=ETrue;
					break;
					}
				}
		}			
	}
