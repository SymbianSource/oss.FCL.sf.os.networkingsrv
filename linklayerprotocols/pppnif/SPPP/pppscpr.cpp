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
// PPP SCpr
// 
//

/**
 @file
 @internalComponent
*/


#include <comms-infras/linkmessages.h>
#include <comms-infras/nifif.h>
#include "pppscpr.h"
#include "pppmessages.h"
#include "CAgentAdapter.h"

#include <elements/sd_mintercept.h>

using namespace Messages;
using namespace ESock;
using namespace MeshMachine;

#ifdef __CFLOG_ACTIVE
#define KPppSCprTag KESockSubConnectionTag
_LIT8(KPppSCprSubTag, "pppscpr");
#endif

//We reserve space for two preallocated activities that may start concurrently on the SCPR
//node: destroy and data client stop.
static const TUint KDefaultMaxPreallocatedActivityCount = 2;
static const TUint KMaxPreallocatedActivitySize = sizeof(MeshMachine::CNodeRetryParallelActivity) + sizeof(MeshMachine::APreallocatedOriginators<4>);
static const TUint KPPPSCPRPreallocatedActivityBufferSize = KDefaultMaxPreallocatedActivityCount * KMaxPreallocatedActivitySize;

namespace PppSCprStates
{


DEFINE_SMELEMENT(TAwaitingPppLinkStatusChange, NetStateMachine::MState, PppSCprStates::TContext)
TBool TAwaitingPppLinkStatusChange::Accept()
	{
	return iContext.iMessage.IsMessage<TPPPMessage::TPppLinkExpectingCallback>();
	}

DEFINE_SMELEMENT(TProcessPppLinkStatusChange, NetStateMachine::MStateTransition, PppSCprStates::TContext)
void TProcessPppLinkStatusChange::DoL()
    {
   
    /* ------------------------------------------------------
     * Logic from Nifman's CNifAgentRef::LinkLayerDown
     * ------------------------------------------------------
     * iState is the state held in the CAgentAdapter
     *
     * iState = EConnecting;
     * iConnectType=EAgentStartCallBack;
     * iAgent->Connect(iConnectType);
     */

    iContext.Node().NotificationToAgent (EFlowToAgentEventTypeLinkLayerDown, NULL);
    iContext.Node().ConnectAgent(EAgentStartCallBack);
    }

} // namespace PppSCprStates





namespace PppSCprLinkStatusChangeActivity
{
DECLARE_DEFINE_NODEACTIVITY(ECFPppLinkStatusChangeActivity, PppLinkStatusChange, TPPPMessage::TPppLinkExpectingCallback)
	FIRST_NODEACTIVITY_ENTRY(PppSCprStates::TAwaitingPppLinkStatusChange, MeshMachine::TNoTag)
    LAST_NODEACTIVITY_ENTRY(KNoTag, PppSCprStates::TProcessPppLinkStatusChange)
NODEACTIVITY_END()
}


// PPP Activity Map
namespace PppSCprActivities
{
DECLARE_DEFINE_ACTIVITY_MAP(pppSCprActivities)
   ACTIVITY_MAP_ENTRY(PppSCprLinkStatusChangeActivity, PppLinkStatusChange)
ACTIVITY_MAP_END_BASE(AgentSCprActivities, agentSCprActivities)
}


// Basic constructor passing in an alternative activity map for the AgentSCpr
CPppSubConnectionProvider* CPppSubConnectionProvider::NewL(ESock::CSubConnectionProviderFactoryBase& aFactory)
    {
    CPppSubConnectionProvider* self = new (ELeave) CPppSubConnectionProvider(aFactory, PppSCprActivities::pppSCprActivities::Self());
    CleanupStack::PushL(self);
    self->ConstructL(KPPPSCPRPreallocatedActivityBufferSize);
    CleanupStack::Pop();
    return self;
    }


// C'tor
CPppSubConnectionProvider::CPppSubConnectionProvider(ESock::CSubConnectionProviderFactoryBase& aFactory,
	    const MeshMachine::TNodeActivityMap& aActivityMap)
	: CAgentSubConnectionProvider(aFactory, aActivityMap)
    {
    // NOTE: We don't do a LOG_CREATE_NODE here because its already being done by
    // the base class. This class doesn't add anything and only overrides the activity
    // map. So for all intents and purposes this is still just an AgentSCpr.
    }




