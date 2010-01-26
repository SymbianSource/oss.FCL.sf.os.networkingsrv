// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Configuration Tests (test docs 4.2)
// 
//

/**
 @file te_dhcpTestStep2.cpp
*/
#include "te_dhcpTestStep2.h"
#include <test/testexecutelog.h>
#include <commdbconnpref.h>
#include <in_sock.h>
#include <nifman.h>
#include <comms-infras/es_config.h>

#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS

#include <utf.h>
#include <networking/dhcpconfig.h>

#endif // SYMBIAN_NETWORKING_DHCP_MSG_HEADERS

#include "../../include/DHCPStatesDebug.h"

#include <simtsy.h>
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <nifman_internal.h>
#endif

TVerdict CDhcpTestStep2_1::doTestStepL()
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

TVerdict CDhcpTestStep2_2::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Get Current IP Address (test doc 4.2.2)
* Start a connection with full DHCP config
* then use the RConnectionIoctl to query for
* the configured address
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
	
	err = connNET1.Start(iConnPrefs); // see test script for IAP used
	TESTEL(err == KErrNone, err);
	
	TRequestStatus stat;
	TConnectionAddrBuf address;
	address().iAddressFamily = IpAddressFamilyL();
	connNET1.Ioctl(KCOLConfiguration, KConnGetCurrentAddr, stat, &address);
	User::WaitForRequest(stat);
	TESTEL(stat == KErrNone, stat.Int());
	
	// we have got an address back from the daemon...now we must check
	// that this address is correct...for this we bind a socket to it!
	RSocket socket;
	err = socket.Open(eSock, KAfInet, KSockDatagram, KProtocolInetUdp, connNET1);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(socket);

	TInetAddr* properAddr = (TInetAddr*)&(address().iAddr);	

	err = socket.Bind(*properAddr);
	TESTEL(err == KErrNone, err);

	CleanupStack::PopAndDestroy(&socket);
	
	err = connNET1.Stop();
	TESTEL(err == KErrNone, err);
	 
	CleanupStack::PopAndDestroy(&connNET1);
	CleanupStack::PopAndDestroy(&eSock); 

	SetTestStepResult(EPass);
	return TestStepResult();
	}

TVerdict CDhcpTestStep2_3::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Get Remaining Lease Time from a configured connection (test case 4.2.3)
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
	
	TConnectionLeaseInfoBuf leaseTime;
	leaseTime().iAddressFamily = IpAddressFamilyL();
	TRequestStatus stat;
	connNET1.Ioctl(KCOLConfiguration, KConnGetAddrLeaseTimeRemain, stat, &leaseTime);
	User::WaitForRequest(stat);
	TESTEL(stat == KErrNone, stat.Int());

	TESTEL(leaseTime().iSecondsRemaining>0, KErrArgument);

	err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::PopAndDestroy(&connNET1);
	CleanupStack::PopAndDestroy(&eSock);
	
	SetTestStepResult(EPass);
	return TestStepResult();
	}

TVerdict CDhcpTestStep2_4::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Get DHCP Server Address on a configured connection (test case 4.2.4)
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
	
	TRequestStatus stat;
	TConnectionAddrBuf address;
	address().iAddressFamily = IpAddressFamilyL();
	connNET1.Ioctl(KCOLConfiguration, KConnGetServerAddr, stat, &address);
	User::WaitForRequest(stat);
	TESTEL(stat == KErrNone, stat.Int());

	err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::PopAndDestroy(&connNET1);
	CleanupStack::PopAndDestroy(&eSock); 
	
	SetTestStepResult(EPass);
	return TestStepResult();
	}


TVerdict CDhcpTestStep2_5::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Get current address on a down connection (test case 4.2.5)
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
	
	err = connNET1.Stop();
	TESTEL(err == KErrNone, err);
	
	TRequestStatus stat;
	TConnectionAddrBuf address;
	address().iAddressFamily = IpAddressFamilyL();
	connNET1.Ioctl(KCOLConfiguration, KConnGetCurrentAddr, stat, &address);
	User::WaitForRequest(stat);
	TESTEL(stat == KErrNotReady, stat.Int());
	
	CleanupStack::PopAndDestroy(&connNET1);
	CleanupStack::PopAndDestroy(&eSock); 
	
	SetTestStepResult(EPass);
	return TestStepResult();
	}


TVerdict CDhcpTestStep2_6::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Get DHCP Server Address on a down connection (test case 4.2.6)
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
	
	err = connNET1.Stop();
	TESTEL(err == KErrNone, err);
	
	TRequestStatus stat;
	TConnectionAddrBuf address;
	address().iAddressFamily = IpAddressFamilyL();
	connNET1.Ioctl(KCOLConfiguration, KConnGetServerAddr, stat, &address);
	User::WaitForRequest(stat);
	TESTEL(stat == KErrNotReady, stat.Int());
	
	CleanupStack::PopAndDestroy(&connNET1);
	CleanupStack::PopAndDestroy(&eSock);
	
	SetTestStepResult(EPass);
	return TestStepResult();
	}

TVerdict CDhcpTestStep2_7::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Get Remaining Lease Time on a down connection (test case 4.2.7)
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
	
	err = connNET1.Stop();
	TESTEL(err == KErrNone, err);
	
	TConnectionLeaseInfoBuf leaseTime;
	leaseTime().iAddressFamily = IpAddressFamilyL();
	TRequestStatus stat;
	connNET1.Ioctl(KCOLConfiguration, KConnGetAddrLeaseTimeRemain, stat, &leaseTime);
	User::WaitForRequest(stat);
	TESTEL(stat == KErrNotReady, stat.Int());
	
	CleanupStack::PopAndDestroy(&connNET1);
	CleanupStack::PopAndDestroy(&eSock);
	
	SetTestStepResult(EPass);
	return TestStepResult();
	}

TVerdict CDhcpTestStep2_8::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Get Active IP Address on a restarted connection (test case 4.2.8)
* First connection should be configured with an addr and lease
* and when the connection is started for the second time, after being
* taken down the same address will be reused as the lease is still
* valid for the address assigned in configuring the connection for the
* first time. 
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
	
	TConnectionAddrBuf address1;
	address1().iAddressFamily = IpAddressFamilyL();
	TConnectionAddrBuf address2;
	address1().iAddressFamily = IpAddressFamilyL();
	for (TUint i=0; i<2; ++i)
		{
		err = connNET1.Start(iConnPrefs); // see test script for IAP used
		TESTEL(err == KErrNone, err);
		
		TRequestStatus stat;
		connNET1.Ioctl(KCOLConfiguration, KConnGetCurrentAddr, stat, i == 0 ? &address1 : &address2);
		User::WaitForRequest(stat);
		TESTEL(stat == KErrNone, stat.Int());
		
		err = connNET1.Stop();
		TESTEL(err == KErrNone, err);
		}
	
	TInetAddr* properAddr = (TInetAddr*)&(address1().iAddr);	
	TUint32 addr1 = properAddr->Address();
	properAddr = (TInetAddr*)&(address2().iAddr);
	TUint32 addr2 = properAddr->Address();

	TESTEL(addr1 == addr2, KErrArgument);
	
	CleanupStack::PopAndDestroy(&connNET1);
	CleanupStack::PopAndDestroy(&eSock);
	
	SetTestStepResult(EPass);
	return TestStepResult();
	}

TVerdict CDhcpTestStep2_9::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Demonstrates the lease time running down for
* the connection.  And checks that it is reported properly
* by querying then waiting, then querying again.
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
	
	TConnectionLeaseInfoBuf leaseTime1;
	leaseTime1().iAddressFamily = IpAddressFamilyL();
	TConnectionLeaseInfoBuf leaseTime2;
	leaseTime2().iAddressFamily = IpAddressFamilyL();
	
	err = connNET1.Start(iConnPrefs); // see test script for IAP used
	TESTEL(err == KErrNone, err);
		
	TRequestStatus stat;
	connNET1.Ioctl(KCOLConfiguration, KConnGetAddrLeaseTimeRemain, stat, &leaseTime1);
	User::WaitForRequest(stat);
	TESTEL(stat == KErrNone, stat.Int());
	
	User::After(2 * 1000000);

	connNET1.Ioctl(KCOLConfiguration, KConnGetAddrLeaseTimeRemain, stat, &leaseTime2);
	User::WaitForRequest(stat);
	TESTEL(stat == KErrNone, stat.Int());

	TESTEL(leaseTime1().iSecondsRemaining > leaseTime2().iSecondsRemaining, KErrArgument);

	err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::PopAndDestroy(&connNET1); 
	CleanupStack::PopAndDestroy(&eSock); 
	
	SetTestStepResult(EPass);
	return TestStepResult();
	}


TVerdict CDhcpTestStep2_GetRaw::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* GetRawOption Data (test case 4.2.10,15,16,19,20,21)
*/
	{
	SetTestStepResult(EFail);

	INFO_PRINTF1(_L("Connecting to socket server.."));

	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);
	
#ifdef _DEBUG
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv4, 126));
#endif		
	INFO_PRINTF1(_L("Opening connection.."));

	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
   	INFO_PRINTF2(_L("Starting IAP %d.."),IAPToUseL());

	err = connNET1.Start(iConnPrefs); // see test script for IAP used
	TESTEL(err == KErrNone, err);

	INFO_PRINTF1(_L("Start completed ok.."));

	DHCPDebug::State state;
	GetDebugHandleL(connNET1);
	LOG_STATEL;
	
	ImmediateCompletionTestL(connNET1);
	
    TBuf8<48> buf;
	buf.FillZ(48);
    const TUint KOptCode = 1; //-- OPTION_CLIENTID(ipv6) or SubnetMask(ipv4)

	INFO_PRINTF1(_L("Calling ioctl.."));

    //-- get raw option data either for ip4 of for ip6, depending on the test setting
    //-- if someone needs to analyse option data, use something like 
    //-- "TDhcp6RawOptionDataPckg pckg(buf);" or "TDhcp4RawOptionDataPckg pckg(buf);"
    err = DhcpGetRawOptionDataL(connNET1, buf, KOptCode);
    TESTEL(err == KErrNone, err);
	
	INFO_PRINTF1(_L("Ioctl completed ok.."));
	LOG_STATEL;

	TDhcp4RawOptionDataPckg pckg(buf);
	TPtr8 buf2(pckg.Buf());

	INFO_PRINTF2(_L("Buffer received ok. Length %d"),buf2.Length());

	INFO_PRINTF1(_L("Stopping IAP.."));
	
	err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	INFO_PRINTF1(_L("Cleaning up.."));
	
	CleanupStack::PopAndDestroy(&connNET1);
	CleanupStack::PopAndDestroy(&eSock); 
	
	SetTestStepResult(EPass);

	return TestStepResult();
	}



TVerdict CDhcpTestStep2_11::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* GetRawOption Data buffer too small (test case 4.2.11)
*/
	{
	SetTestStepResult(EFail);
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);
	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	err = connNET1.Start(iConnPrefs); // see test script for IAP used
	TESTEL(err == KErrNone, err);

    TBuf8<4> buf; //-- buffer is set too small deliberately to test overflow condition
	buf.FillZ(3);
	
    const TUint KOptCode = 1; //-- OPTION_CLIENTID(ipv6) or SubnetMask(ipv4)
    
    //-- get raw option data either for ip4 of for ip6, depending on the test setting
    //-- if someone needs to analyse option data, use something like 
    //-- "TDhcp6RawOptionDataPckg pckg(buf);" or "TDhcp4RawOptionDataPckg pckg(buf);"
    err = DhcpGetRawOptionDataL(connNET1, buf, KOptCode);
    TESTEL(err == KErrOverflow, err);
    
	err = connNET1.Stop();
	TESTEL(err == KErrNone, err);
	
	CleanupStack::PopAndDestroy(&connNET1);
	CleanupStack::PopAndDestroy(&eSock); 
	
	SetTestStepResult(EPass);

	return TestStepResult();
	}

TVerdict CDhcpTestStep2_12::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* GetRawOption Data with bad option (test case 4.2.12)
*/
	{
	SetTestStepResult(EFail);
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);
	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	err = connNET1.Start(iConnPrefs); // see test script for IAP used
	TESTEL(err == KErrNone, err);

    TBuf8<48> buf;
	buf.FillZ(48);

    const TUint KOptCode = 123; //-- bad option
    
    //-- get raw option data either for ip4 of for ip6, depending on the test setting
    //-- if someone needs to analyse option data, use something like 
    //-- "TDhcp6RawOptionDataPckg pckg(buf);" or "TDhcp4RawOptionDataPckg pckg(buf);"
    err = DhcpGetRawOptionDataL(connNET1, buf, KOptCode);
    TESTEL(err == KErrNotFound, err);

	err = connNET1.Stop();
	TESTEL(err == KErrNone, err);	
	
	CleanupStack::PopAndDestroy(&connNET1);
	CleanupStack::PopAndDestroy(&eSock); 
	
	SetTestStepResult(EPass);

	return TestStepResult();
	}


TVerdict CDhcpTestStep2_GetSIPAddrViaDHCP::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Retrieve SIP server address via DHCP(test case 4.2.13.1,16)
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

	INFO_PRINTF1(_L("Start completed ok.."));

	DHCPDebug::State state;
	GetDebugHandleL(connNET1);
	LOG_STATEL;
	
	ImmediateCompletionTestL(connNET1);
	
	TRequestStatus status;
	
	INFO_PRINTF1(_L("Calling ioctl.."));

	// Request the first address available
	TSipServerAddrBuf sipServerAddr;	
	sipServerAddr().index = 0;
	connNET1.Ioctl(KCOLConfiguration, KConnGetSipServerAddr, status, &sipServerAddr);
	User::WaitForRequest(status);
	
	TBool checkTheReceivedData = ETrue;	
	if(status == KErrNotFound && UsingIPv6L() == EFalse)
		{
		// IPv4 SIP options - only 1 can be set at a time..
		//  so a KErrNotFound isn't necessarily a failure..
		//  but let's just make sure the other one's there :-)
		//
		INFO_PRINTF1(_L("IPv4- SIP address not available.. trying SIP domain.."));
		
		TSipServerDomainBuf sipServerDomain;
		sipServerDomain().index = 0;
		connNET1.Ioctl(KCOLConfiguration, KConnGetSipServerDomain, status, &sipServerDomain);
		User::WaitForRequest(status);
		
		checkTheReceivedData = EFalse; // as we've not got an address to check
		}

	if(status.Int() != KErrNone)
		{
		INFO_PRINTF2(_L("SIP option ioctl failed with code %d.."), status.Int());
		TESTEL(status.Int() == KErrNone, status.Int());
		}

	if(checkTheReceivedData)
		{
		THostName addr;
		sipServerAddr().address.Output(addr);
		
		// Verify the retrieved address is correct
		TPtrC addressToCompare;
		
		if(UsingIPv6L())
			addressToCompare.Set(_L("2000::300"));
		else
			addressToCompare.Set(_L("10.20.30.40"));
			
		INFO_PRINTF2(_L("Received SIP address: %S"), &addr);
		if(addr.CompareF(addressToCompare) != 0)
			{
			INFO_PRINTF2(_L("Expected SIP address %S!"), &addressToCompare);
			TESTEL(0, KErrNotFound);
			}
		}
	
	LOG_STATEL;

	err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

		
	CleanupStack::PopAndDestroy(&connNET1);
	CleanupStack::PopAndDestroy(&eSock);

	SetTestStepResult(EPass);

	return TestStepResult();		
	}

TVerdict CDhcpTestStep2_GetSIPDomain::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Retrieve SIP server domain (test case 4.2.14,18)
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

	INFO_PRINTF1(_L("Start completed ok.."));

	DHCPDebug::State state;
	GetDebugHandleL(connNET1);
	LOG_STATEL;

	ImmediateCompletionTestL(connNET1);
	
	TRequestStatus status;
	
	INFO_PRINTF1(_L("Calling ioctl.."));

	// Request the first domain available
	TSipServerDomainBuf sipServerDomain;
	sipServerDomain().index = 0;	
	connNET1.Ioctl(KCOLConfiguration, KConnGetSipServerDomain, status, &sipServerDomain);
	User::WaitForRequest(status);
	
	TBool checkTheReceivedData = ETrue;	
	if(status == KErrNotFound && UsingIPv6L() == EFalse)
		{
		// IPv4 SIP options - only 1 can be set at a time..
		//  so a KErrNotFound isn't necessarily a failure..
		//  but let's just make sure the other one's there :-)
		//
		INFO_PRINTF1(_L("IPv4- SIP domain not available.. trying SIP address.."));

		TSipServerAddrBuf sipServerAddr;	
		sipServerAddr().index = 0;	
		connNET1.Ioctl(KCOLConfiguration, KConnGetSipServerAddr, status, &sipServerAddr);
		User::WaitForRequest(status);

		checkTheReceivedData = EFalse; // as we've not got a domain to check
		}


	if(status.Int() != KErrNone)
		{
		INFO_PRINTF2(_L("SIP option ioctl failed with code %d.."), status.Int());
		TESTEL(status.Int() == KErrNone, status.Int());
		}

	if(checkTheReceivedData)
		{
		INFO_PRINTF2(_L("Received SIP domain 1: %S"), &sipServerDomain().domainName);
		if(sipServerDomain().domainName.CompareF(_L("sip1.test.intra")) != 0)
			{
			INFO_PRINTF1(_L("Expected SIP domain sip1.test.intra!"));
			TESTEL(0, KErrNotFound);
			}

		// Request the second domain available
		sipServerDomain().index = 1;
		connNET1.Ioctl(KCOLConfiguration, KConnGetSipServerDomain, status, &sipServerDomain);
		User::WaitForRequest(status);
		
		TESTEL(status.Int() == KErrNone, status.Int());	
		INFO_PRINTF2(_L("Received SIP domain 2: %S"), &sipServerDomain().domainName);
		if(sipServerDomain().domainName.CompareF(_L("sip2.test.intra")) != 0)
			{
			INFO_PRINTF1(_L("Expected SIP domain sip2.test.intra!"));
			TESTEL(0, KErrNotFound);
			}
		}

	LOG_STATEL;

	err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::PopAndDestroy(&connNET1);
	CleanupStack::PopAndDestroy(&eSock); 
		
	SetTestStepResult(EPass);

	return TestStepResult();		
	}

const TInt KMaxSIPServerAddresses = 4;

TVerdict CDhcpTestStep2_GetSIPAddrViaPCOBuffer::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Retrieve SIP server address (test case 4.2.13.2,17)
*/
	{
	SetTestStepResult(EFail);

    TBuf<39> addresses[KMaxSIPServerAddresses] = {_L("ffff:ffff:ffff:ffff:eeee:eeee:eeee:eeee"),
                                                _L("ffff:ffff:ffff:ffff:cccc:cccc:cccc:cccc"),
                                                _L("ffff:ffff:ffff:ffff:aaaa:aaaa:aaaa:aaaa"),
                                                _L("")};

	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);
	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	err =  RProperty::Set(KUidPSSimTsyCategory,KPSSimTsyTestNumber,0);
	TESTEL(err == KErrNone, err);
	
	err = connNET1.Start(iConnPrefs); // see test script for IAP used
	TESTEL(err == KErrNone, err);

	INFO_PRINTF1(_L("Start completed ok.."));

	TRequestStatus status;
	
	INFO_PRINTF1(_L("Calling ioctl.."));

	// Request the first address available
    for (TInt i=0;i<KMaxSIPServerAddresses;i++)
        {
        TSipServerAddrBuf sipServerAddr;
        sipServerAddr().index = i;
        connNET1.Ioctl(KCOLConfiguration, KConnGetSipServerAddr, status, &sipServerAddr);
        User::WaitForRequest(status);
        
        TBool checkTheReceivedData = ETrue;	
    
        if( (status.Int() != KErrNone && i<KMaxSIPServerAddresses)
            || (i<KMaxSIPServerAddresses && (status.Int() == KErrNotFound) ) )
            {
            INFO_PRINTF2(_L("SIP option ioctl failed with code %d.."), status.Int());
            // Since it is not possible to configure DHCP for this test, KErrNotFound is not returned, 
            // but KErrNotSupported from missing DHCP for address zero.
            if( i == 0 )
            	{
            	TESTEL(status.Int() == KErrNotSupported, status.Int());
            	}
            }
        
        if(checkTheReceivedData)
            {
            THostName addr;
            sipServerAddr().address.Output(addr);
            
            // Verify the retrieved address is correct
            TPtrC addressToCompare;
            
            addressToCompare.Set(addresses[i]);
                
            INFO_PRINTF2(_L("Received SIP address: %S"), &addr);
            if(addr.CompareF(addressToCompare) != 0)
                {
                INFO_PRINTF2(_L("Expected SIP address %S!"), &addressToCompare);
                TESTEL(0, KErrNotFound);
                }
            }
        }

	err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

		
	CleanupStack::PopAndDestroy(&connNET1);
	CleanupStack::PopAndDestroy(&eSock);

	SetTestStepResult(EPass);

	return TestStepResult();		
	}

TVerdict CDhcpTestStep2_GetSIPAddrFailure::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Retrieve SIP server address (test case 4.2.13.6,18)
*/
	{
	SetTestStepResult(EFail);
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);
	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	err =  RProperty::Set(KUidPSSimTsyCategory,KPSSimTsyTestNumber,1);
	TESTEL(err == KErrNone, err);
	
	err = connNET1.Start(iConnPrefs); // see test script for IAP used
	TESTEL(err == KErrNone, err);

	INFO_PRINTF1(_L("Start completed ok.."));

	TRequestStatus status;
	
	INFO_PRINTF1(_L("Calling ioctl.."));

	// Request the first address available
	TSipServerAddrBuf sipServerAddr;
	sipServerAddr().index = 0;
	connNET1.Ioctl(KCOLConfiguration, KConnGetSipServerAddr, status, &sipServerAddr);
	User::WaitForRequest(status);
	
	if(status.Int() == KErrNotFound)
		{
		SetTestStepResult(EPass);
		}

	err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::PopAndDestroy(&connNET1);
	CleanupStack::PopAndDestroy(&eSock);

	SetTestStepResult(EPass);

	return TestStepResult();
	}

TVerdict CDhcpTestStep2_GetSIPAddrBufferOverrun::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Retrieve SIP server address (test case 4.2.13.5,17)
*/
	{
	SetTestStepResult(EPass);
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);
	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	err =  RProperty::Set(KUidPSSimTsyCategory,KPSSimTsyTestNumber,0);
	TESTEL(err == KErrNone, err);
	
	err = connNET1.Start(iConnPrefs); // see test script for IAP used
	TESTEL(err == KErrNone, err);

	INFO_PRINTF1(_L("Start completed ok.."));

	TRequestStatus status;
	
	INFO_PRINTF1(_L("Calling ioctl.."));

	// Request the first address available
	// create rubish buffer with size less then necessary
    TBuf8<40> sipServerAddr;
    sipServerAddr.Fill('Z');

    connNET1.Ioctl(KCOLConfiguration, KConnGetSipServerAddr, status, &sipServerAddr);
	User::WaitForRequest(status);
	
	// We should never get here because we should have hit a panic
	// If the Panic doesnt happen, we'll cause a leave

	SetTestStepResult(EFail);
	
	User::Leave( KErrGeneral );

	return TestStepResult();
	}


TVerdict CDhcpTestStep2_GetSIPServerAddrIndexChecker::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Retrieve SIP server address (test case 4.2.13.3,16)
*/
	{
	SetTestStepResult(EFail);
    
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);

	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
		err =  RProperty::Set(KUidPSSimTsyCategory,KPSSimTsyTestNumber,0);
	TESTEL(err == KErrNone, err);

	err = connNET1.Start(iConnPrefs); // see test script for IAP used
	TESTEL(err == KErrNone, err);

	INFO_PRINTF1(_L("Start completed ok.."));

	TRequestStatus status;
	
	INFO_PRINTF1(_L("Calling ioctl.."));

	TInt i;
	// Request the first address available
    for (i = 0, status = KErrNone ;status == KErrNone && i < 50; i++)
        {
        TSipServerAddrBuf sipServerAddr;	
        sipServerAddr().index = i;
        connNET1.Ioctl(KCOLConfiguration, KConnGetSipServerAddr, status, &sipServerAddr);
        User::WaitForRequest(status);
        
    
 #if CHECK_THE_RECEIVE_DATA       
        if(status == KErrNone)
            {
            THostName addr;
            sipServerAddr().address.Output(addr);
            
            // Verify the retrieved address is correct
            TPtrC addressToCompare;
            
            addressToCompare.Set(addresses[i]);
                
            INFO_PRINTF2(_L("Received SIP address: %S"), &addr);
            if(addr.CompareF(addressToCompare) != 0)
                {
                INFO_PRINTF2(_L("Expected SIP address %S!"), &addressToCompare);
                TESTEL(0, KErrNotFound);
                }
            }
#endif
        }
        if(status != KErrNotSupported)
            {
            INFO_PRINTF2(_L("SIP option ioctl failed with code %d.."), status.Int());
            TESTEL(status.Int() == KErrNotFound, status.Int());
            }
         else
            {
            // Nifman returns KErrNotSupported if DHCP is not configured for the IAP
            // so we want to make sure DHCP fallback is only ever tried once for index
            // zero.  Otherwise, the last address index that the caller tries may return
            // KErrNotSupported instead of the expected KErrNotFound.
            ASSERT( i == 0 );
            }
    
	err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

		
	CleanupStack::PopAndDestroy(&connNET1);
	CleanupStack::PopAndDestroy(&eSock);

	SetTestStepResult(EPass);

	return TestStepResult();
	}

TVerdict CDhcpTestStep2_GetSIPServerAddrNegativeIndexChecker::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Retrieve SIP server address (test case 4.2.13.4,17)
*/
	{
	SetTestStepResult(EFail);
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);
	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	err =  RProperty::Set(KUidPSSimTsyCategory,KPSSimTsyTestNumber,0);
	TESTEL(err == KErrNone, err);
	
	err = connNET1.Start(iConnPrefs); // see test script for IAP used
	TESTEL(err == KErrNone, err);

	INFO_PRINTF1(_L("Start completed ok.."));

	TRequestStatus status;
	
	INFO_PRINTF1(_L("Calling ioctl.."));

	// Request the first address available
	TSipServerAddrBuf sipServerAddr;	
	sipServerAddr().index = -1;
	connNET1.Ioctl(KCOLConfiguration, KConnGetSipServerAddr, status, &sipServerAddr);
	User::WaitForRequest(status);
	
	if(status.Int() == KErrNotSupported)
		{
    	SetTestStepResult(EFail);
		}

	err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

		
	CleanupStack::PopAndDestroy(&connNET1);
	CleanupStack::PopAndDestroy(&eSock);

	SetTestStepResult(EPass);

	return TestStepResult();		
	}
TVerdict CDhcpTestStep2_ClearMOFlag::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* M and O flags should be set as false (test doc 5.1.3)
* Start a connection with full DHCP config
* then use the RConnectionIoctl to query for
* the configured address
*/
	{
	SetTestStepResult(EFail);
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);
	
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
	TESTEL(stat == KErrNotReady, stat.Int());
	
	err = connNET1.Stop();
	TESTEL(err == KErrNone, err);
	 
	CleanupStack::PopAndDestroy(&connNET1);
	CleanupStack::PopAndDestroy(&eSock); 

	SetTestStepResult(EPass);
	return TestStepResult();
	}
	
TVerdict CDhcpTestStep2_NoRAandDHCPServ::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* No RA nad DHCP Server on the link (test doc 5.1.5)
* Start connection should return KErrNone
*/
	{
	SetTestStepResult(EFail);
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);
#ifdef _DEBUG
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv4, 67));
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv6, 580));
#endif		
	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	err = connNET1.Start(iConnPrefs); // see test script for IAP used
	TESTEL(err == KErrNone, err);
	User::After(5000000);		// wait for DHCP SOLICIT messages to be sent out
			
	err = connNET1.Stop();
	TESTEL(err == KErrNone, err);
	 
	CleanupStack::PopAndDestroy(&connNET1);
	CleanupStack::PopAndDestroy(&eSock); 

	SetTestStepResult(EPass);
	return TestStepResult();
	}

#ifdef SYMBIAN_NETWORKING_DHCPSERVER

TVerdict CDhcpTestStep2_23::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Cap Test for DHCP server (test doc 4.2.23)
* Start connection should return KErrNone
*/
	{
	SetTestStepResult(EFail);
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);
	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	err = connNET1.Start(iConnPrefs); // see test script for IAP used
	TESTEL(err == KErrNone, err);
  
	if(RProcess().HasCapability(ECapabilityNetworkServices))
	{
		if(err == KErrPermissionDenied)
			{
			SetTestStepResult(EFail);
			CleanupStack::PopAndDestroy(&connNET1);
			CleanupStack::PopAndDestroy(&eSock); 
			return TestStepResult();
			}
	}
			
	err = connNET1.Stop();
	TESTEL(err == KErrNone, err);
	 
	CleanupStack::PopAndDestroy(&connNET1);
	CleanupStack::PopAndDestroy(&eSock); 

	SetTestStepResult(EPass);
	return TestStepResult();
	}


TVerdict CDhcpTestStep2_24::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Raw options are set by the DHCP server (test doc 4.2.24)
* Start connection should return KErrNone
*/
	{
	SetTestStepResult(EFail);
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);
	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	err = connNET1.Start(iConnPrefs); // see test script for IAP used
	TESTEL(err == KErrNone, err);

	User::After(40 * 1000000);
	
	TRequestStatus status;
	
	TBuf8<31> bufDes;
	bufDes.FillZ(31);
    TDhcp4RawOptionDataPckg pckg(bufDes);
    
	TUint opcode = 54; //OpCode Translates to EDHCPServerID for the server
	pckg.SetOpCode(opcode);
    	
    const TUint dhcpOptName = KConnSetDhcpRawOptionData;	
	
	// Set raw option data either for ip4
	connNET1.Ioctl(KCOLConfiguration, dhcpOptName, status, &bufDes);
	User::WaitForRequest(status);
	
	TESTEL(status == KErrNone, status.Int());
	
	err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::PopAndDestroy(&connNET1);
	CleanupStack::PopAndDestroy(&eSock);
	
	SetTestStepResult(EPass);
	return TestStepResult();


	}

TVerdict CDhcpTestStep2_25::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Fetch the raw option from external DHCP server.Set the RAW option.(test doc 4.2.25)
* Start connection should return KErrNone
*/
	{
	SetTestStepResult(EFail);

	//Start Socket Server
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);
	
	//Start 1st connection with DHCP client to fetch the SIP address
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);

	TCommDbConnPref prefs1;
	prefs1.SetIapId(11);
	prefs1.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);
   		
	err = connNET1.Start(prefs1);
	TESTEL(err == KErrNone, err);
	INFO_PRINTF1(_L("DHCP server started.."));

    TBuf8<48> buf;
	buf.FillZ(48);
    const TUint KOptCode = 6; //-- OPTION_CLIENTID(ipv6) or SubnetMask(ipv4)

	INFO_PRINTF1(_L("Calling ioctl.."));
    INFO_PRINTF1(_L("Getting DHCP4 raw option data"));    
    TDhcp4RawOptionDataPckg pckg(buf);
    pckg.SetOpCode((TUint8)KOptCode);	// the subnet mask
    
    TRequestStatus status;	
    // Get raw option data either for ip4 of ip6
	connNET1.Ioctl(KCOLConfiguration, KConnGetDhcp4RawOptionData, status, &buf);
    User::WaitForRequest(status);
    TESTEL(status.Int() == KErrNone, err);
    INFO_PRINTF1(_L("Ioctl completed ok.."));
    
    err = connNET1.Stop();
	TESTEL(err == KErrNone, err);
	
	RConnection connNET2;
	err = connNET2.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET2);
	
	err = connNET2.Start(iConnPrefs); // see test script for IAP used
	TESTEL(err == KErrNone, err);
	INFO_PRINTF1(_L("Waiting for connection from client..."));

	User::After(10 * 1000000);
	
	// Set raw option data either for ip4
	INFO_PRINTF1(_L("Before IOCTL call..."));
	connNET2.Ioctl(KCOLConfiguration, KConnSetDhcpRawOptionData, status, &buf);
	User::WaitForRequest(status);
	INFO_PRINTF1(_L("after IOCTL call..."));
	TESTEL(status.Int() == KErrNone, err);

    User::After(10 * 1000000);

	err = connNET2.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::PopAndDestroy(&connNET2);
	CleanupStack::PopAndDestroy(&connNET1);
	CleanupStack::PopAndDestroy(&eSock);

	SetTestStepResult(EPass);
	return TestStepResult();
	}
#endif // SYMBIAN_NETWORKING_DHCPSERVER

#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS

TVerdict CDhcpTestStep2_26::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Fetch the siaddr feild in the DHCP message header from external DHCP server.(test doc 4.2.23)
* Start connection should return KErrNone
*/

	{
	SetTestStepResult(EFail);
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);

#ifdef _DEBUG
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv4, 91));
#endif
	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	// see test script for IAP used
	err = connNET1.Start(iConnPrefs); 
	TESTEL(err == KErrNone, err);
    	
	TRequestStatus stat;
	TConnectionAddrBuf offeredTftpServerAddr;
	offeredTftpServerAddr().iAddressFamily = IpAddressFamilyL();
	
	connNET1.Ioctl(KCOLConfiguration, KConnGetDhcpHdrSiaddr, stat, &offeredTftpServerAddr);
   	User::WaitForRequest(stat);
	TESTEL(stat.Int() == KErrNone, stat.Int());

    TInetAddr offeredTftpServer(offeredTftpServerAddr().iAddr);
	THostName addr;
	offeredTftpServer.Output(addr);

    TPtrC expectedTftpServerAddressPtr;

	//Fetch the expected value from the config file
	if(!GetStringFromConfig(ConfigSection(), _L("ExpectTftpServerAddress"), expectedTftpServerAddressPtr))
	{
		INFO_PRINTF1(_L("Unable to read expected TFTP server address from the config.Leaving with KErrNotFound"));
		User::Leave(KErrNotFound);
	}
   	
	if( addr.CompareF(expectedTftpServerAddressPtr) )
	{
		TESTEL(0, KErrGeneral);
	}
	
	CleanupStack::Pop(&connNET1);
    err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::Pop(&eSock);
    eSock.Close();
	
	SetTestStepResult(EPass);

	return TestStepResult();
	}

TVerdict CDhcpTestStep2_27::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Fetch the TFTP server name in sname field in DHCP message header.(test doc 4.2.24)
* Start connection should return KErrNone
*/
	{
	SetTestStepResult(EFail);
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);

#ifdef _DEBUG
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv4, 91));
#endif
	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);

	// see test script for IAP used	
	err = connNET1.Start(iConnPrefs);
	TESTEL(err == KErrNone, err);
    	
	TRequestStatus stat;
	TBuf8<64> offeredTftpServerName; 
	connNET1.Ioctl(KCOLConfiguration, KConnGetDhcpHdrSname, stat, &offeredTftpServerName);
	User::WaitForRequest(stat);
	TESTEL(stat.Int() == KErrNone, stat.Int());

    TPtrC expectedTftpServerNamePtr;

	//Fetch the expected value from the config file
	if(!GetStringFromConfig(ConfigSection(), _L("ExpectTftpServerName1"), expectedTftpServerNamePtr))
	{
		INFO_PRINTF1(_L("Unable to read expected TFTP server name from the config.Leaving with KErrNotFound"));
		User::Leave(KErrNotFound);
	}

    HBufC8* expectedTftpServerName = CnvUtfConverter::ConvertFromUnicodeToUtf8L(expectedTftpServerNamePtr);
	CleanupStack::PushL(expectedTftpServerName);
    
	if( offeredTftpServerName.CompareF(*expectedTftpServerName) )
	{
		TESTEL(0, KErrGeneral);
	}
    
	CleanupStack::PopAndDestroy(expectedTftpServerName);

	CleanupStack::Pop(&connNET1);
    err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::Pop(&eSock);
    eSock.Close();
 
	SetTestStepResult(EPass);
    
	return TestStepResult();
	}

TVerdict CDhcpTestStep2_28::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Fetch the TFTP server name in Option 66 from external DHCP server.(test doc 4.2.25)
* Start connection should return KErrNone
*/

	{
	
	SetTestStepResult(EFail);
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);
	
#ifdef _DEBUG
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv4, 91));
#endif

	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	// see test script for IAP used
	err = connNET1.Start(iConnPrefs); 
	TESTEL(err == KErrNone, err);

	TRequestStatus stat;
	TBuf8<64> offeredTftpServerName ;
	
	connNET1.Ioctl(KCOLConfiguration, KConnGetTftpServerName, stat, &offeredTftpServerName);
	User::WaitForRequest(stat);
	TESTEL(stat.Int() == KErrNone, stat.Int());

	//Fetch the expected value from the config file
	TPtrC expectedTftpserverNamePtr;

	if(!GetStringFromConfig(ConfigSection(), _L("ExpectTftpServerName2"), expectedTftpserverNamePtr))
	{
		INFO_PRINTF1(_L("Unable to read expected TFTP server name from the config.Leaving with KErrNotFound"));
		User::Leave(KErrNotFound);
	}

	HBufC8* expectedTftpServerName = CnvUtfConverter::ConvertFromUnicodeToUtf8L(expectedTftpserverNamePtr);
	CleanupStack::PushL(expectedTftpServerName);

	if(offeredTftpServerName.CompareF(*expectedTftpServerName))
	{
		TESTEL(0, KErrGeneral);
	}
	
	CleanupStack::PopAndDestroy(expectedTftpServerName);

	CleanupStack::Pop(&connNET1);
    err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::Pop(&eSock);
    eSock.Close();

	SetTestStepResult(EPass);

	return TestStepResult();
	}


TVerdict CDhcpTestStep2_29::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Fetch the TFTP server ip address in Option 150 from external DHCP server.(test doc 4.2.26)
* Start connection should return KErrNone
*/
	{
	SetTestStepResult(EFail);
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);
	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
#ifdef _DEBUG
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv4, 91));
#endif
	
	// see test script for IAP used
	err = connNET1.Start(iConnPrefs); 
	TESTEL(err == KErrNone, err);

	TRequestStatus stat;

	// Request the first address available
	TTftpServerAddrBuf tftpServerAddr;
    tftpServerAddr ().index = 0;
	connNET1.Ioctl(KCOLConfiguration, KConnGetTftpServerAddr, stat, &tftpServerAddr);
	User::WaitForRequest(stat);
	TESTEL(stat.Int() == KErrNone, stat.Int());
	
	THostName addr;
	tftpServerAddr().address.Output(addr);
	    
	//Fetch the expected value from the config file
	TPtrC ExpectedTftpserverAddrPtr;

	if(!GetStringFromConfig(ConfigSection(), _L("ExpectTftpServerAddressIndx1"), ExpectedTftpserverAddrPtr))
	{
		INFO_PRINTF1(_L("Unable to read 1st expected TFTP server name from the config.Leaving with KErrNotFound"));
		User::Leave(KErrNotFound);
	}
 
	if(addr.CompareF(ExpectedTftpserverAddrPtr) )
	{
		TESTEL(0, KErrGeneral);
	}

	CleanupStack::Pop(&connNET1);
    err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::Pop(&eSock);
    eSock.Close();

	SetTestStepResult(EPass);

	return TestStepResult();
	}

TVerdict CDhcpTestStep2_30::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Fetch the GeoConf value in Option 123 from external DHCP server.This GeoConf value offered by the DHCP server describes the geographical spatial position of the DHCP server. (test doc 4.2.27)
* Start connection should return KErrNone
*/
	{
	
	SetTestStepResult(EFail);
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);

#ifdef _DEBUG
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv4, 91));
#endif
	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	// see test script for IAP used	
	err = connNET1.Start(iConnPrefs); 
	TESTEL(err == KErrNone, err);
	
    TBuf8<100> geoSpatialConf;
    TDhcpRawOptionMultipleDataPckg geoSpatialConfPkg(geoSpatialConf);
    
	//Adding GeoConf option
	geoSpatialConfPkg.AddRawOptionCodeL(KGeoConfOption);
	    
	TRequestStatus stat;	
    connNET1.Ioctl(KCOLConfiguration, KConnDhcpGetMultipleParams, stat, &geoSpatialConf);
	User::WaitForRequest(stat);
	TESTEL(stat.Int() == KErrNone, stat.Int());

	//Retrive the queried option 
	TPtrC8 geoSpatialConfPtr;
	err = geoSpatialConfPkg.GetRawParameterData(KGeoConfOption,geoSpatialConfPtr);
	TESTEL(err == KErrNone, err);

	//Fetch the expected value from the config file
	TPtrC expectedgeoSpatialConfPtr;

	if(!GetStringFromConfig(ConfigSection(), _L("ExpectedgeoSpatialConf"), expectedgeoSpatialConfPtr))
	{
		INFO_PRINTF1(_L("Unable to read expected Geo spatial option from the config.Leaving with KErrNotFound"));
		User::Leave(KErrNotFound);
	}
	
	HBufC8* expectedgeoSpatialConf = CnvUtfConverter::ConvertFromUnicodeToUtf8L(expectedgeoSpatialConfPtr);	
    CleanupStack::PushL(expectedgeoSpatialConf);
	
   	if( geoSpatialConfPtr.CompareF(*expectedgeoSpatialConf) )
	{
		TESTEL(0, KErrGeneral);
	}

    CleanupStack::PopAndDestroy(expectedgeoSpatialConf);

	CleanupStack::Pop(&connNET1);
    err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::Pop(&eSock);
    eSock.Close();
	
	SetTestStepResult(EPass);

	return TestStepResult();
	}

TVerdict CDhcpTestStep2_31::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Fetch the Geo Civic option value in Option 99 from external DHCP server.This value given by DHCP server denotes the civic address of the DHCP server(test doc 4.2.28)
* Start connection should return KErrNone
*/
	{
	
	SetTestStepResult(EFail);
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);

#ifdef _DEBUG
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv4, 91));
#endif
	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	// see test script for IAP used	
	err = connNET1.Start(iConnPrefs); 
	TESTEL(err == KErrNone, err);
	
	TBuf8<500> geoCivicInfo;
    TDhcpRawOptionMultipleDataPckg geoCivicInfoPkg(geoCivicInfo);
    
	// Adding GeoCivic Option
	geoCivicInfoPkg.AddRawOptionCodeL(KGeoConfCivicOption);
    
	TRequestStatus stat;	
    connNET1.Ioctl(KCOLConfiguration, KConnDhcpGetMultipleParams, stat, &geoCivicInfo);
	User::WaitForRequest(stat);
	TESTEL(stat.Int() == KErrNone, stat.Int());

	//Retrive the queried option 
	TPtrC8 geoCivicPtr;
	err = geoCivicInfoPkg.GetRawParameterData(KGeoConfCivicOption ,geoCivicPtr);
	TESTEL(err == KErrNone, err);

    //Fetch the expected value from the config file
	TPtrC expectedGeoCivicInformationPtr;

	if(!GetStringFromConfig(ConfigSection(), _L("ExpectedGeoCivicInformation"), expectedGeoCivicInformationPtr))
	{
		INFO_PRINTF1(_L("Unable to read expected Geo civic information from the config.Leaving with KErrNotFound"));
		User::Leave(KErrNotFound);
	}
	
	HBufC8* expectedGeoCivicInformation = CnvUtfConverter::ConvertFromUnicodeToUtf8L(expectedGeoCivicInformationPtr);	
	CleanupStack::PushL(expectedGeoCivicInformation);

	if(geoCivicPtr.CompareF(*expectedGeoCivicInformation) )
	{
		TESTEL(0, KErrGeneral);
	}
	
	CleanupStack::PopAndDestroy(expectedGeoCivicInformation);	
	
	CleanupStack::Pop(&connNET1);
    err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::Pop(&eSock);
    eSock.Close();

	SetTestStepResult(EPass);
 
	return TestStepResult();
	}

TVerdict CDhcpTestStep2_32::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Fetch the multiple options from external DHCP server.
* TFTP server IP address in option 150 and Geo Civic address in option 99 are fetched(test doc 4.2.29)
* Start connection should return KErrNone
*/
	{

	SetTestStepResult(EFail);
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);
	
#ifdef _DEBUG
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv4, 91));
#endif

	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	// see test script for IAP used
	err = connNET1.Start(iConnPrefs); 
	TESTEL(err == KErrNone, err);
	
	TBuf8<1000> multipleOptionBuf;
    TDhcpRawOptionMultipleDataPckg multipleOptionBufPkg(multipleOptionBuf);
    
	// Adding TFTP server address option
	multipleOptionBufPkg.AddRawOptionCodeL(KTFtpServerAddress);
    
	// Adding GeoCivic Option
	multipleOptionBufPkg.AddRawOptionCodeL(KGeoConfCivicOption);
    
	TRequestStatus stat;
	connNET1.Ioctl(KCOLConfiguration, KConnDhcpGetMultipleParams, stat, &multipleOptionBuf);
	User::WaitForRequest(stat);
	TESTEL(stat.Int() == KErrNone, stat.Int());

	// Request the first address available
	TTftpServerAddrBuf tftpServerAddr;
    tftpServerAddr ().index = 1;
	connNET1.Ioctl(KCOLConfiguration, KConnGetTftpServerAddr, stat, &tftpServerAddr);
	User::WaitForRequest(stat);
	TESTEL(stat == KErrNone, stat.Int());
	
	THostName addr;
	tftpServerAddr().address.Output(addr);

	//Fetch the expected value from the config file
	TPtrC expectedTftpserverAddrPtr;

	if(!GetStringFromConfig(ConfigSection(), _L("ExpectTftpServerAddressIndx2"), expectedTftpserverAddrPtr))
	{
		INFO_PRINTF1(_L("Unable to read 2nd TFTP server address from the config.Leaving with KErrNotFound"));
		User::Leave(KErrNotFound);
	}
	

	if(addr.CompareF(expectedTftpserverAddrPtr) )
	{
		TESTEL(0, KErrGeneral);
	}
    
	//Retrive the queried option 
	TPtrC8 geoCivicOptionPtr;
	err = multipleOptionBufPkg.GetRawParameterData(KGeoConfCivicOption,geoCivicOptionPtr);
	TESTEL(err == KErrNone, err);

	
    //Fetch the expected value from the config file
	TPtrC expectedGeoCivicInformationPtr;

	if(!GetStringFromConfig(ConfigSection(), _L("ExpectedGeoCivicInformation"), expectedGeoCivicInformationPtr))
	{
		INFO_PRINTF1(_L("Unable to read expected Geo civic information from the config.Leaving with KErrNotFound"));
		User::Leave(KErrNotFound);
	}
	
	HBufC8* expectedGeoCivicInformation = CnvUtfConverter::ConvertFromUnicodeToUtf8L(expectedGeoCivicInformationPtr);
	CleanupStack::PushL(expectedGeoCivicInformation);
		
	if(0 != geoCivicOptionPtr.CompareF(*expectedGeoCivicInformation) )
	{
		TESTEL(0, KErrGeneral);
	}

    CleanupStack::PopAndDestroy(expectedGeoCivicInformation);
	
	CleanupStack::Pop(&connNET1);
    err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::Pop(&eSock);
    eSock.Close();

	SetTestStepResult(EPass);
   	
	return TestStepResult();
	}

TVerdict CDhcpTestStep2_33::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Fetch TFTP server name(Option 66) with insufficient buffer.(test doc 4.2.30)
* Start connection should return KErrNone
*/
	{

	SetTestStepResult(EFail);
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);
	
#ifdef _DEBUG
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv4, 91));
#endif

	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	// see test script for IAP used
	err = connNET1.Start(iConnPrefs); 
	TESTEL(err == KErrNone, err);

	TRequestStatus stat;
	TBuf8<1> tftpServerName ;
	connNET1.Ioctl(KCOLConfiguration, KConnGetTftpServerName, stat, &tftpServerName);
	User::WaitForRequest(stat);
	TESTEL(stat.Int() == KErrOverflow, stat.Int());
    
	CleanupStack::Pop(&connNET1);
    err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::Pop(&eSock);
    eSock.Close();

	SetTestStepResult(EPass);

	return TestStepResult();	
	}

TVerdict CDhcpTestStep2_34::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Fetch the value SIADDR feild when server has not offered SIADDR.(test doc 4.2.31)
* Start connection should return KErrNone
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
	
	// see test script for IAP used
	err = connNET1.Start(iConnPrefs); 
	TESTEL(err == KErrNone, err);
    	
	TRequestStatus stat;
	TConnectionAddrBuf siaddrToRetrieve;
	siaddrToRetrieve().iAddressFamily = IpAddressFamilyL();
	connNET1.Ioctl(KCOLConfiguration, KConnGetDhcpHdrSiaddr, stat, &siaddrToRetrieve);
	User::WaitForRequest(stat);
	TESTEL(stat.Int() == KErrNone, stat.Int());
	
	CleanupStack::Pop(&connNET1);
    err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::Pop(&eSock);
    eSock.Close();
	
	SetTestStepResult(EPass);

	return TestStepResult();
	}

TVerdict CDhcpTestStep2_35::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Fetch option 66 when the same has not been offered by the external DHCP server.(test doc 4.2.32)
* Start connection should return KErrNone
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
	
	// see test script for IAP used
	err = connNET1.Start(iConnPrefs); 
	TESTEL(err == KErrNone, err);
    	
	TBuf8<100> multipleOptionBuf;
    TDhcpRawOptionMultipleDataPckg multipleOptionBufPckg(multipleOptionBuf);
    
    // Adding TFTP server address option
	multipleOptionBufPckg.AddRawOptionCodeL(KTFtpServerAddress);
       
	TRequestStatus stat;	
	connNET1.Ioctl(KCOLConfiguration, KConnDhcpGetMultipleParams, stat, &multipleOptionBuf);
	User::WaitForRequest(stat); 
    TESTEL(stat.Int() == KErrNotFound, stat.Int());

	TBuf8<64> offeredTftpServerName ;
	connNET1.Ioctl(KCOLConfiguration, KConnGetTftpServerName, stat, &offeredTftpServerName);
	User::WaitForRequest(stat);
	TESTEL(stat == KErrNotFound, stat.Int());

	
	CleanupStack::Pop(&connNET1);
    err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::Pop(&eSock);
    eSock.Close();

	SetTestStepResult(EPass);
	
	return TestStepResult();
	}

TVerdict CDhcpTestStep2_36::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Fetch indexed multiple TFTP server IP addresses.offered by the external DHCP server.(test doc 4.2.33)
* Start connection should return KErrNone
*/
	{

	SetTestStepResult(EFail);
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);
	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
#ifdef _DEBUG
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv4, 91));
#endif
	
	// see test script for IAP used
	err = connNET1.Start(iConnPrefs); 
	TESTEL(err == KErrNone, err);

	TRequestStatus stat;

	// Request the first address offered
	TTftpServerAddrBuf tftpServerAddr1;
    tftpServerAddr1().index = 0;
	connNET1.Ioctl(KCOLConfiguration, KConnGetTftpServerAddr, stat, & tftpServerAddr1);
	User::WaitForRequest(stat);
	TESTEL(stat.Int() == KErrNone, stat.Int());
	
    THostName addr;
	tftpServerAddr1().address.Output(addr);

	//Fetch the expected value from the config file
	TPtrC expectedTftpserverAddrPtr;

	if(!GetStringFromConfig(ConfigSection(), _L("ExpectTftpServerAddressIndx1"), expectedTftpserverAddrPtr))
	{
		INFO_PRINTF1(_L("Unable to read 1st TFTP server address from the config.Leaving with KErrNotFound"));
		User::Leave(KErrNotFound);
	}
	
	if( addr.CompareF(expectedTftpserverAddrPtr) )
	{
		TESTEL(0, KErrGeneral);
	}


	// Request the second address offered
	TTftpServerAddrBuf tftpServerAddr2;
    tftpServerAddr2().index = 1;
	connNET1.Ioctl(KCOLConfiguration, KConnGetTftpServerAddr, stat, & tftpServerAddr2);
	User::WaitForRequest(stat);
	TESTEL(stat.Int() == KErrNone, stat.Int());

	tftpServerAddr2().address.Output(addr);
	
	//Fetch the expected value from the config file
    if(!GetStringFromConfig(ConfigSection(), _L("ExpectTftpServerAddressIndx2"), expectedTftpserverAddrPtr))
	{
		INFO_PRINTF1(_L("Unable to read 2nd TFTP server address from the config.Leaving with KErrNotFound"));
		User::Leave(KErrNotFound);
	}

	if(addr.CompareF(expectedTftpserverAddrPtr) )
	{
		TESTEL(0, KErrGeneral);
	}

	// Request the third address offered
	TTftpServerAddrBuf tftpServerAddr3;
    tftpServerAddr3().index = 2;
	connNET1.Ioctl(KCOLConfiguration, KConnGetTftpServerAddr, stat, & tftpServerAddr3);
	User::WaitForRequest(stat);
	TESTEL(stat.Int() == KErrNone, stat.Int());

	tftpServerAddr3().address.Output(addr);
	
	//Fetch the expected value from the config file
    if(!GetStringFromConfig(ConfigSection(), _L("ExpectTftpServerAddressIndx3"), expectedTftpserverAddrPtr))
	{
		INFO_PRINTF1(_L("Unable to read 3rd TFTP server address from the config.Leaving with KErrNotFound"));
		User::Leave(KErrNotFound);
	}
	
	if( addr.CompareF(expectedTftpserverAddrPtr) )
	{
		TESTEL(0, KErrGeneral);
	}
	
    CleanupStack::Pop(&connNET1);
    err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::Pop(&eSock);
    eSock.Close();
	
	SetTestStepResult(EPass);

	return TestStepResult();
	}

TVerdict CDhcpTestStep2_37::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Fetch non existant indexed TFTP server IP adress.(test doc 4.2.34)
* Start connection should return KErrNone
*/
	{
	SetTestStepResult(EFail);
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);
	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
#ifdef _DEBUG
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv4, 91));
#endif
	
	// see test script for IAP used
	err = connNET1.Start(iConnPrefs); 
	TESTEL(err == KErrNone, err);

	TRequestStatus stat;

	// Request the non-exitant tenth address offered
	TTftpServerAddrBuf tftpServerAddr;
    tftpServerAddr ().index = 10;
	connNET1.Ioctl(KCOLConfiguration, KConnGetTftpServerAddr, stat, & tftpServerAddr);
	User::WaitForRequest(stat);
	TESTEL(stat.Int() == KErrNotFound, stat.Int());
	
	CleanupStack::Pop(&connNET1);
    err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::Pop(&eSock);
    eSock.Close();

	SetTestStepResult(EPass);
	
	return TestStepResult();
	}

TVerdict CDhcpTestStep2_38::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Fetch swap server IP adress.(test doc 4.2.35)
* Start connection should return KErrNone
*/

	{
	SetTestStepResult(EFail);
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);

#ifdef _DEBUG
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv4, 91));
#endif
	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	// see test script for IAP used
	err = connNET1.Start(iConnPrefs); 
	TESTEL(err == KErrNone, err);
	
	TBuf8<100> swapServerAddr;
    TDhcpRawOptionMultipleDataPckg swapServerAddrPkg(swapServerAddr);
    
    // Adding swap server address option
	swapServerAddrPkg.AddRawOptionCodeL(KSwapServerAddress);
    
	TRequestStatus stat;	
    connNET1.Ioctl(KCOLConfiguration, KConnDhcpGetMultipleParams, stat, &swapServerAddr);
	User::WaitForRequest(stat);
	TESTEL(stat.Int() == KErrNone, stat.Int());

	CleanupStack::Pop(&connNET1);
    err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::Pop(&eSock);
    eSock.Close();

	SetTestStepResult(EPass);
 
	return TestStepResult();
	}

TVerdict CDhcpTestStep2_39::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Fetch TFTP server IP adress,Geo Civic value and GeoSpatial Option (test doc 4.2.36)
* Start connection should return KErrNone
*/

	{
	SetTestStepResult(EFail);
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);

#ifdef _DEBUG
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv4, 91));
#endif
	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	// see test script for IAP used
	err = connNET1.Start(iConnPrefs); 
	TESTEL(err == KErrNone, err);
	
	//Request Geo Civic Info
	TRequestStatus stat;

	TBuf8<1000> multipleOptionsBuf;
    TDhcpRawOptionMultipleDataPckg multipleOptionsPkg(multipleOptionsBuf);
    
	// Adding GeoCivic Option
	multipleOptionsPkg.AddRawOptionCodeL(KGeoConfCivicOption);
   
    // Adding GeoConf option
	multipleOptionsPkg.AddRawOptionCodeL(KGeoConfOption);
	
	// Adding TFTP server name option
	multipleOptionsPkg.AddRawOptionCodeL(KTFtpServerName);

	connNET1.Ioctl(KCOLConfiguration, KConnDhcpGetMultipleParams, stat, &multipleOptionsBuf);
	User::WaitForRequest(stat);
	TESTEL(stat.Int() == KErrNone, stat.Int());

	TPtrC8 geoCivicPtr;
	err = multipleOptionsPkg.GetRawParameterData(KGeoConfCivicOption,geoCivicPtr);
	TESTEL(err == KErrNone, err);
	
   	//Fetch the expected value from the config file
   	TPtrC expectedValPtr;

	if(!GetStringFromConfig(ConfigSection(), _L("ExpectedGeoCivicInformation"), expectedValPtr))
	{
		INFO_PRINTF1(_L("Unable to read expected Geo civic information from the config.Leaving with KErrNotFound"));
		User::Leave(KErrNotFound);
	}
	
	HBufC8* expectedVal = CnvUtfConverter::ConvertFromUnicodeToUtf8L(expectedValPtr);
	CleanupStack::PushL(expectedVal);
	
	if(geoCivicPtr.CompareF(*expectedVal) )
	{
		TESTEL(0, KErrGeneral);
	}

	CleanupStack::PopAndDestroy(expectedVal);

	TPtrC8 geoCivicLocPtr;
	err = multipleOptionsPkg.GetRawParameterData(KGeoConfOption,geoCivicLocPtr);
	TESTEL(err == KErrNone, err);

	//Fetch the expected value from the config file
	if(!GetStringFromConfig(ConfigSection(), _L("ExpectedgeoSpatialConf"), expectedValPtr))
	{
		INFO_PRINTF1(_L("Unable to read expected Geo spatial information from the config.Leaving with KErrNotFound"));
		User::Leave(KErrNotFound);
	}
		
	expectedVal = CnvUtfConverter::ConvertFromUnicodeToUtf8L(expectedValPtr);	
	CleanupStack::PushL(expectedVal);
	
	if(geoCivicLocPtr.CompareF(*expectedVal) )
	{
		TESTEL(0, KErrGeneral);
	}

    CleanupStack::PopAndDestroy(expectedVal);
	
	TPtrC8 tftpServerNamePtr;
	multipleOptionsPkg.GetRawParameterData(KTFtpServerName,tftpServerNamePtr);
	TESTEL(err == KErrNone, err);

	//Fetch the expected value from the config file
	if(!GetStringFromConfig(ConfigSection(), _L("ExpectTftpServerName2"), expectedValPtr))
	{
		INFO_PRINTF1(_L("Unable to TFTP server address from the config.Leaving with KErrNotFound"));
		User::Leave(KErrNotFound);
	}
		
	expectedVal = CnvUtfConverter::ConvertFromUnicodeToUtf8L(expectedValPtr);	
	CleanupStack::PushL(expectedVal);

	if(tftpServerNamePtr.CompareF(*expectedVal) )
	{
		TESTEL(0, KErrGeneral);
	}
	
    CleanupStack::PopAndDestroy(expectedVal);
	
	CleanupStack::Pop(&connNET1);
    err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::Pop(&eSock);
    eSock.Close();

	SetTestStepResult(EPass);

	return TestStepResult();
	}


TVerdict CDhcpTestStep2_40::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Fetch Geo Civic option value from DHCP server (test doc 4.2.37)
* Start connection should return KErrNone
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
	
	// see test script for IAP used
	err = connNET1.Start(iConnPrefs); 
	TESTEL(err == KErrNone, err);
	
	TBuf8<100> geoCivicInfo;
    TDhcpRawOptionMultipleDataPckg geoCivicInfoPkg(geoCivicInfo);
    
	// Adding GeoCivic Option
	geoCivicInfoPkg.AddRawOptionCodeL(KGeoConfCivicOption);
    
	TRequestStatus stat;	
    connNET1.Ioctl(KCOLConfiguration, KConnDhcpGetMultipleParams, stat, &geoCivicInfo);
	User::WaitForRequest(stat);
	TESTEL(stat.Int() == KErrNotFound, stat.Int());
	
	CleanupStack::Pop(&connNET1);
    err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::Pop(&eSock);
    eSock.Close();
	SetTestStepResult(EPass);	

	return TestStepResult();
	}
	
TVerdict CDhcpTestStep2_41::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Fetch the TFTP server name in Option 66 from external DHCP server.This tests the option overload condition (test doc 4.2.25)
* Start connection should return KErrNone
*/

	{
	SetTestStepResult(EFail);
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);
	
#ifdef _DEBUG
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv4, 98));
#endif

	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	// see test script for IAP used
	err = connNET1.Start(iConnPrefs); 
	TESTEL(err == KErrNone, err);

	TRequestStatus stat;
	TBuf8<64> offeredTftpServerName ;
	
	connNET1.Ioctl(KCOLConfiguration, KConnGetTftpServerName, stat, &offeredTftpServerName);
	User::WaitForRequest(stat);
	TESTEL(stat.Int() == KErrNone, stat.Int());

	CleanupStack::Pop(&connNET1);
    err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::Pop(&eSock);
    eSock.Close();

	SetTestStepResult(EPass);

	return TestStepResult();
	}
	
#endif // SYMBIAN_NETWORKING_DHCP_MSG_HEADERS

/* Definitions for DHCP testing with Codenomicon tool*/

TVerdict CDhcpTestStepCodenomicon1::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Get Current IP Address (test doc 4.2.2)
* Start a connection with full DHCP config
* then use the RConnectionIoctl to query for
* the configured address
*/
	{
	SetTestStepResult(EFail);

	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);
	
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
	
	// we have got an address back from the daemon...now we must check
	// that this address is correct...for this we bind a socket to it!
	RSocket socket;
	err = socket.Open(eSock, KAfInet, KSockDatagram, KProtocolInetUdp, connNET1);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(socket);

	TInetAddr* properAddr = (TInetAddr*)&(address().iAddr);	

	err = socket.Bind(*properAddr);
	TESTEL(err == KErrNone, err);

	CleanupStack::PopAndDestroy(&socket);

	/* Writing ipaddress into c:\ipaddress.txt */
	TUint32 addr1 = properAddr->Address();
	TBuf<0x20> buf;
	
	properAddr->Output(buf);

	INFO_PRINTF2(_L("Ip address %S"), &buf);
	RFs fileserver;
	User::LeaveIfError(fileserver.Connect());
	RFile fp;
	err = fp.Replace(fileserver,_L("c:\\ipaddress.txt"),EFileWrite|EFileStream);

	TBuf8<0x20> buf1;
	buf1.Copy(buf);
	fp.Write(buf1);
	
	fp.Close();
	fileserver.Close(); 
	/* IP address has written into file */
	err = connNET1.Stop();		
		
	TESTEL(err == KErrNone, err);
	 
	CleanupStack::PopAndDestroy(&connNET1);
	CleanupStack::PopAndDestroy(&eSock); 

	SetTestStepResult(EPass);
	return TestStepResult();
	}

TVerdict CDhcpTestStepCodenomicon2::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Get Current IP Address (test doc 4.2.2)
* Start a connection with full DHCP config
* then use the RConnectionIoctl to query for
* the configured address
*/
	{
	SetTestStepResult(EFail);
	INFO_PRINTF1(_L("Codenomicon test entered..."));
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);
	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);

	TInt totaltime=0;
	while(totaltime < 200)
		{
		err = connNET1.Start(iConnPrefs); // see test script for IAP used
		TESTEL(err == KErrNone, err);
	
		TRequestStatus stat;
		TConnectionAddrBuf address;
		address().iAddressFamily = IpAddressFamilyL();
		connNET1.Ioctl(KCOLConfiguration, KConnGetCurrentAddr, stat, &address);
		User::WaitForRequest(stat);
		TESTEL(stat == KErrNone, stat.Int());
	
		// we have got an address back from the daemon...now we must check
		// that this address is correct...for this we bind a socket to it!
		RSocket socket;
		err = socket.Open(eSock, KAfInet, KSockDatagram, KProtocolInetUdp, connNET1);
		TESTEL(err == KErrNone, err);
		CleanupClosePushL(socket);

		TInetAddr* properAddr = (TInetAddr*)&(address().iAddr);	

		err = socket.Bind(*properAddr);
		TESTEL(err == KErrNone, err);

		CleanupStack::PopAndDestroy(&socket);

		err = connNET1.Stop();
		User::After(100000);
		totaltime++;
		}
	
	TESTEL(err == KErrNone, err);
	 
	CleanupStack::PopAndDestroy(&connNET1);
	CleanupStack::PopAndDestroy(&eSock); 
	INFO_PRINTF1(_L("Codenomicon test finished..."));
	SetTestStepResult(EPass);
	return TestStepResult();
	}

#ifdef SYMBIAN_TCPIPDHCP_UPDATE
TVerdict CDhcpTestStep2_DomainSearchList_Test1::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Test case description : Retrieve domain name1 and domain name 2 from the domain 
* search list option(option 24)
*/
	{
	SetTestStepResult(EFail);
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);
#ifdef _DEBUG
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv6, 547));
#endif		
	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	err = connNET1.Start(iConnPrefs); // see test script for IAP used
	TESTEL(err == KErrNone, err);

	INFO_PRINTF1(_L("Start completed ok.."));

	TRequestStatus status;
	
	INFO_PRINTF1(_L("Calling ioctl.."));

	// Request the first domain available
	TDomainSearchListBuf searchlistDomainName;
	TBool checkTheReceivedData = ETrue;
	searchlistDomainName().index = 0;	
	connNET1.Ioctl(KCOLConfiguration, KConnGetDomainSearchList, status, &searchlistDomainName);
	User::WaitForRequest(status);
	
	if(status.Int() != KErrNone)
		{
		INFO_PRINTF2(_L("Domain Search list option ioctl failed with code %d.."), status.Int());
		TESTEL(status.Int() == KErrNone, status.Int());
		}
	TPtrC expecteddomainnameptr;
	if(checkTheReceivedData)
		{
		if(UsingIPv6L())
			{
			if(!GetStringFromConfig(ConfigSection(), _L("ExpectedDomainName1"), expecteddomainnameptr))
				{
				INFO_PRINTF1(_L("Unable to read expected domain name1 from the config.Leaving with KErrNotFound"));
				User::Leave(KErrNotFound);
				}

			if(searchlistDomainName().domainname.CompareF(expecteddomainnameptr))
				{
				TESTEL(0, KErrGeneral);
				}
			}
		}
		// Request the first domain available
		searchlistDomainName().index = 1;	
		connNET1.Ioctl(KCOLConfiguration, KConnGetDomainSearchList, status, &searchlistDomainName);
		User::WaitForRequest(status);
		
		if(status.Int() != KErrNone)
			{
			INFO_PRINTF2(_L("Domain Search list option ioctl failed with code %d.."), status.Int());
			TESTEL(status.Int() == KErrNone, status.Int());
			}

		if(checkTheReceivedData)
			{
			if(UsingIPv6L())
				{
				if(!GetStringFromConfig(ConfigSection(), _L("ExpectedDomainName2"), expecteddomainnameptr))
					{
					INFO_PRINTF1(_L("Unable to read expected domain name2 from the config.Leaving with KErrNotFound"));
					User::Leave(KErrNotFound);
					}

				if(searchlistDomainName().domainname.CompareF(expecteddomainnameptr))
					{
					TESTEL(0, KErrGeneral);
					SetTestStepResult(EFail);
					}
				else
					{
					SetTestStepResult(EPass);
					}
				}
			}
		
	CleanupStack::Pop(&connNET1);
    err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::Pop(&eSock);
    eSock.Close();

	return TestStepResult();
	}

TVerdict CDhcpTestStep2_DomainSearchList_Test2::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Test case description : Negative test : Try to retrieve domain name from the 
* domain serach list option by passing index as -1,the Ioctl should fail with KErrNotFound
*/
	{
	SetTestStepResult(EFail);
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);
#ifdef _DEBUG
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv6, 547));
#endif		
	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	err = connNET1.Start(iConnPrefs); // see test script for IAP used
	TESTEL(err == KErrNone, err);

	INFO_PRINTF1(_L("Start completed ok.."));

	DHCPDebug::State state;
	GetDebugHandleL(connNET1);
	LOG_STATEL;

	ImmediateCompletionTestL(connNET1);
	
	TRequestStatus status;
	
	INFO_PRINTF1(_L("Calling ioctl.."));

	// Request the first domain available
	TDomainSearchListBuf searchlistDomainName;
	searchlistDomainName().index = -1;	
	connNET1.Ioctl(KCOLConfiguration, KConnGetDomainSearchList, status, &searchlistDomainName);
	User::WaitForRequest(status);
	TESTEL(status == KErrNotFound, status.Int());
	
	if(status.Int() == KErrNotFound)
		{
		INFO_PRINTF1(_L("Domain Search list option ioctl invalid test pass.."));
		SetTestStepResult(EPass);
		}
	else
		{
		INFO_PRINTF1(_L("Domain Search list option ioctl invalid test Fail.."));
		SetTestStepResult(EFail);
		}

		
	LOG_STATEL;
	CleanupStack::Pop(&connNET1);
    err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::Pop(&eSock);
    eSock.Close();

	return TestStepResult();		
	}

TVerdict CDhcpTestStep2_DomainSearchList_Test3::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Test case description : Negative test : Try to retrieve search list domain name from the 
* sip domain option
*/
	{
	SetTestStepResult(EFail);
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);
#ifdef _DEBUG
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv6, 547));
#endif		
	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	err = connNET1.Start(iConnPrefs); // see test script for IAP used
	TESTEL(err == KErrNone, err);

	INFO_PRINTF1(_L("Start completed ok.."));
	
	TRequestStatus status;
	
	INFO_PRINTF1(_L("Calling ioctl.."));

	// Request the first domain available
	TDomainSearchListBuf searchlistDomainName;
	searchlistDomainName().index = 0;	
	connNET1.Ioctl(KCOLConfiguration, KConnGetSipServerDomain, status, &searchlistDomainName);
	User::WaitForRequest(status);
	
	TBool checkTheReceivedData = ETrue;	
	
	if(status.Int() != KErrNone)
		{
		INFO_PRINTF2(_L("Domain Search list option ioctl failed with code %d.."), status.Int());
		TESTEL(status.Int() == KErrNone, status.Int());
		}
	
	TPtrC expecteddomainnameptr;
	if(!GetStringFromConfig(ConfigSection(), _L("ExpectedDomainName1"), expecteddomainnameptr))
		{
		INFO_PRINTF1(_L("Unable to read expected domain name1 from the config.Leaving with KErrNotFound"));
		User::Leave(KErrNotFound);
		}

	if(searchlistDomainName().domainname.CompareF(expecteddomainnameptr))
		{
		SetTestStepResult(EPass);
		}
	else
		{
		TESTEL(0, KErrGeneral);
		SetTestStepResult(EFail);
		}
		
	CleanupStack::Pop(&connNET1);
    err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::Pop(&eSock);
    eSock.Close();

	return TestStepResult();	
	}

TVerdict CDhcpTestStep2_DomainSearchList_Test4::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Test case description : Try to retrieve domain name from the domain serach list option 
* by using sipdomain structure
*/
	{
	SetTestStepResult(EFail);
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);
#ifdef _DEBUG
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv6, 547));
#endif		
	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	err = connNET1.Start(iConnPrefs); // see test script for IAP used
	TESTEL(err == KErrNone, err);

	INFO_PRINTF1(_L("Start completed ok.."));
	
	TRequestStatus status;
	
	INFO_PRINTF1(_L("Calling ioctl.."));

	// Request the first domain available
	TSipServerDomainBuf sipServerDomain;
	sipServerDomain().index = 0;	
	connNET1.Ioctl(KCOLConfiguration, KConnGetDomainSearchList, status, &sipServerDomain);
	User::WaitForRequest(status);
	
	TBool checkTheReceivedData = ETrue;	
	
	if(status.Int() != KErrNone)
		{
		INFO_PRINTF2(_L("Domain Search list option ioctl failed with code %d.."), status.Int());
		TESTEL(status.Int() == KErrNone, status.Int());
		}
	TPtrC expecteddomainnameptr;
	if(checkTheReceivedData)
		{
		if(UsingIPv6L())
			{
			if(!GetStringFromConfig(ConfigSection(), _L("ExpectedDomainName1"), expecteddomainnameptr))
				{
				INFO_PRINTF1(_L("Unable to read expected domain name1 from the config.Leaving with KErrNotFound"));
				User::Leave(KErrNotFound);
				}

			if(sipServerDomain().domainName.CompareF(expecteddomainnameptr))
				{
				TESTEL(0, KErrGeneral);
				SetTestStepResult(EFail);
				}
			else
				{
				SetTestStepResult(EPass);
				}
			}
		}
	CleanupStack::Pop(&connNET1);
    err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::Pop(&eSock);
    eSock.Close();

	return TestStepResult();	
	}

TVerdict CDhcpTestStep2_DomainSearchList_Test5::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Test case description : Negative test,Try to retrieve domain name from the domain serach list option 
* by passing the index as 500
*/
	{
	SetTestStepResult(EFail);
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);
#ifdef _DEBUG
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv6, 547));
#endif		
	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	err = connNET1.Start(iConnPrefs); // see test script for IAP used
	TESTEL(err == KErrNone, err);

	INFO_PRINTF1(_L("Start completed ok.."));

	DHCPDebug::State state;
	GetDebugHandleL(connNET1);
	LOG_STATEL;

	ImmediateCompletionTestL(connNET1);
	
	TRequestStatus status;
	
	INFO_PRINTF1(_L("Calling ioctl.."));

	// Request the first domain available
	TDomainSearchListBuf searchlistDomainName;
	searchlistDomainName().index = 500;	
	connNET1.Ioctl(KCOLConfiguration, KConnGetDomainSearchList, status, &searchlistDomainName);
	User::WaitForRequest(status);
	
	if(status.Int() == KErrNotFound)
		{
		INFO_PRINTF1(_L("Domain Search list option ioctl Invalid buffer test pass "));
		SetTestStepResult(EPass);
		}
	else
		{
		INFO_PRINTF1(_L("Domain Search list option ioctl Invalid buffer test fail "));
		SetTestStepResult(EFail);
		}
			
	LOG_STATEL;
	CleanupStack::Pop(&connNET1);
    err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::Pop(&eSock);
    eSock.Close();

	return TestStepResult();		
	}

TVerdict CDhcpTestStep2_DNSServerList_Test1::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Test case description : Retrieve DNS Server address from the DNS Server list option(option 23)
*/
	{
	SetTestStepResult(EFail);
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);
#ifdef _DEBUG
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv6, 547));
#endif		
	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	err = connNET1.Start(iConnPrefs); // see test script for IAP used
	TESTEL(err == KErrNone, err);

	INFO_PRINTF1(_L("Start completed ok.."));

	TRequestStatus status;
	
	INFO_PRINTF1(_L("Calling ioctl.."));

	// Request the first domain available
	TDNSServerAddrBuf serverAddress;
	serverAddress().index = 0;	
	connNET1.Ioctl(KCOLConfiguration, KConnGetDNSServerList, status, &serverAddress);
	User::WaitForRequest(status);

	if(status.Int() != KErrNone)
		{
		INFO_PRINTF2(_L("DNS Server Search list option ioctl failed with code %d.."), status.Int());
		TESTEL(status.Int() == KErrNone, status.Int());
		}
	TPtrC addressToCompare;
	
	THostName addr;
	serverAddress().addres.Output(addr);
	
	// Verify the retrieved address is correct
	if(UsingIPv6L())
		{
		//Fetch the expected value from the config file
		if(!GetStringFromConfig(ConfigSection(), _L("ExpectDNSServerAddress1"), addressToCompare))
			{
			INFO_PRINTF1(_L("Unable to read expected DNS server address from the config.Leaving with KErrNotFound"));
			User::Leave(KErrNotFound);
			}
	   	
		if(addr.CompareF(addressToCompare))
			{
			TESTEL(0, KErrGeneral);
			SetTestStepResult(EFail);
			}
		else
			{
			SetTestStepResult(EPass);
			}
		}
				
	serverAddress().index = 1;	
	connNET1.Ioctl(KCOLConfiguration, KConnGetDNSServerList, status, &serverAddress);
	User::WaitForRequest(status);
	if(status.Int() != KErrNone)
		{
		INFO_PRINTF2(_L("DNS Server Search list option ioctl failed with code %d.."), status.Int());
		TESTEL(status.Int() == KErrNone, status.Int());
		}
	
	//THostName addr;
	serverAddress().addres.Output(addr);
		
	if(UsingIPv6L())
		{
		//Fetch the expected value from the config file
		if(!GetStringFromConfig(ConfigSection(), _L("ExpectDNSServerAddress2"), addressToCompare))
			{
			INFO_PRINTF1(_L("Unable to read expected DNS server address from the config.Leaving with KErrNotFound"));
			User::Leave(KErrNotFound);
			}
	   	
		if(addr.CompareF(addressToCompare))
			{
			TESTEL(0, KErrGeneral);
			SetTestStepResult(EFail);
			}
		else
			{
			SetTestStepResult(EPass);
			}
		}
	CleanupStack::Pop(&connNET1);
    err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::Pop(&eSock);
    eSock.Close();

	return TestStepResult();		
	}

TVerdict CDhcpTestStep2_DNSServerList_Test2::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Test case description : Negative test,Retrieve DNS Server address from the DNS Server list 
* option(option 23)by passing index as -1
*/
	{
	SetTestStepResult(EFail);
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);
#ifdef _DEBUG
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv6, 547));
#endif		
	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	err = connNET1.Start(iConnPrefs); // see test script for IAP used
	TESTEL(err == KErrNone, err);

	INFO_PRINTF1(_L("Start completed ok.."));

	DHCPDebug::State state;
	GetDebugHandleL(connNET1);
	LOG_STATEL;

	ImmediateCompletionTestL(connNET1);
	
	TRequestStatus status;
	
	INFO_PRINTF1(_L("Calling ioctl.."));

	// Request the first domain available
	TDNSServerAddrBuf serverAddress;
	serverAddress().index = -1;	
	connNET1.Ioctl(KCOLConfiguration, KConnGetDNSServerList, status, &serverAddress);
	User::WaitForRequest(status);
	TESTEL(status == KErrNotFound, status.Int());
		
	if(status.Int() == KErrNotFound)
		{
		INFO_PRINTF1(_L("DNS Server list option ioctl invalid test pass.."));
		SetTestStepResult(EPass);
		}
	else
		{
		INFO_PRINTF1(_L("DNS Server list option ioctl invalid test Fail.."));
		SetTestStepResult(EFail);
		}
	
	LOG_STATEL;
	CleanupStack::Pop(&connNET1);
    err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::Pop(&eSock);
    eSock.Close();

	return TestStepResult();		
	}

TVerdict CDhcpTestStep2_DNSServerList_Test3::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Test case description : Negative test,Retrieve DNS Server address from the Sip Server
* address option.
*/
	{
	SetTestStepResult(EFail);
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);
#ifdef _DEBUG
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv6, 547));
#endif		
	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	err = connNET1.Start(iConnPrefs); // see test script for IAP used
	TESTEL(err == KErrNone, err);

	INFO_PRINTF1(_L("Start completed ok.."));

	TRequestStatus status;
	
	INFO_PRINTF1(_L("Calling ioctl.."));

	// Request the first domain available
	TDNSServerAddrBuf serverAddress;
	serverAddress().index = 0;	
	connNET1.Ioctl(KCOLConfiguration, KConnGetSipServerAddr, status, &serverAddress);
	User::WaitForRequest(status);
	
	if(status.Int() != KErrNone)
		{
		INFO_PRINTF2(_L("DNS Server address option ioctl failed with code %d.."), status.Int());
		TESTEL(status.Int() == KErrNone, status.Int());
		}
	
	THostName addr;
	serverAddress().addres.Output(addr);
	
	// Verify the retrieved address is correct
	TPtrC addressToCompare;
	
	if(UsingIPv6L())
		{
		//Fetch the expected value from the config file
		if(!GetStringFromConfig(ConfigSection(), _L("ExpectDNSServerAddress1"), addressToCompare))
			{
			INFO_PRINTF1(_L("Unable to read expected DNS server address from the config.Leaving with KErrNotFound"));
			User::Leave(KErrNotFound);
			}
	   	
		if(addr.CompareF(addressToCompare))
			{
			INFO_PRINTF2(_L("DNS Server address invalid test pass %S!"), &addressToCompare);
			SetTestStepResult(EPass);
			}
		else
			{
			INFO_PRINTF2(_L("DNS Server address invalid test fail %S!"), &addressToCompare);
			TESTEL(0, KErrGeneral);
			SetTestStepResult(EFail);
			}
		}
	CleanupStack::Pop(&connNET1);
    err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::Pop(&eSock);
    eSock.Close();

	return TestStepResult();	
	}

TVerdict CDhcpTestStep2_DNSServerList_Test4::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Test case description : Retrieve DNS Server address from the DNS server list
* option by passing sip server address structure.
*/
	{
	SetTestStepResult(EFail);
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);
#ifdef _DEBUG
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv6, 547));
#endif		
	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	err = connNET1.Start(iConnPrefs); // see test script for IAP used
	TESTEL(err == KErrNone, err);

	INFO_PRINTF1(_L("Start completed ok.."));
	
	TRequestStatus status;
	
	INFO_PRINTF1(_L("Calling ioctl.."));

	// Request the first domain available
	TSipServerAddrBuf sipServerAddr;	
	sipServerAddr().index = 0;
	connNET1.Ioctl(KCOLConfiguration, KConnGetDNSServerList, status, &sipServerAddr);
	User::WaitForRequest(status);

	if(status.Int() != KErrNone)
		{
		INFO_PRINTF2(_L("DNS Server address option ioctl failed with code %d.."), status.Int());
		TESTEL(status.Int() == KErrNone, status.Int());
		}

	THostName addrs;
	sipServerAddr().address.Output(addrs);
	
	// Verify the retrieved address is correct
	TPtrC addressToCompare;
	
	if(UsingIPv6L())
		{
		//Fetch the expected value from the config file
		if(!GetStringFromConfig(ConfigSection(), _L("ExpectDNSServerAddress1"), addressToCompare))
			{
			INFO_PRINTF1(_L("Unable to read expected DNS server address from the config.Leaving with KErrNotFound"));
			User::Leave(KErrNotFound);
			}
	   	
		if(addrs.CompareF(addressToCompare))
			{
			INFO_PRINTF2(_L("DNS Server address invalid test fail %S!"), &addressToCompare);
			TESTEL(0, KErrGeneral);
			SetTestStepResult(EFail);
			}
		else
			{
			INFO_PRINTF2(_L("DNS Server address invalid test pass %S!"), &addressToCompare);
			SetTestStepResult(EPass);
			}
		}
	
	CleanupStack::Pop(&connNET1);
    err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::Pop(&eSock);
    eSock.Close();

	return TestStepResult();
	}

TVerdict CDhcpTestStep2_DNSServerList_Test5::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Test case description : Retrieve DNS Server address from the DNS server list
* option by passing inalid index.
*/
	{
	SetTestStepResult(EFail);
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);
#ifdef _DEBUG
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv6, 547));
#endif		
	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	err = connNET1.Start(iConnPrefs); // see test script for IAP used
	TESTEL(err == KErrNone, err);

	INFO_PRINTF1(_L("Start completed ok.."));
	DHCPDebug::State state;
	GetDebugHandleL(connNET1);
	 LOG_STATEL;

	ImmediateCompletionTestL(connNET1);
	    
	TRequestStatus status;
	
	INFO_PRINTF1(_L("Calling ioctl.."));

	// Request the first domain available
	TSipServerAddrBuf sipServerAddr;	
	sipServerAddr().index = -1;
	connNET1.Ioctl(KCOLConfiguration, KConnGetDNSServerList, status, &sipServerAddr);
	User::WaitForRequest(status);

	if(status.Int() == KErrNotFound)
		{
		INFO_PRINTF2(_L("DNS Server address option negative test passed with code %d.."), status.Int());
		SetTestStepResult(EPass);
		}
	else
		{
		INFO_PRINTF2(_L("DNS Server address option negative test failed with code %d.."), status.Int());
		SetTestStepResult(EFail);
		}
	
	LOG_STATEL;
	CleanupStack::Pop(&connNET1);
    err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::Pop(&eSock);
    eSock.Close();

	return TestStepResult();		
	}

TVerdict CDhcpTestStep2_MultipleParams_Test1::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Test case description : Fetch the Multiple options data from the server using KConnDhcpGetMultipleParams
* variable
*/
	{
	SetTestStepResult(EFail);
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);

#ifdef _DEBUG
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv6, 547));
#endif
	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	// see test script for IAP used
	err = connNET1.Start(iConnPrefs); 
	TESTEL(err == KErrNone, err);
	
	TRequestStatus stat;
	TBuf8<500> multipleOptionsBuf;
    TDhcp6RawOptionMultipleDataPckg multipleOptionsPkg(multipleOptionsBuf);
      
	multipleOptionsPkg.AddRawOptionCodeL(KV6NispDomainName);
	multipleOptionsPkg.AddRawOptionCodeL(KV6NisDomainName);
	INFO_PRINTF1(_L("AddRawOptionCodeL pass"));           
			
	connNET1.Ioctl(KCOLConfiguration, KConnDhcpGetMultipleParams, stat, &multipleOptionsBuf);
	User::WaitForRequest(stat);
	INFO_PRINTF2(_L("Ioctl call stat = %d"),stat);           
	TESTEL(stat.Int() == KErrNone, stat.Int());
	
	TPtrC8 nispdomainnamePtr1;
   	err = multipleOptionsPkg.GetRawParameterData(KV6NispDomainName,nispdomainnamePtr1);
   	TESTEL(err == KErrNone, err);
      	   	
	TPtrC expectedValPtr;
   	
   	if(!GetStringFromConfig(ConfigSection(), _L("ExpectedNispDomainName"), expectedValPtr))
   		{
   		INFO_PRINTF2(_L("Unable to read expected ExpectedNispDomainName from the config.Leaving with KErrNotFound expected value = %s"),expectedValPtr);			
   		User::Leave(KErrNotFound);
   		}
   	else
   		{
   		INFO_PRINTF1(_L("Able to read expected ExpectedNispDomainName from the config file"));
   		SetTestStepResult(EPass);
   		}
   
	HBufC8* expectedVal = CnvUtfConverter::ConvertFromUnicodeToUtf8L(expectedValPtr);
	CleanupStack::PushL(expectedVal);
		
	if (nispdomainnamePtr1.FindF(*expectedVal)!=KErrNotFound)
   		 {
   		 SetTestStepResult(EPass);
   		 }
	else
		{
		SetTestStepResult(EFail);
		}

	CleanupStack::PopAndDestroy(expectedVal);

// First option DONE //   	
 
	TPtrC8 nisdomainnamePtr2;
	err = multipleOptionsPkg.GetRawParameterData(KV6NisDomainName,nisdomainnamePtr2);
	TESTEL(err == KErrNone, err);
  		
   if(!GetStringFromConfig(ConfigSection(), _L("ExpectedNisDomainName"), expectedValPtr))
   		{
   		INFO_PRINTF2(_L("Unable to read expected ExpectedNisDomainName from the config.Leaving with KErrNotFound expected value = %s"),expectedValPtr);			
   		User::Leave(KErrNotFound);
   		}
    else
   	   	{
   		INFO_PRINTF1(_L("Able to read expected ExpectedNisDomainName from the config."));
   	  	SetTestStepResult(EPass);
   	  	}
   	   		
	HBufC8* expectedVal2 = CnvUtfConverter::ConvertFromUnicodeToUtf8L(expectedValPtr);
	CleanupStack::PushL(expectedVal2);

	if(nisdomainnamePtr2.FindF(*expectedVal2)!=KErrNotFound)
		{
		SetTestStepResult(EPass);
		}
	else
		{
		SetTestStepResult(EFail);
		}

	CleanupStack::PopAndDestroy(expectedVal2);
   		
 //Second option //
   
	CleanupStack::Pop(&connNET1);
    err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::Pop(&eSock);
    eSock.Close();

	return TestStepResult();
	}

TVerdict CDhcpTestStep2_MultipleParams_Test2::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Test case description : Try to retireve the option from the server which is not 
* added (using the option 6(ORO))
*/
	{
	SetTestStepResult(EFail);
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);

#ifdef _DEBUG
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv6, 547));
#endif
	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	// see test script for IAP used
	err = connNET1.Start(iConnPrefs); 
	TESTEL(err == KErrNone, err);
	
	DHCPDebug::State state;
	GetDebugHandleL(connNET1);
	LOG_STATEL;

	ImmediateCompletionTestL(connNET1);
	
	TRequestStatus stat;
	TBuf8<500> multipleOptionsBuf;
    TDhcp6RawOptionMultipleDataPckg multipleOptionsPkg(multipleOptionsBuf);
      
	multipleOptionsPkg.AddRawOptionCodeL(KV6NispDomainName);
			
	connNET1.Ioctl(KCOLConfiguration, KConnDhcpGetMultipleParams, stat, &multipleOptionsBuf);
	User::WaitForRequest(stat);
	TESTEL(stat.Int() == KErrNone, stat.Int());
	
	TPtrC8 nispdomainnamePtr1;
   	err = multipleOptionsPkg.GetRawParameterData(KV6NisDomainName,nispdomainnamePtr1);
    if(err==KErrNotFound)
       	{
       	INFO_PRINTF1(_L("KConnDhcpGetMultipleParams negative test pass(test4)"));
       	SetTestStepResult(EPass);
       	}
    
     else
       	{
       	INFO_PRINTF1(_L("KConnDhcpGetMultipleParams negative test pass(test4)"));
       	SetTestStepResult(EFail);
       	}
    
    LOG_STATEL;
	CleanupStack::Pop(&connNET1);
    err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::Pop(&eSock);
    eSock.Close();

	return TestStepResult();
	}

TVerdict CDhcpTestStep2_MultipleParams_Test3::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Test case description : Retrieve the option data from the server and compare with unexpected data and see whether 
* the test case is failing (using the option 6(ORO))
*/
	{
	SetTestStepResult(EFail);
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);

#ifdef _DEBUG
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv6, 547));
#endif
	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	// see test script for IAP used
	err = connNET1.Start(iConnPrefs); 
	TESTEL(err == KErrNone, err);
		
	TRequestStatus stat;
	TBuf8<500> multipleOptionsBuf;
    TDhcp6RawOptionMultipleDataPckg multipleOptionsPkg(multipleOptionsBuf);
      
	multipleOptionsPkg.AddRawOptionCodeL(KV6NispDomainName);
			
	connNET1.Ioctl(KCOLConfiguration, KConnDhcpGetMultipleParams, stat, &multipleOptionsBuf);
	User::WaitForRequest(stat);
	INFO_PRINTF2(_L("Ioctl call stat = %d"),stat);          
	TESTEL(stat.Int() == KErrNone, stat.Int());
	
	TPtrC8 nispdomainnamePtr1;
   	err = multipleOptionsPkg.GetRawParameterData(KV6NispDomainName,nispdomainnamePtr1);
    TESTEL(err == KErrNone, err);
   
    TPtrC expectedValPtr;
   	
   	if(!GetStringFromConfig(ConfigSection(), _L("UnExpectedNispDomainName"), expectedValPtr))
   		{
   		INFO_PRINTF2(_L("Unable to read expected UnExpectedNispDomainName from the config.Leaving with KErrNotFound expected value = %s"),expectedValPtr);			
   		User::Leave(KErrNotFound);
   		}
   	else
   		{
   		INFO_PRINTF1(_L("Able to read expected UnExpectedNispDomainName from the config file"));
   		SetTestStepResult(EPass);
   		}
   
	HBufC8* expectedVal = CnvUtfConverter::ConvertFromUnicodeToUtf8L(expectedValPtr);
	CleanupStack::PushL(expectedVal);
		
	if (nispdomainnamePtr1.FindF(*expectedVal)!=KErrNotFound)
   		 {
   		 SetTestStepResult(EFail);
   		 }
	else
		{
		SetTestStepResult(EPass);
		}

	CleanupStack::PopAndDestroy(expectedVal);
 
	CleanupStack::Pop(&connNET1);
    err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::Pop(&eSock);
    eSock.Close();

	return TestStepResult();
	}

TVerdict CDhcpTestStep2_MultipleParams_Test4::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Test case description : Retrieve the option data from the server without adding the 
* option before Ioctl (using the option 6(ORO))
*/
	{
	SetTestStepResult(EFail);
		
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);

#ifdef _DEBUG
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv6, 547));
#endif
	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	// see test script for IAP used
	err = connNET1.Start(iConnPrefs); 
	TESTEL(err == KErrNone, err);
	
	DHCPDebug::State state;
	GetDebugHandleL(connNET1);
	LOG_STATEL;

	ImmediateCompletionTestL(connNET1);
	
	TRequestStatus stat;
	TBuf8<50> multipleOptionsBuf;
    TDhcp6RawOptionMultipleDataPckg multipleOptionsPkg(multipleOptionsBuf);
  	
    INFO_PRINTF1(_L("Calling Ioctl..."));
    
	connNET1.Ioctl(KCOLConfiguration, KConnDhcpGetMultipleParams, stat, &multipleOptionsBuf);
	User::WaitForRequest(stat);
	INFO_PRINTF2(_L("Ioctl call stat = %d"),stat);  
	if(stat==KErrNone)
    	{
    	INFO_PRINTF1(_L("KConnDhcpGetMultipleParams negative test fail(test4)"));
    	SetTestStepResult(EFail);
    	}
    else
    	{
    	INFO_PRINTF1(_L("KConnDhcpGetMultipleParams negative test pass(test4)"));
    	SetTestStepResult(EPass);
    	}
	
	LOG_STATEL;
	CleanupStack::Pop(&connNET1);
    err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::Pop(&eSock);
    eSock.Close();

	return TestStepResult();
	}

TVerdict CDhcpTestStep2_MultipleParams_Test5::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Test case description : Try to retireve the option data from the server by passing the buffer 
* of smaller size(using the option 6(ORO))
*/
	{
	SetTestStepResult(EFail);
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);

#ifdef _DEBUG
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv6, 547));
#endif
	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	// see test script for IAP used
	err = connNET1.Start(iConnPrefs); 
	TESTEL(err == KErrNone, err);

	DHCPDebug::State state;
    GetDebugHandleL(connNET1);
    LOG_STATEL;

    ImmediateCompletionTestL(connNET1);

	TRequestStatus stat;
	TBuf8<10> multipleOptionsBuf;
    TDhcp6RawOptionMultipleDataPckg multipleOptionsPkg(multipleOptionsBuf);
      
	multipleOptionsPkg.AddRawOptionCodeL(KV6NispDomainName);
			
	connNET1.Ioctl(KCOLConfiguration, KConnDhcpGetMultipleParams, stat, &multipleOptionsBuf);
	User::WaitForRequest(stat);
	INFO_PRINTF2(_L("Ioctl call stat = %d"),stat);  
	TESTEL(stat.Int() == KErrOverflow, stat.Int());
	    
   	if(stat==KErrOverflow)
    	{
    	INFO_PRINTF1(_L("KConnDhcpGetMultipleParams negative test Pass(test5)"));
    	SetTestStepResult(EPass);
    	}
    else
    	{
    	INFO_PRINTF1(_L("KConnDhcpGetMultipleParams negative test fail(test5)"));
    	SetTestStepResult(EFail);
    	}
  
    LOG_STATEL;

	CleanupStack::Pop(&connNET1);
    err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::Pop(&eSock);
    eSock.Close();

	return TestStepResult();
	}

TVerdict CDhcpTestStep2_GetRaw_Test1::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Test case Description : Try to retrieve the option 30 from server using GetRawOption 
* which internall calls Inform request. 
*/
	{
	SetTestStepResult(EFail);

	INFO_PRINTF1(_L("Connecting to socket server.."));

	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);

#ifdef _DEBUG
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv6, 547));
#endif
	
	INFO_PRINTF1(_L("Opening connection.."));

	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	err = connNET1.Start(iConnPrefs); // see test script for IAP used
	TESTEL(err == KErrNone, err);

	INFO_PRINTF1(_L("Start completed ok.."));

    TBuf8<100> buf;
	buf.FillZ(100);
    const TUint KOptCode = 30; 

	INFO_PRINTF1(_L("Calling ioctl.."));

    err = DhcpGetRawOptionDataL(connNET1, buf, KOptCode);
    TESTEL(err == KErrNone, err);
	
	INFO_PRINTF1(_L("Ioctl completed ok.."));

	TDhcp6RawOptionDataPckg pckg(buf);
	TPtr8 buf2(pckg.Buf());

	INFO_PRINTF2(_L("Buffer received ok. Length %d"),buf2.Length());
	
	INFO_PRINTF1(_L("Stopping IAP.."));
	
	TPtrC expectedValPtr;
	   	
  	if(!GetStringFromConfig(ConfigSection(), _L("ExpectedNispDomainName"), expectedValPtr))
   		{
   		INFO_PRINTF2(_L("Unable to read expected UnExpectedNispDomainName from the config.Leaving with KErrNotFound expected value = %s"),expectedValPtr);			
   		User::Leave(KErrNotFound);
   		}
   	else
   		{
   		INFO_PRINTF1(_L("Able to read expected ExpectedNispDomainName from the config file"));
   		SetTestStepResult(EPass);
   		}
	   
	HBufC8* expectedVal = CnvUtfConverter::ConvertFromUnicodeToUtf8L(expectedValPtr);
	CleanupStack::PushL(expectedVal);
	
	if(buf2.FindF(*expectedVal)!=KErrNotFound)
		{
		SetTestStepResult(EPass);
		}
	else
		{
		SetTestStepResult(EFail);
		}
	
	CleanupStack::PopAndDestroy(expectedVal);	
	
	CleanupStack::Pop(&connNET1);
    err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::Pop(&eSock);
    eSock.Close();

	return TestStepResult();
	}

TVerdict CDhcpTestStep2_GetRaw_Test2::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Test case Description : Try to retireve the option data from the server by passing the buffer 
* of smaller size by using GetRawOption
*/
	{
	SetTestStepResult(EFail);

	INFO_PRINTF1(_L("Connecting to socket server.."));

	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);

#ifdef _DEBUG
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv6, 547));
#endif
	
	INFO_PRINTF1(_L("Opening connection.."));

	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	err = connNET1.Start(iConnPrefs); // see test script for IAP used
	TESTEL(err == KErrNone, err);

	INFO_PRINTF1(_L("Start completed ok.."));

	DHCPDebug::State state;
	GetDebugHandleL(connNET1);
	LOG_STATEL;
	
	ImmediateCompletionTestL(connNET1);
	
    TBuf8<10> buf;
	buf.FillZ(10);
    const TUint KOptCode = 30; 

	INFO_PRINTF1(_L("Calling ioctl.."));

    err = DhcpGetRawOptionDataL(connNET1, buf, KOptCode);
    TESTEL(err == KErrOverflow, err);
    
	INFO_PRINTF2(_L("CDhcpTestStep2_GetRaw_Test2 invalid test pass = %d"),err);			
   	
	
	INFO_PRINTF1(_L("Ioctl completed ok.."));
	LOG_STATEL;

	TDhcp6RawOptionDataPckg pckg(buf);
	TPtr8 buf2(pckg.Buf());
		
	INFO_PRINTF2(_L("Buffer received ok. Length %d"),buf2.Length());

	INFO_PRINTF1(_L("Stopping IAP.."));
	
	LOG_STATEL;
	CleanupStack::Pop(&connNET1);
    err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::Pop(&eSock);
    eSock.Close();
    
    SetTestStepResult(EPass);
    
	return TestStepResult();
	}
#endif //SYMBIAN_TCPIPDHCP_UPDATE

