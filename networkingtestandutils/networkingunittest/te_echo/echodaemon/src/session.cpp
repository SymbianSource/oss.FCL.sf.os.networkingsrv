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

#include <e32std.h>
#include "server.h"
#include "session.h"


_LIT(KSessionPanic,"CEchoDaemonSession panic");

CEchoDaemonSession* CEchoDaemonSession::NewL(CEchoDaemonServer& aServer)
	{
	CEchoDaemonSession* self = new (ELeave) CEchoDaemonSession(aServer);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
	}
	
CEchoDaemonSession::~CEchoDaemonSession()
	{
	iServer.DecreaseNumSessions();
	}
	
void CEchoDaemonSession::ServiceL(const RMessage2& aMessage)
	{
	TInt ret = KErrNone;
	TRAPD(err, ret = DispatchMessageL(aMessage));

	if(err != KErrNone)
		{
		ret = err;
		}
		
	aMessage.Complete(ret);
	}
	
CEchoDaemonSession::CEchoDaemonSession(CEchoDaemonServer& aServer)
	: iServer(aServer)
	{
	}

void CEchoDaemonSession::ConstructL()
	{
	iServer.IncreaseNumSessions();
	}


void CEchoDaemonSession::PanicClient(const RMessage2& aMessage, TInt aPanic)
	{
	aMessage.Panic(KSessionPanic, aPanic);
	}


TInt CEchoDaemonSession::DispatchMessageL(const RMessage2& aMessage)
	{
	TInt ret = KErrNotSupported;
	
	switch(aMessage.Function())
		{
		case EStartEchoDaemon:
			ret = Start(aMessage);
			break;
		case EStopEchoDaemon:
			ret = Stop(aMessage);
			break;
		case EStopAllEchoDaemons:
			ret = StopAll(aMessage);
			break;
		default:
			// Unknown function number - panic the client
			PanicClient(aMessage, EBadRequest);
		}
		
	return ret;
	}

TInt CEchoDaemonSession::Start(const RMessage2& aMessage)
	{
	TInt iap = aMessage.Int0();
	TInt prot = aMessage.Int1();
	TInt port = aMessage.Int2();
	
	TRAPD(ret, iServer.StartDaemonL(iap, prot, port));
	return ret;
	}

TInt CEchoDaemonSession::Stop(const RMessage2& aMessage)
	{
	TInt iap = aMessage.Int0();
	TInt prot = aMessage.Int1();
	TInt port = aMessage.Int2();
	
	TRAPD(ret, iServer.StopDaemonL(iap, prot, port));
	return ret;
	}

TInt CEchoDaemonSession::StopAll(const RMessage2& /*aMessage*/)
	{
	TRAPD(ret, iServer.StopAllDaemonsL());
	return ret;
	}
	
