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
// Test cases to harness Symbian DHCP server
// 
//

/**
 @file te_dhcpserverteststep.cpp
*/
#include "te_dhcpserverteststep.h"
#include "../../include/DHCPStatesDebug.h"

#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
#include <networking/dhcpconfig.h>
#endif // SYMBIAN_NETWORKING_DHCP_MSG_HEADERS

#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <nifman_internal.h>
#endif

TVerdict CDhcpTestStepServ_1::doTestStepL()
/**
* @return - EPass if successfully launches server, EFail otherwise
* Launches server on a custom port with Hardware address params set to serve any client
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
    User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv4, 4836));
#endif
    
	err = connNET1.Start(iConnPrefs); // see test script for IAP used
	TESTEL(err == KErrNone, err);

	TRequestStatus status;

    const TInt KMacAddrLength = 6;
    TBuf8<KMacAddrLength> address;
    GetProvisioningMacL(ConfigSection(), _L("AnyMac"), address);
    connNET1.Ioctl(KCOLConfiguration, KConnDhcpSetHwAddressParams, status, &address);

  User::WaitForRequest(status);
  TESTEL(status == KErrNone, status.Int());
    
	INFO_PRINTF1(_L("DHCP server started.."));
	User::After(630 * 1000000);
	INFO_PRINTF1(_L("DHCP server STOPPED.."));
	

	err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::PopAndDestroy(&connNET1);
	CleanupStack::PopAndDestroy(&eSock);
	
	SetTestStepResult(EPass);
	return TestStepResult();

	}

TVerdict CDhcpTestStepServ_2::doTestStepL()
/**
* @return - EPass if successfully launches server, EFail otherwise
* Launches server on a custom port with Hardware address params set 
* to serve one client with a particular MAC address and
* Lease time of the IP address set to 0
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
    User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyDefaultLeaseTime, 0));
    User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv4, 4836));
#endif
    
	err = connNET1.Start(iConnPrefs); // see test script for IAP used
	TESTEL(err == KErrNone, err);

	TRequestStatus status;

    TBuf8<13> bufDes;
    bufDes.FillZ(13);
    TDhcp4RawOptionDataPckg pckg(bufDes);
    
    TUint opcode = 54; //OpCode Translates to EDHCPServerID for the server
    pckg.SetOpCode(opcode);
        
    const TUint dhcpOptName = KConnSetDhcpRawOptionData;    
    
    // Set raw option data either for ip4
    connNET1.Ioctl(KCOLConfiguration, dhcpOptName, status, &bufDes);

    User::WaitForRequest(status);
    TESTEL(status == KErrNone, status.Int());
    
    //Using MAC address of dhcp client that requests IP from this server (04215E312156)
    const TInt KMacAddrLength = 6;
    TBuf8<KMacAddrLength> address;
    GetProvisioningMacL(ConfigSection(), _L("ProvisionMac"), address);
    connNET1.Ioctl(KCOLConfiguration, KConnDhcpSetHwAddressParams, status, &address);

    User::WaitForRequest(status);
    TESTEL(status == KErrNone, status.Int());
    
	INFO_PRINTF1(_L("DHCP server started.."));
	User::After(650 * 1000000);
	INFO_PRINTF1(_L("DHCP server STOPPED.."));
	

	err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::PopAndDestroy(&connNET1);
	CleanupStack::PopAndDestroy(&eSock);
	
	SetTestStepResult(EPass);
	return TestStepResult();

	}

TVerdict CDhcpTestStepServ_3::doTestStepL()
/**
* @return - EPass if successfully launches server, EFail otherwise
* Launches server on a custom port with Hardware address params set 
* to serve any client and lease time of the IP address set to a -ve value
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
    User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyDefaultLeaseTime, -20));
    User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv4, 4836));
#endif
    
	err = connNET1.Start(iConnPrefs); // see test script for IAP used
	TESTEL(err == KErrNone, err);

	TRequestStatus status;

    TBuf8<13> bufDes;
    bufDes.FillZ(13);
    TDhcp4RawOptionDataPckg pckg(bufDes);
    
    TUint opcode = 54; //OpCode Translates to EDHCPServerID for the server
    pckg.SetOpCode(opcode);
        
    const TUint dhcpOptName = KConnSetDhcpRawOptionData;    
    
    // Set raw option data either for ip4
    connNET1.Ioctl(KCOLConfiguration, dhcpOptName, status, &bufDes);

    User::WaitForRequest(status);
    TESTEL(status == KErrNone, status.Int());
    
    const TInt KMacAddrLength = 6;
    TBuf8<KMacAddrLength> address;
    GetProvisioningMacL(ConfigSection(), _L("AnyMac"), address);
    connNET1.Ioctl(KCOLConfiguration, KConnDhcpSetHwAddressParams, status, &address);

    User::WaitForRequest(status);
    TESTEL(status == KErrNone, status.Int());
    
	INFO_PRINTF1(_L("DHCP server started.."));
	User::After(650 * 1000000);
	INFO_PRINTF1(_L("DHCP server STOPPED.."));
	

	err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::PopAndDestroy(&connNET1);
	CleanupStack::PopAndDestroy(&eSock);
	
	SetTestStepResult(EPass);
	return TestStepResult();

	}

TVerdict CDhcpTestStepServ_4::doTestStepL()
/**
* @return - EPass if successfully launches server, EFail otherwise
* Launches server on a custom port with Hardware address params set 
* to serve any client initially, then using provisioning RESET to clear
* the provisioning set earlier and set the provisioning to a single client
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
    User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv4, 4836));
#endif
    
    err = connNET1.Start(iConnPrefs); // see test script for IAP used
    TESTEL(err == KErrNone, err);

    TRequestStatus status;

    const TInt KMacAddrLength = 6;
    TBuf8<KMacAddrLength> address;
    GetProvisioningMacL(ConfigSection(), _L("AnyMac"), address);
    connNET1.Ioctl(KCOLConfiguration, KConnDhcpSetHwAddressParams, status, &address);

    User::WaitForRequest(status);
    TESTEL(status == KErrNone, status.Int());
    
    INFO_PRINTF1(_L("DHCP server started.."));
    User::After(300 * 1000000);
    
    TBuf8<KMacAddrLength> address1;
    GetProvisioningMacL(ConfigSection(), _L("ResetMac"), address1);
    connNET1.Ioctl(KCOLConfiguration, KConnDhcpSetHwAddressParams, status, &address1);

    User::WaitForRequest(status);
    TESTEL(status == KErrNone, status.Int());
    
    INFO_PRINTF1(_L("DHCP server Provisioning Reset.."));
    
    User::After(50 * 1000000);
    
    TBuf8<KMacAddrLength> address2;
    GetProvisioningMacL(ConfigSection(), _L("InvalidMac1"), address2);
    connNET1.Ioctl(KCOLConfiguration, KConnDhcpSetHwAddressParams, status, &address2);

    User::WaitForRequest(status);
    TESTEL(status == KErrNone, status.Int());
    INFO_PRINTF1(_L("DHCP server Provisioning Set.."));
    User::After(300 * 1000000);
    INFO_PRINTF1(_L("DHCP server STOPPED.."));
    

    err = connNET1.Stop();
    TESTEL(err == KErrNone, err);

    CleanupStack::PopAndDestroy(&connNET1);
    CleanupStack::PopAndDestroy(&eSock);
    
    SetTestStepResult(EPass);
    return TestStepResult();

    }

TVerdict CDhcpTestStepServ_5::doTestStepL()
/**
* @return - Negetive testing. Expected to return EFail
* Launches server on a custom port with Hardware address params set 
* to serve one client with hw address length beyond the standard length
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
     User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv4, 4836));
#endif
    
    err = connNET1.Start(iConnPrefs); // see test script for IAP used
    TESTEL(err == KErrNone, err);

    TRequestStatus status;

    const TInt KMacAddrLength = 7;
    TBuf8<KMacAddrLength> address;
    GetProvisioningMacL(ConfigSection(), _L("InvalidMac2"), address);
	connNET1.Ioctl(KCOLConfiguration, KConnDhcpSetHwAddressParams, status, &address);
	
    User::WaitForRequest(status);

    if (status != KErrNone)
    	{
    	SetTestStepError(status.Int());
    	}
    else
    	{
	    INFO_PRINTF1(_L("DHCP server started.."));
	    User::After(40 * 1000000);
	    INFO_PRINTF1(_L("DHCP server STOPPED.."));
	    
	
	    err = connNET1.Stop();
	    TESTEL(err == KErrNone, err);
	
	  	}
    CleanupStack::PopAndDestroy(&connNET1);
    CleanupStack::PopAndDestroy(&eSock);

    SetTestStepResult(EPass);
    return TestStepResult();

    }

TVerdict CDhcpTestStepServ_6::doTestStepL()
/**
* @return - EPass if successfully launches server, EFail otherwise
* Launches client on a custom port where the DHCP server is expected
* to lease out IP with -ve lease time
*/
	{
	SetTestStepResult(EFail);	
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);
#ifdef _DEBUG
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv4, 4836));
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv6, 547));
#endif		
	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	TRequestStatus stat1;
	connNET1.Start(iConnPrefs,stat1);// see test script for IAP used
	
	err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::PopAndDestroy(&connNET1); 
	CleanupStack::PopAndDestroy(&eSock); 
	
	SetTestStepResult(EPass);
	return TestStepResult();
	}
