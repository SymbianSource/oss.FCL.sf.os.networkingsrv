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


#include <comms-infras/ss_log.h>

#include <comms-infras/coremcpractivities.h>
#include <comms-infras/coremcprstates.h>

#include <comms-infras/ss_nodemessages_dataclient.h>

#include "agentmcpr.h"
#include "agentmcprstates.h"
#include "agentmessages.h"

#ifdef _DEBUG
#include "agentmcpravailabilitytesting.h"
#endif

#ifdef _DEBUG
// Panic category for "absolutely impossible!" vanilla ASSERT()-type panics from this module
// (if it could happen through user error then you should give it an explicit, documented, category + code)
_LIT(KSpecAssert_NifManAgtPrCMSta, "NifManAgtPrCMSta");
#endif


#ifdef __CFLOG_ACTIVE
#define KAgentMCprTag KESockMetaConnectionTag
_LIT8(KAgentMCprSubTag, "AgentMCprStates");
#endif

using namespace NetStateMachine;
using namespace ESock;
using namespace MCprActivities;



namespace AgentMCprStates
{
// ---------------- NoBearer Activity Overrides ----------------

EXPORT_DEFINE_SMELEMENT(TSendBindTo, NetStateMachine::MStateTransition, AgentMCprStates::TContext)
void TSendBindTo::DoL()
    {
    __CFLOG_VAR((KAgentMCprTag, KAgentMCprSubTag, _L8("ASendBindTo::DoL()")));
    __ASSERT_DEBUG(iContext.iNodeActivity, User::Panic(KSpecAssert_NifManAgtPrCMSta, 1));

    // Send empty BindTo message to stay in line with the framework design
    ESock::TCommsBinder binder;
	iContext.iNodeActivity->PostToOriginators(ESock::TCFControlProvider::TBindTo(binder).CRef());
    }


} // namespace AgentMCprStates







