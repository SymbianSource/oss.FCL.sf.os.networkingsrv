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


#ifndef SYMBIAN_AGENTCPRFACTORY_H
#define SYMBIAN_AGENTCPRFACTORY_H


#include <comms-infras/ss_connprov.h>
#include <comms-infras/ss_log.h>
#include <comms-infras/ss_nodemessages_legacy.h>
#include <comms-infras/ss_nodemessages_legacy_internal.h>
#include <comms-infras/ss_legacyinterfaces.h>
#include <comms-infras/ss_nodemessages.h>

NONSHARABLE_CLASS(CAgentConnectionProviderFactory) : public ESock::CConnectionProviderFactoryBase
	{		
public:
    enum { iUid = 0x10281DE8 };
	static CAgentConnectionProviderFactory* NewL(TAny* aParentContainer);

protected:
	CAgentConnectionProviderFactory(TUid aFactoryId, ESock::CConnectionFactoryContainer& aParentContainer);

    //Implementation of CConnectionProviderFactoryBase
    virtual ESock::ACommsFactoryNodeId* DoCreateObjectL(ESock::TFactoryQueryBase& aQuery);
	};

#endif
// SYMBIAN_AGENTCPRFACTORY_H

