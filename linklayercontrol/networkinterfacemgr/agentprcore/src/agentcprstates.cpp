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

#include "agentcpr.h"
#include "agentcprstates.h"
#include "agentmessages.h"
#include <comms-infras/ss_nodemessages_dataclient.h>

#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
#include <comms-infras/ss_nodemessages_scpr.h>
#include <networking/ipcpr_states.h>
#endif //SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW

#ifdef _DEBUG
// Panic category for "absolutely impossible!" vanilla ASSERT()-type panics from this module
// (if it could happen through user error then you should give it an explicit, documented, category + code)
_LIT(KSpecAssert_NifManAgtPrCCSta, "NifManAgtPrCCSta");
#endif


#ifdef __CFLOG_ACTIVE
#define KAgentCprTag KESockConnectionTag
_LIT8(KAgentCprSubTag, "agentcprstates");
#endif


using namespace AgentCprStates;
using namespace Messages;
using namespace MeshMachine;
using namespace ESock;

// ---------------- NoBearer Activity Overrides ----------------

DEFINE_SMELEMENT(TSendBindTo, NetStateMachine::MStateTransition, AgentCprStates::TContext)
void TSendBindTo::DoL()
    {
    __CFLOG_VAR((KAgentCprTag, KAgentCprSubTag, _L8("ASendBindTo::DoL()")));
    __ASSERT_DEBUG(iContext.iNodeActivity, User::Panic(KSpecAssert_NifManAgtPrCCSta, 1));

    // Forward the TCommsBinder
    iContext.iNodeActivity->PostToOriginators(iContext.iMessage);
	}

#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
/*
Updates the Parameter bundle with the bearer type
*/
DEFINE_SMELEMENT(TUpdateBundle, NetStateMachine::MStateTransition, AgentCprStates::TContext)
void AgentCprStates::TUpdateBundle::DoL()
	{
	//Receives the request for bearer type query
	TCFScpr::TGetParamsRequest& paramRequest = message_cast<TCFScpr::TGetParamsRequest>(iContext.iMessage);
	if( (! paramRequest.iFamilyBundle.IsNull()) && (iContext.Node().GetParameterBundle() != paramRequest.iFamilyBundle))
		{
		iContext.Node().GetParameterBundle().Open(paramRequest.iFamilyBundle);
		//Find the family for bearer info
		RParameterFamily family = iContext.Node().GetParameterBundle().FindFamily(KBearerInfo);

		if(!family.IsNull())
			{
			//Find Parameterset relevant to the Type Id and Uid
			XBearerInfo *bearerType = static_cast<XBearerInfo*>(family.FindParameterSet(STypeId::CreateSTypeId(KIpBearerInfoUid, KIpBearerInfoParameterType), RParameterFamily::ERequested));

			if(bearerType)
				{
				//Query the agent and set the bearer type
				bearerType->SetBearerType(iContext.Node().AgentProvisionInfo()->AgentAdapter()->Agent()->GetBearerInfo());
				}
			}

		}
	}
#endif //SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW


