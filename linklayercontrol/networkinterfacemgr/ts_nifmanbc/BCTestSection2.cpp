// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "BCTestSection2.h"
#include <comms-infras/nifif.h>
#include <in_sock.h>
#include <CommDbConnPref.h>
#include <es_enum.h>

CTestStep2_1::CTestStep2_1() 
//
// C'tor
//
	{

	_LIT(KTestStepName, "Test2_1");
	iTestStepName = KTestStepName();
	}

enum TVerdict CTestStep2_1::doTestStepL()
//
// Test that RNif::Stop() does not interfere with other connections
//
	{

	TInt err(KErrNone);

	RSocketServ ss;
	err = ss.Connect();
	TESTEL(err==KErrNone, err);
	CleanupClosePushL(ss);

	Log(_L("Starting implicit connection using RSocket::SendTo()"));

	RSocket sock;
	err = sock.Open(ss, KAfInet, KSockDatagram, KProtocolInetUdp);
	TESTEL(err==KErrNone, err);
	CleanupClosePushL(sock);

	TInetAddr local(KInetAddrAny, KInetPortAny);
	err = sock.Bind(local);
	TESTEL(err==KErrNone, err);

	TInetAddr dest(INET_ADDR(1, 2, 3, 4), 9);  // IP address does not matter here - we're not expecting a response
	TBuf8<0x100> buf;
	buf.Fill(TChar('A'), buf.MaxLength ());

	TRequestStatus status;
	sock.SendTo(buf, dest, 0, status);
	User::WaitForRequest(status);
	TESTE(status.Int()==KErrNone, status.Int());

	RConnection conn;
	err = conn.Open(ss);
	TESTEL(err==KErrNone, err);
	CleanupClosePushL(conn);

	TUint count(0);
	err = conn.EnumerateConnections(count);
	TESTE(err==KErrNone, err);
	TEST(count==1);
	Log(_L("%d connections active"), count);

	TConnectionInfoBuf info;
	err = conn.GetConnectionInfo(count, info);
	TESTE(err==KErrNone, err);
	TInt implicitIAP = info().iIapId;
	Log(_L("Implicit connection has IAP ID %d"), implicitIAP);

	Log(_L("Starting explicit connection using RConnection::Start()"));
	TCommDbConnPref pref;
	pref.SetIapId(2);
	err = conn.Start(pref);
	TESTEL(err==KErrNone, err);

	err = conn.EnumerateConnections(count);
	TESTE(err==KErrNone, err);
	TEST(count==2);
	Log(_L("%d connections active"), count);

	err = conn.GetConnectionInfo(count, info);
	TESTE(err==KErrNone, err);
	TInt explicitIAP = info().iIapId;
	Log(_L("Explicit connection has IAP ID %d"), explicitIAP);

	Log(_L("Calling RNif::Stop()"));


	RNif            nif;
	TNifProgressBuf prog;
	
	err = nif.Open();
	TESTEL(err==KErrNone, err);
	
	nif.ProgressNotification(prog, status);
		
	TNifAgentInfo agentInfo;
	err = nif.AgentInfo(agentInfo);

	TESTEL(err==KErrNone, err);
	TBool isNetworkActive;
	err = nif.NetworkActive(isNetworkActive);
	Log(_L("Network active is %d"), isNetworkActive);

	Log(_L("Disabling timers for NIF"));
        err = nif.DisableTimers(ETrue);
	TESTEL(err==KErrNone, err);
	nif.Stop();

    Log(_L("Waiting for implicit connection to disconnect"));

    for(;;)
    {
        User::WaitForRequest(status);//wait on progress notification

        //-- we must subscribe to notification ASAP in order not to miss 
        //-- status change
        nif.ProgressNotification(prog, status);
        
        //-- status may have changed even during ProgressNotification call
        nif.Progress(prog());
        TInt nConnStage = prog().iStage;
        TInt nConnErr   = prog().iError;
        
        if(nConnStage == KAgentUninitialised)
                break;
        
        Log(_L("Stage %d, error %d"), nConnStage, nConnErr);            
            
    }
	 

	nif.Close();

	err = conn.EnumerateConnections(count);
	TESTE(err==KErrNone, err);
	TEST(count==1);
	Log(_L("%d connections active"), count);

	err = conn.GetConnectionInfo(count, info);
	TESTE(err==KErrNone, err);
	TInt remainingIAP = info().iIapId;
	Log(_L("Remaining connection has IAP ID %d"), remainingIAP);

	TEST(explicitIAP==remainingIAP);

	Log(_L("Stopping connection using RConnection::Stop()"));
	conn.Stop();
	sock.Close();
	CleanupStack::Pop();  // sock

	conn.Close();
	CleanupStack::Pop();  // conn

	ss.Close();
	CleanupStack::Pop();  // ss

	return iTestStepResult;
	}

CTestStep2_2::CTestStep2_2() 
//
// C'tor
//
	{

	_LIT(KTestStepName, "Test2_2");
	iTestStepName = KTestStepName();
	}

enum TVerdict CTestStep2_2::doTestStepL()
//
// Test the RGenericAgent connection idle timers
//
	{

	TInt err(KErrNone);

	RTimer timer;
	err = timer.CreateLocal();
	TESTEL(err==KErrNone, err);
	CleanupClosePushL(timer);

	RSocketServ ss;
	err = ss.Connect();
	TESTEL(err==KErrNone, err);
	CleanupClosePushL(ss);

	RConnection conn;
	err = conn.Open(ss);
	TESTEL(err==KErrNone, err);

	RGenericAgent agent;
	err = agent.Open();
	TESTEL(err==KErrNone, err);
	CleanupClosePushL(agent);

	Log(_L("Starting implicit connection using RSocket::SendTo()"));

	RSocket sock;
	err = sock.Open(ss, KAfInet, KSockDatagram, KProtocolInetUdp);
	TESTEL(err==KErrNone, err);
	CleanupClosePushL(sock);

	TInetAddr local(KInetAddrAny, KInetPortAny);
	err = sock.Bind(local);
	TESTEL(err==KErrNone, err);

	TInetAddr dest(INET_ADDR(1, 2, 3, 4), 9);  // IP address does not matter here - we're not expecting a response
	TBuf8<0x100> buf;
	buf.Fill(TChar('A'), buf.MaxLength ());

	TRequestStatus status;
	sock.SendTo(buf, dest, 0, status);
	User::WaitForRequest(status);
	TESTE(status.Int()==KErrNone, status.Int());

	TUint32 socketActivityTO;
	err = agent.GetActiveIntSetting(TPtrC(MODEM_BEARER), TPtrC(LAST_SOCKET_ACTIVITY_TIMEOUT), socketActivityTO);
	TESTEL(err==KErrNone, err);
	
	agent.Close();
	CleanupStack::Pop(2);    // socket and agent
	CleanupClosePushL(sock); // put socket back - it is still in use

	// set timer for the last socket activity timeout (with a 5% margin)
	const TUint32 KMarginFactor = 20; //  5% = 1/20
	TUint32 timeout = (socketActivityTO + (socketActivityTO/KMarginFactor))*1000000;
	TRequestStatus timerStatus;
	timer.After(timerStatus, timeout);
	User::WaitForRequest(timerStatus);
	TESTE(timerStatus.Int()==KErrNone, timerStatus.Int());

	TUint count(0);
	err = conn.EnumerateConnections(count);
	TESTE(err==KErrNone, err);
	TEST(count==0);
	Log(_L("%d connections active"), count);

	sock.Close();
	CleanupStack::Pop();  // sock

	ss.Close();
	CleanupStack::Pop();  // ss

	timer.Close();
	CleanupStack::Pop();  // timer

	return iTestStepResult;
	}

CTestStep2_3::CTestStep2_3() 
//
// C'tor
//
	{

	_LIT(KTestStepName, "Test2_3");
	iTestStepName = KTestStepName();
	}

enum TVerdict CTestStep2_3::doTestStepL()
//
// Test the RNifMonitor and KErrNone tests
//
    {
    TSoIfConnectionInfo connectionInfo, connectionInfoCopy;

    connectionInfo.iNetworkId = 170;
    connectionInfo.iIAPId = 55;
    connectionInfoCopy = connectionInfo;

	// Verify the == operator is implemented correctly
    TESTE(connectionInfoCopy == connectionInfo, KErrGeneral);

    TInt err(KErrNone);
    RNifMonitor mon;
    
    err = mon.Open(_L("genconn.agt"));
    TESTE(err == KErrNone, err);
    mon.Close();

    return EPass;
    }

CTestStep2_4::CTestStep2_4() 
//
// C'tor
//
	{

	_LIT(KTestStepName, "Test2_4");
	iTestStepName = KTestStepName();
	}

enum TVerdict CTestStep2_4::doTestStepL()
//
// Test unsupported nif methods
//
    {
    // MNifIfNotify* notify = (MNifIfNotify*) 0x12345678;  // value doesn't matter here as the we check that this method leaves
    //CNifIfBase* nif;
    // Implement this when we support panics
    TRAPD(err, User::Leave(KErrNotSupported)); //nif = Nif::CreateInterfaceL(_L("PPP"), notify));

    if(err != KErrNotSupported)
        {
        return EFail;
        }

    return EPass;
    }

