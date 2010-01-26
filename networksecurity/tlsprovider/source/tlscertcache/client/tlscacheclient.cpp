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

#include "tlscacheclient.h"

TInt StartServer()
	{
	TUidType serverUid(KNullUid, KNullUid, KCacheServerUid);
	RProcess server;
	TInt err = server.Create(KCacheServerExe, KNullDesC, serverUid);
	if (err != KErrNone)
		{
		return err;
		}
	TRequestStatus stat;
	server.Rendezvous(stat);
	if (stat != KRequestPending)
		{
		server.Kill(0);		// abort startup
		}
	else
		{
		server.Resume();	// logon OK - start the server
		}
	User::WaitForRequest(stat);		// wait for start or death
	err = (server.ExitType() == EExitPanic) ? KErrGeneral : stat.Int();
	server.Close();
	return err;
	}


EXPORT_C TInt RTlsCacheClient::Open(const CX509Certificate& aCert)
	{
	TInt retry = 2;
	for (;;) // loop forever, exit relies on internal count and unexpected errors happening
		{
		TInt err = CreateSession(KCacheServerName, TVersion(1, 0, 0), 2);
		if (err == KErrNone)
			{
			break;
			}
		if (err != KErrNotFound && err != KErrServerTerminated)
			{
			return err;
			}
		if (--retry==0)
			{
			return err;
			}
		err = StartServer();
		if (err != KErrNone && err != KErrAlreadyExists)
			{
			return err;
			}
		}
	
	TPtrC8 encoding(aCert.Encoding());
	TInt err = SendReceive(EOpen, TIpcArgs(&encoding));
	if (err != KErrNone)
		{
		Close();
		}
	return err;
	}
	
EXPORT_C TCacheEntryState RTlsCacheClient::GetStateL()
	{
	TCacheEntryState state;
	TPckg<TCacheEntryState> pkg(state);
	User::LeaveIfError(SendReceive(EGetEntryStatus, TIpcArgs(&pkg)));
	return state;
	}

EXPORT_C void RTlsCacheClient::SetStateL(TCacheEntryState aState)
	{
	TPckgC<TCacheEntryState> pkg(aState);
	User::LeaveIfError(SendReceive(ESetEntryStatus, TIpcArgs(&pkg)));
	}
	
EXPORT_C void RTlsCacheClient::RequestNotify(TRequestStatus& aStatus)
	{
	SendReceive(ENotifyChange, aStatus);
	}
	
EXPORT_C void RTlsCacheClient::Cancel()
	{
	SendReceive(ECancel);
	}
