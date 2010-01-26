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

#include "echodaemon.h"

TInt StartServer()
	{
	TInt ret = KErrNone;

	const TUidType serverUid(KNullUid, KNullUid, KEchoDaemonServUid);
	RProcess server;
	ret = server.Create(KEchoDaemonServerName, _L(""), serverUid);
	
	if(ret != KErrNone)
		return ret;
	
	TRequestStatus serverDiedRequestStatus;
	server.Rendezvous(serverDiedRequestStatus);
	
	if(serverDiedRequestStatus != KRequestPending)
		{
		server.Kill(0); // abort the startup here
		}
	else
		{
		server.Resume(); // start server
		}

    User::WaitForRequest(serverDiedRequestStatus);
    TInt exitReason =(server.ExitType()==EExitPanic) ? KErrGeneral : serverDiedRequestStatus.Int();
	server.Close();
	return exitReason;
	}

EXPORT_C REchoDaemonSession::REchoDaemonSession()
	{}


EXPORT_C REchoDaemonSession::~REchoDaemonSession()
	{}


EXPORT_C TInt REchoDaemonSession::Connect()
	{
	TInt retry = 2; //Attempt connect twice then give up
	TInt ret;
	for(;;)
		{
		ret = CreateSession(KEchoDaemonServerName, Version(), KTTDefaultMessageSlots);
		if	(ret != KErrNotFound && ret != KErrServerTerminated)
			break;
		
		if	(--retry == 0)
			break;
		
		ret = StartServer();
		if	(ret != KErrNone && ret != KErrAlreadyExists)
			break;
		}

	return ret; 
	}


EXPORT_C TVersion REchoDaemonSession::Version () const
	{
	return TVersion(KEchoDaemonServMajorVersionNumber,
		            KEchoDaemonServMinorVersionNumber,
                    KEchoDaemonServBuildVersionNumber);
	}
	

EXPORT_C TInt REchoDaemonSession::Start(TInt aIap, TInt aProtocol,TInt aPort)
	{
	TIpcArgs args;
	args.Set(0, aIap);
	args.Set(1, aProtocol);
	args.Set(2, aPort);
	return SendReceive(EStartEchoDaemon, args);
	}

EXPORT_C TInt REchoDaemonSession::Stop(TInt aIap, TInt aProtocol,TInt aPort)
	{
	TIpcArgs args;
	args.Set(0, aIap);
	args.Set(1, aProtocol);
	args.Set(2, aPort);

	return SendReceive(EStopEchoDaemon, args);
	}

EXPORT_C TInt REchoDaemonSession::StopAll()
	{
	TIpcArgs args;
	return SendReceive(EStopAllEchoDaemons, args);
	}

