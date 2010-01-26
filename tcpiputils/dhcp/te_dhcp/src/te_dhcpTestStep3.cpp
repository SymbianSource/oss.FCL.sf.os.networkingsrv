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
// (test doc 4.3 Renew and Release)
// Renew and Release (test doc 4.3)
// 
//

/**
 @file te_dhcpTestStep3.cpp 
*/
#include "te_dhcpTestStep3.h"
#include <test/testexecutelog.h>
#include <commdbconnpref.h>
#include <in_sock.h>
#include <nifman.h>
#include <comms-infras/es_config.h>
#include "../../include/DHCP_Std.h"

TVerdict CDhcpTestStep3_1::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Renew Lease of a connection that is down (test spec 4.3.1)
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
	err = connNET1.Stop();
	TESTEL(err == KErrNone, err);
	
	//renew
	TRequestStatus stat;
	connNET1.Ioctl(KCOLConfiguration, KConnAddrRenew, stat);
	User::WaitForRequest(stat);
	TESTEL(stat == KErrNotReady, stat.Int());
	
	CleanupStack::PopAndDestroy(&connNET1);
	CleanupStack::PopAndDestroy(&eSock);
	
	SetTestStepResult(EPass);
	return TestStepResult();
	}
	
TVerdict CDhcpTestStep3_2::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Test lease time reporting (test spec 4.3.2), read lease,
* let the lease rundown then query again and compare
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
	
	TConnectionLeaseInfoBuf leaseTime1;
	leaseTime1().iAddressFamily = IpAddressFamilyL();
	TRequestStatus stat;
	connNET1.Ioctl(KCOLConfiguration, KConnGetAddrLeaseTimeRemain, stat, &leaseTime1);
	User::WaitForRequest(stat);
	TESTEL(stat == KErrNone, stat.Int());
	
	// wait for 5 seconds
	User::After(5 * 1000000);

	TConnectionLeaseInfoBuf leaseTime2;
	leaseTime2().iAddressFamily = IpAddressFamilyL();
	connNET1.Ioctl(KCOLConfiguration, KConnGetAddrLeaseTimeRemain, stat, &leaseTime2);
	User::WaitForRequest(stat);
	TESTEL(stat == KErrNone, stat.Int());
	TESTEL(leaseTime2().iSecondsRemaining < leaseTime1().iSecondsRemaining, KErrArgument);
	
	err = connNET1.Stop();
	TESTEL(err == KErrNone, err);
	
	CleanupStack::PopAndDestroy(&connNET1);
	CleanupStack::PopAndDestroy(&eSock);

	SetTestStepResult(EPass);
	return TestStepResult();
	}


TVerdict CDhcpTestStep3_3::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Release Current Lease (test spec 4.3.3)
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
	
	connNET1.Ioctl(KCOLConfiguration, KConnAddrRelease, stat);
	User::WaitForRequest(stat);
	TESTEL(stat == KErrNone, stat.Int());
	
	// we have got an address back from the daemon...but it has since
	// been released....we shouldn;t be able to bind to it
	RSocket socket;
	err = socket.Open(eSock, KAfInet, KSockDatagram, KProtocolInetUdp, connNET1);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(socket);

	TSockAddr& addr = address().iAddr;
	err = socket.Bind(addr);
	TESTEL(err == KErrNotFound, err);

	err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::PopAndDestroy(&socket);
	CleanupStack::PopAndDestroy(&connNET1);
	CleanupStack::PopAndDestroy(&eSock);	
	SetTestStepResult(EPass);
	return TestStepResult();
	}
	

TVerdict CDhcpTestStep3_4::doTestStepL()
/**
*Test starts a new connection. The original DHCP server is stopped and
*reconfigured to pretend as a different DHCP server. The configuration can be changed 
*to either return different range of IP address or a different DHCP server address. 
*This test currently can only be run manually
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual* 
*/
	{	
	SetTestStepResult(EFail);	
	
	TBool isIpv6 = UsingIPv6L();
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);
#ifdef _DEBUG
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv4, 124));
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv6, 564));
#endif		
	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	TRequestStatus stat1;
	connNET1.Start(iConnPrefs,stat1); // see test script for IAP used
	
	TRequestStatus stat2;
	TConnectionAddrBuf address;
	address().iAddressFamily = IpAddressFamilyL();
	User::WaitForRequest(stat1);
	TESTEL(stat1.Int() == KErrNone, stat1.Int());
	
	// dhcp test bed uses dhcp relay for this test, hence conn address is extracted
		connNET1.Ioctl(KCOLConfiguration, KConnGetCurrentAddr, stat2, &address);
	User::WaitForRequest(stat2);
	TESTEL(stat2.Int() == KErrNone, stat2.Int());

	
	//Now change the dest port to point to differnt server running on port 67 by changing publishing the property again
#ifdef _DEBUG
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv4, 67));
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv6, 547));
#endif		

	User::After(2 * 60 * 1000000);	// N.B. the lease from first dhcp server expires after a min.
	
	
	TInt secValue(1);
	TPckg<TInt> val(secValue);		
			
	TRequestStatus stat;
	connNET1.Ioctl(KCOLConfiguration, KConnAddrRenew, stat,&val);
	User::WaitForRequest(stat);
	TESTEL(stat.Int() == KErrNone, stat.Int());
	
	
	
	//Get the server address
	TRequestStatus stat3;
	TConnectionAddrBuf address2;
	address2().iAddressFamily = IpAddressFamilyL();
	
	// dhcp test bed uses dhcp relay for this test, hence conn address is extracted
	connNET1.Ioctl(KCOLConfiguration, KConnGetCurrentAddr, stat3, &address2);

	User::WaitForRequest(stat3);
	TESTEL(stat3.Int() == KErrNone, stat3.Int());
		
	
	TInetAddr* properAddr = (TInetAddr*)&(address().iAddr);	
	
	if(isIpv6)
		{
		TIp6Addr addr1 = properAddr->Ip6Address();
		properAddr = (TInetAddr*)&(address2().iAddr);
		TIp6Addr addr2 = properAddr->Ip6Address();
		TBool brr = addr2.IsEqual(addr1);
		TESTEL(!brr, KErrAlreadyExists);
		}
	else
		{
		TUint32 addr1 = properAddr->Address();
		properAddr = (TInetAddr*)&(address2().iAddr);
		TUint32 addr2 = properAddr->Address();
		TESTEL(addr1 != addr2, KErrAlreadyExists);
		}	
			
	err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::PopAndDestroy(&connNET1); 
	CleanupStack::PopAndDestroy(&eSock); 
	
	SetTestStepResult(EPass);
	return TestStepResult();
	}

