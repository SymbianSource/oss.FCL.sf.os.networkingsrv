// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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

/**
 @file T_ipip.CPP
*/

#include <e32test.h>

#include <rsshared.h>

#include <es_sock.h>
#include <in_sock.h>

#include "connectiontester.h" // gets the active object

#include "ipip6.h"

static RTest tester(_L("IPIP test"));


LOCAL_C TInt AddInterfaceAndRoute(RSocketServ& aServer, const TInetAddr& aIfAddr, 
						  const TInetAddr& aGatewayAddr, CLogger& aLogger)
	{
	RSocket socket;
	TInt err = socket.Open(aServer, KAfInet, KSockDatagram, KProtocolInetUdp);
	if(err)
		return err;
	
	TSoInet6InterfaceInfo opt;
    _LIT(KIfName, "ipcp::comm::0");
    opt.iName     = KIfName;
    opt.iTag      = KIfName;
    opt.iState    = EIfUp;    
    opt.iDoState  = 1;
    opt.iDoId     = 1;
    opt.iDoPrefix = 1;
    opt.iAlias    = 0;
    opt.iDelete   = 0;
	opt.iDoAnycast= 0;
	opt.iDoProxy  = 0;
    opt.iAddress  = aIfAddr;
	opt.iAddress.SetScope(1);
    opt.iDefGate.SetFamily(0);
    opt.iNetMask.SetFamily(0);
    opt.iMtu         = 0;
    opt.iSpeedMetric = 0;

    TBuf<40> txt_addr;
    aIfAddr.Output(txt_addr);
    aLogger.Log(_L("Adding address to Interface %S with address %S"), &KIfName, &txt_addr);
	TPckg<TSoInet6InterfaceInfo> optBuf(opt);
	err = socket.SetOpt(KSoInetConfigInterface, KSolInetIfCtrl, optBuf);
	aLogger.Log(_L("The virtual Interface was added with result %d"), err);
	if (err)
		{
		socket.Close();
	    return err;
		}

	
	TSoInetRouteInfo rtOpt;
	rtOpt.iType = ERtUser;
	rtOpt.iState = ERtReady;
	rtOpt.iMetric = 0;
	rtOpt.iIfAddr = aIfAddr;
	rtOpt.iGateway = aGatewayAddr;
	rtOpt.iDstAddr = aGatewayAddr;
	rtOpt.iNetMask.SetAddress(INET_ADDR(255,255,255,255));
	
	TBuf<40> txt_addr_dest;
    aGatewayAddr.Output(txt_addr_dest);
    aLogger.Log(_L("Adding Route to %S on interface %S"), &txt_addr_dest, &txt_addr);
	TPckg<TSoInetRouteInfo> rtOptBuf(rtOpt);
	err = socket.SetOpt(KSoInetAddRoute, KSolInetRtCtrl, rtOptBuf);
	aLogger.Log(_L("The Route was added with result %d"), err);
	
	socket.Close();
    return err;
	}

void DoItL()
	{
	__UHEAP_MARK;
	// Load Protocol
	CLogger *logger = new(ELeave) CLogger(tester);
	CleanupStack::PushL(logger);
/*
	//TEMP+
	RSocketServ server;
	logger->Log(_L("WELCOME -> New Version\n\nBefore connecting with RSocketServ::Connect()\n"));
	User::LeaveIfError(server.Connect());
	CleanupClosePushL(server);
	logger->Log(_L("The RSocketServ.Connect() was started\n"));
//TEMP-
*/

	RSocketServ server;
	User::LeaveIfError(server.Connect());
	CleanupClosePushL(server);

	TRequestStatus status;
	server.StartProtocol(KAfInetIPIPHookEx, KSockDatagram, KProtocolInetIPIPHookEx, status);
	User::WaitForRequest(status);
	User::LeaveIfError(status.Int());

	// Create the policy -- All these address need to  be modified per use case
	//TInetAddr src(INET_ADDR(192, 168, 21, 250), 7);
	TInetAddr src(INET_ADDR(172, 31, 0, 2), 7);
	// Because port matching requires same address families, and all stack addresses are v4Mapped
	src.ConvertToV4Mapped();
	TInetAddr dest(INET_ADDR(172,31,0,1), 7);
	dest.ConvertToV4Mapped();

	//TInetAddr tunnSrc(INET_ADDR(192, 168, 21, 250), 7);
	TInetAddr tunnSrc(INET_ADDR(192, 168, 12, 10), 7);
	tunnSrc.ConvertToV4Mapped();
	//TInetAddr tunnDest(INET_ADDR(192, 168, 40, 2), 7);
	TInetAddr tunnDest(INET_ADDR(192, 168, 10, 2), 7);
	tunnDest.ConvertToV4Mapped();
	
	// The policy specifies the addresses to be matched
	TIPIPPacketSpec policy(src, dest);
	// The action spec specifies outer pair
	TIPIPActionSpec action(tunnSrc, tunnDest);
	TIPIPPolicyMsg msg(policy, action, EIPIPAddPolicy);
	TPckg<TIPIPPolicyMsg> msgPtr(msg);


	RSocket sock;
	TInt err = sock.Open(server, KProtocolIPIPName);
	User::LeaveIfError(err);
	CleanupClosePushL(sock);
	sock.Ioctl(1, status, &msgPtr);
	User::WaitForRequest(status);
	logger->Log(_L("IPIP policy was Added with result %d"), status.Int());
	User::LeaveIfError(status.Int());

	/*
//TEMP-
	TInetAddr dest(INET_ADDR(192,168,10,2), 7);
	dest.ConvertToV4Mapped();
	RSocket sock;
	//IMPORT_C TInt Open(RSocketServ& aServer,TUint addrFamily,TUint sockType,TUint protocol);
	TInt err = sock.Open(server, KAfInet, KSockStream, KProtocolInetTcp);
	logger->Log(_L("The RSocket::Open() was started with result %d"), err);

	//TInt err = sock.Open(server, KProtocolIPIPName);
	User::LeaveIfError(err);
	CleanupClosePushL(sock);
//TEMP-
*/
	// Create the active objects
	CConnTester* tester = CConnTester::NewLC(logger);
	tester->StartConenction();
	tester->GetProgress();
	// The scheduler starts, and completes when the AOs are done
	CActiveScheduler::Start();

	err = tester->GetResult();
	logger->Log(_L("The Conenction was started with result %d"), err);
	tester->PrintRoutingTableL();

	// Add a new address to the interface
	err = AddInterfaceAndRoute(server, src, dest, *logger);
	logger->Log(_L("The virtual Interface and route were added with result %d"), err);
	tester->PrintRoutingTableL();

	if (!err)
		{
		TInt Port = 7;
		tester->StartTransferL(dest, Port, CSender::EReadWrite);

		CActiveScheduler::Start();

		err = tester->GetResult();
		logger->Log(_L("The data was tranfered with result %d"), err);

		TRAP(err, tester->GetStatsL());
		logger->Log(_L("Read stats with result %d"), err);
		}

	tester->StopConnection();
	err = tester->GetResult();
	logger->Log(_L("The Conenction was stopped with result %d"), err);

	CleanupStack::PopAndDestroy(4); // server, tester, CLogger, sock
	__UHEAP_MARKEND;
	}

int E32Main(void)
	{
	tester.SetLogged(EFalse);
	tester.Title();
	tester.Start(_L("initialising"));
	
	_LIT(KPhbkSyncCMI, "phbsync.cmi");
	TInt err=StartC32WithCMISuppressions(KPhbkSyncCMI);
	User::LeaveIfError(err);

	CTrapCleanup* TheTrapCleanup=CTrapCleanup::New();

	CActiveScheduler* Scheduler=new (ELeave) CActiveScheduler;
	CActiveScheduler::Install(Scheduler);
	
	TRAP(err, DoItL());
	
	delete Scheduler;
	delete TheTrapCleanup;

	tester.Printf(_L("Done!!!"));
	tester.Getch();
	return 0;
	}
