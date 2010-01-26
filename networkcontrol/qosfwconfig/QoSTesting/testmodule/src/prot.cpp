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
 
#include "testmodule.h"


class CExtensionModule : public CProtocolFamilyBase
    {
    public:
    CExtensionModule();
    virtual ~CExtensionModule();
    virtual TInt Install();
    virtual CProtocolBase* NewProtocolL(TUint aSockType,TUint aProtocol);
    virtual TUint ProtocolList(TServerProtocolDesc*& aProtocolList);
    };

extern "C" { IMPORT_C CProtocolFamilyBase *Install(); }
EXPORT_C CProtocolFamilyBase *Install()
    {
    return new CExtensionModule;
    }

CExtensionModule::CExtensionModule()
    {
    __DECLARE_NAME(_S("CExtensionModule"));
    }

CExtensionModule::~CExtensionModule()
    {
    }

TInt CExtensionModule::Install()
    {
    return KErrNone;
    }

TUint CExtensionModule::ProtocolList(TServerProtocolDesc*& aProtocolList)
    {
    TServerProtocolDesc *p = NULL;
    TRAPD(err, p = new (ELeave) TServerProtocolDesc[1]);
    if (err != KErrNone)
        {
        LOG(Log::Printf(_L("TServerProtocolDesc error: %d"), err));
        }
    CTestModule::Identify(p[0]);
    aProtocolList = p;
    return 1;
    }

CProtocolBase* CExtensionModule::NewProtocolL(TUint aSockType,TUint aProtocol)
    {
    CModuleBase *inprt = NULL;
    if (aProtocol == KTestModule && aSockType == KSockDatagram)
	inprt = CTestModule::NewL();
    else
	User::Leave(KErrNotSupported);
    return inprt;
    }
