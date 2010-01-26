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
#include "uscl_ip_subconparams.h"

//const TIp6Addr KInet6AddrMask = {{{0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, 
//                                   0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff}}};

const TInt KInterTestDelay = 40 * 300000;    // 12 sec


//
//  guqos module test cases
//

/* 
 * These methods are valid for Symbian OS 9.0 and onwards.
 */ 
 
 /*
 
CQoSTest_401::CQoSTest_401()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText313, "qos_401");
    iTestStepName = KText313;
    }


CQoSTest_401::~CQoSTest_401()
    {
    }

TVerdict CQoSTest_401::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText314, 
        "qos_401 SBLP, Basic SBLP test");
    Log( KText314 );
    
    RSocketServ             socketServer;
    RSocket                 socket1;
    TInetAddr               dstAddr1;
    RConnection             conn; 
    RSubConnection          subconn1; 
    TRequestStatus          status;
    
    // Connect to ESOCK 
    socketServer.Connect();

    // Open a connection 
    conn.Open(socketServer, KAfInet); 

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 

    // Open UDP sockets 
    socket1.Open(socketServer, 
                 KAfInet, 
                 KSockDatagram, 
                 KProtocolInetUdp, 
                 conn);

    dstAddr1.SetPort(3461);
    _LIT(KText315, "10.1.46.1");
    dstAddr1.Input( KText315 );
    socket1.Connect(dstAddr1, status);
    User::WaitForRequest(status);

    TEST(NifAPIL(socket1, KContextActivateFailAsync) == KErrNone);

    // Create the container for all subconnection parameters 
    RSubConParameterBundle sblpParams;
    CSubConParameterFamily* qosFamily = 
        CSubConParameterFamily::NewL(sblpParams, KSubConQoSFamily);
    CSubConQosGenericParamSet* reqGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::ERequested);
	SetEsockParamSet1(*reqGenericParams);
	
    CSubConParameterFamily* sblpFamily = 
        CSubConParameterFamily::NewL(sblpParams, KSubConAuthorisationFamily);

    // Create and Add SBLP Extension Parameters
    CSubConSBLPR5ExtensionParamSet* sblpExtnParamSet = 
        CSubConSBLPR5ExtensionParamSet::NewL(*sblpFamily, 
        CSubConParameterFamily::ERequested);

    sblpExtnParamSet->SetMAT(_S8("http://www.symbian.com")); 
    
    TFlowId flowId;
    TUint16 mcn = 100;
    TUint16 ifn = 1000;
    flowId.SetMediaComponentNumber(mcn);
    flowId.SetIPFlowNumber(ifn);
    
    sblpExtnParamSet->AddFlowIdL( flowId );

    // Create an instance of RSubConnection
    TEST(subconn1.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn) == KErrNone); 

    // Set properties of the sub-connection 
    TEST(subconn1.SetParameters(sblpParams) == KErrNone); 

    // Move the connected socket onto the new sub-connection
    subconn1.Add(socket1, status);
    // Wait for socket to be added 
    User::WaitForRequest(status);
    TESTE(status == KErrNone, status.Int());

    // This checks that secondary context number one was created
    TEST(CheckTestFile(KTestFile1));

    // Clean up
    subconn1.Close();
    sblpParams.Close(); 

    // To reset the message values of the testnif
    TEST(NifAPIL(socket1, KResetMessage) == KErrNone);

    socket1.Close();
    
    conn.Stop();
    conn.Close();

    socketServer.Close();

    ClearTestFiles();

    return iTestStepResult;
    }

CQoSTest_402::CQoSTest_402()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText314, "qos_402");
    iTestStepName = KText314;
    }


CQoSTest_402::~CQoSTest_402()
    {
    }

TVerdict CQoSTest_402::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText315,"qos_405 sample test");
    Log( KText315 );
    
    RSocketServ             socketServer;
    RSocket                 socket1;
    TInetAddr               dstAddr1;
    RConnection             conn; 
    RSubConnection          subconn1; 
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf;
    
    dstAddr1.SetPort(3461);
    _LIT(KText316, "10.1.46.1");
    dstAddr1.Input( KText316 );
    
    socketServer.Connect();
    conn.Open(socketServer, KAfInet); 
    conn.Start(status); 
    User::WaitForRequest(status); 
    
    socket1.Open(socketServer, KAfInet, KSockDatagram, KProtocolInetUdp, conn);
    socket1.Connect(dstAddr1, status);
    User::WaitForRequest(status);
    
    TEST(subconn1.Open(socketServer, RSubConnection::ECreateNew, conn) == KErrNone); 
    subconn1.Add(socket1, status);
    User::WaitForRequest(status);

	RSubConParameterBundle paramBundle;
	CSubConParameterFamily* paramFamily;
	paramFamily = CSubConParameterFamily::NewL (paramBundle, KSubConQoSFamily);

	CSubConQosGenericParamSet* requestedQoS;
	requestedQoS = CSubConQosGenericParamSet::NewL (*paramFamily, CSubConParameterFamily::ERequested);
	SetEsockParamSet1(*requestedQoS);

    subconn1.EventNotification(subconnNotifBuf, ETrue, eventStatus); 
    TEST(subconn1.SetParameters(paramBundle) == KErrNone); 
  	User::WaitForRequest(eventStatus); 
    TESTL(eventStatus.Int() == KErrNone);
    TESTL(CheckEvent(subconnNotifBuf, KSubConGenericEventParamsGranted) == KErrNone); 
    
    TEST(SendData(socket1) == KErrNone);
    TEST(SendData(socket1) == KErrNone);
    
	
	subconn1.CancelEventNotification();
    subconn1.Close();
    
    //socket1.Close();
    conn.Stop();
    conn.Close();
    socketServer.Close();
    delete paramFamily;
    
    ClearTestFiles();
    return iTestStepResult;
    }*/


CQoSTest_500::CQoSTest_500()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText500, "qos_500");
    iTestStepName = KText500;
    }


CQoSTest_500::~CQoSTest_500()
    {
    }

TVerdict CQoSTest_500::doTestStepL( void )
    {
// testing ims extn 
#ifdef AAA  
     
    /*
    User::After(KInterTestDelay);
    _LIT(KText501, 
        "qos_500 r5 paramters test");
    Log( KText501 );
    
    RSocketServ             socketServer;
    RSocket                 socket1;
    TInetAddr               dstAddr1;
    RConnection             conn; 
    RSubConnection          subconn1; 
    TRequestStatus          status;
    TNotificationEventBuf   subconnNotifBuf;
    
    socketServer.Connect();
	conn.Open(socketServer, KAfInet); 
	conn.Start(status); 
    User::WaitForRequest(status); 

	socket1.Open(socketServer, KAfInet, KSockDatagram, KProtocolInetUdp, conn);
    dstAddr1.SetPort(3461);
    _LIT(KText502, "10.1.46.1");
    dstAddr1.Input( KText502 );
    socket1.Connect(dstAddr1, status);
    User::WaitForRequest(status);

    TEST(subconn1.Open(socketServer, RSubConnection::ECreateNew, conn) == KErrNone); 

	// Move the connected socket onto the new sub-connection. The default qos parameters
	// are used for the channel
    subconn1.Add(socket1, status);
    User::WaitForRequest(status);

	RSubConParameterBundle paramBundle;
	CSubConParameterFamily* paramFamily;
	paramFamily = CSubConParameterFamily::NewL (paramBundle, KSubConQoSFamily);

	CSubConQosGenericParamSet* requestedQoS;

	requestedQoS = CSubConQosGenericParamSet::NewL (*paramFamily, CSubConParameterFamily::ERequested);
	SetEsockParamSet1(*requestedQoS);

    RSubConnection::TEventFilter subconnEventFilter(KSubConnGenericEventsImplUid,KSubConGenericEventParamsGranted    | 
                                 KSubConGenericEventParamsChanged    | KSubConGenericEventParamsRejected   | 
                                 KSubConGenericEventDataClientJoined | KSubConGenericEventDataClientLeft   | 
                                 KSubConGenericEventSubConDown );
	// Event notification is enabled                                 
    subconn1.EventNotification(subconnNotifBuf, &subconnEventFilter, 1, status); 

	// The qos channel is configured for new qos parameters
    TEST(subconn1.SetParameters(paramBundle) == KErrNone); 
    User::WaitForRequest(status);
    TESTE(status == KErrNone, status.Int());
    // Confirmation 
    TEST(CheckEvent(subconnNotifBuf, KSubConGenericEventParamsGranted) == KErrNone);
        
    TEST(SendData(socket1) == KErrNone);
    TEST(SendData(socket1) == KErrNone);
    
	subconn1.CancelEventNotification();
	
	subconn1.Close();
    socket1.Close();
    
    conn.Stop();
    conn.Close();
	socketServer.Close();
	ClearTestFiles();
*/


    /*
        Trying to test the extensions by using RQosPolicy API
    */
    User::After(KInterTestDelay);
    _LIT(KText195, "qos_500 Testing extensions using RQoSPolicy");
    Log( KText195 );

    RSocketServ socketServer;
    RSocket socket;
    TInetAddr dstAddr;
    TRequestStatus status;
    socketServer.Connect();
    socket.Open(socketServer, KAfInet, KSockDatagram, KProtocolInetUdp);
    dstAddr.SetPort(7);
    dstAddr.Input(_L("172.21.8.245"));
    socket.Connect(dstAddr, status);
    User::WaitForRequest(status);
    
    TQoSSelector selector;
    selector.SetAddr(socket);
    RQoSPolicy policy;
    policy.Open(selector);
    
    
    /*
    for r5 parameters
    */
    /*
    TUmtsR5QoSParameters umtsReqParameters, umtsMinParameters;
    SetUmtsR5QoSBackground(umtsReqParameters);
    SetUmtsR5QoSBackgroundMin(umtsMinParameters);
    
    
    CUmtsR5QoSPolicy *umtsR5Policy = CUmtsR5QoSPolicy::NewL();
    
    umtsR5Policy->SetQoSRequested(umtsReqParameters);
    umtsR5Policy->SetQoSMinimum(umtsMinParameters);
    
    TEST(umtsR5Policy->SetHeaderCompression(KPdpHeaderCompression) == KErrNone);

    CQoSParameters *parameters = new (ELeave) CQoSParameters;
    SetQoSParamSet2(*parameters);
    TEST(parameters->AddExtensionL(*umtsR5Policy) == KErrNone);
    */
    
    
    /*
    for sblp parameters
    */
    /*
    TSblpParameters sblpParam;
    sblpParam.SetMAT(_S8("http://www.symbian.com//test_case_700//")); 
    
    RArray<TSblpParameters::TFlowIdentifier> flowIds;
    
    TSblpParameters::TFlowIdentifier flowId;
    flowId.iMediaComponentNumber = 100;
    flowId.iIPFlowNumber = 1000;
    flowIds.Append(flowId);
    
    TSblpParameters::TFlowIdentifier flowId2;
    flowId2.iMediaComponentNumber = 101;
    flowId2.iIPFlowNumber = 1001;
    flowIds.Append(flowId2);
    
    sblpParam.SetFlowIds(flowIds);
    CSblpPolicy *sblpPolicy = CSblpPolicy::NewL();
    sblpPolicy->SetSblpParameters(sblpParam);
    
    CQoSParameters *parameters = new (ELeave) CQoSParameters;
    SetQoSParamSet2(*parameters);
    TEST(parameters->AddExtensionL(*sblpPolicy) == KErrNone);
    */
    
    /*
    for ims parameters
    */
    
    
    TImsParameter ims;
    ims.SetIMSSigallingIndicator(ETrue); 
    
    CImsPolicy *imsPolicy = CImsPolicy::NewL();
    imsPolicy->SetImsParameter(ims);
    
    CQoSParameters *parameters = new (ELeave) CQoSParameters;
    SetQoSParamSet2(*parameters);
    TEST(parameters->AddExtensionL(*imsPolicy) == KErrNone);
    
    
    
    /*
    for r99 parameters
    */
    /*
    TUmtsQoSParameters umtsReqParameters, umtsMinParameters;
    SetUmtsQoSBackground(umtsReqParameters);
    SetUmtsQoSBackgroundMin(umtsMinParameters);
    
    
    CUmtsQoSPolicy *umtsPolicy = CUmtsQoSPolicy::NewL();
    
    umtsPolicy->SetQoSRequested(umtsReqParameters);
    umtsPolicy->SetQoSMinimum(umtsMinParameters);
    
    TEST(umtsPolicy->SetHeaderCompression(KPdpHeaderCompression) == KErrNone);

    CQoSParameters *parameters = new (ELeave) CQoSParameters;
    SetQoSParamSet2(*parameters);
    TEST(parameters->AddExtensionL(*umtsPolicy) == KErrNone);
    */
       
    policy.NotifyEvent(*this);
    TEST(policy.SetQoS(*parameters) == KErrNone);
    WaitForQoSEventL();
    TESTE(iReceivedEvent == EQoSEventAddPolicy, iReceivedEvent);
    TESTE(iReason == KErrNone, iReason);
    
    WaitForQoSEventL();
    TESTE(iReceivedEvent == EQoSEventConfirm, iReceivedEvent);
    
    TEST(SendData(socket) == KErrNone);
    TEST(SendData(socket) == KErrNone);
    
    TEST(policy.CancelNotifyEvent(*this) == KErrNone);
    
    
    TEST(policy.Close() == KErrNone);
    delete parameters;
#endif // AAA
    return iTestStepResult;
    }


CQoSTest_501::CQoSTest_501()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText501, "qos_501");
    iTestStepName = KText501;
    }


CQoSTest_501::~CQoSTest_501()
    {
    }

TVerdict CQoSTest_501::doTestStepL( void )
    {
    /*
        This test case tries to check that the r99 extension is set correctly using 
        RQosPolicy API. This is another way of bypassing the RSubConnection API to 
        check the extension parameters!
    */

    /*
        Trying to test the extensions by using RQosPolicy API
    */
    User::After(KInterTestDelay);
    _LIT(KText195, "qos_501 Testing added extensions using RQoSPolicy");
    Log( KText195 );

    RSocketServ socketServer;
    RSocket socket;
    TInetAddr dstAddr;
    TRequestStatus status;
    socketServer.Connect();
    socket.Open(socketServer, KAfInet, KSockDatagram, KProtocolInetUdp);
    dstAddr.SetPort(7);
    dstAddr.Input(_L("172.21.8.245"));
    socket.Connect(dstAddr, status);
    User::WaitForRequest(status);
    
    TQoSSelector selector;
    selector.SetAddr(socket);
    RQoSPolicy policy;
    policy.Open(selector);
    
    
    /*
    for r99 parameters
    */
    
    TUmtsQoSParameters umtsReqParameters, umtsMinParameters;
    SetUmtsQoSBackground(umtsReqParameters);
    SetUmtsQoSBackgroundMin(umtsMinParameters);
    
    
    CUmtsQoSPolicy *umtsPolicy = CUmtsQoSPolicy::NewL();
    
    umtsPolicy->SetQoSRequested(umtsReqParameters);
    umtsPolicy->SetQoSMinimum(umtsMinParameters);
    
    TEST(umtsPolicy->SetHeaderCompression(KPdpHeaderCompression) == KErrNone);

    CQoSParameters *parameters = new (ELeave) CQoSParameters;
    SetQoSParamSet2(*parameters);
    TEST(parameters->AddExtensionL(*umtsPolicy) == KErrNone);
    
       
    policy.NotifyEvent(*this);
    TEST(policy.SetQoS(*parameters) == KErrNone);
    WaitForQoSEventL();
    TESTE(iReceivedEvent == EQoSEventAddPolicy, iReceivedEvent);
    TESTE(iReason == KErrNone, iReason);
    
    WaitForQoSEventL();
    TESTE(iReceivedEvent == EQoSEventConfirm, iReceivedEvent);
    
    TEST(SendData(socket) == KErrNone);
    TEST(SendData(socket) == KErrNone);
    
   
     /*
        check the r99 extension parameters
    */
    CExtensionBase* extn;
    extn = iParameters->FindExtension(KPfqosExtensionUmts);
    TUmtsQoSParameters requestedParameters;
    ((CUmtsQoSPolicy*)extn)->GetQoSRequested(requestedParameters);
    
   
    TEST(requestedParameters.TrafficClass() == umtsReqParameters.TrafficClass());
    TEST(requestedParameters.DeliveryOrder() == umtsReqParameters.DeliveryOrder());
    TEST(requestedParameters.DeliveryOfErroneusSdu() == umtsReqParameters.DeliveryOfErroneusSdu());
    TEST(requestedParameters.ResidualBer() == umtsReqParameters.ResidualBer());
    TEST(requestedParameters.ErrorRatio() == umtsReqParameters.ErrorRatio());
    TEST(requestedParameters.Priority() == umtsReqParameters.Priority());
    TEST(requestedParameters.TransferDelay() == umtsReqParameters.TransferDelay());
    TEST(requestedParameters.MaxSduSize() == umtsReqParameters.MaxSduSize());
    TEST(requestedParameters.MaxBitrateUplink() == umtsReqParameters.MaxBitrateUplink());
    TEST(requestedParameters.MaxBitrateDownlink() == umtsReqParameters.MaxBitrateDownlink());
    TEST(requestedParameters.GuaranteedBitrateUplink() == umtsReqParameters.GuaranteedBitrateUplink());
    TEST(requestedParameters.GuaranteedBitrateDownlink() == umtsReqParameters.GuaranteedBitrateDownlink());
 
    
    
    TEST(policy.CancelNotifyEvent(*this) == KErrNone);
    
    
    TEST(policy.Close() == KErrNone);
    delete parameters;

    return iTestStepResult;
    }


