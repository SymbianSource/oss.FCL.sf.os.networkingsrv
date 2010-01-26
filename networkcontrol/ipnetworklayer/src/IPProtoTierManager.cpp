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
// IPProto Tier Manager
// 
//

/**
 @file
 @internalComponent
*/

#include <comms-infras/ss_log.h>
#include <comms-infras/ss_coreprstates.h>
#include <comms-infras/ss_corepractivities.h>
#include <comms-infras/coretiermanagerstates.h>
#include <comms-infras/coretiermanageractivities.h>
#include <comms-infras/coretiernotificationactivity.h>
#include <comms-infras/coretiernotificationstates.h>
#include "IPProtoTierManager.h"
#include "IPProtoTierManagerSelector.h"
#include <comms-infras/ss_metaconnprov.h>
#include <in_sock.h>

#ifdef SYMBIAN_TRACE_ENABLE
#define KIPProtoTierMgrTag KESockMetaConnectionTag
// _LIT8(KIPProtoTierMgrSubTag, "ipprototiermgr");
#endif // SYMBIAN_TRACE_ENABLE

using namespace Messages;
using namespace MeshMachine;
using namespace ESock;
using namespace NetStateMachine;
using namespace TMActivities;
using namespace CorePanics;

//-=========================================================
// Panics
#ifdef _DEBUG
_LIT (KIpProtoTMPanic,"IpProtoTMPanic");
#endif


//This transition creates a link between legacy NetMCpr & legacy CLinkPrefsSelector
DEFINE_SMELEMENT(IpProtoTMStates::TSelectProvider, NetStateMachine::MStateTransition, IpProtoTMStates::TContext)
void IpProtoTMStates::TSelectProvider::DoL()
	{
	__ASSERT_DEBUG(iContext.iNodeActivity, User::Panic(KIpProtoTMPanic, KPanicNoActivity));
	CSelectProviderActivity* act = static_cast<CSelectProviderActivity*>(iContext.iNodeActivity);
	
	//Create selector
	act->iSelector = iContext.Node().DoCreateProviderSelectorL(message_cast<TCFSelector::TSimpleSelect>(iContext.iMessage).iSelectionPrefs);
	
	TBool isItLegacyMcpr = EFalse;
	
	//First assumption: At this layer (IpProto) all SelectProvider requests are always comming from MCprs.
#if !defined(__GCCXML__)
	CMetaConnectionProviderBase& mcpr = mnode_cast<CMetaConnectionProviderBase>(address_cast<TNodeId>(iContext.iSender).Node());
#else
	CMetaConnectionProviderBase& mcpr =
		reinterpret_cast<CMetaConnectionProviderBase&>(address_cast<TNodeId>(iContext.iSender).Node());
#endif

	//Second assumption: All MCprs at an upper layer (above IpProto) use the Instance() to indicate that they are legacy.
	if (mcpr.ProviderInfo().Instance() !=0) 
		{ //We have a legacy provider above us
		ASSERT(mcpr.ProviderInfo().TierId() == TUid::Uid(KAfInet)); //Here we can partially test the second assumption

		ASimpleSelectorBase& baseselector = static_cast<ASimpleSelectorBase&>(*act->iSelector);
		if (baseselector.TypeId() == CLinkPrefsSelector::KUid)
			{
			CLinkPrefsSelector& selector = static_cast<CLinkPrefsSelector&>(*act->iSelector);
			selector.iLegacyMCpr = &mcpr;
			isItLegacyMcpr = ETrue;
			}
		}
	
	if (!isItLegacyMcpr)
		{
		//A 399 selection is ongoing. Note: this code is intentionally not generic and needs to be reworking.
		AIpProtoSelectorBase& currentSelector = static_cast<AIpProtoSelectorBase&>(*act->iSelector);
		currentSelector.iNetworkMCpr = &mcpr;
		
		if (iContext.ActivityId() == ESock::ECFActivityConnectionStartRecovery)
			{
			currentSelector.iReselection = ETrue;
			}
		}
	
	ISelectionNotify selectionNotify(act,act->InterfaceVTable());
	act->iSelector->SelectL(selectionNotify);
	//Do not do any further processing here as selectors may be returning synchronously (via ISelectionNotify).
	}

DEFINE_SMELEMENT(IpProtoTMStates::TSelectProviderConnPrefList, NetStateMachine::MStateTransition, IpProtoTMStates::TContext)
void IpProtoTMStates::TSelectProviderConnPrefList::DoL()
	{
	__ASSERT_DEBUG(iContext.iNodeActivity, User::Panic(KIpProtoTMPanic, KPanicNoActivity));
	CSelectProviderActivity* act = static_cast<CSelectProviderActivity*>(iContext.iNodeActivity);

	//Create selector
	//If a ConnPrefList is being used we know what kind of Selector to use
	//(as only one kind supports it) so we can create it without calling
	//the intermidiate DoCreateProviderSelectorL
	act->iSelector = new (ELeave) CIpProtoProviderSelectorConnPrefList(message_cast<TCFSelector::TSelect>(iContext.iMessage).iConnPrefList); 

	//First assumption: At this layer (IpProto) all SelectProvider requests are always comming from MCprs.
#if !defined(__GCCXML__)
	CMetaConnectionProviderBase& mcpr = mnode_cast<CMetaConnectionProviderBase>(address_cast<TNodeId>(iContext.iSender).Node());
#else
	CMetaConnectionProviderBase& mcpr =
		reinterpret_cast<CMetaConnectionProviderBase&>(address_cast<TNodeId>(iContext.iSender).Node());
#endif
		
	if (mcpr.ProviderInfo().Instance() !=0) 
		{ //We have a legacy provider above us
		ASSERT(mcpr.ProviderInfo().TierId() == TUid::Uid(KAfInet)); //Here we can partially test the second assumption

		ASimpleSelectorBase& baseselector = static_cast<ASimpleSelectorBase&>(*act->iSelector);
		if (baseselector.TypeId() == CLinkPrefsSelector::KUid)
			{
			CLinkPrefsSelector& selector = static_cast<CLinkPrefsSelector&>(*act->iSelector);
			selector.iLegacyMCpr = &mcpr;
			}
		}
	else
		{
		//A 399 selection is ongoing. Note: this code is intentionally not generic and needs to be reworking.
		AIpProtoSelectorBase& currentSelector = static_cast<AIpProtoSelectorBase&>(*act->iSelector);
		currentSelector.iNetworkMCpr = &mcpr;
		
		if (iContext.ActivityId() == ESock::ECFActivityConnectionStartRecovery)
			{
			currentSelector.iReselection = ETrue;
			}
		}
	
	ISelectionNotify selectionNotify(act,act->InterfaceVTable());
	act->iSelector->SelectL(selectionNotify);
	//Do not do any further processing here as selectors may be returning synchronously (via ISelectionNotify).
	}

namespace IpProtoTMSelectProviderActivity
{ //Simple parallel activity provider selection, waits untill selection completes (via ISelectionNotify), then gets destroyed
DECLARE_DEFINE_CUSTOM_NODEACTIVITY(ECFActivitySelect, IpProtoTMSelectProvider, TCFSelector::TSimpleSelect, CSelectProviderActivity::NewL)
	FIRST_NODEACTIVITY_ENTRY(TMStates::TAwaitingSelectProvider, MeshMachine::TNoTag)
	NODEACTIVITY_ENTRY(KNoTag, IpProtoTMStates::TSelectProvider, CoreStates::TNeverAccept, MeshMachine::TNoTag)
	LAST_NODEACTIVITY_ENTRY(KNoTag, MeshMachine::TDoNothing)
NODEACTIVITY_END()
}

namespace IpProtoTMSelectProviderConnPrefListActivity
{ //Simple parallel activity provider selection, waits untill selection completes (via ISelectionNotify), then gets destroyed
DECLARE_DEFINE_CUSTOM_NODEACTIVITY(ECFActivitySelect, IpProtoTMSelectConnPrefListProvider, TCFSelector::TSelect, CSelectProviderActivity::NewL)
	FIRST_NODEACTIVITY_ENTRY(TMStates::TAwaitingSelectProviderSuper, MeshMachine::TNoTag)
	NODEACTIVITY_ENTRY(KNoTag, IpProtoTMStates::TSelectProviderConnPrefList, CoreStates::TNeverAccept, MeshMachine::TNoTag)
	LAST_NODEACTIVITY_ENTRY(KNoTag, MeshMachine::TDoNothing)
NODEACTIVITY_END()
}

namespace IpProtoTMActivities
{
DECLARE_DEFINE_ACTIVITY_MAP(stateMap)
	ACTIVITY_MAP_ENTRY(IpProtoTMSelectProviderActivity,IpProtoTMSelectProvider)
	ACTIVITY_MAP_ENTRY(IpProtoTMSelectProviderConnPrefListActivity,IpProtoTMSelectConnPrefListProvider)
ACTIVITY_MAP_END_BASE(TMActivities, coreTMActivities)
}


CIPProtoTierManager* CIPProtoTierManager::NewL(ESock::CTierManagerFactoryBase& aFactory)
	{
 	return new (ELeave) CIPProtoTierManager(aFactory, IpProtoTMActivities::stateMap::Self());
	}

CIPProtoTierManager::CIPProtoTierManager(ESock::CTierManagerFactoryBase& aFactory,
                                         const MeshMachine::TNodeActivityMap& aActivityMap)
:	CCoreTierManager(aFactory,aActivityMap)
	{
	LOG_NODE_CREATE(KIPProtoTierMgrTag, CIPProtoTierManager);
	}

CIPProtoTierManager::~CIPProtoTierManager()
	{
	LOG_NODE_DESTROY(KIPProtoTierMgrTag, CIPProtoTierManager);
	}

ESock::MProviderSelector* CIPProtoTierManager::DoCreateProviderSelectorL(const Meta::SMetaData& aSelectionPreferences)
	{
    return TIpProtoProviderSelectorFactory::NewSelectorL(aSelectionPreferences);
	}

void CIPProtoTierManager::ReceivedL(const TRuntimeCtxId& aSender, const TNodeId& aRecipient, TSignatureBase& aMessage)
	{
   	TNodeContext<CIPProtoTierManager> ctx(*this, aMessage, aSender, aRecipient);
   	CCoreTierManager::Received(ctx);
    User::LeaveIfError(ctx.iReturn);
	}



