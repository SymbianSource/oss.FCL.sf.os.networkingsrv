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

#ifndef __SERVER_H
#define __SERVER_H

#include <e32cons.h>
#include <s32file.h>
#include <d32dbms.h>

#include <networking/echodaemon.h>
#include "daemon.h"


// Default heap size for server thread.
const TUint KAppDefaultHeapSize=0x10000;


// ENUMS

// Server panic codes
enum TEchoDaemonSrvPanic
	{
	ESvrCreateServer,
	ESvrStartServer,
	ESvrFileServer,
	EMainSchedulerError,
	EBadRequest,
	};


// CLASS DECLARATIONS
class CEchoDaemonServer : public CServer2
	{
public:		// Static
	static TInt ThreadFunction();
	static void ThreadFunctionL();
	static void PanicServer(TEchoDaemonSrvPanic aPanic);	
		
public:		// Construction
	static CEchoDaemonServer* NewLC();
	
public:		// Destruction
	~CEchoDaemonServer();
	
public:
	virtual CSession2* NewSessionL(const TVersion& aVersion, const RMessage2& aMessage) const;
	void IncreaseNumSessions();
	void DecreaseNumSessions();
	void StartDaemonL(TInt iIapId, TInt iProtocol, TInt aPort);
	void StopDaemonL(TInt iIapId, TInt iProtocol, TInt aPort);
	void StopAllDaemonsL();
	void IsIapShared(TInt aIapId,TBool& moreThanOne);
	
	TInt RunError(TInt aError);
	
private:	// Construction
	CEchoDaemonServer(TInt aPriority);
	void ConstructL();
	
private:	// Data members
	RPointerArray<CEchoDaemon> iDaemons;
	TInt iNumSessions;
};


#endif	// __SERVER_H
