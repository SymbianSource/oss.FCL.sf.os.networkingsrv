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


#ifndef SYMBIAN_TUNNELAGENTCPRFACTORY_H
#define SYMBIAN_TUNNELAGENTCPRFACTORY_H


#include <comms-infras/ss_connprov.h>
#include <comms-infras/ss_log.h>

#include <comms-infras/ss_nodemessages_legacy.h>
#include <comms-infras/ss_nodemessages_legacy_internal.h>
#include <comms-infras/ss_legacyinterfaces.h>

const TUint KTunnelAgtCprFactoryUid = 0x10285E08;

NONSHARABLE_CLASS(CTunnelAgentConnectionProviderFactory) : public ESock::CConnectionProviderFactoryBase
	{		
public:
    enum { iUid = KTunnelAgtCprFactoryUid };
	static CTunnelAgentConnectionProviderFactory* NewL(TAny* aParentContainer);

protected:
	CTunnelAgentConnectionProviderFactory(TUid aFactoryId, ESock::CConnectionFactoryContainer& aParentContainer);

    //Implementation of CConnectionProviderFactoryBase
    virtual ESock::ACommsFactoryNodeId* DoCreateObjectL(ESock::TFactoryQueryBase& aQuery);
	};

#endif
// SYMBIAN_TUNNELAGENTCPRFACTORY_H

