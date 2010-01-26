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
// IPProto MCPR Factory
// 
//

/**
 @file
 @internalComponent
*/

#include <comms-infras/ss_log.h>
#include <ss_glob.h>
#include "IPProtoMCprFactory.h"
#include "IPProtoMCpr.h"

#include <comms-infras/ss_msgintercept.h>

#ifdef __CFLOG_ACTIVE
#define KIPProtoMCprFactoryTag KESockMetaConnectionTag
_LIT8(KIPProtoMCprFactorySubTag, "ipprotomcprfact");
#endif // __CFLOG_ACTIVE

using namespace ESock;

//-=========================================================
//
// CIPProtoMetaConnectionProviderFactory implementation
//
//-=========================================================
CIPProtoMetaConnectionProviderFactory* CIPProtoMetaConnectionProviderFactory::NewL(TAny* aParentContainer)
	{
	__CFLOG_VAR((KIPProtoMCprFactoryTag, KIPProtoMCprFactorySubTag, _L8("CIPProtoMetaConnectionProviderFactory ::\tNewL(%08x)"), aParentContainer));
 	return new (ELeave) CIPProtoMetaConnectionProviderFactory(TUid::Uid(CIPProtoMetaConnectionProviderFactory::iUid), *(reinterpret_cast<CMetaConnectionFactoryContainer*>(aParentContainer)));
	}

CIPProtoMetaConnectionProviderFactory::CIPProtoMetaConnectionProviderFactory(TUid aFactoryUid, CMetaConnectionFactoryContainer& aParentContainer)
	: CMetaConnectionProviderFactoryBase(aFactoryUid,aParentContainer)
	{
	}

ESock::ACommsFactoryNodeId* CIPProtoMetaConnectionProviderFactory::DoCreateObjectL(ESock::TFactoryQueryBase& aQuery)
	{
	const TMetaConnectionFactoryQuery& query = static_cast<const TMetaConnectionFactoryQuery&>(aQuery);
	CMetaConnectionProviderBase* provider = CIPProtoMetaConnectionProvider::NewL(*this,query.iProviderInfo);

	ESOCK_DEBUG_REGISTER_GENERAL_NODE(iUid, provider);

	return provider;
	}
