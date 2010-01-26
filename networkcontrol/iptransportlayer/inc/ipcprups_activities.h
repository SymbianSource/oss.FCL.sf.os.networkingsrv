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
// IP Connection Provider UPS activity declarations.
// 
//

/**
 @file
 @internalComponent
*/
 
#ifndef IPCPRUPS_ACTIVITIES_H_INCLUDED
#define IPCPRUPS_ACTIVITIES_H_INCLUDED

#define SYMBIAN_NETWORKING_UPS

#ifdef SYMBIAN_NETWORKING_UPS
 
#include <comms-infras/ss_mobility_apiext.h>
#include <comms-infras/corecprstates.h>
#include <comms-infras/upscpractivities.h>

namespace IpCprActivities
{

//
// Support for User Prompt Service (UPS)
//

NONSHARABLE_CLASS(CIpCprUpsCtrlClientJoinActivity) : public UpsCprActivities::CDeferredCtrlClientJoinActivity
/**
Activity base class for performing UPS checking on a control client join.

Enforces the fact that transition SendPolicyCheckRequest must execute in the context of
CReversibleCtrlClientJoinActivity rather than being a standalone function, mainly because of
the cleanup operations performed by the latter on destruction.

This class is also intended to be node agnostic, with pure virtual GetPlatSecResultAndDestinationL()
called to obtain the platform security check result and destination address.

This class could conceivably be moved into the core.
*/
	{
public:
	static const TInt KUpsErrorTag = 10001;		// how are these allocated?

	// CMMCommsProviderBase to make this activity node agnostic (in case it should be moved into core).
	typedef MeshMachine::TNodeContext<ESock::CMMCommsProviderBase, PRStates::TContext, CIpCprUpsCtrlClientJoinActivity> TContext;

	// Transitions
	DECLARE_SMELEMENT_HEADER(TSendPolicyCheckRequest, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, TContext)
		virtual void DoL();
	DECLARE_SMELEMENT_FOOTER(TSendPolicyCheckRequest)

	// State Forks
	DECLARE_SMELEMENT_HEADER( TNoTagOrUpsErrorTag, MeshMachine::TStateFork<TContext>, NetStateMachine::MStateFork, TContext )
		virtual TInt TransitionTag();
	DECLARE_SMELEMENT_FOOTER( TNoTagOrUpsErrorTag )

protected:
    CIpCprUpsCtrlClientJoinActivity(const MeshMachine::TNodeActivity& aActivitySig, MeshMachine::AMMNodeBase& aNode, TUint aNextActivityCount);

private:
	virtual void GetPlatSecResultAndDestinationL(const ESock::MPlatsecApiExt& aPlatSecApi, TInt& aPlatSecResult, TUpsDestinationAddrType& aDestinationType, TDes8& aDestination) const = 0;
	};


NONSHARABLE_CLASS(CIpCprCtrlClientJoinActivity) : public CIpCprUpsCtrlClientJoinActivity
/**
Activity used to perform UPS authorisation on an RConnection::Attach() - i.e. on a control client join
towards IPCpr.
*/
	{
public:
    static MeshMachine::CNodeActivityBase* NewL(const MeshMachine::TNodeActivity& aActivitySig, MeshMachine::AMMNodeBase& aNode);

protected:
    CIpCprCtrlClientJoinActivity(const MeshMachine::TNodeActivity& aActivitySig, MeshMachine::AMMNodeBase& aNode, TUint aNextActivityCount);

private:
	void GetPlatSecResultAndDestinationL(const ESock::MPlatsecApiExt& aPlatSecApi, TInt& aPlatSecResult, TUpsDestinationAddrType& aDestinationType, TDes8& aDestination) const;
	};

}

#endif //SYMBIAN_NETWORKING_UPS

#endif // IPCPRUPS_ACTIVITIES_H_INCLUDED
