// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// IP SubConnection Provider factory class definition.
// 
//

/**
 @file
 @internalComponent
*/

#include "ipdeftscprfactory.h"
#include "ipdeftbasescpr.h"
#include "IPMessages.h"
#include <comms-infras/ss_log.h>

#include <comms-infras/ss_msgintercept.h>

#ifdef __CFLOG_ACTIVE
	#define KIpDeftSCprFactoryTag KESockSubConnectionTag
	// _LIT8(KIpDeftSCprFactorySubTag, "ipdeftscprfactory");
#endif // __CFLOG_ACTIVE

using namespace ESock;

//-=========================================================
//
// CIpDefaultSubConnectionProviderFactory methods
//
//-=========================================================	
EXPORT_C CIpDefaultSubConnectionProviderFactory* CIpDefaultSubConnectionProviderFactory::NewL(TAny* aParentContainer)
    {
	TCFIPMessage::RegisterL();

    return new (ELeave) CIpDefaultSubConnectionProviderFactory(TUid::Uid(CIpDefaultSubConnectionProviderFactory::iUid), 
                                            *reinterpret_cast<ESock::CSubConnectionFactoryContainer*>(aParentContainer));
    }
    
CIpDefaultSubConnectionProviderFactory::CIpDefaultSubConnectionProviderFactory(TUid aFactoryId, ESock::CSubConnectionFactoryContainer& aParentContainer)
	: CSubConnectionProviderFactoryBase(aFactoryId, aParentContainer)
    {
    }

ESock::ACommsFactoryNodeId* CIpDefaultSubConnectionProviderFactory::DoCreateObjectL(ESock::TFactoryQueryBase& aQuery)
    {
    const TDefaultSCPRFactoryQuery& query = static_cast<TDefaultSCPRFactoryQuery&>(aQuery);
    CSubConnectionProviderBase* provider = NULL;
    if (query.iSCPRType == RSubConnection::EAttachToDefault)
        {
        provider = CIpDefaultBaseSubConnectionProvider::NewL(*this);	
	    ESOCK_DEBUG_REGISTER_GENERAL_NODE(iUid, provider);
        }
    else
        {
        User::Leave(KErrNotSupported);
        }
    return provider;
    }

CIpDefaultSubConnectionProviderFactory::~CIpDefaultSubConnectionProviderFactory()
	{
	TCFIPMessage::DeRegister();
	}
