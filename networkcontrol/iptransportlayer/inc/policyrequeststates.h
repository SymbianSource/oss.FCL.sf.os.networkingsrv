// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
 @internal
 @released
*/

#if !defined(POLICYREQUESTSTATES_H)
#define POLICYREQUESTSTATES_H

#include <comms-infras/ss_nodemessages.h>
#include <comms-infras/ss_coreprstates.h>
#include <comms-infras/ss_corepractivities.h>
#include "policyrequest.h"

namespace PolicyRequestStates
{

typedef MeshMachine::TNodeContext<CPolicyRequest, CoreStates::TContext> TContext;

//
//Default self generated error handling for request transitions
template<class TCONTEXT>
NONSHARABLE_CLASS(TPolicyRequestTransitionBase) : public MeshMachine::TStateTransition<TCONTEXT>
    {
public:
    explicit TPolicyRequestTransitionBase(TCONTEXT& aContext)
    :	MeshMachine::TStateTransition<TCONTEXT>(aContext) {}

	virtual void Error(TInt aError)
	    {
        this->iContext.iReturn = aError;
       	this->iContext.Node().iError = aError;
        MeshMachine::TStateTransition<TCONTEXT>::Error(aError);
        }
    };

//
// Default Cancel and Error handling for states of CESockClientActivityBase Activities
// Intended for ECAB'isation of an otherwise normal state
// Not intended to be used as the first ECAB state (no TransitionTag handling) which
// is never actually desired (specific Accept must be present for the first state of ECAB)
template<class TSTATE>
NONSHARABLE_CLASS(TPolicyRequestState) : public TSTATE
    {
public:
	NETSM_TPL_DECLARE_CTR(TPolicyRequestState, NetStateMachine::MState, TContext)

    explicit TPolicyRequestState(TContext& aContext)
    :	TSTATE(aContext)
        {
        }

	virtual TBool Accept()
    	{
    	Messages::TEBase::TError* msg = Messages::message_cast<Messages::TEBase::TError>(&this->iContext.iCFMessageSig);
    	if(msg)
	    	{
			// save error in activity so it will be forwarded on to originator when the node is destroyed.
	    	CPolicyRequest& fr = static_cast<CPolicyRequest&>(this->iContext.Node());
	    	fr.iError = msg->iValue;
	    	// Error handling has to clean-up node properly, including leaving the client nodes. Done through an
	    	// explicit error handling activity that we trigger by forwarding the message to ourself: we can't
	    	// let it get originated by a client because their subsequent leave will abort the activity
			Messages::TNodeId selfId(this->iContext.Node()());
	    	if(this->iContext.iSender != selfId)
	    		{
	    		Messages::RClientInterface::OpenPostMessageClose(selfId, selfId, *msg);
	    		}
    		this->iContext.iCFMessageSig.ClearMessageId();
    		return EFalse;
    		}
    	return TSTATE::Accept();
    	}

	virtual void Cancel()
        {
        // We're handling our own abort sequence
        this->iContext.iCFMessageSig.iReturn = KErrNone;
        }

    };

//
// States
//
DECLARE_SMELEMENT_HEADER( TAwaitingBinderResponse, MeshMachine::TState<TContext>, NetStateMachine::MState, TContext )
	virtual TBool Accept();
DECLARE_SMELEMENT_FOOTER( TAwaitingBinderResponse )

DECLARE_SMELEMENT_HEADER( TAwaitingConnPolicyRequest, MeshMachine::TState<TContext>, NetStateMachine::MState, TContext )
	virtual TBool Accept();
DECLARE_SMELEMENT_FOOTER( TAwaitingConnPolicyRequest )

DECLARE_SMELEMENT_HEADER( TAwaitingCancelError, MeshMachine::TState<TContext>, NetStateMachine::MState, TContext )
	virtual TBool Accept();
DECLARE_SMELEMENT_FOOTER( TAwaitingCancelError )

//
// Activities
//

DECLARE_SMELEMENT_HEADER( TSendQoSParamsToNewSCpr, PolicyRequestStates::TPolicyRequestTransitionBase<TContext>, NetStateMachine::MStateTransition, TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TSendQoSParamsToNewSCpr )

DECLARE_SMELEMENT_HEADER( TJoinReceivedSCpr, PolicyRequestStates::TPolicyRequestTransitionBase<TContext>, NetStateMachine::MStateTransition, TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TJoinReceivedSCpr )

DECLARE_SMELEMENT_HEADER( TRequestCommsBinderFromSCpr, PolicyRequestStates::TPolicyRequestTransitionBase<TContext>, NetStateMachine::MStateTransition, TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TRequestCommsBinderFromSCpr )

DECLARE_SMELEMENT_HEADER( TAwaitingError, MeshMachine::TState<TContext>, NetStateMachine::MState, TContext )
	virtual TBool Accept();
DECLARE_SMELEMENT_FOOTER( TAwaitingError )

DECLARE_SMELEMENT_HEADER( TJoinCpr, PolicyRequestStates::TPolicyRequestTransitionBase<TContext>, NetStateMachine::MStateTransition, TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TJoinCpr )

DECLARE_SMELEMENT_HEADER( TLeaveCpr, PolicyRequestStates::TPolicyRequestTransitionBase<TContext>, NetStateMachine::MStateTransition, TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TLeaveCpr )

DECLARE_SMELEMENT_HEADER( TSendBindToComplete, PolicyRequestStates::TPolicyRequestTransitionBase<TContext>, NetStateMachine::MStateTransition, TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TSendBindToComplete )


//-=========================================================
//Rejoin
//-=========================================================
DECLARE_SMELEMENT_HEADER( TJoinTheDeftSCPr, PolicyRequestStates::TPolicyRequestTransitionBase<TContext>, NetStateMachine::MStateTransition, TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TJoinTheDeftSCPr )

DECLARE_SMELEMENT_HEADER( TLeaveTheDeftSCPr, PolicyRequestStates::TPolicyRequestTransitionBase<TContext>, NetStateMachine::MStateTransition, TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TLeaveTheDeftSCPr )

DECLARE_SMELEMENT_HEADER( TSendRejoinDataClientRequestToDeftSCPr, PolicyRequestStates::TPolicyRequestTransitionBase<TContext>, NetStateMachine::MStateTransition, TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TSendRejoinDataClientRequestToDeftSCPr )

DECLARE_SMELEMENT_HEADER( TSendApplyToDeftSCPr, PolicyRequestStates::TPolicyRequestTransitionBase<TContext>, NetStateMachine::MStateTransition, TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TSendApplyToDeftSCPr )

DECLARE_SMELEMENT_HEADER( TSendApplyToNewSCPr, PolicyRequestStates::TPolicyRequestTransitionBase<TContext>, NetStateMachine::MStateTransition, TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TSendApplyToNewSCPr )

DECLARE_SMELEMENT_HEADER( TSendCancelToDeftSCPr, PolicyRequestStates::TPolicyRequestTransitionBase<TContext>, NetStateMachine::MStateTransition, TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TSendCancelToDeftSCPr )

DECLARE_SMELEMENT_HEADER( TIgnoreAndCloseSubConEvent, PolicyRequestStates::TPolicyRequestTransitionBase<TContext>, NetStateMachine::MStateTransition, TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TIgnoreAndCloseSubConEvent )

//
//SCpr specific
/*DECLARE_SMELEMENT_NAMESPACE_HEADER( TJoinSCpr, PolicyRequestStates, PolicyRequestTransitionBase<TContext>, NetStateMachine::MStateTransition, TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TJoinSCpr )*/

} //namespace PolicyRequestStates


namespace PolicyRequestActivities
{

NONSHARABLE_CLASS(CPolicyRequestActivity) : public MeshMachine::CNodeActivityBase, public CoreActivities::ABindingActivity,
											public ITFHIERARCHY_1(CPolicyRequestActivity, CoreActivities::ABindingActivity)
	{
public:
	typedef ITFHIERARCHY_1(CPolicyRequestActivity, CoreActivities::ABindingActivity) TIfStaticFetcherNearestInHierarchy;

public:
    static MeshMachine::CNodeActivityBase* NewL(const MeshMachine::TNodeActivity& aActivitySig, MeshMachine::AMMNodeBase& aNode)
    	{
		return new (ELeave) CPolicyRequestActivity(aActivitySig,aNode,static_cast<const CPolicyRequest&>(aNode));
    	}

	void ReturnInterfacePtrL(CoreActivities::ABindingActivity*& aInterface)
		{
		aInterface = this;
		}

protected:
	CPolicyRequestActivity(const MeshMachine::TNodeActivity& aActivitySig, MeshMachine::AMMNodeBase& aNode, const CPolicyRequest& aPolicyRequest)
	:MeshMachine::CNodeActivityBase(aActivitySig, aNode),
	 ABindingActivity(aNode.Id()),
	 TIfStaticFetcherNearestInHierarchy(this),
	 iPolicyRequest(&aPolicyRequest)
		{
		}

	virtual ~CPolicyRequestActivity()
		{
		ASSERT(iPolicyRequest);
		if (iPolicyRequest->CountAllActivities()==0)
			{
			delete iPolicyRequest;
			}
		SetError(KErrNone);	// any bad news we're carrying gets passed to the client through the request
		}

private:
	const CPolicyRequest* iPolicyRequest;
	};

} //namespace PolicyRequestActivities


#endif
// QOSPOLICYREQUESTSTATES_H

