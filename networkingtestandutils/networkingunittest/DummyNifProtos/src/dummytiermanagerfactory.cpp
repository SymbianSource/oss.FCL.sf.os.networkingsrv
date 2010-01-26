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
//


#include <ecom/implementationproxy.h>
#include <comms-infras/ss_log.h>
#include <ecom/ecom.h>

#include "dummytiermanagerfactory.h"
#include "dummytiermanager.h"


#ifdef __CFLOG
#define KDummyTierMgrTag KESockMetaConnectionTag
_LIT(KDummyTierMgrSubTag, "DummyTierMgrSubTag");
#endif

// ---------------- Factory Methods ----------------

/**
Creates a Agent Tier Manager Factory
This function also acts as the single ECom entry point into this object.
@param aParentContainer The parent factory container which owns this factory
@return Factory for IP level meta connection providers
*/
CDummyTierManagerFactory* CDummyTierManagerFactory::NewL(TAny* aParentContainer)
	{
	__CFLOG_VAR((KDummyTierMgrTag, KDummyTierMgrSubTag, _L8("CDummyTierManagerFactory::\tNewL(%08x)"), aParentContainer));
 	return new (ELeave) CDummyTierManagerFactory(TUid::Uid(CDummyTierManagerFactory::iUid),TUid::Uid(CDummyTierManagerFactory::iUid),*(reinterpret_cast<ESock::CTierManagerFactoryContainer*>(aParentContainer)));
	}



/**
Constructor for the Agent Tier Manager Factory
@param aFactoryId The UID which this factory can be looked up by
@param aParentContainer The parent factory container which owns this factory
*/
CDummyTierManagerFactory::CDummyTierManagerFactory(TUid aTierTypeId,TUid aFactoryUid, ESock::CTierManagerFactoryContainer& aParentContainer)
	: CTierManagerFactoryBase(aTierTypeId,aFactoryUid, aParentContainer)
	{
	__CFLOG_VAR((KDummyTierMgrTag, KDummyTierMgrSubTag, _L8("CDummyTierManagerFactory %08x:\tCDummyTierManagerFactory"), this));
	}



/**
Creates the actual Tier Manager
*/
ESock::ACommsFactoryNodeId* CDummyTierManagerFactory::DoCreateObjectL(ESock::TFactoryQueryBase& /* aQuery */)
	{
	return CDummyTierManager::NewL(*this);
	}
