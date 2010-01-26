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
//

#ifdef SYMBIAN_NETWORKING_UMTSR5
// EPOC includes
#include <e32base.h>

// Test system includes
#include "Log.h"
#include "TestStep.h"
#include "TestStepQoS.h"
#include "TestSuiteQoS.h"
#include "QoSTest.h"


#include <qoserr.h>
#include "TestIf.h"

#include <f32file.h>
#include <s32file.h>
#include <commdb.h>
#include <comms-infras\nifif.h>
#include <nifman.h>
#include <es_enum.h>
#include <CommDbConnPref.h>

#include <cs_subconparams.h>
#include <cs_subconevents.h>
#include <es_sock.h>
#include "uscl_ip_subconparams.h"



const TInt KInterTestDelay = 40 * 300000;    // 12 sec


//
//  
//



CQoSTest_401::CQoSTest_401()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText48, "qos_401");
    iTestStepName = KText48;
    }

CQoSTest_401::~CQoSTest_401()
    {
    }

TVerdict CQoSTest_401::doTestStepL( void )
    { 
    User::After(KInterTestDelay);
    _LIT(KText, "qos_401 Creation of UMTS R5 secondary context ");
    Log(KText);
    
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn1; 
    RSocket                 socket1;
    TInetAddr               dstAddr1;
    TRequestStatus          status;
    TInt                    ret; 
    
    CleanupClosePushL(socketServer);
    CleanupClosePushL(conn);
    CleanupClosePushL(subconn1);
    CleanupClosePushL(socket1);
    
    
    // Connect to ESOCK 
    ret = socketServer.Connect();
    TESTL(ret == KErrNone); 

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP socket
    ret = socket1.Open(socketServer, KAfInet, KSockDatagram, KProtocolInetUdp, conn);
    TESTL(ret == KErrNone);

    dstAddr1.SetPort(3461);
    _LIT(KText315, "10.1.46.1");
    dstAddr1.Input( KText315 );

    socket1.Connect(dstAddr1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    // open a new sub-connection 
    ret = subconn1.Open(socketServer, RSubConnection::ECreateNew, conn); 
    TESTL(ret == KErrNone);

    
    // Create the container for all subconnection parameters 
    RSubConParameterBundle parameterBundle;
    CleanupClosePushL(parameterBundle);

    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily = CSubConParameterFamily::NewL(parameterBundle, KSubConQoSFamily ); 
    
    // Create the requested/acceptable generic parameter set for QoS 
    // (Qos family takes ownership) 
    CSubConQosGenericParamSet* reqGenericParams = CSubConQosGenericParamSet::NewL(*qosFamily, CSubConParameterFamily::ERequested);
    CSubConQosGenericParamSet* accGenericParams = CSubConQosGenericParamSet::NewL(*qosFamily, CSubConParameterFamily::EAcceptable);
    
    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet1(*reqGenericParams);
    SetEsockParamSet1(*accGenericParams);
    
    CSubConQosR5ParamSet* requestedR5Qos = NULL;
    CSubConQosR5ParamSet* acceptableR5Qos = NULL;
    
    requestedR5Qos = CSubConQosR5ParamSet::NewL(*qosFamily, CSubConParameterFamily::ERequested);
    acceptableR5Qos = CSubConQosR5ParamSet::NewL(*qosFamily, CSubConParameterFamily::EAcceptable);
    
    SetUmtsR5QoSRequested1(*requestedR5Qos); 
    SetUmtsR5QoSAcceptable1(*acceptableR5Qos);
    
    // Subscribe for QoS event notification
    TRequestStatus  eventStatusForAdd;
    TNotificationEventBuf   subconnNotifBufForAdd;
    subconn1.EventNotification(subconnNotifBufForAdd, ETrue, eventStatusForAdd); 
        {
		// Set properties of the sub-connection 
	    ret = subconn1.SetParameters(parameterBundle); 
	    TESTL(ret == KErrNone);
	    
	    // Move the connected socket to the new sub-connection
	    // This will create a new secondary context
	    subconn1.Add(socket1, status);
	    User::WaitForRequest(status);
	    TESTL(status.Int() == KErrNone);
	    
	    // Receive & Process the subconn.SetParameters-event 
	    // (expected KSubConGenericEventParamsGranted)
	    User::WaitForRequest(eventStatusForAdd); 
	    TESTL(eventStatusForAdd.Int() == KErrNone);
	    ret = CheckEvent(subconnNotifBufForAdd, KSubConGenericEventParamsGranted); 
	    TESTE(ret == KErrNone, ret);
	    if(ret == KErrNone)
			{
			// Extract granted UMTS R5 information from subconn
			ret = CheckUmtsR5ExtensionValuesL(subconn1, ETrue, RPacketQoS::ESourceStatisticsDescriptorSpeech);    
			TESTE(ret == KErrNone, ret);
			}
        }
    // unsubscribe for QoS event notification
	subconn1.CancelEventNotification();		
	
    // To reset the message values of the testnif
    TEST(NifAPIL(socket1, KResetMessage) == KErrNone);

    CleanupStack::Pop(&parameterBundle);
    // close and destroy
    CleanupStack::PopAndDestroy(&socket1);
    CleanupStack::PopAndDestroy(&subconn1);
    CleanupStack::PopAndDestroy(&conn);
    CleanupStack::PopAndDestroy(&socketServer);
    ClearTestFiles();
	return iTestStepResult;
    }
    

CQoSTest_402::CQoSTest_402()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText48, "qos_402");
    iTestStepName = KText48;
    }

CQoSTest_402::~CQoSTest_402()
    {
    }

TVerdict CQoSTest_402::doTestStepL( void )
    { 
    User::After(KInterTestDelay);
    _LIT(KText, "qos_402 Failure of UMTS R5 secondary context creation ");
    Log(KText);
    
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn1; 
    RSocket                 socket1;
    TInetAddr               dstAddr1;
    TRequestStatus          status;
    TInt                    ret;  

    // Connect to ESOCK 
    ret = socketServer.Connect();
    TESTL(ret == KErrNone); 
    CleanupClosePushL(socketServer);

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 
    CleanupClosePushL(conn);

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP socket
    ret = socket1.Open(socketServer, KAfInet, KSockDatagram, KProtocolInetUdp, conn);
    TESTL(ret == KErrNone);
     CleanupClosePushL(socket1);

    dstAddr1.SetPort(3461);
    _LIT(KText315, "10.1.46.1");
    dstAddr1.Input( KText315 );

    socket1.Connect(dstAddr1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    // open a new sub-connection 
    ret = subconn1.Open(socketServer, RSubConnection::ECreateNew, conn); 
    TESTL(ret == KErrNone);
    CleanupClosePushL(subconn1);

    // Create the container for all subconnection parameters 
    RSubConParameterBundle parameterBundle;
    CleanupClosePushL(parameterBundle);

    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily = CSubConParameterFamily::NewL(parameterBundle, KSubConQoSFamily ); 
    
    // Create the requested/acceptable generic parameter set for QoS 
    // (Qos family takes ownership) 
    CSubConQosGenericParamSet* reqGenericParams = CSubConQosGenericParamSet::NewL(*qosFamily, CSubConParameterFamily::ERequested);
    CSubConQosGenericParamSet* accGenericParams = CSubConQosGenericParamSet::NewL(*qosFamily, CSubConParameterFamily::EAcceptable);
    
    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet1(*reqGenericParams);
    SetEsockParamSet1(*accGenericParams);
    
    
    CSubConQosR5ParamSet* requestedR5Qos = NULL;
    CSubConQosR5ParamSet* acceptableR5Qos = NULL;
    
    requestedR5Qos = CSubConQosR5ParamSet::NewL(*qosFamily, CSubConParameterFamily::ERequested);
    acceptableR5Qos = CSubConQosR5ParamSet::NewL(*qosFamily, CSubConParameterFamily::EAcceptable);
        
    SetUmtsR5QoSRequested1(*requestedR5Qos); 
    SetUmtsR5QoSAcceptable1(*acceptableR5Qos);
    
    /* 	inform the testnif to fail the r5 context 
    *	modification. 
    */
    TEST(NifAPIL(socket1, KContextActivateFailAsync) == KErrNone);
    
    // Subscribe for QoS event notification
    TRequestStatus  eventStatusForAdd;
    TNotificationEventBuf   subconnNotifBufForAdd;
    subconn1.EventNotification(subconnNotifBufForAdd, ETrue, eventStatusForAdd); 
        {
		// Set properties of the sub-connection 
	    ret = subconn1.SetParameters(parameterBundle); 
	    TESTL(ret == KErrNone);
	    
	    // Move the connected socket to the new sub-connection
	    // This will create a new secondary context
	    subconn1.Add(socket1, status);
	    User::WaitForRequest(status);
	    TESTE(status.Int() == KErrTest, status.Int());

	    // Receive & Process the subconn.SetParameters-event 
	    // (expected KSubConGenericEventParamsRejected)
	    User::WaitForRequest(eventStatusForAdd); 
	    TESTL(eventStatusForAdd.Int() == KErrNone);
	    ret = CheckEvent(subconnNotifBufForAdd, KSubConGenericEventParamsRejected); 
	    TESTE(ret == KErrNone, ret);
        }
    // unsubscribe for QoS event notification
	subconn1.CancelEventNotification();		
	
    // To reset the message values of the testnif
    TEST(NifAPIL(socket1, KResetMessage) == KErrNone);

    CleanupStack::Pop(&parameterBundle);
    // close and destroy
    CleanupStack::PopAndDestroy(&subconn1);
    CleanupStack::PopAndDestroy(&socket1);
    CleanupStack::PopAndDestroy(&conn);
    CleanupStack::PopAndDestroy(&socketServer);
    ClearTestFiles();
	return iTestStepResult;        
    }
    

CQoSTest_403::CQoSTest_403()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText48, "qos_403");
    iTestStepName = KText48;
    }

CQoSTest_403::~CQoSTest_403()
    {
    }


TVerdict CQoSTest_403::doTestStepL( void )
    { 
    User::After(KInterTestDelay);
    _LIT(KText, "qos_403 Creation of UMTS R5 context when SI flag is set");
    Log(KText);
    
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn1; 
    RSocket                 socket1;
    TInetAddr               dstAddr1;
    TRequestStatus          status;
    TInt                    ret; 
    
    
    // Connect to ESOCK 
    ret = socketServer.Connect();
    TESTL(ret == KErrNone); 
    CleanupClosePushL(socketServer);

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 
    CleanupClosePushL(conn);

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP socket
    ret = socket1.Open(socketServer, KAfInet, KSockDatagram, KProtocolInetUdp, conn);
    TESTL(ret == KErrNone);
    CleanupClosePushL(socket1);

    dstAddr1.SetPort(3461);
    _LIT(KText315, "10.1.46.1");
    dstAddr1.Input( KText315 );

    socket1.Connect(dstAddr1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    // open a new sub-connection 
    ret = subconn1.Open(socketServer, RSubConnection::ECreateNew, conn); 
    TESTL(ret == KErrNone);
    CleanupClosePushL(subconn1);

    // Create the container for all subconnection parameters 
    RSubConParameterBundle parameterBundle;
    CleanupClosePushL(parameterBundle);

    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily = CSubConParameterFamily::NewL(parameterBundle, KSubConQoSFamily ); 
    
    // Create the requested/acceptable generic parameter set for QoS 
    // (Qos family takes ownership) 
    CSubConQosGenericParamSet* reqGenericParams = CSubConQosGenericParamSet::NewL(*qosFamily, CSubConParameterFamily::ERequested);
    CSubConQosGenericParamSet* accGenericParams = CSubConQosGenericParamSet::NewL(*qosFamily, CSubConParameterFamily::EAcceptable);
    
    // Set the requested/acceptable Generic Parameters
    // set the priority value to 1.
    SetEsockParamSetWithPriorityOne(*reqGenericParams);
    SetEsockParamSetWithPriorityOne(*accGenericParams);
    
    CSubConQosR5ParamSet* requestedR5Qos = NULL;
    CSubConQosR5ParamSet* acceptableR5Qos = NULL;
    
    requestedR5Qos = CSubConQosR5ParamSet::NewL(*qosFamily, CSubConParameterFamily::ERequested);
    acceptableR5Qos = CSubConQosR5ParamSet::NewL(*qosFamily, CSubConParameterFamily::EAcceptable);
    
    SetUmtsR5QoSRequested1(*requestedR5Qos); 
    SetUmtsR5QoSAcceptable1(*acceptableR5Qos);
    
    // Subscribe for QoS event notification
    TRequestStatus  eventStatusForAdd;
    TNotificationEventBuf   subconnNotifBufForAdd;
    subconn1.EventNotification(subconnNotifBufForAdd, ETrue, eventStatusForAdd); 
        {
	    // Set properties of the sub-connection 
	    ret = subconn1.SetParameters(parameterBundle); 
	    TESTL(ret == KErrNone);

	    // Move the connected socket to the new sub-connection
	    // This will create a new secondary context
	    subconn1.Add(socket1, status);
	    User::WaitForRequest(status);
	    TESTL(status.Int() == KErrNone);
	    
	    // Receive & Process the subconn.SetParameters-event 
	    // (expected KSubConGenericEventParamsGranted)
	    User::WaitForRequest(eventStatusForAdd); 
	    TESTL(eventStatusForAdd.Int() == KErrNone);
	    ret = CheckEvent(subconnNotifBufForAdd, KSubConGenericEventParamsGranted); 
	    TESTE(ret == KErrNone, ret);
	    if(ret == KErrNone)
			{
			// Extract granted UMTS R5 information from subconn
			ret = CheckUmtsR5ExtensionValuesL(subconn1, ETrue, RPacketQoS::ESourceStatisticsDescriptorSpeech);    
			TESTE(ret == KErrNone, ret);
			}
	    
	    
	    // check the umts r5 parameter values returned by the network
	    RSubConParameterBundle bundle1;
	    ret = subconn1.GetParameters(bundle1);
	    TESTL(ret == KErrNone);
	    CSubConParameterFamily* family1 = bundle1.FindFamily(KSubConQoSFamily);
	    CSubConQosR5ParamSet* grantedR5Params = 
						(CSubConQosR5ParamSet*)family1->FindExtensionSet(STypeId(KSubCon3GPPExtParamsFactoryUid, 
																KSubConQosR5ParamsType),CSubConParameterFamily::EGranted);
	    if(grantedR5Params)
	        {
	        TEST(grantedR5Params->GetTrafficClass() == RPacketQoS::ETrafficClassInteractive);
	        TEST(grantedR5Params->GetTrafficHandlingPriority() == 0x02); //RPacketQoS::TTrafficHandlingPriority::ETrafficPriority1
	        }
	    else
	    	{
	    	Log(_L("CSubConQosR5ParamSet Extension not found in the returned event"));	
	    	TEST(EFalse);
	    	}
        }
	// unsubscribe for QoS event notification
	subconn1.CancelEventNotification();		
	
	// To reset the message values of the testnif
    TEST(NifAPIL(socket1, KResetMessage) == KErrNone);
    
    CleanupStack::Pop(&parameterBundle);
    // close and destroy
    CleanupStack::PopAndDestroy(&subconn1);
    CleanupStack::PopAndDestroy(&socket1);
    CleanupStack::PopAndDestroy(&conn);
    CleanupStack::PopAndDestroy(&socketServer);
    ClearTestFiles();
    return iTestStepResult;
    }
    

CQoSTest_404::CQoSTest_404()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText48, "qos_404");
    iTestStepName = KText48;
    }

CQoSTest_404::~CQoSTest_404()
    {
    }

TVerdict CQoSTest_404::doTestStepL( void )
    { 
    User::After(KInterTestDelay);
    /*
    This is testing the r5 context failure when SI flag is set.
    */_LIT(KText, "qos_404 New UMTS R5 context when SI flag is set with different priority value");
    Log(KText);
    
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn1; 
    RSocket                 socket1;
    TInetAddr               dstAddr1;
    TRequestStatus          status;
    TInt                    ret; 
    
    
    // Connect to ESOCK 
    ret = socketServer.Connect();
    TESTL(ret == KErrNone); 
    CleanupClosePushL(socketServer);

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 
    CleanupClosePushL(conn);

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP socket
    ret = socket1.Open(socketServer, KAfInet, KSockDatagram, KProtocolInetUdp, conn);
    TESTL(ret == KErrNone);
    CleanupClosePushL(socket1);

    dstAddr1.SetPort(3461);
    _LIT(KText315, "10.1.46.1");
    dstAddr1.Input( KText315 );

    socket1.Connect(dstAddr1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    // open a new sub-connection 
    ret = subconn1.Open(socketServer, RSubConnection::ECreateNew, conn); 
    TESTL(ret == KErrNone);
    CleanupClosePushL(subconn1);

    // Create the container for all subconnection parameters 
    RSubConParameterBundle parameterBundle;
    CleanupClosePushL(parameterBundle);

    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily = CSubConParameterFamily::NewL(parameterBundle, KSubConQoSFamily ); 
    
    // Create the requested/acceptable generic parameter set for QoS 
    // (Qos family takes ownership) 
    CSubConQosGenericParamSet* reqGenericParams = CSubConQosGenericParamSet::NewL(*qosFamily, CSubConParameterFamily::ERequested);
    CSubConQosGenericParamSet* accGenericParams = CSubConQosGenericParamSet::NewL(*qosFamily, CSubConParameterFamily::EAcceptable);
    
    // Set the requested/acceptable Generic Parameters
    // set the priority value to other than 1.
    SetEsockParamSet1(*reqGenericParams);
    SetEsockParamSet1(*accGenericParams);
    
    CSubConQosR5ParamSet* requestedR5Qos = NULL;
    CSubConQosR5ParamSet* acceptableR5Qos = NULL;
    
    requestedR5Qos = CSubConQosR5ParamSet::NewL(*qosFamily, CSubConParameterFamily::ERequested);
    acceptableR5Qos = CSubConQosR5ParamSet::NewL(*qosFamily, CSubConParameterFamily::EAcceptable);
    
    // set r99 parameters    
    requestedR5Qos->SetTrafficClass(RPacketQoS::ETrafficClassInteractive);
    requestedR5Qos->SetDeliveryOrder(RPacketQoS::EDeliveryOrderRequired);
    requestedR5Qos->SetErroneousSDUDelivery(RPacketQoS::EErroneousSDUDeliveryNotRequired);
    requestedR5Qos->SetResidualBitErrorRatio(RPacketQoS::EBEROnePerThousand);
    requestedR5Qos->SetSDUErrorRatio(RPacketQoS::ESDUErrorRatioOnePerTenThousand);
    requestedR5Qos->SetTrafficHandlingPriority(RPacketQoS::ETrafficPriorityUnspecified);
    requestedR5Qos->SetTransferDelay(500);
    requestedR5Qos->SetMaxSduSize(1500);
    requestedR5Qos->SetMaxBitrateUplink(1500);
    requestedR5Qos->SetMaxBitrateDownlink(900);
    requestedR5Qos->SetGuaBitrateUplink(450);
    requestedR5Qos->SetGuaBitrateDownlink(250);
	// set r5 parameters        
    requestedR5Qos->SetSignallingIndicator(ETrue); 
    requestedR5Qos->SetSourceStatisticsDescriptor(RPacketQoS::ESourceStatisticsDescriptorSpeech);
    
    acceptableR5Qos->SetTrafficClass(RPacketQoS::ETrafficClassInteractive);
    acceptableR5Qos->SetDeliveryOrder(RPacketQoS::EDeliveryOrderNotRequired);
    acceptableR5Qos->SetErroneousSDUDelivery(RPacketQoS::EErroneousSDUDeliveryNotRequired);
    acceptableR5Qos->SetResidualBitErrorRatio(RPacketQoS::EBEROnePerMillion);
    acceptableR5Qos->SetSDUErrorRatio(RPacketQoS::ESDUErrorRatioOnePerHundredThousand);
    acceptableR5Qos->SetTrafficHandlingPriority(RPacketQoS::ETrafficPriorityUnspecified);
    acceptableR5Qos->SetTransferDelay(1000);
    acceptableR5Qos->SetMaxSduSize(1500);
    acceptableR5Qos->SetMaxBitrateUplink(1000);
    acceptableR5Qos->SetMaxBitrateDownlink(800);
    acceptableR5Qos->SetGuaBitrateUplink(400);
    acceptableR5Qos->SetGuaBitrateDownlink(200);
    
    acceptableR5Qos->SetSignallingIndicator(ETrue); 
    acceptableR5Qos->SetSourceStatisticsDescriptor(RPacketQoS::ESourceStatisticsDescriptorSpeech);
       
    
    // Subscribe for QoS event notification
    TRequestStatus  eventStatusForAdd;
    TNotificationEventBuf   subconnNotifBufForAdd;
    subconn1.EventNotification(subconnNotifBufForAdd, ETrue, eventStatusForAdd); 
        {
	    // Set properties of the sub-connection 
	    ret = subconn1.SetParameters(parameterBundle); 
	    TESTL(ret == KErrNone);
	    
	    // Move the connected socket to the new sub-connection
	    // This will create a new secondary context
	    subconn1.Add(socket1, status);
	    User::WaitForRequest(status);
	    TESTL(status.Int() == KErrNone);

	    // Receive & Process the subconn.SetParameters-event 
	    // (expected KSubConGenericEventParamsGranted)
	    User::WaitForRequest(eventStatusForAdd); 
	    TESTL(eventStatusForAdd.Int() == KErrNone);
	    ret = CheckEvent(subconnNotifBufForAdd, KSubConGenericEventParamsGranted); 
	    TESTE(ret == KErrNone, ret);
	    if(ret == KErrNone)
			{
			// Extract granted UMTS R5 information from subconn
			ret = CheckUmtsR5ExtensionValuesL(subconn1, ETrue, RPacketQoS::ESourceStatisticsDescriptorSpeech);    
			TESTE(ret == KErrNone, ret);
			}
	    
	    
	    // check the umts r5 parameter values returned by the network
	    RSubConParameterBundle bundle1;
	    ret = subconn1.GetParameters(bundle1);
	    TESTL(ret == KErrNone);
	    CSubConParameterFamily* family1 = bundle1.FindFamily(KSubConQoSFamily);
	    CSubConQosR5ParamSet* grantedR5Params = 
						(CSubConQosR5ParamSet*)family1->FindExtensionSet(STypeId(KSubCon3GPPExtParamsFactoryUid, 
																KSubConQosR5ParamsType),CSubConParameterFamily::EGranted);
	    if(grantedR5Params)
	        {
	        TEST(grantedR5Params->GetTrafficClass() == RPacketQoS::ETrafficClassInteractive);
	        TEST(grantedR5Params->GetTrafficHandlingPriority() == 0x02); //RPacketQoS::TTrafficHandlingPriority::ETrafficPriority1
	        }
	    else
	    	{
	    	Log(_L("CSubConQosR5ParamSet Extension not found in the returned event"));	
	    	TEST(EFalse);
	    	}
        }
    // unsubscribe for QoS event notification
	subconn1.CancelEventNotification();		
	
    // To reset the message values of the testnif
    TEST(NifAPIL(socket1, KResetMessage) == KErrNone);

    CleanupStack::Pop(&parameterBundle);
    // close and destroy
    CleanupStack::PopAndDestroy(&subconn1);
    CleanupStack::PopAndDestroy(&socket1);
    CleanupStack::PopAndDestroy(&conn);
    CleanupStack::PopAndDestroy(&socketServer);
    ClearTestFiles();
    return iTestStepResult;
    }
    

CQoSTest_405::CQoSTest_405()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText48, "qos_405");
    iTestStepName = KText48;
    }

CQoSTest_405::~CQoSTest_405()
    {
    }

TVerdict CQoSTest_405::doTestStepL( void )
    { 
    User::After(KInterTestDelay);
    _LIT(KText, "qos_405 Failure of UMTS R5 context creation when SI flag is set");
    Log(KText);
    
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn1; 
    RSocket                 socket1;
    TInetAddr               dstAddr1;
    TRequestStatus          status;
    TInt                    ret; 
    
    
    // Connect to ESOCK 
    ret = socketServer.Connect();
    TESTL(ret == KErrNone); 
    CleanupClosePushL(socketServer);

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 
    CleanupClosePushL(conn);

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP socket
    ret = socket1.Open(socketServer, KAfInet, KSockDatagram, KProtocolInetUdp, conn);
    TESTL(ret == KErrNone);
    CleanupClosePushL(socket1);

    dstAddr1.SetPort(3461);
    _LIT(KText315, "10.1.46.1");
    dstAddr1.Input( KText315 );

    socket1.Connect(dstAddr1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    // open a new sub-connection 
    ret = subconn1.Open(socketServer, RSubConnection::ECreateNew, conn); 
    TESTL(ret == KErrNone);
    CleanupClosePushL(subconn1);

    // Create the container for all subconnection parameters 
    RSubConParameterBundle parameterBundle;
    CleanupClosePushL(parameterBundle);

    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily = CSubConParameterFamily::NewL(parameterBundle, KSubConQoSFamily ); 
    
    // Create the requested/acceptable generic parameter set for QoS 
    // (Qos family takes ownership) 
    CSubConQosGenericParamSet* reqGenericParams = CSubConQosGenericParamSet::NewL(*qosFamily, CSubConParameterFamily::ERequested);
    CSubConQosGenericParamSet* accGenericParams = CSubConQosGenericParamSet::NewL(*qosFamily, CSubConParameterFamily::EAcceptable);
    
    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet1(*reqGenericParams);
    SetEsockParamSet1(*accGenericParams);
    
    CSubConQosR5ParamSet* requestedR5Qos = NULL;
    CSubConQosR5ParamSet* acceptableR5Qos = NULL;
    
    requestedR5Qos = CSubConQosR5ParamSet::NewL(*qosFamily, CSubConParameterFamily::ERequested);
    acceptableR5Qos = CSubConQosR5ParamSet::NewL(*qosFamily, CSubConParameterFamily::EAcceptable);
        
    SetUmtsR5QoSRequested1(*requestedR5Qos); 
    SetUmtsR5QoSAcceptable1(*acceptableR5Qos);

    
    /* 	inform the testnif to fail the r5 context 
    *	activation. 
    */
    TEST(NifAPIL(socket1, KContextActivateFailAsync) == KErrNone);
    
    // Subscribe for QoS event notification
    TRequestStatus  eventStatusForAdd;
    TNotificationEventBuf   subconnNotifBufForAdd;
    subconn1.EventNotification(subconnNotifBufForAdd, ETrue, eventStatusForAdd); 
        {
	    // Set properties of the sub-connection 
	    ret = subconn1.SetParameters(parameterBundle); 
	    TESTL(ret == KErrNone);
	    
	    // Move the connected socket to the new sub-connection
	    // This will create a new secondary context
	    subconn1.Add(socket1, status);
	    User::WaitForRequest(status);
	    TESTE(status.Int() == KErrTest, status.Int());

	    // Receive & Process the subconn.SetParameters-event 
	    // (expected KSubConGenericEventParamsRejected)
	    User::WaitForRequest(eventStatusForAdd); 
	    TESTL(eventStatusForAdd.Int() == KErrNone);
	    ret = CheckEvent(subconnNotifBufForAdd, KSubConGenericEventParamsRejected); 
	    TESTE(ret == KErrNone, ret);
        }
	// unsubscribe for QoS event notification
	subconn1.CancelEventNotification();		        
	
    // To reset the message values of the testnif
    TEST(NifAPIL(socket1, KResetMessage) == KErrNone);

    CleanupStack::Pop(&parameterBundle);
    // close and destroy
    CleanupStack::PopAndDestroy(&subconn1);
    CleanupStack::PopAndDestroy(&socket1);
    CleanupStack::PopAndDestroy(&conn);
    CleanupStack::PopAndDestroy(&socketServer);
    ClearTestFiles();
    return iTestStepResult;
    }
    

CQoSTest_406::CQoSTest_406()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText48, "qos_406");
    iTestStepName = KText48;
    }

CQoSTest_406::~CQoSTest_406()
    {
    }

TVerdict CQoSTest_406::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText, "qos_406 : Network downgrades UMTS R5 parameters when SI flag is set");
    Log(KText);
    
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn1; 
    RSocket                 socket1;
    TInetAddr               dstAddr1;
    TRequestStatus          status;
    TInt                    ret; 
    
    
    // Connect to ESOCK 
    ret = socketServer.Connect();
    TESTL(ret == KErrNone); 
    CleanupClosePushL(socketServer);

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 
    CleanupClosePushL(conn);

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP socket
    ret = socket1.Open(socketServer, KAfInet, KSockDatagram, KProtocolInetUdp, conn);
    TESTL(ret == KErrNone);
    CleanupClosePushL(socket1);

    dstAddr1.SetPort(3461);
    _LIT(KText315, "10.1.46.1");
    dstAddr1.Input( KText315 );

    socket1.Connect(dstAddr1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 
    
    // open a new sub-connection 
    ret = subconn1.Open(socketServer, RSubConnection::ECreateNew, conn); 
    TESTL(ret == KErrNone);
    CleanupClosePushL(subconn1);

    
    // Create the container for all subconnection parameters 
    RSubConParameterBundle parameterBundle;
    CleanupClosePushL(parameterBundle);

    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily = CSubConParameterFamily::NewL(parameterBundle, KSubConQoSFamily ); 
    
    // Create the requested/acceptable generic parameter set for QoS 
    // (Qos family takes ownership) 
    CSubConQosGenericParamSet* reqGenericParams = CSubConQosGenericParamSet::NewL(*qosFamily, CSubConParameterFamily::ERequested);
    CSubConQosGenericParamSet* accGenericParams = CSubConQosGenericParamSet::NewL(*qosFamily, CSubConParameterFamily::EAcceptable);
    
    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet1(*reqGenericParams);
    SetEsockParamSet1(*accGenericParams);
    
    CSubConQosR5ParamSet* requestedR5Qos = NULL;
    CSubConQosR5ParamSet* acceptableR5Qos = NULL;
    
    requestedR5Qos = CSubConQosR5ParamSet::NewL(*qosFamily, CSubConParameterFamily::ERequested);
    acceptableR5Qos = CSubConQosR5ParamSet::NewL(*qosFamily, CSubConParameterFamily::EAcceptable);
        
    SetUmtsR5QoSRequested1(*requestedR5Qos); 
    SetUmtsR5QoSAcceptable1(*acceptableR5Qos);
  
    TEST(NifAPIL(socket1, KNetworkDowngradeR5) == KErrNone);
    
    // Subscribe for QoS event notification
    TRequestStatus  eventStatusForAdd;
    TNotificationEventBuf   subconnNotifBufForAdd;
    subconn1.EventNotification(subconnNotifBufForAdd, ETrue, eventStatusForAdd); 
        {
	    // Set properties of the sub-connection 
	    ret = subconn1.SetParameters(parameterBundle); 
	    TESTL(ret == KErrNone);

	    // Move the connected socket to the new sub-connection
	    // This will create a new secondary context
	    subconn1.Add(socket1, status);
	    User::WaitForRequest(status);
	    TESTL(status.Int() == KErrNone);
	    
	    // Receive & Process the subconn.SetParameters-event 
	    // (expected KSubConGenericEventParamsRejected)
	    User::WaitForRequest(eventStatusForAdd); 
	    TESTL(eventStatusForAdd.Int() == KErrNone);
	    ret = CheckEvent(subconnNotifBufForAdd, KSubConGenericEventParamsRejected); 
	    TESTE(ret == KErrNone, ret);
		if(ret == KErrNone)
			{
			// Extract granted UMTS R5 information from subconn
			ret = CheckUmtsR5ExtensionValuesL(subconn1, EFalse, RPacketQoS::ESourceStatisticsDescriptorUnknown);    
			TESTE(ret == KErrNone, ret);
			}
        }
	// unsubscribe for QoS event notification
	subconn1.CancelEventNotification();	
	        
    // To reset the message values of the testnif
    TEST(NifAPIL(socket1, KResetMessage) == KErrNone);

    CleanupStack::Pop(&parameterBundle);
    // close and destroy
    CleanupStack::PopAndDestroy(&subconn1);
    CleanupStack::PopAndDestroy(&socket1);
    CleanupStack::PopAndDestroy(&conn);
    CleanupStack::PopAndDestroy(&socketServer);
    ClearTestFiles();
	return iTestStepResult;
    }
    

CQoSTest_407::CQoSTest_407()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText48, "qos_407");
    iTestStepName = KText48;
    }

CQoSTest_407::~CQoSTest_407()
    {
    }


TVerdict CQoSTest_407::doTestStepL( void )
    { 
    User::After(KInterTestDelay);
    _LIT(KText, "qos_407 : Network downgrading R5 parameters");
    Log(KText);
    
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn1; 
    RSocket                 socket1;
    TInetAddr               dstAddr1;
    TRequestStatus          status;
    TInt                    ret; 
    
    
    // Connect to ESOCK 
    ret = socketServer.Connect();
    TESTL(ret == KErrNone); 
    CleanupClosePushL(socketServer);

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 
    CleanupClosePushL(conn);

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP socket
    ret = socket1.Open(socketServer, KAfInet, KSockDatagram, KProtocolInetUdp, conn);
    TESTL(ret == KErrNone);
    CleanupClosePushL(socket1);

    dstAddr1.SetPort(3461);
    _LIT(KText315, "10.1.46.1");
    dstAddr1.Input( KText315 );

    socket1.Connect(dstAddr1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    // open a new sub-connection 
    ret = subconn1.Open(socketServer, RSubConnection::ECreateNew, conn); 
    TESTL(ret == KErrNone);
    CleanupClosePushL(subconn1);

    // Create the container for all subconnection parameters 
    RSubConParameterBundle parameterBundle;
    CleanupClosePushL(parameterBundle);

    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily = CSubConParameterFamily::NewL(parameterBundle, KSubConQoSFamily ); 
    
    // Create the requested/acceptable generic parameter set for QoS 
    // (Qos family takes ownership) 
    CSubConQosGenericParamSet* reqGenericParams = CSubConQosGenericParamSet::NewL(*qosFamily, CSubConParameterFamily::ERequested);
    CSubConQosGenericParamSet* accGenericParams = CSubConQosGenericParamSet::NewL(*qosFamily, CSubConParameterFamily::EAcceptable);
    
    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet1(*reqGenericParams);
    SetEsockParamSet1(*accGenericParams);
    
    CSubConQosR5ParamSet* requestedR5Qos = NULL;
    CSubConQosR5ParamSet* acceptableR5Qos = NULL;
    
    requestedR5Qos = CSubConQosR5ParamSet::NewL(*qosFamily, CSubConParameterFamily::ERequested);
    acceptableR5Qos = CSubConQosR5ParamSet::NewL(*qosFamily, CSubConParameterFamily::EAcceptable);
        
    SetUmtsR5QoSRequested1(*requestedR5Qos); 
    SetUmtsR5QoSAcceptable1(*acceptableR5Qos);
    
    // Subscribe for QoS event notification
    TRequestStatus  eventStatusForAdd;
    TNotificationEventBuf   subconnNotifBufForAdd;
    subconn1.EventNotification(subconnNotifBufForAdd, ETrue, eventStatusForAdd); 
        {
  
	    TEST(NifAPIL(socket1, KNetworkDowngradeR5) == KErrNone);
	    
	    // Set properties of the sub-connection 
	    ret = subconn1.SetParameters(parameterBundle); 
	    TESTL(ret == KErrNone);

	    // Move the connected socket to the new sub-connection
	    // This will create a new secondary context
	    subconn1.Add(socket1, status);
	    User::WaitForRequest(status);
	    TESTL(status.Int() == KErrNone);
	    
	    // Receive & Process the subconn.SetParameters-event 
	    // (expected KSubConGenericEventParamsRejected)
	    User::WaitForRequest(eventStatusForAdd); 
	    TESTL(eventStatusForAdd.Int() == KErrNone);
	    ret = CheckEvent(subconnNotifBufForAdd, KSubConGenericEventParamsRejected); 
	    TESTE(ret == KErrNone, ret);
		if(ret == KErrNone)
			{
			// Extract granted UMTS R5 information from subconn
			ret = CheckUmtsR5ExtensionValuesL(subconn1, EFalse, RPacketQoS::ESourceStatisticsDescriptorUnknown);    
			TESTE(ret == KErrNone, ret);
			}
		}
	// unsubscribe for QoS event notification
	subconn1.CancelEventNotification();				
    
    // To reset the message values of the testnif
    TEST(NifAPIL(socket1, KResetMessage) == KErrNone);

    CleanupStack::Pop(&parameterBundle);
    // close and destroy
    CleanupStack::PopAndDestroy(&subconn1);
    CleanupStack::PopAndDestroy(&socket1);
    CleanupStack::PopAndDestroy(&conn);
    CleanupStack::PopAndDestroy(&socketServer);
    ClearTestFiles();
    return iTestStepResult;
    }
    


CQoSTest_408::CQoSTest_408()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText48, "qos_408");
    iTestStepName = KText48;
    }

CQoSTest_408::~CQoSTest_408()
    {
    }


TVerdict CQoSTest_408::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText, "qos_408 Configuring default context to UMTS R5 context ");
    Log(KText);
    
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn1; 
    RSocket                 socket1;
    TInetAddr               dstAddr1;
    TRequestStatus          status;
    TInt                    ret; 
    
    
    // Connect to ESOCK 
    ret = socketServer.Connect();
    TESTL(ret == KErrNone); 
    CleanupClosePushL(socketServer);

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 
    CleanupClosePushL(conn);

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP socket
    ret = socket1.Open(socketServer, KAfInet, KSockDatagram, KProtocolInetUdp, conn);
    TESTL(ret == KErrNone);
    CleanupClosePushL(socket1);

    dstAddr1.SetPort(3461);
    _LIT(KText315, "10.1.46.1");
    dstAddr1.Input( KText315 );

    socket1.Connect(dstAddr1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 
    
    // create file when the default secondary is created
    TEST(NifAPIL(socket1, KNotifySecondaryCreated) == KErrNone);

    // open a new sub-connection 
    ret = subconn1.Open(socketServer, RSubConnection::ECreateNew, conn); 
    TESTL(ret == KErrNone);
    CleanupClosePushL(subconn1);
    
    // Subscribe for QoS event notification
    TRequestStatus  eventStatusForAdd;
    TNotificationEventBuf   subconnNotifBufForAdd;
    subconn1.EventNotification(subconnNotifBufForAdd, ETrue, eventStatusForAdd); 
        {
	    // Move the connected socket to the new sub-connection
	    // This will create a new secondary context
	    subconn1.Add(socket1, status);
	    User::WaitForRequest(status);
	    TESTL(status.Int() == KErrNone);
	    
	    // Receive & Process the subconn.SetParameters-event 
	    // (expected KSubConGenericEventParamsGranted)
	    User::WaitForRequest(eventStatusForAdd); 
	    TESTL(eventStatusForAdd.Int() == KErrNone);
	    ret = CheckEvent(subconnNotifBufForAdd, KSubConGenericEventParamsGranted); 
	    TESTE(ret == KErrNone, ret);
        }
	// unsubscribe for QoS event notification
	subconn1.CancelEventNotification();			    
    
    // Create the container for all subconnection parameters 
    RSubConParameterBundle parameterBundle;
    CleanupClosePushL(parameterBundle);

    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily = CSubConParameterFamily::NewL(parameterBundle, KSubConQoSFamily ); 
    
    // Create the requested/acceptable generic parameter set for QoS 
    // (Qos family takes ownership) 
    CSubConQosGenericParamSet* reqGenericParams = CSubConQosGenericParamSet::NewL(*qosFamily, CSubConParameterFamily::ERequested);
    CSubConQosGenericParamSet* accGenericParams = CSubConQosGenericParamSet::NewL(*qosFamily, CSubConParameterFamily::EAcceptable);
    
    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet1(*reqGenericParams);
    SetEsockParamSet1(*accGenericParams);
    
    CSubConQosR5ParamSet* requestedR5Qos = NULL;
    CSubConQosR5ParamSet* acceptableR5Qos = NULL;
    
    requestedR5Qos = CSubConQosR5ParamSet::NewL(*qosFamily, CSubConParameterFamily::ERequested);
    acceptableR5Qos = CSubConQosR5ParamSet::NewL(*qosFamily, CSubConParameterFamily::EAcceptable);
        
    SetUmtsR5QoSRequested1(*requestedR5Qos); 
    SetUmtsR5QoSAcceptable1(*acceptableR5Qos);
	
	// Subscribe for QoS event notification
    TRequestStatus  eventStatusForSetParameters;
    TNotificationEventBuf   subconnNotifBufForSetParameters;
    subconn1.EventNotification(subconnNotifBufForSetParameters, ETrue, eventStatusForSetParameters); 
        {
	    // Set properties of the sub-connection 
	    ret = subconn1.SetParameters(parameterBundle); 
	    TESTL(ret == KErrNone);

	    // Receive & Process the subconn.SetParameters-event 
	    // (expected KSubConGenericEventParamsGranted)
	    User::WaitForRequest(eventStatusForSetParameters); 
	    TESTL(eventStatusForSetParameters.Int() == KErrNone);
	    ret = CheckEvent(subconnNotifBufForSetParameters, KSubConGenericEventParamsGranted); 
	    TESTE(ret == KErrNone, ret);
		if(ret == KErrNone)
			{
			// Extract granted UMTS R5 information from subconn
			ret = CheckUmtsR5ExtensionValuesL(subconn1, ETrue, RPacketQoS::ESourceStatisticsDescriptorSpeech);    
			TESTE(ret == KErrNone, ret);
			}
        }
	// Unsubscribe for QoS event notification
    subconn1.CancelEventNotification();
    
	
    // To reset the message values of the testnif
    TEST(NifAPIL(socket1, KResetMessage) == KErrNone);

    CleanupStack::Pop(&parameterBundle);
    // close and destroy
    CleanupStack::PopAndDestroy(&subconn1);
    CleanupStack::PopAndDestroy(&socket1);
    CleanupStack::PopAndDestroy(&conn);
    CleanupStack::PopAndDestroy(&socketServer);
    ClearTestFiles();
    return iTestStepResult;
    }
    
    
CQoSTest_409::CQoSTest_409()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText48, "qos_409");
    iTestStepName = KText48;
    }

CQoSTest_409::~CQoSTest_409()
    {
    }

TVerdict CQoSTest_409::doTestStepL( void )
    { 
    User::After(KInterTestDelay);
    _LIT(KText, "qos_409 Configuring existing R5 context to new R5 values");
    Log(KText);
    
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn1; 
    RSocket                 socket1;
    TInetAddr               dstAddr1;
    TRequestStatus          status;
    TInt                    ret; 
    
    
    // Connect to ESOCK 
    ret = socketServer.Connect();
    TESTL(ret == KErrNone); 
    CleanupClosePushL(socketServer);

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 
    CleanupClosePushL(conn);

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP socket
    ret = socket1.Open(socketServer, KAfInet, KSockDatagram, KProtocolInetUdp, conn);
    TESTL(ret == KErrNone);
    CleanupClosePushL(socket1);

    dstAddr1.SetPort(3461);
    _LIT(KText315, "10.1.46.1");
    dstAddr1.Input( KText315 );

    socket1.Connect(dstAddr1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 
    
    // create file when the default secondary is created
    TEST(NifAPIL(socket1, KNotifySecondaryCreated) == KErrNone);

    // open a new sub-connection 
    ret = subconn1.Open(socketServer, RSubConnection::ECreateNew, conn); 
    TESTL(ret == KErrNone);
    CleanupClosePushL(subconn1);

	// Subscribe for QoS event notification
    TRequestStatus  eventStatusForAdd;
    TNotificationEventBuf   subconnNotifBufForAdd;
    subconn1.EventNotification(subconnNotifBufForAdd, ETrue, eventStatusForAdd); 
        {    
        // Move the connected socket to the new sub-connection
	    // This will create a new secondary context
	    subconn1.Add(socket1, status);
	    User::WaitForRequest(status);
	    TESTL(status.Int() == KErrNone);
	    
	    // Receive & Process the subconn.SetParameters-event 
	    // (expected KSubConGenericEventParamsGranted)
	    User::WaitForRequest(eventStatusForAdd); 
	    TESTL(eventStatusForAdd.Int() == KErrNone);
	    ret = CheckEvent(subconnNotifBufForAdd, KSubConGenericEventParamsGranted); 
	    TESTE(ret == KErrNone, ret);
        }
    // unsubscribe for QoS event notification
	subconn1.CancelEventNotification();	
    
    // Create the container for all subconnection parameters 
    RSubConParameterBundle parameterBundle;
    CleanupClosePushL(parameterBundle);

    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily = CSubConParameterFamily::NewL(parameterBundle, KSubConQoSFamily ); 
    
    // Create the requested/acceptable generic parameter set for QoS 
    // (Qos family takes ownership) 
    CSubConQosGenericParamSet* reqGenericParams = CSubConQosGenericParamSet::NewL(*qosFamily, CSubConParameterFamily::ERequested);
    CSubConQosGenericParamSet* accGenericParams = CSubConQosGenericParamSet::NewL(*qosFamily, CSubConParameterFamily::EAcceptable);
    
    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet1(*reqGenericParams);
    SetEsockParamSet1(*accGenericParams);
    
    CSubConQosR5ParamSet* requestedR5Qos = NULL;
    CSubConQosR5ParamSet* acceptableR5Qos = NULL;
    
    requestedR5Qos = CSubConQosR5ParamSet::NewL(*qosFamily, CSubConParameterFamily::ERequested);
    acceptableR5Qos = CSubConQosR5ParamSet::NewL(*qosFamily, CSubConParameterFamily::EAcceptable);
        
    SetUmtsR5QoSRequested1(*requestedR5Qos); 
    SetUmtsR5QoSAcceptable1(*acceptableR5Qos);   
    
    // Subscribe for QoS event notification
    TRequestStatus  eventStatusForSetParameters;
    TNotificationEventBuf   subconnNotifBufForSetParameters;
    subconn1.EventNotification(subconnNotifBufForSetParameters, ETrue, eventStatusForSetParameters); 
        {
	    // Set properties of the sub-connection 
	    ret = subconn1.SetParameters(parameterBundle); 
	    TESTL(ret == KErrNone);

	    // Receive & Process the subconn.SetParameters-event 
	    // (expected KSubConGenericEventParamsGranted)
	    User::WaitForRequest(eventStatusForSetParameters); 
	    TESTL(eventStatusForSetParameters.Int() == KErrNone);
	    ret = CheckEvent(subconnNotifBufForSetParameters, KSubConGenericEventParamsGranted); 
	    TESTE(ret == KErrNone, ret);
	    if(ret == KErrNone)
			{
			// Extract granted UMTS R5 information from subconn
			ret = CheckUmtsR5ExtensionValuesL(subconn1, ETrue, RPacketQoS::ESourceStatisticsDescriptorSpeech);    
			TESTE(ret == KErrNone, ret);
			}
        }
	// Unsubscribe for QoS event notification
    subconn1.CancelEventNotification();
    
    
    
    
    
    requestedR5Qos->SetSignallingIndicator(ETrue); 
    requestedR5Qos->SetSourceStatisticsDescriptor(RPacketQoS::ESourceStatisticsDescriptorSpeech);
    
    acceptableR5Qos->SetSignallingIndicator(ETrue); 
    acceptableR5Qos->SetSourceStatisticsDescriptor(RPacketQoS::ESourceStatisticsDescriptorUnknown);
    
    // Subscribe for QoS event notification
    TRequestStatus  eventStatusForSetParameters2;
    TNotificationEventBuf   subconnNotifBufForSetParameters2;
    subconn1.EventNotification(subconnNotifBufForSetParameters2, ETrue, eventStatusForSetParameters2); 
        {
	    // Set properties of the sub-connection 
	    ret = subconn1.SetParameters(parameterBundle); 
	    TESTL(ret == KErrNone);

	    // Receive & Process the subconn.SetParameters-event 
	    // (expected KSubConGenericEventParamsGranted)
	    User::WaitForRequest(eventStatusForSetParameters2); 
	    TESTL(eventStatusForSetParameters2.Int() == KErrNone);
	    ret = CheckEvent(subconnNotifBufForSetParameters2, KSubConGenericEventParamsGranted); 
	    TESTE(ret == KErrNone, ret);
		if(ret == KErrNone)
			{
			// Extract granted UMTS R5 information from subconn
			ret = CheckUmtsR5ExtensionValuesL(subconn1, ETrue, RPacketQoS::ESourceStatisticsDescriptorSpeech);    
			TESTE(ret == KErrNone, ret);
			}
        }
	// Unsubscribe for QoS event notification
    subconn1.CancelEventNotification();			

    // To reset the message values of the testnif
    TEST(NifAPIL(socket1, KResetMessage) == KErrNone);

    CleanupStack::Pop(&parameterBundle);
    // close and destroy
    CleanupStack::PopAndDestroy(&subconn1);
    CleanupStack::PopAndDestroy(&socket1);
    CleanupStack::PopAndDestroy(&conn);
    CleanupStack::PopAndDestroy(&socketServer);
    ClearTestFiles();
	return iTestStepResult;
    }
    

CQoSTest_410::CQoSTest_410()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText48, "qos_410");
    iTestStepName = KText48;
    }

CQoSTest_410::~CQoSTest_410()
    {
    }

TVerdict CQoSTest_410::doTestStepL( void )
    { 
    User::After(KInterTestDelay);
    _LIT(KText, "qos_410 Configuring existing R99 context to new R5 values");
    Log(KText);
    
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn1; 
    RSocket                 socket1;
    TInetAddr               dstAddr1;
    TRequestStatus          status;
    TInt                    ret; 
    
    
    
    // Connect to ESOCK 
    ret = socketServer.Connect();
    TESTL(ret == KErrNone); 
    CleanupClosePushL(socketServer);

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 
    CleanupClosePushL(conn);

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP socket
    ret = socket1.Open(socketServer, KAfInet, KSockDatagram, KProtocolInetUdp, conn);
    TESTL(ret == KErrNone);
    CleanupClosePushL(socket1);

    dstAddr1.SetPort(3461);
    _LIT(KText315, "10.1.46.1");
    dstAddr1.Input( KText315 );

    socket1.Connect(dstAddr1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    TEST(NifAPIL(socket1, KNotifySecondaryCreated) == KErrNone);

    // open a new sub-connection 
    ret = subconn1.Open(socketServer, RSubConnection::ECreateNew, conn); 
    TESTL(ret == KErrNone);
    CleanupClosePushL(subconn1);
	
    // Create the container for all subconnection parameters 
    RSubConParameterBundle parameterBundle;
    CleanupClosePushL(parameterBundle);

    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily = CSubConParameterFamily::NewL(parameterBundle, KSubConQoSFamily ); 
    
    // Create the requested/acceptable generic parameter set for QoS 
    // (Qos family takes ownership) 
    CSubConQosGenericParamSet* reqGenericParams = CSubConQosGenericParamSet::NewL(*qosFamily, CSubConParameterFamily::ERequested);
    CSubConQosGenericParamSet* accGenericParams = CSubConQosGenericParamSet::NewL(*qosFamily, CSubConParameterFamily::EAcceptable);
    
    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet1(*reqGenericParams);
    SetEsockParamSet1(*accGenericParams);
    
    // Create r99 parameter set for QoS 
    CSubConQosIPLinkR99ParamSet* reqIpLink99ParSet = CSubConQosIPLinkR99ParamSet::NewL(*qosFamily, CSubConParameterFamily::ERequested);
    CSubConQosIPLinkR99ParamSet* accIpLink99ParSet = CSubConQosIPLinkR99ParamSet::NewL(*qosFamily, CSubConParameterFamily::EAcceptable);
    
    // Set the requested/acceptable r99 parameter set 
    SetIPLinkR99HighDemand(*reqIpLink99ParSet);
    SetIPLinkR99HighDemandMin(*accIpLink99ParSet);

    
    // Set properties of the sub-connection 
    ret = subconn1.SetParameters(parameterBundle); 
    TESTL(ret == KErrNone);
    
    // Subscribe for QoS event notification
    TRequestStatus  eventStatusForAdd;
    TNotificationEventBuf   subconnNotifBufForAdd;
    subconn1.EventNotification(subconnNotifBufForAdd, ETrue, eventStatusForAdd); 
        {    
	    // Move the connected socket to the new sub-connection
	    // This will create a new secondary context
	    subconn1.Add(socket1, status);
	    User::WaitForRequest(status);
	    TESTL(status.Int() == KErrNone);
		
		// Receive & Process the subconn.SetParameters-event 
	    // (expected KSubConGenericEventParamsGranted)
	    User::WaitForRequest(eventStatusForAdd); 
	    TESTL(eventStatusForAdd.Int() == KErrNone);
	    ret = CheckEvent(subconnNotifBufForAdd, KSubConGenericEventParamsGranted); 
	    TESTE(ret == KErrNone, ret);
        }
    // unsubscribe for QoS event notification
	subconn1.CancelEventNotification();	
    
    RSubConParameterBundle parameterBundle1;
    CleanupClosePushL(parameterBundle1);
    
    CSubConParameterFamily* qosFamily1 = CSubConParameterFamily::NewL(parameterBundle1, KSubConQoSFamily ); 
    
    CSubConQosGenericParamSet* reqGenericParams1 = CSubConQosGenericParamSet::NewL(*qosFamily1, CSubConParameterFamily::ERequested);
    CSubConQosGenericParamSet* accGenericParams1 = CSubConQosGenericParamSet::NewL(*qosFamily1, CSubConParameterFamily::EAcceptable);
    
    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet1(*reqGenericParams1);
    SetEsockParamSet1(*accGenericParams1);
    
    
    CSubConQosR5ParamSet* requestedR5Qos1 = NULL;
    CSubConQosR5ParamSet* acceptableR5Qos1 = NULL;
    
    requestedR5Qos1 = CSubConQosR5ParamSet::NewL(*qosFamily1, CSubConParameterFamily::ERequested);
    acceptableR5Qos1 = CSubConQosR5ParamSet::NewL(*qosFamily1, CSubConParameterFamily::EAcceptable);
        
    SetUmtsR5QoSRequested1(*requestedR5Qos1); 
    SetUmtsR5QoSAcceptable1(*acceptableR5Qos1); 
    
    // Subscribe for QoS event notification
    TRequestStatus  eventStatusForSetParameters2;
    TNotificationEventBuf   subconnNotifBufForSetParameters2;
    subconn1.EventNotification(subconnNotifBufForSetParameters2, ETrue, eventStatusForSetParameters2); 
        {
	    // Set properties of the sub-connection 
	    ret = subconn1.SetParameters(parameterBundle1); 
	    TESTL(ret == KErrNone);

	    // Receive & Process the subconn.SetParameters-event 
	    // (expected KSubConGenericEventParamsGranted)
	    User::WaitForRequest(eventStatusForSetParameters2); 
	    TESTL(eventStatusForSetParameters2.Int() == KErrNone);
	    ret = CheckEvent(subconnNotifBufForSetParameters2, KSubConGenericEventParamsGranted); 
	    TESTE(ret == KErrNone, ret);  
	    if(ret == KErrNone)
			{
			// Extract granted UMTS R5 information from subconn
			ret = CheckUmtsR5ExtensionValuesL(subconn1, ETrue, RPacketQoS::ESourceStatisticsDescriptorSpeech);    
			TESTE(ret == KErrNone, ret);
			}
        }
    // Unsubscribe for QoS event notification
    subconn1.CancelEventNotification();	
        
    // To reset the message values of the testnif
    TEST(NifAPIL(socket1, KResetMessage) == KErrNone);

    CleanupStack::Pop(&parameterBundle1);
    CleanupStack::Pop(&parameterBundle);
    // close and destroy
    CleanupStack::PopAndDestroy(&subconn1);
    CleanupStack::PopAndDestroy(&socket1);
    CleanupStack::PopAndDestroy(&conn);
    CleanupStack::PopAndDestroy(&socketServer);
    ClearTestFiles();
    return iTestStepResult;
    }
    

    

CQoSTest_411::CQoSTest_411()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText48, "qos_411");
    iTestStepName = KText48;
    }

CQoSTest_411::~CQoSTest_411()
    {
    }

TVerdict CQoSTest_411::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText, "qos_411 Failure of configuration to UMTS R5 context");
    Log(KText);
    
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn1; 
    RSocket                 socket1;
    TInetAddr               dstAddr1;
    TRequestStatus          status;
    TInt                    ret; 
    
    
    // Connect to ESOCK 
    ret = socketServer.Connect();
    TESTL(ret == KErrNone); 
    CleanupClosePushL(socketServer);

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 
    CleanupClosePushL(conn);

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP socket
    ret = socket1.Open(socketServer, KAfInet, KSockDatagram, KProtocolInetUdp, conn);
    TESTL(ret == KErrNone);
    CleanupClosePushL(socket1);

    dstAddr1.SetPort(3461);
    _LIT(KText315, "10.1.46.1");
    dstAddr1.Input( KText315 );

    socket1.Connect(dstAddr1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 
    
    TEST(NifAPIL(socket1, KNotifySecondaryCreated) == KErrNone);

    // open a new sub-connection 
    ret = subconn1.Open(socketServer, RSubConnection::ECreateNew, conn); 
    TESTL(ret == KErrNone);
    CleanupClosePushL(subconn1);

	// Subscribe for QoS event notification
    TRequestStatus  eventStatusForAdd;
    TNotificationEventBuf   subconnNotifBufForAdd;
    subconn1.EventNotification(subconnNotifBufForAdd, ETrue, eventStatusForAdd); 
        {    
	    // Move the connected socket to the new sub-connection
	    // This will create a new secondary context
	    subconn1.Add(socket1, status);
	    User::WaitForRequest(status);
	    TESTL(status.Int() == KErrNone);
	    
	    // Receive & Process the subconn.SetParameters-event 
	    // (expected KSubConGenericEventParamsGranted)
	    User::WaitForRequest(eventStatusForAdd); 
	    TESTL(eventStatusForAdd.Int() == KErrNone);
	    ret = CheckEvent(subconnNotifBufForAdd, KSubConGenericEventParamsGranted); 
	    TESTE(ret == KErrNone, ret);
        }
	// unsubscribe for QoS event notification
	subconn1.CancelEventNotification();	

    // Create the container for all subconnection parameters 
    RSubConParameterBundle parameterBundle;
    CleanupClosePushL(parameterBundle);

    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily = CSubConParameterFamily::NewL(parameterBundle, KSubConQoSFamily ); 
    
    // Create the requested/acceptable generic parameter set for QoS 
    // (Qos family takes ownership) 
    CSubConQosGenericParamSet* reqGenericParams = CSubConQosGenericParamSet::NewL(*qosFamily, CSubConParameterFamily::ERequested);
    CSubConQosGenericParamSet* accGenericParams = CSubConQosGenericParamSet::NewL(*qosFamily, CSubConParameterFamily::EAcceptable);
    
    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet1(*reqGenericParams);
    SetEsockParamSet1(*accGenericParams);
    
    CSubConQosR5ParamSet* requestedR5Qos = NULL;
    CSubConQosR5ParamSet* acceptableR5Qos = NULL;
    
    requestedR5Qos = CSubConQosR5ParamSet::NewL(*qosFamily, CSubConParameterFamily::ERequested);
    acceptableR5Qos = CSubConQosR5ParamSet::NewL(*qosFamily, CSubConParameterFamily::EAcceptable);
        
    SetUmtsR5QoSRequested1(*requestedR5Qos); 
    SetUmtsR5QoSAcceptable1(*acceptableR5Qos);
    
    /* 	inform the testnif to fail the r5 context 
    *	modification. 
    */
    TEST(NifAPIL(socket1, KContextModifyActiveFailAsync) == KErrNone);
    
    // Subscribe for QoS event notification
    TRequestStatus  eventStatusForSetParameters;
    TNotificationEventBuf   subconnNotifBufForSetParameters;
    subconn1.EventNotification(subconnNotifBufForSetParameters, ETrue, eventStatusForSetParameters); 
        {
	    // Set properties of the sub-connection 
	    ret = subconn1.SetParameters(parameterBundle); 
	    TESTL(ret == KErrNone);

	    // Receive & Process the subconn.SetParameters-event 
	    // (expected KSubConGenericEventParamsRejected)
	    User::WaitForRequest(eventStatusForSetParameters); 
	    TESTL(eventStatusForSetParameters.Int() == KErrNone);
	    ret = CheckEvent(subconnNotifBufForSetParameters, KSubConGenericEventParamsRejected); 
	    TESTE(ret == KErrNone, ret);
 		}
	// unsubscribe for QoS event notification
	subconn1.CancelEventNotification();	

    // To reset the message values of the testnif
    TEST(NifAPIL(socket1, KResetMessage) == KErrNone);

    CleanupStack::Pop(&parameterBundle);
    
    // close and destroy
    CleanupStack::PopAndDestroy(&subconn1);
    CleanupStack::PopAndDestroy(&socket1);
    CleanupStack::PopAndDestroy(&conn);
    CleanupStack::PopAndDestroy(&socketServer);
    ClearTestFiles();
    return iTestStepResult;
    }


CQoSTest_412::CQoSTest_412()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText48, "qos_412");
    iTestStepName = KText48;
    }

CQoSTest_412::~CQoSTest_412()
    {
    }

TVerdict CQoSTest_412::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    /*
    This is testing the r5 context failure when SI flag is set.
    */_LIT(KText, "qos_412 Configure UMTS R5 secondary context with SI flag and different priority value");
    Log(KText);
    
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn1; 
    RSocket                 socket1;
    TInetAddr               dstAddr1;
    TRequestStatus          status;
    TInt                    ret; 
    
    
    
    // Connect to ESOCK 
    ret = socketServer.Connect();
    TESTL(ret == KErrNone); 
    CleanupClosePushL(socketServer);

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 
    CleanupClosePushL(conn);

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP socket
    ret = socket1.Open(socketServer, KAfInet, KSockDatagram, KProtocolInetUdp, conn);
    TESTL(ret == KErrNone);
    CleanupClosePushL(socket1);

    dstAddr1.SetPort(3461);
    _LIT(KText315, "10.1.46.1");
    dstAddr1.Input( KText315 );

    socket1.Connect(dstAddr1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 
    
    TEST(NifAPIL(socket1, KNotifySecondaryCreated) == KErrNone);

    // open a new sub-connection 
    ret = subconn1.Open(socketServer, RSubConnection::ECreateNew, conn); 
    TESTL(ret == KErrNone);
    CleanupClosePushL(subconn1);

    // Subscribe for QoS event notification
    TRequestStatus  eventStatusForAdd;
    TNotificationEventBuf   subconnNotifBufForAdd;
    subconn1.EventNotification(subconnNotifBufForAdd, ETrue, eventStatusForAdd); 
        {    
	    // Move the connected socket to the new sub-connection
	    // This will create a new secondary context
	    subconn1.Add(socket1, status);
	    User::WaitForRequest(status);
	    TESTL(status.Int() == KErrNone);
	    
	    // Receive & Process the subconn.SetParameters-event 
	    // (expected KSubConGenericEventParamsGranted)
	    User::WaitForRequest(eventStatusForAdd); 
	    TESTL(eventStatusForAdd.Int() == KErrNone);
	    ret = CheckEvent(subconnNotifBufForAdd, KSubConGenericEventParamsGranted); 
	    TESTE(ret == KErrNone, ret);
        }
	// unsubscribe for QoS event notification
	subconn1.CancelEventNotification();	

    // Create the container for all subconnection parameters 
    RSubConParameterBundle parameterBundle;
    CleanupClosePushL(parameterBundle);

    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily = CSubConParameterFamily::NewL(parameterBundle, KSubConQoSFamily ); 
    
    // Create the requested/acceptable generic parameter set for QoS 
    // (Qos family takes ownership) 
    CSubConQosGenericParamSet* reqGenericParams = CSubConQosGenericParamSet::NewL(*qosFamily, CSubConParameterFamily::ERequested);
    CSubConQosGenericParamSet* accGenericParams = CSubConQosGenericParamSet::NewL(*qosFamily, CSubConParameterFamily::EAcceptable);
    
    // Set the requested/acceptable Generic Parameters
    // set the priority value to other than 1.
    SetEsockParamSet1(*reqGenericParams);
    SetEsockParamSet1(*accGenericParams);
    
    CSubConQosR5ParamSet* requestedR5Qos = NULL;
    CSubConQosR5ParamSet* acceptableR5Qos = NULL;
    
    requestedR5Qos = CSubConQosR5ParamSet::NewL(*qosFamily, CSubConParameterFamily::ERequested);
    acceptableR5Qos = CSubConQosR5ParamSet::NewL(*qosFamily, CSubConParameterFamily::EAcceptable);
        
    SetUmtsR5QoSRequested1(*requestedR5Qos); 
    SetUmtsR5QoSAcceptable1(*acceptableR5Qos);   
    
    // Subscribe for QoS event notification
    TRequestStatus  eventStatusForSetParameters;
    TNotificationEventBuf   subconnNotifBufForSetParameters;
    subconn1.EventNotification(subconnNotifBufForSetParameters, ETrue, eventStatusForSetParameters); 
        {
		// Set properties of the sub-connection 
	    ret = subconn1.SetParameters(parameterBundle); 
	    TESTL(ret == KErrNone);

	    // Receive & Process the subconn.SetParameters-event 
	    // (expected KSubConGenericEventParamsGranted)
	    User::WaitForRequest(eventStatusForSetParameters); 
	    TESTL(eventStatusForSetParameters.Int() == KErrNone);
	    ret = CheckEvent(subconnNotifBufForSetParameters, KSubConGenericEventParamsGranted); 
	    TESTE(ret == KErrNone, ret);
		if(ret == KErrNone)
			{
			// Extract granted UMTS R5 information from subconn
			ret = CheckUmtsR5ExtensionValuesL(subconn1, ETrue, RPacketQoS::ESourceStatisticsDescriptorSpeech);    
			TESTE(ret == KErrNone, ret);
			}
        }
	// unsubscribe for QoS event notification
	subconn1.CancelEventNotification();	
    
    // check the umts r5 parameter values returned by the network
    RSubConParameterBundle bundle1;
    ret = subconn1.GetParameters(bundle1);
    TESTL(ret == KErrNone);
    CSubConParameterFamily* family1 = bundle1.FindFamily(KSubConQoSFamily);
    CSubConQosR5ParamSet* grantedR5Params = 
					(CSubConQosR5ParamSet*)family1->FindExtensionSet(STypeId(KSubCon3GPPExtParamsFactoryUid, 
															KSubConQosR5ParamsType),CSubConParameterFamily::EGranted);
    if(grantedR5Params)
        {
        TEST(grantedR5Params->GetTrafficClass() == RPacketQoS::ETrafficClassInteractive);
        TEST(grantedR5Params->GetTrafficHandlingPriority() == 0x02); //RPacketQoS::TTrafficHandlingPriority::ETrafficPriority1
        }
    else
    	{
    	Log(_L("CSubConQosR5ParamSet Extension not found in the returned event"));	
    	TEST(EFalse);
    	}
    
    // To reset the message values of the testnif
    TEST(NifAPIL(socket1, KResetMessage) == KErrNone);
    
    CleanupStack::Pop(&parameterBundle);
    // close and destroy
    CleanupStack::PopAndDestroy(&subconn1);
    CleanupStack::PopAndDestroy(&socket1);
    CleanupStack::PopAndDestroy(&conn);
    CleanupStack::PopAndDestroy(&socketServer);
    ClearTestFiles();
    return iTestStepResult;
    }


CQoSTest_413::CQoSTest_413()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText48, "qos_413");
    iTestStepName = KText48;
    }

CQoSTest_413::~CQoSTest_413()
    {
    }

TVerdict CQoSTest_413::doTestStepL( void )
    { 
    User::After(KInterTestDelay);
    _LIT(KText, "qos_413 Creating UMTS R5 context with IMS extension");
    Log(KText);
    
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn1; 
    RSocket                 socket1;
    TInetAddr               dstAddr1;
    TRequestStatus          status;
    TInt                    ret; 
    
    
    // Connect to ESOCK 
    ret = socketServer.Connect();
    CleanupClosePushL(socketServer);
    TESTL(ret == KErrNone); 

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 
    CleanupClosePushL(conn);

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP socket
    ret = socket1.Open(socketServer, KAfInet, KSockDatagram, KProtocolInetUdp, conn);
    TESTL(ret == KErrNone);
    CleanupClosePushL(socket1);

    dstAddr1.SetPort(3461);
    _LIT(KText315, "10.1.46.1");
    dstAddr1.Input( KText315 );

    socket1.Connect(dstAddr1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    // open a new sub-connection 
    ret = subconn1.Open(socketServer, RSubConnection::ECreateNew, conn); 
    TESTL(ret == KErrNone);
    CleanupClosePushL(subconn1);
	
	// Subscribe for QoS event notification
    TRequestStatus  eventStatusForAdd;
    TNotificationEventBuf   subconnNotifBufForAdd;
    subconn1.EventNotification(subconnNotifBufForAdd, ETrue, eventStatusForAdd); 
        {    
	    // Move the connected socket to the new sub-connection
	    // This will create a new secondary context
	    subconn1.Add(socket1, status);
	    User::WaitForRequest(status);
	    TESTL(status.Int() == KErrNone);
	    
	    // Receive & Process the subconn.SetParameters-event 
	    // (expected KSubConGenericEventParamsGranted)
	    User::WaitForRequest(eventStatusForAdd); 
	    TESTL(eventStatusForAdd.Int() == KErrNone);
	    ret = CheckEvent(subconnNotifBufForAdd, KSubConGenericEventParamsGranted); 
	    TESTE(ret == KErrNone, ret);
        }
	// unsubscribe for QoS event notification
	subconn1.CancelEventNotification();	


    // Create the container for all subconnection parameters 
    RSubConParameterBundle parameterBundle;
    CleanupClosePushL(parameterBundle);

    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily = CSubConParameterFamily::NewL(parameterBundle, KSubConQoSFamily ); 
    
    // Create the requested/acceptable generic parameter set for QoS 
    // (Qos family takes ownership) 
    CSubConQosGenericParamSet* reqGenericParams = CSubConQosGenericParamSet::NewL(*qosFamily, CSubConParameterFamily::ERequested);
    CSubConQosGenericParamSet* accGenericParams = CSubConQosGenericParamSet::NewL(*qosFamily, CSubConParameterFamily::EAcceptable);
    
    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet1(*reqGenericParams);
    SetEsockParamSet1(*accGenericParams);
    
    CSubConQosR5ParamSet* requestedR5Qos = NULL;
    CSubConQosR5ParamSet* acceptableR5Qos = NULL;
    
    requestedR5Qos = CSubConQosR5ParamSet::NewL(*qosFamily, CSubConParameterFamily::ERequested);
    acceptableR5Qos = CSubConQosR5ParamSet::NewL(*qosFamily, CSubConParameterFamily::EAcceptable);

    SetUmtsR5QoSRequested1(*requestedR5Qos); 
    SetUmtsR5QoSAcceptable1(*acceptableR5Qos);   
       
    // Subscribe for QoS event notification
    TRequestStatus  eventStatusForSetParameters;
    TNotificationEventBuf   subconnNotifBufForSetParameters;
    subconn1.EventNotification(subconnNotifBufForSetParameters, ETrue, eventStatusForSetParameters); 
        {
		// Set properties of the sub-connection 
	    ret = subconn1.SetParameters(parameterBundle); 
	    TESTL(ret == KErrNone);

	    // Receive & Process the subconn.SetParameters-event 
	    // (expected KSubConGenericEventParamsGranted)
	    User::WaitForRequest(eventStatusForSetParameters); 
	    TESTL(eventStatusForSetParameters.Int() == KErrNone);
	    ret = CheckEvent(subconnNotifBufForSetParameters, KSubConGenericEventParamsGranted); 
	    TESTE(ret == KErrNone, ret);
	    if(ret == KErrNone)
			{
			// Extract granted UMTS R5 information from subconn
			ret = CheckUmtsR5ExtensionValuesL(subconn1, ETrue, RPacketQoS::ESourceStatisticsDescriptorSpeech);    
			TESTE(ret == KErrNone, ret);
			}
        }
	// unsubscribe for QoS event notification
	subconn1.CancelEventNotification();				

    // Create the container for all subconnection parameters 
    RSubConParameterBundle parameterBundle1;
    CleanupClosePushL(parameterBundle1);

    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily1 = CSubConParameterFamily::NewL(parameterBundle1, KSubConQoSFamily ); 
    
    // Create the requested/acceptable generic parameter set for QoS 
    // (Qos family takes ownership) 
    CSubConQosGenericParamSet* reqGenericParams1 = CSubConQosGenericParamSet::NewL(*qosFamily1, CSubConParameterFamily::ERequested);
    CSubConQosGenericParamSet* accGenericParams1 = CSubConQosGenericParamSet::NewL(*qosFamily1, CSubConParameterFamily::EAcceptable);
    
    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet1(*reqGenericParams1);
    SetEsockParamSet1(*accGenericParams1);
    
    CSubConParameterFamily* qosImsFamily1 = CSubConParameterFamily::NewL(parameterBundle1, KSubConQoSFamily );
    
    // Create and Add IMS Extension Parameters
    CSubConIMSExtParamSet* imsExtnParamSet = NULL;
    imsExtnParamSet = CSubConIMSExtParamSet::NewL(*qosImsFamily1, CSubConParameterFamily::ERequested);
    imsExtnParamSet->SetIMSSignallingIndicator(ETrue); 
    
    // Subscribe for QoS event notification
    TRequestStatus  eventStatusForSetParameters2;
    TNotificationEventBuf   subconnNotifBufForSetParameters2;
    subconn1.EventNotification(subconnNotifBufForSetParameters2, ETrue, eventStatusForSetParameters2); 
        {    
        // Set properties of the sub-connection 
	    ret = subconn1.SetParameters(parameterBundle1); 
	    TESTL(ret == KErrNone);
    
	    // Receive & Process the subconn.SetParameters-event 
	    // (expected KSubConGenericEventParamsGranted)
	    User::WaitForRequest(eventStatusForSetParameters2); 
	    TESTL(eventStatusForSetParameters2.Int() == KErrNone);
	    ret = CheckEvent(subconnNotifBufForSetParameters2, KSubConGenericEventParamsGranted); 
	    TESTE(ret == KErrNone, ret);
		if(ret == KErrNone)
			{
			// Extract granted IMS information from subconn
			ret = CheckImsExtensionValueL(subconn1, ETrue);    
			TESTE(ret == KErrNone, ret);
			}
        }
	// unsubscribe for QoS event notification
	subconn1.CancelEventNotification();	
	 
    // To reset the message values of the testnif
    TEST(NifAPIL(socket1, KResetMessage) == KErrNone);

    CleanupStack::Pop(&parameterBundle1);    
    CleanupStack::Pop(&parameterBundle);
    // close and destroy
    CleanupStack::PopAndDestroy(&subconn1);
    CleanupStack::PopAndDestroy(&socket1);
    CleanupStack::PopAndDestroy(&conn);
    CleanupStack::PopAndDestroy(&socketServer);
    ClearTestFiles();
	return iTestStepResult;
    }
    

CQoSTest_414::CQoSTest_414()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText48, "qos_414");
    iTestStepName = KText48;
    }

CQoSTest_414::~CQoSTest_414()
    {
    }

TVerdict CQoSTest_414::doTestStepL( void )
    { 
    User::After(KInterTestDelay);
    _LIT(KText, "qos_414 Creation of UMTS R5 context in R5 unsupported network");
    Log(KText);
    
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn1; 
    RSocket                 socket1;
    TInetAddr               dstAddr1;
    TRequestStatus          status;
    TInt                    ret; 
    
    
    // Connect to ESOCK 
    ret = socketServer.Connect();
    TESTL(ret == KErrNone); 
    CleanupClosePushL(socketServer);

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 
    CleanupClosePushL(conn);

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP socket
    ret = socket1.Open(socketServer, KAfInet, KSockDatagram, KProtocolInetUdp, conn);
    TESTL(ret == KErrNone);
    CleanupClosePushL(socket1);

    dstAddr1.SetPort(3461);
    _LIT(KText315, "10.1.46.1");
    dstAddr1.Input( KText315 );

    socket1.Connect(dstAddr1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    // open a new sub-connection 
    ret = subconn1.Open(socketServer, RSubConnection::ECreateNew, conn); 
    TESTL(ret == KErrNone);
    CleanupClosePushL(subconn1);

    // Create the container for all subconnection parameters 
    RSubConParameterBundle parameterBundle;
    CleanupClosePushL(parameterBundle);

    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily = CSubConParameterFamily::NewL(parameterBundle, KSubConQoSFamily ); 
    
    // Create the requested/acceptable generic parameter set for QoS 
    // (Qos family takes ownership) 
    CSubConQosGenericParamSet* reqGenericParams = CSubConQosGenericParamSet::NewL(*qosFamily, CSubConParameterFamily::ERequested);
    CSubConQosGenericParamSet* accGenericParams = CSubConQosGenericParamSet::NewL(*qosFamily, CSubConParameterFamily::EAcceptable);
    
    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet1(*reqGenericParams);
    SetEsockParamSet1(*accGenericParams);
    
    CSubConQosR5ParamSet* requestedR5Qos = NULL;
    CSubConQosR5ParamSet* acceptableR5Qos = NULL;
    
    requestedR5Qos = CSubConQosR5ParamSet::NewL(*qosFamily, CSubConParameterFamily::ERequested);
    acceptableR5Qos = CSubConQosR5ParamSet::NewL(*qosFamily, CSubConParameterFamily::EAcceptable);

    SetUmtsR5QoSRequested1(*requestedR5Qos); 
    SetUmtsR5QoSAcceptable1(*acceptableR5Qos);   
       
    /*
    *	Inform the testnif to reset the Umts R5 values
    *
    *	This is what happens when the network doesn't support the  
    *	Umts R5 network
    */
    TEST(NifAPIL(socket1, KNetworkUnsupportedUmtsR5) == KErrNone);   
    
    	// Subscribe for QoS event notification
    TRequestStatus  eventStatusForAdd;
    TNotificationEventBuf   subconnNotifBufForAdd;
    subconn1.EventNotification(subconnNotifBufForAdd, ETrue, eventStatusForAdd); 
        {    
	    // Set properties of the sub-connection 
	    ret = subconn1.SetParameters(parameterBundle); 
	    TESTL(ret == KErrNone);

	    // Move the connected socket to the new sub-connection
	    // This will create a new secondary context
	    subconn1.Add(socket1, status);
	    User::WaitForRequest(status);
	    TESTL(status.Int() == KErrNone);
	    
	    // Receive & Process the subconn.SetParameters-event 
	    // (expected KSubConGenericEventParamsGranted)
	    User::WaitForRequest(eventStatusForAdd); 
	    TESTL(eventStatusForAdd.Int() == KErrNone);
	    ret = CheckEvent(subconnNotifBufForAdd, KSubConGenericEventParamsGranted); 
	    TESTE(ret == KErrNone, ret);
		if(ret == KErrNone)
			{
			// Extract granted UMTS R5 information from subconn
			ret = CheckUmtsR5ExtensionValuesL(subconn1, EFalse, RPacketQoS::ESourceStatisticsDescriptorUnknown);    
			TESTE(ret == KErrNone, ret);
			}
        }
    // unsubscribe for QoS event notification
	subconn1.CancelEventNotification();	
	 
    // To reset the message values of the testnif
    TEST(NifAPIL(socket1, KResetMessage) == KErrNone);

    CleanupStack::Pop(&parameterBundle);
    // close and destroy
    CleanupStack::PopAndDestroy(&subconn1);
    CleanupStack::PopAndDestroy(&socket1);
    CleanupStack::PopAndDestroy(&conn);
    CleanupStack::PopAndDestroy(&socketServer);
    ClearTestFiles();
    return iTestStepResult;
    }

#endif // SYMBIAN_NETWORKING_UMTSR5  
    
    

