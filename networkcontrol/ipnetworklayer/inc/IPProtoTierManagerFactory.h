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
// IPProto Tier Manager Factory
// 
//

/**
 @file
 @internalComponent
*/
 
#ifndef SYMBIAN_IPPROTO_TIER_MANAGER_FACTORY_H
#define SYMBIAN_IPPROTO_TIER_MANAGER_FACTORY_H

#include <comms-infras/ss_tiermanager.h>

class CIPProtoTierManagerFactory : public ESock::CTierManagerFactoryBase
	{
public:
    enum { iUid = 0x10281DF0 };
	static CIPProtoTierManagerFactory* NewL(TAny* aParentContainer);

protected:
	CIPProtoTierManagerFactory(TUid aTierTypeId,TUid aFactoryUid, ESock::CTierManagerFactoryContainer& aParentContainer);
	virtual ESock::ACommsFactoryNodeId* DoCreateObjectL(ESock::TFactoryQueryBase& aQuery);
	};

#endif //SYMBIAN_IPPROTO_TIER_MANAGER_FACTORY_H

