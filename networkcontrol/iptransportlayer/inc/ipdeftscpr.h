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
// IP Default SubConnection Provider class definition.
// 
//

/**
 @file
 @internalComponent
*/

#ifndef SYMBIAN_IPDEFTSCPR_H
#define SYMBIAN_IPDEFTSCPR_H

#include <comms-infras/ss_subconnprov.h>
#include <comms-infras/corescpr.h>
#include <comms-infras/corescprstates.h>
#include <comms-infras/ss_nodeinterfaces.h>
#include <comms-infras/ss_corepractivities.h>
#include "ipdeftbasescpr.h"

NONSHARABLE_CLASS(CIpDefaultSubConnectionProvider) : public CIpDefaultBaseSubConnectionProvider
/** Default QoS IP subconnection provider

@internalTechnology
@released Since 9.4 */
    {
    friend class CIpSubConnectionProviderFactory;
public:
    typedef CIpDefaultSubConnectionProviderFactory FactoryType;
	//TBool ImsFlag();

protected:
    CIpDefaultSubConnectionProvider(ESock::CSubConnectionProviderFactoryBase& aFactory);
    static CIpDefaultSubConnectionProvider* NewL(ESock::CSubConnectionProviderFactoryBase& aFactory);

    virtual ~CIpDefaultSubConnectionProvider();
    virtual void ReceivedL(const Messages::TRuntimeCtxId& aSender, const Messages::TNodeId& aRecipient, Messages::TSignatureBase& aMessage);
    };

NONSHARABLE_CLASS(CCommsIPBinderActivity) : public PRActivities::CCommsBinderActivity
/** Specialisation of CCommsBinderActivity to inlcude an extra param

@internalTechnology
@released Since 9.5 */

	{
public:
	typedef PRStates::TContext TContext;

	DECLARE_SMELEMENT_HEADER( TFetchClientUids, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, TContext )
		virtual void DoL();
	DECLARE_SMELEMENT_FOOTER( TFetchClientUids)

	DECLARE_SMELEMENT_HEADER( TUpdateClientUids, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, TContext )
		virtual void DoL();
	DECLARE_SMELEMENT_FOOTER( TUpdateClientUids)

	DECLARE_AGGREGATED_TRANSITION2(
		TProcessDataClientCreationAndUpdateClientUids,
		CCommsBinderActivity::TProcessDataClientCreation,
		CCommsIPBinderActivity::TUpdateClientUids
	)

	static MeshMachine::CNodeActivityBase* NewL( const MeshMachine::TNodeActivity& aActivitySig, MeshMachine::AMMNodeBase& aNode );

protected:
	CCommsIPBinderActivity(const MeshMachine::TNodeActivity& aActivitySig, MeshMachine::AMMNodeBase& aNode, TUint aNextActivityCount);

	inline TUid GetUid(){ return iAppSid; }
	inline void SetUid(TUid aValue) { iAppSid = aValue; }

private:
	TUid iAppSid;
	};

//-=========================================================
//
// States
//
//-=========================================================
namespace IPDeftSCprStates
{
typedef MeshMachine::TNodeContext<CIpDefaultSubConnectionProvider, SCprStates::TContext> TContext;

DECLARE_SMELEMENT_HEADER( TPolicyChecking, MeshMachine::TStateTransition<IPBaseSCprStates::TContext>, NetStateMachine::MStateTransition, IPBaseSCprStates::TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TPolicyChecking )
}

#endif //SYMBIAN_IPDEFTSCPR_H
