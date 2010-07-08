// Copyright (c) 2003-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// Example CTestStep derived implementation
// 
//

/**
 @file
*/
#include "TeMsgStep.h"
#include "TeMsgSecureSocket.h"

//EPOC includes
#include <test/testexecutelog.h>
#include <es_sock.h>
#include <in_sock.h>
#include <c32root.h>
#include <commdbconnpref.h>
#include <e32std.h>
#include <securesocket.h>
#include <e32property.h>


_LIT(KIap,					"Iap");
_LIT(KIapNumber,			"IapNumber");
_LIT(KTCPConfig,			"TCP Config");
_LIT(KHostName,				"HostName");
_LIT(KPort,					"Port");
_LIT(KDNSName,				"DNSName"); 
_LIT(KPhbkSyncCMI,			"phbsync.cmi");

#if defined (__WINS__)
#define PDD_NAME _L("ECDRV")
#define LDD_NAME _L("ECOMM")
#else
#define PDD_NAME _L("EUART1")
#define LDD_NAME _L("ECOMM")
#endif
#ifndef SIROCCO_CODE_MIGRATION
const TInt iapCount = 2;
#endif
CTestConnectStep::CTestConnectStep():iIapNumber(0), iPort(0), iScheduler(NULL)
/**
 * Constructor
 */
	{
	// Call base class method to set up the human readable name for logging
	SetTestStepName(KConnectWithOverrides);
	}

CTestConnectStep::~CTestConnectStep()
/**
 * Destructor
 */
	{
	iConnection.Stop();
	iConnection.Close();
	iSocketServ.Close();
	}

TVerdict CTestConnectStep::doTestStepPreambleL()
	{
	TVerdict	ret = CTestStep::doTestStepPreambleL();
	
	if (ret != KErrNone)
		{
		SetTestStepResult(EFail);
		}
	else
		{
		iScheduler = new CActiveScheduler();
		CActiveScheduler::Install(iScheduler);
#ifdef SIROCCO_CODE_MIGRATION 
		TInt err;
#else
		INFO_PRINTF1(_L("Load PDD"));	
		TInt err = User::LoadPhysicalDevice(PDD_NAME);
		if (err != KErrNone && err != KErrAlreadyExists)
			{
			INFO_PRINTF2(_L("Could not load PDD! Leaving with error %d"), err);
			SetTestStepResult(EFail);
			User::Leave(err);
			}
#endif
		INFO_PRINTF1(_L("Load LDD"));	
		err = User::LoadLogicalDevice(LDD_NAME);
		if (err != KErrNone && err != KErrAlreadyExists)
			{
			INFO_PRINTF2(_L("Could not load LDD! Leaving with error %d"), err);
			SetTestStepResult(EFail);
			User::Leave(err);
			}
		}
	return ret;
	}

TVerdict CTestConnectStep::doTestStepPostambleL()
	{
	delete iScheduler;
	iScheduler=NULL;
	CActiveScheduler::Install(NULL);
	return CTestStep::doTestStepPostambleL();
	}


TVerdict CTestConnectStep::doTestStepL()
/**
 * @return - TVerdict code
 * Override of base class pure virtual
 * Demonstrates reading configuration parameters fom an ini file section
 */
	{
	if(TestStepResult()==EPass)
		{
		INFO_PRINTF1(_L("In Test Step ConnectWithOverrides"));
		// When bootstrapping C32 we have to avoid the PhBkSyncServer being started, since
 		// it needs a different CommDB
		TInt ret = StartC32WithCMISuppressions(KPhbkSyncCMI);
		
		if((ret!=KErrNone) && (ret!=KErrAlreadyExists))
			{
			INFO_PRINTF2(_L("error is : %d \n"),ret);
			}
		else
			{
			INFO_PRINTF1(_L("Started C32\n"));
			}

		RHostResolver hr;

		INFO_PRINTF1(_L("Connect to RSocketServ"));			
		User::LeaveIfError(iSocketServ.Connect());

		// Set up the connections, both overridden as host resolver won't work 
		// on the default interface
		INFO_PRINTF1(_L("Open the RConnection interface"));			
		User::LeaveIfError(iConnection.Open(iSocketServ));

		// get the IapNumber value from the ini file
		if(!GetIntFromConfig(ConfigSection(), KIapNumber, iIapNumber))
			{
			if(!GetIntFromConfig(KIap, KIapNumber, iIapNumber))
				{
				iIapNumber=1;
				INFO_PRINTF1(_L("Failed to read Iap from ini file, using default"));
				}
			}
		INFO_PRINTF2((_L("IapNumber = %d")), iIapNumber);

		TRequestStatus status;
#ifdef SIROCCO_CODE_MIGRATION
	TCommDbConnPref prefs;
	prefs.SetIapId(iIapNumber);
	prefs.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);
		const TUid KMyPropertyCat = {0x101FD9C5};
        const TUint32 KMyPropertyDestPortv4 = 67;
        TInt err = RProperty::Define(KMyPropertyCat, KMyPropertyDestPortv4, RProperty::EInt, TSecurityPolicy(TSecurityPolicy::EAlwaysPass),
		     TSecurityPolicy(ECapabilityWriteDeviceData));
        User::LeaveIfError(RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv4, 93));
#else

		TInt rank = 1;

		// Create a multiple connection preference object
		TCommDbMultiConnPref prefs;

		for(TInt i=0;i<iapCount;i++)
			{
			TCommDbConnPref pref;
			// Set the direction
			pref.SetDirection(ECommDbConnectionDirectionOutgoing);
			// Set the bear ser
			pref.SetBearerSet(KCommDbBearerPSD | KCommDbBearerCSD);
			// Set the IAP
			pref.SetIapId(iIapNumber + i);
			// Set the dialog preference to Do Not Prompt
			pref.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);
			// Set the rank
			User::LeaveIfError(prefs.SetPreference(rank, pref));

			rank++;
			}

		prefs.SetConnectionAttempts(rank-1);
#endif 

		// Start the connection
		iConnection.Start(prefs, status);

		
		User::WaitForRequest(status);

		TInt result = status.Int();

		if(result!=KErrNone)
			{
			ERR_PRINTF2(_L("Failed to start connection. Error %d"), result);
			SetTestStepResult(EFail);
			User::Leave(EFail);
			}
		else
			{
			INFO_PRINTF1(_L("Connection started with overrides"));
			}

		// Set up the host resolver
		INFO_PRINTF1(_L("Initialise a host resolver service"));			
		User::LeaveIfError(hr.Open(iSocketServ, KAfInet, KProtocolInetTcp, iConnection));
		CleanupClosePushL(hr);


		// Test the interfaces conn, host resolution should work ok
		TBuf<64> hostname;
		TRequestStatus status1;
		TNameEntry nameEntry;

		TPtrC temphost;
		if(GetStringFromConfig(KTCPConfig, KHostName, temphost))
			{
			INFO_PRINTF2(_L("Hostname is : %S"), &temphost);
			}
		else
			{
			ERR_PRINTF1(_L("No hostname"));
			SetTestStepResult(EFail);
			User::Leave(EFail);
			}
		hostname = temphost;

		hr.GetByName(hostname, nameEntry, status1);
		User::WaitForRequest(status1);

		TInt result1 = status1.Int();

		if(result1!=KErrNone)
			{
			ERR_PRINTF2(_L("Failed to resolve the name. Error %d"), result1);
			SetTestStepResult(EFail);
			User::Leave(EFail);
			}
		else
			{
			INFO_PRINTF1(_L("Resolved the name successfully"));
			}

		// open socket
		INFO_PRINTF1(_L("Open the socket"));			
		User::LeaveIfError(iSocket.Open(iSocketServ, KAfInet, KSockStream, KProtocolInetTcp, iConnection));
		CleanupClosePushL(iSocket);

		// print the host resolver's ip address
		TInetAddr inetAddr;
		inetAddr = TInetAddr(nameEntry().iAddr);
		TBuf<50> addr;
		inetAddr.Output(addr);
		INFO_PRINTF2(_L("The host resolver's ip address is: %S"), &addr);
		
		// get the port number from the ini file
		if(!GetIntFromConfig(KTCPConfig, KPort, iPort))
			{
			ERR_PRINTF1(_L("Failed to read Port from ini file"));
			SetTestStepResult(EFail);
			User::Leave(EFail);
			}
		INFO_PRINTF2((_L("Port = %d")), iPort);

		// connect to the socket
		inetAddr.SetPort(iPort);
		//nameEntry().iAddr.SetPort(iPort);
		TRequestStatus status2;
		//iSocket.Connect(nameEntry().iAddr,status2);
		iSocket.Connect(inetAddr,status2);
		User::WaitForRequest(status2);

		TInt result2 = status2.Int();

		if(result2!=KErrNone)
			{
			ERR_PRINTF2(_L("Failed to connect to the socket. Error %d"), result2);
			SetTestStepResult(EFail);
			User::Leave(EFail);
			}
		else
			{
			INFO_PRINTF1(_L("Connect to the socket successfully"));
			}


		TPtrC ptrDNSName;
		if(GetStringFromConfig(KTCPConfig, KDNSName, ptrDNSName))
			{
			INFO_PRINTF2(_L("DNSName is : %S"), &ptrDNSName);
			}
		else
			{
			ERR_PRINTF1(_L("No DNSName"));
			SetTestStepResult(EFail);
			User::Leave(EFail);
			}
		TBuf8<256>	bufDnsName;
		bufDnsName.Copy(ptrDNSName);

		// connect to the secure socket
		CTestSecureSocket* iTestSecureSocket = CTestSecureSocket::NewL(*this, iSocket, bufDnsName);
		CleanupStack::PushL(iTestSecureSocket);
		iTestSecureSocket->StartHandshake();
		CActiveScheduler::Start();

		CleanupStack::PopAndDestroy(3); //iTestSecureSocket, iSocket, hr
		}
	
	return TestStepResult();
	}


