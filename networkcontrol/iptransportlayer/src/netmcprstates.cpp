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
// THIS API IS INTERNAL TO NETWORKING AND IS SUBJECT TO CHANGE AND NOT FOR EXTERNAL USE
// 
//

/**
 @file
 @internalTechnology
 @prototype
*/

#define SYMBIAN_NETWORKING_UPS

#include "netmcprstates.h"
#ifndef SYMBIAN_NETWORKING_UPS
#include "IPMessages.h"
#endif


using namespace NetMCprStates;
using namespace ESock;
using namespace Messages;

#ifdef DUMMY_MOBILITY_MCPR


DEFINE_SMELEMENT(TAwaitingMigrationRejectedOrMigrationAccepted, NetStateMachine::MState, TContext);
TBool TAwaitingMigrationRejectedOrMigrationAccepted::Accept()
	{
	return(iContext.iMessage.IsMessage<TCFMobilityProvider::TMigrationAccepted>() ||
		   iContext.iMessage.IsMessage<TCFMobilityProvider::TMigrationRejected>());
	}

DEFINE_SMELEMENT(TAwaitingMigrationRequestedOrMigrationRejected, NetStateMachine::MState, TContext);
TBool TAwaitingMigrationRequestedOrMigrationRejected::Accept()
	{
	return(iContext.iMessage.IsMessage<TCFMobilityProvider::TMigrationRequested>() ||
		   iContext.iMessage.IsMessage<TCFMobilityProvider::TMigrationRejected>());
	}

DEFINE_SMELEMENT(TMigrationRequestedOrMigrationRejected, NetStateMachine::MStateFork, TContext);
TInt TMigrationRequestedOrMigrationRejected::TransitionTag()
	{
	if (iContext.iMessage.IsMessage<TCFMobilityProvider::TMigrationRequested>())
		return KMigrationRequested;
	else
		return KMigrationRejected;
	}

DEFINE_SMELEMENT(TSendMigrationAvailable, NetStateMachine::MStateTransition, TContext);
void TSendMigrationAvailable::DoL()
	{
	ASSERT(iContext.iMessage.iPeer);
	TCFMessage::TMigrationAvailable msg(iContext.Node()(), KActivityNull, 0xdeadbeef);
	msg.PostTo(iContext.iMessage.iSender);
	}

DEFINE_SMELEMENT(TSendMigrateToAccessPoint, NetStateMachine::MStateTransition, TContext);
void TSendMigrateToAccessPoint::DoL()
	{
	TCFMessage::TMigrateToAccessPoint msg(iContext.Node()(), KActivityNull, 0xdeadbeef);
	msg.PostTo(iContext.iMessage.iSender);
	}

DEFINE_SMELEMENT(TProcessPolicyParams, NetStateMachine::MStateTransition, TContext)
void TProcessPolicyParams::DoL()
    {
	iContext.Node().ProcessPolicyParamsL(message_cast<TCFIPMessage::TPolicyParams>(iContext.iMessage));
    }

#endif // DUMMY_MOBILITY_MCPR

DEFINE_SMELEMENT(TAwaitingPolicyParams, NetStateMachine::MState, TContext)
TBool TAwaitingPolicyParams::Accept()
    {
	return iContext.iMessage.IsMessage<TCFIPMessage::TPolicyParams>();
    }

#ifdef SYMBIAN_NETWORKING_UPS
DEFINE_SMELEMENT(TAwaitingUPSStatusChange, NetStateMachine::MState, TContext)
TBool TAwaitingUPSStatusChange::Accept()
	{
	return iContext.iMessage.IsMessage<UpsMessage::TUPSStatusChange>();
	}
#endif

DEFINE_SMELEMENT(TProcessPolicyParams, NetStateMachine::MStateTransition, TContext)
void TProcessPolicyParams::DoL()
    {
	TCFIPMessage::TPolicyParams& qosPolicyParams = message_cast<TCFIPMessage::TPolicyParams>(iContext.iMessage);

	iContext.Node().ProcessPolicyParamsL(iContext.iSender, qosPolicyParams);
    }

#ifdef SYMBIAN_NETWORKING_UPS

using namespace ESock;

//
// Support for User Prompt Service (UPS)
//

// UPS States

DEFINE_NETMCPR_STATE(NetMCprStates, TAwaitingNoBearerNotFromSelf);

TBool TAwaitingNoBearerNotFromSelf::Accept()
/**
Accept a TNoBearer which has not been self posted from this node.
*/
    {
	if (iContext.iMessage.IsMessage<TCFControlProvider	::TNoBearer>()) 
		{
		// Left hand side must be TRuntimeCtxId to invoke TRuntimeCtxId::operator==(...).
		// Also, there is no TRuntimeCtxId::operator!=(...).
		// @TODO EC120 - we want to compare node ids - is this safe??
		// Do we want to ensure that iContext.iSender is at least as derived as a TNodeId,
		// to avoid comparison of TRuntimeCtxId against TNodeId and not taking into account
		// the node id?
		return (!(iContext.iSender == iContext.Node().Id()));
		}
	else
		{
		return EFalse;
		}
    }

DEFINE_NETMCPR_STATE(NetMCprStates, TAwaitingBindToCompleteOrCancel);

TBool TAwaitingBindToCompleteOrCancel::Accept()
/**
Accept a TNoBearer which has not been self posted from this node.
*/
    {
    CoreNetStates::TAwaitingBindToComplete state(iContext);
    if (state.Accept())
    	{
    	return ETrue;
    	}
    else
	if (iContext.iMessage.IsMessage<TEBase::TCancel>()) 
		{
		iContext.iNodeActivity->SetError(KErrCancel);
		iContext.iMessage.ClearMessageId();		
		}
	return EFalse;
    }

// UPS Transitions

DEFINE_NETMCPR_TRANSITION(NetMCprStates, TSendNoBearerToSelf);

void TSendNoBearerToSelf::DoL()
/**
Self post a TNoBearer
*/
    {
    const TNodeId& self = iContext.Node().Id();
    
	iContext.Activity()->PostRequestTo(self, TCFControlProvider::TNoBearer().CRef());
    }

#endif // SYMBIAN_NETWORKING_UPS
