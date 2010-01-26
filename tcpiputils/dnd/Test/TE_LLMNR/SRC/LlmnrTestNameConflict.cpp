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
//


#include "LlmnrTestNameConflict.h"
#include <testexecutelog.h>

_LIT(KNameConflictSection, "NameConflictTest"); //-- ini file section

CTestStepLLMNR_NameConflict::CTestStepLLMNR_NameConflict(CLlmnrTestServer *apLlmnrTestServer /*= NULL*/)
                            :CTestStepLLMNR(apLlmnrTestServer) 
    {
    // Call base class method to set up the human readable name for INFO_PRINTF1ging
    SetTestStepName(KTestStepLLMNR_Report);
    }

CTestStepLLMNR_NameConflict::~CTestStepLLMNR_NameConflict()
    {
    }

//---------------------------------------------------------------------------------------------------------

TVerdict CTestStepLLMNR_NameConflict::doTestStepL()
    {
    
    TInt    nRes;
    TName   tmpBuf;
    
    INFO_PRINTF1(_L("Testing Link-Local hosts name conflict..."));
    
    TESTL(ipTestServer->iNetworkInfo.Initialized()); //-- LLMNR should have been initialized
    
    TNetworkInfo& networkInfo = ipTestServer->iNetworkInfo;
    
    //-- find the node which will be the server
    INFO_PRINTF1(_L("Choosing server among peers..."));
    nRes = FindServerIPaddrIndex(networkInfo);
    TESTL(nRes >= 0);
    
    TBool bServer=EFalse;
    //-- check, if the server address is our local
    if(networkInfo[nRes].iLocal)
        {
        INFO_PRINTF1(_L("I am the server! I am the server! :)"));
        bServer=ETrue;
        }
    else
        {    
        INFO_PRINTF1(_L("I am the client!"));
        INFO_PRINTF1(_L("external server chosen."));
        }
    
    networkInfo[nRes].iAddr.Output(tmpBuf);
    INFO_PRINTF1(tmpBuf);
    
    if(bServer)
        ProceedServerL(nRes); //-- this node has become server 
    else
        ProceedClientL(nRes); //-- this node has become client
    
    SetTestStepResult(EPass);
    return TestStepResult();
    }

//---------------------------------------------------------------------------------------------------------

/**
*   Act as server: wait for clients connections and exchange information with them 
*
*   @param  aSrvIndex index in iNetworkInfo array corresponding to server's IP address
*/
void    CTestStepLLMNR_NameConflict::ProceedServerL(TInt aSrvIndex)
    {
    TName   tmpBuf;
    TName   tmpBuf1;
    TInt    nRes;
    TInt    cntNode;
    
    TESTL(aSrvIndex >=0 );
    
    //-- get port number
    TESTL(GetIntFromConfig(KNameConflictSection, _L("Port"), nRes) && nRes >0);
    const TInt chatPort = nRes;
    
    //-- get connection timeout value
    TESTL(GetIntFromConfig(KNameConflictSection, _L("ConnectTimeout"), nRes) && nRes > 0);
    const TInt connTimeOut= nRes*KOneSecond;
    
    //-- list IP addresses of potential clients and count their number
    TNetworkInfo& networkInfo = ipTestServer->iNetworkInfo;
    
    TInt nExpectedClients = 0;
    TInt nConnectedClients = 0;
    
    INFO_PRINTF2(_L("Listening on port %d, awaiting connections from:"), chatPort);
    for(cntNode=0; cntNode < networkInfo.NodesCount(); ++cntNode)
        {
        
        TNodeInfo& currNode = networkInfo[cntNode]; //-- current node info
        currNode.iClientConnected = EFalse; //-- reset connected client flag
        
        //-- expect connction from alive, non-local host
        if(currNode.iAlive && !currNode.iLocal && currNode.iIpUnique)
            {
            nExpectedClients++;
            currNode.iAddr.Output(tmpBuf);
            INFO_PRINTF1(tmpBuf);
            }
        }
    
    if(nExpectedClients < 1)
        {
        INFO_PRINTF1(_L("Number of expected clients less than 1!"));
        TESTL(EFalse);
        }
    
    //-- wait for clients connections
    
    TRequestStatus  rqStat, rqStatTimeout;
    RSocket         sockListener;
    TInetAddr       inetAddr;
    TInetAddr       inetCliAddr;
    RTimer          tmrTimeOut;
    
    CSrvConnInfo*    pConnInfo= new(ELeave)CSrvConnInfo; //-- contains information about connected clients
    CleanupStack::PushL(pConnInfo);
    CleanupClosePushL(sockListener);
    
    
    //-- set up a timeout timer
    TESTL(tmrTimeOut.CreateLocal() == KErrNone);
    tmrTimeOut.After(rqStatTimeout, connTimeOut);
    
    //-- open and set up listener socket
    nRes = sockListener.Open(ipTestServer->iSocketServer, KAfInet, KSockStream, KProtocolInetTcp);
    TESTL(nRes == KErrNone);
    
    inetAddr.SetPort(chatPort);
    
    nRes =sockListener.Bind(inetAddr);
    TESTL(nRes == KErrNone);
    
    nRes =sockListener.Listen(nExpectedClients);
    TESTL(nRes == KErrNone);
    
    //-- listen for incoming connections and accept them
    for(nConnectedClients=0; nConnectedClients < nExpectedClients ;)
        {
        rqStat=KRequestPending;
        
        //-- allocate a socket to communicate with client
        TInt index = pConnInfo->AllocSlot();
        TESTL(index>=0);
        
        RSocket&    cliSock = pConnInfo->Socket(index);
        
        nRes = cliSock.Open(ipTestServer->iSocketServer);
        TESTL(nRes == KErrNone);
        
        sockListener.Accept(cliSock, rqStat);
        
        //-- wait for incoming connection with timeout
        User::WaitForRequest(rqStat, rqStatTimeout);
        
        //-- check timeout
        if(rqStatTimeout.Int() != KRequestPending)
            {//--  connection timeout 
            sockListener.CancelAll();
            cliSock.CancelAll();
            INFO_PRINTF1(_L("Client connection timeout!"));
            break;
            }
        
        TESTL(rqStat.Int() == KErrNone);
        
        //-- client has connected
        
        //-- print out client's IP address
        cliSock.RemoteName(inetCliAddr);
        inetCliAddr.Output(tmpBuf1);
        
        tmpBuf.Copy(_L("client "));
        tmpBuf.Append(tmpBuf1);
        tmpBuf.Append(_L(" has connected."));
        INFO_PRINTF1(tmpBuf);
        
        //-- check whether it is a known client
        TInt nIndex = networkInfo.FindIP(inetCliAddr);
        if(nIndex < 0)
            {//-- connection from client that is not listed in networkInfo 
            //-- strange and impossible situation 
            INFO_PRINTF1(_L("??? Unknown client!"));   
            TESTL(EFalse);
            }
        
        //-- mark client as "Connected" 
        if(networkInfo[nIndex].iClientConnected)
            {//-- strange situation, secondary connect from the same client
            INFO_PRINTF1(_L("??? This client is already connected !"));   
            INFO_PRINTF1(_L("Check your test configuration. You probably have 2 nodes with the same MAC."));   
            
            TESTL(EFalse);
            }
        
        networkInfo[nIndex].iClientConnected = ETrue;
        nConnectedClients ++;
        
        }// for
    
    tmrTimeOut.Cancel();
    User::WaitForRequest(rqStatTimeout);
    
    //-- check number of connected clients
    if(nConnectedClients < nExpectedClients)
        {
        INFO_PRINTF3(_L("warning: Expected %d connections, %d clients have connected"), nExpectedClients, nConnectedClients);
        if(nConnectedClients < 1)
            {//-- need to have at least 1 client connected
            INFO_PRINTF1(_L("No one has connected, nothing to do, leaving...."));
            TESTL(EFalse);
            }
        }
    
    //-- so, here we have connInfo with array of sockets connected to the clients
    //-- start communication with the clients.
    //-- 1. Send data to the client (the client should check this data)
    //-- 2. wait for client response
    //-- 3. check the data from the client
    
    TDataExchBuf srvData;
    TDataExchBuf clientData;
    
    //-- calculate and store hash of the node information.
    //-- if there is no name conflict, each node shall have absolutely same nodes table and its hash
    nRes = networkInfo.NodesDataHash(srvData().iData);
    TESTL(nRes == KErrNone);
    
    //-- send data to every connected client and receive its response
    for(cntNode=0; cntNode < nConnectedClients; cntNode++)
        {
        RSocket& sock = pConnInfo->Socket(cntNode); //-- socket to communicate with the particular client
        
        tmpBuf.Copy(_L("Sending data to the client "));
        sock.RemoteName(inetCliAddr);
        inetCliAddr.Output(tmpBuf1);
        tmpBuf.Append(tmpBuf1);
        INFO_PRINTF1(tmpBuf);
        
        //-- send data to the client
        nRes = SendData(sock, srvData);
        TESTL(nRes == KErrNone);
        
        //-- wait and receive data from the client
        INFO_PRINTF1(_L("Waiting for data from the client..."));
        
        nRes = RecvData(sock, clientData, connTimeOut);
        TESTL(nRes == KErrNone);
        
        INFO_PRINTF1(_L("Checking client's response..."));
        if(clientData == srvData)
            {
            INFO_PRINTF1(_L("Ok, no name conflict detected"));
            }
        else
            {
            INFO_PRINTF1(_L("client's data differ from server's!"));
            INFO_PRINTF1(_L("It means LLMNR name conflict. Test failed."));
            TESTL(EFalse);
            }
        
        }// for
    
    User::After(4*KOneSecond);
    //-- disconnect and cancel everything
    CleanupStack::PopAndDestroy(2); //sockListener, pConnInfo
    
    INFO_PRINTF1(KNewLine);
}


//---------------------------------------------------------------------------------------------------------

/**
*   Act as client: try to connect to the server and then exchange information.
*
*   @param  aSrvIndex index in iNetworkInfo array corresponding to server's IP address
*/
void    CTestStepLLMNR_NameConflict::ProceedClientL(TInt aSrvIndex)
    {
    TName   tmpBuf;
    TName   tmpBuf1;
    TInt    nRes;
    
    TESTL(aSrvIndex >=0 );
    
    //-- port to communicate with server
    
    TESTL(GetIntFromConfig(KNameConflictSection, _L("Port"), nRes) && nRes > 0);
    const TInt chatPort = nRes;
    
    //-- max delay between 2 connect attempts
    TESTL(GetIntFromConfig(KNameConflictSection, _L("ConnectMaxDelay"), nRes) && nRes >=0 );
    const TInt connDelay = nRes*KOneSecond;
    
    //-- max number of connection attempts
    TESTL(GetIntFromConfig(KNameConflictSection, _L("ConnectAttempts"), nRes) && nRes >=1);
    const TInt connMaxAttempts = nRes;
    
    //-- get connection timeout value
    TESTL(GetIntFromConfig(KNameConflictSection, _L("ConnectTimeout"), nRes) && nRes > 0);
    const TInt connTimeOut= nRes*KOneSecond;
    
    
    TNetworkInfo& networkInfo = ipTestServer->iNetworkInfo;
    
    TInetAddr& serverIP = networkInfo[aSrvIndex].iAddr; //-- server's IP address
    serverIP.SetPort(chatPort);
    
    serverIP.Output(tmpBuf1);
    tmpBuf.Copy(_L("Connecting to "));
    tmpBuf.Append(tmpBuf1);
    tmpBuf.AppendFormat(_L(" on port %d:"), chatPort);
    INFO_PRINTF1(tmpBuf);
    
    //-- try to connect to the server
    TInt cntConn;
    TRequestStatus  rqStat(KRequestPending);
    RSocket         sock;
    
    CleanupClosePushL(sock);
    
    //-- make a number of connection attempts with random delays between
    for(cntConn=0; cntConn < connMaxAttempts; ++cntConn)
        {
        INFO_PRINTF2(_L("attempt %d"), cntConn+1);
        
        sock.Close();
        rqStat=KRequestPending;
        
        nRes = sock.Open(ipTestServer->iSocketServer, KAfInet, KSockStream, KProtocolInetTcp);
        TESTL(nRes == KErrNone);
        
        sock.Connect(serverIP, rqStat);
        User::WaitForRequest(rqStat);
        
        if(rqStat.Int() == KErrNone)
            break; //-- connected to the server.
        
        //-- short random delay between connection attempts
        User::After(ipTestServer->RandomizeNum(connDelay));
        }
    
    if(rqStat.Int() != KErrNone)
        {//-- couldn't connect to the server
        sock.CancelAll();
        sock.Close();
        INFO_PRINTF1(_L("could not connect to the server! leaving..."));
        TESTL(EFalse);
        }
    
    //-- here we are, connected to the server
    INFO_PRINTF1(_L("connected!"));
    
    //--------------------------------------------------------
    //-- 1. receive data from the server
    //-- 2. check the data 
    //-- 3. send client's data to the server
    //--------------------------------------------------------
    
    TDataExchBuf srvData;
    TDataExchBuf clientData;
    
    //-- calculate hash of the node information.
    //-- if there is no name conflict, server's data shall be the same as clients'
    nRes = networkInfo.NodesDataHash(clientData().iData);
    TESTL(nRes == KErrNone);
    
    INFO_PRINTF1(_L("Waiting for data from the server..."));
    
    nRes = RecvData(sock, srvData, connTimeOut);
    
    TESTL(nRes == KErrNone);
    
    INFO_PRINTF1(_L("Data from server received!"));
    INFO_PRINTF1(_L("Sending response..."));
    
    User::After(KOneSecond);
    nRes = SendData(sock, clientData);
    TESTL(nRes == KErrNone);
    
    INFO_PRINTF1(_L("Data sent to the server OK"));
    INFO_PRINTF1(_L("Checking server's data..."));
    
    
    if(clientData == srvData)
        {
        INFO_PRINTF1(_L("Ok, no name conflict detected"));
        }
    else
        {
        INFO_PRINTF1(_L("client's data differ from server's!"));
        INFO_PRINTF1(_L("It means LLMNR name conflict. Test failed."));
        TESTL(EFalse);
        }
    
    CleanupStack::PopAndDestroy(1); //sock
    
    INFO_PRINTF1(KNewLine);
    
}

/**
*   Send data via connected socket.  
*
*   @param    aSocket connected socket
*   @param    aData   package containing data to send
*   @return   standard error code
*/
TInt    CTestStepLLMNR_NameConflict::SendData(RSocket& aSocket, const TDataExchBuf& aData)
    {
    TRequestStatus  rqStat(KRequestPending);
    
    aSocket.Send(aData,0,rqStat);
    User::WaitForRequest(rqStat);
    
    TInt nRes = rqStat.Int();
    
    if(nRes != KErrNone)
        INFO_PRINTF2(_L("CTestStepLLMNR_NameConflict::SendData finished with error, code: %d"), nRes);
    
    return nRes;
    }


/**
*   Receive data via connected socket.
*
*   @param  aSocket  connected socket
*   @param  aData    package for received data
*   @param  aTimeout timeout in microseconds. if negative or zero, will wait for data forever - no timeout
*
*   @return standard error code.
*/
TInt    CTestStepLLMNR_NameConflict::RecvData(RSocket& aSocket, TDataExchBuf& aData, TInt aTimeout/*=0*/)
    {
    TRequestStatus  rqStat(KRequestPending);
    TRequestStatus  rqStatTimeout(KRequestPending);
    TSockXfrLength  xfrLen;
    RTimer          tmrTimeOut;
    TInt            nRes=KErrGeneral;
    
    
    if(aTimeout > 0)
        {//-- use receiving data with timeout
        
        //-- set up a timeout timer
        nRes = tmrTimeOut.CreateLocal();
        if(nRes != KErrNone)
            return nRes;
        
        
        tmrTimeOut.After(rqStatTimeout, aTimeout);
        
        //-- receive data from socket
        aSocket.RecvOneOrMore(aData, 0, rqStat, xfrLen);
        User::WaitForRequest(rqStat, rqStatTimeout);
        
        if(rqStat == KRequestPending)
            {//-- timeout.
            aSocket.CancelRecv();
            User::WaitForRequest(rqStat);
            nRes = KErrTimedOut;
            }
        else
            {//-- it's OK, cancel timer
            nRes = rqStat.Int();
            
            if(rqStatTimeout.Int() == KRequestPending)
                {
                tmrTimeOut.Cancel();
                User::WaitForRequest(rqStatTimeout);
                }
            }
        
        tmrTimeOut.Close();
        
        }
    else
        {//-- receiving without timeout, infinite wait
        aSocket.RecvOneOrMore(aData, 0, rqStat, xfrLen);
        User::WaitForRequest(rqStat);
        nRes = rqStat.Int();
        }
    
    if(nRes != KErrNone)
        INFO_PRINTF2(_L("CTestStepLLMNR_NameConflict::RecvData finished with error, code: %d"), nRes);
    
    
    return nRes;
    }

//---------------------------------------------------------------------------------------------------------

/**
*   Choose IP address for the server within HostInfo array among peers' IPs.
*   The server usually is chosen by minimal string representation of its IP address.
*   So that everyone will choose the same host.
*   
*   @param  aNetworkInfo    reference to the array of hosts TNetworkInfo 
*   @return Index in this array corresponding to the server's IP or negative value as error indication.
*/
TInt   CTestStepLLMNR_NameConflict::FindServerIPaddrIndex(const TNetworkInfo& aNetworkInfo)
    {
    TName   minBuf;
    TName   currBuf;
    TInt    indexMin;
    TInt    i;
    
    if(aNetworkInfo.NodesCount() < KMinNodes)
        return KErrNotFound;
    
    indexMin=0;
    
    //-- find the first alive node
    for(i=1; i<aNetworkInfo.NodesCount(); ++i)
        {
        if(aNetworkInfo[i].iAlive)
            {
            indexMin=i;
            break;
            }
        }
    
    if(i == aNetworkInfo.NodesCount())
        return KErrNotFound; //-- didn't find anyone
    
    //-- we will compare character representation of the IP addresses, not binary
    aNetworkInfo[indexMin].iAddr.Output(minBuf);
    
    //-- find the minimal string
    for(i=0; i<aNetworkInfo.NodesCount(); ++i)
        {
        if( !aNetworkInfo[i].iAlive )
            continue; //-- skip dead node
        
        aNetworkInfo[i].iAddr.Output(currBuf);  
        
        #if 1
        if(currBuf.Compare(minBuf) < 0) //-- a node with minimal IP will be the server
        #else
        if(currBuf.Compare(minBuf) > 0) //-- a node with maximal IP will be the server
        #endif
                {
                indexMin=i;
                aNetworkInfo[indexMin].iAddr.Output(minBuf);
                }
        }
    
    
    return indexMin;        
    }


//---------------------------------------------------------------------------------------------------------

CSrvConnInfo::CSrvConnInfo()
    {
    }

CSrvConnInfo::~CSrvConnInfo()
    {
    Reset();
    }
/**
*   Reset data and free resources
*/
void CSrvConnInfo::Reset()
    {
    //-- close all sockets and empty the array
    for(TInt i=0; i<ipSockets.Count(); ++i)
        {
        if(ipSockets[i])
            {
            ipSockets[i]->Close();
            delete ipSockets[i];
            }
        }
    
    ipSockets.Close();
    }

/**
*   Allocate slot for another client connection.
*   Actually just creates a new socket for talking to the client.
*
*   @return index, which then can be used with Socket() for accessing created slot data. Value < 0 means error.
*/
TInt    CSrvConnInfo::AllocSlot()
    {
    TInt nIndex = ipSockets.Count();                         
    
    if(ipSockets.Append(new RSocket) != KErrNone)
        return KErrNotFound;
    
    
    return nIndex;
    }





