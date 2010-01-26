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
#include "uscl_Qos3GPP_subconparams.h"



const TInt KInterTestDelay = 40 * 300000;    // 12 sec


//
//  
//

CQoSTest_301::CQoSTest_301()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText, "qos_301");
    iTestStepName = KText;
    }

CQoSTest_301::~CQoSTest_301()
    {
    }


TVerdict CQoSTest_301::doTestStepL( void )
	{
	
	User::After(KInterTestDelay);
    _LIT(KText, "qos_301 Creation of dedicated IMS PDP context ");
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
    
    // Create and Add IMS Extension Parameters
    CSubConIMSExtParamSet* imsExtnParamSet = NULL;
    imsExtnParamSet = CSubConIMSExtParamSet::NewL(*qosFamily, CSubConParameterFamily::ERequested);
    imsExtnParamSet->SetIMSSignallingIndicator(ETrue); 
    
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
			// Extract granted IMS information from subconn
			ret = CheckImsExtensionValueL(subconn1, ETrue);    
			TESTE(ret == KErrNone, ret);
			}
        }
	// unsubscribe for QoS event notification
	subconn1.CancelEventNotification();		

    CleanupStack::Pop(&parameterBundle);
    // close and destroy
    CleanupStack::PopAndDestroy(&socket1);
    CleanupStack::PopAndDestroy(&subconn1);
    CleanupStack::PopAndDestroy(&conn);
    CleanupStack::PopAndDestroy(&socketServer);
    ClearTestFiles();
    return iTestStepResult;
    }
    

CQoSTest_302::CQoSTest_302()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText, "qos_302");
    iTestStepName = KText;
    }

CQoSTest_302::~CQoSTest_302()
    {
    }


TVerdict CQoSTest_302::doTestStepL( void )
	{
	User::After(KInterTestDelay);
    _LIT(KText, "qos_302 Failure of dedicated IMS PDP context creation ");
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
    
    // Create and Add IMS Extension Parameters
    CSubConIMSExtParamSet* imsExtnParamSet = NULL;
    imsExtnParamSet = CSubConIMSExtParamSet::NewL(*qosFamily, CSubConParameterFamily::ERequested);
    imsExtnParamSet->SetIMSSignallingIndicator(ETrue); 
    
    // inform the testnif to fail the activation of the context
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
    CleanupStack::PopAndDestroy(&socket1);
    CleanupStack::PopAndDestroy(&subconn1);
    CleanupStack::PopAndDestroy(&conn);
    CleanupStack::PopAndDestroy(&socketServer);
    ClearTestFiles();
    return iTestStepResult;
    }
    
CQoSTest_303::CQoSTest_303()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText, "qos_303");
    iTestStepName = KText;
    }

CQoSTest_303::~CQoSTest_303()
    {
    }

TVerdict CQoSTest_303::doTestStepL( void )
	{
	User::After(KInterTestDelay);
    _LIT(KText, "qos_303 Secondary context creation when IMS flag is reset");
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
    
    // Create and Add IMS Extension Parameters
    CSubConIMSExtParamSet* imsExtnParamSet = NULL;
    imsExtnParamSet = CSubConIMSExtParamSet::NewL(*qosFamily, CSubConParameterFamily::ERequested);
    imsExtnParamSet->SetIMSSignallingIndicator(EFalse); 
    
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
			// Extract granted IMS information from subconn
			ret = CheckImsExtensionValueL(subconn1, EFalse);    
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
    
CQoSTest_304::CQoSTest_304()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText, "qos_304");
    iTestStepName = KText;
    }

CQoSTest_304::~CQoSTest_304()
    {
    }

TVerdict CQoSTest_304::doTestStepL( void )
	{
    User::After(KInterTestDelay);
    _LIT(KText, "qos_304 Creation of dedicated IMS context in unsupported Network");
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
    
    // Create and Add IMS Extension Parameters
    CSubConIMSExtParamSet* imsExtnParamSet = NULL;
    imsExtnParamSet = CSubConIMSExtParamSet::NewL(*qosFamily, CSubConParameterFamily::ERequested);
    imsExtnParamSet->SetIMSSignallingIndicator(ETrue); 
    
   /*
    *	Inform the testnif to reset the ims flag
    *
    *	This is what happens when the network doesn't support the dedicated 
    *	pdp context for ims
    */
    TEST(NifAPIL(socket1, KNetworkUnsupportedIms) == KErrNone);
    
    
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
	    ret = CheckEvent(subconnNotifBufForAdd, KSubConGenericEventParamsGranted); 
	    TESTE(ret == KErrNone, ret);
	    if(ret == KErrNone)
			{
			// Extract granted IMS information from subconn
			ret = CheckImsExtensionValueL(subconn1, EFalse);    
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


CQoSTest_305::CQoSTest_305()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText, "qos_305");
    iTestStepName = KText;
    }

CQoSTest_305::~CQoSTest_305()
    {
    }


TVerdict CQoSTest_305::doTestStepL( void )
	{
	User::After(KInterTestDelay);
    _LIT(KText, "qos_305 Configuring normal secondary context to dedicated IMS context");
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
        
        User::WaitForRequest(eventStatusForAdd); 
        TESTL(eventStatusForAdd.Int() == KErrNone);
        ret = CheckEvent(subconnNotifBufForAdd, KSubConGenericEventParamsGranted); 
        TESTE(ret == KErrNone, ret);
        }
    // Unsubscribe for QoS event notification
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
    
    // Create and Add IMS Extension Parameters
    CSubConIMSExtParamSet* imsExtnParamSet = NULL;
    imsExtnParamSet = CSubConIMSExtParamSet::NewL(*qosFamily, CSubConParameterFamily::ERequested);
    imsExtnParamSet->SetIMSSignallingIndicator(ETrue); 
    
    
    
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
        	// Extract granted IMS information from subconn
        	ret = CheckImsExtensionValueL(subconn1, ETrue);    
        	TESTE(ret == KErrNone, ret);
        	}
        }
    // Unsubscribe for QoS event notification
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

CQoSTest_306::CQoSTest_306()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText48, "qos_306");
    iTestStepName = KText48;
    }

CQoSTest_306::~CQoSTest_306()
    {
    }

TVerdict CQoSTest_306::doTestStepL( void )
    {
   	User::After(KInterTestDelay);
    _LIT(KText, "qos_306 Failure of secondary context configuration to dedicated IMS context");
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
        
        User::WaitForRequest(eventStatusForAdd); 
        TESTL(eventStatusForAdd.Int() == KErrNone);
        ret = CheckEvent(subconnNotifBufForAdd, KSubConGenericEventParamsGranted); 
        TESTE(ret == KErrNone, ret);
        }
    // Unsubscribe for QoS event notification
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
    
    // Create and Add IMS Extension Parameters
    CSubConIMSExtParamSet* imsExtnParamSet = NULL;
    imsExtnParamSet = CSubConIMSExtParamSet::NewL(*qosFamily, CSubConParameterFamily::ERequested);
    imsExtnParamSet->SetIMSSignallingIndicator(ETrue); 
    
    // inform the testnif to fail the modification of the active context
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
	// Unsubscribe for QoS event notification
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

CQoSTest_307::CQoSTest_307()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText48, "qos_307");
    iTestStepName = KText48;
    }

CQoSTest_307::~CQoSTest_307()
    {
    }

TVerdict CQoSTest_307::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText, "qos_307 Secondary context configuration when IMS flag is reset");
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
        
        User::WaitForRequest(eventStatusForAdd); 
        TESTL(eventStatusForAdd.Int() == KErrNone);
        ret = CheckEvent(subconnNotifBufForAdd, KSubConGenericEventParamsGranted); 
        TESTE(ret == KErrNone, ret);
        }
    // Unsubscribe for QoS event notification
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
    
    // Create and Add IMS Extension Parameters
    CSubConIMSExtParamSet* imsExtnParamSet = NULL;
    imsExtnParamSet = CSubConIMSExtParamSet::NewL(*qosFamily, CSubConParameterFamily::ERequested);
    imsExtnParamSet->SetIMSSignallingIndicator(EFalse); 
    
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
			// Extract granted IMS information from subconn
			ret = CheckImsExtensionValueL(subconn1, EFalse);    
			TESTE(ret == KErrNone, ret);
			}
        }
	// Unsubscribe for QoS event notification
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

#endif //SYMBIAN_NETWORKING_UMTSR5  
