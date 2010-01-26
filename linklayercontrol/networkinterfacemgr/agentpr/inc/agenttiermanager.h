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
 @prototype
*/


#ifndef SYMBIAN_AGENTTIERMANAGER_H
#define SYMBIAN_AGENTTIERMANAGER_H

#include <comms-infras/coretiermanager.h>

class CAgentTierManager : public CCoreTierManager
	{
public:
	static CAgentTierManager* NewL(ESock::CTierManagerFactoryBase& aFactory);
	~CAgentTierManager();

protected:
	CAgentTierManager(ESock::CTierManagerFactoryBase& aFactory, const MeshMachine::TNodeActivityMap& aActivityMap);
	virtual ESock::MProviderSelector* DoCreateProviderSelectorL(const Meta::SMetaData& aSelectionPreferences);
	virtual void ReceivedL(const Messages::TRuntimeCtxId& aSender, const Messages::TNodeId& aRecipient, Messages::TSignatureBase& aMessage);
	};

#endif
// SYMBIAN_AGENTTIERMANAGER_H


