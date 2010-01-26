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
// IPSecPolicyManSession.h - IPSec Policy Manager Session
//



/**
 @internalComponent
*/
#ifndef __IPSECPOLICYMANSESSION__
#define __IPSECPOLICYMANSESSION__

#include <e32base.h>
#include <e32std.h>

const TInt KIPSecPolicyManActive = 1;
//
//
//
class CIPSecPolicyManagerServer;
class CIPSecPolicyManagerSession:public CSession2
    {
    public:
        enum 
            { 
            EIPSecPolicyLoadPolicy,
                EIPSecPolicyUnloadPolicy,
                EIPSecPolicyCancel,
                EIPSecPolicyGetIPSecSAInfo
            };
        static CIPSecPolicyManagerSession* NewL(CIPSecPolicyManagerServer* aServer);
        ~CIPSecPolicyManagerSession(void);
        void ServiceL(const RMessage2& aMessage);
        
    private:
        CIPSecPolicyManagerSession(CIPSecPolicyManagerServer* aServer );
        CIPSecPolicyManagerServer* iServer;    
    };
#endif
