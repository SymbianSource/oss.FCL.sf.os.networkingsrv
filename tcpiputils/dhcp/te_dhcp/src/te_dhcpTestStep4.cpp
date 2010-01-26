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
// (test spec test cases 4.4 Dynamic Interactions)
// Dynamic Server interaction (tset doc 4.4 )
// 
//

/**
 @file te_dhcpTestStep4.cpp
*/
#include "te_dhcpTestStep4.h"
#include <test/testexecutelog.h>
#include <commdbconnpref.h>
#include <in_sock.h>
#include <nifman.h>
#include <comms-infras/es_config.h>
#include "../../include/DHCP_Std.h"

TVerdict CDhcpTestStep4_1::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Opens a connection and issues two subsequent requests (test spec 4.4.3)
* Can only have one outstanding request at a time, so the second request
* will be errored with KErrInUse...
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
	
	TRequestStatus stat1;
	TConnectionAddrBuf address;
	address().iAddressFamily = IpAddressFamilyL();
	connNET1.Ioctl(KCOLConfiguration, KConnGetCurrentAddr, stat1, &address);
	TRequestStatus stat2;
	connNET1.Ioctl(KCOLConfiguration, KConnGetCurrentAddr, stat2, &address);
	User::WaitForRequest(stat2);
	User::WaitForRequest(stat1);
	TESTEL(stat1 == KErrNone, stat1.Int());
	TESTEL(stat2 == KErrInUse, stat2.Int());

	err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::PopAndDestroy(&connNET1);
	CleanupStack::PopAndDestroy(&eSock);
	
	SetTestStepResult(EPass);
	return TestStepResult();
	}

TVerdict CDhcpTestStep4_2::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Start a connection, issue a request and stop the connection(test spec 4.4.4)
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
	
	TRequestStatus stat1;
	connNET1.Ioctl(KCOLConfiguration, KConnAddrRenew, stat1);
	err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	User::WaitForRequest(stat1);
	TESTEL(stat1 == KErrCancel, stat1.Int());
	
	CleanupStack::PopAndDestroy(&connNET1); 
	CleanupStack::PopAndDestroy(&eSock);
	
	SetTestStepResult(EPass);
	return TestStepResult();
	}

TVerdict CDhcpTestStep4_3::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Cancel request from an ioctl (test doc 4.4.5)
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
	//TConnectionAddrBuf address;
	//address().iAddressFamily = IpAddressFamilyL();
	connNET1.Ioctl(KCOLConfiguration, KConnAddrRenew, stat, NULL);
	connNET1.CancelIoctl();
	User::WaitForRequest(stat);
	TESTEL(stat == KErrCancel, stat.Int());
	
	err = connNET1.Stop();
	TESTEL(err == KErrNone, err);
	
	CleanupStack::PopAndDestroy(&connNET1);
	CleanupStack::PopAndDestroy(&eSock); 
	
	SetTestStepResult(EPass);
	return TestStepResult();
	}
