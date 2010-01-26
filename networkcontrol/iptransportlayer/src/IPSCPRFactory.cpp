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

#include "IPSCPRFactory.h"
#include "ipdeftscpr.h"
#include "ipscpr.h"
#include "IPMessages.h"

#include <comms-infras/ss_log.h>

#include <comms-infras/ss_msgintercept.h>

#ifdef __CFLOG_ACTIVE
	#define KIpSCprFactoryTag KESockSubConnectionTag
	// _LIT8(KIpSCprFactorySubTag, "ipscprfactory");
#endif // __CFLOG_ACTIVE

using namespace ESock;

//-=========================================================
//
// CIpSubConnectionProviderFactory methods
//
//-=========================================================	
EXPORT_C CIpSubConnectionProviderFactory* CIpSubConnectionProviderFactory::NewL(TAny* aParentContainer)
    {
	TCFIPMessage::RegisterL();

    return new (ELeave) CIpSubConnectionProviderFactory(TUid::Uid(CIpSubConnectionProviderFactory::iUid), 
                                            *reinterpret_cast<ESock::CSubConnectionFactoryContainer*>(aParentContainer));
    }
CIpSubConnectionProviderFactory::CIpSubConnectionProviderFactory(TUid aFactoryId, ESock::CSubConnectionFactoryContainer& aParentContainer)
	: CSubConnectionProviderFactoryBase(aFactoryId, aParentContainer)
    {
    }

ACommsFactoryNodeId* CIpSubConnectionProviderFactory::DoCreateObjectL(ESock::TFactoryQueryBase& aQuery)
    {
    const TDefaultSCPRFactoryQuery& query = static_cast<const TDefaultSCPRFactoryQuery&>(aQuery);
    CSubConnectionProviderBase* provider(NULL);
    
    if (query.iSCPRType == RSubConnection::ECreateNew)
        {
        provider = CIpSubConnectionProvider::NewL(*this);
		ESOCK_DEBUG_REGISTER_GENERAL_NODE(iUid, provider);
        }
    else if (query.iSCPRType == RSubConnection::EAttachToDefault)
        {
        provider = CIpDefaultSubConnectionProvider::NewL(*this);	
		ESOCK_DEBUG_REGISTER_GENERAL_NODE(iUid, provider);
        }
    else
        {
        User::Leave(KErrNotSupported);
        }

    return provider;
    }

CIpSubConnectionProviderFactory::~CIpSubConnectionProviderFactory()
    {
	TCFIPMessage::DeRegister();
    }
