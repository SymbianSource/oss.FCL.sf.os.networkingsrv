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

/**
 @file
 @internalTechnology
 @prototype
*/
//EPOC/DV3/team/2009/CF/dev/symtb92/os/commsfw/datacommsserver/networkinterfacemgr/agentpr/src/agenttiermanager.cpp#2
#include <comms-infras/ss_log.h>
#include <comms-infras/coretiermanagerstates.h>
#include <comms-infras/coretiermanageractivities.h>

#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <commsdat_partner.h>
#include <es_enum_internal.h>
#endif

#include "agenttiermanager.h"
#include "agenttiermanagerselector.h"


#if defined __CFLOG_ACTIVE || defined SYMBIAN_TRACE_ENABLE
#define KAgentTierMgrTag KESockMetaConnectionTag
_LIT8(KAgentTierMgrSubTag, "agenttiermgr");
#endif

using namespace Messages;
using namespace MeshMachine;
using namespace ESock;
using namespace NetStateMachine;

namespace AgentTierManagerActivities
{
DECLARE_DEFINE_ACTIVITY_MAP(stateMap)
ACTIVITY_MAP_END_BASE(TMActivities, coreTMActivities)
}


/**
Factory function for the factory which manages network level meta connection providers.
This function also acts as the single ECom entry point into this object.
@param aParentContainer The parent factory container which owns this factory
@return Factory for IP level meta connection providers
*/
CAgentTierManager* CAgentTierManager::NewL(ESock::CTierManagerFactoryBase& aFactory)
	{
 	return new (ELeave) CAgentTierManager(aFactory, AgentTierManagerActivities::stateMap::Self());
	}


/**
Constructor for Network level meta connection providers.
@param aParentContainer The parent factory container which owns this factory
*/
CAgentTierManager::CAgentTierManager(ESock::CTierManagerFactoryBase& aFactory,
                                   const MeshMachine::TNodeActivityMap& aActivityMap)
:	CCoreTierManager(aFactory,aActivityMap)
	{
	LOG_NODE_CREATE(KAgentTierMgrTag, CAgentTierManager);
	}

CAgentTierManager::~CAgentTierManager()
	{
	LOG_NODE_DESTROY(KAgentTierMgrTag, CAgentTierManager);
	}

/**
Create selector for this Tier.
*/
MProviderSelector* CAgentTierManager::DoCreateProviderSelectorL(const Meta::SMetaData& aSelectionPreferences)
	{
	__CFLOG_VAR((KAgentTierMgrTag, KAgentTierMgrSubTag, _L8("CAgentTierManager[%08x]::DoSelectProvider()"), this));
  	return TAgentSelectorFactory::NewSelectorL(aSelectionPreferences);
	}


void CAgentTierManager::ReceivedL(const TRuntimeCtxId& aSender, const TNodeId& aRecipient, TSignatureBase& aMessage)
	{
   	TNodeContext<CAgentTierManager> ctx(*this, aMessage, aSender, aRecipient);
   	CCoreTierManager::Received(ctx);
    User::LeaveIfError(ctx.iReturn);
	}

