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
// for (WINS && EKA1) versions will be xxxServer.Dll and require a thread to be started
// in the process of the client. The client initialises the server by calling the
// one and only ordinal.
// On EKA2 this is an exe.
// 
//

/**
 @file te_dhcpTestServer.cpp
*/

#include <in_sock.h>

#include "te_dhcpTestServer.h"
#include "te_dhcpTestStep1.h"
#include "te_dhcpTestStep2.h"
#include "te_dhcpTestStep3.h"
#include "te_dhcpTestStep4.h"
#include "te_dhcpTestStep5.h"
#include "te_dhcpTestStep6.h"
#include "te_dhcpTestCommandSteps.h"
#include "te_dhcpTestStepOOM.h"
#include "te_dhcpserverteststep.h"

#include <in6_opt.h>
#include <commdbconnpref.h>
#include <commsdattypesv1_1.h>

#define _DEBUG_DHCP_STATE_NAMES
#include "../../include/DHCPStatesDebug.h"

CDhcpTestServer* CDhcpTestServer::NewL()
/**
* @return - Instance of the test server
* Called inside the MainL() function to create and start the
* CTestServer derived server.
*/
	{
	CDhcpTestServer * server = new (ELeave) CDhcpTestServer();
	CleanupStack::PushL(server);
	// CServer base class call
	server->StartL(server->ServerName());

	CleanupStack::Pop(server);
	return server;
	}


#if (!defined EKA2)
LOCAL_C void MainL()
/**
* REQUIRES semaphore to sync with client as the Rendezvous()
* calls are not available
*/
	{
	CActiveScheduler* sched=NULL;
	sched = new(ELeave) CActiveScheduler;
	CleanupStack::PushL(sched);
	CActiveScheduler::Install(sched);

	CDhcpTestServer* server = NULL;

	TRAPD(err,server = CDhcpTestServer::NewL());
	// if there is an error, then we don't want to be doing all the
	// fancy stuff, just cleanup, adnd the rest is taken care of
	// for us...
	if(!err)
		{
		CleanupStack::PushL(server);
		RSemaphore sem;
		// The client API will already have created the semaphore
		User::LeaveIfError(sem.OpenGlobal(KServerName));
		CleanupStack::Pop(server);
		// Sync with the client then enter the active scheduler
		sem.Signal();
		sem.Close();
		sched->Start();
		}
	delete server;
	CleanupStack::PopAndDestroy(sched);
	}
#else
// EKA2 much simpler
// Just an E32Main and a MainL()
LOCAL_C void MainL()
/**
* Much simpler, uses the new Rendezvous() call to sync with the client
*/
	{
	// Leave the hooks in for platform security
#if (defined __DATA_CAGING__)
	RProcess().DataCaging(RProcess::EDataCagingOn);
	RProcess().SecureApi(RProcess::ESecureApiOn);
#endif
	CActiveScheduler* sched=NULL;
	sched = new(ELeave) CActiveScheduler;
	CleanupStack::PushL(sched);
	CActiveScheduler::Install(sched);

	CDhcpTestServer* server = NULL;
	// Create the CTestServer derived server
	TRAPD(err,server = CDhcpTestServer::NewL());
	// if there is an error, then we don't want to be doing all the
	// fancy stuff, just cleanup, adnd the rest is taken care of
	// for us...
	if(!err)
		{
		// Sync with the client and enter the active scheduler
		RProcess::Rendezvous(KErrNone);
#ifdef _DEBUG
		RProperty::Define(KMyPropertyCat, KMyPropertyDestPortv4, RProperty::EInt, TSecurityPolicy(TSecurityPolicy::EAlwaysPass),
					TSecurityPolicy(ECapabilityWriteDeviceData));
		RProperty::Define(KMyPropertyCat, KMyPropertyDestPortv6, RProperty::EInt, TSecurityPolicy(TSecurityPolicy::EAlwaysPass),
					TSecurityPolicy(ECapabilityWriteDeviceData));
		RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv4, 67);	// set to default values
		RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv6, 547);	// set to default values
        RProperty::Define(KMyPropertyCat, KMyDefaultLeaseTime, RProperty::EInt, TSecurityPolicy(TSecurityPolicy::EAlwaysPass),
                    TSecurityPolicy(ECapabilityWriteDeviceData));
        RProperty::Set(KMyPropertyCat, KMyDefaultLeaseTime, 21600);	// Define and set default values for server lease time
#endif
		sched->Start();
		}
	delete server;
	CleanupStack::PopAndDestroy(sched);
	}
#endif

// Only a DLL on emulator for typhoon and earlier
#if (defined __WINS__ && !defined EKA2)
TInt ThreadFunc (TAny* /*aParam*/)	// define the ordinal 1 on eka1
#else			
GLDEF_C TInt E32Main()	// define 32main if eka2...
#endif
/**
* @return - Standard Epoc error code on exit
*/
	{
	CTrapCleanup* cleanup = CTrapCleanup::New();
	if(cleanup == NULL)
		{
		return KErrNoMemory;
		}
	TInt err = KErrNone;
	TRAP(err,MainL());
	delete cleanup;
	return err;
    }

#if (defined __WINS__ && !defined EKA2)
/**
  * Define this helper function to start a new thread in the client
  * process for WINS and EKA1...
  */
EXPORT_C TInt NewServer() 
/**
* @return - Standard Epoc error codes
* 1st and only ordinal, called by the client API to initialise the server
*/
	{
	RThread thread;
	
	TBuf<KMaxTestExecuteNameLength> threadName(KServerName);
	// Create a hopefully unique thread name and use the ThreadFunc
	const TInt KMaxHeapSize = 0x1000000;			//< Allow a 1Mb max heap
	TInt err = thread.Create(threadName, ThreadFunc, KDefaultStackSize,
								KMinHeapSize, KMaxHeapSize, NULL, EOwnerProcess);
	if (err)
		{
		return err;
		}
	thread.Resume();
	thread.Close();
	return KErrNone;
	}


GLDEF_C TInt E32Dll(enum TDllReason)
	{
	return 0;
	}
#endif



CTestStep* CDhcpTestServer::CreateTestStep(const TDesC& aStepName)
/**
* @return - A CTestStep derived instance
* Implementation of CTestServer pure virtual
*/
	{
	CDhcpTestStepBase* testStep = NULL;
	// This server creates just one step but create as many as you want
	// They are created "just in time" when the worker thread is created
	if(aStepName == KDhcpTestStep1_1)
		testStep = new CDhcpTestStep1_1();
	else if(aStepName == KDhcpTestStep1_2)
		testStep = new CDhcpTestStep1_2();
	else if(aStepName == KDhcpTestStep1_3)
		testStep = new CDhcpTestStep1_3();
	else if(aStepName == KDhcpTestStep1_4)
		testStep = new CDhcpTestStep1_4();
	else if(aStepName == KDhcpTestStep1_5)
		testStep = new CDhcpTestStep1_5();
	else if(aStepName == KDhcpTestStep1_6)
		testStep = new CDhcpTestStep1_6();
	else if(aStepName == KDhcpTestStep1_7)
		testStep = new CDhcpTestStep1_7();
	else if(aStepName == KDhcpTestStep1_8)
		testStep = new CDhcpTestStep1_8();
	
	else if(aStepName == KDhcpTestStep2_1)
		testStep = new CDhcpTestStep2_1();
	else if(aStepName == KDhcpTestStep2_2)
		testStep = new CDhcpTestStep2_2();
	else if(aStepName == KDhcpTestStep2_3)
		testStep = new CDhcpTestStep2_3();
	else if(aStepName == KDhcpTestStep2_4)
		testStep = new CDhcpTestStep2_4();
	else if(aStepName == KDhcpTestStep2_5)
		testStep = new CDhcpTestStep2_5();
	else if(aStepName == KDhcpTestStep2_6)
		testStep = new CDhcpTestStep2_6();
	else if(aStepName == KDhcpTestStep2_7)
		testStep = new CDhcpTestStep2_7();
	else if(aStepName == KDhcpTestStep2_8)
		testStep = new CDhcpTestStep2_8();
	else if(aStepName == KDhcpTestStep2_9)
		testStep = new CDhcpTestStep2_9();
    else if(aStepName == KDhcpTestStep2_GetRaw)
		testStep = new CDhcpTestStep2_GetRaw();
	else if(aStepName == KDhcpTestStep2_11)
		testStep = new CDhcpTestStep2_11();
    else if(aStepName == KDhcpTestStep2_12)
		testStep = new CDhcpTestStep2_12();		
    else if(aStepName == KDhcpTestStep2_ClearMOFlag)
        testStep = new CDhcpTestStep2_ClearMOFlag();
    else if(aStepName == KDhcpTestStep2_NoRAandDHCPServ)
        testStep = new CDhcpTestStep2_NoRAandDHCPServ(); 
	else if(aStepName == KDhcpTestStep2_GetSIPAddrViaDHCP)
		testStep = new CDhcpTestStep2_GetSIPAddrViaDHCP();
	else if(aStepName == KDhcpTestStep2_GetSIPAddrViaPCOBuffer)
		testStep = new CDhcpTestStep2_GetSIPAddrViaPCOBuffer();
    else if(aStepName == KDhcpTestStep2_GetSIPDomain)
		testStep = new CDhcpTestStep2_GetSIPDomain();	
	else if(aStepName == KDhcpTestStep2_GetSIPAddrFailure)
		testStep = new CDhcpTestStep2_GetSIPAddrFailure();
	else if(aStepName == KDhcpTestStep2_GetSIPAddrBufferOverrun)
		testStep = new CDhcpTestStep2_GetSIPAddrBufferOverrun();
	else if(aStepName == KDhcpTestStep2_GetSIPServerAddrIndexChecker)
		testStep = new CDhcpTestStep2_GetSIPServerAddrIndexChecker();
	else if(aStepName == KDhcpTestStep2_GetSIPServerAddrNegativeIndexChecker)
		testStep = new CDhcpTestStep2_GetSIPServerAddrNegativeIndexChecker();
    #ifdef SYMBIAN_TCPIPDHCP_UPDATE
	/* PREQ1898 DHCPv6 test cases */
	else if(aStepName == KDhcpTestStep2_DomainSearchList_Test1)
		testStep = new CDhcpTestStep2_DomainSearchList_Test1();	
	else if(aStepName == KDhcpTestStep2_DomainSearchList_Test2)
		testStep = new CDhcpTestStep2_DomainSearchList_Test2();	
	else if(aStepName == KDhcpTestStep2_DomainSearchList_Test3)
		testStep = new CDhcpTestStep2_DomainSearchList_Test3();	
	else if(aStepName == KDhcpTestStep2_DomainSearchList_Test4)
		testStep = new CDhcpTestStep2_DomainSearchList_Test4();	
	else if(aStepName == KDhcpTestStep2_DomainSearchList_Test5)
		testStep = new CDhcpTestStep2_DomainSearchList_Test5();	
	
	else if(aStepName == KDhcpTestStep2_DNSServerList_Test1)
		testStep = new CDhcpTestStep2_DNSServerList_Test1();	
	else if(aStepName == KDhcpTestStep2_DNSServerList_Test2)
		testStep = new CDhcpTestStep2_DNSServerList_Test2();	
	else if(aStepName == KDhcpTestStep2_DNSServerList_Test3)
		testStep = new CDhcpTestStep2_DNSServerList_Test3();	
	else if(aStepName == KDhcpTestStep2_DNSServerList_Test4)
		testStep = new CDhcpTestStep2_DNSServerList_Test4();	
	else if(aStepName == KDhcpTestStep2_DNSServerList_Test5)
		testStep = new CDhcpTestStep2_DNSServerList_Test5();		

	else if(aStepName == KDhcpTestStep2_MultipleParams_Test1)
		testStep = new CDhcpTestStep2_MultipleParams_Test1();
	else if(aStepName == KDhcpTestStep2_MultipleParams_Test2)
		testStep = new CDhcpTestStep2_MultipleParams_Test2();	
	else if(aStepName == KDhcpTestStep2_MultipleParams_Test3)
		testStep = new CDhcpTestStep2_MultipleParams_Test3();	
	else if(aStepName == KDhcpTestStep2_MultipleParams_Test4)
		testStep = new CDhcpTestStep2_MultipleParams_Test4();		
	else if(aStepName == KDhcpTestStep2_MultipleParams_Test5)
		testStep = new CDhcpTestStep2_MultipleParams_Test5();	
	
	else if(aStepName == KDhcpTestStep2_GetRaw_Test1)
		testStep = new CDhcpTestStep2_GetRaw_Test1();
	else if(aStepName == KDhcpTestStep2_GetRaw_Test2)
		testStep = new CDhcpTestStep2_GetRaw_Test2();	
	/* End of PREQ 1898 test case */
    #endif //SYMBIAN_TCPIPDHCP_UPDATE 
	
	else if(aStepName == KDhcpTestStep3_1)
		testStep = new CDhcpTestStep3_1();
	else if(aStepName == KDhcpTestStep3_2)
		testStep = new CDhcpTestStep3_2();
	else if(aStepName == KDhcpTestStep3_3)
		testStep = new CDhcpTestStep3_3();		
	else if(aStepName == KDhcpTestStep3_4)
		testStep = new CDhcpTestStep3_4();
	
	else if(aStepName == KDhcpTestStep4_1)
		testStep = new CDhcpTestStep4_1();
	else if(aStepName == KDhcpTestStep4_2)
		testStep = new CDhcpTestStep4_2();
	else if(aStepName == KDhcpTestStep4_3)
		testStep = new CDhcpTestStep4_3();

	// tests in section 5 cannot be run automatically
	// overnight, but may be able to run on hardware
	else if(aStepName == KDhcpTestStep5_1)
		testStep = new CDhcpTestStep5_1();
	else if(aStepName == KDhcpTestStep5_2)
		testStep = new CDhcpTestStep5_2();
	else if(aStepName == KDhcpTestStep5_3)
		testStep = new CDhcpTestStep5_3();
	else if(aStepName == KDhcpTestStep5_4)
		testStep = new CDhcpTestStep5_4();
	else if(aStepName == KDhcpTestStep5_5)
		testStep = new CDhcpTestStep5_5();
	else if(aStepName == KDhcpTestStep5_6)
		testStep = new CDhcpTestStep5_6();
	else if(aStepName == KDhcpTestStep5_7)
		testStep = new CDhcpTestStep5_7();
	else if(aStepName == KDhcpTestStep5_8)
		testStep = new CDhcpTestStep5_8();

	if(aStepName == KDhcpTestStep12_1)
		testStep = new CDhcpTestStep12_1();

	else if(aStepName == KDhcpTestStepOOM_1)
		testStep = new CDhcpTestStepOOM_1();
	
#ifdef SYMBIAN_NETWORKING_DHCPSERVER
//tests added for the DHCP server implementation and run manually

	else if(aStepName == KDhcpTestStep2_23)
		testStep = new CDhcpTestStep2_23();
	else if(aStepName == KDhcpTestStep2_24)
		testStep = new CDhcpTestStep2_24();
	else if(aStepName == KDhcpTestStep2_25)
		testStep = new CDhcpTestStep2_25();
	else if(aStepName == KDhcpTestStepOOM_2)
		testStep = new CDhcpTestStepOOM_2();
		
#endif // SYMBIAN_NETWORKING_DHCPSERVER		
		
	// commands
	else if(aStepName == KDhcpTestCommandSetAddressMode)
		testStep = new CDhcpTestCommandSetAddressMode();
	else if(aStepName == KDhcpTestCommandSetIAPToUse)
		testStep = new CDhcpTestCommandSetIAPToUse();
	else if(aStepName == KDhcpTestCommandSetDebugFlags)
		testStep = new CDhcpTestCommandSetDebugFlags();
	else if(aStepName == KDhcpTestCommandSetIPv4LinkLocal)
		testStep = new CDhcpTestCommandSetIPv4LinkLocal();
	
#ifdef  SYMBIAN_NETWORKING_DHCP_MSG_HEADERS

	else if(aStepName == KDhcpTestStep2_26)
		testStep = new CDhcpTestStep2_26();
	else if(aStepName == KDhcpTestStep2_27)
		testStep = new CDhcpTestStep2_27();
	else if(aStepName == KDhcpTestStep2_28)
		testStep = new CDhcpTestStep2_28();
	else if(aStepName == KDhcpTestStep2_29)
		testStep = new CDhcpTestStep2_29();
	else if(aStepName == KDhcpTestStep2_30)
		testStep = new CDhcpTestStep2_30();
	else if(aStepName == KDhcpTestStep2_31)
		testStep = new CDhcpTestStep2_31();
	else if(aStepName == KDhcpTestStep2_32)
		testStep = new CDhcpTestStep2_32();
	else if(aStepName == KDhcpTestStep2_33)
		testStep = new CDhcpTestStep2_33();
	else if(aStepName == KDhcpTestStep2_34)
		testStep = new CDhcpTestStep2_34();
	else if(aStepName == KDhcpTestStep2_35)
		testStep = new CDhcpTestStep2_35();
	else if(aStepName == KDhcpTestStep2_36)
		testStep = new CDhcpTestStep2_36();
	else if(aStepName == KDhcpTestStep2_37)
		testStep = new CDhcpTestStep2_37();
	else if(aStepName == KDhcpTestStep2_38)
		testStep = new CDhcpTestStep2_38();
	else if(aStepName == KDhcpTestStep2_39)
		testStep = new CDhcpTestStep2_39();
	else if(aStepName == KDhcpTestStep2_40)
		testStep = new CDhcpTestStep2_40();	
	else if(aStepName == KDhcpTestStep2_41)
		testStep = new CDhcpTestStep2_41();	
	else if(aStepName == KDhcpTestStepOOM_3)
		testStep = new CDhcpTestStepOOM_3();
		
#endif //SYMBIAN_NETWORKING_DHCP_MSG_HEADERS  			
	
  /* Added for DHCP testing with Codenomicon tool*/			
	else if(aStepName == KDhcpTestStepCodenomicon1)
		testStep = new CDhcpTestStepCodenomicon1();	
	else if(aStepName == KDhcpTestStepCodenomicon2)
		testStep = new CDhcpTestStepCodenomicon2();	
	else if(aStepName == KDhcpTestStepServ_1)
	    testStep = new CDhcpTestStepServ_1(); 
	else if(aStepName == KDhcpTestStepServ_2)
	    testStep = new CDhcpTestStepServ_2(); 
	else if(aStepName == KDhcpTestStepServ_3)
	    testStep = new CDhcpTestStepServ_3(); 
	else if(aStepName == KDhcpTestStepServ_4)
			testStep = new CDhcpTestStepServ_4(); 
	else if(aStepName == KDhcpTestStepServ_5)
			testStep = new CDhcpTestStepServ_5(); 
	else if(aStepName == KDhcpTestStepServ_6)
			testStep = new CDhcpTestStepServ_6(); 
	
	if(testStep)
		{
		testStep->SetTestServer(this);
		}
			
	return testStep;
	}


//----------------------------------------------------------------------------------

/**
 * sets the test server to be used by the test step for storing / accessing
 *  data which persists across test steps (e.g. IAP to use, IPv6 mode)
 */   
void CDhcpTestStepBase::SetTestServer(CDhcpTestServer* aServ)
	{
	iTestServer = aServ;
	}

/**
 * sets whether we're in IPv6 mode
@leave   if the test server is not available.
 */   
void CDhcpTestStepBase::Set_UsingIPv6L(TBool aIP6)
	{
	User::LeaveIfNull(iTestServer);
	iTestServer->iUsingIPv6 = aIP6;

	}

/**
 * sets which IAP subsequent test cases should use
@leave   if the test server is not available.
 */   
void CDhcpTestStepBase::SetIAPToUseL(TInt aIAP)
	{
	User::LeaveIfNull(iTestServer);
	iTestServer->iIAPToUse = aIAP;
	}


/**
* adds the ECDPrivate attribute to the IAP record - any component trying to access this record will require the ReadDeviceData capability
@leave   if the test server is not available or the IAP record with the specified ID cannot be retrieved and modified by commsdat
*/
void CDhcpTestStepBase::MakeIAPPrivateL( TInt id )
	{
	// Create a session which can view hidden records if necessary.
#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
	CMDBSession* session = CMDBSession::NewLC(KCDVersion1_2);
#else
	CMDBSession* session = CMDBSession::NewLC(KCDVersion1_1);
#endif
	session->SetAttributeMask( ECDHidden | ECDPrivate );
	
	// Load the IAP record.
	CCDIAPRecord* iapRecord = static_cast<CCDIAPRecord*>( CCDRecordBase::RecordFactoryL( KCDTIdIAPRecord ) );
	CleanupStack::PushL( iapRecord );
    iapRecord->SetRecordId( id );
	iapRecord->LoadL( *session );

	// Make it private.
	iapRecord->SetAttributes( iapRecord->Attributes() | ECDPrivate );
	iapRecord->ModifyL( *session );

	// Cleanup.
	CleanupStack::Pop();
	CleanupStack::Pop();
	}


/**
@return  ETrue if we are using ipv6, and EFalse otherwise.
@leave   if the value from test server can't be obtained.
*/   
TBool CDhcpTestStepBase::UsingIPv6L(void) const
{
	User::LeaveIfNull(iTestServer);
	return iTestServer->iUsingIPv6;
}

/**
@return  the IAP we're currently set to use
@leave   if the value from test server can't be obtained.
*/   
TInt CDhcpTestStepBase::IAPToUseL(void)
	{
	User::LeaveIfNull(iTestServer);
	TInt iap = iTestServer->iIAPToUse;
	if(iap == 0)
		{
		INFO_PRINTF1(_L("You must use the SetIAPToUse test step before running this test"));
		User::Leave(KErrArgument); 
		}
	return iap;
	}

/**
@leave   if the value from commsdat can't be obtained.
*/   
void CDhcpTestStepBase::InitialServiceLinkL( CMDBSession* aDbSession, CCDIAPRecord* aIapRecord )
	{
	// This will be made more simple quite soon when commsdat supports linked records
	//
	if (aIapRecord->iService.iLinkedRecord == 0)
		{
		const TDesC& servType = aIapRecord->iServiceType;
		__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CCDHCPDb::InitialServiceLinkL() service type = \"%S\""), &servType));
		if (servType.CompareF(TPtrC(KCDTypeNameDialOutISP))==0)
			{
			aIapRecord->iService.iLinkedRecord = static_cast<CCDDialOutISPRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdDialOutISPRecord));
			}
		else if (servType.CompareF(TPtrC(KCDTypeNameDialInISP))==0)
			{
			aIapRecord->iService.iLinkedRecord = static_cast<CCDDialInISPRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdDialInISPRecord));
			}
		else if (servType.CompareF(TPtrC(KCDTypeNameLANService))==0)
			{
			aIapRecord->iService.iLinkedRecord = static_cast<CCDLANServiceRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdLANServiceRecord));
			}
		else if (servType.CompareF(TPtrC(KCDTypeNameVPNService))==0)
			{
			aIapRecord->iService.iLinkedRecord = static_cast<CCDVPNServiceRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdVPNServiceRecord));
			}
		else
			{
			__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CCDHCPDb::InitialServiceLinkL() Invalid Service Type!!!")));
			User::Leave(KErrBadName);	
			}
		
		aIapRecord->iService.iLinkedRecord->SetRecordId(aIapRecord->iService);
		}

	aIapRecord->iService.iLinkedRecord->LoadL(*aDbSession);
	}

/**
@return  ETrue if the IAP we are using or EFalse otherwise
@leave   if the value from commsdat can't be obtained.
*/   
TBool CDhcpTestStepBase::IsIAPAddrStaticL()
	{
	// Create a session which can view hidden records if necessary.
#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
	CMDBSession* session = CMDBSession::NewLC(KCDVersion1_2);
#else
	CMDBSession* session = CMDBSession::NewLC(KCDVersion1_1);
#endif
	
	session->SetAttributeMask( ECDHidden | ECDPrivate );
	
	// Load the IAP record.
	CCDIAPRecord* iapRecord = static_cast<CCDIAPRecord*>( CCDRecordBase::RecordFactoryL( KCDTIdIAPRecord ) );
	CleanupStack::PushL( iapRecord );
    iapRecord->SetRecordId( IAPToUseL() );
	iapRecord->LoadL( *session );

	// Get the service record.
	CCDServiceRecordBase* service;
	InitialServiceLinkL( session, iapRecord );
	service = static_cast<CCDServiceRecordBase*>(iapRecord->iService.iLinkedRecord);

	// Read any statically assigned IP address.
	TInetAddr addr;
	TInt ignoreThis;
	CMDBField<TBool>* addrFromServer = static_cast<CMDBField<TBool>*>( service->GetFieldByNameL( KCDTypeNameIpAddrFromServer, ignoreThis ) );
	if( !*addrFromServer )
		{
		CMDBField<TDesC>* cdbAddr = static_cast<CMDBField<TDesC>*>( service->GetFieldByNameL( KCDTypeNameIpAddr, ignoreThis ) );
		addr.Input( *cdbAddr );
		}

	// Cleanup.
	CleanupStack::PopAndDestroy();
	CleanupStack::PopAndDestroy();
	
	return !addr.IsUnspecified();
	}

/**
@return Flags listing the currently configured address types.
@leave   if any system error occurs.
*/
TUint CDhcpTestStepBase::GetCurrentAddressTypesL( RSocket &socket )
{
	TUint flags = KCurAddrNone;
	TPtr8 empty( NULL, 0 );

	// Query the size of the buffer required.
	TInt listLength = socket.GetOpt( KSoInetAddressInfo, KSolInetIfQuery, empty );
	
	if( listLength > 0 )
		{
		// Allocate the buffer and read the list of addresses.
		TInetAddressInfo* addrInfoBuffer = new(ELeave) TInetAddressInfo[listLength];
		TPtr8 opt( (TUint8* )addrInfoBuffer, listLength * sizeof( TInetAddressInfo ) );
		if( socket.GetOpt( KSoInetAddressInfo, KSolInetIfQuery, opt ) == KErrNone )
			{
			listLength = opt.Length() / (TInt)sizeof( TInetAddressInfo );

			for( TInt count = 0; count < listLength; count ++ )
				{
				// Check that the address has been assigned.
				if( ( addrInfoBuffer[count].iState == TInetAddressInfo::EAssigned ) ||
					( addrInfoBuffer[count].iState == TInetAddressInfo::ETentative ) )
					{
					TIp6Addr& addr( addrInfoBuffer[count].iAddress );
					TInetAddr inetAddr = TInetAddr( addr, 0 );

					if( !inetAddr.IsUnspecified() && !inetAddr.IsLoopback() )
						{
						TBuf<39> addrStr;
						inetAddr.Output( addrStr );
						
						 // Some addresses seem to be set to the IPv6 family which causes
						 // IsLinkLocal to match using the IPv6 link local mask but not the
						 // IPv4 link local mask.  We test manually as well which should be
						 // safe as ZEROCONF randomly generated link-locals can only be created
						 // during IPv4 testing.
						if( inetAddr.IsLinkLocal() ||
							( ( INET_ADDR( inetAddr.Ptr()[10], inetAddr.Ptr()[11], inetAddr.Ptr()[12], inetAddr.Ptr()[13] ) & KInetAddrLinkLocalNetMask ) == KInetAddrLinkLocalNet ) )
							{
							flags |= KCurAddrIPv4LinkLocal;
							
							INFO_PRINTF2( _L( "Linklocal address %S found on interface"), &addrStr );
							}
						else
							{
							flags |= KCurAddrGlobal;

							INFO_PRINTF2( _L( "Global address %S found on interface"), &addrStr );
							}
						}
					}
				}
			}
		}
		
	if( flags == KCurAddrNone )
		{
		INFO_PRINTF1( _L( "No addresses found on interface") );
		}
	
	return flags;
}



/**
@return KAfInet6 or KAfInet depending on the test configuration (see test .ini file)
@leave   if the value from ini file can't be obtained.
*/
TUint   CDhcpTestStepBase::IpAddressFamilyL(void)
{
    return UsingIPv6L() ? KAfInet6 :  KAfInet;
}

/**
    Test step preamble. Just prints out which type AddrFamily we use.
*/
TVerdict	CDhcpTestStepBase::doTestStepPreambleL()
{
    if(UsingIPv6L())
    {
        INFO_PRINTF1(_L("KAfInet6 Address Family selected"));    
    }
    else
    {
        INFO_PRINTF1(_L("KAfInet Address Family selected"));    
    }
    
    INFO_PRINTF2(_L("This test step will use IAP %d"), IAPToUseL());
    
	iConnPrefs.SetIapId(IAPToUseL());
	iConnPrefs.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);

    return EPass;
}


/**
    Get raw option data either for ip4 of for ip6, depending on the test setting
    @param  aConn    RConnection object, interface to the DHCP server
    @param  aBufDes  buffer for option data
    @param  aOptCode option code   
    
    @return IOCTL completion code
*/
TInt CDhcpTestStepBase::DhcpGetRawOptionDataL(RConnection &aConn, TDes8& aBufDes, TUint aOptCode)
{
	TUint dhcpOptName;
    TRequestStatus stat;
    
	//-- DHCP option data structure is totally different for DHCPv4 and DHCPv6
	//-- so, we need to handle getting raw option data separately for v4 and v6.
	if(UsingIPv6L())
	{//-- we are dealing with DHCPv6 
        INFO_PRINTF1(_L("Getting DHCP6 raw option data")); 
        TDhcp6RawOptionDataPckg pckg6(aBufDes);
        pckg6.SetOpCode((TUint16)aOptCode);	// OPTION_CLIENTID

        dhcpOptName = KConnGetDhcp6RawOptionData;
	}
	else
	{//-- we are dealing with DHCPv4 
    	INFO_PRINTF1(_L("Getting DHCP4 raw option data"));    
    	TDhcp4RawOptionDataPckg pckg(aBufDes);
    	pckg.SetOpCode((TUint8)aOptCode);	// the subnet mask
    	
    	dhcpOptName = KConnGetDhcp4RawOptionData; // the same as KConnGetDhcpRawOptionData
	}
	
	// Get raw option data either for ip4 of ip6
	aConn.Ioctl(KCOLConfiguration, dhcpOptName, stat, &aBufDes);
    User::WaitForRequest(stat);
    
    return stat.Int();
}

void CDhcpTestStepBase::GetDebugHandleL(RConnection& aConn)
	{
    TRequestStatus stat;
    TPckgBuf<TInt> pckg;
	aConn.Ioctl(KCOLConfiguration, KDHCP_GetPubSubMonitorHandle, stat, &pckg);
    User::WaitForRequest(stat);
	if(stat.Int() != KErrNone)
		{
		INFO_PRINTF1(_L("Can't fetch debug handle from DHCP. Ensure it was built in debug mode"));
		User::Leave(stat.Int());
		}
	iDebugHandle = pckg();
	}

void CDhcpTestStepBase::ImmediateCompletionTestL(RConnection& /*aConn*/)
{
	DHCPDebug::State state;

	TBool bExpectImmediateCompletion;
    if(GetBoolFromConfig(ConfigSection(),_L("ExpectImmediateCompletion"), bExpectImmediateCompletion) == EFalse)
    	{
		INFO_PRINTF1(_L("Couldn't read ExpectImmediateCompletion value from test ini file."));
		SetTestStepResult(EFail);
		User::Leave(KErrArgument);
    	}

    DHCPDebug::Readiness ready=DHCPDebug::EUnknown;
    QUERY_READYL(ready);
    
    if( ready == DHCPDebug::EReady )
    	{
    	if( bExpectImmediateCompletion )
    		{
	    	INFO_PRINTF1(_L("DHCP shouldn't be ready so quickly after RConnection::Start completed"));
			User::Leave(KErrUnknown);
			}
		else
		    {
		    INFO_PRINTF1(_L("As expected, DHCP is ready after RConnection::Start completed."));
		    }
		}
	else if ( ready == DHCPDebug::ENotReady )
		{
		if( bExpectImmediateCompletion )
			{
			INFO_PRINTF1(_L("As expected, DHCP isn't yet ready though RConnection::Start completed."));
			}
		else
		    {
	    	INFO_PRINTF1(_L("DHCP should be ready after RConnection::Start completed"));
			User::Leave(KErrUnknown);
			}			
		}
	else
    	{
    	INFO_PRINTF1(_L("Couldn't read readiness state"));
		User::Leave(KErrUnknown);
    	}

	LOG_STATEL;
}

const TPtrC CDhcpTestServer::ServerName() const
	{

	// On EKA2, test server runs in its own process.
	// So we arrive at the server name using the exe from which it is loaded.
	// This is useful when doing cap tests, as we create duplicate exe's using setcap then.
	TParsePtrC serverName(RProcess().FileName());
	return serverName.Name();
	}

TUint CDhcpTestStepBase::WAIT_FOR_STATE_CHANGE_WITH_TIMEOUTL( TUint aTimeout )
/**
* Waits for the DHCP state to change or for a timeout to occur.  Not implemented
* as macro because it's too complicated.
*
* Returns aTimeout if the wait times-out or the remainder if the state changed
* (guaranteed to be less than aTimeout and at least one microsecond).
*/
	{
    TAutoClose<RTimer> timer;
    User::LeaveIfError( timer.iObj.CreateLocal() );
    timer.PushL();
	
	TRequestStatus propStatus;
	TRequestStatus timerStatus;
	
	// Attach to the property.
	TAutoClose<RProperty> a;
	TUid u = TUid::Uid(0x101fd9c5);
	User::LeaveIfError( a.iObj.Attach(u, (TUint)(DHCPDebug::EState + iDebugHandle)) );
	a.PushL();
	
	// Subscribe to the property.
	TRequestStatus pubStat;
	a.iObj.Subscribe(pubStat);
	
	TTime startTime;
	startTime.UniversalTime();
	
	// Start the timeout timer.
	timer.iObj.After( timerStatus, aTimeout );
	
	// Wait for the property to change or the timer to timeout.
	User::WaitForRequest( pubStat, timerStatus );
	
	a.iObj.Cancel();
	timer.iObj.Cancel();

	a.Pop();
	timer.Pop();
	
	// Check to see if the property changed or the timer timed-out.
	if( timerStatus.Int() == KErrNone )
		{
		return aTimeout;
		}
	else
		{
		User::LeaveIfError( pubStat.Int() );
		}
	
	TTime endTime;
	endTime.UniversalTime();
		
	return Min( Max( TInt( endTime.MicroSecondsFrom( startTime ).Int64() ), 1 ), aTimeout );
	}

void CDhcpTestStepBase::GetProvisioningMacL(TDesC16& aSectioName, const TDesC16& aKeyName, TDes8& aHwAddress) 
    {
    TPtrC macAddressString;
    TInt macAddressLength = aHwAddress.Length();
    if (!GetStringFromConfig(aSectioName, aKeyName, macAddressString))
        {
        User::Leave(KErrNotFound);
        }
    
    aHwAddress.FillZ();
    
    TUint8 bitVal;
    for (TInt index=0, offset=0; index < macAddressLength; index++)
        {
        TLex bitText(macAddressString.Mid(offset,2));
        bitText.Val(bitVal, EHex);
        aHwAddress.Append(bitVal);
        offset+=2;
        }
    }

