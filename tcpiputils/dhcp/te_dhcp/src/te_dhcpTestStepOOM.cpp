// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Basic Server Interaction Tests (test doc 4.1)
// 
//

/**
 @file
*/
#include "te_dhcpTestStepOOM.h"
#include <test/testexecutelog.h>
#include <commdbconnpref.h>
#include <in_sock.h>
#include <nifman.h>
#include "te_dhcpTestServer.h"
#include <comms-infras/startprocess.h>
#include "Te_TestDaemonClient.h"

#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS

#include <networking/dhcpconfig.h>

#endif // SYMBIAN_NETWORKING_DHCP_MSG_HEADERS

TVerdict CDhcpTestStepOOM_1::doTestStepL()
/**Checks OOM condition in DHCP during RConnection::Start(...) client request.*/
{
    #ifndef __DHCP_HEAP_CHECK_CONTROL
    	INFO_PRINTF1(_L("This test step is disabled for release build."));
        SetTestStepResult(EInconclusive);
	    return TestStepResult();
    #else

    INFO_PRINTF1(_L("DHCP heap check test during RConnection::Start(...) client request"));

	SetTestStepResult(EFail);
	TBool minorProblems = EFalse;
	
	//Start DHCP server.
	RStartProcess processStart;
	TInt err = processStart.Start(KDHCPProcessName);
	TESTEL( err == KErrNone, err);

	//Connect to the server. We will use this connection to issue heap debug commands
	RTestDaemonClient debugSession;
	debugSession.CreateSession();
	CleanupClosePushL(debugSession);

	//Connect to Socket Server
	RSocketServ eSock;
	err = eSock.Connect();
	TESTEL( err == KErrNone, err);
	CleanupClosePushL(eSock);
	
	//Prepare everything for RConnection.Start() that will be later issued in loop.
	RConnection conn;
	err = conn.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(conn);

	//Set debug option to force discovery process.
	INFO_PRINTF1(_L("Set debug option to force discovery process..."));
	TRequestStatus stat;
    TDhcpMemDbgParamBuf dhcpMemDbgParamBuf;
	dhcpMemDbgParamBuf() = KDHCP_ForceDiscovery;
	debugSession.Ioctl(KCOLConfiguration, KDhcpMemDbgIoctl|KDHCP_DbgFlags, stat, &dhcpMemDbgParamBuf);
	User::WaitForRequest(stat);
	TESTEL(stat == KErrNone, stat.Int());
  
	//Mark heap
    INFO_PRINTF1(_L("DHCP heap mark"));
	dhcpMemDbgParamBuf()=0;
    debugSession.Ioctl(KCOLConfiguration, KDhcpMemDbgIoctl|KDHCP_DbgMarkHeap, stat, &dhcpMemDbgParamBuf);
    User::WaitForRequest(stat);
    TESTEL(stat == KErrNone, stat.Int());

	//Init local data for loop
	TInt failAfter = 0; // start with success case (just to detect memory leaks)
	TInt KErrNoneReturned = 0;

	//Loop goes till 10 KErrNone are returned in row (or test harness times out)
	while (KErrNoneReturned < 10)
		{
		if(failAfter == 0)
			INFO_PRINTF1(_L("******* TEST FOR NO SIMULATED ALLOC. FAILS i.e. memory leak test ********"));
		else
			INFO_PRINTF2(_L("******* TEST WHEN ALLOC. NUMBER %d IN DHCP SERVER FAILS ********"),failAfter);

		//Set Heap to fail after (failAfter) tries
		INFO_PRINTF1(_L("Setting FailNext debug in heap..."));
        dhcpMemDbgParamBuf() = failAfter; //-- number of expected allocated cells
		debugSession.Ioctl(KCOLConfiguration, KDhcpMemDbgIoctl|KDHCP_DbgFailNext, stat, &dhcpMemDbgParamBuf);
		User::WaitForRequest(stat);
		TESTEL(stat == KErrNone, stat.Int());

		//Start Connection
		INFO_PRINTF1(_L("Starting connection..."));
		TInt ret = conn.Start(iConnPrefs); // see test script for IAP used

		INFO_PRINTF2(_L("RConnection::Start returned %d"),ret);

		if (KErrNone == ret)
			{
			//Action is completed succesfuly.Remove FailNext Heap mark. Otherwise, it might panic on Stop();
			INFO_PRINTF1(_L("Removing FailNext mark in heap..."));
			dhcpMemDbgParamBuf() = -1;
			debugSession.Ioctl(KCOLConfiguration, KDhcpMemDbgIoctl|KDHCP_DbgFailNext, stat, &dhcpMemDbgParamBuf);
			User::WaitForRequest(stat);
			TESTEL(stat == KErrNone, stat.Int());

			//Check we talked to a server...
			TRequestStatus stat;
			TConnectionAddrBuf address;
			address().iAddressFamily = IpAddressFamilyL();
			conn.Ioctl(KCOLConfiguration, KConnGetServerAddr, stat, &address);
			User::WaitForRequest(stat);
			TESTEL(stat == KErrNone, stat.Int());
			
			TInetAddr* properAddr = (TInetAddr*)&(address().iAddr);	
			THostName addr;
			properAddr->Output(addr);
			INFO_PRINTF2(_L("Talked to DHCP server %S (just so I know Start's KErrNone was valid)"),&addr);
			
			//Stop the connection
			INFO_PRINTF1(_L("Stopping connection..."));
			err = conn.Stop();
			TESTEL(stat == KErrNone, stat.Int());
			KErrNoneReturned++;
			}
		else 
			{
			if (KErrNoMemory != ret && failAfter != 1)
				{
				minorProblems = ETrue;
				INFO_PRINTF1(_L("ERROR: Return code not KErrNone or KErrNoMemory! Test will fail."));
				}
			KErrNoneReturned = 0;
			}
			
		//Wait 2 sec before checking heap. It will give time DHCP to complete clearence.
	    INFO_PRINTF1(_L("Wait for 2 seconds..."));
		User::After(2000000);

		//Check Heap. DHCP will panic if there is memory leak.
	    INFO_PRINTF1(_L("Checking heap (Memory leak in DHCP server will cause panic)..."));
	    dhcpMemDbgParamBuf()=0; //-- number of expected allocated cells
	    debugSession.Ioctl(KCOLConfiguration, KDhcpMemDbgIoctl|KDHCP_DbgCheckHeap, stat, &dhcpMemDbgParamBuf);
	    User::WaitForRequest(stat);
	    TESTEL(stat == KErrNone, stat.Int());

		//Re-open connection
		INFO_PRINTF1(_L("Closing connection..."));
		CleanupStack::PopAndDestroy(&conn);
		INFO_PRINTF1(_L("Re-opening connection..."));
		err = conn.Open(eSock);
		TESTEL(err == KErrNone, err);
		CleanupClosePushL(conn);
		INFO_PRINTF1(_L("******* LOOP PASSED *******"));
		
		//Increase "fail after" counter and try again.
		failAfter++;
		}

	//Set debug option to force discovery process.
	INFO_PRINTF1(_L("Remove debug options"));
	dhcpMemDbgParamBuf() = 0;
	debugSession.Ioctl(KCOLConfiguration, KDhcpMemDbgIoctl|KDHCP_DbgFlags, stat, &dhcpMemDbgParamBuf);
	User::WaitForRequest(stat);
	TESTEL(stat == KErrNone, stat.Int());
 
	//Close RConnection and RSocketServ objects
	CleanupStack::PopAndDestroy(&conn);
	CleanupStack::PopAndDestroy(&eSock); 
  
	//Close DHCP debug session
	CleanupStack::PopAndDestroy(&debugSession);

	SetTestStepResult(minorProblems?EFail:EPass);
	return TestStepResult();

    #endif //__DHCP_HEAP_CHECK_CONTROL
}
#ifdef SYMBIAN_NETWORKING_DHCPSERVER
    
    TVerdict CDhcpTestStepOOM_2::doTestStepL()
/**Checks OOM condition in DHCP during RConnection::Start(...) client request.*/
{
    #ifndef __DHCP_HEAP_CHECK_CONTROL
    	INFO_PRINTF1(_L("This test step is disabled for release build."));
        SetTestStepResult(EInconclusive);
	    return TestStepResult();
    #else

    INFO_PRINTF1(_L("DHCP heap check test during RConnection::Start(...) client request"));

	SetTestStepResult(EFail);
	
	//Start DHCP server.
	RStartProcess processStart;
	TInt err = processStart.Start(KDHCPProcessName);
	TESTEL( err == KErrNone, err);

	//Connect to the server. We will use this connection to issue heap debug commands
	RTestDaemonClient debugSession;
	debugSession.CreateSession();
	CleanupClosePushL(debugSession);

	//Connect to Socket Server
	RSocketServ eSock;
	err = eSock.Connect();
	TESTEL( err == KErrNone, err);
	CleanupClosePushL(eSock);
	
	//Prepare everything for RConnection.Start().
	RConnection conn;
	err = conn.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(conn);

	TRequestStatus stat;

	//Mark heap
	TDhcpMemDbgParamBuf dhcpMemDbgParamBuf;
    INFO_PRINTF1(_L("DHCP heap mark"));
	dhcpMemDbgParamBuf()=0;
    debugSession.Ioctl(KCOLConfiguration, KDhcpMemDbgIoctl|KDHCP_DbgMarkHeap, stat, &dhcpMemDbgParamBuf);
    User::WaitForRequest(stat);
    TESTEL(stat == KErrNone, stat.Int());
   
	//Start Connection
	INFO_PRINTF1(_L("Starting connection..."));
	TInt ret = conn.Start(iConnPrefs); // see test script for IAP used
	INFO_PRINTF2(_L("RConnection::Start returned %d"),ret);
	
	//Stop the connection
	INFO_PRINTF1(_L("Stopping connection..."));
	err = conn.Stop();
	TESTEL(stat == KErrNone, stat.Int());

	//Wait 2 sec before checking heap. It will give time DHCP to complete clearence.
	INFO_PRINTF1(_L("Wait for 2 seconds..."));
	User::After(2 * 100000);

	//Check Heap. DHCP will panic if there is memory leak.
    INFO_PRINTF1(_L("Checking heap (Memory leak in DHCP server will cause panic)..."));
    dhcpMemDbgParamBuf()=0; //-- number of expected allocated cells
    debugSession.Ioctl(KCOLConfiguration, KDhcpMemDbgIoctl|KDHCP_DbgMarkEnd, stat, &dhcpMemDbgParamBuf);
    User::WaitForRequest(stat);
    TESTEL(stat == KErrNone, stat.Int());

	//Close RConnection and RSocketServ objects
	CleanupStack::PopAndDestroy(&conn);
	CleanupStack::PopAndDestroy(&eSock); 
  
	//Close DHCP debug session
	CleanupStack::PopAndDestroy(&debugSession);

	SetTestStepResult(EPass);
	return TestStepResult();

    #endif //__DHCP_HEAP_CHECK_CONTROL
}
#endif // SYMBIAN_NETWORKING_DHCPSERVER


#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS

TVerdict CDhcpTestStepOOM_3::doTestStepL()
/**Checks OOM condition in DHCP during RConnection::Start(...) client request.*/
{
    #ifndef __DHCP_HEAP_CHECK_CONTROL
    	INFO_PRINTF1(_L("This test step is disabled for release build."));
        SetTestStepResult(EInconclusive);
	    return TestStepResult();
    #else

	#ifdef _DEBUG
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv4, 91));
	#endif

    INFO_PRINTF1(_L("DHCP heap check test during RConnection::Start(...) client request"));

	SetTestStepResult(EFail);
	TBool minorProblems = EFalse;
	
	//Start DHCP server.
	RStartProcess processStart;
	TInt err = processStart.Start(KDHCPProcessName);
	TESTEL( err == KErrNone, err);

	//Connect to the server. We will use this connection to issue heap debug commands
	RTestDaemonClient debugSession;
	debugSession.CreateSession();
	CleanupClosePushL(debugSession);

	//Connect to Socket Server
	RSocketServ eSock;		  
	err = eSock.Connect();
	TESTEL( err == KErrNone, err);
	CleanupClosePushL(eSock);
	
	//Prepare everything for RConnection.Start().
	RConnection conn;
	err = conn.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(conn);

	//Set debug option to force discovery process.
	INFO_PRINTF1(_L("Set debug option to force discovery process..."));
	TRequestStatus stat;
    TDhcpMemDbgParamBuf dhcpMemDbgParamBuf;
	dhcpMemDbgParamBuf() = KDHCP_ForceDiscovery;
	debugSession.Ioctl(KCOLConfiguration, KDhcpMemDbgIoctl|KDHCP_DbgFlags, stat, &dhcpMemDbgParamBuf);
	User::WaitForRequest(stat);
	TESTEL(stat == KErrNone, stat.Int());

	//Mark heap
    INFO_PRINTF1(_L("DHCP heap mark"));
	dhcpMemDbgParamBuf()=0;
    debugSession.Ioctl(KCOLConfiguration, KDhcpMemDbgIoctl|KDHCP_DbgMarkHeap, stat, &dhcpMemDbgParamBuf);
    User::WaitForRequest(stat);
    TESTEL(stat == KErrNone, stat.Int());

	//Init local data for loop
	TInt failAfter = 1; // check if IOCTL call handles one or more alloc failures gracefully
	TInt KErrNoneReturned = 0;

	//Start Connection
	INFO_PRINTF1(_L("Starting connection..."));
	TInt ret = conn.Start(iConnPrefs); // see test script for IAP used
	INFO_PRINTF2(_L("RConnection::Start returned %d"),ret);

	

	//Loop goes till 10 KErrNone are returned in row (or test harness times out)
	while (KErrNoneReturned < 10)
		{
		if(failAfter == 0)
			INFO_PRINTF1(_L("******* TEST FOR NO SIMULATED ALLOC. FAILS i.e. memory leak test ********"));
		else
			INFO_PRINTF2(_L("******* TEST WHEN ALLOC. NUMBER %d IN DHCP SERVER FAILS ********"),failAfter);

		//Set Heap to fail after (failAfter) tries
		INFO_PRINTF1(_L("Setting FailNext debug in heap..."));
        dhcpMemDbgParamBuf() = failAfter; //-- number of expected allocated cells
		debugSession.Ioctl(KCOLConfiguration, KDhcpMemDbgIoctl|KDHCP_DbgFailNext, stat, &dhcpMemDbgParamBuf);
		User::WaitForRequest(stat);
		TESTEL(stat == KErrNone, stat.Int());
		
		//Prepare option 150 and option 99 to be queried
		TBuf8<1000> multipleOptionBuf;
		TDhcpRawOptionMultipleDataPckg multipleOptionBufPkg(multipleOptionBuf);
		multipleOptionBufPkg.AddRawOptionCodeL(KTFtpServerAddress);
		multipleOptionBufPkg.AddRawOptionCodeL(KGeoConfCivicOption);

		conn.Ioctl(KCOLConfiguration, KConnDhcpGetMultipleParams, stat, &multipleOptionBuf);
		User::WaitForRequest(stat);

		if (KErrNone == stat.Int())
			{
			//Action is completed succesfuly.Remove FailNext Heap mark. Otherwise, it might panic on Stop();
			INFO_PRINTF1(_L("Removing FailNext mark in heap..."));
			dhcpMemDbgParamBuf() = -1;
			debugSession.Ioctl(KCOLConfiguration, KDhcpMemDbgIoctl|KDHCP_DbgFailNext, stat, &dhcpMemDbgParamBuf);
			User::WaitForRequest(stat);
			TESTEL(stat == KErrNone, stat.Int());

			//Check we talked to a server...
			TConnectionAddrBuf address;
			address().iAddressFamily = IpAddressFamilyL();
			conn.Ioctl(KCOLConfiguration, KConnGetServerAddr, stat, &address);
			User::WaitForRequest(stat);
			TESTEL(stat == KErrNone, stat.Int());
			
			TInetAddr* properAddr = (TInetAddr*)&(address().iAddr);	
			THostName addr;
			properAddr->Output(addr);
			INFO_PRINTF2(_L("Talked to DHCP server %S (just so I know Start's KErrNone was valid)"),&addr);
			
			KErrNoneReturned++;
			}
		else 
			{
			if (KErrNoMemory != stat.Int() && failAfter != 1)
				{
				minorProblems = ETrue;
				INFO_PRINTF1(_L("ERROR: Return code not KErrNone or KErrNoMemory! Test will fail."));
				}
			KErrNoneReturned = 0;
			}
			
		INFO_PRINTF1(_L("******* LOOP PASSED *******"));
		//Increase "fail after" counter and try again.
		
		failAfter++;
		if(failAfter > 100 )
			{
			minorProblems = ETrue;
			break;
			}
		}

	//Stop the connection
	INFO_PRINTF1(_L("Stopping connection..."));
	err = conn.Stop();
	TESTEL(stat == KErrNone, stat.Int());

	//Wait 2 sec before checking heap. It will give time DHCP to complete clearence.
	INFO_PRINTF1(_L("Wait for 2 seconds..."));
	User::After(2000000);

	//Check Heap. DHCP will panic if there is memory leak.
    INFO_PRINTF1(_L("Checking heap (Memory leak in DHCP server will cause panic)..."));
    dhcpMemDbgParamBuf()=0; //-- number of expected allocated cells
    debugSession.Ioctl(KCOLConfiguration, KDhcpMemDbgIoctl|KDHCP_DbgCheckHeap, stat, &dhcpMemDbgParamBuf);
    User::WaitForRequest(stat);
    TESTEL(stat == KErrNone, stat.Int());
	
	//Set debug option to force discovery process.
	INFO_PRINTF1(_L("Remove debug options"));
	dhcpMemDbgParamBuf() = 0;
	debugSession.Ioctl(KCOLConfiguration, KDhcpMemDbgIoctl|KDHCP_DbgFlags, stat, &dhcpMemDbgParamBuf);
	User::WaitForRequest(stat);
	TESTEL(stat == KErrNone, stat.Int());

	//Close RConnection and RSocketServ objects
	CleanupStack::PopAndDestroy(&conn);
	CleanupStack::PopAndDestroy(&eSock); 
  
	//Close DHCP debug session
	CleanupStack::PopAndDestroy(&debugSession);

	SetTestStepResult(minorProblems?EFail:EPass);
	return TestStepResult();

    #endif //__DHCP_HEAP_CHECK_CONTROL
}
#endif // SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
