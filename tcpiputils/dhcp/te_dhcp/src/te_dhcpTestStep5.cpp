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
// Configuration Tests (test docs 4.5)
// 
//

/**
 @file te_dhcpTestStep5.cpp
*/
#include "te_dhcpTestStep5.h"
#include <test/testexecutelog.h>
#include <commdbconnpref.h>
#include <in_sock.h>
#include <nifman.h>
#include <comms-infras/es_config.h>
#include <comms-infras/startprocess.h>
#include "Te_TestDaemonClient.h"


TVerdict CDhcpTestStep5_1::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Inform Scenario (test case 4.5.1)
*/
	{
	SetTestStepResult(EFail);
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);
#ifdef _DEBUG
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv4, 128));
#endif		
	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	err = connNET1.Start(iConnPrefs); // see test script for IAP used
	TESTEL(err == KErrNone, err);
	
	TRequestStatus stat;
	TConnectionAddrBuf address;
	address().iAddressFamily = IpAddressFamilyL();
	connNET1.Ioctl(KCOLConfiguration, KConnGetCurrentAddr, stat, &address);
	User::WaitForRequest(stat);
	TESTEL(stat == KErrNone, stat.Int());
	
	//Address matches that stored in CommDB 
	TBuf16<KCommsDbSvrMaxFieldLength> buf;
	err = connNET1.GetDesSetting(TPtrC(SERVICE_IP_ADDR), buf);
	TESTEL(err == KErrNone, err);
	TInetAddr addressFromDb;
	addressFromDb.Input(buf);
	//TInetAddr addressFromDhcp;
	//addressFromDhcp.SetAddress(address().iAddr);
	TESTEL(addressFromDb.Match(address().iAddr), KErrNotFound);

	err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::PopAndDestroy(&connNET1);
	CleanupStack::PopAndDestroy(&eSock); 
	
	SetTestStepResult(EPass);
	return TestStepResult();
	}

TVerdict CDhcpTestStep5_2::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Renew Current Lease whilst having static IP address (test spec 4.5.2)
* static addresses have no lease, they are simply used.
*/
	{
	SetTestStepResult(EFail);
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);
#ifdef _DEBUG
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv4, 126));
#endif		
	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	err = connNET1.Start(iConnPrefs); // see test script for IAP used
	TESTEL(err == KErrNone, err);
	
	TRequestStatus stat1;
	connNET1.Ioctl(KCOLConfiguration, KConnAddrRenew, stat1);
	User::WaitForRequest(stat1);
	TESTEL(stat1 == KErrNotSupported, stat1.Int());

	err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::PopAndDestroy(&connNET1);
	CleanupStack::PopAndDestroy(&eSock);
	
	SetTestStepResult(EPass);
	return TestStepResult();
	}


TVerdict CDhcpTestStep5_3::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Release Lease of statically configured connection (test spec 4.3.3)
* Releasing a lease of a statically configured connection is not
* possible as there is no lease on static ip address connections...
* so the required behaviour is that the release fails...
*/
	{
	SetTestStepResult(EFail);
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);
#ifdef _DEBUG
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv4, 126));
#endif		
	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	err = connNET1.Start(iConnPrefs); // see test script for IAP used
	TESTEL(err == KErrNone, err);
	
	TRequestStatus stat1;
	connNET1.Ioctl(KCOLConfiguration, KConnAddrRelease, stat1);
	User::WaitForRequest(stat1);
	TESTEL(stat1 == KErrNotSupported, stat1.Int());

	err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::PopAndDestroy(&connNET1);
	CleanupStack::PopAndDestroy(&eSock);
	
	SetTestStepResult(EPass);
	return TestStepResult();
	}

TVerdict CDhcpTestStep5_4::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Renew Current Lease (test spec 4.5.4)
* and check that renewed lease is longer than 
* current lease, some of which has been used up...
*/
	{
	SetTestStepResult(EFail);
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);
#ifdef _DEBUG
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv4, 67));
#endif		
	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	err = connNET1.Start(iConnPrefs); // see test script for IAP used
	TESTEL(err == KErrNone, err);
	
	// wait for 5 seconds to run down some of the lease...
	User::After(5 * 1000000);
	
	TConnectionLeaseInfoBuf leaseTime1;
	leaseTime1().iAddressFamily = IpAddressFamilyL();
	TRequestStatus stat;
	connNET1.Ioctl(KCOLConfiguration, KConnGetAddrLeaseTimeRemain, stat, &leaseTime1);
	User::WaitForRequest(stat);
	TESTEL(stat == KErrNone, stat.Int());

	//renew
	connNET1.Ioctl(KCOLConfiguration, KConnAddrRenew, stat);
	User::WaitForRequest(stat);
	TESTEL(stat == KErrNone, stat.Int());
	
	TConnectionLeaseInfoBuf leaseTime2;
	leaseTime1().iAddressFamily = IpAddressFamilyL();
	connNET1.Ioctl(KCOLConfiguration, KConnGetAddrLeaseTimeRemain, stat, &leaseTime2);
	User::WaitForRequest(stat);
	TESTEL(stat == KErrNone, stat.Int());
	TESTEL(leaseTime2().iSecondsRemaining > leaseTime1().iSecondsRemaining, KErrArgument);

	err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::PopAndDestroy(&connNET1);
	CleanupStack::PopAndDestroy(&eSock);
	
	SetTestStepResult(EPass);
	return TestStepResult();
	}


	
TVerdict CDhcpTestStep5_5::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Opens one connection and starts it, then stops
* and restarts the connection several times to test the behaviour (test spec 4.4.5)
*/
	{
	SetTestStepResult(EFail);
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);
#ifdef _DEBUG
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv4, 67));
#endif		
	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	err = connNET1.Start(iConnPrefs); // see test script for IAP used
	TESTEL(err == KErrNone, err);
	
	for (TInt i = 0; i < 3; ++i)
		{
		err = connNET1.Stop();
		TESTEL(err == KErrNone, err);
		
		err = connNET1.Start(iConnPrefs);
		TESTEL(err == KErrNone, err);
		}
	
	err = connNET1.Stop();
	TESTEL(err == KErrNone, err);
 
	CleanupStack::PopAndDestroy(&connNET1);
	CleanupStack::PopAndDestroy(&eSock);
	
	SetTestStepResult(EPass);
	return TestStepResult();
	}	


TVerdict CDhcpTestStep5_6::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Waits for Renew timeout to expire.
* and check that renewed lease is longer than 
* current lease, some of which has been used up...
*/
	{
    #ifndef __DHCP_HEAP_CHECK_CONTROL
    	INFO_PRINTF1(_L("This test step is disabled for release build."));
        SetTestStepResult(EInconclusive);
	    return TestStepResult();
    #else

    INFO_PRINTF1(_L("Checking Renew process:"));
	SetTestStepResult(EFail);

	//Start DHCP server.
	RStartProcess processStart;
	TInt err = processStart.Start(KDHCPProcessName);
	TESTEL( err == KErrNone || err ==KErrAlreadyExists, err);

	//Connect to the server. We will use this connection to issue heap debug commands
	RTestDaemonClient debugSession;
	debugSession.CreateSession();
	CleanupClosePushL(debugSession);

	
	//Connect to Socket Server
	RSocketServ eSock;
	err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);
#ifdef _DEBUG
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv4, 67));
#endif		
	
	//open connection
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	//Set debug option for short lease time and timeout.
	INFO_PRINTF1(_L("Set debug option for short lease time and timeout..."));
	TRequestStatus stat;
    TDhcpMemDbgParamBuf dhcpMemDbgParamBuf;
	dhcpMemDbgParamBuf() = KDHCP_SetShortLease | KDHCP_SetShortRetryTimeOut;
	debugSession.Ioctl(KCOLConfiguration, KDhcpMemDbgIoctl|KDHCP_DbgFlags, stat, &dhcpMemDbgParamBuf);
	User::WaitForRequest(stat);
	TESTEL(stat == KErrNone, stat.Int());


	//start connection
	err = connNET1.Start(iConnPrefs); // see test script for IAP used
	TESTEL(err == KErrNone, err);
	

	//Read initial remaining lease time
	TConnectionLeaseInfoBuf remainingLease1, remainingLease2;
	remainingLease1().iAddressFamily = IpAddressFamilyL();
	connNET1.Ioctl(KCOLConfiguration, KConnGetAddrLeaseTimeRemain, stat, &remainingLease1);
	User::WaitForRequest(stat);
	TESTEL(stat == KErrNone, stat.Int());
    INFO_PRINTF2(_L("Remaining lease time: %d secs"),remainingLease1().iSecondsRemaining);

	//start loop for reading lease time
	TBool inLoop = ETrue;
	while (inLoop)
	{
		// wait for 5 seconds to run down some of the lease...
		User::After(5 * 1000000);
	
		//Read lease time
		remainingLease2().iAddressFamily = IpAddressFamilyL();
		connNET1.Ioctl(KCOLConfiguration, KConnGetAddrLeaseTimeRemain, stat, &remainingLease2);
		User::WaitForRequest(stat);
		TESTEL(stat == KErrNone, stat.Int());
	    INFO_PRINTF2(_L("Remaining lease time: %d secs"),remainingLease2().iSecondsRemaining);
		
		//Test is completed when new remaining lease is greater then old one
		if( (static_cast<TInt>(remainingLease2().iSecondsRemaining) > 0) &&
			(remainingLease2().iSecondsRemaining > remainingLease1().iSecondsRemaining) )
			inLoop = EFalse;

		if( static_cast<TInt>(remainingLease2().iSecondsRemaining) > 0 )
			remainingLease1().iSecondsRemaining = remainingLease2().iSecondsRemaining;
	}
	err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	//Remove all debug options.
	INFO_PRINTF1(_L("Remove all debug options..."));
	dhcpMemDbgParamBuf() = 0;
	debugSession.Ioctl(KCOLConfiguration, KDhcpMemDbgIoctl|KDHCP_DbgFlags, stat, &dhcpMemDbgParamBuf);
	User::WaitForRequest(stat);
	TESTEL(stat == KErrNone, stat.Int());
	
	CleanupStack::PopAndDestroy(&connNET1);
	CleanupStack::PopAndDestroy(&eSock);
	
	//Close DHCP debug session
	CleanupStack::PopAndDestroy(&debugSession);

	SetTestStepResult(EPass);
	return TestStepResult();
	#endif
	}

TVerdict CDhcpTestStep5_7::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Duplicate Address(test doc 4.5.7) - test forces a decline message
* to be sent to the server, but then should rediscover and get a 
* new address, so that connection starts successfully.
*/
	{
#ifndef __DHCP_HEAP_CHECK_CONTROL
    INFO_PRINTF1(_L("This test step is disabled for release build."));
    SetTestStepResult(EInconclusive);
	return TestStepResult();
#else
	
	SetTestStepResult(EFail);
	
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);
#ifdef _DEBUG
	// DAD case
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv4, 118));
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv6, 561));
#endif		
	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	err = connNET1.Start(iConnPrefs); // see test script for IAP used
	TESTEL(err == KErrNone, err);
	
	err = connNET1.Stop();
	TESTEL(err == KErrNone, err);
	
	CleanupStack::PopAndDestroy(&connNET1);
	CleanupStack::PopAndDestroy(&eSock);
	
	SetTestStepResult(EPass);
	return TestStepResult();
#endif
	}

TVerdict CDhcpTestStep5_8::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Get Current IP Address too early (test doc 4.2.1)
* Start a connection and query it before waiting for the connection start
* to complete.
*/
	{
	SetTestStepResult(EFail);	
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);

#ifdef _DEBUG
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv4, 67));
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv6, 547));
#endif		

	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	TRequestStatus stat1;
	connNET1.Start(iConnPrefs,stat1);// see test script for IAP used
	
	TRequestStatus stat2;
	TConnectionAddrBuf address;
	address().iAddressFamily = IpAddressFamilyL();
	connNET1.Ioctl(KCOLConfiguration, KConnGetCurrentAddr, stat2, &address);
	User::WaitForRequest(stat2);
	User::WaitForRequest(stat1);
	TESTEL(stat2.Int() == KErrNotReady, stat2.Int());
	TESTEL(stat1.Int() == KErrNone, stat1.Int());
	
	// now wait again to check that it will actually work
	connNET1.Ioctl(KCOLConfiguration, KConnGetCurrentAddr, stat2, &address);
	User::WaitForRequest(stat2);
	TESTEL(stat2.Int() == KErrNone, stat2.Int());
	
	err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::PopAndDestroy(&connNET1); 
	CleanupStack::PopAndDestroy(&eSock); 
	
	SetTestStepResult(EPass);
	return TestStepResult();
	}

