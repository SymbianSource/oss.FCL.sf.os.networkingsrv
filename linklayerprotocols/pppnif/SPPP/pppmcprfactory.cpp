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
// PPP MCPR Factory
// 
//

/**
 @file
 @internalComponent
*/

#include <ecom/implementationproxy.h>
#include "pppmcprfactory.h"
#include "pppmcpr.h"
#include <comms-infras/ss_log.h>
#include <ss_glob.h>
#include <ecom/ecom.h>

#include <comms-infras/ss_msgintercept.h>

#ifdef __CFLOG_ACTIVE
#define KPPPMCprFactoryTag KESockMetaConnectionTag
_LIT8(KPPPMCprFactorySubTag, "pppmcprfactory");
#endif

using namespace ESock;

//-=========================================================
//
// CPppMetaConnectionProviderFactory methods
//
//-=========================================================	
CPppMetaConnectionProviderFactory* CPppMetaConnectionProviderFactory::NewL(TAny* aParentContainer)
	{
	__CFLOG_VAR((KPPPMCprFactoryTag, KPPPMCprFactorySubTag, _L8("CPppMetaConnectionProviderFactory ::\tNewL(%08x)"), aParentContainer));
 	return new (ELeave) CPppMetaConnectionProviderFactory(TUid::Uid(CPppMetaConnectionProviderFactory::iUid), *(reinterpret_cast<CMetaConnectionFactoryContainer*>(aParentContainer)));
	}

CPppMetaConnectionProviderFactory::CPppMetaConnectionProviderFactory(TUid aFactoryId, CMetaConnectionFactoryContainer& aParentContainer)
	: CMetaConnectionProviderFactoryBase(aFactoryId,aParentContainer)
	{
	__CFLOG_VAR((KPPPMCprFactoryTag, KPPPMCprFactorySubTag, _L8("CPppMetaConnectionProviderFactory %08x:\tCPppMetaConnectionProviderFactory Constructor"), this));
	}

ACommsFactoryNodeId* CPppMetaConnectionProviderFactory::DoCreateObjectL(ESock::TFactoryQueryBase& aQuery)
	{
	const TMetaConnectionFactoryQuery& query = static_cast<const TMetaConnectionFactoryQuery&>(aQuery);
	CMetaConnectionProviderBase* provider = CPppMetaConnectionProvider::NewL(*this,query.iProviderInfo);
	
	ESOCK_DEBUG_REGISTER_GENERAL_NODE(iUid, provider);

	return provider;
	}

