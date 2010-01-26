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

#include "agenttiermanagerfactory.h"
#include "agenttiermanager.h"


#ifdef __CFLOG_ACTIVE
#define KAgentTierMgrTag KESockMetaConnectionTag
_LIT8(KAgentTierMgrSubTag, "agenttiermgr");
#endif




// ---------------- Factory Methods ----------------

/**
Creates a Agent Tier Manager Factory
This function also acts as the single ECom entry point into this object.
@param aParentContainer The parent factory container which owns this factory
@return Factory for IP level meta connection providers
*/
CAgentTierManagerFactory* CAgentTierManagerFactory::NewL(TAny* aParentContainer)
	{
	__CFLOG_VAR((KAgentTierMgrTag, KAgentTierMgrSubTag, _L8("CAgentTierManagerFactory::\tNewL(%08x)"), aParentContainer));
 	return new (ELeave) CAgentTierManagerFactory(TUid::Uid(CAgentTierManagerFactory::iUid),TUid::Uid(CAgentTierManagerFactory::iUid),*(reinterpret_cast<ESock::CTierManagerFactoryContainer*>(aParentContainer)));
	}



/**
Constructor for the Agent Tier Manager Factory
@param aFactoryId The UID which this factory can be looked up by
@param aParentContainer The parent factory container which owns this factory
*/
CAgentTierManagerFactory::CAgentTierManagerFactory(TUid aTierTypeId,TUid aFactoryUid, ESock::CTierManagerFactoryContainer& aParentContainer)
	: CTierManagerFactoryBase(aTierTypeId, aFactoryUid, aParentContainer)
	{
	}



/**
Creates the actual Tier Manager
*/
ESock::ACommsFactoryNodeId* CAgentTierManagerFactory::DoCreateObjectL(ESock::TFactoryQueryBase& /* aQuery */)
	{
	return CAgentTierManager::NewL(*this);
	}

