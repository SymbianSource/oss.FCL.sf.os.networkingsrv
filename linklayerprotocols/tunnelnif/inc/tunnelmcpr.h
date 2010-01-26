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
// Tunnel MCPR
// 
//

/**
 @file
 @internalComponent
*/

#ifndef SYMBIAN_TUNNELMCPR_H
#define SYMBIAN_TUNNELMCPR_H

#include <comms-infras/ss_mcprnodemessages.h>
#include "agentmcpr.h"
#include "tunnelmcprfactory.h"

namespace TunnelMCprStates
    {
    class TCreateProvisionInfo;
    }

class CTunnelAgentHandler;

class CTunnelMetaConnectionProvider : public CAgentMetaConnectionProvider
/** Tunnel meta connection provider

@internalTechnology
@released Since 9.4 */
    {
    friend class TunnelMCprStates::TCreateProvisionInfo;
public:
    typedef CTunnelMetaConnectionProviderFactory FactoryType;

	static CTunnelMetaConnectionProvider* NewL(ESock::CMetaConnectionProviderFactoryBase& aFactory, const ESock::TProviderInfo& aProviderInfo);
	virtual ~CTunnelMetaConnectionProvider();

protected:
    CTunnelMetaConnectionProvider(ESock::CMetaConnectionProviderFactoryBase& aFactory, const ESock::TProviderInfo& aProviderInfo);
    void SetAccessPointConfigFromDbL();

	virtual void ReceivedL(const Messages::TRuntimeCtxId& aSender, const Messages::TNodeId& aRecipient, Messages::TSignatureBase& aMessage);
	void ConstructL();

private:
	CTunnelAgentHandler* iAgentHandler;
    };


#endif //SYMBIAN_TUNNELMCPR_H
