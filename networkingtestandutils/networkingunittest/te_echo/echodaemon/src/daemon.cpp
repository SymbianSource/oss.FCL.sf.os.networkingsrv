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

#include "daemon.h"
#include <in_sock.h>
#include <commdbconnpref.h>
CEchoDaemon* CEchoDaemon::NewL(TInt aIapId, TInt aProtocol, TInt aPort)
	{
	CEchoDaemon *daemon = NULL;
	
	switch (aProtocol)
		{
		case KProtocolInetTcp:
			daemon = new (ELeave) CEchoTcpDaemon(aIapId, aProtocol, aPort);
			break;
		case KProtocolInetUdp:
		case KProtocolInetDummy:
			daemon = new (ELeave) CEchoUdpDaemon(aIapId, aProtocol, aPort);
			break;
		default:
			User::Leave(KErrNotSupported);
			break;
		}
	CleanupStack::PushL(daemon);
	daemon->ConstructL();
	CleanupStack::Pop(daemon);

	return daemon;
	}

CEchoDaemon::CEchoDaemon(TInt aIapId, TInt aProtocol, TInt aPort)
	: iIapId(aIapId), iProtocol(aProtocol), iPort(aPort)
	{}

_LIT(KThreadName, "EchoServerIap%dProto%dPort%d");
const TInt KHeapSize = 0x2000;
const TInt KThreadNameLen = 0x20;

TInt CEchoDaemon::DaemonThreadFunction(TAny *aThis)
	{
	CEchoDaemon* daemon = reinterpret_cast<CEchoDaemon*>(aThis);
	TRAPD(err, daemon->RunDaemonL());
	return err;
	}

void CEchoDaemon::ConstructL()
	{
	TBuf<KThreadNameLen> name;
	name.Format(KThreadName, iIapId, iProtocol,iPort);
	
	TInt ret = iThread.Create(name, DaemonThreadFunction, KDefaultStackSize, KHeapSize, KHeapSize, reinterpret_cast<TAny*>(this));
	User::LeaveIfError(ret);
   	}	

CEchoDaemon::~CEchoDaemon()
	{
	iConnection.Close();
	iSocketServ.Close();

	iThread.Close();
	}
		
void CEchoDaemon::StartL()
	{
	User::LeaveIfError(iSocketServ.Connect());
	User::LeaveIfError(iSocketServ.ShareAuto());
	User::LeaveIfError(iConnection.Open(iSocketServ));

	TCommDbConnPref prefs;
	prefs.SetIapId(IapId());
	User::LeaveIfError(iConnection.Start(prefs));
	
	iThread.Resume();
	}

void CEchoDaemon::StopL(TBool aMoreThanOne)
	{
	iThread.Kill(KErrCancel);

	// make sure sockets have been cleaned up
	if (iBlankSocket.SubSessionHandle())
		iBlankSocket.CancelAll();
	iBlankSocket.Close();
	if (iSocket.SubSessionHandle())
		iSocket.CancelAll();
	iSocket.Close();
	
	if (!aMoreThanOne)
	User::LeaveIfError(iConnection.Stop());
	}

const TInt KListenQSize = 5;

void CEchoTcpDaemon::RunDaemonL()
	{
	User::LeaveIfError(iSocket.Open(iSocketServ, KAfInet,  KSockStream, KProtocolInetTcp, iConnection));

	TInetAddr addr;
	addr.SetPort(Port());
	User::LeaveIfError(iSocket.Bind(addr));
	User::LeaveIfError(iSocket.Listen(KListenQSize));

	TBuf8<1500> buf;
	TRequestStatus status;
	while (1)
		{
		iBlankSocket.Open(iSocketServ);
		iSocket.Accept(iBlankSocket, status);
		User::WaitForRequest(status);

		if (status.Int() != KErrNone)
			break;
		
		while (1)
			{
			TSockXfrLength len;
			iBlankSocket.RecvOneOrMore(buf, 0, status, len);
			User::WaitForRequest(status);
			if (status.Int() != KErrNone)
				break;

			iBlankSocket.Send(buf, 0, status, len);
			User::WaitForRequest(status);
			if (status.Int() != KErrNone)
				break;
			}
		iBlankSocket.Close();
		}
	iSocket.Close();
	}

void CEchoUdpDaemon::RunDaemonL()
	{
	User::LeaveIfError(iSocket.Open(iSocketServ, KAfInet,  KSockDatagram, KProtocolInetUdp, iConnection));

	TInetAddr serAddr,cliAddr;
	serAddr.SetPort(Port());
	User::LeaveIfError(iSocket.Bind(serAddr));
	
	TBuf8<1500> buf;
	TRequestStatus status;
	while (1)
		{
		// receiving
		buf.FillZ();
		iSocket.RecvFrom(buf,cliAddr,0,status);
		User::WaitForRequest(status);
		if (status.Int() != KErrNone)
			break;

		// setting remote server
		iSocket.SendTo(buf,cliAddr,0,status);
		User::WaitForRequest(status);
		if (status.Int() != KErrNone)
			break;
		}
	iSocket.Close();
	}
