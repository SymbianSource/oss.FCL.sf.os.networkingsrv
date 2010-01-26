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

#ifndef __ECHODAEMON_H
#define __ECHODAEMON_H

#include <e32base.h>

_LIT(KEchoDaemonServerName,"echodaemonsrv");

const TUid KEchoDaemonServUid = { 0x10283010 };
const TInt KEchoPort = 7;
static const TInt KProtocolInetDummy = 253;

// startup semaphore
_LIT(KServerSemaphore,"ServerSemaphore");

// A version must be specifyed when creating a session with the server
const TUint KEchoDaemonServMajorVersionNumber=0;
const TUint KEchoDaemonServMinorVersionNumber=0;
const TUint KEchoDaemonServBuildVersionNumber=1;

enum TEchoDaemonRqst {
	EStartEchoDaemon,
    EStopEchoDaemon,
	EStopAllEchoDaemons
	};

#define KTTMaxAsyncRequests		(4)
#define KTTDataTransferRequests	(3)
#define KTTDefaultMessageSlots	(KTTMaxAsyncRequests)


class REchoDaemonSession : public RSessionBase
	{
public:
	IMPORT_C REchoDaemonSession();
	IMPORT_C ~REchoDaemonSession();

	IMPORT_C TInt Connect();
	IMPORT_C TVersion Version() const;

	IMPORT_C TInt Start(TInt aIap, TInt aProtocol,TInt aPort);
	IMPORT_C TInt Stop(TInt aIap, TInt aProtocol,TInt aPort);
	IMPORT_C TInt StopAll();
	};

#endif 
