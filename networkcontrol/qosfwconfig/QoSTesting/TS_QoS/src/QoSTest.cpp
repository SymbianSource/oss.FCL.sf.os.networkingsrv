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

const TIp6Addr KInet6AddrMask = {{{0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, 
                                   0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff}}};

const TInt KInterTestDelay = 40 * 300000;    // 12 sec


//
//  CQoSParameters test cases
//

/* 
 * These methods are valid for Symbian OS 9.0 and onwards.
 */ 
CQoSTest_001::CQoSTest_001()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText9, "qos_001");
    iTestStepName = KText9;
    }

CQoSTest_001::~CQoSTest_001()
    {
    }

TVerdict CQoSTest_001::doTestStepL( void )
    {
    _LIT(KText10, "qos_001: CQoSParametes constructor behaviour");
    Log( KText10 );

    CQoSParameters *parameters = new (ELeave) CQoSParameters;
    TEST(!(parameters->AdaptMode() & KPfqosOptionCanAdapt));
    TEST(parameters->GetDownLinkDelay() == 0);
    TEST(parameters->GetUpLinkDelay() == 0);
    TEST(parameters->GetDownLinkMaximumPacketSize() == 0);
    TEST(parameters->GetUpLinkMaximumPacketSize() == 0);
    TEST(parameters->GetDownLinkAveragePacketSize() == 0);
    TEST(parameters->GetUpLinkAveragePacketSize() == 0);
    TEST(parameters->GetName().Length() == 0);
    TEST(parameters->GetDownLinkPriority() == KQoSLowestPriority);
    TEST(parameters->GetUpLinkPriority() == KQoSLowestPriority);
    TEST(parameters->GetDownLinkMaximumBurstSize() == 0);
    TEST(parameters->GetUpLinkMaximumBurstSize() == 0);
    TEST(parameters->GetDownlinkBandwidth() == 0);
    TEST(parameters->GetUplinkBandwidth() == 0);

    delete parameters;
    return iTestStepResult;
    }


CQoSTest_002::CQoSTest_002()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText11, "qos_002");
    iTestStepName = KText11;
    }

CQoSTest_002::~CQoSTest_002()
    {
    }

TVerdict CQoSTest_002::doTestStepL( void )
    {
    _LIT(KText12, "qos_002: CQoSParameters, Setting parameter values that \
differ from the default ones");
    Log( KText12 );

    _LIT(KName,"TestName");
    TName flowspecName( KName );
    TBool adaptMode = ETrue;
    TInt priorityDownlink = 2;
    TInt priorityUplink = 3;
    TInt delayDownlink = 4;
    TInt delayUplink = 5;
    TInt maxPacketSizeDownlink = 8;
    TInt maxPacketSizeUplink = 9;
    TInt minPolicedUnitDownlink = 12;
    TInt minPolicedUnitUplink = 13;
    TInt tokenBucketSizeDownlink = 14;
    TInt tokenBucketSizeUplink = 15;
    TInt tokenRateDownlink = 16;
    TInt tokenRateUplink = 17;

    CQoSParameters *parameters = new (ELeave) CQoSParameters;

    parameters->SetAdaptMode(adaptMode);
    parameters->SetDownLinkDelay(delayDownlink);
    parameters->SetUpLinkDelay(delayUplink);
    parameters->SetDownLinkMaximumPacketSize(maxPacketSizeDownlink);
    parameters->SetUpLinkMaximumPacketSize(maxPacketSizeUplink);
    parameters->SetDownLinkAveragePacketSize(minPolicedUnitDownlink);
    parameters->SetUpLinkAveragePacketSize(minPolicedUnitUplink);
    parameters->SetName(flowspecName);
    parameters->SetDownLinkPriority(priorityDownlink);
    parameters->SetUpLinkPriority(priorityUplink);
    parameters->SetDownLinkMaximumBurstSize(tokenBucketSizeDownlink);
    parameters->SetUpLinkMaximumBurstSize(tokenBucketSizeUplink);
    parameters->SetDownlinkBandwidth(tokenRateDownlink);
    parameters->SetUplinkBandwidth(tokenRateUplink);


    TEST(parameters->AdaptMode() == adaptMode);
    TEST(parameters->GetDownLinkDelay() == delayDownlink);
    TEST(parameters->GetUpLinkDelay() == delayUplink);
    TEST(parameters->GetDownLinkMaximumPacketSize() == maxPacketSizeDownlink);
    TEST(parameters->GetUpLinkMaximumPacketSize() == maxPacketSizeUplink);
    TEST(parameters->GetDownLinkAveragePacketSize() == 
         minPolicedUnitDownlink);
    TEST(parameters->GetUpLinkAveragePacketSize() == minPolicedUnitUplink);
    TEST(parameters->GetName() == flowspecName);
    TEST(parameters->GetDownLinkPriority() == priorityDownlink);
    TEST(parameters->GetUpLinkPriority() == priorityUplink);
    TEST(parameters->GetDownLinkMaximumBurstSize() == 
         tokenBucketSizeDownlink);
    TEST(parameters->GetUpLinkMaximumBurstSize() == tokenBucketSizeUplink);
    TEST(parameters->GetDownlinkBandwidth() == tokenRateDownlink);
    TEST(parameters->GetUplinkBandwidth() == tokenRateUplink);

    delete parameters;
    return iTestStepResult;
    }



CQoSTest_003::CQoSTest_003()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText13, "qos_003");
    iTestStepName = KText13;
    }

CQoSTest_003::~CQoSTest_003()
    {
    }

TVerdict CQoSTest_003::doTestStepL( void )
    {
    _LIT(KText14, "qos_003: CQoSParameters, CopyL");
    Log( KText14 );

    CQoSParameters *parameters = new (ELeave) CQoSParameters;
    CQoSParameters *parameters2 = new (ELeave) CQoSParameters;

    SetQoSParamSet2(*parameters);
    parameters2->CopyL(*parameters);

    TEST(parameters2->AdaptMode() == parameters->AdaptMode());
    TEST(parameters2->GetDownLinkDelay() == parameters->GetDownLinkDelay());
    TEST(parameters2->GetUpLinkDelay() == parameters->GetUpLinkDelay());
    TEST(parameters2->GetDownLinkMaximumPacketSize() == 
         parameters->GetDownLinkMaximumPacketSize());
    TEST(parameters2->GetUpLinkMaximumPacketSize() == 
         parameters->GetUpLinkMaximumPacketSize());
    TEST(parameters2->GetDownLinkAveragePacketSize() == 
         parameters->GetDownLinkAveragePacketSize());
    TEST(parameters2->GetUpLinkAveragePacketSize() == 
         parameters->GetUpLinkAveragePacketSize());
    TEST(parameters2->GetName() == parameters->GetName());
    TEST(parameters2->GetDownLinkPriority() == 
         parameters->GetDownLinkPriority());
    TEST(parameters2->GetUpLinkPriority() == parameters->GetUpLinkPriority());
    TEST(parameters2->GetDownLinkMaximumBurstSize() == 
         parameters->GetDownLinkMaximumBurstSize());
    TEST(parameters2->GetUpLinkMaximumBurstSize() == 
         parameters->GetUpLinkMaximumBurstSize());
    TEST(parameters2->GetDownlinkBandwidth() == 
         parameters->GetDownlinkBandwidth());
    TEST(parameters2->GetUplinkBandwidth() == 
         parameters->GetUplinkBandwidth());

    delete parameters;
    delete parameters2;
    
    return iTestStepResult;
    }


CQoSTest_120::CQoSTest_120()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText41, "qos_120");
    iTestStepName = KText41;
    }

CQoSTest_120::~CQoSTest_120()
    {
    }

TVerdict CQoSTest_120::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText42, "qos_120 RSubConnection, Open()");
    Log( KText42 );

    RSocketServ     socketServer;
    RConnection     conn; 
    RSubConnection  subconn; 
    TRequestStatus  status;
    TInt            ret; 
    
    // Connect to ESOCK 
    ret = socketServer.Connect();
    CleanupClosePushL(socketServer); 
    TESTL(ret == KErrNone); 

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);
    
    // Create a new sub-connection 
    ret = subconn.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    TESTL(ret == KErrNone);

    subconn.Close();
    conn.Close();

    CleanupStack::Pop(&socketServer); 
    socketServer.Close();

    return iTestStepResult;
    }

CQoSTest_121::CQoSTest_121()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText44, "qos_121");
    iTestStepName = KText44;
    }

CQoSTest_121::~CQoSTest_121()
    {
    }

TVerdict CQoSTest_121::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText45, "qos_121 RSubConnection, Open(), Call Open() twice");
    Log( KText45 );

    RSocketServ     socketServer;
    RConnection     conn; 
    RSubConnection  subconn1, subconn2; 
    TRequestStatus  status;
    TInt            ret; 

    // Connect to ESOCK 
    ret = socketServer.Connect();
    CleanupClosePushL(socketServer); 
    TESTL(ret == KErrNone); 

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Create a new sub-connection 
    ret = subconn1.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    TESTL(ret == KErrNone);

    // Create one more sub-connection 
    ret = subconn2.Open(socketServer, 
                        RSubConnection::ECreateNew, 
                        conn); 
    TESTL(ret == KErrNone);

    subconn1.Close();
    subconn2.Close();
    conn.Close();

    CleanupStack::Pop(&socketServer); 
    socketServer.Close();

    return iTestStepResult;
    }


CQoSTest_122::CQoSTest_122()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText48, "qos_122");
    iTestStepName = KText48;
    }

CQoSTest_122::~CQoSTest_122()
    {
    }

TVerdict CQoSTest_122::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText49, "qos_122 RSubConnection, SetParameters");
    Log( KText49 );

    RSocket                 socket;
    TInetAddr               dstAddr;
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn; 
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf;
    TInt                    ret; 

    // Connect to ESOCK 
    ret = socketServer.Connect();
    CleanupClosePushL(socketServer); 
    TESTL(ret == KErrNone); 

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP socket 
    ret = socket.Open(socketServer, 
                      KAfInet, 
                      KSockDatagram, 
                      KProtocolInetUdp, 
                      conn);
    TESTL(ret == KErrNone); 

    dstAddr.SetPort(3221);
    _LIT(KText50, "10.1.22.1");
    dstAddr.Input( KText50 );

    ret = socket.SetLocalPort(3332);
    TESTL(ret == KErrNone); 

    socket.Connect(dstAddr, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    // Create a new sub-connection 
    ret = subconn.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket onto the new sub-connection
    subconn.Add(socket, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Create the container for all subconnection parameters 
    RSubConParameterBundle subconnParams; 
    
    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily = 
        CSubConParameterFamily::NewL(subconnParams, KSubConQoSFamily);
    CleanupDeletePushL(qosFamily);

    // Create the requested/acceptable generic parameter set for QoS 
    // (Qos family takes ownership) 
    CSubConQosGenericParamSet* reqGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::ERequested);
    // CleanupDeletePushL(reqGenericParams);

    CSubConQosGenericParamSet* accGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::EAcceptable);
    // CleanupDeletePushL(accGenericParams);

    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet1(*reqGenericParams);
    SetEsockParamSet1(*accGenericParams);

    // Subscribe for QoS Params notification
    subconn.EventNotification(subconnNotifBuf, ETrue, eventStatus); 
    
    // Set properties of the sub-connection 
    ret = subconn.SetParameters(subconnParams); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn.SetParameters-event 
    // (expected KSubConGenericEventParamsGranted)
    User::WaitForRequest(eventStatus); 
    TESTL(eventStatus.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf, KSubConGenericEventParamsGranted); 
    TESTE(ret == KErrNone,ret);
    
    
    

    // Clean up
    subconn.Close();
    subconnParams.Close(); 
    
    socket.Close();
    
    conn.Stop();
    conn.Close();
    
    // CleanupStack::Pop(accGenericParams); 
    // CleanupStack::Pop(reqGenericParams); 
    CleanupStack::Pop(qosFamily); 
    CleanupStack::Pop(&socketServer); 
    socketServer.Close();

    return iTestStepResult;
   
    
 #if 0
    
    
    // Connect to the socket server (Esock)
   TInt result;
   RSocketServ socketServer;
   CleanupClosePushL (socketServer);
   result = socketServer.Connect (); 
   Log(_L("socketServer.Connect() = %d\n"), result);

   // Open and start a connection
   RConnection connection;
   CleanupClosePushL (connection);
   result = connection.Open (socketServer); 
   Log(_L("connection.Open(socketServer) = %d\n"), result);

   TRequestStatus status;
   connection.Start (status); 
   User::WaitForRequest (status); 
   Log(_L("connection.Start(status) - status.Int() = %d\n"), status.Int());

   // Create a new sub-connection
   RSubConnection newSubConn;
   CleanupClosePushL (newSubConn);
   result = newSubConn.Open (socketServer, RSubConnection::ECreateNew, connection);
   Log(_L("newSubConn.Open(socketServer, RSubConnection::ECreateNew, connection) = %d\n"), result);
   
   // 8.4 - Creating and setting properties for a sub-connection
   // Create a parameter bundle and add to the cleanup stack
   RSubConParameterBundle parameterBundle;
   CleanupClosePushL (parameterBundle);
   Log(_L("RSubConParameterBundle created\n"));

	// NOTE: No need to push this onto the cleanup stack. If it succeeds they will be owned by parameterBundle
   CSubConParameterFamily* parameterFamily = CSubConParameterFamily::NewL (parameterBundle, KSubConQoSFamily);
   Log(_L("CSubConParameterFamily created\n"));
   
	// NOTE: No need to push these onto the cleanup stack. If they succeed they will be owned by parameterFamily
   CSubConQosGenericParamSet* requestedQoS = CSubConQosGenericParamSet::NewL (*parameterFamily, CSubConParameterFamily::ERequested);
	CSubConQosGenericParamSet* acceptableQoS = CSubConQosGenericParamSet::NewL (*parameterFamily, CSubConParameterFamily::EAcceptable);
   Log(_L("CSubConQosGenericParamSet's created (ERequested and EAcceptable)\n"));
   
   // Set some requested and acceptable QoS values
   SetEsockParamSet1 (*requestedQoS);
   SetEsockParamSet1 (*acceptableQoS);
   Log(_L("QoS values initialised\n"));
   
   // 8.6 - Registering for events - Using filters 
   RSubConnection::TEventFilter eventFilter;
   TNotificationEventBuf eventBuffer;
   TRequestStatus eventStatus;
   eventFilter.iEventGroupUid = KSubConnGenericEventsImplUid;
   eventFilter.iEventMask = KSubConGenericEventParamsRejected | KSubConGenericEventParamsGranted;
   newSubConn.EventNotification (eventBuffer, &eventFilter, 1, eventStatus);
   
   // Set parameters
   TInt setParamsResult;
   setParamsResult = newSubConn.SetParameters (parameterBundle);
   Log(_L("newSubConn.SetParameters (parameterBundle) = %d\n"), setParamsResult);
   
   if (setParamsResult == KErrNone)
      {
      // Because this is a new sub-connection negotiation will not take place until a socket
      // is connected on it
      RSocket socket;
      CleanupClosePushL (socket);
      
      TInt socketResult;
      socketResult = socket.Open (socketServer, KAfInet, KSockDatagram, KProtocolInetUdp);
      Log(_L("socket.Open (socketServer, KAfInet, KSockStream, KProtocolInetTcp) = %d\n"), socketResult);
      
      if (socketResult == KErrNone)
         {
         // Add socket to the sub-connection
         TRequestStatus addSocketStatus;
         newSubConn.Add (socket, addSocketStatus);
         
         TInetAddr remoteAddr;
         remoteAddr.SetPort (3221);  // discard
         remoteAddr.Input (_L("10.1.22.1"));
         
         TRequestStatus connectStatus;
         socket.Connect (remoteAddr, connectStatus);
         User::WaitForRequest (connectStatus); 
         Log(_L("socket.Connect (remoteAddr, connectStatus) - connectStatus.Int() = %d\n"), connectStatus.Int());
         
         socketResult = connectStatus.Int();
         
         if (socketResult == KErrNone)
            {
            Log(_L("Waiting for add socket to complete\n"));
            User::WaitForRequest (addSocketStatus);
            Log(_L("newSubConn.Add (socket, addSocketStatus) - addSocketStatus.Int() = %d\n"), addSocketStatus.Int());
            }
         }
      
      if (socketResult == KErrNone)
         {
         Log(_L("Waiting for event notification\n"));
         User::WaitForRequest(eventStatus);
         Log (_L("newSubConn.EventNotification (eventBuffer, &eventFilter, 1, eventStatus) - eventStatus.Int() = %d\n"), eventStatus.Int());
         Log(_L("eventBuffer.GroupId() = %08x\neventBuffer.Id() = %08x\n"), eventBuffer.GroupId(), eventBuffer.Id());
         
         if (eventStatus.Int() == KErrNone)
            {
            // 8.7 - Extracting information from received events
            CSubConNotificationEvent* event;
            event = CSubConNotificationEvent::NewL (eventBuffer);
            CleanupStack::PushL (event);
            
            if (event->GroupId () == KSubConnGenericEventsImplUid)
               {
               CSubConGenEventParamsRejected* rejectedEvent = NULL;
               switch (event->Id ())
                  {
                  case KSubConGenericEventParamsRejected:
                     rejectedEvent = static_cast<CSubConGenEventParamsRejected*>(event);
                     Log(_L("Event KSubConGenericEventParamsRejected received - rejectedEvent->Error () = %d\n"), rejectedEvent->Error ());
                     break;
                     
                  case KSubConGenericEventParamsGranted:
                     Log(_L("Event KSubConGenericEventParamsGranted received, sending some data\n"));
                     TRequestStatus sendStatus;
                     socket.Send (_L8("discard this text"), (TUint)0, sendStatus);
                     User::WaitForRequest(sendStatus); 
                     Log(_L("socket.Send (_L8(\"discard this text\"), (TUint)0, sendStatus) - sendStatus.Int() = %d\n"), sendStatus.Int());
                     break;
                  }
               }
               
            // Pop and destroy event
            CleanupStack::PopAndDestroy ();
            }
         }
      else
         {
         newSubConn.CancelEventNotification ();
         }
         
      // Pop and close socket
      CleanupStack::PopAndDestroy ();
      }
   else
      {
      newSubConn.CancelEventNotification ();
      }
      
   // Pop and close parameterBundle, newSubConn, connection, socketServer
   CleanupStack::PopAndDestroy (4);
  
   Log(_L("\r\n\r\nDone\r\nPress a key\r\n"));
   
   return iTestStepResult;
#endif
    }


CQoSTest_123::CQoSTest_123()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText51, "qos_123");
    iTestStepName = KText51;
    }

CQoSTest_123::~CQoSTest_123()
    {
    }

TVerdict CQoSTest_123::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText52, "qos_123 RSubConnection, functions fail due to incomplete \
initialization");
    Log( KText52 );

    RSocket                 socket;
    TInetAddr               dstAddr;
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn; 
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf;
    TInt                    ret; 

    // Connect to ESOCK 
    ret = socketServer.Connect();
    CleanupClosePushL(socketServer); 
    TESTL(ret == KErrNone); 

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP socket 
    ret = socket.Open(socketServer, 
                      KAfInet, 
                      KSockDatagram, 
                      KProtocolInetUdp, 
                      conn);
    TESTL(ret == KErrNone); 

    dstAddr.SetPort(3221);
    _LIT(KText50, "10.1.22.1");
    dstAddr.Input( KText50 );

    ret = socket.SetLocalPort(3332);
    TESTL(ret == KErrNone); 

    socket.Connect(dstAddr, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    // Create a new sub-connection 
    ret = subconn.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket onto the new sub-connection
    subconn.Add(socket, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Create the container for all subconnection parameters 
    RSubConParameterBundle subconnParams; 
    
    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily = 
        CSubConParameterFamily::NewL(subconnParams, KSubConQoSFamily);
    CleanupDeletePushL(qosFamily);

    // Create the requested/acceptable generic parameter set for QoS 
    // (Qos family takes ownership) 
    CSubConQosGenericParamSet* reqGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::ERequested);
    // CleanupDeletePushL(reqGenericParams);

    CSubConQosGenericParamSet* accGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::EAcceptable);
    // CleanupDeletePushL(accGenericParams);

    // Do not set the requested/acceptable Generic Parameters because 
    // the idea of the test case is that RSubConnection should return 
    // KSubConGenericEventParamsRejected due to incomplete initialization. 
    // 
    // SetEsockParamSet1(*reqGenericParams);
    // SetEsockParamSet1(*accGenericParams);

    // Subscribe for QoS Params notification
    subconn.EventNotification(subconnNotifBuf, ETrue, eventStatus); 
    
    // Set properties of the sub-connection 
    ret = subconn.SetParameters(subconnParams); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn.SetParameters-event 
    // (expected KSubConGenericEventParamsRejected)
    User::WaitForRequest(eventStatus); 
    TESTL(eventStatus.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf, KSubConGenericEventParamsRejected);
    TESTE(ret == KErrNone, ret);

    // Clean up
    subconn.Close();
    subconnParams.Close();

    socket.Close();

    conn.Stop();
    conn.Close();

    // CleanupStack::Pop(accGenericParams); 
    // CleanupStack::Pop(reqGenericParams); 
    CleanupStack::Pop(qosFamily); 
    CleanupStack::Pop(&socketServer); 
    socketServer.Close();

    return iTestStepResult;
    }

CQoSTest_124::CQoSTest_124()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText54, "qos_124");
    iTestStepName = KText54;
    }

CQoSTest_124::~CQoSTest_124()
    {
    }

TVerdict CQoSTest_124::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText55, "qos_124 RSubConnection, Arbitrary params requested");
    Log( KText55 );

    RSocket                 socket1, socket2;
    TInetAddr               dstAddr1, dstAddr2;
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn; 
    TRequestStatus          status1, status2, eventStatus;
    TNotificationEventBuf   subconnNotifBuf;
    TInt                    ret; 

    // Connect to ESOCK 
    ret = socketServer.Connect();
    CleanupClosePushL(socketServer); 
    TESTL(ret == KErrNone); 

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 

    // Start the connection 
    conn.Start(status1); 
    User::WaitForRequest(status1); 
    TESTL(status1.Int() == KErrNone);

    // Open UDP sockets 
    ret = socket1.Open(socketServer, 
                       KAfInet, 
                       KSockDatagram, 
                       KProtocolInetUdp, 
                       conn);
    TESTL(ret == KErrNone); 

    ret = socket2.Open(socketServer, 
                       KAfInet, 
                       KSockDatagram, 
                       KProtocolInetUdp, 
                       conn);
    TESTL(ret == KErrNone); 

    dstAddr1.SetPort(3241);
    _LIT(KText56, "10.1.24.1");
    dstAddr1.Input( KText56 );

    ret = socket1.SetLocalPort(3332);
    TESTL(ret == KErrNone); 

    socket1.Connect(dstAddr1, status1);
    User::WaitForRequest(status1);
    TESTL(status1.Int() == KErrNone); 

    dstAddr2.SetPort(3242);
    _LIT(KText57, "10.1.24.2");
    dstAddr2.Input( KText57 );

    ret = socket2.SetLocalPort(3243);
    TESTL(ret == KErrNone); 

    socket2.Connect(dstAddr2, status2);
    User::WaitForRequest(status2);
    TESTL(status2.Int() == KErrNone); 

    TEST(SendData(socket1) == KErrNone);
    TEST(SendData(socket2) == KErrNone);

    // Create a new sub-connection 
    ret = subconn.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    TESTL(ret == KErrNone);

    // Move the connected sockets onto the new sub-connection
    subconn.Add(socket1, status1);
    User::WaitForRequest(status1);
    TESTL(status1.Int() == KErrNone);

    subconn.Add(socket2, status2);
    User::WaitForRequest(status2);
    TESTL(status2.Int() == KErrNone);

    // Create the container for all subconnection parameters 
    RSubConParameterBundle subconnParams; 
    
    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily = 
        CSubConParameterFamily::NewL(subconnParams, KSubConQoSFamily);
    CleanupDeletePushL(qosFamily);

    // Create the requested/acceptable generic parameter set for QoS 
    // (Qos family takes ownership) 
    CSubConQosGenericParamSet* reqGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::ERequested);
    // CleanupDeletePushL(reqGenericParams);

    CSubConQosGenericParamSet* accGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::EAcceptable);
    // CleanupDeletePushL(accGenericParams);

    // Set the arbitrary parameters 
    reqGenericParams->SetUplinkBandwidth(400);
    reqGenericParams->SetUpLinkMaximumBurstSize(8192);
    reqGenericParams->SetUpLinkMaximumPacketSize(1);
    reqGenericParams->SetUpLinkAveragePacketSize(6552222);
    reqGenericParams->SetUpLinkDelay(10000);
    reqGenericParams->SetUpLinkPriority(1);

    reqGenericParams->SetDownlinkBandwidth(10);
    reqGenericParams->SetDownLinkMaximumBurstSize(10);
    reqGenericParams->SetDownLinkMaximumPacketSize(20480000);
    reqGenericParams->SetDownLinkAveragePacketSize(0);
    reqGenericParams->SetDownLinkDelay(0);
    reqGenericParams->SetDownLinkPriority(KQoSHighestPriority);

    accGenericParams->SetUplinkBandwidth(400);
    accGenericParams->SetUpLinkMaximumBurstSize(8192);
    accGenericParams->SetUpLinkMaximumPacketSize(1);
    accGenericParams->SetUpLinkAveragePacketSize(6552222);
    accGenericParams->SetUpLinkDelay(10000);
    accGenericParams->SetUpLinkPriority(1);

    accGenericParams->SetDownlinkBandwidth(10);
    accGenericParams->SetDownLinkMaximumBurstSize(10);
    accGenericParams->SetDownLinkMaximumPacketSize(20480000);
    accGenericParams->SetDownLinkAveragePacketSize(0);
    accGenericParams->SetDownLinkDelay(0);
    accGenericParams->SetDownLinkPriority(KQoSHighestPriority);

    // Subscribe for QoS Params notification
    subconn.EventNotification(subconnNotifBuf, ETrue, eventStatus); 
    
    // Set properties of the sub-connection 
    ret = subconn.SetParameters(subconnParams); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn.SetParameters-event 
    // (expected KSubConGenericEventParamsRejected)
    User::WaitForRequest(eventStatus); 
    TESTL(eventStatus.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf, KSubConGenericEventParamsRejected); 
    TESTE(ret == KErrNone, ret);

    // Make sure that sockets are working although SetParameters failed 
    TEST(SendData(socket1) == KErrNone);
    TEST(SendData(socket2) == KErrNone);

    // Clean up
    subconn.Close();
    subconnParams.Close(); 
    
    socket1.Close();
    socket2.Close();
    
    conn.Stop();
    conn.Close();
    
    // CleanupStack::Pop(accGenericParams); 
    // CleanupStack::Pop(reqGenericParams); 
    CleanupStack::Pop(qosFamily); 
    CleanupStack::Pop(&socketServer); 
    socketServer.Close();

    return iTestStepResult;
    }

CQoSTest_125::CQoSTest_125()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText58, "qos_125");
    iTestStepName = KText58;
    }

CQoSTest_125::~CQoSTest_125()
    {
    }

TVerdict CQoSTest_125::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText59, "qos_125 RSubConnection, Deleting subconn while pending \
SetParameters()");
    Log( KText59 );

    RSocket                 socket;
    TInetAddr               dstAddr;
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn; 
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf;
    TInt                    ret; 

    // Connect to ESOCK 
    ret = socketServer.Connect();
    CleanupClosePushL(socketServer); 
    TESTL(ret == KErrNone); 

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP socket 
    ret = socket.Open(socketServer, 
                      KAfInet, 
                      KSockDatagram, 
                      KProtocolInetUdp, 
                      conn);
    TESTL(ret == KErrNone); 
    
    dstAddr.SetPort(3251);
    _LIT(KText60, "10.1.25.1");
    dstAddr.Input( KText60 );

    ret = socket.SetLocalPort(3332);
    TESTL(ret == KErrNone); 

    socket.Connect(dstAddr, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    // Create a new sub-connection 
    ret = subconn.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    TESTL(ret == KErrNone);

    // Create the container for all subconnection parameters 
    RSubConParameterBundle subconnParams; 
    
    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily = 
        CSubConParameterFamily::NewL(subconnParams, KSubConQoSFamily);
    CleanupDeletePushL(qosFamily);

    // Create the requested/acceptable generic parameter set for QoS 
    // (Qos family takes ownership) 
    CSubConQosGenericParamSet* reqGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::ERequested);
    // CleanupDeletePushL(reqGenericParams);

    CSubConQosGenericParamSet* accGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::EAcceptable);
    // CleanupDeletePushL(accGenericParams);

    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet1(*reqGenericParams);
    SetEsockParamSet1(*accGenericParams);

    // Set properties of the sub-connection 
    ret = subconn.SetParameters(subconnParams); 
    TESTL(ret == KErrNone);

    subconn.Close();

    // Now make sure that socket is still operational.
    TInt err;
    err = SendData(socket);
    TESTE(err == KErrNone, err);

    // And assure QoS framework is still up'n'running
    TEST(subconn.Open(socketServer, 
                      RSubConnection::ECreateNew, 
                      conn) == KErrNone); 

    // Move the connected socket onto the new sub-connection
    subconn.Add(socket, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Subscribe for QoS Params notification
    subconn.EventNotification(subconnNotifBuf, ETrue, eventStatus); 

    // Set properties of the sub-connection 
    TEST(subconn.SetParameters(subconnParams) == KErrNone); 

    // Receive & Process the subconn.SetParameters-event 
    // (expected KSubConGenericEventParamsGranted)
    User::WaitForRequest(eventStatus); 
    TESTL(eventStatus.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf, KSubConGenericEventParamsGranted); 
    TESTE(ret == KErrNone, ret);

    // Clean up
    subconn.Close();
    subconnParams.Close(); 
    
    socket.Close();
    
    conn.Stop();
    conn.Close();
    
    // CleanupStack::Pop(accGenericParams); 
    // CleanupStack::Pop(reqGenericParams); 
    CleanupStack::Pop(qosFamily); 
    CleanupStack::Pop(&socketServer); 
    socketServer.Close();

    return iTestStepResult;
    }


CQoSTest_126::CQoSTest_126()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText61, "qos_126");
    iTestStepName = KText61;
    }

CQoSTest_126::~CQoSTest_126()
    {
    }

TVerdict CQoSTest_126::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText62, 
        "qos_126 RSubConnection, Deleting subconn while pending Add()");
    Log( KText62 );

    RSocket                 socket1, socket2;
    TInetAddr               dstAddr1, dstAddr2;
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn; 
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf;
    TInt                    ret; 

    // Connect to ESOCK 
    ret = socketServer.Connect();
    CleanupClosePushL(socketServer); 
    TESTL(ret == KErrNone); 

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP sockets 
    ret = socket1.Open(socketServer, 
                       KAfInet, 
                       KSockDatagram, 
                       KProtocolInetUdp, 
                       conn);
    TESTL(ret == KErrNone); 

    ret = socket2.Open(socketServer, 
                       KAfInet, 
                       KSockDatagram, 
                       KProtocolInetUdp, 
                       conn);
    TESTL(ret == KErrNone); 

    dstAddr1.SetPort(3261);
    _LIT(KText63, "10.1.26.1");
    dstAddr1.Input( KText63 );

    ret = socket1.SetLocalPort(3262);
    TESTL(ret == KErrNone); 

    socket1.Connect(dstAddr1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    dstAddr2.SetPort(3262);
    _LIT(KText64, "10.1.26.2");
    dstAddr2.Input( KText64 );

    ret = socket2.SetLocalPort(3263);
    TESTL(ret == KErrNone); 

    socket2.Connect(dstAddr2, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    // Create a new sub-connection 
    ret = subconn.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket1 onto the new sub-connection
    subconn.Add(socket1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Create the container for all subconnection parameters 
    RSubConParameterBundle subconnParams; 
    
    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily = 
        CSubConParameterFamily::NewL(subconnParams, KSubConQoSFamily);
    CleanupDeletePushL(qosFamily);

    // Create the requested/acceptable generic parameter set for QoS 
    // (Qos family takes ownership) 
    CSubConQosGenericParamSet* reqGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::ERequested);
    // CleanupDeletePushL(reqGenericParams);

    CSubConQosGenericParamSet* accGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::EAcceptable);
    // CleanupDeletePushL(accGenericParams);

    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet1(*reqGenericParams);
    SetEsockParamSet1(*accGenericParams);

    // Subscribe for QoS Params notification
    subconn.EventNotification(subconnNotifBuf, ETrue, eventStatus); 

    // Set properties of the sub-connection 
    ret = subconn.SetParameters(subconnParams); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn.SetParameters-event 
    // (expected KSubConGenericEventParamsGranted)
    User::WaitForRequest(eventStatus); 
    TESTL(eventStatus.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf, KSubConGenericEventParamsGranted); 
    TESTE(ret == KErrNone, ret);

    // Move the connected socket2 onto the new sub-connection
    // and close the sub-connection immediately
    subconn.Add(socket2, status);
    subconn.Close();

    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Now make sure that sockets are still operational.
    TEST(SendData(socket1) == KErrNone);
    TEST(SendData(socket2) == KErrNone);

    // And assure QoS framework is still up'n'running
    ret = subconn.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    TESTL(ret == KErrNone);

    // Clean up
    subconn.Close();
    subconnParams.Close(); 

    socket1.Close();

    conn.Stop();
    conn.Close();

    // CleanupStack::Pop(accGenericParams); 
    // CleanupStack::Pop(reqGenericParams); 
    CleanupStack::Pop(qosFamily); 
    CleanupStack::Pop(&socketServer); 
    socketServer.Close();

    return iTestStepResult;
    }


CQoSTest_127::CQoSTest_127()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText65, "qos_127");
    iTestStepName = KText65;
    }

CQoSTest_127::~CQoSTest_127()
    {
    }

TVerdict CQoSTest_127::doTestStepL( void )
    {
    User::After(KInterTestDelay);

    _LIT(KText66, 
        "qos_127 RSubConnection, Deleting subconn while pending Remove()"); 
    Log( KText66 );

    RSocket                 socket1, socket2;
    TInetAddr               dstAddr1, dstAddr2;
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn; 
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf;
    TInt                    ret; 

    // Connect to ESOCK 
    ret = socketServer.Connect();
    CleanupClosePushL(socketServer); 
    TESTL(ret == KErrNone); 

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP sockets 
    ret = socket1.Open(socketServer, 
                       KAfInet, 
                       KSockDatagram, 
                       KProtocolInetUdp, 
                       conn);
    TESTL(ret == KErrNone); 

    ret = socket2.Open(socketServer, 
                       KAfInet, 
                       KSockDatagram, 
                       KProtocolInetUdp, 
                       conn);
    TESTL(ret == KErrNone); 

    dstAddr1.SetPort(3261);
    _LIT(KText63, "10.1.27.1");
    dstAddr1.Input( KText63 );

    ret = socket1.SetLocalPort(3262);
    TESTL(ret == KErrNone); 

    socket1.Connect(dstAddr1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    dstAddr2.SetPort(3263);
    _LIT(KText64, "10.1.27.2");
    dstAddr2.Input( KText64 );

    ret = socket2.SetLocalPort(3264);
    TESTL(ret == KErrNone); 

    socket2.Connect(dstAddr2, status);
    User::WaitForRequest(status);    
    TESTL(status.Int() == KErrNone); 

    // Create a new sub-connection 
    ret = subconn.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket1 onto the new sub-connection
    subconn.Add(socket1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Create the container for all subconnection parameters 
    RSubConParameterBundle subconnParams; 
    
    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily = 
        CSubConParameterFamily::NewL(subconnParams, KSubConQoSFamily);
    CleanupDeletePushL(qosFamily);

    // Create the requested/acceptable generic parameter set for QoS 
    // (Qos family takes ownership) 
    CSubConQosGenericParamSet* reqGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::ERequested);
//    CleanupDeletePushL(reqGenericParams);

    CSubConQosGenericParamSet* accGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::EAcceptable);
//    CleanupDeletePushL(accGenericParams);

    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet1(*reqGenericParams);
    SetEsockParamSet1(*accGenericParams);

    // Subscribe for QoS Params notification
    subconn.EventNotification(subconnNotifBuf, ETrue, eventStatus); 

    // Set properties of the sub-connection 
    ret = subconn.SetParameters(subconnParams); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn.SetParameters-event 
    // (expected KSubConGenericEventParamsGranted)
    User::WaitForRequest(eventStatus); 
    TESTL(eventStatus.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf, KSubConGenericEventParamsGranted); 
    TESTE(ret == KErrNone, ret);

    // Subscribe for QoS Params notification
    subconn.EventNotification(subconnNotifBuf, ETrue, eventStatus); 

    // Move the connected socket2 onto the new sub-connection
    subconn.Add(socket2, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Receive & Process the subconn.Add-event 
    // (expected KSubConGenericEventDataClientJoined)
    User::WaitForRequest(eventStatus);
    ret = CheckEvent(subconnNotifBuf, KSubConGenericEventDataClientJoined); 
    TESTE(ret == KErrNone, ret);

    // Remove the connected socket2 from the sub-connection
    // and close the sub-connection immediately
    subconn.Remove(socket2, status);
    subconn.Close();

    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Now make sure that sockets are still operational.
    TEST(SendData(socket1) == KErrNone);
    TEST(SendData(socket2) == KErrNone);

    // And assure QoS framework is still up'n'running
    ret = subconn.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    //??TESTE(ret == KErrNotReady, ret);
	TESTE(ret == KErrNone, ret);
    // Clean up
    subconn.Close();
    subconnParams.Close(); 

    socket1.Close();
    socket2.Close();

    conn.Stop();
    conn.Close();

//  CleanupStack::Pop(accGenericParams); 
//  CleanupStack::Pop(reqGenericParams); 
    CleanupStack::Pop(qosFamily); 
    CleanupStack::Pop(&socketServer); 
    socketServer.Close();

    return iTestStepResult;
    }


CQoSTest_128::CQoSTest_128()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText69, "qos_128");
    iTestStepName = KText69;
    }

CQoSTest_128::~CQoSTest_128()
    {
    }

TVerdict CQoSTest_128::doTestStepL( void )
    {

    User::After(KInterTestDelay);

    _LIT(KText70, "qos_128 RSubConnection, Closing socket while pending \
SetParameters()");
    Log( KText70 );

    RSocket                 socket1;
    TInetAddr               dstAddr1;
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn; 
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf;
    TInt                    ret; 

    // Connect to ESOCK 
    ret = socketServer.Connect();
    CleanupClosePushL(socketServer); 
    TESTL(ret == KErrNone); 

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP sockets 
    ret = socket1.Open(socketServer, 
                       KAfInet, 
                       KSockDatagram, 
                       KProtocolInetUdp, 
                       conn);
    TESTL(ret == KErrNone); 

    dstAddr1.SetPort(3261);
    _LIT(KText63, "10.1.28.1");
    dstAddr1.Input( KText63 );

    ret = socket1.SetLocalPort(3262);
    TESTL(ret == KErrNone); 

    socket1.Connect(dstAddr1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    // Create a new sub-connection 
    ret = subconn.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket1 onto the new sub-connection 
    subconn.Add(socket1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Create the container for all subconnection parameters 
    RSubConParameterBundle subconnParams; 
    
    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily = 
        CSubConParameterFamily::NewL(subconnParams, KSubConQoSFamily);
    CleanupDeletePushL(qosFamily);

    // Create the requested/acceptable generic parameter set for QoS 
    // (Qos family takes ownership) 
    CSubConQosGenericParamSet* reqGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::ERequested);
    // CleanupDeletePushL(reqGenericParams);

    CSubConQosGenericParamSet* accGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::EAcceptable);
    // CleanupDeletePushL(accGenericParams);

    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet1(*reqGenericParams);
    SetEsockParamSet1(*accGenericParams);

    // Subscribe for QoS Params notification
    subconn.EventNotification(subconnNotifBuf, ETrue, eventStatus); 

    // Set properties of the sub-connection 
    ret = subconn.SetParameters(subconnParams); 
    TESTL(ret == KErrNone);

    // and close socket1 immediately
    socket1.Close();

    // Receive & Process the subconn.SetParameters-event 
    // (expected KSubConGenericEventDataClientLeft)
    User::WaitForRequest(eventStatus); 
    TESTL(eventStatus.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf, KSubConGenericEventDataClientLeft); 
    TESTE(ret == KErrNone, ret);

    // Clean up
    subconn.Close();
    subconnParams.Close(); 

    conn.Stop();
    conn.Close();

//  CleanupStack::Pop(accGenericParams); 
//  CleanupStack::Pop(reqGenericParams); 
    CleanupStack::Pop(qosFamily); 
    CleanupStack::Pop(&socketServer); 
    socketServer.Close();

    return iTestStepResult;
    }


CQoSTest_129::CQoSTest_129()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText72, "qos_129");
    iTestStepName = KText72;
    }

CQoSTest_129::~CQoSTest_129()
    {
    }

TVerdict CQoSTest_129::doTestStepL( void )
    {
    User::After(KInterTestDelay);

    _LIT(KText73, "qos_129 RSubConnection, Closing socket2 while pending \
Add() for socket2");
    Log( KText73 );

    RSocket                 socket1, socket2;
    TInetAddr               dstAddr1, dstAddr2;
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn; 
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf;
    TInt                    ret; 

    // Connect to ESOCK 
    ret = socketServer.Connect();
    CleanupClosePushL(socketServer); 
    TESTL(ret == KErrNone); 

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP sockets 
    ret = socket1.Open(socketServer, 
                       KAfInet, 
                       KSockDatagram, 
                       KProtocolInetUdp, 
                       conn);
    TESTL(ret == KErrNone); 

    ret = socket2.Open(socketServer, 
                       KAfInet, 
                       KSockDatagram, 
                       KProtocolInetUdp, 
                       conn);
    TESTL(ret == KErrNone); 

    dstAddr1.SetPort(3261);
    _LIT(KText63, "10.1.29.1");
    dstAddr1.Input( KText63 );

    ret = socket1.SetLocalPort(3262);
    TESTL(ret == KErrNone); 

    socket1.Connect(dstAddr1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    dstAddr2.SetPort(3263);
    _LIT(KText64, "10.1.29.2");
    dstAddr2.Input( KText64 );

    ret = socket2.SetLocalPort(3264);
    TESTL(ret == KErrNone); 

    socket2.Connect(dstAddr2, status);
    User::WaitForRequest(status);    
    TESTL(status.Int() == KErrNone); 

    // Create a new sub-connection 
    ret = subconn.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket1 onto the new sub-connection
    subconn.Add(socket1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Create the container for all subconnection parameters 
    RSubConParameterBundle subconnParams; 

    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily = 
        CSubConParameterFamily::NewL(subconnParams, KSubConQoSFamily);
    CleanupDeletePushL(qosFamily);

    // Create the requested/acceptable generic parameter set for QoS 
    // (Qos family takes ownership) 
    CSubConQosGenericParamSet* reqGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::ERequested);
    //CleanupDeletePushL(reqGenericParams);

    CSubConQosGenericParamSet* accGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::EAcceptable);
    //CleanupDeletePushL(accGenericParams);

    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet1(*reqGenericParams);
    SetEsockParamSet1(*accGenericParams);

    // Subscribe for QoS Params notification
    subconn.EventNotification(subconnNotifBuf, ETrue, eventStatus); 
    
    // Set properties of the sub-connection 
    ret = subconn.SetParameters(subconnParams); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn.SetParameters-event 
    // (expected KSubConGenericEventParamsGranted)
    User::WaitForRequest(eventStatus); 
    TESTL(eventStatus.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf, KSubConGenericEventParamsGranted); 
    TESTE(ret == KErrNone, ret);

    // Move the connected socket2 onto the new sub-connection 
    // and close the sub-connection immediately
    subconn.Add(socket2, status);
    socket2.Close();

    User::WaitForRequest(status);
    TESTL(status.Int() == KErrAbort);

    // Now make sure that socket1 is still operational.
    TEST(SendData(socket1) == KErrNone);

    // Clean up
    subconn.Close();
    subconnParams.Close(); 

    socket1.Close();

    conn.Stop();
    conn.Close();

    //CleanupStack::Pop(accGenericParams); 
    //CleanupStack::Pop(reqGenericParams); 
    CleanupStack::Pop(qosFamily); 
    CleanupStack::Pop(&socketServer); 
    socketServer.Close();

    return iTestStepResult;
    }


CQoSTest_130::CQoSTest_130()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText76, "qos_130");
    iTestStepName = KText76;
    }

CQoSTest_130::~CQoSTest_130()
    {
    }

TVerdict CQoSTest_130::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText77, "qos_130 RSubConnection, Closing socket1 while pending \
Add() for socket2");
    Log( KText77 );

    RSocket                 socket1, socket2;
    TInetAddr               dstAddr1, dstAddr2;
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn; 
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf;
    TInt                    ret; 

    // Connect to ESOCK 
    ret = socketServer.Connect();
    CleanupClosePushL(socketServer); 
    TESTL(ret == KErrNone); 

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP sockets 
    ret = socket1.Open(socketServer, 
                       KAfInet, 
                       KSockDatagram, 
                       KProtocolInetUdp, 
                       conn);
    TESTL(ret == KErrNone); 

    ret = socket2.Open(socketServer, 
                       KAfInet, 
                       KSockDatagram, 
                       KProtocolInetUdp, 
                       conn);
    TESTL(ret == KErrNone); 

    dstAddr1.SetPort(3261);
    _LIT(KText63, "10.1.30.1");
    dstAddr1.Input( KText63 );

    ret = socket1.SetLocalPort(3262);
    TESTL(ret == KErrNone); 

    socket1.Connect(dstAddr1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    dstAddr2.SetPort(3263);
    _LIT(KText64, "10.1.31.2");
    dstAddr2.Input( KText64 );

    ret = socket2.SetLocalPort(3264);
    TESTL(ret == KErrNone); 

    socket2.Connect(dstAddr2, status);
    User::WaitForRequest(status);    
    TESTL(status.Int() == KErrNone); 

    // Create a new sub-connection 
    ret = subconn.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket1 onto the new sub-connection
    subconn.Add(socket1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Create the container for all subconnection parameters 
    RSubConParameterBundle subconnParams; 
    
    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily = 
        CSubConParameterFamily::NewL(subconnParams, KSubConQoSFamily);
    CleanupDeletePushL(qosFamily);

    // Create the requested/acceptable generic parameter set for QoS 
    // (Qos family takes ownership) 
    CSubConQosGenericParamSet* reqGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::ERequested);
    //CleanupDeletePushL(reqGenericParams);

    CSubConQosGenericParamSet* accGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::EAcceptable);
    //CleanupDeletePushL(accGenericParams);

    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet1(*reqGenericParams);
    SetEsockParamSet1(*accGenericParams);

    // Subscribe for QoS Params notification
    subconn.EventNotification(subconnNotifBuf, ETrue, eventStatus); 
    
    // Set properties of the sub-connection 
    ret = subconn.SetParameters(subconnParams); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn.SetParameters-event 
    // (expected KSubConGenericEventParamsGranted)
    User::WaitForRequest(eventStatus); 
    TESTL(eventStatus.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf, KSubConGenericEventParamsGranted); 
    TESTE(ret == KErrNone, ret);

    WakeupNifL();
    TEST(SendData(socket1) == KErrNone);

    // Move the connected socket2 onto the new sub-connection 
    // and close the socket1 immediately
    subconn.Add(socket2, status);
    socket1.Close();

    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Clean up
    subconn.Close();
    subconnParams.Close(); 

    socket1.Close();
    socket2.Close();

    conn.Stop();
    conn.Close();

    //CleanupStack::Pop(accGenericParams); 
    //CleanupStack::Pop(reqGenericParams); 
    CleanupStack::Pop(qosFamily); 
    CleanupStack::Pop(&socketServer); 
    socketServer.Close();

    return iTestStepResult;
    }


CQoSTest_131::CQoSTest_131()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText81, "qos_131");
    iTestStepName = KText81;
    }

CQoSTest_131::~CQoSTest_131()
    {
    }

TVerdict CQoSTest_131::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText82, "qos_131 RSubConnection, Closing socket2 while pending \
Remove() on socket2");
    Log( KText82 );

    RSocket                 socket1, socket2;
    TInetAddr               dstAddr1, dstAddr2;
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn; 
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf;
    TInt                    ret; 

    // Connect to ESOCK 
    ret = socketServer.Connect();
    CleanupClosePushL(socketServer); 
    TESTL(ret == KErrNone); 

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP sockets 
    ret = socket1.Open(socketServer, 
                       KAfInet, 
                       KSockDatagram, 
                       KProtocolInetUdp, 
                       conn);
    TESTL(ret == KErrNone); 

    ret = socket2.Open(socketServer, 
                       KAfInet, 
                       KSockDatagram, 
                       KProtocolInetUdp, 
                       conn);
    TESTL(ret == KErrNone); 

    dstAddr1.SetPort(3261);
    _LIT(KText63, "10.1.31.1");
    dstAddr1.Input( KText63 );

    ret = socket1.SetLocalPort(3262);
    TESTL(ret == KErrNone); 

    socket1.Connect(dstAddr1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    dstAddr2.SetPort(3263);
    _LIT(KText64, "10.1.31.2");
    dstAddr2.Input( KText64 );

    ret = socket2.SetLocalPort(3264);
    TESTL(ret == KErrNone); 

    socket2.Connect(dstAddr2, status);
    User::WaitForRequest(status);    
    TESTL(status.Int() == KErrNone); 

    // Create a new sub-connection 
    ret = subconn.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket1 onto the new sub-connection
    subconn.Add(socket1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Create the container for all subconnection parameters 
    RSubConParameterBundle subconnParams; 
    
    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily = 
        CSubConParameterFamily::NewL(subconnParams, KSubConQoSFamily);
    CleanupDeletePushL(qosFamily);

    // Create the requested/acceptable generic parameter set for QoS 
    // (Qos family takes ownership) 
    CSubConQosGenericParamSet* reqGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::ERequested);
    //CleanupDeletePushL(reqGenericParams);

    CSubConQosGenericParamSet* accGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::EAcceptable);
    //CleanupDeletePushL(accGenericParams);

    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet1(*reqGenericParams);
    SetEsockParamSet1(*accGenericParams);

    // Subscribe for QoS Params notification
    subconn.EventNotification(subconnNotifBuf, ETrue, eventStatus); 
    
    // Set properties of the sub-connection 
    ret = subconn.SetParameters(subconnParams); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn.SetParameters-event 
    // (expected KSubConGenericEventParamsGranted)
    User::WaitForRequest(eventStatus); 
    TESTL(eventStatus.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf, KSubConGenericEventParamsGranted); 
    TESTE(ret == KErrNone, ret);

    // Move the connected socket2 onto the new sub-connection
    subconn.Add(socket2, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Remove the connected socket2 from the sub-connection
    // and close the socket2 immediately
    subconn.Remove(socket2, status);
    socket2.Close();

    User::WaitForRequest(status);
    TESTL(status.Int() == KErrAbort);

    // Clean up
    subconn.Close();
    subconnParams.Close(); 

    socket1.Close();

    conn.Stop();
    conn.Close();

    //CleanupStack::Pop(accGenericParams); 
    //CleanupStack::Pop(reqGenericParams); 
    CleanupStack::Pop(qosFamily); 
    CleanupStack::Pop(&socketServer); 
    socketServer.Close();

    return iTestStepResult;
    }


CQoSTest_132::CQoSTest_132()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText85, "qos_132");
    iTestStepName = KText85;
    }

CQoSTest_132::~CQoSTest_132()
    {
    }

TVerdict CQoSTest_132::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText86, "qos_132 RSubConnection, Closing socket1 while pending \
Remove() on socket2"); 
    Log( KText86 );

    RSocket                 socket1, socket2;
    TInetAddr               dstAddr1, dstAddr2;
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn; 
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf;
    TInt                    ret; 

    // Connect to ESOCK 
    ret = socketServer.Connect();
    CleanupClosePushL(socketServer); 
    TESTL(ret == KErrNone); 

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP sockets 
    ret = socket1.Open(socketServer, 
                       KAfInet, 
                       KSockDatagram, 
                       KProtocolInetUdp, 
                       conn);
    TESTL(ret == KErrNone); 

    ret = socket2.Open(socketServer, 
                       KAfInet, 
                       KSockDatagram, 
                       KProtocolInetUdp, 
                       conn);
    TESTL(ret == KErrNone); 

    dstAddr1.SetPort(3261);
    _LIT(KText63, "10.1.32.1");
    dstAddr1.Input( KText63 );

    ret = socket1.SetLocalPort(3262);
    TESTL(ret == KErrNone); 

    socket1.Connect(dstAddr1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    dstAddr2.SetPort(3263);
    _LIT(KText64, "10.1.32.2");
    dstAddr2.Input( KText64 );

    ret = socket2.SetLocalPort(3264);
    TESTL(ret == KErrNone); 

    socket2.Connect(dstAddr2, status);
    User::WaitForRequest(status);    
    TESTL(status.Int() == KErrNone); 

    // Create a new sub-connection 
    ret = subconn.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket1 onto the new sub-connection
    subconn.Add(socket1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Create the container for all subconnection parameters 
    RSubConParameterBundle subconnParams; 
    
    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily = 
        CSubConParameterFamily::NewL(subconnParams, KSubConQoSFamily);
    CleanupDeletePushL(qosFamily);

    // Create the requested/acceptable generic parameter set for QoS 
    // (Qos family takes ownership) 
    CSubConQosGenericParamSet* reqGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::ERequested);
    //CleanupDeletePushL(reqGenericParams);

    CSubConQosGenericParamSet* accGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::EAcceptable);
    //CleanupDeletePushL(accGenericParams);

    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet1(*reqGenericParams);
    SetEsockParamSet1(*accGenericParams);

    // Subscribe for QoS Params notification
    subconn.EventNotification(subconnNotifBuf, ETrue, eventStatus); 

    // Set properties of the sub-connection 
    ret = subconn.SetParameters(subconnParams); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn.SetParameters-event 
    // (expected KSubConGenericEventParamsGranted)
    User::WaitForRequest(eventStatus); 
    TESTL(eventStatus.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf, KSubConGenericEventParamsGranted); 
    TESTE(ret == KErrNone, ret);

    // Move the connected socket2 onto the new sub-connection
    subconn.Add(socket2, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Remove the connected socket2 from the sub-connection
    // and close the socket2 immediately
    subconn.Remove(socket2, status);
    socket1.Close();

    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Clean up
    subconn.Close();
    subconnParams.Close(); 

    socket2.Close();

    conn.Stop();
    conn.Close();

    //CleanupStack::Pop(accGenericParams); 
    //CleanupStack::Pop(reqGenericParams); 
    CleanupStack::Pop(qosFamily); 
    CleanupStack::Pop(&socketServer); 
    socketServer.Close();

    return iTestStepResult;
    }


CQoSTest_134::CQoSTest_134()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText89, "qos_134");
    iTestStepName = KText89;
    }

CQoSTest_134::~CQoSTest_134()
    {
    }

TVerdict CQoSTest_134::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText90, "qos_134 RSubConnection, Add(), Remove()");
    Log( KText90 );

    RSocket                 socket1, socket2;
    TInetAddr               dstAddr1, dstAddr2;
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn; 
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf;
    TInt                    ret; 

    // Connect to ESOCK 
    ret = socketServer.Connect();
    CleanupClosePushL(socketServer); 
    TESTL(ret == KErrNone); 

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP sockets 
    ret = socket1.Open(socketServer, 
                       KAfInet, 
                       KSockDatagram, 
                       KProtocolInetUdp, 
                       conn);
    TESTL(ret == KErrNone); 

    ret = socket2.Open(socketServer, 
                       KAfInet, 
                       KSockDatagram, 
                       KProtocolInetUdp, 
                       conn);
    TESTL(ret == KErrNone); 

    dstAddr1.SetPort(3261);
    _LIT(KText63, "10.1.34.1");
    dstAddr1.Input( KText63 );

    ret = socket1.SetLocalPort(3262);
    TESTL(ret == KErrNone); 

    socket1.Connect(dstAddr1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    dstAddr2.SetPort(3263);
    _LIT(KText64, "10.1.34.2");
    dstAddr2.Input( KText64 );

    ret = socket2.SetLocalPort(3264);
    TESTL(ret == KErrNone); 

    socket2.Connect(dstAddr2, status);
    User::WaitForRequest(status);    
    TESTL(status.Int() == KErrNone); 

    // Create a new sub-connection 
    ret = subconn.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket1 onto the new sub-connection
    subconn.Add(socket1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Create the container for all subconnection parameters 
    RSubConParameterBundle subconnParams; 
    
    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily = 
        CSubConParameterFamily::NewL(subconnParams, KSubConQoSFamily);
    CleanupDeletePushL(qosFamily);

    // Create the requested/acceptable generic parameter set for QoS 
    // (Qos family takes ownership) 
    CSubConQosGenericParamSet* reqGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::ERequested);
    //CleanupDeletePushL(reqGenericParams);

    CSubConQosGenericParamSet* accGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::EAcceptable);
    //CleanupDeletePushL(accGenericParams);

    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet1(*reqGenericParams);
    SetEsockParamSet1(*accGenericParams);

    // Subscribe for QoS Params notification
    subconn.EventNotification(subconnNotifBuf, ETrue, eventStatus); 

    // Set properties of the sub-connection 
    ret = subconn.SetParameters(subconnParams); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn.SetParameters-event 
    // (expected KSubConGenericEventParamsGranted)
    User::WaitForRequest(eventStatus); 
    TESTL(eventStatus.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf, KSubConGenericEventParamsGranted); 
    TESTE(ret == KErrNone, ret);

    // Move the connected socket2 onto the new sub-connection
    subconn.Add(socket2, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Remove the connected socket2 from the sub-connection
    subconn.Remove(socket2, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Close the sub-connection
    subconn.Close();

    // Write to sockets and see that they still work.
    TEST(SendData(socket1) == KErrNone);
    TEST(SendData(socket2) == KErrNone);

    // Clean up
    subconnParams.Close(); 

    socket1.Close();
    socket2.Close();

    conn.Stop();
    conn.Close();

    //CleanupStack::Pop(accGenericParams); 
    //CleanupStack::Pop(reqGenericParams); 
    CleanupStack::Pop(qosFamily); 
    CleanupStack::Pop(&socketServer); 
    socketServer.Close();

    return iTestStepResult;
    }


CQoSTest_135::CQoSTest_135()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText93, "qos_135");
    iTestStepName = KText93;
    }

CQoSTest_135::~CQoSTest_135()
    {
    }

TVerdict CQoSTest_135::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText94, "qos_135 RSubConnection, Attempting to use SetParameters() \
without correct extension module");
    Log( KText94 );

    // create KUseTestModuleNonExist file so that testnif knows to load test 
    // module - or to be specific, it tries to load the module which is not 
    // there
    TInt err = CreateTestFile(KUseTestModuleNonExist);
    if(err != KErrNone)
        {
        _LIT(KText95, "Creating testfile failed, aborting test");
        Log( KText95 );
        return EInconclusive;
        }

    RSocket                 socket;
    TInetAddr               dstAddr;
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn; 
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf;
    TInt                    ret; 

    // Connect to ESOCK 
    ret = socketServer.Connect();
    CleanupClosePushL(socketServer); 
    TESTL(ret == KErrNone); 

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP socket 
    ret = socket.Open(socketServer, 
                      KAfInet, 
                      KSockDatagram, 
                      KProtocolInetUdp, 
                      conn);
    TESTL(ret == KErrNone); 
    
    dstAddr.SetPort(3351);
    _LIT(KText50, "10.1.35.1");
    dstAddr.Input( KText50 );

    ret = socket.SetLocalPort(3352);
    TESTL(ret == KErrNone);

    socket.Connect(dstAddr, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    // Create a new sub-connection 
    ret = subconn.Open(socketServer, 
                      RSubConnection::ECreateNew, 
                      conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket onto the new sub-connection
    subconn.Add(socket, status);
    User::WaitForRequest(status);
    TESTL(status.Int() != KErrNone);

    // Writing to socket should still work
    TEST(SendData(socket) == KErrNone);

    // Clean up
    subconn.Close();
    
    socket.Close();
    
    conn.Stop();
    conn.Close();

    CleanupStack::Pop(&socketServer); 
    socketServer.Close();

    // When test files are written they must also be cleared, otherwise
    // next case can get confused
    ClearTestFiles();

    return iTestStepResult;
    }


CQoSTest_136::CQoSTest_136()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText97, "qos_136");
    iTestStepName = KText97;
    }

CQoSTest_136::~CQoSTest_136()
    {
    }

TVerdict CQoSTest_136::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText98, "qos_136 RSubConnection, Re-negotiate QoS");
    Log( KText98 );

    RSocket                 socket;
    TInetAddr               dstAddr;
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn; 
    TRequestStatus          status, eventStatus; 
    TNotificationEventBuf   subconnNotifBuf;
    TInt                    ret; 

    // Connect to ESOCK 
    ret = socketServer.Connect(); 
    CleanupClosePushL(socketServer); 
    TESTL(ret == KErrNone); 

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP socket 
    ret = socket.Open(socketServer, 
                      KAfInet, 
                      KSockDatagram, 
                      KProtocolInetUdp, 
                      conn);
    TESTL(ret == KErrNone); 

    dstAddr.SetPort(3221);
    _LIT(KText50, "10.1.36.1");
    dstAddr.Input( KText50 );

    ret = socket.SetLocalPort(3332);
    TESTL(ret == KErrNone);
    
    socket.Connect(dstAddr, status);
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone); 

    // Create a new sub-connection 
    ret = subconn.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    TESTL(ret == KErrNone);
    
    // Move the connected socket onto the new sub-connection
    subconn.Add(socket, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Create the container for all subconnection parameters 
    RSubConParameterBundle subconnParams; 
    
    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily = 
        CSubConParameterFamily::NewL(subconnParams, KSubConQoSFamily);
    CleanupDeletePushL(qosFamily);

    // Create the requested/acceptable generic parameter set for QoS 
    // (Qos family takes ownership) 
    CSubConQosGenericParamSet* reqGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::ERequested);
    //CleanupDeletePushL(reqGenericParams);

    CSubConQosGenericParamSet* accGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::EAcceptable);
    //CleanupDeletePushL(accGenericParams);

    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet2(*reqGenericParams);
    SetEsockParamSet2(*accGenericParams);

    // Subscribe for QoS Params notification
    subconn.EventNotification(subconnNotifBuf, ETrue, eventStatus); 
    
    // Set properties of the sub-connection 
    ret = subconn.SetParameters(subconnParams); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn.SetParameters-event 
    // (expected KSubConGenericEventParamsGranted)
    User::WaitForRequest(eventStatus); 
    TESTL(eventStatus.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf, KSubConGenericEventParamsGranted);
    TESTE(ret == KErrNone, ret); 
    
    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet1(*reqGenericParams);
    SetEsockParamSet1(*accGenericParams);

    // Subscribe for QoS Params notification
    subconn.EventNotification(subconnNotifBuf, ETrue, eventStatus); 
    
    // Set properties of the sub-connection 
    ret = subconn.SetParameters(subconnParams); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn.SetParameters-event 
    // (expected KSubConGenericEventParamsGranted)
    User::WaitForRequest(eventStatus); 
    TESTL(eventStatus.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf, KSubConGenericEventParamsGranted); 
    TESTE(ret == KErrNone, ret);

    // Clean up
    subconn.Close();
    subconnParams.Close(); 
    
    socket.Close();
    
    conn.Stop();
    conn.Close();

    //CleanupStack::Pop(accGenericParams); 
    //CleanupStack::Pop(reqGenericParams); 
    CleanupStack::Pop(qosFamily); 
    CleanupStack::Pop(&socketServer); 
    socketServer.Close();

    return iTestStepResult;
    }


CQoSTest_142::CQoSTest_142()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText103, "qos_142");
    iTestStepName = KText103;
    }

CQoSTest_142::~CQoSTest_142()
    {
    }

TVerdict CQoSTest_142::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText104, "qos_142 RSubConnection, Maximum number of sockets per one\
 secondary PDP-context");
    Log( KText104 ); 

    RSocket                 socket1, socket2, socket3, socket4, socket5, 
                            socket6, socket7, socket8, socket9;
    TInetAddr               dstAddr1, dstAddr2, dstAddr3, dstAddr4, dstAddr5, 
                            dstAddr6, dstAddr7, dstAddr8, dstAddr9;
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn; 
    TRequestStatus          status, eventStatus; 
    TNotificationEventBuf   subconnNotifBuf;
    TInt                    ret; 

    // Connect to ESOCK 
    ret = socketServer.Connect(); 
    CleanupClosePushL(socketServer); 
    TESTL(ret == KErrNone); 

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP sockets 
    ret = socket1.Open(socketServer, 
                       KAfInet, 
                       KSockDatagram, 
                       KProtocolInetUdp, 
                       conn);
    TESTL(ret == KErrNone); 

    ret = socket2.Open(socketServer, 
                       KAfInet, 
                       KSockDatagram, 
                       KProtocolInetUdp, 
                       conn);
    TESTL(ret == KErrNone); 

    ret = socket3.Open(socketServer, 
                       KAfInet, 
                       KSockDatagram, 
                       KProtocolInetUdp, 
                       conn);
    TESTL(ret == KErrNone); 

    ret = socket4.Open(socketServer, 
                       KAfInet, 
                       KSockDatagram, 
                       KProtocolInetUdp, 
                       conn);
    TESTL(ret == KErrNone); 

    ret = socket5.Open(socketServer, 
                       KAfInet, 
                       KSockDatagram, 
                       KProtocolInetUdp, 
                       conn);
    TESTL(ret == KErrNone); 

    ret = socket6.Open(socketServer, 
                       KAfInet, 
                       KSockDatagram, 
                       KProtocolInetUdp, 
                       conn);
    TESTL(ret == KErrNone); 

    ret = socket7.Open(socketServer, 
                       KAfInet, 
                       KSockDatagram, 
                       KProtocolInetUdp, 
                       conn);
    TESTL(ret == KErrNone); 

    ret = socket8.Open(socketServer, 
                       KAfInet, 
                       KSockDatagram, 
                       KProtocolInetUdp, 
                       conn);
    TESTL(ret == KErrNone); 

    ret = socket9.Open(socketServer, 
                       KAfInet, 
                       KSockDatagram, 
                       KProtocolInetUdp, 
                       conn);
    TESTL(ret == KErrNone); 

    dstAddr1.SetPort(3261);
    _LIT(KText63, "10.1.42.1");
    dstAddr1.Input( KText63 );

    ret = socket1.SetLocalPort(3262);
    TESTL(ret == KErrNone); 

    socket1.Connect(dstAddr1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    dstAddr2.SetPort(3263);
    _LIT(KText64, "10.1.42.2");
    dstAddr2.Input( KText64 );
    ret = socket2.SetLocalPort(3264);
    TESTL(ret == KErrNone); 

    socket2.Connect(dstAddr2, status);
    User::WaitForRequest(status);    
    TESTL(status.Int() == KErrNone); 

    dstAddr3.SetPort(3265);
    _LIT(KText65, "10.1.42.3");
    dstAddr3.Input( KText65 );
    ret = socket3.SetLocalPort(3266);
    TESTL(ret == KErrNone); 

    socket3.Connect(dstAddr3, status);
    User::WaitForRequest(status);    
    TESTL(status.Int() == KErrNone); 

    dstAddr4.SetPort(3267);
    _LIT(KText66, "10.1.42.4");
    dstAddr4.Input( KText66 );
    ret = socket4.SetLocalPort(3268);
    TESTL(ret == KErrNone); 

    socket4.Connect(dstAddr4, status);
    User::WaitForRequest(status);    
    TESTL(status.Int() == KErrNone); 

    dstAddr5.SetPort(3269);
    _LIT(KText67, "10.1.42.5");
    dstAddr5.Input( KText67 );
    ret = socket5.SetLocalPort(3270);
    TESTL(ret == KErrNone); 

    socket5.Connect(dstAddr5, status);
    User::WaitForRequest(status);    
    TESTL(status.Int() == KErrNone); 

    dstAddr6.SetPort(3271);
    _LIT(KText68, "10.1.42.6");
    dstAddr6.Input( KText68 );
    ret = socket6.SetLocalPort(3272);
    TESTL(ret == KErrNone); 

    socket6.Connect(dstAddr6, status);
    User::WaitForRequest(status);    
    TESTL(status.Int() == KErrNone); 

    dstAddr7.SetPort(3273);
    _LIT(KText69, "10.1.42.7");
    dstAddr7.Input( KText69 );
    ret = socket7.SetLocalPort(3274);
    TESTL(ret == KErrNone); 

    socket7.Connect(dstAddr7, status);
    User::WaitForRequest(status);    
    TESTL(status.Int() == KErrNone); 

    dstAddr8.SetPort(3275);
    _LIT(KText70, "10.1.42.8");
    dstAddr8.Input( KText70 );
    ret = socket8.SetLocalPort(3276);
    TESTL(ret == KErrNone); 

    socket8.Connect(dstAddr8, status);
    User::WaitForRequest(status);    
    TESTL(status.Int() == KErrNone); 

    dstAddr9.SetPort(3277);
    _LIT(KText71, "10.1.42.9");
    dstAddr9.Input( KText71 );
    ret = socket9.SetLocalPort(3278);
    TESTL(ret == KErrNone); 

    socket9.Connect(dstAddr9, status);
    User::WaitForRequest(status);    
    TESTL(status.Int() == KErrNone); 

    // Create a new sub-connection 
    ret = subconn.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    TESTL(ret == KErrNone);
    
    // Move the connected socket onto the new sub-connection
    subconn.Add(socket1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Create the container for all subconnection parameters 
    RSubConParameterBundle subconnParams; 
    
    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily = 
        CSubConParameterFamily::NewL(subconnParams, KSubConQoSFamily);
    CleanupDeletePushL(qosFamily);

    // Create the requested/acceptable generic parameter set for QoS 
    // (Qos family takes ownership) 
    CSubConQosGenericParamSet* reqGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::ERequested);
//    CleanupDeletePushL(reqGenericParams);

    CSubConQosGenericParamSet* accGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::EAcceptable);
//    CleanupDeletePushL(accGenericParams);

    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet1(*reqGenericParams);
    SetEsockParamSet1(*accGenericParams);

    // Subscribe for QoS Params notification
    subconn.EventNotification(subconnNotifBuf, ETrue, eventStatus); 
    
    // Set properties of the sub-connection 
    ret = subconn.SetParameters(subconnParams); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn.SetParameters-event 
    // (expected KSubConGenericEventParamsGranted)
    User::WaitForRequest(eventStatus); 
    TESTL(eventStatus.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf, KSubConGenericEventParamsGranted); 
    TESTE(ret == KErrNone, ret);

    // Move the connected socket2 onto the new sub-connection
    subconn.Add(socket2, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Move the connected socket3 onto the new sub-connection
    subconn.Add(socket3, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Move the connected socket4 onto the new sub-connection
    subconn.Add(socket4, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Move the connected socket5 onto the new sub-connection
    subconn.Add(socket5, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Move the connected socket6 onto the new sub-connection
    subconn.Add(socket6, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Move the connected socket7 onto the new sub-connection
    subconn.Add(socket7, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Move the connected socket8 onto the new sub-connection
    subconn.Add(socket8, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Move the connected socket9 onto the new sub-connection
    subconn.Add(socket9, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == EQoSJoinFailure);

    // Clean up
    subconn.Close();
    subconnParams.Close(); 

    socket1.Close();
    socket2.Close();
    socket3.Close();
    socket4.Close();
    socket5.Close();
    socket6.Close();
    socket7.Close();
    socket8.Close();
    socket9.Close();

    conn.Stop();
    conn.Close();

//    CleanupStack::Pop(accGenericParams); 
//    CleanupStack::Pop(reqGenericParams); 
    CleanupStack::Pop(qosFamily); 
    CleanupStack::Pop(&socketServer); 
    socketServer.Close();

    return iTestStepResult;
    }


CQoSTest_143::CQoSTest_143()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText105, "qos_143");
    iTestStepName = KText105;
    }

CQoSTest_143::~CQoSTest_143()
    {
    }

TVerdict CQoSTest_143::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText106, "qos_143 RSubConnection, Happyday scenario");
    Log( KText106 );

    RSocket                 socket1, socket2;
    TInetAddr               dstAddr1, dstAddr2;
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn; 
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf;
    TInt                    ret; 

    // Connect to ESOCK 
    ret = socketServer.Connect();
    CleanupClosePushL(socketServer); 
    TESTL(ret == KErrNone); 

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP sockets 
    ret = socket1.Open(socketServer, 
                       KAfInet, 
                       KSockDatagram, 
                       KProtocolInetUdp, 
                       conn);
    TESTL(ret == KErrNone); 

    ret = socket2.Open(socketServer, 
                       KAfInet, 
                       KSockDatagram, 
                       KProtocolInetUdp, 
                       conn);
    TESTL(ret == KErrNone); 

    dstAddr1.SetPort(3431);
    _LIT(KText63, "10.1.43.1");
    dstAddr1.Input( KText63 );
    ret = socket1.SetLocalPort(3432);
    TESTL(ret == KErrNone); 

    socket1.Connect(dstAddr1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    dstAddr2.SetPort(3433);
    _LIT(KText64, "10.1.43.2");
    dstAddr2.Input( KText64 );
    ret = socket2.SetLocalPort(3434);
    TESTL(ret == KErrNone); 

    socket2.Connect(dstAddr2, status);
    User::WaitForRequest(status);    
    TESTL(status.Int() == KErrNone); 

    // Create a new sub-connection 
    ret = subconn.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket1 onto the new sub-connection
    subconn.Add(socket1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Create the container for all subconnection parameters 
    RSubConParameterBundle subconnParams; 
    
    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily = 
        CSubConParameterFamily::NewL(subconnParams, KSubConQoSFamily);
    CleanupDeletePushL(qosFamily);

    // Create the requested/acceptable generic parameter set for QoS 
    // (Qos family takes ownership) 
    CSubConQosGenericParamSet* reqGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::ERequested);
//    CleanupDeletePushL(reqGenericParams);

    CSubConQosGenericParamSet* accGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::EAcceptable);
//    CleanupDeletePushL(accGenericParams);

    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet1(*reqGenericParams);
    SetEsockParamSet1(*accGenericParams);

    // Subscribe for QoS Params notification
    subconn.EventNotification(subconnNotifBuf, ETrue, eventStatus); 

    // Set properties of the sub-connection 
    ret = subconn.SetParameters(subconnParams); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn.SetParameters-event 
    // (expected KSubConGenericEventParamsGranted)
    User::WaitForRequest(eventStatus); 
    TESTL(eventStatus.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf, KSubConGenericEventParamsGranted); 
    TESTE(ret == KErrNone, ret);

    // Move the connected socket2 onto the new sub-connection
    subconn.Add(socket2, status);
    User::WaitForRequest(status);
    TESTE(status.Int() == KErrNone, status.Int());

    // Remove the connected socket1 from the sub-connection
    subconn.Remove(socket1, status);
    User::WaitForRequest(status);
    TESTE(status.Int() == KErrNone, status.Int());

    // Remove the connected socket2 from the sub-connection
    subconn.Remove(socket2, status);
    User::WaitForRequest(status);
    TESTE(status.Int() == KErrNone, status.Int());

    // Clean up
    subconn.Close();
    subconnParams.Close(); 

    socket1.Close();
    socket2.Close();

    conn.Stop();
    conn.Close();

//    CleanupStack::Pop(accGenericParams); 
//    CleanupStack::Pop(reqGenericParams); 
    CleanupStack::Pop(qosFamily); 
    CleanupStack::Pop(&socketServer); 
    socketServer.Close();

    return iTestStepResult;
    }


CQoSTest_144::CQoSTest_144()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText109, "qos_144");
    iTestStepName = KText109;
    }

CQoSTest_144::~CQoSTest_144()
    {
    }

TVerdict CQoSTest_144::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText110, "qos_144 RSubConnection, Informing of no QoS available for\
 interface");
    Log( KText110 );

    RSocket                 socket;
    TInetAddr               dstAddr;
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn; 
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf;
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
    ret = socket.Open(socketServer, 
                      KAfInet, 
                      KSockDatagram, 
                      KProtocolInetUdp, 
                      conn);
    TESTL(ret == KErrNone); 
   	CleanupClosePushL(socket);
    
    dstAddr.SetPort(3441);
    _LIT(KText50, "127.0.0.1");
    dstAddr.Input( KText50 );

    ret = socket.SetLocalPort(3442);
    TESTL(ret == KErrNone);

    socket.Connect(dstAddr, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 




    // Create a new sub-connection 
    ret = subconn.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    TESTL(ret == KErrNone);
	CleanupClosePushL(subconn);

    // Move the connected socket onto the new sub-connection
    subconn.Add(socket, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNotReady);

	
    // close and destroy
  	CleanupStack::PopAndDestroy(&subconn);
  	CleanupStack::PopAndDestroy(&socket);
    CleanupStack::PopAndDestroy(&conn);
    CleanupStack::PopAndDestroy(&socketServer);

    return iTestStepResult;
    }


CQoSTest_145::CQoSTest_145()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText112, "qos_145");
    iTestStepName = KText112;
    }

CQoSTest_145::~CQoSTest_145()
    {
    }

TVerdict CQoSTest_145::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText113, "qos_145 RSubConnection, Support for multiple \
RSubConnection handles");
    Log( KText113 );

    RSocket                 socket1, socket2, socket3;
    TInetAddr               dstAddr1, dstAddr2, dstAddr3;
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn1, subconn2; 
    TRequestStatus          status, eventStatus1, eventStatus2;
    TNotificationEventBuf   subconnNotifBuf1, subconnNotifBuf2;
    TInt                    ret; 

    // Connect to ESOCK 
    ret = socketServer.Connect(); 
    CleanupClosePushL(socketServer); 
    TESTL(ret == KErrNone); 

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP sockets 
    ret = socket1.Open(socketServer, 
                       KAfInet, 
                       KSockDatagram, 
                       KProtocolInetUdp, 
                       conn);
    TESTL(ret == KErrNone); 

    ret = socket2.Open(socketServer, 
                       KAfInet, 
                       KSockDatagram, 
                       KProtocolInetUdp, 
                       conn);
    TESTL(ret == KErrNone); 

    ret = socket3.Open(socketServer, 
                       KAfInet, 
                       KSockDatagram, 
                       KProtocolInetUdp, 
                       conn);
    TESTL(ret == KErrNone); 

    dstAddr1.SetPort(3451);
    _LIT(KText63, "10.1.45.1");
    dstAddr1.Input( KText63 );

    ret = socket1.SetLocalPort(3452);
    TESTL(ret == KErrNone);
    
    socket1.Connect(dstAddr1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    dstAddr2.SetPort(3453);
    _LIT(KText64, "10.1.45.2");
    dstAddr2.Input( KText64 );
    
    ret = socket2.SetLocalPort(3454);
    TESTL(ret == KErrNone);
    
    socket2.Connect(dstAddr2, status);
    User::WaitForRequest(status);    
    TESTL(status.Int() == KErrNone); 

    dstAddr3.SetPort(3455);
    _LIT(KText65, "10.1.45.3");
    dstAddr3.Input( KText65 );
    
    ret = socket3.SetLocalPort(3456);
    TESTL(ret == KErrNone);
    
    socket3.Connect(dstAddr3, status);
    User::WaitForRequest(status);    
    TESTL(status.Int() == KErrNone); 

    // Create a new sub-connection 
    ret = subconn1.Open(socketServer, 
                        RSubConnection::ECreateNew, 
                        conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket1 onto the new sub-connection
    subconn1.Add(socket1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Create another sub-connection 
    ret = subconn2.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket2 onto the new sub-connection
    subconn2.Add(socket2, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); // -14 = KErrInUse

    // Create the container for all subconnection parameters 
    RSubConParameterBundle subconnParams; 
    
    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily = 
        CSubConParameterFamily::NewL(subconnParams, KSubConQoSFamily);
    CleanupDeletePushL(qosFamily);

    // Create the requested/acceptable generic parameter set for QoS 
    // (Qos family takes ownership) 
    CSubConQosGenericParamSet* reqGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::ERequested);
    CleanupDeletePushL(reqGenericParams);

    CSubConQosGenericParamSet* accGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::EAcceptable);
    CleanupDeletePushL(accGenericParams);

    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet1(*reqGenericParams);
    SetEsockParamSet1(*accGenericParams);

    // Subscribe for QoS Params notification
    subconn1.EventNotification(subconnNotifBuf1, ETrue, eventStatus1); 

    // Set properties of the sub-connection for subconn1
    ret = subconn1.SetParameters(subconnParams); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn1.SetParameters-event 
    // (expected KSubConGenericEventParamsGranted)
    User::WaitForRequest(eventStatus1); 
    TESTL(eventStatus1.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf1, KSubConGenericEventParamsGranted);
    TESTE(ret == KErrNone, ret); 

    // Subscribe for QoS Params notification for subconn2
    subconn2.EventNotification(subconnNotifBuf2, ETrue, eventStatus2); 

    // Set properties of the sub-connection 
    ret = subconn2.SetParameters(subconnParams); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn2.SetParameters-event 
    // (expected KSubConGenericEventParamsGranted)
    User::WaitForRequest(eventStatus2); 
    TESTL(eventStatus2.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf2, KSubConGenericEventParamsGranted);
    TESTE(ret == KErrNone, ret); 

    // Set the requested Generic Parameters
    SetEsockParamSet2(*reqGenericParams);
    SetEsockParamSet2(*accGenericParams);

    subconn2.EventNotification(subconnNotifBuf2, ETrue, eventStatus2); 

    // Set properties of the sub-connection 
    ret = subconn2.SetParameters(subconnParams); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn2.SetParameters-event 
    // (expected KSubConGenericEventParamsGranted)
    User::WaitForRequest(eventStatus2); 
    TESTL(eventStatus2.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf2, KSubConGenericEventParamsGranted);
    TESTE(ret == KErrNone, ret); 

    /*
    // Move the connected socket3 onto the new sub-connection
    subconn2.Add(socket3, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    // Remove the connected socket3 from the sub-connection
    // and close the sub-connection immediately
    subconn2.Remove(socket3, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);
    */
    
    // Clean up
    subconn1.Close();
    subconn2.Close();
    subconnParams.Close(); 

    socket1.Close();
    socket2.Close();
    socket3.Close();

    conn.Stop();
    conn.Close();

    CleanupStack::Pop(accGenericParams); 
    CleanupStack::Pop(reqGenericParams); 
    CleanupStack::Pop(qosFamily); 
    CleanupStack::Pop(&socketServer); 
    socketServer.Close();

    return iTestStepResult;
    }


CQoSTest_146::CQoSTest_146()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText116, "qos_146");
    iTestStepName = KText116;
    }

CQoSTest_146::~CQoSTest_146()
    {
    }

TVerdict CQoSTest_146::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText117, "qos_146 RSubConnection, Multiple secondary PDP contexts");
    Log( KText117 );

    RSocket                 socket1, socket2;
    TInetAddr               dstAddr1, dstAddr2;
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn1, subconn2; 
    TRequestStatus          status, eventStatus1, eventStatus2;
    TNotificationEventBuf   subconnNotifBuf1, subconnNotifBuf2;
    TInt                    ret; 

    // Connect to ESOCK 
    ret = socketServer.Connect();
    CleanupClosePushL(socketServer); 
    TESTL(ret == KErrNone); 

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP sockets 
    ret = socket1.Open(socketServer, 
                       KAfInet, 
                       KSockDatagram, 
                       KProtocolInetUdp, 
                       conn);
    TESTL(ret == KErrNone); 

    ret = socket2.Open(socketServer, 
                       KAfInet, 
                       KSockDatagram, 
                       KProtocolInetUdp, 
                       conn);
    TESTL(ret == KErrNone); 

    dstAddr1.SetPort(3461);
    _LIT(KText63, "10.1.46.1");
    dstAddr1.Input( KText63 );

    ret = socket1.SetLocalPort(3462);
    TESTL(ret == KErrNone);

    socket1.Connect(dstAddr1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    dstAddr2.SetPort(3463);
    _LIT(KText64, "10.1.46.2");
    dstAddr2.Input( KText64 );

    ret = socket2.SetLocalPort(3464);
    TESTL(ret == KErrNone);

    socket2.Connect(dstAddr2, status);
    User::WaitForRequest(status);    
    TESTL(status.Int() == KErrNone); 

    TEST(NifAPIL(socket1, KNotifySecondaryCreated) == KErrNone);

    // Create a new sub-connection 
    ret = subconn1.Open(socketServer, 
                        RSubConnection::ECreateNew, 
                        conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket1 onto the new sub-connection
    subconn1.Add(socket1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Create another sub-connection 
    ret = subconn2.Open(socketServer, 
                        RSubConnection::ECreateNew, 
                        conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket2 onto the new sub-connection
    subconn2.Add(socket2, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Create the container for all subconnection parameters 
    RSubConParameterBundle subconnParams; 
    
    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily = 
        CSubConParameterFamily::NewL(subconnParams, KSubConQoSFamily);
    CleanupDeletePushL(qosFamily);

    // Create the requested/acceptable generic parameter set for QoS 
    // (Qos family takes ownership) 
    CSubConQosGenericParamSet* reqGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::ERequested);
    CleanupDeletePushL(reqGenericParams);

    CSubConQosGenericParamSet* accGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::EAcceptable);
    CleanupDeletePushL(accGenericParams);

    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet1(*reqGenericParams);
    SetEsockParamSet1(*accGenericParams);

    // Subscribe for QoS Params notification
    subconn1.EventNotification(subconnNotifBuf1, ETrue, eventStatus1); 

    // Set properties of the sub-connection for subconn1
    ret = subconn1.SetParameters(subconnParams); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn1.SetParameters-event 
    // (expected KSubConGenericEventParamsGranted)
    User::WaitForRequest(eventStatus1); 
    TESTL(eventStatus1.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf1, KSubConGenericEventParamsGranted);
    TESTE(ret == KErrNone, ret); 

    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet2(*reqGenericParams);
    SetEsockParamSet2(*accGenericParams);

    // Subscribe for QoS Params notification for subconn2
    subconn2.EventNotification(subconnNotifBuf2, ETrue, eventStatus2); 

    // Set properties of the sub-connection 
    ret = subconn2.SetParameters(subconnParams); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn2.SetParameters-event 
    // (expected KSubConGenericEventParamsGranted)
    User::WaitForRequest(eventStatus2); 
    TESTL(eventStatus2.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf2, KSubConGenericEventParamsGranted);
    TESTE(ret == KErrNone, ret);
    
    TEST(CheckTestFile(KTestFile1));
    TEST(CheckTestFile(KTestFile2));

    // Clean up
    subconn1.Close();
    subconn2.Close();
    subconnParams.Close(); 

    // To reset the message values of the testnif
    TEST(NifAPIL(socket1, KResetMessage) == KErrNone);

    socket1.Close();
    socket2.Close();

    conn.Stop();
    conn.Close();

    CleanupStack::Pop(accGenericParams); 
    CleanupStack::Pop(reqGenericParams); 
    CleanupStack::Pop(qosFamily); 
    CleanupStack::Pop(&socketServer); 
    socketServer.Close();

    ClearTestFiles();

    return iTestStepResult;
    }


//
//  Context setting error situations
//


CQoSTest_090::CQoSTest_090()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText120, "qos_090");
    iTestStepName = KText120;
    }

CQoSTest_090::~CQoSTest_090()
    {
    }

TVerdict CQoSTest_090::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText121, "qos_090 SetDefaultQoS fails");
    Log( KText121 );

    TInt err = CreateTestFile(KTestSetDefaultQoSFail);
    if(err != KErrNone)
        {
        _LIT(KText122, "Creating testfile failed, aborting test");
        Log( KText122 );
        return EInconclusive;
        }

    RSocket                 socket;
    TInetAddr               dstAddr;
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn; 
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf;
    TInt                    ret; 

    // Connect to ESOCK 
    ret = socketServer.Connect();
    CleanupClosePushL(socketServer); 
    TESTL(ret == KErrNone); 

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP socket 
    ret = socket.Open(socketServer, 
                      KAfInet, 
                      KSockDatagram, 
                      KProtocolInetUdp, 
                      conn);
    TESTL(ret == KErrNone); 
    
    dstAddr.SetPort(3901);
    _LIT(KText50, "10.1.90.1");
    dstAddr.Input( KText50 );

    socket.Connect(dstAddr, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    // Create a new sub-connection 
    ret = subconn.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket onto the new sub-connection
    subconn.Add(socket, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Create the container for all subconnection parameters 
    RSubConParameterBundle subconnParams; 
    
    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily = 
        CSubConParameterFamily::NewL(subconnParams, KSubConQoSFamily);
    CleanupDeletePushL(qosFamily);

    // Create the requested/acceptable generic parameter set for QoS 
    // (Qos family takes ownership) 
    CSubConQosGenericParamSet* reqGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::ERequested);
    //CleanupDeletePushL(reqGenericParams);

    CSubConQosGenericParamSet* accGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::EAcceptable);
    //CleanupDeletePushL(accGenericParams);

    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet1(*reqGenericParams);
    SetEsockParamSet1(*accGenericParams);

    // Subscribe for QoS Params notification
    subconn.EventNotification(subconnNotifBuf, ETrue, eventStatus); 
    
    // Set properties of the sub-connection 
    ret = subconn.SetParameters(subconnParams); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn.SetParameters-event 
    // (expected KSubConGenericEventParamsGranted)
    User::WaitForRequest(eventStatus); 
    TESTL(eventStatus.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf, KSubConGenericEventParamsGranted); 
    TESTE(ret == KErrNone, ret);

    // Clean up
    subconn.Close();
    subconnParams.Close(); 
    
    socket.Close();
    
    conn.Stop();
    conn.Close();
    
    //CleanupStack::Pop(accGenericParams); 
    //CleanupStack::Pop(reqGenericParams); 
    CleanupStack::Pop(qosFamily); 
    CleanupStack::Pop(&socketServer); 
    socketServer.Close();

    ClearTestFiles();

    return iTestStepResult;
    }


CQoSTest_091::CQoSTest_091()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText124, "qos_091");
    iTestStepName = KText124;
    }

CQoSTest_091::~CQoSTest_091()
    {
    }

TVerdict CQoSTest_091::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText125, "qos_091 Creating secondary context fails synchronously");
    Log( KText125 );

    RSocket                 socket;
    TInetAddr               dstAddr;
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn; 
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf;
    TInt                    ret; 

    // Connect to ESOCK 
    ret = socketServer.Connect();
    CleanupClosePushL(socketServer); 
    TESTL(ret == KErrNone); 

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP socket 
    ret = socket.Open(socketServer, 
                      KAfInet, 
                      KSockDatagram, 
                      KProtocolInetUdp, 
                      conn);
    TESTL(ret == KErrNone); 
    
    dstAddr.SetPort(3901);
    _LIT(KText50, "10.1.91.1");
    dstAddr.Input( KText50 );

    socket.Connect(dstAddr, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    TEST(NifAPIL(socket, KContextCreateFail) == KErrNone);

    // Create a new sub-connection 
    ret = subconn.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket onto the new sub-connection
    subconn.Add(socket, status);
    User::WaitForRequest(status);
    TESTL(status.Int() != KErrNone);

    // Clean up
    // To reset the message values of the testnif
    TEST(NifAPIL(socket, KResetMessage) == KErrNone);

    subconn.Close();
    
    socket.Close();
    
    conn.Stop();
    conn.Close();
    
    CleanupStack::Pop(&socketServer); 
    socketServer.Close();

    return iTestStepResult;
    }


CQoSTest_092::CQoSTest_092()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText127, "qos_092");
    iTestStepName = KText127;
    }

CQoSTest_092::~CQoSTest_092()
    {
    }

TVerdict CQoSTest_092::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText128, "qos_092 Activating context fails synchronously");
    Log( KText128 );

    RSocket                 socket;
    TInetAddr               dstAddr;
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn; 
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf;
    TInt                    ret; 

    // Connect to ESOCK 
    ret = socketServer.Connect();
    CleanupClosePushL(socketServer); 
    TESTL(ret == KErrNone); 

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP socket 
    ret = socket.Open(socketServer, 
                      KAfInet, 
                      KSockDatagram, 
                      KProtocolInetUdp, 
                      conn);
    TESTL(ret == KErrNone); 
    
    dstAddr.SetPort(3901);
    _LIT(KText50, "10.1.92.1");
    dstAddr.Input( KText50 );

    socket.Connect(dstAddr, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    TEST(NifAPIL(socket, KContextActivateFail) == KErrNone);

    // Create a new sub-connection 
    ret = subconn.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket onto the new sub-connection
    subconn.Add(socket, status);
    User::WaitForRequest(status);
    TESTL(status.Int() != KErrNone);

    // Clean up
    // To reset the message values of the testnif
    TEST(NifAPIL(socket, KResetMessage) == KErrNone);

    subconn.Close();
    
    socket.Close();
    
    conn.Stop();
    conn.Close();
    
    CleanupStack::Pop(&socketServer); 
    socketServer.Close();

    return iTestStepResult;
    }


CQoSTest_093::CQoSTest_093()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText130, "qos_093");
    iTestStepName = KText130;
    }

CQoSTest_093::~CQoSTest_093()
    {
    }

TVerdict CQoSTest_093::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText131, "qos_093 Setting QoS fails synchronously");
    Log( KText131 );

    RSocket                 socket;
    TInetAddr               dstAddr;
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn; 
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf;
    TInt                    ret; 

    // Connect to ESOCK 
    ret = socketServer.Connect();
    CleanupClosePushL(socketServer); 
    TESTL(ret == KErrNone); 

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP socket 
    ret = socket.Open(socketServer, 
                      KAfInet, 
                      KSockDatagram, 
                      KProtocolInetUdp, 
                      conn);
    TESTL(ret == KErrNone); 
    
    dstAddr.SetPort(3901);
    _LIT(KText50, "10.1.93.1");
    dstAddr.Input( KText50 );

    socket.Connect(dstAddr, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    TEST(NifAPIL(socket, KContextQoSSetFail) == KErrNone);

    // Create a new sub-connection 
    ret = subconn.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket onto the new sub-connection
    subconn.Add(socket, status);
    User::WaitForRequest(status);
    TESTL(status.Int() != KErrNone);

    // Clean up
    // To reset the message values of the testnif
    TEST(NifAPIL(socket, KResetMessage) == KErrNone);

    subconn.Close();
    
    socket.Close();
    
    conn.Stop();
    conn.Close();
    
    CleanupStack::Pop(&socketServer); 
    socketServer.Close();

    return iTestStepResult;
    }


CQoSTest_094::CQoSTest_094()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText133, "qos_094");
    iTestStepName = KText133;
    }

CQoSTest_094::~CQoSTest_094()
    {
    }

TVerdict CQoSTest_094::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText134, "qos_094 TFT modification fails synchronously");
    Log( KText134 );

    RSocket                 socket;
    TInetAddr               dstAddr;
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn; 
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf;
    TInt                    ret; 

    // Connect to ESOCK 
    ret = socketServer.Connect();
    CleanupClosePushL(socketServer); 
    TESTL(ret == KErrNone); 

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP socket 
    ret = socket.Open(socketServer, 
                      KAfInet, 
                      KSockDatagram, 
                      KProtocolInetUdp, 
                      conn);
    TESTL(ret == KErrNone); 
    
    dstAddr.SetPort(3901);
    _LIT(KText50, "10.1.94.1");
    dstAddr.Input( KText50 );

    socket.Connect(dstAddr, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    TEST(NifAPIL(socket, KContextTFTModifyFail) == KErrNone);

    // Create a new sub-connection 
    ret = subconn.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket onto the new sub-connection
    subconn.Add(socket, status);
    User::WaitForRequest(status);
    TESTL(status.Int() != KErrNone);

    // Clean up
    // To reset the message values of the testnif
    TEST(NifAPIL(socket, KResetMessage) == KErrNone);

    subconn.Close();
    
    socket.Close();
    
    conn.Stop();
    conn.Close();
    
    CleanupStack::Pop(&socketServer); 
    socketServer.Close();

    return iTestStepResult;
    }


CQoSTest_101::CQoSTest_101()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText136, "qos_101");
    iTestStepName = KText136;
    }

CQoSTest_101::~CQoSTest_101()
    {
    }

TVerdict CQoSTest_101::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText137, "qos_101 Creating secondary context fails asynchronously");
    Log( KText137 );

    RSocket                 socket;
    TInetAddr               dstAddr;
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn; 
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf;
    TInt                    ret; 

    // Connect to ESOCK 
    ret = socketServer.Connect();
    CleanupClosePushL(socketServer); 
    TESTL(ret == KErrNone); 

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP socket 
    ret = socket.Open(socketServer, 
                      KAfInet, 
                      KSockDatagram, 
                      KProtocolInetUdp, 
                      conn);
    TESTL(ret == KErrNone); 
    
    dstAddr.SetPort(3901);
    _LIT(KText50, "10.1.101.1");
    dstAddr.Input( KText50 );

    socket.Connect(dstAddr, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    TEST(NifAPIL(socket, KContextCreateFailAsync) == KErrNone);

    // Create a new sub-connection 
    ret = subconn.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket onto the new sub-connection
    subconn.Add(socket, status);
    User::WaitForRequest(status);
    TESTL(status.Int() != KErrNone);

    // Clean up
    // To reset the message values of the testnif
    TEST(NifAPIL(socket, KResetMessage) == KErrNone);

    subconn.Close();
    
    socket.Close();
    
    conn.Stop();
    conn.Close();
    
    CleanupStack::Pop(&socketServer); 
    socketServer.Close();

    return iTestStepResult;
    }


CQoSTest_102::CQoSTest_102()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText139, "qos_102");
    iTestStepName = KText139;
    }

CQoSTest_102::~CQoSTest_102()
    {
    }

TVerdict CQoSTest_102::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText140, "qos_102 Activating context fails asynchronously");
    Log( KText140 );

    RSocket                 socket;
    TInetAddr               dstAddr;
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn; 
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf;
    TInt                    ret; 

    // Connect to ESOCK 
    ret = socketServer.Connect();
    CleanupClosePushL(socketServer); 
    TESTL(ret == KErrNone); 

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP socket 
    ret = socket.Open(socketServer, 
                      KAfInet, 
                      KSockDatagram, 
                      KProtocolInetUdp, 
                      conn);
    TESTL(ret == KErrNone); 
    
    dstAddr.SetPort(3901);
    _LIT(KText50, "10.1.102.1");
    dstAddr.Input( KText50 );

    socket.Connect(dstAddr, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 
    
    /*
    Informing testnif to fail the activation creates KErrTest error code.
    So during activation of the default secondary context, the status returned is
    KErrTest.
    */
    TEST(NifAPIL(socket, KContextActivateFailAsync) == KErrNone);

    
    // Create a new sub-connection 
    ret = subconn.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket onto the new sub-connection
    subconn.Add(socket, status);
    User::WaitForRequest(status);
    TESTE(status.Int() == KErrTest,status.Int());
    
    
    // Clean up
    // To reset the message values of the testnif
    TEST(NifAPIL(socket, KResetMessage) == KErrNone);

    subconn.Close();
    //subconnParams.Close(); 
    
    socket.Close();
    
    conn.Stop();
    conn.Close();
    
    CleanupStack::Pop(&socketServer); 
    socketServer.Close();

    return iTestStepResult;
    }


CQoSTest_103::CQoSTest_103()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText142, "qos_103");
    iTestStepName = KText142;
    }

CQoSTest_103::~CQoSTest_103()
    {
    }

TVerdict CQoSTest_103::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText143, "qos_103 Setting QoS fails asynchronously");
    Log( KText143 );

    RSocket                 socket;
    TInetAddr               dstAddr;
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn; 
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf;
    TInt                    ret; 

    // Connect to ESOCK 
    ret = socketServer.Connect();
    CleanupClosePushL(socketServer); 
    TESTL(ret == KErrNone); 

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP socket 
    ret = socket.Open(socketServer, 
                      KAfInet, 
                      KSockDatagram, 
                      KProtocolInetUdp, 
                      conn);
    TESTL(ret == KErrNone); 
    
    dstAddr.SetPort(3901);
    _LIT(KText50, "10.1.103.1");
    dstAddr.Input( KText50 );

    socket.Connect(dstAddr, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    TEST(NifAPIL(socket, KContextQoSSetFailAsync) == KErrNone);

    // Create a new sub-connection 
    ret = subconn.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket onto the new sub-connection
    subconn.Add(socket, status);
    User::WaitForRequest(status);
    TESTL(status.Int() != KErrNone);

    // Clean up
    // To reset the message values of the testnif
    TEST(NifAPIL(socket, KResetMessage) == KErrNone);

    subconn.Close();
    
    socket.Close();
    
    conn.Stop();
    conn.Close();
    
    CleanupStack::Pop(&socketServer); 
    socketServer.Close();

    return iTestStepResult;
    }


CQoSTest_104::CQoSTest_104()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText145, "qos_104");
    iTestStepName = KText145;
    }

CQoSTest_104::~CQoSTest_104()
    {
    }

TVerdict CQoSTest_104::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText146, "qos_104 TFT modification fails asynchronously");
    Log( KText146 );

    RSocket                 socket;
    TInetAddr               dstAddr;
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn; 
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf;
    TInt                    ret; 

    // Connect to ESOCK 
    ret = socketServer.Connect();
    CleanupClosePushL(socketServer); 
    TESTL(ret == KErrNone); 

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP socket 
    ret = socket.Open(socketServer, 
                      KAfInet, 
                      KSockDatagram, 
                      KProtocolInetUdp, 
                      conn);
    TESTL(ret == KErrNone); 
    
    dstAddr.SetPort(3901);
    _LIT(KText50, "10.1.104.1");
    dstAddr.Input( KText50 );

    socket.Connect(dstAddr, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    TEST(NifAPIL(socket, KContextTFTModifyFailAsync) == KErrNone);

    // Create a new sub-connection 
    ret = subconn.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket onto the new sub-connection
    subconn.Add(socket, status);
    User::WaitForRequest(status);
    TESTL(status.Int() != KErrNone);

    // Clean up
    // To reset the message values of the testnif
    TEST(NifAPIL(socket, KResetMessage) == KErrNone);

    subconn.Close();
    
    socket.Close();
    
    conn.Stop();
    conn.Close();
    
    CleanupStack::Pop(&socketServer); 
    socketServer.Close();

    return iTestStepResult;
    }


CQoSTest_902::CQoSTest_902()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText223, "qos_902");
    iTestStepName = KText223;
    }

CQoSTest_902::~CQoSTest_902()
    {
    }

TVerdict CQoSTest_902::doTestStepL( void )
    {
    //
    //  !!! NOTICE !!!
    //  This case has to be run separately from all of the other cases
    //
    _LIT(KText224, "qos_902, Drop PDP context");
    Log( KText224 );

    _LIT(KQoSSimNoUse, "c:\\system\\data\\UmtsSim_Server_simconf.902.ini");
    _LIT(KQoSSimInUse, "c:\\system\\data\\UmtsSim_Server_simconf.ini");

    RFs fs;
    fs.Connect();

    TInt err = fs.Rename(KQoSSimNoUse, KQoSSimInUse);
    if(err != KErrNone)
        {
        _LIT(KText225, "Renaming file didn't succeed");
        Log( KText225 );
        return EInconclusive;
        }

    RSocket                 socket;
    TInetAddr               dstAddr;
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn; 
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf, subconnNotifBuf2;
    TInt                    ret; 

    // Connect to ESOCK 
    ret = socketServer.Connect();
    CleanupClosePushL(socketServer); 
    TESTL(ret == KErrNone); 

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP socket 
    ret = socket.Open(socketServer, 
                      KAfInet, 
                      KSockDatagram, 
                      KProtocolInetUdp, 
                      conn);
    TESTL(ret == KErrNone); 
    
    dstAddr.SetPort(39021);
    _LIT(KText50, "10.90.2.1");
    dstAddr.Input( KText50 );

    socket.Connect(dstAddr, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    // Create a new sub-connection 
    ret = subconn.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket onto the new sub-connection
    subconn.Add(socket, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Create the container for all subconnection parameters 
    RSubConParameterBundle subconnParams; 
    
    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily = 
        CSubConParameterFamily::NewL(subconnParams, KSubConQoSFamily);
    CleanupDeletePushL(qosFamily);

    // Create the requested/acceptable generic parameter set for QoS 
    // (Qos family takes ownership) 
    CSubConQosGenericParamSet* reqGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::ERequested);
    //CleanupDeletePushL(reqGenericParams);

    CSubConQosGenericParamSet* accGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::EAcceptable);
    //CleanupDeletePushL(accGenericParams);

    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet1(*reqGenericParams);
    SetEsockParamSet1(*accGenericParams);

    // Subscribe for QoS Params notification
    subconn.EventNotification(subconnNotifBuf, ETrue, eventStatus); 
    
    // Set properties of the sub-connection 
    ret = subconn.SetParameters(subconnParams); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn.SetParameters-event 
    // (expected KSubConGenericEventParamsGranted)
    User::WaitForRequest(eventStatus); 
    TESTL(eventStatus.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf, KSubConGenericEventParamsGranted); 
    TESTE(ret == KErrNone, ret);

	TRAP(err, iEventListener = CEventListener::NewL());

    subconn.EventNotification(subconnNotifBuf2,  
                              ETrue, 
                              iEventListener->iStatus);

    // Umtssim will be dropping the packets, wait for the event
    // (expected KSubConGenericEventParamsRejected)
   	WaitForSubConnEventL();
    TESTE(iEventListener->iStatus.Int() == KErrNone, 
          iEventListener->iStatus.Int());
    ret = CheckEvent(subconnNotifBuf2, 
                     KSubConGenericEventParamsRejected); 
    TESTE(ret == KErrNone, ret);

    // Clean up
    err = fs.Rename(KQoSSimInUse, KQoSSimNoUse);
    if(err != KErrNone)
        {
        _LIT(KText227, "Naming. UmtsSim_Server_simconf.ini back to \
UmtsSim_Server_simconf.ini.nouse failed!!! Do it manually before running \
any other cases");
        Log( KText227 );
        return EAbort;
        }

    subconn.Close();
    subconnParams.Close(); 
    
    socket.Close();
    
    conn.Stop();
    conn.Close();
    
    //CleanupStack::Pop(accGenericParams); 
    //CleanupStack::Pop(reqGenericParams); 
    CleanupStack::Pop(qosFamily); 
    CleanupStack::Pop(&socketServer); 
    socketServer.Close();

    return iTestStepResult;
    }


CQoSTest_052::CQoSTest_052()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText154, "qos_052");
    iTestStepName = KText154;
    }

CQoSTest_052::~CQoSTest_052()
    {
    }

TVerdict CQoSTest_052::doTestStepL( void )
    {
        
    User::After(KInterTestDelay);
    _LIT(KText302, "qos_052 RQoSPolicy, SetQoS to upgrade parameters, verify \
through GetQoS");
    Log( KText302 );

    RSocketServ socketServer;
    RSocket socket;
    TInetAddr dstAddr;
    TRequestStatus status;
    
    socketServer.Connect();
    socket.Open(socketServer, KAfInet, KSockDatagram, KProtocolInetUdp);
    dstAddr.SetPort(3521);
    _LIT(KText303, "10.1.52.1");
    dstAddr.Input( KText303 );
    socket.Connect(dstAddr, status);
    User::WaitForRequest(status);

    TQoSSelector selector;
    selector.SetAddr(socket);
    RQoSPolicy policy;
    policy.Open(selector);
    policy.NotifyEvent(*this);
    CQoSParameters *parameters = new(ELeave) CQoSParameters();
    SetQoSParamSet2(*parameters);
    TEST(policy.SetQoS(*parameters) == KErrNone);
    WaitForQoSEventL();
    TESTE(iReceivedEvent == EQoSEventAddPolicy, iReceivedEvent);
    TESTE(iReason == KErrNone, iReason);

    WaitForQoSEventL();
    TESTE(iReceivedEvent == EQoSEventConfirm, iReceivedEvent);

    SetQoSParamSet1(*parameters);
    TEST(policy.SetQoS(*parameters) == KErrNone);
    WaitForQoSEventL();
    TESTE(iReceivedEvent == EQoSEventAddPolicy, iReceivedEvent);
    if(iReceivedEvent == EQoSEventAddPolicy)
        {
        TEST(iParameters->GetUplinkBandwidth() == parameters->GetUplinkBandwidth());
        TEST(iParameters->GetUpLinkMaximumBurstSize() == 
            parameters->GetUpLinkMaximumBurstSize());
        TEST(iParameters->GetUpLinkMaximumPacketSize() == 
            parameters->GetUpLinkMaximumPacketSize());
        TEST(iParameters->GetUpLinkAveragePacketSize() == 
            parameters->GetUpLinkAveragePacketSize());
        TEST(iParameters->GetUpLinkDelay() == parameters->GetUpLinkDelay());
        TEST(iParameters->GetUpLinkPriority() == parameters->GetUpLinkPriority());

        TEST(iParameters->GetDownlinkBandwidth() == 
            parameters->GetDownlinkBandwidth());
        TEST(iParameters->GetDownLinkMaximumBurstSize() == 
            parameters->GetDownLinkMaximumBurstSize());
        TEST(iParameters->GetDownLinkMaximumPacketSize() == 
            parameters->GetDownLinkMaximumPacketSize());
        TEST(iParameters->GetDownLinkAveragePacketSize() == 
            parameters->GetDownLinkAveragePacketSize());
        TEST(iParameters->GetDownLinkDelay() == parameters->GetDownLinkDelay());
        TEST(iParameters->GetDownLinkPriority() == 
            parameters->GetDownLinkPriority());

        }

    WaitForQoSEventL();
    TESTE(iReceivedEvent == EQoSEventConfirm, iReceivedEvent);

    TEST(policy.GetQoS() == KErrNone);
    WaitForQoSEventL();
    TESTE(iReceivedEvent == EQoSEventGetPolicy, iReceivedEvent);
    if(iReceivedEvent == EQoSEventGetPolicy)
        {
          TEST(iParameters->GetUplinkBandwidth() == parameters->GetUplinkBandwidth());
        TEST(iParameters->GetUpLinkMaximumBurstSize() == 
            parameters->GetUpLinkMaximumBurstSize());
        TEST(iParameters->GetUpLinkMaximumPacketSize() == 
            parameters->GetUpLinkMaximumPacketSize());
        TEST(iParameters->GetUpLinkAveragePacketSize() == 
            parameters->GetUpLinkAveragePacketSize());
        TEST(iParameters->GetUpLinkDelay() == parameters->GetUpLinkDelay());
        TEST(iParameters->GetUpLinkPriority() == parameters->GetUpLinkPriority());

        TEST(iParameters->GetDownlinkBandwidth() == 
            parameters->GetDownlinkBandwidth());
        TEST(iParameters->GetDownLinkMaximumBurstSize() == 
            parameters->GetDownLinkMaximumBurstSize());
        TEST(iParameters->GetDownLinkMaximumPacketSize() == 
            parameters->GetDownLinkMaximumPacketSize());
        TEST(iParameters->GetDownLinkAveragePacketSize() == 
            parameters->GetDownLinkAveragePacketSize());
        TEST(iParameters->GetDownLinkDelay() == parameters->GetDownLinkDelay());
        TEST(iParameters->GetDownLinkPriority() == 
            parameters->GetDownLinkPriority());

        }

        TEST(policy.CancelNotifyEvent(*this) == KErrNone);
        TEST(policy.Close() == KErrNone);
        socket.Close();
        socketServer.Close();
        delete parameters;
        return iTestStepResult;
    
    }


CQoSTest_059::CQoSTest_059()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText180, "qos_059");
    iTestStepName = KText180;
    }

CQoSTest_059::~CQoSTest_059()
    {
    }

TVerdict CQoSTest_059::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText181, "qos_059 RQoSPolicy, LoadPolicyFileL()"); 
    Log( KText181 );

    TQoSSelector selector;
    selector.SetProtocol(17);
    
    _LIT(KPolicyFile, "c:\\qospolicieslegacy.ini");
    RQoSPolicy policy;
    policy.Open(selector);
    policy.NotifyEvent(*this);
    TEST(policy.LoadPolicyFile(KPolicyFile) == KErrNone);
    WaitForQoSEventL();
    TESTE(iReceivedEvent == EQoSEventLoadPolicyFile, iReceivedEvent);
    TESTE(iReason == KErrNone, iReason);

    if(iReason == KErrNotFound)
        {
        _LIT(KText182, "Policyfile qospolicieslegacy.ini not found!");
        Log( KText182, iReason);
        return EInconclusive;
        }

    if(iReceivedEvent == EQoSEventLoadPolicyFile)
        {
        TEST(policy.GetQoS() == KErrNone);
        WaitForQoSEventL();
        TESTE(iReceivedEvent == EQoSEventGetPolicy, iReceivedEvent);
        TESTE(iReason == KErrNone, iReason);

        if(iReceivedEvent == EQoSEventGetPolicy && iReason == KErrNone)
            {
            // Test we get the parameters that are defined in 
            // qospolicieslegacy.ini

            TEST(iParameters->GetUplinkBandwidth() == 1024);
            TEST(iParameters->GetUpLinkMaximumBurstSize() == 896);
            TEST(iParameters->GetUpLinkMaximumPacketSize() == 768);
            TEST(iParameters->GetUpLinkAveragePacketSize() == 0);
            TEST(iParameters->GetUpLinkDelay() == 0);
            TEST(iParameters->GetUpLinkPriority() == 3);

            TEST(iParameters->GetDownlinkBandwidth() == 1025);
            TEST(iParameters->GetDownLinkMaximumBurstSize() == 897);
            TEST(iParameters->GetDownLinkMaximumPacketSize() == 769);
            TEST(iParameters->GetDownLinkAveragePacketSize() == 1);
            TEST(iParameters->GetDownLinkDelay() == 1);
            TEST(iParameters->GetDownLinkPriority() == 4);
            }
        }

    TEST(policy.UnloadPolicyFile(KPolicyFile) == KErrNone);
    WaitForQoSEventL();
    TESTE(iReceivedEvent == EQoSEventUnloadPolicyFile, iReceivedEvent);
    TESTE(iReason == KErrNone, iReason);

    TEST(policy.CancelNotifyEvent(*this) == KErrNone);
    TEST(policy.Close() == KErrNone);

    return iTestStepResult;
    }


CQoSTest_202::CQoSTest_202()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText285, "qos_202");
    iTestStepName = KText285;
    }

CQoSTest_202::~CQoSTest_202()
    {
    }


TVerdict CQoSTest_202::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText286, "qos_202, ");
    Log( KText286 );
    _LIT(KText287, "REQ305.9: Passing Application UID to QoS framework.");
    Log( KText287 );
    
    RSocketServ socketServer;
    RSocket socket;
    TInetAddr dstAddr;
    TRequestStatus status;
    
    socketServer.Connect();
    socket.Open(socketServer, KAfInet, KSockDatagram, KProtocolInetUdp);

    dstAddr.SetPort(32021);
    _LIT(KText288, "10.20.2.1");
    dstAddr.Input( KText288 );
    
    socket.SetLocalPort(32021);
    socket.Connect(dstAddr, status);

    User::WaitForRequest(status);
        
    
    TQoSSelector selector;
    selector.SetProtocol(17);
 
     _LIT(KPolicyFile, "c:\\qospoliciesuid.ini");
    RQoSPolicy policy;
    policy.Open(selector);
    policy.NotifyEvent(*this);
    TEST(policy.LoadPolicyFile(KPolicyFile) == KErrNone);
    WaitForQoSEventL();


    TESTE(iReceivedEvent == EQoSEventLoadPolicyFile, iReceivedEvent);
    TESTE(iReason == KErrNone, iReason);

    if(iReason == KErrNotFound)
        {
        _LIT(KText289, "Policyfile qospoliciesuid.ini not found!");
        Log( KText289, iReason);
        return EInconclusive;
        }

    _LIT(KText290, "Calling GetQoS() - the policy used must match what we \
specified");
    Log( KText290 );
    TEST(policy.GetQoS() == KErrNone);
    WaitForQoSEventL();
    TESTE(iReceivedEvent == EQoSEventGetPolicy, iReceivedEvent);
    TESTE(iReason == KErrNone, iReason);

    if(iReceivedEvent == EQoSEventGetPolicy && iReason == KErrNone)
        {
        // Test we get the parameters that are defined in qospoliciesuid.ini
        TEST(iParameters->GetUplinkBandwidth() == 1024);
        TEST(iParameters->GetUpLinkMaximumBurstSize() == 896);
        TEST(iParameters->GetUpLinkMaximumPacketSize() == 768);
        TEST(iParameters->GetUpLinkAveragePacketSize() == 0);
        TEST(iParameters->GetUpLinkDelay() == 0);
        TEST(iParameters->GetUpLinkPriority() == 3);

        TEST(iParameters->GetDownlinkBandwidth() == 1025);
        TEST(iParameters->GetDownLinkMaximumBurstSize() == 897);
        TEST(iParameters->GetDownLinkMaximumPacketSize() == 769);
        TEST(iParameters->GetDownLinkAveragePacketSize() == 1);
        TEST(iParameters->GetDownLinkDelay() == 1);
        TEST(iParameters->GetDownLinkPriority() == 4);
        }

    TEST(policy.UnloadPolicyFile(KPolicyFile) == KErrNone);
    WaitForQoSEventL();
    TESTE(iReceivedEvent == EQoSEventUnloadPolicyFile, iReceivedEvent);
    TESTE(iReason == KErrNone, iReason);

    TEST(policy.CancelNotifyEvent(*this) == KErrNone);
    TEST(policy.Close() == KErrNone);

    socket.Close();
    socketServer.Close();

    return iTestStepResult;
    }

/*
CQoSTest_300::CQoSTest_300()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText48, "qos_300");
    iTestStepName = KText48;
    }

CQoSTest_300::~CQoSTest_300()
    {
    }


TVerdict CQoSTest_300::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText49, "qos_300 SBLP test");
    Log( KText49 );

    TSblpParameters sblpParam;
    sblpParam.SetMAT(_S8("http://www.symbian.com"));
    
    TAuthToken authToken;
    sblpParam.GetMAT(authToken);
    //TEST(authToken.Compare(_S8("http://www.symbian.com")) == 0);

    RArray<TSblpParameters::TFlowIdentifier> flowIds;
    
    TSblpParameters::TFlowIdentifier flowId1;

    flowId1.iMediaComponentNumber = 1;
    flowId1.iIPFlowNumber=101;
    flowIds.Append(flowId1);

    TSblpParameters::TFlowIdentifier flowId2;

    flowId2.iMediaComponentNumber = 2;
    flowId2.iIPFlowNumber=102;
    flowIds.Append(flowId2);


    sblpParam.SetFlowIds(flowIds);

    TBuf<255> label;
    label.Copy(authToken);
    Log( _L("media authorization   : %S"), &label);

    RArray<TSblpParameters::TFlowIdentifier> flowIdsRcvd;
    sblpParam.GetFlowIds(flowIdsRcvd);

    TInt i;
    for(i=0; i<flowIdsRcvd.Count();i++)
    {
        Log( _L("media component number: %d"), flowIdsRcvd[i].iMediaComponentNumber);
        Log( _L("IP flow number        : %d"), flowIdsRcvd[i].iIPFlowNumber);
    }



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
    _LIT(KText49, "qos_301 SBLP test");
    Log( KText49 );

    TSblpParameters sblpParam;
    sblpParam.SetMAT(_S8("http://www.symbian.com gggggggggggggggggggggggggg H"));
    
    TAuthorizationToken authToken;
    sblpParam.GetMAT(authToken);
    //TEST(authToken.Compare(_S8("http://www.symbian.com")) == 0);

    RArray<TSblpParameters::TFlowIdentifier> flowIds;
    
    TSblpParameters::TFlowIdentifier flowIdTemp;
    for(TUint16 j = 0; j<90; j++)
    {
        flowIdTemp.iMediaComponentNumber = static_cast< TUint16 >(j+100);
        flowIdTemp.iIPFlowNumber = static_cast< TUint16 >(j+1000);
        flowIds.Append(flowIdTemp);    
    }
    
    
    TSblpParameters::TFlowIdentifier flowId1;

    flowId1.iMediaComponentNumber = 1;
    flowId1.iIPFlowNumber=101;
    flowIds.Append(flowId1);

    TSblpParameters::TFlowIdentifier flowId2;

    flowId2.iMediaComponentNumber = 2;
    flowId2.iIPFlowNumber=102;
    flowIds.Append(flowId2);


    sblpParam.SetFlowIds(flowIds);

    Log( _L("\n"));
    Log( _L("Original values"));
    TBuf<255> label;
    label.Copy(authToken);
    Log( _L("media authorization   : %S"), &label);

    RArray<TSblpParameters::TFlowIdentifier> flowIdsRcvd;
    sblpParam.GetFlowIds(flowIdsRcvd);

    TInt i;
    for(i=0; i<flowIdsRcvd.Count();i++)
    {
        Log( _L("media component number: %d"), flowIdsRcvd[i].iMediaComponentNumber);
        Log( _L("IP flow number        : %d"), flowIdsRcvd[i].iIPFlowNumber);
    }


    CSblpPolicy *sblpPolicy = CSblpPolicy::NewL();
    
    sblpPolicy->SetSblpParameters(sblpParam);

    TSblpParameters param;
    sblpPolicy->GetSblpParameters(param);


    HBufC8* data = HBufC8::NewL(8192);
    TPtr8 tmp(data->Des());
    TPtr8 bufPtr(0,0);
    bufPtr.Set(tmp);

    bufPtr = sblpPolicy->Data();
    
    CSblpPolicy *sblpPolicyTest = CSblpPolicy::NewL();

    // print to check the parsing
    TSblpParameters sblpParamParsed;
    
    sblpPolicyTest->ParseMessage(bufPtr);
    
    sblpPolicyTest->GetSblpParameters(sblpParamParsed);
    
    Log( _L("\n"));
    Log( _L("Parsed values"));
    sblpParamParsed.GetMAT(authToken);
    label.Copy(authToken);
    Log( _L("media authorization   : %S"), &label);

    RArray<TSblpParameters::TFlowIdentifier> flowIdsParsed;
    sblpParamParsed.GetFlowIds(flowIdsParsed);

    for(i=0; i<flowIdsParsed.Count();i++)
    {
        Log( _L("media component number: %d"), flowIdsParsed[i].iMediaComponentNumber);
        Log( _L("IP flow number        : %d"), flowIdsParsed[i].iIPFlowNumber);
    }
    
    
    delete sblpPolicyTest;
    delete sblpPolicy;

    return iTestStepResult;
    }
*/

CQoSTest_700::CQoSTest_700()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText313, "qos_700");
    iTestStepName = KText313;
    }


CQoSTest_700::~CQoSTest_700()
    {
    }

TVerdict CQoSTest_700::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText314, 
        "qos_700 SBLP, Basic SBLP test");
    Log( KText314 );
    
    RSocketServ             socketServer;
    RSocket                 socket1;
    TInetAddr               dstAddr1;
    RConnection             conn; 
    RSubConnection          subconn1; 
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf;
    TInt                    ret; 
    
    // Connect to ESOCK 
    ret = socketServer.Connect();
    CleanupClosePushL(socketServer); 
    TESTL(ret == KErrNone); 

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP sockets 
    ret = socket1.Open(socketServer, 
                       KAfInet, 
                       KSockDatagram, 
                       KProtocolInetUdp, 
                       conn);
    TESTL(ret == KErrNone);

    dstAddr1.SetPort(3461);
    _LIT(KText315, "10.1.46.1");
    dstAddr1.Input( KText315 );

    socket1.Connect(dstAddr1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    TEST(NifAPIL(socket1, KNotifySecondaryCreated) == KErrNone);

    // Create a new sub-connection 
    ret = subconn1.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket onto the new sub-connection
    subconn1.Add(socket1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Create the container for all subconnection parameters 
    RSubConParameterBundle sblpParams;

    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily = 
        CSubConParameterFamily::NewL(sblpParams, KSubConQoSFamily);
    CleanupDeletePushL(qosFamily);

    // Create the requested/acceptable generic parameter set for QoS 
    // (Qos family takes ownership) 
    CSubConQosGenericParamSet* reqGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::ERequested);
//    CleanupDeletePushL(reqGenericParams);

    CSubConQosGenericParamSet* accGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::EAcceptable);
//    CleanupDeletePushL(accGenericParams);

    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet1(*reqGenericParams);
    SetEsockParamSet1(*accGenericParams);

    CSubConParameterFamily* sblpFamily = 
        CSubConParameterFamily::NewL(sblpParams, KSubConAuthorisationFamily);
    CleanupDeletePushL(sblpFamily);

    // Create and Add SBLP Extension Parameters
    CSubConSBLPR5ExtensionParamSet* sblpExtnParamSet = 
        CSubConSBLPR5ExtensionParamSet::NewL(*sblpFamily, 
        CSubConParameterFamily::ERequested);
//    CleanupDeletePushL(sblpExtnParamSet);

    sblpExtnParamSet->SetMAT(_S8("http://www.symbian.com//test_case_700//")); 
    
    TFlowId flowId;
    TUint16 mcn = 100;
    TUint16 ifn = 1000;
    flowId.SetMediaComponentNumber(mcn);
    flowId.SetIPFlowNumber(ifn);
    
    sblpExtnParamSet->AddFlowIdL( flowId );

    // Subscribe for QoS Params notification
    subconn1.EventNotification(subconnNotifBuf, ETrue, eventStatus); 

    // Set properties of the sub-connection 
    ret = subconn1.SetParameters(sblpParams); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn.SetParameters-event 
    // (expected KSubConGenericEventParamsGranted)
    User::WaitForRequest(eventStatus); 
    TESTL(eventStatus.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf, KSubConGenericEventParamsGranted); 
    TESTE(ret == KErrNone, ret);

    // This checks that secondary context number one was created
    TEST(CheckTestFile(KTestFile1));
    
    // Check that sblp parameter has been added to the 
    // current context
    TEST(CheckTestFile(KTestSblpAdd));
    

    // Clean up
    subconn1.Close();
    sblpParams.Close(); 

    // To reset the message values of the testnif
    TEST(NifAPIL(socket1, KResetMessage) == KErrNone);

    socket1.Close();
    
    conn.Stop();
    conn.Close();

//    CleanupStack::Pop(sblpExtnParamSet);
    CleanupStack::Pop(sblpFamily); 
//    CleanupStack::Pop(accGenericParams); 
//    CleanupStack::Pop(reqGenericParams); 
    CleanupStack::Pop(qosFamily); 
    CleanupStack::Pop(&socketServer); 
    socketServer.Close();

    ClearTestFiles();

    return iTestStepResult;
    }


CQoSTest_701::CQoSTest_701()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText313, "qos_701");
    iTestStepName = KText313;
    }


CQoSTest_701::~CQoSTest_701()
    {
    }

TVerdict CQoSTest_701::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText314, 
        "qos_701 SBLP, Establish a new secondary PDP context for SBLP");
    Log( KText314 );

    RSocketServ             socketServer;
    RSocket                 socket1, socket2;
    TInetAddr               dstAddr1, dstAddr2;
    RConnection             conn; 
    RSubConnection          subconn1, subconn2; 
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf1, subconnNotifBuf2;
    TInt                    ret; 

    // Connect to ESOCK 
    ret = socketServer.Connect();
    CleanupClosePushL(socketServer); 
    TESTL(ret == KErrNone); 

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP sockets 
    ret = socket1.Open(socketServer, 
                       KAfInet, 
                       KSockDatagram, 
                       KProtocolInetUdp, 
                       conn);
    TESTL(ret == KErrNone);

    ret = socket2.Open(socketServer, 
                       KAfInet, 
                       KSockDatagram, 
                       KProtocolInetUdp, 
                       conn);
    TESTL(ret == KErrNone);

    dstAddr1.SetPort(3461);
    _LIT(KText315, "10.1.46.1");
    dstAddr1.Input( KText315 );

    socket1.Connect(dstAddr1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 
    
    dstAddr2.SetPort(3462);
    _LIT(KText316, "10.1.46.2");
    dstAddr2.Input( KText316 );

    socket2.Connect(dstAddr2, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    TEST(NifAPIL(socket1, KNotifySecondaryCreated) == KErrNone);

    // Create a new sub-connection 
    ret = subconn1.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket onto the new sub-connection
    subconn1.Add(socket1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Create another instance of RSubConnection
    ret = subconn2.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket onto the new sub-connection
    subconn2.Add(socket2, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Create the container for all subconnection parameters 
    RSubConParameterBundle subconnParams; 
    
    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily = 
        CSubConParameterFamily::NewL(subconnParams, KSubConQoSFamily);
    CleanupDeletePushL(qosFamily);

    // Create the requested/acceptable generic parameter set for QoS 
    // (Qos family takes ownership) 
    CSubConQosGenericParamSet* reqGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::ERequested);
//    CleanupDeletePushL(reqGenericParams);

    CSubConQosGenericParamSet* accGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::EAcceptable);
//    CleanupDeletePushL(accGenericParams);

    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet1(*reqGenericParams);
    SetEsockParamSet1(*accGenericParams);

    // Subscribe for QoS Params notification
    subconn1.EventNotification(subconnNotifBuf1, ETrue, eventStatus); 

    // Set properties of the sub-connection 
    ret = subconn1.SetParameters(subconnParams); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn1.SetParameters-event 
    // (expected KSubConGenericEventParamsGranted)
    User::WaitForRequest(eventStatus); 
    TESTL(eventStatus.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf1, KSubConGenericEventParamsGranted); 
    TESTE(ret == KErrNone, ret);

    // Create another container for all subconnection parameters 
    RSubConParameterBundle sblpParams;

    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily2 = 
        CSubConParameterFamily::NewL(sblpParams, KSubConQoSFamily);
    CleanupDeletePushL(qosFamily2);

    CSubConQosGenericParamSet* reqGenericParams2 = 
        CSubConQosGenericParamSet::NewL(*qosFamily2, 
                                        CSubConParameterFamily::ERequested);
//    CleanupDeletePushL(reqGenericParams2);

    CSubConQosGenericParamSet* accGenericParams2 = 
        CSubConQosGenericParamSet::NewL(*qosFamily2, 
                                        CSubConParameterFamily::EAcceptable);
//    CleanupDeletePushL(accGenericParams2);

    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet1(*reqGenericParams2);
    SetEsockParamSet1(*accGenericParams2);
    
    CSubConParameterFamily* sblpFamily = 
        CSubConParameterFamily::NewL(sblpParams, KSubConAuthorisationFamily);
    CleanupDeletePushL(sblpFamily);

    // Create and Add SBLP Extension Parameters
    CSubConSBLPR5ExtensionParamSet* sblpParamSet = 
        CSubConSBLPR5ExtensionParamSet::NewL(*sblpFamily, 
        CSubConParameterFamily::ERequested);
//    CleanupDeletePushL(sblpParamSet);

    sblpParamSet->SetMAT(_S8("http://www.symbian.com//test_case_701//")); 
    
    TFlowId flowId;
    TUint16 mcn = 100;
    TUint16 ifn = 1000;
    flowId.SetMediaComponentNumber(mcn);
    flowId.SetIPFlowNumber(ifn);

    sblpParamSet->AddFlowIdL( flowId );

    subconn2.EventNotification(subconnNotifBuf2, ETrue, eventStatus); 

    // Set properties of the sub-connection 
    ret = subconn2.SetParameters(sblpParams); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn2.SetParameters-event 
    // (expected KSubConGenericEventParamsGranted)
    User::WaitForRequest(eventStatus); 
    TESTL(eventStatus.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf2, KSubConGenericEventParamsGranted); 
    TESTE(ret == KErrNone, ret);
    
    // Check that sblp parameter has been added to the 
    // current context
    TEST(CheckTestFile(KTestSblpAdd));
    

    RSubConParameterBundle bundle;
    ret = subconn2.GetParameters(bundle);
    TESTL(ret == KErrNone);
    
    CSubConParameterFamily* genFamily = bundle.FindFamily(
                                            KSubConQoSFamily);
    if(genFamily)
        {
        CSubConQosGenericParamSet* requestedGeneric = 
            (CSubConQosGenericParamSet*)genFamily->GetGenericSet(
            CSubConParameterFamily::ERequested);
        
        if(requestedGeneric)
            {
            // Check that requested generic parameters are correct 
            TEST(requestedGeneric->GetUplinkBandwidth() == 
                 reqGenericParams2->GetUplinkBandwidth());
            TEST(requestedGeneric->GetUpLinkMaximumBurstSize() == 
                 reqGenericParams2->GetUpLinkMaximumBurstSize());
            TEST(requestedGeneric->GetUpLinkMaximumPacketSize() == 
                 reqGenericParams2->GetUpLinkMaximumPacketSize());
            TEST(requestedGeneric->GetUpLinkAveragePacketSize() == 
                 reqGenericParams2->GetUpLinkAveragePacketSize());
            TEST(requestedGeneric->GetUpLinkDelay() == 
                 reqGenericParams2->GetUpLinkDelay());
            TEST(requestedGeneric->GetUpLinkPriority() == 
                 reqGenericParams2->GetUpLinkPriority());
            TEST(requestedGeneric->GetDownlinkBandwidth() == 
                 reqGenericParams2->GetDownlinkBandwidth());
            TEST(requestedGeneric->GetDownLinkMaximumBurstSize() == 
                 reqGenericParams2->GetDownLinkMaximumBurstSize());
            TEST(requestedGeneric->GetDownLinkMaximumPacketSize() == 
                 reqGenericParams2->GetDownLinkMaximumPacketSize());
            TEST(requestedGeneric->GetDownLinkAveragePacketSize() == 
                 reqGenericParams2->GetDownLinkAveragePacketSize());
            TEST(requestedGeneric->GetDownLinkDelay() == 
                 reqGenericParams2->GetDownLinkDelay());
            TEST(requestedGeneric->GetDownLinkPriority() == 
                 reqGenericParams2->GetDownLinkPriority());
            TEST(requestedGeneric->GetHeaderMode() == 
                 reqGenericParams2->GetHeaderMode());
            }

        CSubConQosGenericParamSet* acceptbleGeneric = 
            (CSubConQosGenericParamSet*)genFamily->GetGenericSet(
            CSubConParameterFamily::EAcceptable);
        
        if(acceptbleGeneric)
            {
            // Check that acceptable generic parameters are correct 
            TEST(acceptbleGeneric->GetUplinkBandwidth() == 
                 accGenericParams2->GetUplinkBandwidth());
            TEST(acceptbleGeneric->GetUpLinkMaximumBurstSize() == 
                 accGenericParams2->GetUpLinkMaximumBurstSize());
            TEST(acceptbleGeneric->GetUpLinkMaximumPacketSize() == 
                 accGenericParams2->GetUpLinkMaximumPacketSize());
            TEST(acceptbleGeneric->GetUpLinkAveragePacketSize() == 
                 accGenericParams2->GetUpLinkAveragePacketSize());
            TEST(acceptbleGeneric->GetUpLinkDelay() == 
                 accGenericParams2->GetUpLinkDelay());
            TEST(acceptbleGeneric->GetUpLinkPriority() == 
                 accGenericParams2->GetUpLinkPriority());
            TEST(acceptbleGeneric->GetDownlinkBandwidth() == 
                 accGenericParams2->GetDownlinkBandwidth());
            TEST(acceptbleGeneric->GetDownLinkMaximumBurstSize() == 
                 accGenericParams2->GetDownLinkMaximumBurstSize());
            TEST(acceptbleGeneric->GetDownLinkMaximumPacketSize() == 
                 accGenericParams2->GetDownLinkMaximumPacketSize());
            TEST(acceptbleGeneric->GetDownLinkAveragePacketSize() == 
                 accGenericParams2->GetDownLinkAveragePacketSize());
            TEST(acceptbleGeneric->GetDownLinkDelay() == 
                 accGenericParams2->GetDownLinkDelay());
            TEST(acceptbleGeneric->GetDownLinkPriority() == 
                 accGenericParams2->GetDownLinkPriority());
            TEST(acceptbleGeneric->GetHeaderMode() == 
                 accGenericParams2->GetHeaderMode());
            }

        CSubConParameterFamily* sblpFamily = bundle.FindFamily(
                                                 KSubConAuthorisationFamily);
        if(sblpFamily)
            {
            CSubConSBLPR5ExtensionParamSet* sblpParamtrs = 
                (CSubConSBLPR5ExtensionParamSet*)sblpFamily->FindExtensionSet(
                TUid::Uid(KSubConnSBLPR5ExtensionParamsType), 
                CSubConParameterFamily::ERequested);
        
            if(sblpParamtrs)
                {
                // Check that SBLP specific parameters are correct
                TEST(sblpParamtrs->GetMAT() == 
                     sblpParamSet->GetMAT());
        
                TInt nrOfFlows = sblpParamtrs->GetNumberOfFlowIds();
                for(TInt i = 0; i < nrOfFlows; i++)
                    {
                    const TFlowId& fid = sblpParamtrs->GetFlowIdAt(i);
        
                    TEST(fid.GetMediaComponentNumber() == 
                         flowId.GetMediaComponentNumber());
                    TEST(fid.GetIPFlowNumber() == 
                         flowId.GetIPFlowNumber());
                    }
                }
            }
        }

    // This checks that secondary context number one was created
    TEST(CheckTestFile(KTestFile1));
    // This checks that secondary context number two was created
    TEST(CheckTestFile(KTestFile2));

    // Clean up
    subconn1.CancelEventNotification();
    subconn2.CancelEventNotification();
    subconn1.Close();
    subconn2.Close();
    subconnParams.Close(); 

    // To reset the message values of the testnif
    TEST(NifAPIL(socket1, KResetMessage) == KErrNone);

    socket1.Close();
    socket2.Close();
    
    conn.Stop();
    conn.Close();

//    CleanupStack::Pop(sblpParamSet); 
    CleanupStack::Pop(sblpFamily);
//    CleanupStack::Pop(accGenericParams2); 
//    CleanupStack::Pop(reqGenericParams2); 
    CleanupStack::Pop(qosFamily2);
//    CleanupStack::Pop(accGenericParams); 
//    CleanupStack::Pop(reqGenericParams); 
    CleanupStack::Pop(qosFamily); 
    CleanupStack::Pop(&socketServer); 
    socketServer.Close();

    ClearTestFiles();

    return iTestStepResult;
    }
    
CQoSTest_702::CQoSTest_702()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText317, "qos_702");
    iTestStepName = KText317;
    }

CQoSTest_702::~CQoSTest_702()
    {
    }

TVerdict CQoSTest_702::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText318, "qos_702 SBLP, Establish a new secondary PDP context for \
the new connection");
    Log( KText318 );

    RSocketServ             socketServer;
    RSocket                 socket1, socket2;
    TInetAddr               dstAddr1, dstAddr2;
    RConnection             conn; 
    RSubConnection          subconn1, subconn2; 
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf1, subconnNotifBuf2;
    TInt                    ret; 
    
    // Connect to ESOCK 
    ret = socketServer.Connect();
    CleanupClosePushL(socketServer); 
    TESTL(ret == KErrNone); 

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP sockets 
    ret = socket1.Open(socketServer, 
                       KAfInet, 
                       KSockDatagram, 
                       KProtocolInetUdp, 
                       conn);
    TESTL(ret == KErrNone);

    ret = socket2.Open(socketServer, 
                       KAfInet, 
                       KSockDatagram, 
                       KProtocolInetUdp, 
                       conn);
    TESTL(ret == KErrNone);
    
    dstAddr1.SetPort(3461);
    _LIT(KText319, "10.1.46.1");
    dstAddr1.Input( KText319 );

    socket1.Connect(dstAddr1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    dstAddr2.SetPort(3462);
    _LIT(KText320, "10.1.46.2");
    dstAddr2.Input( KText320 );

    socket2.Connect(dstAddr2, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 
    
    TEST(NifAPIL(socket1, KNotifySecondaryCreated) == KErrNone);

    // Create a new sub-connection 
    ret = subconn1.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket onto the new sub-connection
    subconn1.Add(socket1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Create another instance of RSubConnection
    ret = subconn2.Open(socketServer, 
                        RSubConnection::ECreateNew, 
                        conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket onto the new sub-connection
    subconn2.Add(socket2, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Create the container for all subconnection parameters 
    RSubConParameterBundle sblpParams;

    CSubConParameterFamily* qosFamily2 = 
        CSubConParameterFamily::NewL(sblpParams, KSubConQoSFamily);
    CleanupDeletePushL(qosFamily2);

    CSubConQosGenericParamSet* reqGenericParams2 = 
        CSubConQosGenericParamSet::NewL(*qosFamily2, 
                                        CSubConParameterFamily::ERequested);
//    CleanupDeletePushL(reqGenericParams2);

    CSubConQosGenericParamSet* accGenericParams2 = 
        CSubConQosGenericParamSet::NewL(*qosFamily2, 
                                        CSubConParameterFamily::EAcceptable);
//    CleanupDeletePushL(accGenericParams2);

    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet1(*reqGenericParams2);
    SetEsockParamSet1(*accGenericParams2);
    
    CSubConParameterFamily* sblpFamily = 
        CSubConParameterFamily::NewL(sblpParams, KSubConAuthorisationFamily);
    CleanupDeletePushL(sblpFamily);

    // Create and Add SBLP Extension Parameters
    CSubConSBLPR5ExtensionParamSet* sblpParamSet = 
        CSubConSBLPR5ExtensionParamSet::NewL(*sblpFamily, 
        CSubConParameterFamily::ERequested);
//    CleanupDeletePushL(sblpParamSet);

    sblpParamSet->SetMAT(_S8("http://www.symbian.com//test_case_702//")); 
    
    TFlowId flowId;
    TUint16 mcn = 100;
    TUint16 ifn = 1000;
    flowId.SetMediaComponentNumber(mcn);
    flowId.SetIPFlowNumber(ifn);
    
    sblpParamSet->AddFlowIdL( flowId );

    subconn2.EventNotification(subconnNotifBuf2, ETrue, eventStatus); 

    // Set properties of the sub-connection 
    ret = subconn2.SetParameters(sblpParams); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn2.SetParameters-event 
    // (expected KSubConGenericEventParamsGranted)
    User::WaitForRequest(eventStatus); 
    TESTL(eventStatus.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf2, KSubConGenericEventParamsGranted); 
    TESTE(ret == KErrNone, ret);
    
    
    
    // Check that sblp parameter has been added to the 
    // current context
    TEST(CheckTestFile(KTestSblpAdd));
    
    

    // Create another container for all subconnection parameters 
    RSubConParameterBundle subconnParams; 
    
    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily = 
        CSubConParameterFamily::NewL(subconnParams, KSubConQoSFamily);
    CleanupDeletePushL(qosFamily);

    // Create the requested generic parameter set for QoS 
    // (Qos family takes ownership) 
    CSubConQosGenericParamSet* reqGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::ERequested);
//    CleanupDeletePushL(reqGenericParams);

    CSubConQosGenericParamSet* accGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::EAcceptable);
//    CleanupDeletePushL(accGenericParams);

    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet1(*reqGenericParams);
    SetEsockParamSet1(*accGenericParams);

    subconn1.EventNotification(subconnNotifBuf1, ETrue, eventStatus); 

    // Set properties of the sub-connection 
    ret = subconn1.SetParameters(subconnParams); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn1.SetParameters-event 
    // (expected KSubConGenericEventParamsGranted)
    User::WaitForRequest(eventStatus); 
    TESTL(eventStatus.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf1, KSubConGenericEventParamsGranted); 
    TESTE(ret == KErrNone, ret);
    
    // This checks that secondary context number one was created
    TEST(CheckTestFile(KTestFile1));
    // This checks that secondary context number two was created
    TEST(CheckTestFile(KTestFile2));

    // Clean up
    subconn1.CancelEventNotification();
    subconn2.CancelEventNotification();
    subconn1.Close();
    subconn2.Close();
    subconnParams.Close(); 

    // To reset the message values of the testnif
    TEST(NifAPIL(socket1, KResetMessage) == KErrNone);

    socket1.Close();
    socket2.Close();
    
    conn.Stop();
    conn.Close();

//    CleanupStack::Pop(accGenericParams); 
//    CleanupStack::Pop(reqGenericParams); 
    CleanupStack::Pop(qosFamily); 
//    CleanupStack::Pop(sblpParamSet); 
    CleanupStack::Pop(sblpFamily);
//    CleanupStack::Pop(accGenericParams2); 
//    CleanupStack::Pop(reqGenericParams2); 
    CleanupStack::Pop(qosFamily2);
    CleanupStack::Pop(&socketServer); 
    socketServer.Close();

    ClearTestFiles();

    return iTestStepResult;
    }
    
CQoSTest_703::CQoSTest_703()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText321, "qos_703");
    iTestStepName = KText321;
    }

CQoSTest_703::~CQoSTest_703()
    {
    }

TVerdict CQoSTest_703::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText322, 
        "qos_703 SBLP, Two secondary contexts created for 3 RSubConnections");
    Log( KText322 );

    RSocketServ             socketServer;
    RSocket                 socket1, socket2, socket3;
    TInetAddr               dstAddr1, dstAddr2, dstAddr3;
    RConnection             conn; 
    RSubConnection          subconn1, subconn2, subconn3; 
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf1, subconnNotifBuf2, 
                            subconnNotifBuf3;
    TInt                    ret; 

    // Connect to ESOCK 
    ret = socketServer.Connect();
    CleanupClosePushL(socketServer); 
    TESTL(ret == KErrNone); 

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP sockets 
    ret = socket1.Open(socketServer, 
                       KAfInet, 
                       KSockDatagram, 
                       KProtocolInetUdp, 
                       conn);
    TESTL(ret == KErrNone);

    ret = socket2.Open(socketServer, 
                       KAfInet, 
                       KSockDatagram, 
                       KProtocolInetUdp, 
                       conn);
    TESTL(ret == KErrNone);

    ret = socket3.Open(socketServer, 
                       KAfInet, 
                       KSockDatagram, 
                       KProtocolInetUdp, 
                       conn);
    TESTL(ret == KErrNone);

    dstAddr1.SetPort(3461);
    _LIT(KText323, "10.1.46.1");
    dstAddr1.Input( KText323 );
    socket1.Connect(dstAddr1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 
    
    dstAddr2.SetPort(3462);
    _LIT(KText324, "10.1.46.2");
    dstAddr2.Input( KText324 );
    socket2.Connect(dstAddr2, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    dstAddr3.SetPort(3463);
    _LIT(KText321, "10.1.46.3");
    dstAddr3.Input( KText321 );
    socket3.Connect(dstAddr3, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    TEST(NifAPIL(socket1, KNotifySecondaryCreated) == KErrNone);

    // Create a new sub-connection 
    ret = subconn1.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket onto the new sub-connection
    subconn1.Add(socket1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Create another instance of RSubConnection
    ret = subconn2.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket onto the new sub-connection
    subconn2.Add(socket2, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Create another instance of RSubConnection
    ret = subconn3.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket onto the new sub-connection
    subconn3.Add(socket3, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Create the container for all subconnection parameters 
    RSubConParameterBundle subconnParams; 
    
    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily = 
        CSubConParameterFamily::NewL(subconnParams, KSubConQoSFamily);
    CleanupDeletePushL(qosFamily);

    // Create the requested generic parameter set for QoS 
    // (Qos family takes ownership) 
    CSubConQosGenericParamSet* reqGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::ERequested);
//    CleanupDeletePushL(reqGenericParams);

    CSubConQosGenericParamSet* accGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::EAcceptable);
//    CleanupDeletePushL(accGenericParams);

    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet1(*reqGenericParams);
    SetEsockParamSet1(*accGenericParams);

    // Subscribe for QoS Params notification
    subconn1.EventNotification(subconnNotifBuf1, ETrue, eventStatus); 

    // Set properties of the sub-connection 
    ret = subconn1.SetParameters(subconnParams); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn1.SetParameters-event 
    // (expected KSubConGenericEventParamsGranted)
    User::WaitForRequest(eventStatus); 
    TESTL(eventStatus.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf1, KSubConGenericEventParamsGranted); 
    TESTE(ret == KErrNone, ret);

    // Create the container for all subconnection parameters 
    RSubConParameterBundle subconnParams2; 
    
    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily2 = 
        CSubConParameterFamily::NewL(subconnParams2, KSubConQoSFamily);
    CleanupDeletePushL(qosFamily2);

    // Create the requested generic parameter set for QoS 
    // (Qos family takes ownership) 
    CSubConQosGenericParamSet* reqGenericParams2 = 
        CSubConQosGenericParamSet::NewL(*qosFamily2, 
                                        CSubConParameterFamily::ERequested);
//    CleanupDeletePushL(reqGenericParams2);

    CSubConQosGenericParamSet* accGenericParams2 = 
        CSubConQosGenericParamSet::NewL(*qosFamily2, 
                                        CSubConParameterFamily::EAcceptable);
//    CleanupDeletePushL(accGenericParams2);

    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet1(*reqGenericParams2);
    SetEsockParamSet1(*accGenericParams2);

    // Subscribe for QoS Params notification
    subconn2.EventNotification(subconnNotifBuf2, ETrue, eventStatus); 

    // Set properties of the sub-connection 
    ret = subconn2.SetParameters(subconnParams2); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn2.SetParameters-event 
    // (expected KSubConGenericEventParamsGranted)
    User::WaitForRequest(eventStatus); 
    TESTL(eventStatus.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf2, KSubConGenericEventParamsGranted); 
    TESTE(ret == KErrNone, ret);

    // Create third container for all subconnection parameters 
    RSubConParameterBundle sblpParams;

    CSubConParameterFamily* qosFamily3 = 
        CSubConParameterFamily::NewL(sblpParams, KSubConQoSFamily);
    CleanupDeletePushL(qosFamily3);

    CSubConQosGenericParamSet* reqGenericParams3 = 
        CSubConQosGenericParamSet::NewL(*qosFamily3, 
                                        CSubConParameterFamily::ERequested);
//    CleanupDeletePushL(reqGenericParams3);

    CSubConQosGenericParamSet* accGenericParams3 = 
        CSubConQosGenericParamSet::NewL(*qosFamily3, 
                                        CSubConParameterFamily::EAcceptable);
//    CleanupDeletePushL(accGenericParams3);

    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet1(*reqGenericParams3);
    SetEsockParamSet1(*accGenericParams3);

    CSubConParameterFamily* sblpFamily = 
        CSubConParameterFamily::NewL(sblpParams, KSubConAuthorisationFamily);
    CleanupDeletePushL(sblpFamily);

    // Create and Add SBLP Extension Parameters
    CSubConSBLPR5ExtensionParamSet* sblpParamSet = 
        CSubConSBLPR5ExtensionParamSet::NewL(*sblpFamily, 
        CSubConParameterFamily::ERequested);
//    CleanupDeletePushL(sblpParamSet);

    sblpParamSet->SetMAT(_S8("http://www.symbian.com//test_case_703//")); 
    
    TFlowId flowId;
    TUint16 mcn = 100;
    TUint16 ifn = 1000;
    flowId.SetMediaComponentNumber(mcn);
    flowId.SetIPFlowNumber(ifn);
    
    sblpParamSet->AddFlowIdL( flowId );

    subconn3.EventNotification(subconnNotifBuf3, ETrue, eventStatus); 

    // Set properties of the sub-connection 
    ret = subconn3.SetParameters(sblpParams); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn3.SetParameters-event 
    // (expected KSubConGenericEventParamsGranted)
    User::WaitForRequest(eventStatus); 
    TESTL(eventStatus.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf3, KSubConGenericEventParamsGranted); 
    TESTE(ret == KErrNone, ret);
    
    
    // Check that sblp parameter has been added to the 
    // current context
    TEST(CheckTestFile(KTestSblpAdd));
    

    // This checks that secondary context number one was created
    TEST(CheckTestFile(KTestFile1));
    // This checks that secondary context number two was created
    TEST(CheckTestFile(KTestFile2));

    // Clean up
    subconn1.CancelEventNotification();
    subconn2.CancelEventNotification();
    subconn3.CancelEventNotification();
    subconn1.Close();
    subconn2.Close();
    subconn3.Close();
    subconnParams.Close(); 

    // To reset the message values of the testnif
    TEST(NifAPIL(socket1, KResetMessage) == KErrNone);

    socket1.Close();
    socket2.Close();
    socket3.Close();

    conn.Stop();
    conn.Close();

//    CleanupStack::Pop(sblpParamSet); 
    CleanupStack::Pop(sblpFamily);
//    CleanupStack::Pop(accGenericParams3); 
//    CleanupStack::Pop(reqGenericParams3); 
    CleanupStack::Pop(qosFamily3);
//    CleanupStack::Pop(accGenericParams2); 
//    CleanupStack::Pop(reqGenericParams2); 
    CleanupStack::Pop(qosFamily2);
//    CleanupStack::Pop(accGenericParams); 
//    CleanupStack::Pop(reqGenericParams); 
    CleanupStack::Pop(qosFamily); 
    CleanupStack::Pop(&socketServer); 
    socketServer.Close();

    ClearTestFiles();

    return iTestStepResult;
    }

CQoSTest_704::CQoSTest_704()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText321, "qos_704");
    iTestStepName = KText321;
    }

CQoSTest_704::~CQoSTest_704()
    {
    }

TVerdict CQoSTest_704::doTestStepL( void )
    {
    //
    //  !!! NOTICE !!!
    //  This case has to be run separately from all of the other cases
    //
    User::After(KInterTestDelay);
    _LIT(KText322, 
        "qos_704 SBLP, Drop default PDP context and use SBLP context as new \
default context");
    Log( KText322 );

    _LIT(KQoSSimNoUse, "c:\\system\\data\\UmtsSim_Server_simconf.704.ini");
    _LIT(KQoSSimInUse, "c:\\system\\data\\UmtsSim_Server_simconf.ini");

    RFs fs;
    fs.Connect();
    // If this leaves, fs is not closed, but who cares, it was not
    // closed otherwise either...
	CFileMan* fman = CFileMan::NewL(fs);

    //TInt err = fs.Rename(KQoSSimNoUse, KQoSSimInUse);
    TInt err = fman->Copy(KQoSSimNoUse, KQoSSimInUse);
    delete fman;
    fs.Close();

    if(err != KErrNone)
        {
        //_LIT(KText225, "Renaming file didn't succeed");
        _LIT(KText225, "Copying file didn't succeed");
        Log( KText225 );
        return EInconclusive;
        }

    RSocketServ             socketServer;
    RSocket                 socket1, socket2;
    TInetAddr               dstAddr1, dstAddr2;
    RConnection             conn; 
    RSubConnection          subconn1, subconn2; 
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf1, subconnNotifBuf2, 
                            subconnNotifBuf3; 
    TInt                    ret; 

    // Connect to ESOCK 
    ret = socketServer.Connect();
    CleanupClosePushL(socketServer); 
    TESTL(ret == KErrNone); 

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP sockets 
    ret = socket1.Open(socketServer, 
                       KAfInet, 
                       KSockDatagram, 
                       KProtocolInetUdp, 
                       conn);
    TESTL(ret == KErrNone); 

    ret = socket2.Open(socketServer, 
                       KAfInet, 
                       KSockDatagram, 
                       KProtocolInetUdp, 
                       conn);
    TESTL(ret == KErrNone); 

    dstAddr1.SetPort(3461);
    _LIT(KText315, "10.1.46.1");
    dstAddr1.Input( KText315 );
    socket1.Connect(dstAddr1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    dstAddr2.SetPort(3462);
    _LIT(KText316, "10.1.46.2");
    dstAddr2.Input( KText316 );
    socket2.Connect(dstAddr2, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    TEST(NifAPIL(socket1, KNotifySecondaryCreated) == KErrNone);

    // Create a new sub-connection 
    ret = subconn1.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket onto the new sub-connection
    subconn1.Add(socket1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Create another instance of RSubConnection
    ret = subconn2.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket onto the new sub-connection
    subconn2.Add(socket2, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Create the container for all subconnection parameters 
    RSubConParameterBundle subconnParams; 
    
    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily = 
        CSubConParameterFamily::NewL(subconnParams, KSubConQoSFamily);
    CleanupDeletePushL(qosFamily);

    // Create the requested generic parameter set for QoS 
    // (Qos family takes ownership) 
    CSubConQosGenericParamSet* reqGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::ERequested);
    //CleanupDeletePushL(reqGenericParams);

    CSubConQosGenericParamSet* accGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::EAcceptable);
    //CleanupDeletePushL(accGenericParams);

    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet1(*reqGenericParams);
    SetEsockParamSet1(*accGenericParams);

    // Subscribe for QoS Params notification
    subconn1.EventNotification(subconnNotifBuf1, ETrue, eventStatus); 
    
    // Set properties of the sub-connection 
    ret = subconn1.SetParameters(subconnParams); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn1.SetParameters-event 
    // (expected KSubConGenericEventParamsGranted)
    User::WaitForRequest(eventStatus); 
    TESTL(eventStatus.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf1, KSubConGenericEventParamsGranted);
    TESTE(ret == KErrNone, ret); 

    // Create another container for all subconnection parameters 
    RSubConParameterBundle sblpParams;

    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily2 = 
        CSubConParameterFamily::NewL(sblpParams, KSubConQoSFamily);
    CleanupDeletePushL(qosFamily2);

    CSubConQosGenericParamSet* reqGenericParams2 = 
        CSubConQosGenericParamSet::NewL(*qosFamily2, 
                                        CSubConParameterFamily::ERequested);
    //CleanupDeletePushL(reqGenericParams2);

    CSubConQosGenericParamSet* accGenericParams2 = 
        CSubConQosGenericParamSet::NewL(*qosFamily2, 
                                        CSubConParameterFamily::EAcceptable);
    //CleanupDeletePushL(accGenericParams2);

    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet1(*reqGenericParams2);
    SetEsockParamSet1(*accGenericParams2);
    
    CSubConParameterFamily* sblpFamily = 
        CSubConParameterFamily::NewL(sblpParams, KSubConAuthorisationFamily);
    CleanupDeletePushL(sblpFamily);

    // Create and Add SBLP Extension Parameters
    CSubConSBLPR5ExtensionParamSet* sblpParamSet = 
        CSubConSBLPR5ExtensionParamSet::NewL(*sblpFamily, 
        CSubConParameterFamily::ERequested);
    //CleanupDeletePushL(sblpParamSet);

    sblpParamSet->SetMAT(_S8("http://www.symbian.com//test_case_704//")); 
    
    TFlowId flowId;
    TUint16 mcn = 100;
    TUint16 ifn = 1000;
    flowId.SetMediaComponentNumber(mcn);
    flowId.SetIPFlowNumber(ifn);
    
    sblpParamSet->AddFlowIdL( flowId );

    subconn2.EventNotification(subconnNotifBuf2, ETrue, eventStatus); 

    // Set properties of the sub-connection 
    ret = subconn2.SetParameters(sblpParams); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn2.SetParameters-event 
    // (expected KSubConGenericEventParamsGranted)
    User::WaitForRequest(eventStatus); 
    TESTL(eventStatus.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf2, KSubConGenericEventParamsGranted);
    TESTE(ret == KErrNone, ret);
    
    // Check that sblp parameter has been added to the 
    // current context
    TEST(CheckTestFile(KTestSblpAdd));
    
     

	TRAP(err, iEventListener = CEventListener::NewL());

    subconn2.EventNotification(subconnNotifBuf3,  
                               ETrue, 
                               iEventListener->iStatus);

    // Umtssim will drop the packets, wait for the event 
    // (expected KSubConGenericEventParamsRejected)
   	WaitForSubConnEventL();
    TESTE(iEventListener->iStatus.Int() == KErrNone, 
          iEventListener->iStatus.Int());
    ret = CheckEvent(subconnNotifBuf3, 
                     KSubConGenericEventParamsRejected); 
    TESTE(ret == KErrNone, ret);

    // This checks that secondary context number one was created
    TEST(CheckTestFile(KTestFile1));
    // This checks that secondary context number two was created
    TEST(CheckTestFile(KTestFile2));

//	err = fs.Rename(KQoSSimInUse, KQoSSimNoUse);
//	if(err != KErrNone)
//  	{
//	_LIT(KText227, "Naming. UmtsSim_Server_simconf.ini back to \
//UmtsSim_Server_simconf.ini.nouse failed!!! Do it manually before running \
//any other cases");
//        Log( KText227 );
//        return EAbort;
//        }

    // Clean up
    subconn1.CancelEventNotification();
    subconn2.CancelEventNotification();
    subconn1.Close();
    subconn2.Close();
    subconnParams.Close(); 

    // To reset the message values of the testnif
    TEST(NifAPIL(socket1, KResetMessage) == KErrNone);

    socket1.Close();
    socket2.Close();
    
    conn.Stop();
    conn.Close();
    
    //CleanupStack::Pop(sblpParamSet);
    CleanupStack::Pop(sblpFamily);
    //CleanupStack::Pop(accGenericParams2); 
    //CleanupStack::Pop(reqGenericParams2); 
    CleanupStack::Pop(qosFamily2);
    //CleanupStack::Pop(accGenericParams); 
    //CleanupStack::Pop(reqGenericParams); 
    CleanupStack::Pop(qosFamily); 
    CleanupStack::Pop(&socketServer); 
    socketServer.Close();

    ClearTestFiles();

    return iTestStepResult;
    }

CQoSTest_705::CQoSTest_705()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText313, "qos_705");
    iTestStepName = KText313;
    }


CQoSTest_705::~CQoSTest_705()
    {
    }

TVerdict CQoSTest_705::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText314, 
        "qos_705 SBLP, Multiple Flow IDs");
    Log( KText314 );
    
    RSocketServ             socketServer;
    RSocket                 socket1;
    TInetAddr               dstAddr1;
    RConnection             conn; 
    RSubConnection          subconn1; 
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf;
    TInt                    ret; 
    
    // Connect to ESOCK 
    ret = socketServer.Connect();
    CleanupClosePushL(socketServer); 
    TESTL(ret == KErrNone); 

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP sockets 
    ret = socket1.Open(socketServer, 
                       KAfInet, 
                       KSockDatagram, 
                       KProtocolInetUdp, 
                       conn);
    TESTL(ret == KErrNone); 

    dstAddr1.SetPort(3461);
    _LIT(KText315, "10.1.46.1");
    dstAddr1.Input( KText315 );
    socket1.Connect(dstAddr1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    TEST(NifAPIL(socket1, KNotifySecondaryCreated) == KErrNone);

    // Create a new sub-connection 
    ret = subconn1.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket onto the new sub-connection
    subconn1.Add(socket1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Create the container for all subconnection parameters 
    RSubConParameterBundle sblpParams;

    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily = 
        CSubConParameterFamily::NewL(sblpParams, KSubConQoSFamily);
    CleanupDeletePushL(qosFamily);

    CSubConQosGenericParamSet* reqGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::ERequested);
    //CleanupDeletePushL(reqGenericParams);

    CSubConQosGenericParamSet* accGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::EAcceptable);
    //CleanupDeletePushL(accGenericParams);

    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet1(*reqGenericParams);
    SetEsockParamSet1(*accGenericParams);
    
    CSubConParameterFamily* sblpFamily = 
        CSubConParameterFamily::NewL(sblpParams, KSubConAuthorisationFamily);
    CleanupDeletePushL(sblpFamily);

    // Create and Add SBLP Extension Parameters
    CSubConSBLPR5ExtensionParamSet* sblpExtnParamSet = 
        CSubConSBLPR5ExtensionParamSet::NewL(*sblpFamily, 
        CSubConParameterFamily::ERequested);
    CleanupDeletePushL(sblpExtnParamSet);

    sblpExtnParamSet->SetMAT(_S8("http://www.symbian.com//test_case_705//")); 

    // Add the first MAT
    TFlowId flowId;
    TUint16 mcn = 100;
    TUint16 ifn = 1000;
    flowId.SetMediaComponentNumber(mcn);
    flowId.SetIPFlowNumber(ifn);

    sblpExtnParamSet->AddFlowIdL( flowId );

    // Add the second MAT
    mcn++;
    ifn++;
    flowId.SetMediaComponentNumber(mcn);
    flowId.SetIPFlowNumber(ifn);

    sblpExtnParamSet->AddFlowIdL( flowId );

    // Subscribe for QoS Params notification
    subconn1.EventNotification(subconnNotifBuf, ETrue, eventStatus); 
    
    // Set properties of the sub-connection 
    ret = subconn1.SetParameters(sblpParams); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn1.SetParameters-event 
    // (expected KSubConGenericEventParamsGranted)
    User::WaitForRequest(eventStatus); 
    TESTL(eventStatus.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf, KSubConGenericEventParamsGranted);
    TESTE(ret == KErrNone, ret);
    
    // Check that sblp parameter has been added to the 
    // current context
    TEST(CheckTestFile(KTestSblpAdd)); 

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

    CleanupStack::Pop(sblpExtnParamSet);
    CleanupStack::Pop(sblpFamily); 
    //CleanupStack::Pop(accGenericParams); 
    //CleanupStack::Pop(reqGenericParams); 
    CleanupStack::Pop(qosFamily); 
    CleanupStack::Pop(&socketServer); 
    socketServer.Close();

    ClearTestFiles();

    return iTestStepResult;
    }


CQoSTest_706::CQoSTest_706()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText313, "qos_706");
    iTestStepName = KText313;
    }


CQoSTest_706::~CQoSTest_706()
    {
    }

TVerdict CQoSTest_706::doTestStepL( void )
    {
    /*
    User::After(KInterTestDelay);
    _LIT(KText314, 
        "qos_706 SBLP, SBLP rejection code");
    Log( KText314 );
    
    RSocketServ             socketServer;
    RSocket                 socket1;
    TInetAddr               dstAddr1;
    RConnection             conn; 
    RSubConnection          subconn1; 
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf;
    TInt                    ret; 

    // Connect to ESOCK 
    ret = socketServer.Connect();
    CleanupClosePushL(socketServer); 
    TESTL(ret == KErrNone); 

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP sockets 
    ret = socket1.Open(socketServer, 
                       KAfInet, 
                       KSockDatagram, 
                       KProtocolInetUdp, 
                       conn);
    TESTL(ret == KErrNone); 

    dstAddr1.SetPort(3461);
    _LIT(KText315, "10.1.46.1");
    dstAddr1.Input( KText315 );
    socket1.Connect(dstAddr1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    TEST(NifAPIL(socket1, KContextActivateRejectSBLP) == KErrNone);

    // Create a new sub-connection 
    ret = subconn1.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket onto the new sub-connection
    subconn1.Add(socket1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() != KErrNone);

    // Clean up
    subconn1.Close();

    // To reset the message values of the testnif
    TEST(NifAPIL(socket1, KResetMessage) == KErrNone);

    socket1.Close();
    
    conn.Stop();
    conn.Close();

    CleanupStack::Pop(&socketServer); 
    socketServer.Close();

    ClearTestFiles();

    return iTestStepResult;
    
    */
    
    User::After(KInterTestDelay);
    _LIT(KText, "qos_706 SBLP, SBLP rejection code");
    Log(KText);
    
    RSocketServ             socketServer;
    RConnection             conn; 
    RSubConnection          subconn1; 
    RSocket                 socket1;
    TInetAddr               dstAddr1;
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf;
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
   
    // Move the connected socket onto the new sub-connection
    subconn1.Add(socket1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    TEST(NifAPIL(socket1, KContextActivateRejectSBLP) == KErrNone);
    
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
    
    
    
    
    CSubConParameterFamily* sblpFamily = CSubConParameterFamily::NewL(parameterBundle, KSubConAuthorisationFamily);

    // Create and Add SBLP Extension Parameters
    CSubConSBLPR5ExtensionParamSet* sblpExtnParamSet = CSubConSBLPR5ExtensionParamSet::NewL(*sblpFamily, CSubConParameterFamily::ERequested);

    sblpExtnParamSet->SetMAT(_S8("http://www.symbian.com//test_case_706//")); 

    // Add the first MAT
    TFlowId flowId;
    TUint16 mcn = 100;
    TUint16 ifn = 1000;
    flowId.SetMediaComponentNumber(mcn);
    flowId.SetIPFlowNumber(ifn);

    sblpExtnParamSet->AddFlowIdL( flowId );

    // Add the second MAT
    mcn++;
    ifn++;
    flowId.SetMediaComponentNumber(mcn);
    flowId.SetIPFlowNumber(ifn);

    sblpExtnParamSet->AddFlowIdL( flowId );

    // Subscribe for QoS Params notification
    subconn1.EventNotification(subconnNotifBuf, ETrue, eventStatus); 
    
    // Set properties of the sub-connection 
    ret = subconn1.SetParameters(parameterBundle); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn1.SetParameters-event 
    // (expected KSubConGenericEventParamsGranted)
    User::WaitForRequest(eventStatus); 
    TESTL(eventStatus.Int() == KErrNone);
    
    ret = CheckEvent(subconnNotifBuf, KSubConGenericEventParamsRejected);
    TESTE(ret == KErrNone, ret);

	if(ret == KErrNone)
		{
		// Extract information from received Event
		CSubConNotificationEvent* event;
		event = CSubConNotificationEvent::NewL(subconnNotifBuf);
		CleanupStack::PushL(event);
		CSubConGenEventParamsRejected* rejectedEvent = static_cast<CSubConGenEventParamsRejected*>(event);
		TInt error = rejectedEvent->Error();
		if (error != KErrNone )
			{
			// Check if the error id is KErrGprsUserAuthenticationFailure ?
			TESTE(error == KErrGprsUserAuthenticationFailure, error);
			}
		else
			{
			Log(_L("SBLP rejection error code not received with the CSubConGenEventParamsRejected Event"));	
			TESTL(KErrNotFound);
			}		
		CleanupStack::PopAndDestroy (event);
		}

    // Check that sblp parameter has been added to the 
    // current context
    TEST(!CheckTestFile(KTestSblpAdd)); 

    // This checks that secondary context number one was created
    TEST(CheckTestFile(KTestFile1));

    // To reset the message values of the testnif
    TEST(NifAPIL(socket1, KResetMessage) == KErrNone);

	parameterBundle.Close();
    CleanupStack::Pop(&parameterBundle);
    // close and destroy
    CleanupStack::PopAndDestroy(&subconn1);
    CleanupStack::PopAndDestroy(&socket1);
    CleanupStack::PopAndDestroy(&conn);
    CleanupStack::PopAndDestroy(&socketServer);
    ClearTestFiles();
    return iTestStepResult;
    }


CQoSTest_707::CQoSTest_707()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText313, "qos_707");
    iTestStepName = KText313;
    }


CQoSTest_707::~CQoSTest_707()
    {
    }

TVerdict CQoSTest_707::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText314, 
        "qos_707 SBLP, Maximum length MAT");
    Log( KText314 );
    
    RSocketServ             socketServer;
    RSocket                 socket1;
    TInetAddr               dstAddr1;
    RConnection             conn; 
    RSubConnection          subconn1; 
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf;
    TInt                    ret; 
    
    // Connect to ESOCK 
    ret = socketServer.Connect();
    CleanupClosePushL(socketServer); 
    TESTL(ret == KErrNone); 

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP sockets 
    ret = socket1.Open(socketServer, 
                       KAfInet, 
                       KSockDatagram, 
                       KProtocolInetUdp, 
                       conn);
    TESTL(ret == KErrNone); 

    dstAddr1.SetPort(3461);
    _LIT(KText315, "10.1.46.1");
    dstAddr1.Input( KText315 );
    socket1.Connect(dstAddr1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    TEST(NifAPIL(socket1, KNotifySecondaryCreated) == KErrNone);

    // Create a new sub-connection 
    ret = subconn1.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket onto the new sub-connection
    subconn1.Add(socket1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Create the container for all subconnection parameters 
    RSubConParameterBundle sblpParams;

    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily = 
        CSubConParameterFamily::NewL(sblpParams, KSubConQoSFamily);
    CleanupDeletePushL(qosFamily);

    CSubConQosGenericParamSet* reqGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::ERequested);
    //CleanupDeletePushL(reqGenericParams);

    CSubConQosGenericParamSet* accGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::EAcceptable);
    //CleanupDeletePushL(accGenericParams);

    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet1(*reqGenericParams);
    SetEsockParamSet1(*accGenericParams);
    
    CSubConParameterFamily* sblpFamily = 
        CSubConParameterFamily::NewL(sblpParams, KSubConAuthorisationFamily);
    CleanupDeletePushL(sblpFamily);

    // Create and Add SBLP Extension Parameters
    CSubConSBLPR5ExtensionParamSet* sblpExtnParamSet = 
        CSubConSBLPR5ExtensionParamSet::NewL(*sblpFamily, 
        CSubConParameterFamily::ERequested);
    CleanupDeletePushL(sblpExtnParamSet);

    sblpExtnParamSet->SetMAT(_S8("http://www.symbian.com//test_case_707//\
This_media_authorization_token_tests_the_maximum_length_of_fully_qualified\
_domain_name_255_characters//abcdefghijklmnopqrstuvwxyz//abcdefghijklmnopq\
rstuvwxyz//abcdefghijklmnopqrstuvwxyz//abcdefghijklmnopqrstuvwxyz//")); 
    
    TFlowId flowId;
    TUint16 mcn = 100;
    TUint16 ifn = 1000;
    flowId.SetMediaComponentNumber(mcn);
    flowId.SetIPFlowNumber(ifn);
    
    sblpExtnParamSet->AddFlowIdL( flowId );

    // Subscribe for QoS Params notification
    subconn1.EventNotification(subconnNotifBuf, ETrue, eventStatus); 

    // Set properties of the sub-connection 
    ret = subconn1.SetParameters(sblpParams); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn.SetParameters-event 
    // (expected KSubConGenericEventParamsGranted)
    User::WaitForRequest(eventStatus); 
    TESTL(eventStatus.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf, KSubConGenericEventParamsGranted);
    TESTE(ret == KErrNone, ret); 
    
    // Check that sblp parameter has been added to the 
    // current context
    TEST(CheckTestFile(KTestSblpAdd));
    

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

    CleanupStack::Pop(sblpExtnParamSet);
    CleanupStack::Pop(sblpFamily); 
    //CleanupStack::Pop(accGenericParams); 
    //CleanupStack::Pop(reqGenericParams); 
    CleanupStack::Pop(qosFamily); 
    CleanupStack::Pop(&socketServer); 
    socketServer.Close();

    ClearTestFiles();

    return iTestStepResult;
    }


CQoSTest_708::CQoSTest_708()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText313, "qos_708");
    iTestStepName = KText313;
    }


CQoSTest_708::~CQoSTest_708()
    {
    }

TVerdict CQoSTest_708::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText314, 
        "qos_708 SBLP, Maximum + 1 length MAT");
    Log( KText314 );

    RSocketServ             socketServer;
    RSocket                 socket1;
    TInetAddr               dstAddr1;
    RConnection             conn; 
    RSubConnection          subconn1; 
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf;
    TInt                    ret; 

    // Connect to ESOCK 
    ret = socketServer.Connect();
    CleanupClosePushL(socketServer); 
    TESTL(ret == KErrNone); 

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP sockets 
    ret = socket1.Open(socketServer, 
                       KAfInet, 
                       KSockDatagram, 
                       KProtocolInetUdp, 
                       conn);
    TESTL(ret == KErrNone); 

    dstAddr1.SetPort(3461);
    _LIT(KText315, "10.1.46.1");
    dstAddr1.Input( KText315 );
    socket1.Connect(dstAddr1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    // Create a new sub-connection 
    ret = subconn1.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket onto the new sub-connection
    subconn1.Add(socket1, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Create the container for all subconnection parameters 
    RSubConParameterBundle sblpParams;

    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily = 
        CSubConParameterFamily::NewL(sblpParams, KSubConQoSFamily);
    CleanupDeletePushL(qosFamily);

    CSubConQosGenericParamSet* reqGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::ERequested);
    //CleanupDeletePushL(reqGenericParams);

    CSubConQosGenericParamSet* accGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::EAcceptable);
    //CleanupDeletePushL(accGenericParams);

    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet1(*reqGenericParams);
    SetEsockParamSet1(*accGenericParams);
    
    CSubConParameterFamily* sblpFamily = 
        CSubConParameterFamily::NewL(sblpParams, KSubConAuthorisationFamily);
    CleanupDeletePushL(sblpFamily);

    // Create and Add SBLP Extension Parameters
    CSubConSBLPR5ExtensionParamSet* sblpExtnParamSet = 
        CSubConSBLPR5ExtensionParamSet::NewL(*sblpFamily, 
        CSubConParameterFamily::ERequested);
    CleanupDeletePushL(sblpExtnParamSet);

    sblpExtnParamSet->SetMAT(_S8("http://www.symbian.com//test_case_708//\
This_media_authorization_token_tests_the_exceeded_length_of_fully_qualified\
_domain_name_255_characters//abcdefghijklmnopqrstuvwxyz//abcdefghijklmnopq\
rstuvwxyz//abcdefghijklmnopqrstuvwxyz//abcdefghijklmnopqrstuvwxyz//a")); 
    
    TFlowId flowId;
    TUint16 mcn = 100;
    TUint16 ifn = 1000;
    flowId.SetMediaComponentNumber(mcn);
    flowId.SetIPFlowNumber(ifn);
    
    sblpExtnParamSet->AddFlowIdL( flowId );

    // Subscribe for QoS Params notification
    subconn1.EventNotification(subconnNotifBuf, ETrue, eventStatus); 

    // Set properties of the sub-connection 
    ret = subconn1.SetParameters(sblpParams); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn.SetParameters-event 
    // (expected KSubConGenericEventParamsRejected)
    User::WaitForRequest(eventStatus); 
    TESTL(eventStatus.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf, KSubConGenericEventParamsRejected);
    TESTE(ret == KErrNone, ret);
    
    
    // Clean up
    subconn1.Close();
    sblpParams.Close(); 

    // To reset the message values of the testnif
    TEST(NifAPIL(socket1, KResetMessage) == KErrNone);

    socket1.Close();
    
    conn.Stop();
    conn.Close();

    CleanupStack::Pop(sblpExtnParamSet);
    CleanupStack::Pop(sblpFamily); 
    //CleanupStack::Pop(accGenericParams); 
    //CleanupStack::Pop(reqGenericParams); 
    CleanupStack::Pop(qosFamily); 
    CleanupStack::Pop(&socketServer); 
    socketServer.Close();

    ClearTestFiles();

    return iTestStepResult;
    }


CQoSTest_072::CQoSTest_072()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText202, "qos_072");
    iTestStepName = KText202;
    }

CQoSTest_072::~CQoSTest_072()
    {
    }

TVerdict CQoSTest_072::doTestStepL( void )
    {
    User::After(KInterTestDelay / 9);
    _LIT(KText203, "qos_072, CSubConQosIPLinkR99ParamSet, Requested and \
acceptable parameters");
    Log( KText203 );

    RSocketServ             socketServer;
    RSocket                 socket;
    TInetAddr               dstAddr;
    RConnection             conn; 
    RSubConnection          subconn; 
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf;
    TInt                    ret; 

    // Connect to ESOCK 
    ret = socketServer.Connect();
    CleanupClosePushL(socketServer); 
    TESTL(ret == KErrNone); 

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP sockets 
    ret = socket.Open(socketServer, 
                      KAfInet, 
                      KSockDatagram, 
                      KProtocolInetUdp, 
                      conn);
    TESTL(ret == KErrNone); 

    _LIT(KText204, "10.1.72.1");
    dstAddr.Input( KText204 );
    dstAddr.SetPort(3721);
    socket.Connect(dstAddr, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    // Create a new sub-connection 
    ret = subconn.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket onto the new sub-connection
    subconn.Add(socket, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Create the container for all subconnection parameters 
    RSubConParameterBundle subconnParams; 
    
    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily = 
        CSubConParameterFamily::NewL(subconnParams, KSubConQoSFamily);
    CleanupDeletePushL(qosFamily);

    // Create the requested generic parameter set for QoS 
    // (Qos family takes ownership) 
    CSubConQosGenericParamSet* reqGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::ERequested);
    // CleanupDeletePushL(reqGenericParams);

    CSubConQosGenericParamSet* accGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::EAcceptable);
    // CleanupDeletePushL(accGenericParams);

    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet1(*reqGenericParams);
    SetEsockParamSet1(*accGenericParams);

    // Create the technology specific parameter set for QoS 
    CSubConQosIPLinkR99ParamSet* reqIpLink99ParSet = 
        CSubConQosIPLinkR99ParamSet::NewL(*qosFamily, 
                                          CSubConParameterFamily::ERequested);
    // CleanupDeletePushL(reqIpLink99ParSet);

    CSubConQosIPLinkR99ParamSet* accIpLink99ParSet = 
        CSubConQosIPLinkR99ParamSet::NewL(*qosFamily, 
                                         CSubConParameterFamily::EAcceptable);
    // CleanupDeletePushL(accIpLink99ParSet);

    // Set the requested/acceptable technology specific parameter set 
    SetIPLinkR99HighDemand(*reqIpLink99ParSet);
    SetIPLinkR99HighDemandMin(*accIpLink99ParSet);

    TEST(NifAPIL(socket, KDowngrade) == KErrNone);

    // Subscribe for QoS Params notification
    subconn.EventNotification(subconnNotifBuf, ETrue, eventStatus); 
    
    // Set properties of the sub-connection 
    ret = subconn.SetParameters(subconnParams); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn.SetParameters-event 
    // (expected KSubConGenericEventParamsGranted)
    User::WaitForRequest(eventStatus); 
    TESTL(eventStatus.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf, KSubConGenericEventParamsGranted); 
    TESTE(ret == KErrNone, ret);

    RSubConParameterBundle bundle;
    ret = subconn.GetParameters(bundle);
    TESTL(ret == KErrNone);
    
    CSubConParameterFamily* family = bundle.FindFamily(KSubConQoSFamily);
    if(family)
        {
        CSubConQosGenericParamSet* requestedGeneric = 
            (CSubConQosGenericParamSet*)family->GetGenericSet(
            CSubConParameterFamily::ERequested);
        
        if(requestedGeneric)
            {
            // Check that requested generic parameters are correct 
            TEST(requestedGeneric->GetUplinkBandwidth() == 
                 reqGenericParams->GetUplinkBandwidth());
            TEST(requestedGeneric->GetUpLinkMaximumBurstSize() == 
                 reqGenericParams->GetUpLinkMaximumBurstSize());
            TEST(requestedGeneric->GetUpLinkMaximumPacketSize() == 
                 reqGenericParams->GetUpLinkMaximumPacketSize());
            TEST(requestedGeneric->GetUpLinkAveragePacketSize() == 
                 reqGenericParams->GetUpLinkAveragePacketSize());
            TEST(requestedGeneric->GetUpLinkDelay() == 
                 reqGenericParams->GetUpLinkDelay());
            TEST(requestedGeneric->GetUpLinkPriority() == 
                 reqGenericParams->GetUpLinkPriority());
            TEST(requestedGeneric->GetDownlinkBandwidth() == 
                 reqGenericParams->GetDownlinkBandwidth());
            TEST(requestedGeneric->GetDownLinkMaximumBurstSize() == 
                 reqGenericParams->GetDownLinkMaximumBurstSize());
            TEST(requestedGeneric->GetDownLinkMaximumPacketSize() == 
                 reqGenericParams->GetDownLinkMaximumPacketSize());
            TEST(requestedGeneric->GetDownLinkAveragePacketSize() == 
                 reqGenericParams->GetDownLinkAveragePacketSize());
            TEST(requestedGeneric->GetDownLinkDelay() == 
                 reqGenericParams->GetDownLinkDelay());
            TEST(requestedGeneric->GetDownLinkPriority() == 
                 reqGenericParams->GetDownLinkPriority());
            TEST(requestedGeneric->GetHeaderMode() == 
                 reqGenericParams->GetHeaderMode());
            }

        CSubConQosGenericParamSet* acceptbleGeneric = 
            (CSubConQosGenericParamSet*)family->GetGenericSet(
            CSubConParameterFamily::EAcceptable);
        
        if(acceptbleGeneric)
            {
            // Check that acceptable generic parameters are correct 
            TEST(acceptbleGeneric->GetUplinkBandwidth() == 
                 accGenericParams->GetUplinkBandwidth());
            TEST(acceptbleGeneric->GetUpLinkMaximumBurstSize() == 
                 accGenericParams->GetUpLinkMaximumBurstSize());
            TEST(acceptbleGeneric->GetUpLinkMaximumPacketSize() == 
                 accGenericParams->GetUpLinkMaximumPacketSize());
            TEST(acceptbleGeneric->GetUpLinkAveragePacketSize() == 
                 accGenericParams->GetUpLinkAveragePacketSize());
            TEST(acceptbleGeneric->GetUpLinkDelay() == 
                 accGenericParams->GetUpLinkDelay());
            TEST(acceptbleGeneric->GetUpLinkPriority() == 
                 accGenericParams->GetUpLinkPriority());
            TEST(acceptbleGeneric->GetDownlinkBandwidth() == 
                 accGenericParams->GetDownlinkBandwidth());
            TEST(acceptbleGeneric->GetDownLinkMaximumBurstSize() == 
                 accGenericParams->GetDownLinkMaximumBurstSize());
            TEST(acceptbleGeneric->GetDownLinkMaximumPacketSize() == 
                 accGenericParams->GetDownLinkMaximumPacketSize());
            TEST(acceptbleGeneric->GetDownLinkAveragePacketSize() == 
                 accGenericParams->GetDownLinkAveragePacketSize());
            TEST(acceptbleGeneric->GetDownLinkDelay() == 
                 accGenericParams->GetDownLinkDelay());
            TEST(acceptbleGeneric->GetDownLinkPriority() == 
                 accGenericParams->GetDownLinkPriority());
            TEST(acceptbleGeneric->GetHeaderMode() == 
                 accGenericParams->GetHeaderMode());
            }

        CSubConQosIPLinkR99ParamSet* requestedIpParams = 
            (CSubConQosIPLinkR99ParamSet*)family->FindExtensionSet(
            TUid::Uid(KSubConQosIPLinkR99ParamsType), 
            CSubConParameterFamily::ERequested);

            if(requestedIpParams)
                {
                // Check that requested technology specific parameters 
                // are correct
                TEST(requestedIpParams->GetTrafficClass() == 
                     reqIpLink99ParSet->GetTrafficClass());
                TEST(requestedIpParams->GetDeliveryOrder() == 
                     reqIpLink99ParSet->GetDeliveryOrder());
                TEST(requestedIpParams->GetErroneousSDUDelivery() == 
                     reqIpLink99ParSet->GetErroneousSDUDelivery());
                TEST(requestedIpParams->GetResidualBitErrorRatio() == 
                     reqIpLink99ParSet->GetResidualBitErrorRatio());
                TEST(requestedIpParams->GetSDUErrorRatio() == 
                     reqIpLink99ParSet->GetSDUErrorRatio());
                TEST(requestedIpParams->GetTrafficHandlingPriority() == 
                     reqIpLink99ParSet->GetTrafficHandlingPriority());
                TEST(requestedIpParams->GetTransferDelay() == 
                     reqIpLink99ParSet->GetTransferDelay());
                TEST(requestedIpParams->GetMaxSduSize() == 
                     reqIpLink99ParSet->GetMaxSduSize());
                TEST(requestedIpParams->GetMaxBitrateUplink() == 
                     reqIpLink99ParSet->GetMaxBitrateUplink());
                TEST(requestedIpParams->GetMaxBitrateDownlink() == 
                     reqIpLink99ParSet->GetMaxBitrateDownlink());
                TEST(requestedIpParams->GetGuaBitrateUplink() == 
                     reqIpLink99ParSet->GetGuaBitrateUplink());
                TEST(requestedIpParams->GetGuaBitrateDownlink() == 
                     reqIpLink99ParSet->GetGuaBitrateDownlink());
                }
        
        CSubConQosIPLinkR99ParamSet* acceptbleIpParams = 
            (CSubConQosIPLinkR99ParamSet*)family->FindExtensionSet(
            TUid::Uid(KSubConQosIPLinkR99ParamsType), 
            CSubConParameterFamily::EAcceptable);

            if(acceptbleIpParams)
                {
                // Check that acceptable technology specific parameters 
                // are correct
                TEST(acceptbleIpParams->GetTrafficClass() == 
                     accIpLink99ParSet->GetTrafficClass());
                TEST(acceptbleIpParams->GetDeliveryOrder() == 
                     accIpLink99ParSet->GetDeliveryOrder());
                TEST(acceptbleIpParams->GetErroneousSDUDelivery() == 
                     accIpLink99ParSet->GetErroneousSDUDelivery());
                TEST(acceptbleIpParams->GetResidualBitErrorRatio() == 
                     accIpLink99ParSet->GetResidualBitErrorRatio());
                TEST(acceptbleIpParams->GetSDUErrorRatio() == 
                     accIpLink99ParSet->GetSDUErrorRatio());
                TEST(acceptbleIpParams->GetTrafficHandlingPriority() == 
                     accIpLink99ParSet->GetTrafficHandlingPriority());
                TEST(acceptbleIpParams->GetTransferDelay() == 
                     accIpLink99ParSet->GetTransferDelay());
                TEST(acceptbleIpParams->GetMaxSduSize() == 
                     accIpLink99ParSet->GetMaxSduSize());
                TEST(acceptbleIpParams->GetMaxBitrateUplink() == 
                     accIpLink99ParSet->GetMaxBitrateUplink());
                TEST(acceptbleIpParams->GetMaxBitrateDownlink() == 
                     accIpLink99ParSet->GetMaxBitrateDownlink());
                TEST(acceptbleIpParams->GetGuaBitrateUplink() == 
                     accIpLink99ParSet->GetGuaBitrateUplink());
                TEST(acceptbleIpParams->GetGuaBitrateDownlink() == 
                     accIpLink99ParSet->GetGuaBitrateDownlink());
                }
        }

    // Clean up
    // To reset the message values of the testnif
    TEST(NifAPIL(socket, KResetMessage) == KErrNone);

    subconn.Close();
    subconnParams.Close(); 
    
    socket.Close();
    
    conn.Stop();
    conn.Close();

    // CleanupStack::Pop(accIpLink99ParSet);
    // CleanupStack::Pop(reqIpLink99ParSet);
    // CleanupStack::Pop(accGenericParams); 
    // CleanupStack::Pop(reqGenericParams); 
    CleanupStack::Pop(qosFamily); 
    CleanupStack::Pop(&socketServer); 
    socketServer.Close();

    return iTestStepResult;
    }


CQoSTest_073::CQoSTest_073()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText205, "qos_073");
    iTestStepName = KText205;
    }

CQoSTest_073::~CQoSTest_073()
    {
    }

TVerdict CQoSTest_073::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText206, "qos_073, CSubConQosIPLinkR99ParamSet, Requested \
parameters, header compression");
    Log( KText206 );

    RSocketServ             socketServer;
    RSocket                 socket;
    TInetAddr               dstAddr;
    RConnection             conn; 
    RSubConnection          subconn; 
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf;
    TInt                    ret; 

    // Connect to ESOCK 
    ret = socketServer.Connect();
    CleanupClosePushL(socketServer); 
    TESTL(ret == KErrNone); 

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP sockets 
    ret = socket.Open(socketServer, 
                      KAfInet, 
                      KSockDatagram, 
                      KProtocolInetUdp, 
                      conn);
    TESTL(ret == KErrNone); 

    _LIT(KText204, "10.1.73.1");
    dstAddr.Input( KText204 );
    dstAddr.SetPort(3731);
    socket.Connect(dstAddr, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    // Create a new sub-connection 
    ret = subconn.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket onto the new sub-connection
    subconn.Add(socket, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Create the container for all subconnection parameters 
    RSubConParameterBundle subconnParams; 
    
    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily = 
        CSubConParameterFamily::NewL(subconnParams, KSubConQoSFamily);
    CleanupDeletePushL(qosFamily);

    // Create the requested generic parameter set for QoS 
    // (Qos family takes ownership) 
    CSubConQosGenericParamSet* reqGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::ERequested);
    CleanupDeletePushL(reqGenericParams);

    CSubConQosGenericParamSet* accGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::EAcceptable);
    CleanupDeletePushL(accGenericParams);

    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet1(*reqGenericParams);
    SetEsockParamSet1(*accGenericParams);

    // Create the technology specific parameter set for QoS 
    CSubConQosIPLinkR99ParamSet* reqIpLink99ParSet = 
        CSubConQosIPLinkR99ParamSet::NewL(*qosFamily, 
                                          CSubConParameterFamily::ERequested);
    CleanupDeletePushL(reqIpLink99ParSet);

    CSubConQosIPLinkR99ParamSet* accIpLink99ParSet = 
        CSubConQosIPLinkR99ParamSet::NewL(*qosFamily, 
                                         CSubConParameterFamily::EAcceptable);
    CleanupDeletePushL(accIpLink99ParSet);

    // Set the requested/acceptable technology specific parameter set 
    SetIPLinkR99HighDemand(*reqIpLink99ParSet);
    SetIPLinkR99HighDemandMin(*accIpLink99ParSet);

    /*
     * SetHeaderCompression is not yet supported. 
     * 
     * Until there is a support for it in RSubConnection, 
     * there is no reason for executing this test case.  
     *
    TEST(subconnParams->SetHeaderCompression(KPdpHeaderCompression) == 
        KErrNone);
     */

    // Subscribe for QoS Params notification
    subconn.EventNotification(subconnNotifBuf, ETrue, eventStatus); 
    
    // Set properties of the sub-connection 
    ret = subconn.SetParameters(subconnParams); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn.SetParameters-event 
    // (expected KSubConGenericEventParamsGranted)
    User::WaitForRequest(eventStatus); 
    TESTL(eventStatus.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf, KSubConGenericEventParamsGranted); 
    TESTE(ret == KErrNone, ret);

    // Clean up
    subconn.Close();
    subconnParams.Close(); 
    
    socket.Close();
    
    conn.Stop();
    conn.Close();
    
    CleanupStack::Pop(accIpLink99ParSet);
    CleanupStack::Pop(reqIpLink99ParSet);
    CleanupStack::Pop(accGenericParams); 
    CleanupStack::Pop(reqGenericParams); 
    CleanupStack::Pop(qosFamily); 
    CleanupStack::Pop(&socketServer); 
    socketServer.Close();

    return iTestStepResult;
    }


CQoSTest_075::CQoSTest_075()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText208, "qos_075");
    iTestStepName = KText208;
    }

CQoSTest_075::~CQoSTest_075()
    {
    }

TVerdict CQoSTest_075::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText209, "qos_075, CSubConQosIPLinkR99ParamSet, Activating context \
fails, return UMTS specific error");
    Log( KText209 );

    RSocketServ             socketServer;
    RSocket                 socket;
    TInetAddr               dstAddr;
    RConnection             conn; 
    RSubConnection          subconn; 
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf;
    TInt                    ret; 

    // Connect to ESOCK 
    ret = socketServer.Connect();
    CleanupClosePushL(socketServer); 
    TESTL(ret == KErrNone); 

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP sockets 
    ret = socket.Open(socketServer, 
                      KAfInet, 
                      KSockDatagram, 
                      KProtocolInetUdp, 
                      conn);
    TESTL(ret == KErrNone); 

    _LIT(KText204, "10.1.75.1");
    dstAddr.Input( KText204 );
    dstAddr.SetPort(3751);
    socket.Connect(dstAddr, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone); 

    // Create a new sub-connection 
    ret = subconn.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket onto the new sub-connection
    subconn.Add(socket, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Create the container for all subconnection parameters 
    RSubConParameterBundle subconnParams; 
    
    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily = 
        CSubConParameterFamily::NewL(subconnParams, KSubConQoSFamily);
    CleanupDeletePushL(qosFamily);

    // Create the requested generic parameter set for QoS 
    // (Qos family takes ownership) 
    CSubConQosGenericParamSet* reqGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::ERequested);
    CleanupDeletePushL(reqGenericParams);

    CSubConQosGenericParamSet* accGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::EAcceptable);
    CleanupDeletePushL(accGenericParams);

    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet1(*reqGenericParams);
    SetEsockParamSet1(*accGenericParams);

    // Create the technology specific parameter set for QoS 
    CSubConQosIPLinkR99ParamSet* reqIpLink99ParSet = 
        CSubConQosIPLinkR99ParamSet::NewL(*qosFamily, 
                                          CSubConParameterFamily::ERequested);
    CleanupDeletePushL(reqIpLink99ParSet);

    CSubConQosIPLinkR99ParamSet* accIpLink99ParSet = 
        CSubConQosIPLinkR99ParamSet::NewL(*qosFamily, 
                                         CSubConParameterFamily::EAcceptable);
    CleanupDeletePushL(accIpLink99ParSet);

    // Set the requested/acceptable technology specific parameter set 
    SetIPLinkR99HighDemand(*reqIpLink99ParSet);
    SetIPLinkR99HighDemandMin(*accIpLink99ParSet);

    /*
     * SetHeaderCompression is not yet supported. 
     * 
     * Until there is a support for it in RSubConnection, 
     * there is no reason for executing this test case.  
     *
    TEST(subconnParams->SetHeaderCompression(KPdpHeaderCompression) == 
        KErrNone);
     */

    TEST(NifAPIL(socket, KContextActivateFailAsync) == KErrNone);

    // Subscribe for QoS Params notification
    subconn.EventNotification(subconnNotifBuf, ETrue, eventStatus); 
    
    // Set properties of the sub-connection 
    ret = subconn.SetParameters(subconnParams); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn.SetParameters-event 
    User::WaitForRequest(eventStatus); 
    TESTL(eventStatus.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf, KSubConGenericEventParamsRejected); 
    TESTE(ret == KErrNone, ret);

    // Clean up
    // To reset the message values of the testnif
    TEST(NifAPIL(socket, KResetMessage) == KErrNone);

    subconn.Close();
    subconnParams.Close(); 
    
    socket.Close();
    
    conn.Stop();
    conn.Close();
    
    CleanupStack::Pop(accIpLink99ParSet);
    CleanupStack::Pop(reqIpLink99ParSet);
    CleanupStack::Pop(accGenericParams); 
    CleanupStack::Pop(reqGenericParams); 
    CleanupStack::Pop(qosFamily); 
    CleanupStack::Pop(&socketServer); 
    socketServer.Close();

    return iTestStepResult;
    }


CQoSTest_076::CQoSTest_076()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText211, "qos_076");
    iTestStepName = KText211;
    }

CQoSTest_076::~CQoSTest_076()
    {
    }

TVerdict CQoSTest_076::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText212, "qos_076, CSubConQosIPLinkR99ParamSet, RemoveExtension()");
    Log( KText212 );

    RSocketServ             socketServer;
    RSocket                 socket;
    TInetAddr               dstAddr;
    RConnection             conn; 
    RSubConnection          subconn; 
    TRequestStatus          status, eventStatus;
    TNotificationEventBuf   subconnNotifBuf;
    TInt                    ret; 

    // Connect to ESOCK 
    ret = socketServer.Connect();
    CleanupClosePushL(socketServer); 
    TESTL(ret == KErrNone); 

    // Open a connection 
    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 

    // Start the connection 
    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);

    // Open UDP sockets 
    ret = socket.Open(socketServer, 
                      KAfInet, 
                      KSockDatagram, 
                      KProtocolInetUdp, 
                      conn);
    TESTL(ret == KErrNone); 

    _LIT(KText204, "10.1.76.1");
    dstAddr.Input( KText204 );
    dstAddr.SetPort(3761);
    socket.Connect(dstAddr, status);
    User::WaitForRequest(status);

    // Create a new sub-connection 
    ret = subconn.Open(socketServer, 
                       RSubConnection::ECreateNew, 
                       conn); 
    TESTL(ret == KErrNone);

    // Move the connected socket onto the new sub-connection
    subconn.Add(socket, status);
    User::WaitForRequest(status);
    TESTL(status.Int() == KErrNone);

    // Create the container for all subconnection parameters 
    RSubConParameterBundle subconnParams; 
    
    // Create a container for QoS subconnection parameters 
    // (Param bundle takes ownership) 
    CSubConParameterFamily* qosFamily = 
        CSubConParameterFamily::NewL(subconnParams, KSubConQoSFamily);
    CleanupDeletePushL(qosFamily);

    // Create the requested generic parameter set for QoS 
    // (Qos family takes ownership) 
    CSubConQosGenericParamSet* reqGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::ERequested);
    CleanupDeletePushL(reqGenericParams);

    CSubConQosGenericParamSet* accGenericParams = 
        CSubConQosGenericParamSet::NewL(*qosFamily, 
                                        CSubConParameterFamily::EAcceptable);
    CleanupDeletePushL(accGenericParams);

    // Set the requested/acceptable Generic Parameters
    SetEsockParamSet1(*reqGenericParams);
    SetEsockParamSet1(*accGenericParams);

    // Create the technology specific parameter set for QoS 
    CSubConQosIPLinkR99ParamSet* reqIpLink99ParSet = 
        CSubConQosIPLinkR99ParamSet::NewL(*qosFamily, 
                                          CSubConParameterFamily::ERequested);
    CleanupDeletePushL(reqIpLink99ParSet);

    CSubConQosIPLinkR99ParamSet* accIpLink99ParSet = 
        CSubConQosIPLinkR99ParamSet::NewL(*qosFamily, 
                                         CSubConParameterFamily::EAcceptable);
    CleanupDeletePushL(accIpLink99ParSet);

    // Set the requested/acceptable technology specific parameter set 
    SetIPLinkR99HighDemand(*reqIpLink99ParSet);
    SetIPLinkR99HighDemandMin(*accIpLink99ParSet);

    /*
     * RemoveExtension is not yet supported. 
     * 
     * Until there is a support for it in RSubConnection, 
     * there is no reason for executing this test case.  
     *
    TEST(subconnParams->RemoveExtension(KPfqosExtensionUmts) == KErrNone);
    */

    // Subscribe for QoS Params notification
    subconn.EventNotification(subconnNotifBuf, ETrue, eventStatus); 
    
    // Set properties of the sub-connection 
    ret = subconn.SetParameters(subconnParams); 
    TESTL(ret == KErrNone);

    // Receive & Process the subconn.SetParameters-event 
    // (expected KSubConGenericEventParamsGranted)
    User::WaitForRequest(eventStatus); 
    TESTL(eventStatus.Int() == KErrNone);
    ret = CheckEvent(subconnNotifBuf, KSubConGenericEventParamsGranted); 
    TESTE(ret == KErrNone, ret);

    // Clean up
    subconn.Close();
    subconnParams.Close(); 
    
    socket.Close();
    
    conn.Stop();
    conn.Close();
    
    CleanupStack::Pop(accIpLink99ParSet);
    CleanupStack::Pop(reqIpLink99ParSet);
    CleanupStack::Pop(accGenericParams); 
    CleanupStack::Pop(reqGenericParams); 
    CleanupStack::Pop(qosFamily); 
    CleanupStack::Pop(&socketServer); 
    socketServer.Close();

    return iTestStepResult;
    }




//
//  TQoSSelector test cases
//

CQoSTest_010::CQoSTest_010()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText15, "qos_010");
    iTestStepName = KText15;
    }

// destructor
CQoSTest_010::~CQoSTest_010()
    {
    }

TVerdict CQoSTest_010::doTestStepL( void )
    {
    _LIT(KText16, "qos_010: TQoSSelector, Init values by Set(RSocket& \
aSocket)");
    Log( KText16 );

    RSocketServ socketServer;
    RSocket socket;
    TRequestStatus status;
    TUint dstPort = 3101;
    TUint localPort = 3102;

    TInetAddr dstAddr;
    dstAddr.SetPort(dstPort);
    _LIT(KText17, "10.1.10.1");
    dstAddr.Input( KText17 );
    TInetAddr mask;
    mask.SetAddress(KInet6AddrMask);

    socketServer.Connect();
    socket.Open(socketServer, KAfInet, KSockDatagram, KProtocolInetUdp);
    socket.SetLocalPort(localPort);
    socket.Connect(dstAddr, status);
    User::WaitForRequest(status);

    TSockAddr localSockAddr;
    socket.LocalName(localSockAddr);
    TInetAddr localInetAddr(localSockAddr);

    dstAddr.ConvertToV4Mapped();
    localInetAddr.ConvertToV4Mapped();

    TQoSSelector selector;
    TEST(selector.SetAddr(socket) == KErrNone);
    // Cannot test the source address here. Check test document for info
    //TEST(selector.GetSrc().Match(localInetAddr));
    TEST(selector.GetDst().Match(dstAddr));
    TEST(selector.GetSrcMask().Match(mask));
    TEST(selector.GetDstMask().Match(mask));
    TEST(selector.IapId() == 0);
    TEST(selector.Protocol() == KProtocolInetUdp);
    TEST(selector.MaxPortSrc() == localPort);
    TEST(selector.MaxPortDst() == dstPort);

    socket.Close();
    socketServer.Close();

    return iTestStepResult;
    }



CQoSTest_011::CQoSTest_011()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText18, "qos_011");
    iTestStepName = KText18;
    }

// destructor
CQoSTest_011::~CQoSTest_011()
    {
    }

TVerdict CQoSTest_011::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText19, "qos_011: TQoSSelector, Init values by overloaded Set()");
    Log( KText19 );

    TUint protocol = 17;
    TUint srcPort = 3111;
    TUint dstPort = 3112;
    TUint srcPortMax = 3113;
    TUint dstPortMax = 3114;
    TInetAddr src, dst, mask;

    _LIT(KText20, "10.1.11.1");
    src.Input( KText20 );
    src.SetPort(srcPort);
    mask.SetAddress(KInet6AddrMask);
    
    _LIT(KText21, "10.1.11.2");
    dst.Input( KText21 );
    dst.SetPort(dstPort);

    dst.ConvertToV4Mapped();
    src.ConvertToV4Mapped();

    TQoSSelector selector;
    TEST(selector.SetAddr(src, mask, dst, mask, protocol, srcPortMax, 
         dstPortMax) == KErrNone);
    TEST(selector.GetSrc().Match(src));
    TEST(selector.GetDst().Match(dst));
    TEST(selector.GetSrcMask().Match(mask));
    TEST(selector.GetDstMask().Match(mask));
    TEST(selector.IapId() == 0);
    TEST(selector.Protocol() == protocol);
    TEST(selector.MaxPortSrc() == srcPortMax);
    TEST(selector.MaxPortDst() == dstPortMax);

    return iTestStepResult;
    }



CQoSTest_012::CQoSTest_012()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText22, "qos_012");
    iTestStepName = KText22;
    }

// destructor
CQoSTest_012::~CQoSTest_012()
    {
    }

TVerdict CQoSTest_012::doTestStepL( void )
    {
    _LIT(KText23, "qos_012 Testing TQoSSelector, Init values by separate Set \
functions");
    Log( KText23 );

    TUint protocol = 17;
    TUint srcPort = 3121;
    TUint dstPort = 3122;
    TInt iapId = 999;
    TUint srcPortMax = 3123;
    TUint dstPortMax = 3124;
    TInetAddr src, dst, mask;
    _LIT(KText24, "10.1.12.1");
    src.Input( KText24 );
    src.SetPort(srcPort);
    _LIT(KText25, "10.1.12.2");
    dst.Input( KText25 );
    dst.SetPort(dstPort);
    mask.SetAddress(KInet6AddrNone);

    dst.ConvertToV4Mapped();
    src.ConvertToV4Mapped();

    TQoSSelector selector;
    TEST(selector.SetSrc(src) == KErrNone);
    TEST(selector.SetDst(dst) == KErrNone);
    selector.SetIapId(iapId);
    selector.SetProtocol(protocol);
    TEST(selector.SetMaxPortSrc(srcPortMax) == KErrNone);
    TEST(selector.SetMaxPortDst(dstPortMax) == KErrNone);
    
    TEST(selector.GetSrc().Match(src));
    TEST(selector.GetDst().Match(dst));
    TEST(selector.GetSrcMask().Match(mask));
    TEST(selector.GetDstMask().Match(mask));
    TEST(selector.IapId() == iapId);
    TEST(selector.Protocol() == protocol);
    TEST(selector.MaxPortSrc() == srcPortMax);
    TEST(selector.MaxPortDst() == dstPortMax);
    
    return iTestStepResult;
    }



CQoSTest_013::CQoSTest_013()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText26, "qos_013");
    iTestStepName = KText26;
    }

// destructor
CQoSTest_013::~CQoSTest_013()
    {
    }

TVerdict CQoSTest_013::doTestStepL( void )
    {
    _LIT(KText27, "qos_013 Testing TQoSSelector, Operator==(), Comparing \
selectors");
    Log( KText27 );

    RSocketServ socketServer;
    RSocket socket;
    TUint protocol = 17;
    TUint srcPort = 3131;
    TUint dstPort = 3132;
    TQoSSelector selector1, selector2;
    TInetAddr srcAddr, dstAddr(dstPort), srcMask, dstMask;    
    TSockAddr localSockAddr;
    TRequestStatus status;

    _LIT(KText28, "10.1.13.1");
    dstAddr.Input( KText28 );
    _LIT(KText29, "10.1.13.2");
    dstMask.Input( KText29 );

    socketServer.Connect();
    socket.Open(socketServer, KAfInet, KSockDatagram, KProtocolInetUdp);
    socket.SetLocalPort(srcPort);
    socket.Connect(dstAddr, status);
    User::WaitForRequest(status);

    socket.LocalName(localSockAddr);
    srcAddr = TInetAddr(localSockAddr);
    TEST(selector1.SetAddr(socket) == KErrNone);
    TEST(selector2.SetAddr(socket) == KErrNone);
    TEST(selector1 == selector2);

    TEST(selector1.SetAddr(srcAddr, srcMask, dstAddr, dstMask, protocol, 
         srcPort, dstPort) == KErrNone);
    TEST(selector2.SetAddr(srcAddr, srcMask, dstAddr, dstMask, protocol, 
         srcPort, dstPort) == KErrNone);
    TEST(selector1 == selector2);

    socket.Close();
    socketServer.Close();

    return iTestStepResult;
    }



CQoSTest_014::CQoSTest_014()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText30, "qos_014");
    iTestStepName = KText30;
    }

// destructor
CQoSTest_014::~CQoSTest_014()
    {
    }

TVerdict CQoSTest_014::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText31, "qos_014 Testing TQoSSelector, operator==(), comparing non \
matching selectors");
    Log( KText31 );

    RSocketServ socketServer;
    RSocket socket;
    // TUint protocol = 17;
    TUint srcPort = 3141;
    TUint dstPort = 3142;
    TQoSSelector selector1, selector2;
    TInetAddr dstAddr(dstPort);
    TRequestStatus status;

    _LIT(KText32, "10.1.14.1");
    dstAddr.Input( KText32 );

    socketServer.Connect();
    socket.Open(socketServer, KAfInet, KSockDatagram, KProtocolInetUdp);
    socket.SetLocalPort(srcPort);
    socket.Connect(dstAddr, status);
    User::WaitForRequest(status);

    TEST(selector1.SetAddr(socket) == KErrNone);
    TEST(selector2.SetAddr(socket) == KErrNone);
    TEST(selector1 == selector2);

    selector1.SetIapId(123);
    TEST(!(selector1 == selector2));

    socket.Close();
    socketServer.Close();

    return iTestStepResult;
    }



CQoSTest_015::CQoSTest_015()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText33, "qos_015");
    iTestStepName = KText33;
    }

// destructor
CQoSTest_015::~CQoSTest_015()
    {
    }

TVerdict CQoSTest_015::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText34, "qos_015 Testing TQoSSelector, Match()");
    Log( KText34 );

    RSocketServ socketServer;
    RSocket socket;
    TRequestStatus status;
    TUint dstPort = 3151;
    TUint localPort = 3152;

    TInetAddr dstAddr;
    dstAddr.SetPort(dstPort);
    _LIT(KText35, "10.1.15.1");
    dstAddr.Input( KText35 );
    TInetAddr mask;
    _LIT(KText36, "255.255.255.255");
    mask.Input( KText36 );

    socketServer.Connect();
    socket.Open(socketServer, KAfInet, KSockDatagram, KProtocolInetUdp);
    socket.SetLocalPort(localPort);
    socket.Connect(dstAddr, status);
    User::WaitForRequest(status);

    TQoSSelector selector;
    TEST(selector.SetAddr(socket) == KErrNone);
    TEST(selector.Match(socket));

    socket.Close();
    socketServer.Close();

    return iTestStepResult;
    }



CQoSTest_016::CQoSTest_016()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText37, "qos_016");
    iTestStepName = KText37;
    }

// destructor
CQoSTest_016::~CQoSTest_016()
    {
    }

TVerdict CQoSTest_016::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText38, "qos_016 Testing TQoSSelector, Match() fails");
    Log( KText38 );

    RSocketServ socketServer;
    RSocket socket;
    TRequestStatus status;
    TUint dstPort = 3161;
    TUint localPort = 3162;

    TInetAddr dstAddr;
    dstAddr.SetPort(dstPort);
    _LIT(KText39, "10.1.16.1");
    dstAddr.Input( KText39 );
    TInetAddr mask;
    _LIT(KText40, "255.255.255.255");
    mask.Input( KText40 );

    socketServer.Connect();
    socket.Open(socketServer, KAfInet, KSockDatagram, KProtocolInetUdp);
    socket.SetLocalPort(localPort);
    socket.Connect(dstAddr, status);
    User::WaitForRequest(status);

    TQoSSelector selector;
    TEST(selector.SetAddr(socket) == KErrNone);
    TEST(selector.Match(socket));

    selector.SetIapId(123);
    TEST(!selector.Match(socket));

    socket.Close();
    socketServer.Close();

    return iTestStepResult;
    }


//
//  RQoSPolicy test cases
//


CQoSTest_050::CQoSTest_050()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText148, "qos_050");
    iTestStepName = KText148;
    }

CQoSTest_050::~CQoSTest_050()
    {
    }

TVerdict CQoSTest_050::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText149, "qos_050 RQoSPolicy, Open");
    Log( KText149 );

    RSocketServ socketServer;
    RSocket socket;
    TInetAddr dstAddr;
    TRequestStatus status;
    
    socketServer.Connect();
    socket.Open(socketServer, KAfInet, KSockDatagram, KProtocolInetUdp);

    dstAddr.SetPort(3501);
    _LIT(KText150, "10.1.50.1");
    dstAddr.Input( KText150 );
    socket.Connect(dstAddr, status);
    User::WaitForRequest(status);

    TQoSSelector selector;
    selector.SetAddr(socket);
    RQoSPolicy policy;
    TEST(policy.Open(selector) == KErrNone);

    TEST(policy.Close() == KErrNone);
    socket.Close();
    socketServer.Close();
    return iTestStepResult;
    }


CQoSTest_051::CQoSTest_051()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText151, "qos_051");
    iTestStepName = KText151;
    }

CQoSTest_051::~CQoSTest_051()
    {
    }

TVerdict CQoSTest_051::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText152, "qos_051 RQoSPolicy, SetQoS fails without Open()");
    Log( KText152 );

    RSocketServ socketServer;
    RSocket socket;
    TInetAddr dstAddr;
    TRequestStatus status;
    
    socketServer.Connect();
    socket.Open(socketServer, KAfInet, KSockDatagram, KProtocolInetUdp);

    dstAddr.SetPort(3511);
    _LIT(KText153, "10.1.51.1");
    dstAddr.Input( KText153 );
    socket.Connect(dstAddr, status);
    User::WaitForRequest(status);

    TQoSSelector selector;
    selector.SetAddr(socket);
    RQoSPolicy policy;
    CQoSParameters *parameter = new(ELeave) CQoSParameters();
    SetQoSParamSet1(*parameter);
    TEST(policy.SetQoS(*parameter) == KErrNotReady);

    TEST(policy.Close() == KErrNotReady);
    socket.Close();
    socketServer.Close();
    delete parameter;
    return iTestStepResult;
    }


CQoSTest_053::CQoSTest_053()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText157, "qos_053");
    iTestStepName = KText157;
    }

CQoSTest_053::~CQoSTest_053()
    {
    }

TVerdict CQoSTest_053::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText158, "qos_053 RQoSPolicy, GetQoS, when no SetQoS");
    Log( KText158 );

    RSocketServ socketServer;
    RSocket socket;
    TInetAddr dstAddr;
    TRequestStatus status;
    
    socketServer.Connect();
    socket.Open(socketServer, KAfInet, KSockDatagram, KProtocolInetUdp);

    dstAddr.SetPort(3531);
    _LIT(KText159, "10.1.53.1");
    dstAddr.Input( KText159 );
    socket.Connect(dstAddr, status);
    User::WaitForRequest(status);

    TQoSSelector selector;
    TEST(selector.SetAddr(socket) == KErrNone);
    RQoSPolicy policy;
    TEST(policy.Open(selector) == KErrNone);
    TEST(policy.NotifyEvent(*this) == KErrNone);
    TEST(policy.GetQoS() == KErrNone);
    WaitForQoSEventL();
    TESTE(iReceivedEvent == EQoSEventGetPolicy, iReceivedEvent);
    if(iReceivedEvent == EQoSEventGetPolicy)
        {
        TESTE(iReason == KErrNotFound, iReason);
        }

    TEST(policy.CancelNotifyEvent(*this) == KErrNone);
    TEST(policy.Close() == KErrNone);

    socket.Close();
    socketServer.Close();
    return iTestStepResult;
    }


CQoSTest_054::CQoSTest_054()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText160, "qos_054");
    iTestStepName = KText160;
    }

CQoSTest_054::~CQoSTest_054()
    {
    }

TVerdict CQoSTest_054::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText161, "qos_054 RQoSPolicy, Legacy application support");
    Log( KText161 );

    TInetAddr dstAddr;
    TRequestStatus status;
    RSocketServ socketServer;
    socketServer.Connect();

    RSocket socket;
    socket.Open(socketServer, KAfInet, KSockDatagram, KProtocolInetUdp);
    _LIT(KText162, "10.1.54.1");
    dstAddr.Input( KText162 );
    dstAddr.SetPort(3541);
    socket.Connect(dstAddr, status);
    User::WaitForRequest(status);

    _LIT(KPolicyFile, "c:\\qospolicieslegacy.ini");
    TQoSSelector selector;
    RQoSPolicy policy;
    policy.Open(selector);
    policy.NotifyEvent(*this);
    TEST(policy.LoadPolicyFile(KPolicyFile) == KErrNone);
    WaitForQoSEventL();
    TESTE(iReceivedEvent == EQoSEventLoadPolicyFile, iReceivedEvent);
    if(iReceivedEvent == EQoSEventLoadPolicyFile)
        {
        if(iReason != KErrNone)
            {
            _LIT(KText163, "Policyfile qospolicieslegacy.ini not found!");
            Log( KText163 );
            return EInconclusive;
            }
        }

    TEST(NifAPIL(socket, KNotifySecondaryCreated) == KErrNone);

    // Sending to udp socket should create a secondary pdp context
    TEST(SendData(socket) == KErrNone);

    // Delay, which gives nif time to write the testfi in case
    // creation of secondary pdp context succeess.
    RTimer timer;
    timer.CreateLocal();
    timer.After(status, 20000000);
    User::WaitForRequest(status);
    timer.Close();
    
    TEST(CheckTestFile(KTestFile1));

    TEST(policy.UnloadPolicyFile(KPolicyFile) == KErrNone);
    WaitForQoSEventL();
    TESTE(iReceivedEvent == EQoSEventUnloadPolicyFile, iReceivedEvent);
    TESTE(iReason == KErrNone, iReason);


    TEST(policy.CancelNotifyEvent(*this) == KErrNone);

    // To reset the message values of the testnif
    TEST(NifAPIL(socket, KResetMessage) == KErrNone);

    TEST(policy.Close() == KErrNone);

    socket.Close();
    socketServer.Close();

    ClearTestFiles();

    return iTestStepResult;

    /*
    
    Note: After sending data to the nif, the secondary context 
    would be created and we need to provide a delay for the nif 
    inorder to notify us back. This Rtimer delay in the testcase 
    should be lesser than the lastsocketactivity timeout (long timeout ) 
    value specified in the commdb. Because this long timeout will 
    determine the destruction of the nif. And once the nif is destroyed 
    we won't get the notification back (the KTestFile1 will not be created).    
    
    */
    }



CQoSTest_055::CQoSTest_055()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText164, "qos_055");
    iTestStepName = KText164;
    }

CQoSTest_055::~CQoSTest_055()
    {
    }

TVerdict CQoSTest_055::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText165, "qos_055 RQoSPolicy, Additional modules needed");
    Log( KText165 );

    TInt err = CreateTestFile(KUseNoModule);
    if(err != KErrNone)
        {
        _LIT(KText166, "Creating testfile failed, aborting test");
        Log( KText166 );
        return EInconclusive;
        }


    _LIT(KPolicyFile, "c:\\qospoliciesadd1.ini");
    TQoSSelector selector;
    selector.SetProtocol(17);
    RQoSPolicy policy;
    policy.Open(selector);
    policy.NotifyEvent(*this);

    TEST(policy.LoadPolicyFile(KPolicyFile) == KErrNone);
    WaitForQoSEventL();
    if(iReason == KErrNotFound)
        {
        _LIT(KText167, "Policyfile qospoliciesadd1.ini not found. TestCase \
aborted");
        Log( KText167 );
        return EInconclusive;

        }

    TESTE(iReceivedEvent == EQoSEventLoadPolicyFile, iReceivedEvent);
    TESTE(iReason == KErrNone, iReason);


    TInetAddr dstAddr;
    TRequestStatus status;
    RSocketServ socketServer;
    socketServer.Connect();

    RSocket socket;
    socket.Open(socketServer, KAfInet, KSockDatagram, KProtocolInetUdp);
    _LIT(KText168, "10.1.55.1");
    dstAddr.Input( KText168 );
    dstAddr.SetPort(3551);
    socket.Connect(dstAddr, status);
    User::WaitForRequest(status);

    //TEST(SendData(socket) == KErrNone);
    WakeupNifL();

    // Check that test module is loaded
    TEST(CheckTestFile(KTestModuleLoaded));

    err = KErrNone;
    err = policy.UnloadPolicyFile(KPolicyFile);
    TESTE(err == KErrNone, err);

    WaitForQoSEventL();
    TESTE(iReceivedEvent == EQoSEventUnloadPolicyFile, iReceivedEvent);
    TESTE(iReason == KErrNone, iReason);


    /*


    // It could be a few seconds before we can unload the policyfile. 
    for (int i = 0; i < 3; i++)
        {
        policy.UnloadPolicyFile(KPolicyFile);
        WaitForQoSEventL();
        // Following 2 lines commented out.
        //TESTE(iReceivedEvent == EQoSEventUnloadPolicyFile, iReceivedEvent);
        //TESTE(iReason == KErrNone, iReason);

        if (iReceivedEvent == EQoSEventUnloadPolicyFile)
            break;

        RTimer delay;
        delay.CreateLocal();
        delay.After(status, 500000);
        User::WaitForRequest(status);
        delay.Close();
        }
        
        
    */
    
    TEST(policy.CancelNotifyEvent(*this) == KErrNone);

    policy.Close();
    socket.Close();
    socketServer.Close();

    

    // To give testmodule time to unload after socket is closed, do a delay
    RTimer timer;
    timer.CreateLocal();
    timer.After(status, 30000000);
    User::WaitForRequest(status);
    timer.Close(); 

    // Check that test module is unloaded
    TEST(CheckTestFile(KTestModuleUnloaded));
    ClearTestFiles();
    

    return iTestStepResult;
    }


CQoSTest_056::CQoSTest_056()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText169, "qos_056");
    iTestStepName = KText169;
    }

CQoSTest_056::~CQoSTest_056()
    {
    }

TVerdict CQoSTest_056::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText170, "qos_056 RQoSPolicy, Additional modules needed by NIF");
    Log( KText170 );

    TInt err = CreateTestFile(KUseTestModule);
    if(err != KErrNone)
        {
        _LIT(KText171, "Creating testfile failed, aborting test");
        Log( KText171 );
        return EInconclusive;
        }

    TInetAddr dstAddr;
    TRequestStatus status;
    RSocketServ socketServer;
    socketServer.Connect();

    RSocket socket;
    socket.Open(socketServer, KAfInet, KSockDatagram, KProtocolInetUdp);
    _LIT(KText172, "10.1.56.1");
    dstAddr.Input( KText172 );
    dstAddr.SetPort(3561);
    socket.Connect(dstAddr, status);
    User::WaitForRequest(status);

    TEST(SendData(socket) == KErrNone); 

    CQoSParameters *parameters = new(ELeave) CQoSParameters();
    SetQoSParamSet1(*parameters);
    TQoSSelector selector;
    selector.SetAddr(socket);
    RQoSPolicy policy;
    TEST(policy.Open(selector) == KErrNone);
    TEST(policy.NotifyEvent(*this) == KErrNone);
    TEST(policy.SetQoS(*parameters) == KErrNone);
    WaitForQoSEventL();
    TESTE(iReceivedEvent == EQoSEventAddPolicy, iReceivedEvent);
    TESTE(iReason == KErrNone, iReason);

    WaitForQoSEventL();
    TESTE(iReceivedEvent == EQoSEventConfirm, iReceivedEvent);

    // Check that test module is loaded
    TEST(CheckTestFile(KTestModuleLoaded));

    TEST(SendData(socket) == KErrNone);

    TEST(policy.CancelNotifyEvent(*this) == KErrNone);

    TEST(policy.Close() == KErrNone);
    delete parameters;
    socket.Close();
    socketServer.Close();

    // To give testmodule time to unload after socket is closed, do a delay
    RTimer timer;
    timer.CreateLocal();
    timer.After(status, 25000000);
    User::WaitForRequest(status);
    timer.Close();

    // Check that test module is unloaded
    TEST(CheckTestFile(KTestModuleUnloaded));
    ClearTestFiles();

    return iTestStepResult;

    /* 
    
    Note:The unload delay value specified in the qos.ini determines 
    the testmodule's destruction. If the unload delay specified in 
    qos.ini is greater than the Rtimer wait value in the testcase 
    after closing the socket server then the KtestModuleUnloaded 
    file created by the destructor of test module will not be present. 
    So please adjust the timer value in case if the test case fails at 
    this point.    
    
    */
    }



CQoSTest_057::CQoSTest_057()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText173, "qos_057");
    iTestStepName = KText173;
    }

CQoSTest_057::~CQoSTest_057()
    {
    }

TVerdict CQoSTest_057::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText174, "qos_057 RQoSPolicy, Additional modules needed by NIF, \
modules not present");
    Log( KText174 );

    // create KUseTestModuleNonExist file so nif knows to load non-existing 
    // test module
    TInt err = CreateTestFile(KUseTestModuleNonExist);
    if(err != KErrNone)
        {
        _LIT(KText175, "Creating testfile failed, aborting test");
        Log( KText175 );
        return EInconclusive;
        }

    RSocketServ socketServer;
    RSocket socket;
    TInetAddr dstAddr;
    TRequestStatus status;
    socketServer.Connect();
    socket.Open(socketServer, KAfInet, KSockDatagram, KProtocolInetUdp);
    dstAddr.SetPort(3571);
    _LIT(KText176, "10.1.57.1");
    dstAddr.Input( KText176 );
    socket.Connect(dstAddr, status);
    User::WaitForRequest(status);

    TEST(SendData(socket) == KErrNone);

    TQoSSelector selector;
    selector.SetAddr(socket);
    RQoSPolicy policy;
    policy.Open(selector);
    policy.NotifyEvent(*this);
    CQoSParameters *parameters = new(ELeave) CQoSParameters();
    SetQoSParamSet1(*parameters);
    policy.SetQoS(*parameters);

    WaitForQoSEventL();
    TESTE(iReceivedEvent == EQoSEventAddPolicy, iReceivedEvent);
    TESTE(iReason == KErrNone, iReason);
    
    WaitForQoSEventL();
    TESTE(iReceivedEvent == EQoSEventFailure, iReceivedEvent);
    TESTE(iReason == EQoSNoModules, iReason);
    
    TEST(SendData(socket) == KErrNone);

    ClearTestFiles();

    TEST(policy.CancelNotifyEvent(*this) == KErrNone);
    policy.Close();
    socket.Close();
    socketServer.Close();
    delete parameters;

    return iTestStepResult;
    }



CQoSTest_058::CQoSTest_058()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText177, "qos_058");
    iTestStepName = KText177;
    }

CQoSTest_058::~CQoSTest_058()
    {
    }

TVerdict CQoSTest_058::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText178, "qos_058 RQoSPolicy, SetQoS, Attempt to re-negotiate, \
resulting CQoSFailure");
    Log( KText178 );

    RSocketServ socketServer;
    RSocket socket;
    TInetAddr dstAddr;
    TRequestStatus status;
    
    socketServer.Connect();
    socket.Open(socketServer, KAfInet, KSockDatagram, KProtocolInetUdp);
    dstAddr.SetPort(3581);
    _LIT(KText179, "10.1.58.1");
    dstAddr.Input( KText179 );
    socket.Connect(dstAddr, status);
    User::WaitForRequest(status);

    TEST(SendData(socket) == KErrNone);

    CQoSParameters *parameters = new(ELeave) CQoSParameters();
    SetQoSParamSet2(*parameters);
    parameters->SetAdaptMode(EFalse);
    TQoSSelector selector;
    selector.SetAddr(socket);
    RQoSPolicy policy;
    policy.Open(selector);
    policy.NotifyEvent(*this);

    TEST(policy.SetQoS(*parameters) == KErrNone);
    WaitForQoSEventL();
    TESTE(iReceivedEvent == EQoSEventAddPolicy, iReceivedEvent);
    TESTE(iReason == KErrNone, iReason);

    WaitForQoSEventL();
    TESTE(iReceivedEvent == EQoSEventConfirm, iReceivedEvent);

    SetQoSParamSet1(*parameters);
    TEST(policy.SetQoS(*parameters) == KErrNone);
    WaitForQoSEventL();
    TESTE(iReceivedEvent == EQoSEventAddPolicy, iReceivedEvent);
    TESTE(iReason == KErrNone, iReason);

    WaitForQoSEventL();
    TESTE(iReceivedEvent == EQoSEventConfirm, iReceivedEvent);

    TEST(SendData(socket) == KErrNone);

    TEST(policy.CancelNotifyEvent(*this) == KErrNone);
    TEST(policy.Close() == KErrNone);
    socket.Close();
    socketServer.Close();
    delete parameters;

    return iTestStepResult;
    }

CQoSTest_060::CQoSTest_060()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText183, "qos_060");
    iTestStepName = KText183;
    }

CQoSTest_060::~CQoSTest_060()
    {
    }

TVerdict CQoSTest_060::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText184, "qos_060 RQoSPolicy, Duplicate policies loaded");
    Log( KText184 );

    RSocketServ socketServer;
    RSocket socket;
    TInetAddr dstAddr;
    TRequestStatus status;
    
    socketServer.Connect();
    socket.Open(socketServer, KAfInet, KSockDatagram, KProtocolInetUdp);
    dstAddr.SetPort(3601);
    _LIT(KText185, "10.1.60.1");
    dstAddr.Input( KText185 );
    socket.Connect(dstAddr, status);
    User::WaitForRequest(status);

    TQoSSelector selector;
    selector.SetAddr(socket);
    
    _LIT(KPolicyFile, "c:\\qospolicieslegacy.ini");
    RQoSPolicy policy;
    policy.Open(selector);
    policy.NotifyEvent(*this);
    TEST(policy.LoadPolicyFile(KPolicyFile) == KErrNone);
    WaitForQoSEventL();
    if(iReason == KErrNotFound)
        {
        _LIT(KText186, "Policyfile qospolicieslegacy.ini not found!");
        Log(KText186, iReason);
        return EInconclusive;
        }
    TESTE(iReceivedEvent == EQoSEventLoadPolicyFile, iReceivedEvent);
    TESTE(iReason == KErrNone, iReason);
    
    TEST(policy.LoadPolicyFile(KPolicyFile) == KErrNone);
    WaitForQoSEventL();
    TESTE(iReceivedEvent == EQoSEventLoadPolicyFile, iReceivedEvent);
    TESTE(iReason == KErrAlreadyExists, iReason);

    TEST(policy.UnloadPolicyFile(KPolicyFile) == KErrNone);
    WaitForQoSEventL();
    TESTE(iReceivedEvent == EQoSEventUnloadPolicyFile, iReceivedEvent);
    TESTE(iReason == KErrNone, iReason);


    TEST(policy.CancelNotifyEvent(*this) == KErrNone);

    policy.Close();
    socket.Close();
    socketServer.Close();

    return iTestStepResult;
    }


CQoSTest_061::CQoSTest_061()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText187, "qos_061");
    iTestStepName = KText187;
    }

CQoSTest_061::~CQoSTest_061()
    {
    }

TVerdict CQoSTest_061::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText188, "qos_061 RQoSPolicy, Loading and unloading policies.");
    Log( KText188 );

    RSocketServ socketServer;
    RSocket socket;
    TInetAddr dstAddr;
    TRequestStatus status;
    
    socketServer.Connect();
    socket.Open(socketServer, KAfInet, KSockDatagram, KProtocolInetUdp);
    dstAddr.SetPort(3611);
    _LIT(KText189, "10.1.61.1");
    dstAddr.Input( KText189 );
    socket.Connect(dstAddr, status);
    User::WaitForRequest(status);

    TQoSSelector selector;
    selector.SetAddr(socket);
    
    _LIT(KPolicyFile, "c:\\qospolicieslegacy.ini");
    RQoSPolicy policy;
    policy.Open(selector);
    policy.NotifyEvent(*this);

    TEST(policy.LoadPolicyFile(KPolicyFile) == KErrNone);
    WaitForQoSEventL();
    if(iReason == KErrNotFound)
        {
        _LIT(KText190, "Policyfile qospolicieslegacy.ini not found!");
        Log( KText190, iReason);
        return EInconclusive;
        }

    TESTE(iReceivedEvent == EQoSEventLoadPolicyFile, iReceivedEvent);
    TESTE(iReason == KErrNone, iReason);
    
    TEST(policy.LoadPolicyFile(KPolicyFile) == KErrNone);
    WaitForQoSEventL();
    TESTE(iReceivedEvent == EQoSEventLoadPolicyFile, iReceivedEvent);
    TESTE(iReason == KErrAlreadyExists, iReason);

    TEST(policy.UnloadPolicyFile(KPolicyFile) == KErrNone);
    WaitForQoSEventL();
    TESTE(iReceivedEvent == EQoSEventUnloadPolicyFile, iReceivedEvent);
    TESTE(iReason == KErrNone, iReason);

    TEST(policy.LoadPolicyFile(KPolicyFile) == KErrNone);
    WaitForQoSEventL();
    TESTE(iReceivedEvent == EQoSEventLoadPolicyFile, iReceivedEvent);
    TESTE(iReason == KErrNone, iReason);

    TEST(policy.UnloadPolicyFile(KPolicyFile) == KErrNone);
    WaitForQoSEventL();
    TESTE(iReceivedEvent == EQoSEventUnloadPolicyFile, iReceivedEvent);
    TESTE(iReason == KErrNone, iReason);


    TEST(policy.CancelNotifyEvent(*this) == KErrNone);

    TEST(policy.Close() == KErrNone);
    socket.Close();
    socketServer.Close();

    return iTestStepResult;
    }



CQoSTest_062::CQoSTest_062()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText191, "qos_062");
    iTestStepName = KText191;
    }

CQoSTest_062::~CQoSTest_062()
    {
    }

TVerdict CQoSTest_062::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText192, "qos_062 RQoSPolicy, Unloading non-existent policies");
    Log( KText192 );

    RSocketServ socketServer;
    RSocket socket;
    TInetAddr dstAddr;
    TRequestStatus status;
    
    socketServer.Connect();
    socket.Open(socketServer, KAfInet, KSockDatagram, KProtocolInetUdp);
    dstAddr.SetPort(3621);
    _LIT(KText193, "10.1.62.1");
    dstAddr.Input( KText193 );
    socket.Connect(dstAddr, status);
    User::WaitForRequest(status);

    TQoSSelector selector;
    selector.SetAddr(socket);
    
    _LIT(KPolicyFile, "c:\\qospolicieslegacy.ini");
    RQoSPolicy policy;
    policy.Open(selector);
    policy.NotifyEvent(*this);
    TEST(policy.UnloadPolicyFile(KPolicyFile) == KErrNone);
    WaitForQoSEventL();
    TESTE(iReceivedEvent == EQoSEventUnloadPolicyFile, iReceivedEvent);
    TESTE(iReason == KErrNotFound, iReason);
    
    TEST(policy.CancelNotifyEvent(*this) == KErrNone);

    TEST(policy.Close() == KErrNone);
    socket.Close();
    socketServer.Close();

    return iTestStepResult;
    }



CQoSTest_064::CQoSTest_064()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText194, "qos_064");
    iTestStepName = KText194;
    }

CQoSTest_064::~CQoSTest_064()
    {
    }

TVerdict CQoSTest_064::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText195, "qos_064 RQoSPolicy, Happy day Scenario");
    Log( KText195 );

    TInetAddr dstAddr, srcAddr;
    
    _LIT(KText196, "10.1.64.1");
    dstAddr.Input( KText196 );
    dstAddr.SetPort(3641);
    _LIT(KText197, "10.1.64.2");
    srcAddr.Input( KText197 );
    srcAddr.SetPort(3642);
    
    TQoSSelector selector;
    selector.SetDst(dstAddr);
    selector.SetIapId(11);
    selector.SetMaxPortDst(3643);
    selector.SetMaxPortSrc(3644);
    selector.SetProtocol(15);
    selector.SetSrc(srcAddr); 

    CQoSParameters *parameters = new(ELeave) CQoSParameters();
    SetQoSParamSet2(*parameters);

    RQoSPolicy policy;
    policy.Open(selector);
    policy.NotifyEvent(*this);
    TEST(policy.SetQoS(*parameters) == KErrNone);
    WaitForQoSEventL();
    TESTE(iReceivedEvent == EQoSEventAddPolicy, iReceivedEvent);
    TESTE(iReason == KErrNone, iReason);

    TEST(policy.CancelNotifyEvent(*this) == KErrNone);
    
    SetQoSParamSet1(*parameters);
    TEST(policy.SetQoS(*parameters) == KErrNone);
    WaitForQoSEventL();
    // Check we did'n receive any event
    TESTE(iReceivedEvent == KErrTimedOut, iReceivedEvent);

    
    TEST(policy.Close() == KErrNone);
    delete parameters;

    return iTestStepResult;
    }




//
//  CUmtsQoSPolicy test cases
//

CQoSTest_070::CQoSTest_070()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText198, "qos_070");
    iTestStepName = KText198;
    }

CQoSTest_070::~CQoSTest_070()
    {
    }

TVerdict CQoSTest_070::doTestStepL( void )
    {
    User::After(KInterTestDelay / 9);
    _LIT(KText199, "qos_070 TUmtsQoSParameters, Constructor behavior");
    Log( KText199 );

    TUmtsQoSParameters parameters;
    TEST(parameters.TrafficClass() == ETrafficClassUnspecified);
    TEST(parameters.DeliveryOrder() == EDeliveryOrderUnspecified);
    TEST(parameters.DeliveryOfErroneusSdu() == 
         EErroneousSDUDeliveryUnspecified);
    TEST(parameters.ResidualBer() == EBERUnspecified);
    TEST(parameters.ErrorRatio() == ESDUErrorRatioUnspecified);
    TEST(parameters.Priority() == ETrafficPriorityUnspecified);
    TEST((TUint)parameters.TransferDelay() == KTransferDelayUnspecified);
    TEST((TUint)parameters.MaxSduSize() == KMaxSDUUnspecified);
    TEST((TUint)parameters.MaxBitrateUplink() == KMaxBitRateUnspecified);
    TEST((TUint)parameters.MaxBitrateDownlink() == KMaxBitRateUnspecified);
    TEST((TUint)parameters.GuaranteedBitrateUplink() == 
         KGrtdBitRateUnspecified);
    TEST((TUint)parameters.GuaranteedBitrateDownlink() == 
         KGrtdBitRateUnspecified);

    return iTestStepResult;
    }


CQoSTest_071::CQoSTest_071()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText200, "qos_071");
    iTestStepName = KText200;
    }

CQoSTest_071::~CQoSTest_071()
    {
    }

TVerdict CQoSTest_071::doTestStepL( void )
    {
    User::After(KInterTestDelay / 9);
    _LIT(KText201, "qos_071 TUmtsQoSParametes, Set() -functions, parameter \
transaction.");
    Log( KText201 );

    TUmtsQoSParameters parameters;
    
    TEST(parameters.SetTrafficClass(ETrafficClassBackground) == KErrNone);
    TEST(parameters.TrafficClass() == ETrafficClassBackground);

    TEST(parameters.SetDeliveryOrder(EDeliveryOrderNotRequired) == KErrNone);
    TEST(parameters.DeliveryOrder() == EDeliveryOrderNotRequired);

    TEST(parameters.SetDeliveryOfErroneusSdu(EErroneousSDUDeliveryNotRequired)
         == KErrNone);
    TEST(parameters.DeliveryOfErroneusSdu() == 
         EErroneousSDUDeliveryNotRequired);

    TEST(parameters.SetResidualBer(EBERSixPerHundredMillion) == KErrNone);
    TEST(parameters.ResidualBer() == EBERSixPerHundredMillion);

    TEST(parameters.SetErrorRatio(ESDUErrorRatioOnePerMillion) == KErrNone);
    TEST(parameters.ErrorRatio() == ESDUErrorRatioOnePerMillion);

    TEST(parameters.SetPriority(ETrafficPriority3) == KErrNone);
    TEST(parameters.Priority() == ETrafficPriority3);

    TEST(parameters.SetTransferDelay(KTransferDelayMax) == KErrNone);
    TEST((TUint)parameters.TransferDelay() == KTransferDelayMax);

    TEST(parameters.SetMaxSduSize(KMaxSDUMaximum) == KErrNone);
    TEST((TUint)parameters.MaxSduSize() == KMaxSDUMaximum);

    TEST(parameters.SetMaxBitrateUplink(KMaxBitRateMaximum) == KErrNone);
    TEST((TUint)parameters.MaxBitrateUplink() == KMaxBitRateMaximum);

    TEST(parameters.SetMaxBitrateDownlink(KMaxBitRateMaximum) == KErrNone);
    TEST((TUint)parameters.MaxBitrateDownlink() == KMaxBitRateMaximum);

    TEST(parameters.SetGuaranteedBitrateUplink(KGrtdBitRateMaximum) == 
         KErrNone);
    TEST((TUint)parameters.GuaranteedBitrateUplink() == KGrtdBitRateMaximum);

    TEST(parameters.SetGuaranteedBitrateDownlink(KGrtdBitRateMaximum) == 
         KErrNone);
    TEST((TUint)parameters.GuaranteedBitrateDownlink() == 
         KGrtdBitRateMaximum);

    return iTestStepResult;
    }


CQoSTest_077::CQoSTest_077()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText214, "qos_077");
    iTestStepName = KText214;
    }

CQoSTest_077::~CQoSTest_077()
    {
    }

TVerdict CQoSTest_077::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText215, "qos_077, CUmtsQoSPolicy, FindExtension()");
    Log( KText215 );

    TRequestStatus status;
    TInetAddr dstAddr;
    _LIT(KText216, "10.1.77.1");
    dstAddr.Input( KText216 );
    dstAddr.SetPort(3771);
    RSocketServ socketServer;
    RSocket socket;
    socketServer.Connect();
    socket.Open(socketServer, KAfInet, KSockDatagram, KProtocolInetUdp);
    socket.Connect(dstAddr, status);
    User::WaitForRequest(status);

    CQoSParameters *parameters = new(ELeave) CQoSParameters();
    SetQoSParamSet1(*parameters);
    
    TUmtsQoSParameters umtsReqParameters;
    SetUmtsQoSHighDemand(umtsReqParameters);

    CUmtsQoSPolicy *umtsPolicy = CUmtsQoSPolicy::NewL();
    umtsPolicy->SetQoSRequested(umtsReqParameters);

    // Check that no extension is found in this phase    
    TEST(parameters->FindExtension(KPfqosExtensionUmts) == NULL);

    // Add the extension...
    TEST(parameters->AddExtensionL(*umtsPolicy) == KErrNone);

    // ...Find the added extension...
    CUmtsQoSPolicy *policy = 
        ((CUmtsQoSPolicy*)parameters->FindExtension(KPfqosExtensionUmts));
    TEST(policy != NULL);

    // ...and check that this is same as extension that was added
    if(policy != NULL)
        {
        TUmtsQoSParameters param;
        policy->GetQoSRequested(param);
        TEST(param.DeliveryOfErroneusSdu() == 
             umtsReqParameters.DeliveryOfErroneusSdu());
        TEST(param.TrafficClass() == umtsReqParameters.TrafficClass());
        TEST(param.DeliveryOrder() == umtsReqParameters.DeliveryOrder());
        TEST(param.ResidualBer() == umtsReqParameters.ResidualBer());
        TEST(param.ErrorRatio() == umtsReqParameters.ErrorRatio());
        TEST(param.Priority() == umtsReqParameters.Priority());
        TEST(param.TransferDelay() == umtsReqParameters.TransferDelay());
        TEST(param.MaxSduSize() == umtsReqParameters.MaxSduSize());
        TEST(param.MaxBitrateUplink() == 
             umtsReqParameters.MaxBitrateUplink());
        TEST(param.MaxBitrateDownlink() == 
             umtsReqParameters.MaxBitrateDownlink());
        TEST(param.GuaranteedBitrateUplink() == 
             umtsReqParameters.GuaranteedBitrateUplink());
        TEST(param.GuaranteedBitrateDownlink() == 
             umtsReqParameters.GuaranteedBitrateDownlink());
        }

    socket.Close();
    socketServer.Close();
    delete parameters;
    delete umtsPolicy;

    return iTestStepResult;
    }


CQoSTest_601::CQoSTest_601()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText291, "qos_601");
    iTestStepName = KText291;
    }

CQoSTest_601::~CQoSTest_601()
    {
    }

TVerdict CQoSTest_601::doTestStepL( void )
    {
    //
    //  !!! NOTICE !!!
    //  This case has to be run separately from all of the other cases
    //
    _LIT(KText292, "qos_601, Data caging challenge");
    Log( KText292 );

    _LIT(KQoSInitial, "c:\\private\\101F7989\\esock\\ip.qos.esk");
    _LIT(KQoSForgery, "c:\\private\\101F7989\\esock\\ip.qos.esk_forged");

    RFs fs;
    fs.Connect();

    TInt err = fs.Rename(KQoSInitial, KQoSForgery);
    if(err != KErrNone)
        {
        _LIT(KText293, "Renaming file didn't succeed");
        Log( KText293 );
        return EPass;
        }
    _LIT(KText294, "Renaming file succeeded");
    Log( KText294 );
    err = fs.Rename(KQoSForgery, KQoSInitial);
    return EFail;
    }


CQoSTest_203::CQoSTest_203()
    {
    // store the name of this test case
    // this is the name that is used by the script file
    _LIT(KText48, "qos_203");
    iTestStepName = KText48;
    }

CQoSTest_203::~CQoSTest_203()
    {
    }

TVerdict CQoSTest_203::doTestStepL( void )
    {
    User::After(KInterTestDelay);
    _LIT(KText49, "qos_203 RSubConnection Testcase for recreating error scenerio");
    Log( KText49 );

    RSocketServ             socketServer;
    RSocket                 socket1;
    TInetAddr               dstAddr1;
    RConnection             conn; 
    RSubConnection          subconn1; 
    TRequestStatus          status,socketStatus1, subconStatus1;
    TNotificationEventBuf   subconnNotifBuf;
    TInt                    ret; 

    
    dstAddr1.SetPort(3203);
    _LIT(KText50, "10.1.20.3");
    dstAddr1.Input( KText50 );
    
    ret = socketServer.Connect();
    CleanupClosePushL(socketServer); 
    TESTL(ret == KErrNone); 

    ret = conn.Open(socketServer, KAfInet); 
    TESTL(ret == KErrNone); 

    conn.Start(status); 
    User::WaitForRequest(status); 
    TESTL(status.Int() == KErrNone);


    ret = subconn1.Open(socketServer, RSubConnection::ECreateNew, conn); 
    TESTL(ret == KErrNone);

    ret = socket1.Open(socketServer, KAfInet, KSockDatagram, KProtocolInetUdp, conn);
    TESTL(ret == KErrNone); 
    Log(_L("socket1 opened"));
    
    ret = socket1.SetLocalPort(3204);
    TESTL(ret == KErrNone); 

    
    subconn1.Add(socket1, subconStatus1);
    Log(_L("subconn1 Add() called"));
    
    socket1.Connect(dstAddr1, socketStatus1);
    Log(_L("socket1 Connect() called"));
    Log(_L("waiting for socket1 to get the connect status"));
    User::WaitForRequest(socketStatus1);
    Log(_L("socket1 connect status returned"));
    TESTL(socketStatus1.Int() == KErrNone); 
    
    
    Log(_L("waiting for subconn1 to get the Add status"));
    User::WaitForRequest(subconStatus1);
    Log(_L("subcon1 Add status returned"));
    TESTL(subconStatus1.Int() == KErrNone);
    
    
    
    subconn1.Close();
    socket1.Close();
    conn.Stop();
    conn.Close();
    
    CleanupStack::Pop(&socketServer); 
    socketServer.Close();

    return iTestStepResult;
    }

