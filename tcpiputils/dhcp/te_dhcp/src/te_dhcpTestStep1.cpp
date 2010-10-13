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
 @file te_dhcpTestStep1.cpp
*/
#include "te_dhcpTestStep1.h"
#include <test/testexecutelog.h>
#include <commdbconnpref.h>
#include <in_sock.h>
#include <nifman.h>
#include "te_dhcpTestServer.h"
#include <comms-infras/es_config.h>
#include "../../include/DHCP_Std.h"
#include <comms-infras/startprocess.h>
#include "te_TestDaemonClient.h"
#include "DHCPStatesDebug.h"

TVerdict CDhcpTestStep1_1::doTestStepL()
/**
* @return - TVerdict code, only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Connect to DHCP Daemon (test doc 4.1.1)
* Test cannot explicitly connect as we use the
* RConnection API here which will test full
* functionality through the system. This test
* therefore starts an ethernet connection with
* settings in commDB to specify using dhcp configuration
* to provide the connection with an address. If the connection
* starts successfully, then DHCP was used to configure an
* address therefore connection to DHCP by NIFMAN was achieved...
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

	CleanupStack::PopAndDestroy(&connNET1);
	CleanupStack::PopAndDestroy(&eSock);
	
	SetTestStepResult(EPass);
	return TestStepResult();
	}

TVerdict CDhcpTestStep1_2::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Connect to DHCP Daemon - Daemon not found(test doc 4.1.2)
* Daemon name in commDB specifies a non-existent daemon therefore
* connection start will fail as the settings for it are incorrect...
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
	TESTEL(err == KErrNotFound, err);

	CleanupStack::PopAndDestroy(&connNET1);
	CleanupStack::PopAndDestroy(&eSock);
	
	SetTestStepResult(EPass);
	return TestStepResult();
	}

TVerdict CDhcpTestStep1_3::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Get Active IP Address (test doc 4.1.3)
* from a connection that does not use DHCP
* configuration but the static settings in
* commDB. Therefore any request for the address
* from the DHCP daemon will fail.
*/
	{
	SetTestStepResult(EFail);
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	INFO_PRINTF2(_L("eSock.Connect() returned %d"),err);
	CleanupClosePushL(eSock);
#ifdef _DEBUG
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv4, 67));
#endif		
	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	INFO_PRINTF2(_L("connNET1.Open(eSock) returned %d"),err);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	err = connNET1.Start(iConnPrefs); // see test script for IAP used
	INFO_PRINTF2(_L("connNET1.Start(prefs) returned %d"),err);
	TESTEL(err == KErrNone, err);
	
	TRequestStatus stat;
	TConnectionAddrBuf address;
	address().iAddressFamily = IpAddressFamilyL();
	INFO_PRINTF1(_L("Just before connNET1.Ioctl(...)"));
	

	INFO_PRINTF2(_L("Size: %d "), address.Size());
	INFO_PRINTF2(_L("MaxSize: %d "), address.MaxSize());
	INFO_PRINTF2(_L("MaxLength: %d "), address.MaxLength());
	
	connNET1.Ioctl(KCOLConfiguration, KConnGetCurrentAddr, stat, &address);
	User::WaitForRequest(stat);
	INFO_PRINTF2(_L("err = connNET1.Ioctl(...) returned %d"),stat.Int());
	TESTEL(stat == KErrNotSupported, stat.Int());

	err = connNET1.Stop();
	INFO_PRINTF2(_L("connNET1.Stop() returned %d"),err);
	TESTEL(err == KErrNone, err);

	CleanupStack::PopAndDestroy(&connNET1);
	CleanupStack::PopAndDestroy(&eSock);
	
	SetTestStepResult(EPass);
	return TestStepResult();
	}

TVerdict CDhcpTestStep1_4::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Get DHCP Server Address (test case 4.1.4)
* As no DHCP configuration is used for this
* test, the connection cannot be queried for 
* the DHCP Server that assigned any address...
*/
	{
	SetTestStepResult(EFail);
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL( err == KErrNone, err);
	CleanupClosePushL(eSock);
#ifdef _DEBUG
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv4, 67));
#endif		
	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL( err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	err = connNET1.Start(iConnPrefs); // see test script for IAP used
	TESTEL(err == KErrNone, err);
	
	TRequestStatus stat;
	TConnectionAddrBuf address;
	address().iAddressFamily = IpAddressFamilyL();
	connNET1.Ioctl(KCOLConfiguration, KConnGetServerAddr, stat, &address);
	User::WaitForRequest(stat);
	TESTEL( stat == KErrNotSupported, stat.Int());

	err = connNET1.Stop();
	TESTEL(err == KErrNone, err);
	
	CleanupStack::PopAndDestroy(&connNET1);
	CleanupStack::PopAndDestroy(&eSock);
	
	SetTestStepResult(EPass);
	return TestStepResult();
	}

TVerdict CDhcpTestStep1_5::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Get Remaining Lease Time (test case 4.1.5)
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
	
	TConnectionLeaseInfoBuf leaseTime1;
	leaseTime1().iAddressFamily = IpAddressFamilyL();
	TRequestStatus stat;
	connNET1.Ioctl(KCOLConfiguration, KConnGetAddrLeaseTimeRemain, stat, &leaseTime1);
	User::WaitForRequest(stat);
	TESTEL(stat == KErrNotSupported, stat.Int());

	err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::PopAndDestroy(&connNET1);
	CleanupStack::PopAndDestroy(&eSock);
	
	SetTestStepResult(EPass);
	return TestStepResult();
	}


TVerdict CDhcpTestStep1_6::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Renew Current Lease (test case 4.1.6)
*/
	{
	SetTestStepResult(EFail);
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL( err == KErrNone, err);
	CleanupClosePushL(eSock);
#ifdef _DEBUG
	User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv4, 67));
#endif		
	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL( err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	err = connNET1.Start(iConnPrefs); // see test script for IAP used
	TESTEL( err == KErrNone, err);
	
	TRequestStatus stat;
	connNET1.Ioctl(KCOLConfiguration, KConnAddrRenew, stat);
	User::WaitForRequest(stat);
	TESTEL( stat == KErrNotSupported, stat.Int());

	err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::PopAndDestroy(&connNET1);
	CleanupStack::PopAndDestroy(&eSock);
	
	SetTestStepResult(EPass);
	return TestStepResult();
	}

TVerdict CDhcpTestStep1_7::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Release Current Lease (test case 4.1.7)
*/
	{
	SetTestStepResult(EFail);
	
	RSocketServ eSock;
	TInt err = eSock.Connect();
	TESTEL( err == KErrNone, err);
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
	connNET1.Ioctl(KCOLConfiguration, KConnAddrRelease, stat);
	User::WaitForRequest(stat);
	TESTEL(stat == KErrNotSupported, stat.Int());

	err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::PopAndDestroy(&connNET1);
	CleanupStack::PopAndDestroy(&eSock);
	
	SetTestStepResult(EPass);
	return TestStepResult();
	}
	
TVerdict CDhcpTestStep1_8::doTestStepL()
/**
* @return - TVerdict code only ever returns EPass, else leaves with error!
* Override of base class pure virtual
* Check that the TCP/IP link local option is honoured if the DHCP server is unavailable.
*/
	{
    #ifndef __DHCP_HEAP_CHECK_CONTROL
    	INFO_PRINTF1(_L("This test step is disabled for release build."));
        SetTestStepResult(EInconclusive);
	    return TestStepResult();
    #else

	SetTestStepResult(EFail);

	TInt ipv4linklocal = 0;
	
	if( !UsingIPv6L() )
		{
		// Open the .ini file.
		TAutoClose<RFs> fs;
		User::LeaveIfError( fs.iObj.Connect() );
		fs.PushL();
		TAutoClose<RFile> file;
		User::LeaveIfError( file.iObj.Open( fs.iObj, TCPIP_INI_PATH, EFileRead | EFileShareAny ) );
		file.PushL();

		// Read the contents of the file.
		TInt size;
		User::LeaveIfError( file.iObj.Size( size ) );
		TAutoClose<RBuf8> buf;
		buf.iObj.CreateL( size );
		buf.PushL();
		User::LeaveIfError( file.iObj.Read( buf.iObj ) );
		
		// Read the ipv4linklocal value if present.
		TInt valPos = buf.iObj.Find( TCPIP_INI_IPV4LINKLOCAL );
		if( valPos != KErrNotFound )
			{
			ipv4linklocal = buf.iObj[valPos + ( (const TDes8 &)TCPIP_INI_IPV4LINKLOCAL ).Length()] - '0'; // convert to integer by subtracting base of ASCII integer range
			
			// Check that the value is valid.
			if( ( ipv4linklocal < 0 ) || ( ipv4linklocal > 3 ) )
				{
		    	INFO_PRINTF2(_L("Invalid ipv4linklocal option %d in tcpip.ini"), ipv4linklocal);
		        
		        SetTestStepResult(EInconclusive);
			    
			    return TestStepResult();
				}
			}
			
		buf.Pop();
		file.Pop();
		fs.Pop();
		}
		
	// Check to see if the IP address is static.
	bool isStatic = IsIAPAddrStaticL();
	if( isStatic )
		{
		INFO_PRINTF1(_L("A static IP address has been configured."));
		}
	else
		{
		INFO_PRINTF1(_L("A static IP address has not been configured."));
		}
	
	// Start DHCP server.
	RStartProcess processStart;
	TInt err = processStart.Start(KDHCPProcessName);
	TESTEL( err == KErrNone || err ==KErrAlreadyExists, err);

	// Connect to the server. We will use this connection to issue heap debug commands.
	RTestDaemonClient debugSession;
	debugSession.CreateSession();
	CleanupClosePushL(debugSession);

	RSocketServ eSock;
	err = eSock.Connect();
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(eSock);
	
	RConnection connNET1;
	err = connNET1.Open(eSock);
	TESTEL(err == KErrNone, err);
	CleanupClosePushL(connNET1);
	
	RSocket socket;
	CleanupClosePushL(socket);

	TRequestStatus stat;
	TUint curAddrTypesDHCPDiscoverFailed = KCurAddrNone;

	// Check to see if configuration deamon controlled link locals are enabled or
	// the address is static (must not try this test in other circumstances - the
	// interface will not start without a valid initial address).
	if( ( ipv4linklocal == 3 ) || isStatic )
		{
		INFO_PRINTF1(_L("Testing address types after failed DHCP initialisation..."));

		// Set debug flags.
		INFO_PRINTF1(_L("Setting debug option to simulate DHCP discovery failure..."));
	    TDhcpMemDbgParamBuf dhcpMemDbgParamBuf;
		dhcpMemDbgParamBuf() = KDHCP_FailDiscover;
		debugSession.Ioctl(KCOLConfiguration, KDhcpMemDbgIoctl|KDHCP_DbgFlags, stat, &dhcpMemDbgParamBuf);
		User::WaitForRequest(stat);
		TESTEL(stat == KErrNone, stat.Int());

		err = connNET1.Start(iConnPrefs); // see test script for IAP used
		TESTEL(err == KErrNone, err);
		
		User::LeaveIfError( socket.Open( eSock, KAfInet, KSockDatagram, KProtocolInetUdp, connNET1 ) );
		
		// Check to see if a link local is configured after the DHCP initialisation
		// process fails.
		curAddrTypesDHCPDiscoverFailed = GetCurrentAddressTypesL( socket );

		// Remove debug flags.
		INFO_PRINTF1(_L("Clearing debug options."));
		dhcpMemDbgParamBuf() = 0;
		debugSession.Ioctl(KCOLConfiguration, KDhcpMemDbgIoctl|KDHCP_DbgFlags, stat, &dhcpMemDbgParamBuf);
		User::WaitForRequest(stat);
		TESTEL(stat == KErrNone, stat.Int());
		
		socket.Close();
		connNET1.Stop();
		}

	// Restart the interface to remove any link local address.
	err = connNET1.Start(iConnPrefs);
	TESTEL(err == KErrNone, err);
	User::LeaveIfError( socket.Open( eSock, KAfInet, KSockDatagram, KProtocolInetUdp, connNET1 ) );

	// Check to see if DHCP has been enabled.
	TConnectionAddrBuf address;
	address().iAddressFamily = IpAddressFamilyL();
	connNET1.Ioctl( KCOLConfiguration, KConnGetCurrentAddr, stat, &address );
	User::WaitForRequest( stat );
	TBool dhcpEnabledForIAP = stat.Int() != KErrNotSupported;
	if( dhcpEnabledForIAP )
		{
		INFO_PRINTF1(_L("DHCP client is available."));

		GetDebugHandleL(connNET1);
  		}
   	else
		{
  		INFO_PRINTF1(_L("DHCP client is not available."));
   		}
	
	// Check to see if a link local is configured after the DHCP initialisation
	// process succeeds.
	INFO_PRINTF1(_L("Testing address types after successful DHCP initialisation..."));
	TUint curAddrTypesDHCP = GetCurrentAddressTypesL( socket );
	
	TUint curAddrTypesDHCPReleased = KCurAddrNone;
	TUint curAddrTypesDHCPRenewFailed = KCurAddrNone;
	
	if( dhcpEnabledForIAP && !isStatic )
		{
		// Read lease time
		TConnectionLeaseInfoBuf remainingLease1, remainingLease2;
		remainingLease2().iAddressFamily = IpAddressFamilyL();
		connNET1.Ioctl(KCOLConfiguration, KConnGetAddrLeaseTimeRemain, stat, &remainingLease2);
		User::WaitForRequest(stat);
		
		// Check to see if a lease time is supported or if the DHCPv6
		// 'M' and 'O' flags are both false.
		if( stat.Int() == KErrNone )
			{
			// Release the address and wait for the operation to finish.
			connNET1.Ioctl( KCOLConfiguration, KConnAddrRelease, stat );
			User::WaitForRequest(stat);
			TESTEL(stat == KErrNone, stat.Int());
			
			// Check to see if a link local is configured after the address is released (there should not be).
			INFO_PRINTF1(_L("Testing address types after DHCP release..."));
			curAddrTypesDHCPReleased = GetCurrentAddressTypesL( socket );

			// Set debug flags.
			INFO_PRINTF1(_L("Setting debug option to ensure short lease for first renewal..."));
		    TDhcpMemDbgParamBuf dhcpMemDbgParamBuf;
			dhcpMemDbgParamBuf() = KDHCP_SetShortLease;
			debugSession.Ioctl(KCOLConfiguration, KDhcpMemDbgIoctl|KDHCP_DbgFlags, stat, &dhcpMemDbgParamBuf);
			User::WaitForRequest(stat);
			TESTEL(stat == KErrNone, stat.Int());

			// Renew and ensure the lease will be short
			connNET1.Ioctl( KCOLConfiguration, KConnAddrRenew, stat );
			User::WaitForRequest(stat);
			TESTEL(stat == KErrNone, stat.Int());

			// Set debug flags.
			INFO_PRINTF1(_L("Setting debug option to simulate DHCP renewal failure after second renewal..."));
			dhcpMemDbgParamBuf() = KDHCP_FailDiscover | KDHCP_FailRenew | KDHCP_FailRebind;
			debugSession.Ioctl(KCOLConfiguration, KDhcpMemDbgIoctl|KDHCP_DbgFlags, stat, &dhcpMemDbgParamBuf);
			User::WaitForRequest(stat);
			TESTEL(stat == KErrNone, stat.Int());

			// Read initial remaining lease time
			remainingLease1().iAddressFamily = IpAddressFamilyL();
			connNET1.Ioctl(KCOLConfiguration, KConnGetAddrLeaseTimeRemain, stat, &remainingLease1);
			User::WaitForRequest(stat);
			TESTEL(stat == KErrNone, stat.Int());
		    INFO_PRINTF2(_L("Remaining lease time: %d secs"),remainingLease1().iSecondsRemaining);
		    
		    const TUint KInterval = 5 * 1000000;
		    TUint timeToWait = KInterval;
		    
			// Start loop for reading lease time.
			while( ETrue )
				{
				// Wait for 5 seconds to run down some of the lease or forever
				// until a state change occurs if the lease has expired...
				if( timeToWait == KMaxTUint32 )
					{
					DHCP_DEBUG_SUBSCRIBEL( DHCPDebug::EState + iDebugHandle );
					}
				else
					{
					timeToWait -= WAIT_FOR_STATE_CHANGE_WITH_TIMEOUTL( timeToWait );
					}
			
				// Check to see if we should report another interval has elapsed.
				if( timeToWait == 0 )
					{
					// Read lease time
					remainingLease2().iAddressFamily = IpAddressFamilyL();
					connNET1.Ioctl(KCOLConfiguration, KConnGetAddrLeaseTimeRemain, stat, &remainingLease2);
					User::WaitForRequest(stat);
					TESTEL(stat == KErrNone, stat.Int());

					// Output the remaining lease time.
				    INFO_PRINTF2(_L("Remaining lease time: %d secs"),remainingLease2().iSecondsRemaining);
					timeToWait = KInterval;

					if( remainingLease2().iSecondsRemaining <= 0 )
						{
						// The lease has expired.
						timeToWait = KMaxTUint32;
						}
					}
				
				// Test is complete when renewal fails.
				DHCPDebug::State state;
				QUERY_STATEL( state );
				if( ( state == DHCPDebug::EDHCPIPAddressAcquisition ) || ( state == DHCPDebug::EDHCPIP6Solicit ) || ( state == DHCPDebug::EDHCPIP4Select ) || ( state == DHCPDebug::EDHCPIP6Select ) )
					{
				    INFO_PRINTF2(_L("Renewal failed."),remainingLease2().iSecondsRemaining);
					
					break;
					}
				}

			// Check to see if a link local is configured after the DHCP renewal
			// process fails.
			curAddrTypesDHCPRenewFailed = GetCurrentAddressTypesL( socket );

			// Remove debug flags.
			INFO_PRINTF1(_L("Clearing debug options."));
			dhcpMemDbgParamBuf() = 0;
			debugSession.Ioctl(KCOLConfiguration, KDhcpMemDbgIoctl|KDHCP_DbgFlags, stat, &dhcpMemDbgParamBuf);
			User::WaitForRequest(stat);
			TESTEL(stat == KErrNone, stat.Int());
			}
		else
			{
			if( stat.Int() != KErrNotReady )
				{
				User::Leave( stat.Int() );
				}
			}
		}
	
	err = connNET1.Stop();
	TESTEL(err == KErrNone, err);

	CleanupStack::PopAndDestroy(&socket);
	CleanupStack::PopAndDestroy(&connNET1);
	CleanupStack::PopAndDestroy(&eSock);
	
	//Close DHCP debug session
	CleanupStack::PopAndDestroy(&debugSession);
	
	// If the link local option is enabled, verify that a link local address
	// is configured after the DHCP discovery process fails.
	if( UsingIPv6L() )
		{
		SetTestStepResult(EPass);
		}
	else
		{
		switch( ipv4linklocal )
			{
			// EV4LLDisabled
			case 0:
				{
	  	 		INFO_PRINTF1(_L("Link local option is set to EV4LLDisabled."));

				// Link locals are always disabled - the TCP/IP stack should not create
				// one for any test step.
				if( !( curAddrTypesDHCPDiscoverFailed & KCurAddrIPv4LinkLocal ) &&
					!( curAddrTypesDHCP & KCurAddrIPv4LinkLocal ) &&
					!( curAddrTypesDHCPReleased & KCurAddrIPv4LinkLocal ) &&
					!( curAddrTypesDHCPRenewFailed & KCurAddrIPv4LinkLocal ) )
					{
					SetTestStepResult(EPass);
					}
				else
					{
					INFO_PRINTF1(_L("Link local address management is incorrect."));
			
			        SetTestStepResult(EFail);
					}
				}
			break;	

			// EV4LLAlways
			case 1:
				{
	  	 		INFO_PRINTF1(_L("Link local option is set to EV4LLAlways."));

				// Link locals are always enabled - the TCP/IP stack should create
				// one regardless of any assigned address.
				if( isStatic )
					{
					if( ( curAddrTypesDHCPDiscoverFailed & KCurAddrIPv4LinkLocal ) &&
						( curAddrTypesDHCP & KCurAddrIPv4LinkLocal ) )
						{
						SetTestStepResult(EPass);
						}
					else
						{
						INFO_PRINTF1(_L("Link local address management is incorrect."));
			
				        SetTestStepResult(EFail);
						}
					}
				else
					{
					if( ( curAddrTypesDHCP & KCurAddrIPv4LinkLocal ) &&
						( curAddrTypesDHCPReleased & KCurAddrIPv4LinkLocal ) &&
						( curAddrTypesDHCPRenewFailed & KCurAddrIPv4LinkLocal ) )
						{
						SetTestStepResult(EPass);
						}
					else
						{
						INFO_PRINTF1(_L("Link local address management is incorrect."));
			
				        SetTestStepResult(EFail);
						}
					}
				}
			break;	

			// EV4LLConditional
			case 2:
				{
	  	 		INFO_PRINTF1(_L("Link local option is set to EV4LLConditional."));

				// Link locals are conditional - the TCP/IP stack should create
				// one if there is no statically assigned address.
				if( isStatic )
					{
					if( !( curAddrTypesDHCPDiscoverFailed & KCurAddrIPv4LinkLocal ) &&
						!( curAddrTypesDHCP & KCurAddrIPv4LinkLocal ) &&
						!( curAddrTypesDHCPReleased & KCurAddrIPv4LinkLocal ) &&
						!( curAddrTypesDHCPRenewFailed & KCurAddrIPv4LinkLocal ) )
						{
						SetTestStepResult(EPass);
						}
					else
						{
						INFO_PRINTF1(_L("Link local address management is incorrect."));
			
				        SetTestStepResult(EFail);
						}
					}
				else
					{
					if( ( curAddrTypesDHCP & KCurAddrIPv4LinkLocal ) &&
						( curAddrTypesDHCPReleased & KCurAddrIPv4LinkLocal ) &&
						( curAddrTypesDHCPRenewFailed & KCurAddrIPv4LinkLocal ) )
						{
						SetTestStepResult(EPass);
						}
					else
						{
						INFO_PRINTF1(_L("Link local address management is incorrect."));
			
				        SetTestStepResult(EFail);
						}
					}
				}
			break;

			// EV4LLConfigDaemonControlled
			case 3:
				{
	  	 		INFO_PRINTF1(_L("Link local option is set to EV4LLConfigDaemonControlled."));

				// Link locals are controlled by the configuration daemon -
				// the TCP/IP stack should create one if no address was assigned
				// by DHCP and there is no static address.
				if( isStatic )
					{
					if( !( curAddrTypesDHCPDiscoverFailed & KCurAddrIPv4LinkLocal ) &&
						!( curAddrTypesDHCP & KCurAddrIPv4LinkLocal ) &&
						!( curAddrTypesDHCPReleased & KCurAddrIPv4LinkLocal ) &&
						!( curAddrTypesDHCPRenewFailed & KCurAddrIPv4LinkLocal ) )
						{
						SetTestStepResult(EPass);
						}
					else
						{
						INFO_PRINTF1(_L("Link local address management is incorrect."));
			
				        SetTestStepResult(EFail);
						}
					}
				else
					{
					if( ( curAddrTypesDHCPDiscoverFailed & KCurAddrIPv4LinkLocal ) &&
						!( curAddrTypesDHCP & KCurAddrIPv4LinkLocal ) &&
						!( curAddrTypesDHCPReleased & KCurAddrIPv4LinkLocal ) &&
						( curAddrTypesDHCPRenewFailed & KCurAddrIPv4LinkLocal ) )
						{
						SetTestStepResult(EPass);
						}
					else
						{
						INFO_PRINTF1(_L("Link local address management is incorrect."));
			
				        SetTestStepResult(EFail);
						}
					}
				}
			break;
			
			default:
				{
				ASSERT( false ); // should never get here
				}
			break;
			}
		}

	return TestStepResult();
	
	#endif // __DHCP_HEAP_CHECK_CONTROL
	}
