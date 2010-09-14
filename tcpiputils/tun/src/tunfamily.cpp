// Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "tunfamily.h"
#include "tun.h"
#include <in_chk.h>
#include <es_prot.h>


CProtocolFamilyTun::CProtocolFamilyTun()
    {
    __DECLARE_NAME(_S("CProtocolFamilyTun"));
    }

CProtocolFamilyTun::~CProtocolFamilyTun()
//Destructor
    {}

TInt CProtocolFamilyTun::Install()
    {
    return KErrNone;
    }

TInt CProtocolFamilyTun::Remove()
    {
    return KErrNone;
    }

TUint CProtocolFamilyTun::ProtocolList(TServerProtocolDesc* &aProtocolList)
    {
    const TInt KArraySize = 1;
    // Esock catches this leave; hence TRAP is not needed
    TServerProtocolDesc *p = new (ELeave) TServerProtocolDesc[KArraySize]; 
    CProtocolTun::Identify(p[KArraySize-1]);
    aProtocolList = p;
    return 1;
    }

CProtocolBase* CProtocolFamilyTun::NewProtocolL(TUint /*aSockType*/,
                                                   TUint aProtocol)
    {
    if (aProtocol != KProtocolTUN)
        {
        Panic(ETunPanic_BadBind);
        }

    CProtocolTun* instance = CProtocolTun::NewL(); 
    return instance;

    }



//
// Entrypoint
//
#ifndef EKA2
GLDEF_C TInt E32Dll()
    {
    return KErrNone;
    }
#endif //#ifndef EKA2

// Force export of non-mangled name
extern "C" { IMPORT_C CProtocolFamilyBase* Install(void); }
EXPORT_C CProtocolFamilyBase* Install(void)
    {
    CProtocolFamilyTun* protocol = new CProtocolFamilyTun();
    if (protocol)
        {
        return protocol;
        }
    else 
        {
        return NULL;
        }
    }

