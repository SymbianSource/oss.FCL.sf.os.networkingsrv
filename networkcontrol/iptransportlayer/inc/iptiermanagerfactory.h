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
// IPTIERMANAGERFACTORY_H.H
// 
//

/**
 @file
 @internalTechnology
 @prototype
*/

#ifndef SYMBIAN_IP_TIER_MANAGER_FACTORY_H
#define SYMBIAN_IP_TIER_MANAGER_FACTORY_H

#include <comms-infras/ss_tiermanager.h>
#include <in_sock.h>

NONSHARABLE_CLASS(CIpTierManagerFactory) : public ESock::CTierManagerFactoryBase
	{
public:
    enum { iUid = 0x10281DD8,
    	   iType = KAfInet };
	IMPORT_C static CIpTierManagerFactory* NewL(TAny* aParentContainer);

protected:
	CIpTierManagerFactory(TUid aTierTypeId, TUid aFactoryUid, ESock::CTierManagerFactoryContainer& aParentContainer);
	virtual ESock::ACommsFactoryNodeId* DoCreateObjectL(ESock::TFactoryQueryBase& aQuery);
	};

#endif //SYMBIAN_IP_TIER_MANAGER_FACTORY_H

