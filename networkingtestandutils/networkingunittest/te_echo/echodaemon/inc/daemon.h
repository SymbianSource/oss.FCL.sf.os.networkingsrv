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

#ifndef __DAEMON_H
#define __DAEMON_H

#include <networking/echodaemon.h>
#include <es_sock.h>

class CEchoDaemon : public CBase
	{
public:
	static CEchoDaemon* NewL(TInt aIapId, TInt aProtocol,TInt aPort);
	~CEchoDaemon();
		
	void StartL();
	void StopL(TBool aMoreThanOne);
	
	virtual void RunDaemonL() = 0;

	TInt IapId() { return iIapId; }
	TInt Protocol() { return iProtocol; }
	TInt Port() { return iPort;}

	static TInt DaemonThreadFunction(TAny *aThis);
	
protected:
	CEchoDaemon(TInt aIapId, TInt aProtocol, TInt aPort);
	void ConstructL();

private:
	TInt iIapId;
	TInt iProtocol;
	RThread iThread;
	TInt iPort;

protected:
	RSocket iSocket, iBlankSocket;
	RSocketServ iSocketServ;
	RConnection iConnection;
	};

class CEchoTcpDaemon : public CEchoDaemon
	{
public:
	CEchoTcpDaemon(TInt aIapId, TInt aProtocol, TInt aPort): CEchoDaemon(aIapId, aProtocol,aPort)
		{}
		
	void RunDaemonL();
	};

class CEchoUdpDaemon : public CEchoDaemon
	{
public:
	CEchoUdpDaemon(TInt aIapId, TInt aProtocol,TInt aPort) : CEchoDaemon(aIapId, aProtocol,aPort)
		{}
	
	void RunDaemonL();
	};

#endif
