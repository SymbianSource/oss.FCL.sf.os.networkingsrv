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

#include "LlmnrInit.h"
#include "in6_opt.h"


_LIT(KNodesSection, "LLMNR_Init"); //-- ini file section


CTestStepLLMNR_Init::CTestStepLLMNR_Init(CLlmnrTestServer *apLlmnrTestServer /*= NULL*/)
                    :CTestStepLLMNR(apLlmnrTestServer)
    {
    // Call base class method to set up the human readable name for INFO_PRINTF1ging
    SetTestStepName(KTestStepLLMNR_Report);
    }

CTestStepLLMNR_Init::~CTestStepLLMNR_Init()
    {
    }

//-- compares 2 IP addresses, for array search.
TBool IPAddrIsEqual(const TInetAddr& aFirst, const TInetAddr& aSecond)
    {
    return aFirst.Match(aSecond);
    }

//---------------------------------------------------------------------------------------------------------

TVerdict CTestStepLLMNR_Init::doTestStepL()
    {
    
    TInt        cntNode, nRes;
    TName       tmpBuf;
    TName       tmpBuf1;
    
    SetTestStepResult(EPass);
    
    TESTL(GetIntFromConfig(KNodesSection, _L("StartUpDelay"), nRes) && nRes > 0);
    const TInt startUpDelay = nRes*KOneSecond;  
    
    INFO_PRINTF1(KNewLine);
    INFO_PRINTF1(_L("Initializing LLMNR test engine..."));
    
    //-- wait some time for settling name conflict resolution within LLMNR mechanism 
    User::After(ipTestServer->RandomizeNum(startUpDelay, startUpDelay));
    
    RIPAddrArray&   localAddrs  = ipTestServer->iLocalAddrs;
    TNetworkInfo&   networkInfo = ipTestServer->iNetworkInfo;
    
    //-- reset network information and local IP addresses table
    networkInfo.Reset();
    localAddrs.Reset();
    
    //--------------------------------------------
    
    //-- load network configuration from ini file
    LoadNetworkConfigFromIniL(networkInfo);
    
    //-- start up network layer and LLMNR engine
    TESTL(StartUpLLMNR());
    
    //-- wait some time for settling name conflict resolution within LLMNR mechanism
    User::After(ipTestServer->RandomizeNum(startUpDelay));
    
    //-- list available network interfaces and their IP addresses
    ListInterfacesL();
    
    //-- try to resolve every host name and obtain its IP address
    ProbeNodes(networkInfo);
    
    //-----------------------------------------------------------
    
    //-- print out alive hosts names, IP addresses, etc and check the number of hosts
    INFO_PRINTF1(_L("--- available hosts list:\n"));
    
    
    TInt foreignNodeCnt = 0; //-- number of foreign hosts (IP addresses that not ours)
    TInt localNodeCnt = 0;   //-- number of local IP addresses
    
    for(cntNode=0; cntNode < networkInfo.NodesCount(); ++cntNode)
        {
        TNodeInfo& currNode = networkInfo[cntNode]; //-- current node info
        
        if(currNode.iAlive)
            {
            
            //-- print host name and its IP address
            tmpBuf.Copy(currNode.iHostName);
            tmpBuf.Append(_L(" "));
            
            currNode.iAddr.Output(tmpBuf1);
            tmpBuf.Append(tmpBuf1);
            
            //-- search IP address in the array of local host addresses.
            //-- if found, mark the appropriate hostInfo element as having local address.
            currNode.iLocal = (ipTestServer->iLocalAddrs.Find(currNode.iAddr, TIdentityRelation<TInetAddr>(IPAddrIsEqual)) != KErrNotFound);
            
            if(currNode.iLocal)
                {   
                tmpBuf.Append(_L(" [local address]"));
                localNodeCnt++;
                }
            
            if(currNode.iIpUnique)
                tmpBuf.Append(_L(" [U]"));
            
            if(! currNode.iLocal && currNode.iIpUnique)
                foreignNodeCnt++;
            
            INFO_PRINTF1(tmpBuf);
            }
        }
    
    INFO_PRINTF1(KNewLine);
    
    //-- check the number of nodes with local IP addresses to ensure that test config are correct
    if(localNodeCnt <1) 
        {
        INFO_PRINTF1(_L("??? Error! No nodes found with local IP. Check the test configuration.\n"));
        SetTestStepResult(EInconclusive);
        return TestStepResult();
        }
    
    //-- we need at least 1 other external host
    if(foreignNodeCnt < 1)
        {
        INFO_PRINTF1(_L("Error! For successfull testing you should have at least 1 external hosts available!\n"));
        SetTestStepResult(EInconclusive);
        return TestStepResult();
        }
    
    //-- check if all IP adresses are link-local
    INFO_PRINTF1(_L("check all hosts' IP adresses to be link-local"));
    for(cntNode=0; cntNode< networkInfo.NodesCount(); ++cntNode)
        {
        TNodeInfo& currNode = networkInfo[cntNode]; //-- current node info
        
        if(currNode.iAlive)
            if(! currNode.iAddr.IsLinkLocal())
                {
                tmpBuf.Copy(_L("host "));
                tmpBuf.Append(currNode.iHostName);
                tmpBuf.Append(_L(" "));
                currNode.iAddr.Output(tmpBuf1);
                tmpBuf.Append(tmpBuf1);
                tmpBuf.Copy(_L(" IP address isn't link local! "));
                
                INFO_PRINTF1(tmpBuf);
                SetTestStepResult(EInconclusive);
                return TestStepResult();
                }
        }
    
    //-----------------------------------------------------------
    //-- LLMNR has been initialized and information about network collected
    networkInfo.SetInitialized(ETrue); 
    
    SetTestStepResult(EPass);
    return TestStepResult();
}



/**
*   Load network iformation (hosts names) from ini file.
*
*   @param aNetworkInfo ref. to the TNetworkInfo structure, which will be populated
*/
void   CTestStepLLMNR_Init::LoadNetworkConfigFromIniL(TNetworkInfo&  aNetworkInfo)
    {
    TName   tmpBuf;
    TInt    nRes;
    
    //-- get number of hosts which are supposed to be running
    //-- there should be at last 2 host names in the list
    TESTL(GetIntFromConfig(KNodesSection, _L("NumNodes"), nRes) && nRes >= KMinNodes);
    const TInt numNodes = nRes;  
    
    //-- get maximal number of trials of host hame resolution
    TESTL(GetIntFromConfig(KNodesSection, _L("ConnectTrials"), nRes) && nRes >= 1);
    const TInt maxTrials = nRes;  
    
    //-- populate NetworkInfo array with information from ini file.
    TNodeInfo nodeInfo;
    
    for(TInt i=0; i<numNodes; ++i)
        {
        //-- get host name from ini file
        tmpBuf.Format(_L("HostName%d"),i+1);
        if(!GetIniFileString(KNodesSection, tmpBuf, nodeInfo.iHostName))  
            continue;
        
        //-- put host info to the array
        nodeInfo.iCntTrials = maxTrials;
        aNetworkInfo.AppendNode(nodeInfo);
        }
    
    //-- check number of hosts to communicate with.
    TESTL(aNetworkInfo.NodesCount() >= KMinNodes);
    }

/**
*   Tries to resolve each host name from the table loaded from ini file.
*   @param aNetworkInfo ref. to the TNetworkInfo structure, which will be populated
*/
void CTestStepLLMNR_Init::ProbeNodes(TNetworkInfo&  aNetworkInfo)
    {
    TName       tmpBuf;
    TInt        nRes;
    TNameEntry  nameEntry;
    
    //-- start name resolution for every entry in HostInfo
    GetIntFromConfig(KNodesSection, _L("ScanMaxDelay"), nRes);
    const TInt scanMaxDelay = nRes*KOneSecond;
    
    for(;;)
        {
        TInt cntFinishedHosts = 0;
        
        for(TInt cntNode=0; cntNode< aNetworkInfo.NodesCount(); ++cntNode)
            {
            
            TNodeInfo& currNode = aNetworkInfo[cntNode]; //-- current node info
            
            if( ! currNode.iAlive && currNode.iCntTrials >0) 
                {//-- the host is worth asking its IP address
                
                //-- a small random delay between sending queries
                User::After(ipTestServer->RandomizeNum(scanMaxDelay));
                
                INFO_PRINTF1(KNewLine);
                tmpBuf.Copy(_L("- Probing host: "));
                tmpBuf.Append(currNode.iHostName);
                INFO_PRINTF1(tmpBuf);
                
                nRes = ipTestServer->iHostResolver.GetByName(currNode.iHostName, nameEntry);
                
                if(nRes == KErrNone)
                    {//-- current host name has been resolver successfully, mark it alive and store its IP address
                    
                    //-- check if the obtained IP address is unique in TNetworkInfo
                    currNode.iIpUnique = (aNetworkInfo.FindIP(nameEntry().iAddr) < 0);
                    
                    currNode.iAddr = nameEntry().iAddr;   
                    currNode.iAlive = ETrue;
                    
                    currNode.iAddr.Output(nameEntry().iName);
                    tmpBuf.Copy(_L("name resolved: "));
                    tmpBuf.Append(nameEntry().iName);
                    INFO_PRINTF1(tmpBuf);
                    }
                else 
                    {//-- no response    
                    currNode.iCntTrials --; //-- decrease trials counter for this hostname
                    INFO_PRINTF2(_L("name resolution error: %d"), nRes);
                    }
                }
            else
                cntFinishedHosts++;
            
            }
        
        //-- check, whether we need to continue nodes scanning
        //-- if either host is alive or has trials counted down to 0, exit
        if(cntFinishedHosts == aNetworkInfo.NodesCount())
            break;
        } //for(;;)
    
    INFO_PRINTF1(KNewLine);
    
    }

/**
*   Start up the LLMNR.
*   @return ETrue on success.
*/
TBool   CTestStepLLMNR_Init::StartUpLLMNR()
    {
    TInt        nRes;
    TNameEntry  nameEntry;
    
    
    const  TInt KMaxTrials = 3; //-- maximal number of trials
    
    //-- magic hostname, shall always be in tcp.ini file
    //-- we will try to resolve this name to start LLMNR engine
    _LIT(KMagicHostName, "startup.host.ru");
    
    INFO_PRINTF1(_L("Starting the LLMNR engine..."));
    
    for(TInt cntTrials=0; cntTrials < KMaxTrials ;++cntTrials)
        {
        nRes = ipTestServer->iHostResolver.GetByName(KMagicHostName, nameEntry);
        if(nRes == KErrNone)
            return ETrue;
        }
    
    INFO_PRINTF1(_L("Unable to start !"));
    
    return EFalse;
    }


/**
*   List existing network interfaces, IP addresses bound to them 
*   and fill up array of IP addresses for all interfaces.
*/
void CTestStepLLMNR_Init::ListInterfacesL()
    {
    
    RSocket socket;
    TInt    nRes;
    TInt    exceed;
    TInt    idx;
    TName   tmpBuf;
    TName   tmpBuf1;
    
    nRes = socket.Open(ipTestServer->iSocketServer, KAfInet, KSockStream, KProtocolInetTcp);
    TESTL(nRes == KErrNone);
    
    TUint   bufsize = 2048;
    
    HBufC8 *buffer =NULL;
    buffer = GetBuffer(buffer, bufsize);
    TESTL(buffer != NULL);
    
    TPtr8 bufdes = buffer->Des();
    
    //-- reset array of local addresses
    ipTestServer->iLocalAddrs.Reset();
    
    //-- list all available network interfaces
    INFO_PRINTF1(KNewLine);
    INFO_PRINTF1(_L("--- available network interfaces:"));
    
    do
        {//-- get list of network interfaces
        // if exceed>0, all interface could not fit into the buffer.
        // In that case allocate a bigger buffer and retry.
        // There should be no reason for this while loop to take more than 2 rounds.
        bufdes.Set(buffer->Des());
        exceed = socket.GetOpt(KSoInetInterfaceInfo, KSolInetIfQuery, bufdes);
        if(exceed > 0)
            {
            bufsize += exceed * sizeof(TInetInterfaceInfo);
            buffer = GetBuffer(buffer, bufsize);
            TESTL(buffer != NULL);
            }
        } while (exceed > 0);
        
        if (exceed < 0)
            {
            INFO_PRINTF1(_L("socket.GetOpt() error!"));
            TESTL(EFalse);
            }
        
        TOverlayArray<TInetInterfaceInfo> infoIface(bufdes);
        
        for(idx=0; idx < infoIface.Length(); ++idx)
            {
            TInetInterfaceInfo& iface = infoIface[idx];
            
            tmpBuf.Format(_L("index:%d, name: "),iface.iIndex);
            tmpBuf.Append(iface.iName );
            tmpBuf.AppendFormat(_L(" state:%d"), iface.iState);
            INFO_PRINTF1(tmpBuf);
            }
        
        //-- list all IP addresses, bound to the interfaces
        //-- and append this address to the array of host-local addresses
        INFO_PRINTF1(KNewLine);
        INFO_PRINTF1(_L("--- IP addresses bound to the interfaces:"));
        
        do
            {
            // if exceed>0, all interface could not fit into the buffer.
            // In that case allocate a bigger buffer and retry.
            // There should be no reason for this while loop to take more than 2 rounds.
	        bufdes.Set(buffer->Des());
            exceed = socket.GetOpt(KSoInetAddressInfo, KSolInetIfQuery, bufdes);
            if(exceed > 0)
                {
                bufsize += exceed * sizeof(TInetAddressInfo);
                buffer = GetBuffer(buffer, bufsize);
                }
            } while (exceed > 0);
            
            if (exceed < 0)
                {
                INFO_PRINTF1(_L("socket.GetOpt() error!"));
                TESTL(EFalse);
                }
            
            
            //-- print out IP addresses
            TOverlayArray<TInetAddressInfo> infoAddr(bufdes);
            TInetAddr inetAddr;
            
            for(idx=0; idx < infoAddr.Length(); ++idx)
                {
                TInetAddressInfo& addr = infoAddr[idx];
                
                tmpBuf.Format(_L("iface index: %d, scopeID: %d, state: %d, IP addr: "), addr.iInterface, addr.iScopeId, addr.iState);
                inetAddr.SetAddress(addr.iAddress);
                inetAddr.Output(tmpBuf1);
                tmpBuf.Append(tmpBuf1);
                INFO_PRINTF1(tmpBuf);
                
                //-- if obtained IP address is valid and not loopback, add it to the array.
                if(inetAddr.IsLoopback() || inetAddr.IsUnspecified())
                    {}
                else
                    ipTestServer->iLocalAddrs.Append(inetAddr);
                
                }
            
            
            delete buffer;
            socket.Close();
            
            INFO_PRINTF1(KNewLine);
}


/**
* Allocate buffer on heap or reallocate it if required size is more than existing
*
* @param    apBuf           pointer to the existing buffer. If NULL new bufeer will be allocated.
* @param    aBufLenRequired reuired buffer length.
* @return   pointer to the new buffer   
* @return   NULL in case of memory problems or other errors
*/
HBufC8* CTestStepLLMNR_Init::GetBuffer(HBufC8* apBuf, TInt aBufLenRequired)
    {
    HBufC8* pBufResult = NULL;
    
    if(aBufLenRequired <= 0 ) 
        return NULL; //-- invalid argument
    
    if(!apBuf)
        {//-- the buffer is not allocated at all, try allocate it
        pBufResult = HBufC8::New(aBufLenRequired);
        }
    else
        {//-- the buffer is already allocated. Check its size and try to reallocate if
        //-- required size is bigger than existing. Otherwise do nothing.   
        
        if(apBuf->Des().MaxSize() >= aBufLenRequired)
            pBufResult = apBuf; //-- do nothing, existing buffer is big enough
        else
            {  //-- delete and allocate new buffer to avoid data copying
            delete apBuf;
            pBufResult = HBufC8::New(aBufLenRequired);
            }
        
        }
    
    return pBufResult;
    }




