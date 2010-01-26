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

// EPOC includes
#include <e32base.h>
#include <in_sock.h>

// Test system includes
#include "LlmnrTestQueries.h"
#include <testexecutelog.h>

#include "in6_opt.h"



// Test step 1.1
CTestStepLLMNR_Queries::CTestStepLLMNR_Queries(CLlmnrTestServer *apLlmnrTestServer /*= NULL*/)
                       :CTestStepLLMNR(apLlmnrTestServer) 
    {
    // Call base class method to set up the human readable name for INFO_PRINTF1ging
    SetTestStepName(KTestStepLLMNR_Report);
    }

CTestStepLLMNR_Queries::~CTestStepLLMNR_Queries()
    {
    }


//---------------------------------------------------------------------------------------------------------

TVerdict CTestStepLLMNR_Queries::doTestStepL( void )
    {
    
    SetTestStepResult(EPass);
    
    TESTL(ipTestServer->iNetworkInfo.Initialized()); //-- LLMNR should have been initialized
    
    //-- Test LLMNR query mechanism. Queries: multicast A(for IPv4) AAAA (for IPv6) and unicast PTR.
    //-- actually, it has been already teststed above, because at present only A, AAAA, and PTR 
    //-- queries supported.
    
    INFO_PRINTF1(_L("Testing LLMNR dns queries..."));
    
    for(TInt cntNode=0; cntNode< ipTestServer->iNetworkInfo.NodesCount(); ++cntNode)
        {
        TNodeInfo& nodeInfo = ipTestServer->iNetworkInfo[cntNode];
        if(nodeInfo.iAlive)
            {
            if(nodeInfo.iAddr.Family() == KAfInet)
                TestDNSqueriesIP4L(nodeInfo);//-- IPv4 Address
            else
                if(nodeInfo.iAddr.Family() == KAfInet6)
                    TestDNSqueriesIP6L(nodeInfo);//-- IPv6 Address
                else 
                    TESTL(EFalse);
            }
        
        } //for(cntHost...
    
    INFO_PRINTF1(KNewLine);
    
    SetTestStepResult(EPass);
    return TestStepResult();
    }


/**
*   Sending multicast A and unicast PTR queries to the specified host (IPv4 assumed)
*/
void CTestStepLLMNR_Queries::TestDNSqueriesIP4L(const TNodeInfo& aNodeInfo)
    {
    TDnsQueryBufEx  dnsQryBuf;
    TDnsRespABuf    dnsRespABuf;
    TNameEntry      nameEntry;
    
    TInt            nRes;
    TName           tmpBuf;
    TName           tmpBuf1;
    
    TESTL(aNodeInfo.iAddr.Family() == KAfInet);
    
    tmpBuf.Copy(_L("Sending multicast A query to resolve the hostname: "));
    tmpBuf.Append(aNodeInfo.iHostName);
    INFO_PRINTF1(tmpBuf);
    
    dnsQryBuf().SetType(KDnsRRTypeA);
    dnsQryBuf().SetData(aNodeInfo.iHostName);
    
    nRes = ipTestServer->iHostResolver.Query(dnsQryBuf, dnsRespABuf);  //-- make a query and get the result
    TESTL( nRes==KErrNone ); //-- the host is supposed to be alive...
    
    dnsRespABuf().HostAddress().Output(tmpBuf1);                 
    
    tmpBuf.Copy(_L("Query result: "));
    tmpBuf.Append(tmpBuf1);
    INFO_PRINTF1(tmpBuf);
    
    //-- check whether obtained IP address matches the one we obtained before.
    TESTL(dnsRespABuf().HostAddress().Match(aNodeInfo.iAddr));
    
    INFO_PRINTF1(KNewLine);
    
    //-- use HostResolver functionality to send PTR query.
    tmpBuf.Copy(_L("Sending unicast PTR query to resolve the address: "));
    tmpBuf.Append(tmpBuf1);
    INFO_PRINTF1(tmpBuf);
    
    nRes = ipTestServer->iHostResolver.GetByAddress(dnsRespABuf().HostAddress(), nameEntry);
    TESTL( nRes==KErrNone ); //-- the host is supposed to be alive...
    
    tmpBuf.Copy(_L("Query result: "));
    tmpBuf.Append(nameEntry().iName);
    INFO_PRINTF1(tmpBuf);
    INFO_PRINTF1(KNewLine);
    
    }

/**
*   Sending multicast AAAA and unicast PTR queries to the specified host (IPv6 assumed)
*/
void CTestStepLLMNR_Queries::TestDNSqueriesIP6L(const TNodeInfo& aNodeInfo)
    {
    TDnsQueryBufEx  dnsQryBuf;
    TDnsRespAAAABuf dnsRespAAAABuf;
    TNameEntry      nameEntry;
    
    TInt            nRes;
    TName           tmpBuf;
    TName           tmpBuf1;
    
    TESTL(aNodeInfo.iAddr.Family() == KAfInet6);
    
    tmpBuf.Copy(_L("Sending multicast AAAA query to resolve the hostname: "));
    tmpBuf.Append(aNodeInfo.iHostName);
    INFO_PRINTF1(tmpBuf);
    
    dnsQryBuf().SetType(KDnsRRTypeAAAA);
    dnsQryBuf().SetData(aNodeInfo.iHostName);
    
    nRes = ipTestServer->iHostResolver.Query(dnsQryBuf, dnsRespAAAABuf);  //-- make a query and get the result
    TESTL( nRes==KErrNone ); //-- the host is supposed to be alive...
    
    dnsRespAAAABuf().HostAddress().Output(tmpBuf1);                 
    
    tmpBuf.Copy(_L("Query result: "));
    tmpBuf.Append(tmpBuf1);
    INFO_PRINTF1(tmpBuf);
    
    //-- check whether obtained IP address matches the one we obtained before.
    TESTL(dnsRespAAAABuf().HostAddress().Match(aNodeInfo.iAddr));
    
    INFO_PRINTF1(KNewLine);
    
    //-- use HostResolver functionality to send PTR query.
    tmpBuf.Copy(_L("Sending unicast PTR query to resolve the address: "));
    tmpBuf.Append(tmpBuf1);
    INFO_PRINTF1(tmpBuf);
    
    nRes = ipTestServer->iHostResolver.GetByAddress(dnsRespAAAABuf().HostAddress(), nameEntry);
    TESTL( nRes==KErrNone ); //-- the host is supposed to be alive...
    
    tmpBuf.Copy(_L("Query result: "));
    tmpBuf.Append(nameEntry().iName);
    INFO_PRINTF1(tmpBuf);
    INFO_PRINTF1(KNewLine);
    
    }





