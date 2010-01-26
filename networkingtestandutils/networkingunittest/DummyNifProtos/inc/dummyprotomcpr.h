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
// DummyProto MCPR
// 
//

/**
 @file
 @internalComponent
*/

#ifndef SYMBIAN_DUMMYPROTOMCPR_H
#define SYMBIAN_DUMMYPROTOMCPR_H

#include <comms-infras/ss_mcprnodemessages.h>
#include "agentmcpr.h"
#include "dummyprotomcprfactory.h"

//#define KDummyProtoMCprTag KESockConnectionTag

namespace DummyProtoMCprStates
    {
	class TSendProvision;
    }

class CDummyProtoProvision;
class CDummyProtoAgentHandler;

class CDummyProtoMetaConnectionProvider : public CAgentMetaConnectionProvider
/** PPP meta connection provider

@internalTechnology
@released Since 9.4 */
    {
    friend class DummyProtoMCprStates::TSendProvision;
public:
    typedef CDummyProtoMetaConnectionProviderFactory FactoryType;

	static CDummyProtoMetaConnectionProvider* NewL(ESock::CMetaConnectionProviderFactoryBase& aFactory, const ESock::TProviderInfo& aProviderInfo);
	virtual ~CDummyProtoMetaConnectionProvider();

protected:
    CDummyProtoMetaConnectionProvider(ESock::CMetaConnectionProviderFactoryBase& aFactory, const ESock::TProviderInfo& aProviderInfo);
    void SetAccessPointConfigFromDbL();

protected:
	virtual void ReceivedL(const Messages::TRuntimeCtxId& aSender, const Messages::TNodeId& aRecipient, Messages::TSignatureBase& aMessage);
	void ConstructL();

private:
	TIp6Addr PresetIP6Addr(ESock::CCommsDatIapView* aReader, CommsDat::TMDBElementId aElementId);	// Duplicated??

private:
	CDummyProtoAgentHandler* iAgentHandler;		// DummyProto Agent Notification Handler
    };

#endif //SYMBIAN_DUMMYPROTOMCPR_H
