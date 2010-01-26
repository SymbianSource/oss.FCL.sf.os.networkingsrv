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
// Dummy Proto MCPR Factory
// 
//

/**
 @file
 @internalComponent
*/

#include <ecom/implementationproxy.h>
#include "dummyprotomcprfactory.h"
#include "dummyprotomcpr.h"
#include <comms-infras/ss_log.h>
#include <ss_glob.h>
#include <ecom/ecom.h>

#include <comms-infras/ss_msgintercept.h>

#if defined(SYMBIAN_TRACE_ENABLE)
    _LIT8(KDummyProtoMCprSubTag, "dummymcpr");
#endif

using namespace ESock;

//-=========================================================
//
// CDummyProtoMetaConnectionProviderFactory methods
//
//-=========================================================	
CDummyProtoMetaConnectionProviderFactory* CDummyProtoMetaConnectionProviderFactory::NewL(TAny* aParentContainer)
	{
 	return new (ELeave) CDummyProtoMetaConnectionProviderFactory(TUid::Uid(CDummyProtoMetaConnectionProviderFactory::iUid), *(reinterpret_cast<CMetaConnectionFactoryContainer*>(aParentContainer)));
	}

CDummyProtoMetaConnectionProviderFactory::CDummyProtoMetaConnectionProviderFactory(TUid aFactoryId, CMetaConnectionFactoryContainer& aParentContainer)
	: CMetaConnectionProviderFactoryBase(aFactoryId, aParentContainer)
	{
	}

ESock::ACommsFactoryNodeId* CDummyProtoMetaConnectionProviderFactory::DoCreateObjectL(ESock::TFactoryQueryBase& aQuery)
	{
	const TMetaConnectionFactoryQuery& query = static_cast<const TMetaConnectionFactoryQuery&>(aQuery);
	CMetaConnectionProviderBase* provider = CDummyProtoMetaConnectionProvider::NewL(*this, query.iProviderInfo);
	
	ESOCK_DEBUG_REGISTER_GENERAL_NODE(iUid, provider);

	return provider;
	}


