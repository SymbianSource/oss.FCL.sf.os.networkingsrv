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
// This is part of an ECOM plug-in
// 
//

#define SYMBIAN_NETWORKING_UPS

#include <ecom/implementationproxy.h>
#include "netmcprfactory.h"
#ifdef SYMBIAN_NETWORKING_UPS
#include "netmcprups.h"
#else
#include "netmcpr.h"
#endif
#include <comms-infras/ss_log.h>
#include <ss_glob.h>
#include <ecom/ecom.h>

#include <comms-infras/ss_msgintercept.h>

#ifdef __CFLOG_ACTIVE
#define KNetMCprFactoryTag KESockMetaConnectionTag
_LIT8(KNetMCprFactorySubTag, "netmcprfactory");
#endif

using namespace ESock;


EXPORT_C CNetworkMetaConnectionProviderFactory* CNetworkMetaConnectionProviderFactory::NewL(TAny* aParentContainer)
/** Factory function for the factory which manages network level meta connection providers.
This function also acts as the single ECom entry point into this object.
@param aParentContainer the parent factory container which owns this factory
@return factory for IP level meta connection providers
*/
	{
	__CFLOG_VAR((KNetMCprFactoryTag, KNetMCprFactorySubTag, _L8("CNetworkMetaConnectionProviderFactory ::\tNewL(%08x)"), aParentContainer));
 	return new (ELeave) CNetworkMetaConnectionProviderFactory(TUid::Uid(CNetworkMetaConnectionProviderFactory::iUid), *(reinterpret_cast<CMetaConnectionFactoryContainer*>(aParentContainer)));
	}

CNetworkMetaConnectionProviderFactory::CNetworkMetaConnectionProviderFactory(TUid aFactoryUid, CMetaConnectionFactoryContainer& aParentContainer)
	: CMetaConnectionProviderFactoryBase(aFactoryUid,aParentContainer)
/** Constructor for Network level meta connection providers.
@param aFactoryId the ID which this factory can be looked up by
@param aParentContainer the parent factory container which owns this factory
*/
	{
	}

ACommsFactoryNodeId* CNetworkMetaConnectionProviderFactory::DoCreateObjectL(TFactoryQueryBase& aQuery)
	{
	const TMetaConnectionFactoryQuery& query = static_cast<const TMetaConnectionFactoryQuery&>(aQuery);
#ifdef SYMBIAN_NETWORKING_UPS
	CMetaConnectionProviderBase* provider = CUpsNetworkMetaConnectionProvider::NewL(*this,query.iProviderInfo);
#else
	CMetaConnectionProviderBase* provider = CNetworkMetaConnectionProvider::NewL(*this,query.iProviderInfo);
#endif

	ESOCK_DEBUG_REGISTER_GENERAL_NODE(iUid, provider);

	return provider;
	}

