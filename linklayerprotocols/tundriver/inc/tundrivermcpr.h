/**
*   Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
*   All rights reserved.
*   This component and the accompanying materials are made available
*   under the terms of "Eclipse Public License v1.0"
*   which accompanies this distribution, and is available
*   at the URL "http://www.eclipse.org/legal/epl-v10.html".
*   
*   Initial Contributors:
*   Nokia Corporation - initial contribution.
*   
*   Contributors:
*
*   Description:
*   Header file for the tun driver Mcpr configuration.
* 
*
*/

/**
 @file tundrivermcpr.h
 @internalTechnology
*/

#ifndef TUNDRIVERMCPR_H
#define TUNDRIVERMCPR_H

#include <comms-infras/ss_mcprnodemessages.h>
#include "agentmcpr.h"
#include "tundrivermcprfactory.h"


namespace TunDriverMCprStates
    {
	class TSendProvision;
    }

class CTunDriverMetaConnectionProvider : public CAgentMetaConnectionProvider
   {
    friend class TunDriverMCprStates::TSendProvision;
public:
    typedef CTunDriverMetaConnectionProviderFactory FactoryType;

	static CTunDriverMetaConnectionProvider* NewL(ESock::CMetaConnectionProviderFactoryBase& aFactory, const ESock::TProviderInfo& aProviderInfo);
	virtual ~CTunDriverMetaConnectionProvider();

protected:
    CTunDriverMetaConnectionProvider(ESock::CMetaConnectionProviderFactoryBase& aFactory, const ESock::TProviderInfo& aProviderInfo);
    void SetAccessPointConfigFromDbL();

protected:
	virtual void ReceivedL(const Messages::TRuntimeCtxId& aSender, const Messages::TNodeId& aRecipient, Messages::TSignatureBase& aMessage);
	void ConstructL();
    };

#endif //TUNDRIVERMCPR_H
