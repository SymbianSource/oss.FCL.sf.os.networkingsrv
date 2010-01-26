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
// IPSecPolicyManServer.h - IPSec Policy Manager Server main module
//



/**
 @internalComponent
*/
#ifndef __IPSECPOLICYMANSERVER_H__
#define __IPSECPOLICYMANSERVER_H__
#include <e32base.h>
#include "ipsecpolmansession.h"


class CIPSecPolicyManagerHandler;


//
//
//
class CIPSecPolicyManagerSession;
class CIPSecPolicyManagerHandler;

class CIPSecPolicyManagerServer:public CPolicyServer

    {
    public:
        static CIPSecPolicyManagerServer* NewL(void);
        static CIPSecPolicyManagerServer* NewLC(void);
        ~CIPSecPolicyManagerServer(void);
        CSession2* NewSessionL(const TVersion& aVersion, const RMessage2& ) const;
        TInt RunError(TInt aError);
    private:
        CIPSecPolicyManagerServer(void);
    private:
        
        
        static const TUint ipsecRangeCount;
        static const CPolicyServer::TPolicyElement ipsecElements[];
        static const TInt ipsecRanges[];
        static const TUint8 elementIndex[];
        static const CPolicyServer::TPolicy ipsecPolicy;
        
        
    public:     
        void StopIPSecPolicyManagerServer(void);
        TInt            iSessionCount;
        CIPSecPolicyManagerHandler* iIPSecPolicyManagerHandler;
        
        
        CIPSecPolicyManagerSession* testing;
    };
#endif
