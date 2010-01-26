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

#define SYMBIAN_NETWORKING_UPS

#include <comms-infras/ss_log.h>
#include <comms-infras/ss_corepractivities.h>
#include <comms-infras/coretiermanagerstates.h>
#include <comms-infras/coretiermanageractivities.h>
#include "iptiermanager.h"
#include "iptiermanagerselector.h"

#ifdef SYMBIAN_NETWORKING_UPS
#include "iptiermanagerfactory.h"		// CIpTierManagerFactory::iUid
#include <comms-infras/netups.h>
#include <comms-infras/netupsserviceid.h>
using namespace NetUps;
#endif

#ifdef SYMBIAN_TRACE_ENABLE
	#define KIpTierMgrTag KESockMetaConnectionTag
	// _LIT8(KIpTierMgrSubTag, "iptiermgr");
#endif

using namespace Messages;
using namespace MeshMachine;
using namespace ESock;
using namespace NetStateMachine;

namespace IpTierManagerActivities
{
DECLARE_DEFINE_ACTIVITY_MAP(stateMap)
ACTIVITY_MAP_END_BASE(TMActivities, coreTMActivities)
}


CIpTierManager* CIpTierManager::NewL(ESock::CTierManagerFactoryBase& aFactory)
/** Factory function for the factory which manages ip level meta connection providers.
This function also acts as the single ECom entry point into this object.
@param aParentContainer the parent factory container which owns this factory
@return factory for IP level meta connection providers
*/
	{
#ifdef SYMBIAN_NETWORKING_UPS
 	CIpTierManager* self = new (ELeave) CIpTierManager(aFactory, IpTierManagerActivities::stateMap::Self());
	CleanupStack::PushL(self);
	self->OpenNetUpsL();
	CleanupStack::Pop(self);
	return self;
#else
 	return new (ELeave) CIpTierManager(aFactory, IpTierManagerActivities::stateMap::Self());
#endif
	}

CIpTierManager::CIpTierManager(ESock::CTierManagerFactoryBase& aFactory,
                                 const MeshMachine::TNodeActivityMap& aActivityMap)
:	CCoreTierManager(aFactory,aActivityMap)
/** Constructor for ip level meta connection providers.
@param aFactoryId the ID which this factory can be looked up by
@param aParentContainer the parent factory container which owns this factory
*/
	{
	LOG_NODE_CREATE(KIpTierMgrTag, CIpTierManager);
	}

CIpTierManager::~CIpTierManager()
	{
#ifdef SYMBIAN_NETWORKING_UPS
	CloseNetUps();
#endif

	LOG_NODE_DESTROY(KIpTierMgrTag, CIpTierManager);
	}

MProviderSelector* CIpTierManager::DoCreateProviderSelectorL(const Meta::SMetaData& aSelectionPreferences)
	{
	//Decide which selector to create based on the information available.
	return TIpProviderSelectorFactory::NewSelectorL(aSelectionPreferences);
	}

void CIpTierManager::ReceivedL(const TRuntimeCtxId& aSender, const TNodeId& aRecipient, TSignatureBase& aMessage)
	{
   	TNodeContext<CIpTierManager> ctx(*this, aMessage, aSender, aRecipient);
   	CCoreTierManager::Received(ctx);
    User::LeaveIfError(ctx.iReturn);
	}

#ifdef SYMBIAN_NETWORKING_UPS

void CIpTierManager::OpenNetUpsL()
	{
	// @TODO PREQ1116 - temporary hack, this needs to be allocated from a DLL that will retain the
	// NetUps instance until ESock terminates.
	ASSERT(iNetUps == NULL);
	TRAP_IGNORE(iNetUps = NetUps::CNetUps::NewL(NetUps::EIpServiceId));
	}

void CIpTierManager::CloseNetUps()
	{
	if (iNetUps)
		{
		delete iNetUps;
		iNetUps = NULL;
		}
	}

#endif
