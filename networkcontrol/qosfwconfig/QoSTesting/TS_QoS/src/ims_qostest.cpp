// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// guqostest.cpp
//

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
//#include <ip_subconparams.h>
#include "uscl_sblp.h"



const TInt KInterTestDelay = 40 * 300000;    // 12 sec


//
//  
//


CQoSTest_300::CQoSTest_300()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText, "qos_300");
    iTestStepName = KText;
    }

CQoSTest_300::~CQoSTest_300()
    {
    }


TVerdict CQoSTest_300::doTestStepL( void )
	{
	User::After(KInterTestDelay);
    _LIT(KText, "qos_300 configuring dedicated pdp context for IMS");
    Log(KText);
    
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn1; 
    RSocket                 socket1;
    TInetAddr               dstAddr1;
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf;
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

    TEST(NifAPIL(socket1, KNotifySecondaryCreated) == KErrNone);

    // open a new sub-connection 
    
    ret = subconn1.Open(socketServer, RSubConnection::ECreateNew, conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket to the new sub-connection
    // This will create a new secondary context
    subconn1.Add(socket1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

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
    
    CSubConParameterFamily* qosImsFamily = CSubConParameterFamily::NewL(parameterBundle, KSubConQoSFamily );
    
    // Create and Add IMS Extension Parameters
    CSubConIMSExtParamSet* imsExtnParamSet = NULL;
    TRAPD(err1, imsExtnParamSet = CSubConIMSExtParamSet::NewL(*qosImsFamily, CSubConParameterFamily::ERequested));
    if (err1 != KErrNone)
        {
        Log(_L("CSubConIMSExtParamSet::NewL error: %d"), err1);
        // throw the exception to be handled by the test framework
        User::LeaveIfError(err1); 
        }
    imsExtnParamSet->SetIMSSignallingIndicator(ETrue); 
    
    // Subscribe for QoS Params notification
    subconn1.EventNotification(subconnNotifBuf, ETrue, eventStatus); 

    // Set properties of the sub-connection 
    ret = subconn1.SetParameters(parameterBundle); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn.SetParameters-event 
    // (expected KSubConGenericEventParamsGranted)
    User::WaitForRequest(eventStatus); 
    TESTL(eventStatus.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf, KSubConGenericEventParamsGranted); 
    TESTL(ret == KErrNone);

    // This checks that secondary context number one was created
    TEST(CheckTestFile(KTestFile1));
    
    // Check that the created secondary context was modified to
    // ims dedicated context
    TEST(CheckTestFile(KTestIMS));
    

    
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

CQoSTest_301::CQoSTest_301()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText48, "qos_301");
    iTestStepName = KText48;
    }

CQoSTest_301::~CQoSTest_301()
    {
    }


TVerdict CQoSTest_301::doTestStepL( void )
    {
   	User::After(KInterTestDelay);
    _LIT(KText, "qos_301 configuring dedicated pdp context for IMS - failure case");
    Log(KText);
    
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn1; 
    RSocket                 socket1;
    TInetAddr               dstAddr1;
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf;
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

    TEST(NifAPIL(socket1, KNotifySecondaryCreated) == KErrNone);

    // open a new sub-connection 
    
    ret = subconn1.Open(socketServer, RSubConnection::ECreateNew, conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket to the new sub-connection
    // This will create a new secondary context
    subconn1.Add(socket1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

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
    
    CSubConParameterFamily* qosImsFamily = CSubConParameterFamily::NewL(parameterBundle, KSubConQoSFamily );
    
    // Create and Add IMS Extension Parameters
    CSubConIMSExtParamSet* imsExtnParamSet = NULL;
    TRAPD(err1, imsExtnParamSet = CSubConIMSExtParamSet::NewL(*qosImsFamily, CSubConParameterFamily::ERequested));
    if (err1 != KErrNone)
        {
        Log(_L("CSubConIMSExtParamSet::NewL error: %d"), err1);
        // throw the exception to be handled by the test framework
        User::LeaveIfError(err1); 
        }
    imsExtnParamSet->SetIMSSignallingIndicator(ETrue); 
    
    // inform the testnif to fail the modification of the active context
    TEST(NifAPIL(socket1, KContextModifyActiveFailAsync) == KErrNone);
    
    // Subscribe for QoS Params notification
    subconn1.EventNotification(subconnNotifBuf, ETrue, eventStatus); 

    // Set properties of the sub-connection 
    ret = subconn1.SetParameters(parameterBundle); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn.SetParameters-event 
    // (expected KSubConGenericEventParamsRejected)
    User::WaitForRequest(eventStatus); 
    TESTL(eventStatus.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf, KSubConGenericEventParamsRejected); 
    TESTL(ret == KErrNone);

    // This checks that secondary context number one was created
    TEST(CheckTestFile(KTestFile1));
    
    // Check that ims parameter has NOT been added to the 
    // current context
    TEST(!CheckTestFile(KTestIMS));
    

    
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

CQoSTest_302::CQoSTest_302()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText48, "qos_302");
    iTestStepName = KText48;
    }

CQoSTest_302::~CQoSTest_302()
    {
    }


TVerdict CQoSTest_302::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText, "qos_302 Secondary context configuration when IMS flag is reset");
    Log(KText);
    
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn1; 
    RSocket                 socket1;
    TInetAddr               dstAddr1;
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf;
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

    TEST(NifAPIL(socket1, KNotifySecondaryCreated) == KErrNone);

    // open a new sub-connection 
    
    ret = subconn1.Open(socketServer, RSubConnection::ECreateNew, conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket to the new sub-connection
    // This will create a new secondary context
    subconn1.Add(socket1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

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
    
    CSubConParameterFamily* qosImsFamily = CSubConParameterFamily::NewL(parameterBundle, KSubConQoSFamily );
    
    // Create and Add IMS Extension Parameters
    CSubConIMSExtParamSet* imsExtnParamSet = NULL;
    TRAPD(err1, imsExtnParamSet = CSubConIMSExtParamSet::NewL(*qosImsFamily, CSubConParameterFamily::ERequested));
    if (err1 != KErrNone)
        {
        Log(_L("CSubConIMSExtParamSet::NewL error: %d"), err1);
        // throw the exception to be handled by the test framework
        User::LeaveIfError(err1); 
        }
    imsExtnParamSet->SetIMSSignallingIndicator(EFalse); 
    
    
    // Subscribe for QoS Params notification
    subconn1.EventNotification(subconnNotifBuf, ETrue, eventStatus); 

    // Set properties of the sub-connection 
    ret = subconn1.SetParameters(parameterBundle); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn.SetParameters-event 
    // (expected KSubConGenericEventParamsGranted)
    User::WaitForRequest(eventStatus); 
    TESTL(eventStatus.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf, KSubConGenericEventParamsGranted); 
    TESTL(ret == KErrNone);

    // This checks that secondary context number one was created
    TEST(CheckTestFile(KTestFile1));
    
    // Check that ims parameter has NOT been added to the 
    // current context
    TEST(!CheckTestFile(KTestIMS));
    

    
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
    _LIT(KText48, "qos_303");
    iTestStepName = KText48;
    }

CQoSTest_303::~CQoSTest_303()
    {
    }


TVerdict CQoSTest_303::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText, "qos_303 Network doesn't support the dedicated IMS context");
    Log(KText);
    
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn1; 
    RSocket                 socket1;
    TInetAddr               dstAddr1;
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf;
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

    TEST(NifAPIL(socket1, KNotifySecondaryCreated) == KErrNone);

    // open a new sub-connection 
    
    ret = subconn1.Open(socketServer, RSubConnection::ECreateNew, conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket to the new sub-connection
    // This will create a new secondary context
    subconn1.Add(socket1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

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
    
    CSubConParameterFamily* qosImsFamily = CSubConParameterFamily::NewL(parameterBundle, KSubConQoSFamily );
    
    // Create and Add IMS Extension Parameters
    CSubConIMSExtParamSet* imsExtnParamSet = NULL;
    TRAPD(err1, imsExtnParamSet = CSubConIMSExtParamSet::NewL(*qosImsFamily, CSubConParameterFamily::ERequested));
    if (err1 != KErrNone)
        {
        Log(_L("CSubConIMSExtParamSet::NewL error: %d"), err1);
        // throw the exception to be handled by the test framework
        User::LeaveIfError(err1); 
        }
    imsExtnParamSet->SetIMSSignallingIndicator(ETrue); 
    
   /*
    *	Inform the testnif to reset the ims flag
    *
    *	This is what happens when the network doesn't support the dedicated 
    *	pdp context for ims
    */
    TEST(NifAPIL(socket1, KNetworkUnsupportedIms) == KErrNone);
    
    
    // Subscribe for QoS Params notification
    subconn1.EventNotification(subconnNotifBuf, ETrue, eventStatus); 

    // Set properties of the sub-connection 
    ret = subconn1.SetParameters(parameterBundle); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn.SetParameters-event 
    // (expected KSubConGenericEventParamsRejected)
    User::WaitForRequest(eventStatus); 
    TESTL(eventStatus.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf, KSubConGenericEventParamsRejected); 
    TESTL(ret == KErrNone);
    
    /*
    what kind of error guqos will send???!
    */

    // This checks that secondary context number one was created
    TEST(CheckTestFile(KTestFile1));
    
    // Check that ims parameter has NOT been added to the 
    // current context
    TEST(!CheckTestFile(KTestIMS));
    
    // check the ims parameter value returned by the network
    RSubConParameterBundle bundle1;
    ret = subconn1.GetParameters(bundle1);
    TESTL(ret == KErrNone);
    
    CSubConParameterFamily* family1 = bundle1.FindFamily(KSubConQoSFamily);
    
    
	CSubConIMSExtParamSet* grantedImsParams = 
     	(CSubConIMSExtParamSet*)family1->FindExtensionSet(TUid::Uid(KSubConIMSExtParamsType),
     	CSubConParameterFamily::EGranted);

    if(grantedImsParams)
        {
        TEST(grantedImsParams->GetIMSSignallingIndicator() == EFalse);
        }
    else
    	{
    	Log(_L("CSubConIMSExtParamSet not found in the returned event"));	
    	TEST(EFalse);
    	}


    
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

CQoSTest_400::CQoSTest_400()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText48, "qos_400");
    iTestStepName = KText48;
    }

CQoSTest_400::~CQoSTest_400()
    {
    }


TVerdict CQoSTest_400::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText, "qos_400 Secondary context with UMTS R5 parameters - successful case");
    Log(KText);
    
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn1; 
    RSocket                 socket1;
    TInetAddr               dstAddr1;
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf;
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

    TEST(NifAPIL(socket1, KNotifySecondaryCreated) == KErrNone);

    // open a new sub-connection 
    
    ret = subconn1.Open(socketServer, RSubConnection::ECreateNew, conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket to the new sub-connection
    // This will create a new secondary context
    subconn1.Add(socket1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

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
    
    TRAPD(err2, requestedR5Qos = CSubConQosR5ParamSet::NewL(*qosFamily, CSubConParameterFamily::ERequested));
    if (err2 != KErrNone)
        {
        Log(_L("CSubConQosR5ParamSet::NewL error: %d"), err2);
        // throw the exception to be handled by the test framework
        User::LeaveIfError(err2); 
        }
	TRAPD(err3, acceptableR5Qos = CSubConQosR5ParamSet::NewL(*qosFamily, CSubConParameterFamily::EAcceptable));
    if (err3 != KErrNone)
        {
        Log(_L("CSubConQosR5ParamSet::NewL error: %d"), err3);
        // throw the exception to be handled by the test framework
        User::LeaveIfError(err3); 
        }        
        
        
    requestedR5Qos->SetSignallingIndicator(ETrue); 
    requestedR5Qos->SetSourceStatisticsDescriptor(RPacketQoS::ESourceStatisticsDescriptorSpeech);
    
    acceptableR5Qos->SetSignallingIndicator(ETrue); 
    acceptableR5Qos->SetSourceStatisticsDescriptor(RPacketQoS::ESourceStatisticsDescriptorSpeech);
       
    
    // Subscribe for QoS Params notification
    subconn1.EventNotification(subconnNotifBuf, ETrue, eventStatus); 

    // Set properties of the sub-connection 
    ret = subconn1.SetParameters(parameterBundle); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn.SetParameters-event 
    // (expected KSubConGenericEventParamsGranted)
    User::WaitForRequest(eventStatus); 
    TESTL(eventStatus.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf, KSubConGenericEventParamsGranted); 
    TESTL(ret == KErrNone);

    // This checks that secondary context number one was created
    TEST(CheckTestFile(KTestFile1));
    
    // Check that umtsr5 parameter has been added to the 
    // current context
    TEST(CheckTestFile(KTestUmtsR5));
    

    
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
    _LIT(KText, "qos_401 Secondary context with UMTS R5 parameters - general failure case");
    Log(KText);
    
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn1; 
    RSocket                 socket1;
    TInetAddr               dstAddr1;
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf;
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

    TEST(NifAPIL(socket1, KNotifySecondaryCreated) == KErrNone);

    // open a new sub-connection 
    
    ret = subconn1.Open(socketServer, RSubConnection::ECreateNew, conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket to the new sub-connection
    // This will create a new secondary context
    subconn1.Add(socket1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

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
    
    TRAPD(err2, requestedR5Qos = CSubConQosR5ParamSet::NewL(*qosFamily, CSubConParameterFamily::ERequested));
    if (err2 != KErrNone)
        {
        Log(_L("CSubConQosR5ParamSet::NewL error: %d"), err2);
        // throw the exception to be handled by the test framework
        User::LeaveIfError(err2); 
        }
	TRAPD(err3, acceptableR5Qos = CSubConQosR5ParamSet::NewL(*qosFamily, CSubConParameterFamily::EAcceptable));
    if (err3 != KErrNone)
        {
        Log(_L("CSubConQosR5ParamSet::NewL error: %d"), err3);
        // throw the exception to be handled by the test framework
        User::LeaveIfError(err3); 
        }        
        
        
    requestedR5Qos->SetSignallingIndicator(ETrue); 
    requestedR5Qos->SetSourceStatisticsDescriptor(RPacketQoS::ESourceStatisticsDescriptorSpeech);
    
    acceptableR5Qos->SetSignallingIndicator(ETrue); 
    acceptableR5Qos->SetSourceStatisticsDescriptor(RPacketQoS::ESourceStatisticsDescriptorSpeech);
    
    /* 	inform the testnif to fail the r5 context 
    *	modification. 
    *
    *
    */
    TEST(NifAPIL(socket1, KContextModifyActiveFailAsync) == KErrNone);
       
    
    // Subscribe for QoS Params notification
    subconn1.EventNotification(subconnNotifBuf, ETrue, eventStatus); 

    // Set properties of the sub-connection 
    ret = subconn1.SetParameters(parameterBundle); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn.SetParameters-event 
    // (expected KSubConGenericEventParamsRejected)
    User::WaitForRequest(eventStatus); 
    TESTL(eventStatus.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf, KSubConGenericEventParamsRejected); 
    TESTL(ret == KErrNone);

    // This checks that secondary context number one was created
    TEST(CheckTestFile(KTestFile1));
    
    // Check that umtsr5 parameter has NOT been added to the 
    // current context
    TEST(!CheckTestFile(KTestUmtsR5));
    

    
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
    _LIT(KText, "qos_402 New secondary context with UMTS R5 parameters and SI flag set");
    Log(KText);
    
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn1; 
    RSocket                 socket1;
    TInetAddr               dstAddr1;
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf;
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

    TEST(NifAPIL(socket1, KNotifySecondaryCreated) == KErrNone);

    // open a new sub-connection 
    
    ret = subconn1.Open(socketServer, RSubConnection::ECreateNew, conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket to the new sub-connection
    // This will create a new secondary context
    subconn1.Add(socket1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

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
    
    TRAPD(err2, requestedR5Qos = CSubConQosR5ParamSet::NewL(*qosFamily, CSubConParameterFamily::ERequested));
    if (err2 != KErrNone)
        {
        Log(_L("CSubConQosR5ParamSet::NewL error: %d"), err2);
        // throw the exception to be handled by the test framework
        User::LeaveIfError(err2); 
        }
	TRAPD(err3, acceptableR5Qos = CSubConQosR5ParamSet::NewL(*qosFamily, CSubConParameterFamily::EAcceptable));
    if (err3 != KErrNone)
        {
        Log(_L("CSubConQosR5ParamSet::NewL error: %d"), err3);
        // throw the exception to be handled by the test framework
        User::LeaveIfError(err3); 
        }        
    
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
       
    
    // Subscribe for QoS Params notification
    subconn1.EventNotification(subconnNotifBuf, ETrue, eventStatus); 

    // Set properties of the sub-connection 
    ret = subconn1.SetParameters(parameterBundle); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn.SetParameters-event 
    // (expected KSubConGenericEventParamsGranted)
    User::WaitForRequest(eventStatus); 
    TESTL(eventStatus.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf, KSubConGenericEventParamsGranted); 
    TESTL(ret == KErrNone);

    // This checks that secondary context number one was created
    TEST(CheckTestFile(KTestFile1));
    
    // Check that umtsr5 parameter has been added to the 
    // current context
    TEST(CheckTestFile(KTestUmtsR5));
    
    
    // check the umts r5 parameter values returned by the network
    RSubConParameterBundle bundle1;
    ret = subconn1.GetParameters(bundle1);
    TESTL(ret == KErrNone);
    
    CSubConParameterFamily* family1 = bundle1.FindFamily(KSubConQoSFamily);
    
	CSubConQosR5ParamSet* grantedR5Params = 
     	(CSubConQosR5ParamSet*)family1->FindExtensionSet(TUid::Uid(KSubConQosR5ParamsType),
     	CSubConParameterFamily::EGranted);

    if(grantedR5Params)
        {
        TEST(grantedR5Params->GetTrafficClass() == RPacketQoS::ETrafficClassInteractive);
        TEST(grantedR5Params->GetTrafficHandlingPriority() == 0x02); //RPacketQoS::TTrafficHandlingPriority::ETrafficPriority1
        }
    else
    	{
    	Log(_L("CSubConQosR5ParamSet not found in the returned event"));	
    	TEST(EFalse);
    	}


    
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
    /*
    This is testing the r5 context failure when SI flag is set.
    */_LIT(KText, "qos_403 New UMTS R5 secondary context with SI flag and different priority value");
    Log(KText);
    
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn1; 
    RSocket                 socket1;
    TInetAddr               dstAddr1;
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf;
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

    TEST(NifAPIL(socket1, KNotifySecondaryCreated) == KErrNone);

    // open a new sub-connection 
    
    ret = subconn1.Open(socketServer, RSubConnection::ECreateNew, conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket to the new sub-connection
    // This will create a new secondary context
    subconn1.Add(socket1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

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
    
    TRAPD(err2, requestedR5Qos = CSubConQosR5ParamSet::NewL(*qosFamily, CSubConParameterFamily::ERequested));
    if (err2 != KErrNone)
        {
        Log(_L("CSubConQosR5ParamSet::NewL error: %d"), err2);
        // throw the exception to be handled by the test framework
        User::LeaveIfError(err2); 
        }
	TRAPD(err3, acceptableR5Qos = CSubConQosR5ParamSet::NewL(*qosFamily, CSubConParameterFamily::EAcceptable));
    if (err3 != KErrNone)
        {
        Log(_L("CSubConQosR5ParamSet::NewL error: %d"), err3);
        // throw the exception to be handled by the test framework
        User::LeaveIfError(err3); 
        }        
    
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
       
    
    // Subscribe for QoS Params notification
    subconn1.EventNotification(subconnNotifBuf, ETrue, eventStatus); 

    // Set properties of the sub-connection 
    ret = subconn1.SetParameters(parameterBundle); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn.SetParameters-event 
    // (expected KSubConGenericEventParamsGranted)
    User::WaitForRequest(eventStatus); 
    TESTL(eventStatus.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf, KSubConGenericEventParamsGranted); 
    TESTL(ret == KErrNone);

    // This checks that secondary context number one was created
    TEST(CheckTestFile(KTestFile1));
    
    // Check that umtsr5 parameter has been added to the 
    // current context
    TEST(CheckTestFile(KTestUmtsR5));
    
    
    // check the umts r5 parameter values returned by the network
    RSubConParameterBundle bundle1;
    ret = subconn1.GetParameters(bundle1);
    TESTL(ret == KErrNone);
    
    CSubConParameterFamily* family1 = bundle1.FindFamily(KSubConQoSFamily);
    
	CSubConQosR5ParamSet* grantedR5Params = 
     	(CSubConQosR5ParamSet*)family1->FindExtensionSet(TUid::Uid(KSubConQosR5ParamsType),
     	CSubConParameterFamily::EGranted);

    if(grantedR5Params)
        {
        TEST(grantedR5Params->GetTrafficClass() == RPacketQoS::ETrafficClassInteractive);
        TEST(grantedR5Params->GetTrafficHandlingPriority() == 0x02); //RPacketQoS::TTrafficHandlingPriority::ETrafficPriority1
        }
    else
    	{
    	Log(_L("CSubConQosR5ParamSet not found in the returned event"));	
    	TEST(EFalse);
    	}


    
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
    _LIT(KText, "qos_404 Failure of UMTS R5 context when SI flag is set");
    Log(KText);
    
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn1; 
    RSocket                 socket1;
    TInetAddr               dstAddr1;
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf;
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

    TEST(NifAPIL(socket1, KNotifySecondaryCreated) == KErrNone);

    // open a new sub-connection 
    
    ret = subconn1.Open(socketServer, RSubConnection::ECreateNew, conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket to the new sub-connection
    // This will create a new secondary context
    subconn1.Add(socket1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

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
    
    TRAPD(err2, requestedR5Qos = CSubConQosR5ParamSet::NewL(*qosFamily, CSubConParameterFamily::ERequested));
    if (err2 != KErrNone)
        {
        Log(_L("CSubConQosR5ParamSet::NewL error: %d"), err2);
        // throw the exception to be handled by the test framework
        User::LeaveIfError(err2); 
        }
	TRAPD(err3, acceptableR5Qos = CSubConQosR5ParamSet::NewL(*qosFamily, CSubConParameterFamily::EAcceptable));
    if (err3 != KErrNone)
        {
        Log(_L("CSubConQosR5ParamSet::NewL error: %d"), err3);
        // throw the exception to be handled by the test framework
        User::LeaveIfError(err3); 
        }        
        
        
    requestedR5Qos->SetSignallingIndicator(ETrue); 
    requestedR5Qos->SetSourceStatisticsDescriptor(RPacketQoS::ESourceStatisticsDescriptorSpeech);
    
    acceptableR5Qos->SetSignallingIndicator(ETrue); 
    acceptableR5Qos->SetSourceStatisticsDescriptor(RPacketQoS::ESourceStatisticsDescriptorSpeech);
    
    /* 	inform the testnif to fail the r5 context 
    *	modification. 
    *
    *
    */
    TEST(NifAPIL(socket1, KContextModifyActiveFailAsync) == KErrNone);
       
    
    // Subscribe for QoS Params notification
    subconn1.EventNotification(subconnNotifBuf, ETrue, eventStatus); 

    // Set properties of the sub-connection 
    ret = subconn1.SetParameters(parameterBundle); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn.SetParameters-event 
    // (expected KSubConGenericEventParamsRejected)
    User::WaitForRequest(eventStatus); 
    TESTL(eventStatus.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf, KSubConGenericEventParamsRejected); 
    TESTL(ret == KErrNone);

    // This checks that secondary context number one was created
    TEST(CheckTestFile(KTestFile1));
    
    // Check that umtsr5 parameter has NOT been added to the 
    // current context
    TEST(!CheckTestFile(KTestUmtsR5));
    

    
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
    _LIT(KText, "qos_405 : Network downgrades UMTS R5 parameters when SI flag is set");
    Log(KText);
    
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn1; 
    RSocket                 socket1;
    TInetAddr               dstAddr1;
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf;
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

    TEST(NifAPIL(socket1, KNotifySecondaryCreated) == KErrNone);

    // open a new sub-connection 
    
    ret = subconn1.Open(socketServer, RSubConnection::ECreateNew, conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket to the new sub-connection
    // This will create a new secondary context
    subconn1.Add(socket1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

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
    
    TRAPD(err2, requestedR5Qos = CSubConQosR5ParamSet::NewL(*qosFamily, CSubConParameterFamily::ERequested));
    if (err2 != KErrNone)
        {
        Log(_L("CSubConQosR5ParamSet::NewL error: %d"), err2);
        // throw the exception to be handled by the test framework
        User::LeaveIfError(err2); 
        }
	TRAPD(err3, acceptableR5Qos = CSubConQosR5ParamSet::NewL(*qosFamily, CSubConParameterFamily::EAcceptable));
    if (err3 != KErrNone)
        {
        Log(_L("CSubConQosR5ParamSet::NewL error: %d"), err3);
        // throw the exception to be handled by the test framework
        User::LeaveIfError(err3); 
        }        
        
        
    requestedR5Qos->SetSignallingIndicator(ETrue); 
    requestedR5Qos->SetSourceStatisticsDescriptor(RPacketQoS::ESourceStatisticsDescriptorSpeech);
    
    acceptableR5Qos->SetSignallingIndicator(ETrue); 
    acceptableR5Qos->SetSourceStatisticsDescriptor(RPacketQoS::ESourceStatisticsDescriptorSpeech);
       
    
    // Subscribe for QoS Params notification
    subconn1.EventNotification(subconnNotifBuf, ETrue, eventStatus); 

    // Set properties of the sub-connection 
    ret = subconn1.SetParameters(parameterBundle); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn.SetParameters-event 
    // (expected KSubConGenericEventParamsGranted)
    User::WaitForRequest(eventStatus); 
    TESTL(eventStatus.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf, KSubConGenericEventParamsGranted); 
    TESTL(ret == KErrNone);

    // This checks that secondary context number one was created
    TEST(CheckTestFile(KTestFile1));
    
    // Check that umtsr5 parameter has been added to the 
    // current context
    TEST(CheckTestFile(KTestUmtsR5));
    
    
    
   /*
	*	inform testnif to downgrade the r5 values
	*
	*
	*/   
    
    TEST(NifAPIL(socket1, KNetworkDowngradeR5) == KErrNone);
    
    /*
    *
    *	How application is informed about the downgrading???
    	to do:
    *
    */
    

    
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
    
    

