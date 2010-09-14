/**
*   Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
*   All rights reserved.
*   This component and the accompanying materials are made available
*   under the terms of "Eclipse Public License v1.0"
*   which accompanies this distribution, and is available
*   at the URL "http://www.eclipse.org/legal/epl-v10.html".
*   
*   Initial Contributors:
*   Nokia Corporation - initial contribution.
*   
*   Contributors:
*
*   Description:
*   Factory implementation for tunnel driver mcpr.
* 
*
*/

/**
 @file tundrivermcprfactory.cpp
 @internalTechnology
*/

#include <ecom/implementationproxy.h>
#include "tundrivermcprfactory.h"
#include "tundrivermcpr.h"
#include <comms-infras/ss_log.h>
#include <ss_glob.h>
#include <ecom/ecom.h>

#include <comms-infras/ss_msgintercept.h>

using namespace ESock;

CTunDriverMetaConnectionProviderFactory* CTunDriverMetaConnectionProviderFactory::NewL(TAny* aParentContainer)
/**
* CTunDriverMetaConnectionProviderFactory::NewL constructs a Default MCPR Flow Factory
* @param aParentContainer construction data passed by ECOM
* @returns pointer to a constructed factory object.
*/
	{
 	return new (ELeave) CTunDriverMetaConnectionProviderFactory(TUid::Uid(CTunDriverMetaConnectionProviderFactory::iUid), *(reinterpret_cast<CMetaConnectionFactoryContainer*>(aParentContainer)));
	}

CTunDriverMetaConnectionProviderFactory::CTunDriverMetaConnectionProviderFactory(TUid aFactoryId, CMetaConnectionFactoryContainer& aParentContainer)
	: CMetaConnectionProviderFactoryBase(aFactoryId, aParentContainer)
/**
* CTunDriverMetaConnectionProviderFactory::CTunDriverMetaConnectionProviderFactory is a default constructor.
* @return
*/
	{
	}

ESock::ACommsFactoryNodeId* CTunDriverMetaConnectionProviderFactory::DoCreateObjectL(ESock::TFactoryQueryBase& aQuery)
/**
* CTunDriverMetaConnectionProviderFactory::DoCreateObjectL is called from UpperlLayer.
* @return
*/
	{
	const TMetaConnectionFactoryQuery& query = static_cast<const TMetaConnectionFactoryQuery&>(aQuery);
	CMetaConnectionProviderBase* provider = CTunDriverMetaConnectionProvider::NewL(*this, query.iProviderInfo);
	
	ESOCK_DEBUG_REGISTER_GENERAL_NODE(iUid, provider);

	return provider;
	}


