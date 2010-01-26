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
// Ethernet MCPR
// 
//

/**
 @file
 @internalComponent
*/

#ifndef SYMBIAN_ETHMCPR_H
#define SYMBIAN_ETHMCPR_H

#include <comms-infras/ss_mcprnodemessages.h>
#include <comms-infras/agentmcpr.h>

namespace EthMCprStates
    {
	class TSendProvision;
    DECLARE_EXPORT_ACTIVITY_MAP(stateMap)
    }
class CEthMetaConnectionProviderFactory;

class CEthMetaConnectionProvider : public CAgentMetaConnectionProvider
/** PPP meta connection provider

@internalTechnology
@released Since 9.4 */
    {
    friend class EthMCprStates::TSendProvision;

public:
    typedef CEthMetaConnectionProviderFactory FactoryType;

	IMPORT_C static CEthMetaConnectionProvider* NewL(ESock::CMetaConnectionProviderFactoryBase& aFactory, const ESock::TProviderInfo& aProviderInfo);
	IMPORT_C virtual ~CEthMetaConnectionProvider();

protected:
    IMPORT_C CEthMetaConnectionProvider(ESock::CMetaConnectionProviderFactoryBase& aFactory,
							            const ESock::TProviderInfo& aProviderInfo,
							            const MeshMachine::TNodeActivityMap& aActivityMap);

    void SetAccessPointConfigFromDbL();
	IMPORT_C virtual void ReceivedL(const Messages::TRuntimeCtxId& aSender, const Messages::TNodeId& aRecipient, Messages::TSignatureBase& aMessage);
	IMPORT_C void ConstructL();

private:
	void ProvisionLinkConfigL(ESock::CCommsDatIapView* aIapView, ESock::RMetaExtensionContainer& aMec);
	void ProvisionNetworkConfigL(ESock::CCommsDatIapView* aIapView, ESock::RMetaExtensionContainer& aMec);
    void ProvisionIp4ConfigL(ESock::CCommsDatIapView* aIapView, TUint32 aOrder, ESock::RMetaExtensionContainer& aMec);
    void ProvisionIp6ConfigL(ESock::CCommsDatIapView* aIapView, TUint32 aOrder, ESock::RMetaExtensionContainer& aMec);
    };



#endif //SYMBIAN_ETHMCPR_H
