// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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


#ifndef SYMBIAN_TUNNELAGENTCPR_H
#define SYMBIAN_TUNNELAGENTCPR_H
#include "agentcpr.h"

#include <comms-infras/corecpr.h>
#include <comms-infras/ss_nodemessages_legacy.h>
#include <comms-infras/ss_nodemessages_legacy_internal.h>
#include <comms-infras/ss_legacyinterfaces.h>
#include <comms-infras/ss_commsdataobject.h>
#include <comms-infras/cagentadapter.h>

namespace ESock
{
	class MPlatsecApiExt;
}

namespace TunnelAgentCprStates
{
	class TJoinRealIAP;
}


class CTunnelAgentConnectionProvider : public CAgentConnectionProvider,
									   public ESock::MPlatsecApiExt,
									   public ITFHIERARCHY_LINK_1(CTunnelAgentConnectionProvider, CAgentConnectionProvider, ESock::MPlatsecApiExt)

	{
	friend class TunnelAgentCprStates::TJoinRealIAP;

public:
	typedef ITFHIERARCHY_LINK_1(CTunnelAgentConnectionProvider, CAgentConnectionProvider, ESock::MPlatsecApiExt) TIfStaticFetcherNearestInHierarchy;

public:
    IMPORT_C static CTunnelAgentConnectionProvider* NewL(ESock::CConnectionProviderFactoryBase& aFactory);
    IMPORT_C ~CTunnelAgentConnectionProvider();

    using CAgentConnectionProvider::ReturnInterfacePtrL;
    void ReturnInterfacePtrL(ESock::MPlatsecApiExt*& aInterface);
protected:
    CTunnelAgentConnectionProvider(ESock::CConnectionProviderFactoryBase& aFactory);
    CTunnelAgentConnectionProvider(ESock::CConnectionProviderFactoryBase& aFactory, const MeshMachine::TNodeActivityMap& aActivityMap);
	// MPlatSecApiExt
	TInt SecureId(TSecureId& aResult) const;
	TInt VendorId(TVendorId& aResult) const;
	TBool HasCapability(const TCapability aCapability) const;
	TInt CheckPolicy(const TSecurityPolicy& aPolicy) const;
	};


#endif
// SYMBIAN_TUNNELAGENTCPR_H

