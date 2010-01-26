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
// IP SubConnection Provider states/transitions
// 
//

/**
 @file
 @internalComponent
*/

#include <comms-infras/corescpractivities.h>
#include <comms-infras/ss_coreprstates.h>
#include "ipscprstates.h"
#include "ipscprlog.h"

using namespace Messages;
using namespace ESock;
using namespace IpSCprStates;
using namespace MeshMachine;
using namespace CorePanics;

#if _DEBUG
_LIT(KIpSCprPanic, "IpSCprPanic");
#endif

//-=========================================================
//
// Activities
//
//-=========================================================

//-=========================================================
//
//States
//
//-=========================================================
DEFINE_SMELEMENT(TAwaitingJoinComplete, NetStateMachine::MState, IpSCprStates::TContext)
TBool TAwaitingJoinComplete::Accept()
    {
	__IPCPRLOG(IpCprLog::Printf(_L("TAwaitingJoinComplete::Accept [%08x]"), this));
	return iContext.iMessage.IsMessage<TCFPeer::TJoinComplete>();
    }
//-=========================================================
//
//State Forks
//
//-=========================================================

DEFINE_SMELEMENT(TNoTagOrSendApplyResponseOrErrorTag, NetStateMachine::MStateFork, IpSCprStates::TContext)
TInt TNoTagOrSendApplyResponseOrErrorTag::TransitionTag()
	{
	__IPCPRLOG(IpCprLog::Printf(_L("TNoTagSendApplyResponseOrErrorTag::TransitionTag [%08x]"), this));

	RIPDataClientNodeInterface* client = static_cast<RIPDataClientNodeInterface*>(
	    iContext.Node().GetFirstClient<TDefaultClientMatchPolicy>(TClientType(TCFClientType::EData, TCFClientType::EJoining)));

	if (!client)
    	{
    	return SCprActivities::Apply::ESendApplyResponse;
    	}

	TInt err = iContext.Node().AddressCompletionValidation(*client);
	if( KErrNone == err )
		{
		return KNoTag;
		}
	
	iContext.iNodeActivity->SetError(err);
	
    return KErrorTag;
	}

DEFINE_SMELEMENT(TNoTagOrSendApplyResponse, NetStateMachine::MStateFork, IpSCprStates::TContext)
TInt TNoTagOrSendApplyResponse::TransitionTag()
	{
	__IPCPRLOG(IpCprLog::Printf(_L("TNoTagOrSendApplyResponse::TransitionTag [%08x]"), this));

	if (iContext.Node().GetFirstClient<TDefaultClientMatchPolicy>(TClientType(TCFClientType::EData, TCFClientType::EJoining)) == NULL)
    	{
    	return SCprActivities::Apply::ESendApplyResponse;
    	}

    return KNoTag;
	}

DEFINE_SMELEMENT(TNoTagOrDoNothingTag, NetStateMachine::MStateFork, IpSCprStates::TContext)
TInt TNoTagOrDoNothingTag::TransitionTag()
	{
	__IPCPRLOG(IpCprLog::Printf(_L("TNoTagOrNoBearerOrDoNothingTag::TransitionTag [%08x]"), this));

	RIPDataClientNodeInterface* client =
	    static_cast<RIPDataClientNodeInterface*>(iContext.iPeer);

	if(!(client->Flags() & TCFClientType::EJoining)
		&& (KErrNone == iContext.Node().AddressCompletionValidation(*client)) )
		{
		return KNoTag;
		}

	return KDoNothingTag;
	}

DEFINE_SMELEMENT(TDoNothingTag, NetStateMachine::MStateFork, IpSCprStates::TContext)
TInt TDoNothingTag::TransitionTag()
	{
	return KDoNothingTag;
	}

DEFINE_SMELEMENT(TNoTagOrAlreadyStarted, NetStateMachine::MStateFork, IpSCprStates::TContext)
TInt TNoTagOrAlreadyStarted::TransitionTag()
    {
    __ASSERT_DEBUG(iContext.iPeer || iContext.iSender == iContext.NodeId(), User::Panic(KIpSCprPanic,KPanicPeerMessage));
	if (iContext.Node().GetFirstClient<TDefaultClientMatchPolicy>(TClientType(TCFClientType::EServProvider, TCFClientType::EStarted)) != NULL)
		{
		// check Params
		RCFParameterFamilyBundleC& bundle = iContext.Node().GetOrCreateParameterBundleL();
		TInt num,i;
        num = bundle.CountParameterSets();
        STypeId typeId = STypeId::CreateSTypeId( CSubConIPAddressInfoParamSet::EUid,
                                                CSubConIPAddressInfoParamSet::ETypeId );
        for(i=0; i<num; i++)
            {
            RParameterFamily family = bundle.GetFamilyAtIndex(i);
            if (family.IsNull())
                {
                continue;
                }
            else if(family.FindParameterSet(typeId,RParameterFamily::ERequested)!= NULL)
                {
                return CoreNetStates::KAlreadyStarted;
                }
            }
		}
	
    return MeshMachine::KNoTag;
    }
//-=========================================================
//
//Transitions
//
//-=========================================================

#ifndef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
DEFINE_SMELEMENT(TSendParamsToServiceProvider, NetStateMachine::MStateTransition, IpSCprStates::TContext)
void TSendParamsToServiceProvider::DoL()
	{
	__IPCPRLOG(IpCprLog::Printf(_L("TSendParamsToServiceProvider::DoL [%08x]"), this));

	iContext.iNodeActivity->PostRequestTo(*iContext.Node().ServiceProvider(),
	    TCFScpr::TParamsRequest(iContext.Node().iParameterBundle).CRef());
	}
#endif // SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW

DEFINE_SMELEMENT(TSendSelfStart, NetStateMachine::MStateTransition, IpSCprStates::TContext)
void TSendSelfStart::DoL()
	{
	__IPCPRLOG(IpCprLog::Printf(_L("TSendSelfStart::DoL [%08x]"), this));

	__ASSERT_DEBUG(iContext.iNodeActivity, User::Panic(KIpSCprPanic, KPanicNoActivity));
    iContext.iNodeActivity->PostRequestTo(iContext.NodeId(),
    	TCFServiceProvider::TStart().CRef());
	}

DEFINE_SMELEMENT(TSetJoiningFlag, NetStateMachine::MStateTransition, IpSCprStates::TContext)
void TSetJoiningFlag::DoL()
	{
	RIPDataClientNodeInterface* client =
	    static_cast<RIPDataClientNodeInterface*>(iContext.iPeer);

	ASSERT(client);

	client->SetFlags(TCFClientType::EJoining);
   }

DEFINE_SMELEMENT(TClearJoiningFlag, NetStateMachine::MStateTransition, IpSCprStates::TContext)
void TClearJoiningFlag::DoL()
	{
	RIPDataClientNodeInterface* client = static_cast<RIPDataClientNodeInterface*>(
	    iContext.Node().GetFirstClient<TDefaultClientMatchPolicy>(TClientType(TCFClientType::EData, TCFClientType::EJoining)));

	ASSERT(client);

	client->ClearFlags(TCFClientType::EJoining);
   }

DEFINE_SMELEMENT(TPrepareToAddClientToQosChannel, NetStateMachine::MStateTransition, IpSCprStates::TContext)
void TPrepareToAddClientToQosChannel::DoL()
	{
	RIPDataClientNodeInterface* client =
	    static_cast<RIPDataClientNodeInterface*>(iContext.iPeer);

	if (client && !(client->Flags() & TCFClientType::EJoining))
	   {
	   client->SetFlags(TCFClientType::EJoining);

	   IpSCprStates::TAddClientToQosChannel addToChannel(iContext);
	   TRAPD(err, addToChannel.DoL();)

	   client->ClearFlags(TCFClientType::EJoining);

	   User::LeaveIfError(err);
	   }
   }

DEFINE_SMELEMENT(TAddClientToQosChannel, NetStateMachine::MStateTransition, IpSCprStates::TContext)
void TAddClientToQosChannel::DoL()
	{
	__IPCPRLOG(IpCprLog::Printf(_L("TAddClientToQosChannel::DoL [%08x]"), this));

	RIPDataClientNodeInterface* clientToBeAdded = static_cast<RIPDataClientNodeInterface*>(
	    iContext.Node().GetFirstClient<TDefaultClientMatchPolicy>(TClientType(TCFClientType::EData, TCFClientType::EJoining)));
	ASSERT(clientToBeAdded);


	if (KErrNone == iContext.Node().AddressCompletionValidation(*clientToBeAdded) )
		{
		CSubConIPAddressInfoParamSet* IPAdressInfoParamSet = iContext.Node().InitBundleL();

		if(IPAdressInfoParamSet)
			{
			// add Param info
			IPAdressInfoParamSet->AddParamInfo(CSubConIPAddressInfoParamSet::TSubConIPAddressInfo(clientToBeAdded->iCliSrcAddr, clientToBeAdded->iCliDstAddr, clientToBeAdded->iProtocolId, CSubConIPAddressInfoParamSet::TSubConIPAddressInfo::EAdd));
			__IPCPRLOG(IpCprLog::Printf(_L("SubConIPAddressParamInfo is added")));
			}
		else
			{
			// InitBundleL is already solving this
			User::Leave(KErrArgument);
			}

		// kick start
		ASSERT(iContext.iNodeActivity);
		iContext.iNodeActivity->PostRequestTo(iContext.NodeId(), TCFServiceProvider::TStart().CRef());

		__IPCPRLOG(IpCprLog::Printf(_L("TPrepareToAddClientToQosChannel::DoL [%08x], ActivityId=[%08x]"), this, iContext.iNodeActivity->ActivityId()));
		}

	}

DEFINE_SMELEMENT(TCreateAddressInfoBundle, NetStateMachine::MStateTransition, IpSCprStates::TContext)
void TCreateAddressInfoBundle::DoL()
	{
	RIPDataClientNodeInterface* dataClient =
	    static_cast<RIPDataClientNodeInterface*>(iContext.iPeer);

	ASSERT(dataClient);

	CSubConIPAddressInfoParamSet* IPAdressInfoParamSet = iContext.Node().InitBundleL();

	if(IPAdressInfoParamSet)
		{
		// add Param info
		IPAdressInfoParamSet->AddParamInfo(CSubConIPAddressInfoParamSet::TSubConIPAddressInfo(dataClient->iCliSrcAddr, dataClient->iCliDstAddr, dataClient->iProtocolId, CSubConIPAddressInfoParamSet::TSubConIPAddressInfo::EAdd));
		__IPCPRLOG(IpCprLog::Printf(_L("SubConIPAddressParamInfo is added")));
		}
	}

DEFINE_SMELEMENT(TCreateAddressInfoBundleFromJoiningClient, NetStateMachine::MStateTransition, IpSCprStates::TContext)
void TCreateAddressInfoBundleFromJoiningClient::DoL()
	{
	RIPDataClientNodeInterface* clientToBeAdded = static_cast<RIPDataClientNodeInterface*>(
	    iContext.Node().GetFirstClient<TDefaultClientMatchPolicy>(TClientType(TCFClientType::EData, TCFClientType::EJoining)));
	ASSERT(clientToBeAdded);

	CSubConIPAddressInfoParamSet* IPAdressInfoParamSet = iContext.Node().InitBundleL();

	if(IPAdressInfoParamSet)
		{
		// add Param info
		IPAdressInfoParamSet->AddParamInfo(CSubConIPAddressInfoParamSet::TSubConIPAddressInfo(clientToBeAdded->iCliSrcAddr, clientToBeAdded->iCliDstAddr, clientToBeAdded->iProtocolId, CSubConIPAddressInfoParamSet::TSubConIPAddressInfo::EAdd));
		__IPCPRLOG(IpCprLog::Printf(_L("SubConIPAddressParamInfo is added")));
		}
	}
