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


#ifndef __LLMNR_TEST_QUERIES_H__
#define __LLMNR_TEST_QUERIES_H__

#include "TestStepLLMNR.h"
#include "dns_qry.h"

/**
*   Multicast DNS queries test
*/
class CTestStepLLMNR_Queries : public CTestStepLLMNR
    {
    public:
        CTestStepLLMNR_Queries(CLlmnrTestServer *apLlmnrTestServer = NULL);
        ~CTestStepLLMNR_Queries();
        
        virtual TVerdict doTestStepL( void );
        
    private:
        
        void    TestDNSqueriesIP4L(const TNodeInfo& aNodeInfo);
        void    TestDNSqueriesIP6L(const TNodeInfo& aNodeInfo);
    };


/**
*   A slight extention in order to make using this class more convenient
*   This makes TDnsQuery accept unicode string parameters.
*/
class TDnsQueryEx : public TDnsQuery
    {
    public: 
        void SetData(const TDesC& aData) {iQryData.Copy(aData);}
    };

typedef TPckgBuf<TDnsQueryEx> TDnsQueryBufEx;


_LIT(KTestStepLLMNR_Queries,"TestStepLLMNR_Queries");

#endif //__LLMNR_TEST_QUERIES_H__





