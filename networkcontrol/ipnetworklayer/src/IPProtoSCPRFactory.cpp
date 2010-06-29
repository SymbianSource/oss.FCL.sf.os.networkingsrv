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
// ipprotodeftscprfactory.cpp
// IPProto SubConnection Provider factory class definition.
// 
//

/**
 @file
 @internalComponent
*/

#include "IPProtoMessages.h"
#include "IPProtoSCPRFactory.h"
#include "ipprotodeftscpr.h"
#include "IPProtoSCPR.h"
#include <comms-infras/ss_msgintercept.h>

using namespace ESock;

//-=========================================================
//
// CIPProtoSubConnectionProviderFactory methods
//
//-=========================================================	
CIPProtoSubConnectionProviderFactory* CIPProtoSubConnectionProviderFactory::NewL(TAny* aParentContainer)
    {
    return new (ELeave) CIPProtoSubConnectionProviderFactory(TUid::Uid(CIPProtoSubConnectionProviderFactory::iUid), 
                                            *reinterpret_cast<ESock::CSubConnectionFactoryContainer*>(aParentContainer));
    }
    
CIPProtoSubConnectionProviderFactory::CIPProtoSubConnectionProviderFactory(TUid aFactoryId, ESock::CSubConnectionFactoryContainer& aParentContainer)
	: CSubConnectionProviderFactoryBase(aFactoryId, aParentContainer)
    {  
    }

CIPProtoSubConnectionProviderFactory::~CIPProtoSubConnectionProviderFactory()
    {
    }

ACommsFactoryNodeId* CIPProtoSubConnectionProviderFactory::DoCreateObjectL(TFactoryQueryBase& aQuery)
    {
    const TDefaultSCPRFactoryQuery& query = static_cast<const TDefaultSCPRFactoryQuery&>(aQuery);
    CSubConnectionProviderBase* provider = NULL;
    if (query.iSCPRType == RSubConnection::EAttachToDefault)
        {
        provider = CIPProtoDeftSubConnectionProvider::NewL(*this);
		ESOCK_DEBUG_REGISTER_GENERAL_NODE(iUid, provider);
        }
    else if (query.iSCPRType == RSubConnection::ECreateNew)
        {
        provider = CIPProtoSubConnectionProvider::NewL(*this);
		ESOCK_DEBUG_REGISTER_GENERAL_NODE(iUid, provider);
        }
    else
        {
        User::Leave(KErrNotSupported);
        }
    return provider;
    }


