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
// secpolreader.h - key management daemon security policy module
//



/**
 @internalComponent
*/
#ifndef _SECPOLREADER_H_
#define _SECPOLREADER_H_

#include <es_sock.h>
#include <in_sock.h>
#include "ipsecpolmanhandler.h"

//class CListenerControl;
class CSecpolReader : public CActive
    {
    public:
        CSecpolReader(CIPSecPolicyManagerHandler* control);
        TInt Construct(const TDesC &aSocket);
        ~CSecpolReader() ;
    private:
        // active object stuff, completion and cancel callback functions
        void RunL();
        void DoCancel();
        TInt RunError(TInt aError);     
    private:
        // API call instance
        RSocket iSocket;
        TBuf8<1000> iMsg;
        TSockAddr iAddr;
        CIPSecPolicyManagerHandler *iControl;
    };
#endif
