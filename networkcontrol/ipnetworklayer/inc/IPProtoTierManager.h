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
// IPProto Tier Manager
// 
//

/**
 @file
 @internalComponent
*/

#ifndef SYMBIAN_IPPROTO_TIER_MANAGER_H
#define SYMBIAN_IPPROTO_TIER_MANAGER_H

#include <comms-infras/coretiermanager.h>
#include <comms-infras/coretiermanagerstates.h>

class CIPProtoTierManager : public CCoreTierManager
	{
public:
	static CIPProtoTierManager* NewL(ESock::CTierManagerFactoryBase& aFactory);
	~CIPProtoTierManager();
	virtual ESock::MProviderSelector* DoCreateProviderSelectorL(const Meta::SMetaData& aSelectionPreferences);

protected:
	CIPProtoTierManager(ESock::CTierManagerFactoryBase& aFactory, const MeshMachine::TNodeActivityMap& aActivityMap);
	virtual void ReceivedL(const Messages::TRuntimeCtxId& aSender, const Messages::TNodeId& aRecipient, Messages::TSignatureBase& aMessage);
	};





namespace IpProtoTMStates
{

typedef MeshMachine::TNodeContext<CIPProtoTierManager,TMStates::TContext> TContext;

//-=========================================================
//
//Transitions
//
//-=========================================================

DECLARE_SMELEMENT_HEADER( TSelectProvider, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TSelectProvider )

DECLARE_SMELEMENT_HEADER( TSelectProviderConnPrefList, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TSelectProviderConnPrefList )

DECLARE_SMELEMENT_HEADER( TAbsorbQueryBundle, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, TContext )
	virtual void DoL();
DECLARE_SMELEMENT_FOOTER( TAbsorbQueryBundle )


DECLARE_AGGREGATED_TRANSITION2(
   TAbsorbQueryBundleAndReplyNotSupported,
   TAbsorbQueryBundle,
   MeshMachine::TRaiseError<KErrNotSupported>
   )

} //namespace IpProtoTMStates

#endif //SYMBIAN_IPPROTO_TIER_MANAGER_H

