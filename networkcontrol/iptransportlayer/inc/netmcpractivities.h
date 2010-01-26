// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef NETMCPR_ACTIVITIES_H_INCLUDED
#define NETMCPR_ACTIVITIES_H_INCLUDED

#define SYMBIAN_NETWORKING_UPS

#include <comms-infras/mobilitymcpractivities.h>

#ifdef SYMBIAN_NETWORKING_UPS
#include <comms-infras/upsmcpractivities.h>
#endif //SYMBIAN_NETWORKING_UPS

class CNetworkMetaConnectionProvider;

namespace NetMCprActivities
{

#ifdef DUMMY_MOBILITY_MCPR
// [TODO] IK remove me when we have a proper mobility mcpr
enum {
ECFActivityDummyMobilityActivity = ESock::ECFActivityCustom

#ifdef SYMBIAN_NETWORKING_UPS
, ECFActivityUpsNoBearer, ECFActivityUpsProviderStatus
#endif //SYMBIAN_NETWORKING_UPS

};

#endif //DUMMY_MOBILITY_MCPR

#ifdef SYMBIAN_NETWORKING_UPS

#ifndef DUMMY_MOBILITY_MCPR
enum {
ECFActivityUpsNoBearer = ESock::ECFActivityCustom, ECFActivityUpsProviderStatus, ECFActivityUpsStatusChange
};
#endif //DUMMY_MOBILITY_MCPR

#endif //SYMBIAN_NETWORKING_UPS

DECLARE_ACTIVITY_MAP(netMCprActivities)
}	// namespace NetMCprActivities

namespace NetMCprLegacyActivities
{

NONSHARABLE_CLASS(CDeferredSelectionActivity) : public MeshMachine::CNodeActivityBase,
												protected MeshMachine::AContextStore
	{
public:
    static MeshMachine::CNodeActivityBase* NewL(const MeshMachine::TNodeActivity& aActivitySig, MeshMachine::AMMNodeBase& aNode);
    void ParkReConnectRequestL(const MeshMachine::TNodeContextBase& aContext);
    void ReDispatchReConnectRequestL(const MeshMachine::TNodeContextBase& aContext);
    void ReplyToOriginators(Messages::TEErrorRecovery::TErrorRecoveryResponse& aCFMessageSig);
    ~CDeferredSelectionActivity();

protected:
    CDeferredSelectionActivity(const MeshMachine::TNodeActivity& aActivitySig, MeshMachine::AMMNodeBase& aNode);

public:
	Messages::TErrContext iOriginalErrContext; //Error context on which this activity started
	Messages::TNodeId iSelectedMcpr;
	Messages::TNodeId iTierManager;

protected:
	typedef MeshMachine::TNodeContext<CCoreMetaConnectionProvider, MCprStates::TContext> TContext;

public:

	//
	// Default Error handling for mcpr states of CReConnectionActivity
	template<class TSTATE, class TCONTEXT = TContext>
	NONSHARABLE_CLASS(TState) : public TSTATE
	    {
	public:
		NETSM_TPL_DECLARE_CTR(TState, NetStateMachine::MState, TCONTEXT)

	    explicit TState(TCONTEXT& aContext)
	    :	TSTATE(aContext)
	        {
	        }

		virtual TBool Accept()
	    	{
		Messages::TEBase::TError* msg = Messages::message_cast<Messages::TEBase::TError>(&this->iContext.iMessage);
	    	if(msg && this->iContext.iNodeActivity != NULL)
		    	{
				CDeferredSelectionActivity& ac = static_cast<CDeferredSelectionActivity&>(*this->iContext.iNodeActivity);
				Messages::TEErrorRecovery::TErrorRecoveryResponse errResp(Messages::TErrResponse(Messages::TErrResponse::EPropagate,ac.iOriginalErrContext.iStateChange.iError,ac.iOriginalErrContext.iMessageId));
				ac.ReplyToOriginators(errResp);
		    	ac.SetIdle();
	    		this->iContext.iMessage.ClearMessageId();
	    		return EFalse;
	    		}
	    	return TSTATE::Accept();
	    	}
	    };

	DECLARE_SMELEMENT_HEADER( TAwaitingConnectionStartRecoveryRequest, MCprStates::TAwaitingConnectionStartRecoveryRequest, NetStateMachine::MState, CDeferredSelectionActivity::TContext )
		virtual TBool Accept();
	DECLARE_SMELEMENT_FOOTER( TAwaitingConnectionStartRecoveryRequest )

	DECLARE_SMELEMENT_HEADER( TParkReConnectRequestAndFindOrCreateTierManager, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, CDeferredSelectionActivity::TContext )
		virtual void DoL();
	DECLARE_SMELEMENT_FOOTER( TParkReConnectRequestAndFindOrCreateTierManager )

	DECLARE_SMELEMENT_HEADER( TCompleteDeferredSelection, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, CDeferredSelectionActivity::TContext )
		virtual void DoL();
	DECLARE_SMELEMENT_FOOTER( TCompleteDeferredSelection )

	DECLARE_SMELEMENT_HEADER( TProcessSelectComplete, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, CDeferredSelectionActivity::TContext )
		virtual void DoL();
	DECLARE_SMELEMENT_FOOTER( TProcessSelectComplete )

	DECLARE_SMELEMENT_HEADER( TJoinServiceProvider, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, CDeferredSelectionActivity::TContext )
		virtual void DoL();
	DECLARE_SMELEMENT_FOOTER( TJoinServiceProvider )

	DECLARE_SMELEMENT_HEADER( TReDispatchReConnectRequest, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, CDeferredSelectionActivity::TContext )
		virtual void DoL();
	DECLARE_SMELEMENT_FOOTER( TReDispatchReConnectRequest )

	DECLARE_SMELEMENT_HEADER( TJoinTierManager, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, CDeferredSelectionActivity::TContext )
		virtual void DoL();
	DECLARE_SMELEMENT_FOOTER( TJoinTierManager )
	};

} //NetMCprLegacyActivities

namespace NetMCprActivities
{
NONSHARABLE_CLASS(CPromptingReSelectActivity) : public MeshMachine::CNodeActivityBase,
                                   				protected MeshMachine::AContextStore
	{
public:
    static MeshMachine::CNodeActivityBase* NewL(const MeshMachine::TNodeActivity& aActivitySig, MeshMachine::AMMNodeBase& aNode);
    void ParkReConnectRequestL(const MeshMachine::TNodeContextBase& aContext);
    void ReDispatchReConnectRequestL(const MeshMachine::TNodeContextBase& aContext);
    void ReplyToOriginators(Messages::TEErrorRecovery::TErrorRecoveryResponse& aCFMessageSig);
    ~CPromptingReSelectActivity();

protected:
	CPromptingReSelectActivity(const MeshMachine::TNodeActivity& aActivitySig, MeshMachine::AMMNodeBase& aNode);

public:
	Messages::TErrContext iOriginalErrContext; //Error context on which this activity started
	Messages::TNodeId iSelectedMcpr;
	Messages::TNodeId iTierManager;

protected:
	typedef MeshMachine::TNodeContext<CCoreMetaConnectionProvider, MCprStates::TContext> TContext;

public:

	//
	// Default Error handling for mcpr states of CReConnectionActivity
	template<class TSTATE, class TCONTEXT = TContext>
	class TState : public TSTATE
	    {
	public:
		NETSM_TPL_DECLARE_CTR(TState, NetStateMachine::MState, TCONTEXT)

	    explicit TState(TCONTEXT& aContext)
	    :	TSTATE(aContext)
	        {
	        }

		virtual TBool Accept()
	    	{
	    Messages::TEBase::TError* msg = Messages::message_cast<Messages::TEBase::TError>(&this->iContext.iMessage);
	    	if(msg && this->iContext.iNodeActivity != NULL)
		    	{
		    	CPromptingReSelectActivity& ac = static_cast<CPromptingReSelectActivity&>(*this->iContext.iNodeActivity);
				Messages::TEErrorRecovery::TErrorRecoveryResponse errResp(Messages::TErrResponse(Messages::TErrResponse::EPropagate,ac.iOriginalErrContext.iStateChange.iError,ac.iOriginalErrContext.iMessageId));
				ac.ReplyToOriginators(errResp);
		    	ac.SetIdle();
	    		this->iContext.iMessage.ClearMessageId();
	    		return EFalse;
	    		}
	    	return TSTATE::Accept();
	    	}
	    };

	DECLARE_SMELEMENT_HEADER( TAwaitingConnectionStartRecoveryRequest, MCprStates::TAwaitingConnectionStartRecoveryRequest, NetStateMachine::MState, CPromptingReSelectActivity::TContext )
		virtual TBool Accept();
	DECLARE_SMELEMENT_FOOTER( TAwaitingConnectionStartRecoveryRequest )

	DECLARE_SMELEMENT_HEADER( TParkReConnectRequestAndFindOrCreateTierManager, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, CPromptingReSelectActivity::TContext )
		virtual void DoL();
	DECLARE_SMELEMENT_FOOTER( TParkReConnectRequestAndFindOrCreateTierManager )

	DECLARE_SMELEMENT_HEADER( TCompletePromptingReSelection, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, CPromptingReSelectActivity::TContext )
		virtual void DoL();
	DECLARE_SMELEMENT_FOOTER( TCompletePromptingReSelection )

	DECLARE_SMELEMENT_HEADER( TProcessSelectComplete, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, CPromptingReSelectActivity::TContext )
		virtual void DoL();
	DECLARE_SMELEMENT_FOOTER( TProcessSelectComplete )

	DECLARE_SMELEMENT_HEADER( TJoinServiceProvider, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, CPromptingReSelectActivity::TContext )
		virtual void DoL();
	DECLARE_SMELEMENT_FOOTER( TJoinServiceProvider )

	DECLARE_SMELEMENT_HEADER( TReDispatchReConnectRequest, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, CPromptingReSelectActivity::TContext )
		virtual void DoL();
	DECLARE_SMELEMENT_FOOTER( TReDispatchReConnectRequest )

	DECLARE_SMELEMENT_HEADER( TJoinTierManager, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, CPromptingReSelectActivity::TContext )
		virtual void DoL();
	DECLARE_SMELEMENT_FOOTER( TJoinTierManager )
	};

} //NetMCprActivities

#endif // NETMCPR_ACTIVITIES_H_INCLUDED
