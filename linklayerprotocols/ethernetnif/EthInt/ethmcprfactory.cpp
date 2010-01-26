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
// Ethernet MCPR Factory
// 
//

/**
 @file
 @internalComponent
*/

#include "ethmcprfactory.h"
#include "ethmcpr.h"
#include <comms-infras/ss_log.h>
#include <ss_glob.h>

#include <comms-infras/ss_msgintercept.h>

#ifdef __CFLOG_ACTIVE
	#define KEthMCprFactoryTag KESockMetaConnectionTag
	//_LIT8(KEthMCprFactorySubTag, "ethmcprfactory");
#endif

using namespace ESock;

//-=========================================================
//
// CEthMetaConnectionProviderFactory methods
//
//-=========================================================	
CEthMetaConnectionProviderFactory* CEthMetaConnectionProviderFactory::NewL(TAny* aParentContainer)
	{
	__CFLOG_VAR((KEthMCprFactoryTag, KEthMCprFactorySubTag, _L8("CEthMetaConnectionProviderFactory ::\tNewL(%08x)"), aParentContainer));
 	return new (ELeave) CEthMetaConnectionProviderFactory(TUid::Uid(CEthMetaConnectionProviderFactory::iUid), *(reinterpret_cast<CMetaConnectionFactoryContainer*>(aParentContainer)));
	}

CEthMetaConnectionProviderFactory::CEthMetaConnectionProviderFactory(TUid aFactoryId, CMetaConnectionFactoryContainer& aParentContainer)
	: CMetaConnectionProviderFactoryBase(aFactoryId,aParentContainer)
	{
	__CFLOG_VAR((KEthMCprFactoryTag, KEthMCprFactorySubTag, _L8("CEthMetaConnectionProviderFactory %08x:\tCEthMetaConnectionProviderFactory Constructor"), this));
	}

ESock::ACommsFactoryNodeId* CEthMetaConnectionProviderFactory::DoCreateObjectL(ESock::TFactoryQueryBase& aQuery)
	{
	const TMetaConnectionFactoryQuery& query = static_cast<const TMetaConnectionFactoryQuery&>(aQuery);
	__CFLOG_VAR((KEthMCprFactoryTag, KEthMCprFactorySubTag, _L8("CEthMetaConnectionProviderFactory %08x:\tDoCreateL()"), this));
	CMetaConnectionProviderBase* provider = CEthMetaConnectionProvider::NewL(*this, query.iProviderInfo);
	
	ESOCK_DEBUG_REGISTER_GENERAL_NODE(iUid, provider);
	
	return provider;
	}
