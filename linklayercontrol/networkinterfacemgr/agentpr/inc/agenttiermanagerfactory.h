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


#ifndef SYMBIAN_AGENTTIERMANAGERFACTORY_H
#define SYMBIAN_AGENTTIERMANAGERFACTORY_H


#include <comms-infras/ss_tiermanager.h>
#include <comms-infras/ss_nodemessages.h>

class CAgentTierManagerFactory : public ESock::CTierManagerFactoryBase
	{
public:
    enum { iUid = 0x10281DF5 };
	static CAgentTierManagerFactory* NewL(TAny* aParentContainer);

protected:
	CAgentTierManagerFactory(TUid aTierTypeId,TUid aFactoryUid, ESock::CTierManagerFactoryContainer& aParentContainer);
	virtual ESock::ACommsFactoryNodeId* DoCreateObjectL(ESock::TFactoryQueryBase& aQuery);
	};

#endif
// SYMBIAN_AGENTTIERMANAGERFACTORY_H


