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
// IPTIERMANAGER_H.H
// 
//

/**
 @file
 @internalTechnology
 @prototype
*/

#ifndef SYMBIAN_IP_TIER_MANAGER_H
#define SYMBIAN_IP_TIER_MANAGER_H

#define SYMBIAN_NETWORKING_UPS

#include <comms-infras/coretiermanager.h>

#ifdef SYMBIAN_NETWORKING_UPS
namespace NetUps
	{
	class CNetUps;
	}
#endif

NONSHARABLE_CLASS(CIpTierManager) : public CCoreTierManager
	{
public:
	static CIpTierManager* NewL(ESock::CTierManagerFactoryBase& aFactory);
	~CIpTierManager();

protected:
	CIpTierManager(ESock::CTierManagerFactoryBase& aFactory, const MeshMachine::TNodeActivityMap& aActivityMap);
	virtual ESock::MProviderSelector* DoCreateProviderSelectorL(const Meta::SMetaData& aSelectionPreferences);
	virtual void ReceivedL(const Messages::TRuntimeCtxId& aSender, const Messages::TNodeId& aRecipient, Messages::TSignatureBase& aMessage);

#ifdef SYMBIAN_NETWORKING_UPS
private:
	void OpenNetUpsL();
	void CloseNetUps();

private:
	NetUps::CNetUps* iNetUps;		// Very first reference to NetUps
#endif
	};

#endif //SYMBIAN_IP_TIER_MANAGER_H

