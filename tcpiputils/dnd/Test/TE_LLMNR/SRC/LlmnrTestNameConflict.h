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

#ifndef     __LLMNRTESTNAMECONFLICT_H__
#define     __LLMNRTESTNAMECONFLICT_H__

#include "TestStepLLMNR.h"

#include <hash.h>

/**
*   Auxilary class that allocates and stores server's information about 
*   connected clients. 'Server' means the node which has been chosen to be the server :)
*/
class CSrvConnInfo : public CBase
    {
    public:
        CSrvConnInfo();
        ~CSrvConnInfo();
        
        void Reset();
        
        /** get a reference to the allocated socket by index. @see AllocSlot*/
        RSocket&    Socket(TInt aIndex) {return *ipSockets[aIndex];}
        TInt        AllocSlot();
        
    protected:
        RPointerArray<RSocket>  ipSockets;
    };


/**
*   Auxilary class that incapsulates data sent between server and clients
*/
class TDataExch
    {
    public:
        
        TBuf8<MD5_HASH> iData; //< client and server exchange MD5 hash data
        
        /** comparison operator */
        TBool operator == (const TDataExch& aRhs)
            { return( iData.Compare(aRhs.iData) == 0);}
    };

typedef TPckgBuf<TDataExch> TDataExchBuf;


/**
*   LLMNR name conflict test step representation
*/
class CTestStepLLMNR_NameConflict: public CTestStepLLMNR
    {
    public:
        CTestStepLLMNR_NameConflict(CLlmnrTestServer *apLlmnrTestServer = NULL);
        ~CTestStepLLMNR_NameConflict();
        
        virtual TVerdict doTestStepL();	
        
    private:       
        
        TInt    FindServerIPaddrIndex(const TNetworkInfo& aNetworkInfo); 
        
        void    ProceedServerL(TInt aSrvIndex);
        void    ProceedClientL(TInt aSrvIndex);
        
        TInt    SendData(RSocket& aSocket, const TDataExchBuf& aData);
        TInt    RecvData(RSocket& aSocket, TDataExchBuf& aData, TInt aTimeout=0);
    };

_LIT(KTestStepLLMNR_NameConflict,"TestStepLLMNR_NameConflict");



#endif  //__LLMNRTESTNAMECONFLICT_H__





