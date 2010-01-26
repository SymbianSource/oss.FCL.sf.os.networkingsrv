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
#include "IPSCPRStates.h"
#include <elements/nm_messages_peer.h>

#include "../../iptransportlayer/src/ipscprlog.h"

using namespace Messages;
using namespace ESock;
using namespace NetStateMachine;
using namespace QoSIpSCprStates;
//-=========================================================
//
// Activities
//
//-=========================================================
DEFINE_SMELEMENT(TStoreAddressUpdateAndAddClientToQoSChannel, NetStateMachine::MStateTransition, QoSIpSCprStates::TContext)
void TStoreAddressUpdateAndAddClientToQoSChannel::DoL()
	{
	__IPCPRLOG(IpCprLog::Printf(_L("TStoreAddressUpdateAndAddClientToQoSChannel::DoL [%08x]"), this));
   IPBaseSCprStates::TStoreAddressUpdate storeAddressUpdate(iContext);
   storeAddressUpdate.DoL();
   
   RIPDataClientNodeInterface* client = 
   	    static_cast<RIPDataClientNodeInterface*>(iContext.iPeer);        
   
   if (client && !(client->Flags() & TCFClientType::EJoining))
       {
       client->SetFlags(TCFClientType::EJoining);
       QoSIpSCprStates::TAddClientToQoSChannel addToChannel(iContext);
       addToChannel.DoL();
       client->ClearFlags(TCFClientType::EJoining);
       }
   }


DEFINE_SMELEMENT(TAddClientToQoSChannel, NetStateMachine::MStateTransition, QoSIpSCprStates::TContext)
void TAddClientToQoSChannel::DoL()
	{
	__IPCPRLOG(IpCprLog::Printf(_L("TAddClientToQoSChannel::DoL [%08x]"), this));
	RIPDataClientNodeInterface* clientToBeAdded = static_cast<RIPDataClientNodeInterface*>(
	    iContext.Node().GetFirstClient<TDefaultClientMatchPolicy>(TClientType(TCFClientType::EData, TCFClientType::EJoining)));
    ASSERT(clientToBeAdded);
    
    if (clientToBeAdded)
        {
        iContext.Node().DataClientJoiningL(*clientToBeAdded);
        if (iContext.iNodeActivity)
            {
        	clientToBeAdded->iActivityAwaitingResponse = iContext.iNodeActivity->ActivityId();
            }
    	clientToBeAdded->ClearFlags(TCFClientType::EJoining);        
        }
	}
	
DEFINE_SMELEMENT(TRemoveClientToQoSChannel, NetStateMachine::MStateTransition, QoSIpSCprStates::TContext)
void TRemoveClientToQoSChannel::DoL()
	{
	RIPDataClientNodeInterface* clientToBeRemoved = static_cast<RIPDataClientNodeInterface*>(
	    iContext.Node().GetFirstClient<TDefaultClientMatchPolicy>(TClientType(TCFClientType::EData, TClientType::ELeaving)));
    ASSERT(clientToBeRemoved);
    
    if (clientToBeRemoved)
        {
        iContext.Node().DataClientLeaving(*clientToBeRemoved);
        if (iContext.iNodeActivity)
            {
        	clientToBeRemoved->iActivityAwaitingResponse = iContext.iNodeActivity->ActivityId();
            }
    	clientToBeRemoved->ClearFlags(TClientType::ELeaving);
        }
	}
	
DEFINE_SMELEMENT(TRemoveLeavingClientFromQoSChannel, NetStateMachine::MStateTransition, QoSIpSCprStates::TContext)
void TRemoveLeavingClientFromQoSChannel::DoL()
    {    
	__IPCPRLOG(IpCprLog::Printf(_L("TRemoveClientFromQoSChannel::DoL [%08x]"), this));
    if (iContext.iPeer && 
        iContext.iPeer->Type() & TCFClientType::EData &&
        !iContext.iPeer->Flags() & TCFClientType::EJoining)
        {
    	RIPDataClientNodeInterface& client = 
    	    static_cast<RIPDataClientNodeInterface&>(*iContext.iPeer);
        iContext.Node().DataClientLeaving(static_cast<RIPDataClientNodeInterface&>(client));
        }
    PRStates::TProcessClientLeave processClientLeave(iContext);
    processClientLeave.DoL();
    }
	
DEFINE_SMELEMENT(TSetParameters, NetStateMachine::MStateTransition, QoSIpSCprStates::TContext)
void TSetParameters::DoL()
    {
	__IPCPRLOG(IpCprLog::Printf(_L("TSetParameters::DoL [%08x]"), this));
	iContext.Node().SetQoSParametersL();
#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	iContext.PostToSender(TCFScpr::TSetParamsResponse(iContext.Node().iParameterBundle).CRef());
#else
	iContext.PostToSender(TCFScpr::TParamsResponse(iContext.Node().iParameterBundle).CRef());
#endif // SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
    }

DEFINE_SMELEMENT(TOpenInternalSocket, NetStateMachine::MStateTransition, QoSIpSCprStates::TContext)
void TOpenInternalSocket::DoL()
    {
	__IPCPRLOG(IpCprLog::Printf(_L("TOpenInternalSocket::DoL [%08x]"), this));

	TNodeCtxId originator(iContext.iNodeActivity->ActivityId(), iContext.NodeId());
	CQoSSocketOpener* opener = CQoSSocketOpener::NewL(iContext.Node(), originator); // cleans up itself when RunL runs
	opener->Open();
    }

//-=========================================================
//
//States
//
//-=========================================================
DEFINE_SMELEMENT(TAwaitingJoinComplete, NetStateMachine::MState, QoSIpSCprStates::TContext)
TBool TAwaitingJoinComplete::Accept()
    {
	__IPCPRLOG(IpCprLog::Printf(_L("TAwaitingJoinComplete::Accept [%08x]"), this));
	return iContext.iMessage.IsMessage<TCFPeer::TJoinComplete>();
    }
    
DEFINE_SMELEMENT(TAwaitingLeaveComplete, NetStateMachine::MState, QoSIpSCprStates::TContext)
TBool TAwaitingLeaveComplete::Accept()
    {
	return iContext.iMessage.IsMessage<TEPeer::TLeaveComplete>();
    }    

//-=========================================================
//
//State Forks
//
//-=========================================================

DEFINE_SMELEMENT(TNoTagOrSendApplyResponse, NetStateMachine::MStateFork, QoSIpSCprStates::TContext)
TInt TNoTagOrSendApplyResponse::TransitionTag()
	{
	__IPCPRLOG(IpCprLog::Printf(_L("TNoTagOrSendApplyResponse::TransitionTag [%08x]"), this));
	RNodeInterface* joiningClient = iContext.Node().GetFirstClient<TDefaultClientMatchPolicy>(TClientType(TCFClientType::EData, TCFClientType::EJoining));
	RNodeInterface* leavingClient = iContext.Node().GetFirstClient<TDefaultClientMatchPolicy>(TClientType(TCFClientType::EData, TClientType::ELeaving));
	ASSERT(!(joiningClient && leavingClient) );
	
	if (joiningClient)
    	{
    	return SCprStates::KClientsJoining;
    	}
    else if (leavingClient)
    	{
    	return SCprStates::KClientsLeaving;
    	}
    return MeshMachine::KNoTag;
	}

//-=========================================================
//
//Transitions
//
//-=========================================================


