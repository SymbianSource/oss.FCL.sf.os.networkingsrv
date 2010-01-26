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
// This is part of an ECOM plug-in
// 
//

#include "iptiermanagerfactory.h"
#include "iptiermanager.h"
#include <comms-infras/ss_log.h>

#ifdef __CFLOG_ACTIVE
#define KIpTierMgrFactoryTag KESockMetaConnectionTag
_LIT8(KIpTierMgrSubTag, "iptiermanager");
#endif // __CFLOG_ACTIVE

using namespace ESock;


EXPORT_C CIpTierManagerFactory* CIpTierManagerFactory::NewL(TAny* aParentContainer)
/** Factory function for the factory which manages ip level meta connection providers.
This function also acts as the single ECom entry point into this object.
@param aParentContainer the parent factory container which owns this factory
@return factory for IP level meta connection providers
*/
	{
	__CFLOG_VAR((KIpTierMgrFactoryTag, KIpTierMgrSubTag, _L8("CIpTierManagerFactory::\tNewL(%08x)"), aParentContainer));
 	return new (ELeave) CIpTierManagerFactory(TUid::Uid(CIpTierManagerFactory::iType),TUid::Uid(CIpTierManagerFactory::iUid),*(reinterpret_cast<CTierManagerFactoryContainer*>(aParentContainer)));
	}

CIpTierManagerFactory::CIpTierManagerFactory(TUid aTierTypeId, TUid aFactoryUid, CTierManagerFactoryContainer& aParentContainer)
	: CTierManagerFactoryBase(aTierTypeId, aFactoryUid, aParentContainer)
/** Constructor for Ip level meta connection providers.
@param aFactoryId the ID which this factory can be looked up by
@param aParentContainer the parent factory container which owns this factory
*/
	{
	}

ESock::ACommsFactoryNodeId* CIpTierManagerFactory::DoCreateObjectL(ESock::TFactoryQueryBase& /* aQuery */)
	{
	return CIpTierManager::NewL(*this);
	}
