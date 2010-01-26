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

#ifndef NETMCPRUPS_ACTIVITIES_H_INCLUDED
#define NETMCPRUPS_ACTIVITIES_H_INCLUDED

#define SYMBIAN_NETWORKING_UPS

#ifdef SYMBIAN_NETWORKING_UPS

#include <comms-infras/upsmcpractivities.h>

namespace NetMCprUpsActivities 
/**
Support activities for User Prompt Service (UPS)
*/
{

NONSHARABLE_CLASS(CUpsAuthorisationActivity) : public UpsMCprActivities::CUpsAuthorisationActivityBase
/**
Base class used to implement IP technology activities requiring UPS authorisation.

Mainly used to implement the GetUpsDestinationString() pure virtual which CUpsAuthorisationActivityBase
calls to form the UPS destination string from an IPv4/IPv6 address.  Can also be used as an activity in
its own right (see CUpsAuthorisationActivityBase - where all the relevant functionality is derived from).
*/
	{
public:
    static MeshMachine::CNodeActivityBase* NewL(const MeshMachine::TNodeActivity& aActivitySig, MeshMachine::AMMNodeBase& aNode);

	// from CUpsAuthorisationActivityBase
	NetUps::CNetUps* GetNetUpsL(TInt32 aServiceId);
	TInt GetUpsDestinationString(TUpsDestinationAddrType aDestinationType, const TUpsDestinationAddr& aDestination, TDes& aOutputString);
	const TDesC& GetUpsAccessPointNameL();

protected:
	void PerformPolicyCheckRequestActionsL(const Messages::TNodeId& aCommsId);
    void PerformPolicyCheckResponseActions(NetUps::TNetUpsDecision aNetUpsDecision, const Messages::TNodeId& aCommsId);
    CUpsAuthorisationActivity(const MeshMachine::TNodeActivity& aActivitySig, MeshMachine::AMMNodeBase& aNode, TUint aNextActivityCount);
	};

NONSHARABLE_CLASS(CUpsNoBearer) : public CUpsAuthorisationActivity
/**
Class used to implement IP technology NoBearer activity with UPS authorisation.

Intended to be used in conjunction with the existing core NoBearer activity (MCprNoBearer).
Contains methods to forward BindTo and BindToComplete messages between the originator and
MCprNoBearer.  Also contains the id of the MCprNoBearer activity that sent the BindTo message,
which is required in order to send the BindToComplete message back to the correct activity in
MCprNoBearer.
*/
	{
public:
	// Note: TContext used to be CUpsAuthorisationActivityBase::TContext, but it was made more
	// specialised to allow NoTagOrInvokeUps to access CNetworkMetaConnectionProvider::UpsDisabled().
	typedef MeshMachine::TNodeContext<CUpsNetworkMetaConnectionProvider, CUpsAuthorisationActivityBase::TContext> TContext;

	static const TInt KInvokeUps = 10000;			// tuple tag (how are these allocated?)

    static MeshMachine::CNodeActivityBase* NewL(const MeshMachine::TNodeActivity& aActivitySig, MeshMachine::AMMNodeBase& aNode);

	// Utility
	
	inline void  SetBindToOriginatorActivityId(TUint aActivityId);
	inline TUint BindToOriginatorActivityId() const;

	// Transitions

	DECLARE_SMELEMENT_HEADER(TForwardBindToToOriginators, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, TContext)
		virtual void DoL();
	DECLARE_SMELEMENT_FOOTER(TForwardBindToToOriginators)

	DECLARE_SMELEMENT_HEADER(TForwardBearerToOriginators, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, TContext)
		virtual void DoL();
	DECLARE_SMELEMENT_FOOTER(TForwardBearerToOriginators)

	DECLARE_SMELEMENT_HEADER(TForwardBindToCompleteToSelf, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, TContext)
		virtual void DoL();
	DECLARE_SMELEMENT_FOOTER(TForwardBindToCompleteToSelf)

	// State Forks

	DECLARE_SMELEMENT_HEADER(TNoTagOrInvokeUps, MeshMachine::TStateFork<TContext>, NetStateMachine::MStateFork, TContext)
		virtual TInt TransitionTag();
	DECLARE_SMELEMENT_FOOTER(TNoTagOrInvokeUps)
		
protected:
    CUpsNoBearer(const MeshMachine::TNodeActivity& aActivitySig, MeshMachine::AMMNodeBase& aNode, TUint aNextActivityCount);

private:
	TUint iBindToOriginatorActivityId;	// activity id of MCprNoBearer activity
	};

inline void CUpsNoBearer::SetBindToOriginatorActivityId(TUint aActivityId)
	{ iBindToOriginatorActivityId = aActivityId; }

inline TUint CUpsNoBearer::BindToOriginatorActivityId() const
	{ return iBindToOriginatorActivityId; }

} //NetMCprUpsActivities

namespace NetMCprPolicyCheckRequestActivity
	{
	DECLARE_NODEACTIVITY(NetMCprPolicyCheckRequest)
	}

namespace NetMCprUpsNoBearerActivity
	{
	DECLARE_NODEACTIVITY(NetMCprUpsNoBearer)
	}

namespace NetMCprMonitorProviderStatusActivity
	{
	DECLARE_NODEACTIVITY(NetMCprUpsProviderStatusChange)
	}

namespace NetMCprUpsStatusChangeActivity
	{
	DECLARE_NODEACTIVITY(NetMCprUpsStatusChange)
	}

#endif //SYMBIAN_NETWORKING_UPS

#endif // NETMCPRUPS_ACTIVITIES_H_INCLUDED
