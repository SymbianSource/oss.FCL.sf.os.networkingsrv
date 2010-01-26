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
*/


#ifndef SYMBIAN_DUMMYTIERMANAGER_H
#define SYMBIAN_DUMMYTIERMANAGER_H

#include <comms-infras/coretiermanager.h>
#include <comms-infras/coretiermanagerstates.h>

#include <elements/nm_signatures.h>

const TUid KUidConnPrefListTestingPubSub = {0x10285E24};

class CDummyTierManager : public CCoreTierManager
	{
public:
	static CDummyTierManager* NewL(ESock::CTierManagerFactoryBase& aFactory);
	~CDummyTierManager();

protected:
	CDummyTierManager(ESock::CTierManagerFactoryBase& aFactory, const MeshMachine::TNodeActivityMap& aActivityMap);
	virtual ESock::MProviderSelector* DoCreateProviderSelectorL(const Meta::SMetaData& aSelectionPreferences);
	virtual void ReceivedL(const Messages::TRuntimeCtxId& aSender, const Messages::TNodeId& aRecipient, Messages::TSignatureBase& aMessage);
	};

namespace DummyTMStates
	{

	typedef MeshMachine::TNodeContext<CDummyTierManager,TMStates::TContext> TContext;

	DECLARE_SMELEMENT_HEADER( TSelectionTest, MeshMachine::TStateTransition<TContext>, NetStateMachine::MStateTransition, TContext )
		virtual void DoL();
	DECLARE_SMELEMENT_FOOTER( TSelectionTest )
	}


#endif
// SYMBIAN_DUMMYTIERMANAGER_H

