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

#include <ecom/implementationproxy.h>
#include "IPSCPRFactory.h"
#include "IPSCPR.h"
#include "ipdeftbasescpr.h"
#include <comms-infras/ss_log.h>

#include <comms-infras/ss_msgintercept.h>

#ifdef __CFLOG_ACTIVE
#define KIpSCprFactoryTag KESockSubConnectionTag
// _LIT8(KIpSCprFactorySubTag, "ipscprfactory");
#endif // __CFLOG_ACTIVE

using namespace ESock;

//-=========================================================
//
// ECOM data
//
//-=========================================================	
const TImplementationProxy ImplementationTable[] =
	{
	IMPLEMENTATION_PROXY_ENTRY(CIpDefaultSubConnectionProviderFactory::iUid, CIpDefaultSubConnectionProviderFactory::NewL)
	};

/**
ECOM Implementation Factory
*/
EXPORT_C const TImplementationProxy* ImplementationGroupProxy(TInt& aTableCount)
    {
    aTableCount = sizeof(ImplementationTable) / sizeof(TImplementationProxy);
    return ImplementationTable;
   }


//-=========================================================
//
// CIpDefaultSubConnectionProviderFactory methods
//
//-=========================================================	
CIpDefaultSubConnectionProviderFactory* CIpDefaultSubConnectionProviderFactory::NewL(TAny* aParentContainer)
    {
    return new (ELeave) CIpDefaultSubConnectionProviderFactory(TUid::Uid(CIpDefaultSubConnectionProviderFactory::iUid), 
                                            *reinterpret_cast<ESock::CSubConnectionFactoryContainer*>(aParentContainer));
    }
    
CIpDefaultSubConnectionProviderFactory::CIpDefaultSubConnectionProviderFactory(TUid aFactoryId, ESock::CSubConnectionFactoryContainer& aParentContainer)
	: CSubConnectionProviderFactoryBase(aFactoryId, aParentContainer)
    {
    }

ACommsFactoryNodeId* CIpDefaultSubConnectionProviderFactory::DoCreateObjectL(ESock::TFactoryQueryBase& aQuery)
    {
	const TDefaultSCPRFactoryQuery& query = static_cast<const TDefaultSCPRFactoryQuery&>(aQuery);
    CSubConnectionProviderBase* provider = NULL;
    if (query.iSCPRType == RSubConnection::ECreateNew)
        {
        provider = CIpSubConnectionProvider::NewL(*this);
		ESOCK_DEBUG_REGISTER_GENERAL_NODE(iUid, provider);
        }
    else if (query.iSCPRType == RSubConnection::EAttachToDefault)
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


