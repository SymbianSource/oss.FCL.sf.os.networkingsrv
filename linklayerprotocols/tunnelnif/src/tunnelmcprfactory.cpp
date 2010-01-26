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
// Tunnel MCPR Factory
// 
//

/**
 @file
 @internalComponent
*/

#include "tunnelmcprfactory.h"
#include "tunnelmcpr.h"
#include <comms-infras/ss_log.h>
#include <ss_glob.h>

#include <comms-infras/ss_msgintercept.h>

#ifdef __CFLOG_ACTIVE
	#define KTunnelMCprFactoryTag KESockMetaConnectionTag
	//_LIT8(KTunnelMCprFactorySubTag, "tunnelmcprfactory");
#endif // __CFLOG_ACTIVE

using namespace ESock;

//-=========================================================
//
// CTunnelMetaConnectionProviderFactory methods
//
//-=========================================================	
CTunnelMetaConnectionProviderFactory* CTunnelMetaConnectionProviderFactory::NewL(TAny* aParentContainer)
	{
	__CFLOG_VAR((KTunnelMCprFactoryTag, KTunnelMCprFactorySubTag, _L8("CTunnelMetaConnectionProviderFactory ::\tNewL(%08x)"), aParentContainer));
 	return new (ELeave) CTunnelMetaConnectionProviderFactory(TUid::Uid(CTunnelMetaConnectionProviderFactory::iUid), *(reinterpret_cast<CMetaConnectionFactoryContainer*>(aParentContainer)));
	}

CTunnelMetaConnectionProviderFactory::CTunnelMetaConnectionProviderFactory(TUid aFactoryId, CMetaConnectionFactoryContainer& aParentContainer)
	: CMetaConnectionProviderFactoryBase(aFactoryId,aParentContainer)
	{
	__CFLOG_VAR((KTunnelMCprFactoryTag, KTunnelMCprFactorySubTag, _L8("CTunnelMetaConnectionProviderFactory %08x:\tCTunnelMetaConnectionProviderFactory Constructor"), this));
	}

ACommsFactoryNodeId* CTunnelMetaConnectionProviderFactory::DoCreateObjectL(ESock::TFactoryQueryBase& aQuery)
	{
	const TMetaConnectionFactoryQuery& query = static_cast<const TMetaConnectionFactoryQuery&>(aQuery);
	CMetaConnectionProviderBase* provider = CTunnelMetaConnectionProvider::NewL(*this, query.iProviderInfo);	
	
	ESOCK_DEBUG_REGISTER_GENERAL_NODE(iUid, provider);

	return provider;
	}

