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
*/

#include "e32property.h"
#include <comms-infras/ss_log.h>
#include <comms-infras/coretiermanagerstates.h>
#include <comms-infras/coretiermanageractivities.h>

#include "dummynifvar.h"
#include "dummynif_params.h"
#include "dummytiermanager.h"
#include "dummytiermanagerselector.h"

#include <comms-infras/ss_mcprnodemessages.h>
#include <comms-infras/esock_params.h>



using namespace ESock;


#ifdef __CFLOG
#define KDummyTierMgrTag KESockMetaConnectionTag
#endif

using namespace ESock;
using namespace NetStateMachine;
using namespace MeshMachine;
using namespace Messages;

namespace DummyTMSelectProviderActivity
{ //Simple parallel activity provider selection, waits untill selection completes (via ISelectionNotify), then gets destroyed
DECLARE_DEFINE_CUSTOM_NODEACTIVITY(ECFActivitySelect, DummyTMSelectProvider, TCFSelector::TSelect, TMActivities::CSelectProviderActivity::NewL)
	FIRST_NODEACTIVITY_ENTRY(TMStates::TAwaitingSelectProviderSuper, MeshMachine::TNoTag)
	THROUGH_NODEACTIVITY_ENTRY(KNoTag, DummyTMStates::TSelectionTest, MeshMachine::TNoTag)
	NODEACTIVITY_ENTRY(KNoTag, TMStates::TSelectProviderSuper, CoreStates::TNeverAccept, MeshMachine::TNoTag)
	LAST_NODEACTIVITY_ENTRY(KNoTag, MeshMachine::TDoNothing)
NODEACTIVITY_END()
}


DEFINE_SMELEMENT(DummyTMStates::TSelectionTest, NetStateMachine::MStateTransition, DummyTMStates::TContext)
void DummyTMStates::TSelectionTest::DoL()
	{
	RConnPrefList handle = message_cast<TCFSelector::TSelect>(iContext.iMessage).iConnPrefList;
	RConnPrefList::TIter<TDummyPref> iter = handle.getIter<TDummyPref>();
	while(!iter.IsEnd())
		{
		TInt AP = iter->AP();

		RProperty property;
		TInt result = property.Attach(TUid::Uid(0x10203FDD), 0);
		if(result == KErrNone)
			{
			TInt propertyValue;
			result = property.Get(propertyValue);
			if(result == KErrNone && propertyValue == AP)
				{
				property.Set(KErrNone);
				}
			}
		iter++;
		}
	}

namespace DummyTierManagerActivities
{
	DECLARE_DEFINE_ACTIVITY_MAP(stateMap)
	ACTIVITY_MAP_ENTRY(DummyTMSelectProviderActivity,DummyTMSelectProvider)
	ACTIVITY_MAP_END_BASE(TMActivities, coreTMActivities)
}


/**
Factory function for the factory which manages network level meta connection providers.
This function also acts as the single ECom entry point into this object.
@param aParentContainer The parent factory container which owns this factory
@return Factory for IP level meta connection providers
*/
CDummyTierManager* CDummyTierManager::NewL(ESock::CTierManagerFactoryBase& aFactory)
	{
 	return new (ELeave) CDummyTierManager(aFactory, DummyTierManagerActivities::stateMap::Self());
	}


/**
Constructor for Network level meta connection providers.
@param aParentContainer The parent factory container which owns this factory
*/
CDummyTierManager::CDummyTierManager(ESock::CTierManagerFactoryBase& aFactory,
                                   const MeshMachine::TNodeActivityMap& aActivityMap)
:	CCoreTierManager(aFactory,aActivityMap)
	{
	LOG_NODE_CREATE(KDummyTierMgrTag, CDummyTierManager);
	}

CDummyTierManager::~CDummyTierManager()
	{
	LOG_NODE_DESTROY(KDummyTierMgrTag, CDummyTierManager);
	}

/**
Create selector for this Tier.
*/
MProviderSelector* CDummyTierManager::DoCreateProviderSelectorL(const Meta::SMetaData& aSelectionPreferences)
	{
	__CFLOG_VAR((KDummyTierMgrTag, KDummyTierMgrSubTag, _L8("CDummyTierManager[%08x]::DoSelectProvider()"), this));
  	return TDummySelectorFactory::NewSelectorL(aSelectionPreferences);
	}


void CDummyTierManager::ReceivedL(const TRuntimeCtxId& aSender, const TNodeId& aRecipient, TSignatureBase& aMessage)
	{
	TNodeContext<CDummyTierManager> ctx(*this, aMessage, aSender, aRecipient);
	CCoreTierManager::Received(ctx);
    User::LeaveIfError(ctx.iReturn);
	}


