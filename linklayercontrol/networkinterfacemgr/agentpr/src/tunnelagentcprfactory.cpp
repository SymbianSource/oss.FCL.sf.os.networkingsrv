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
// Tunnel Connection Provider ECOM Factory
// 
//

/**
 @file
 @internalComponent
*/


#include "tunnelagentcprfactory.h"
#include "tunnelagentcpr.h"

#include <comms-infras/ss_msgintercept.h>

using namespace ESock;


#ifdef _DEBUG
#define KTunnelAgentCprFactoryTag KESockConnectionTag
#endif


// ---------------- Factory Methods ----------------

/**
Creates an instance of the Tunnel CPr Factory
@param aParentContainer the parent factory container which owns this factory
@return A pointer to the instance of the Tunnel Connection Provider factory 
*/
CTunnelAgentConnectionProviderFactory* CTunnelAgentConnectionProviderFactory::NewL(TAny* aParentContainer)
    {
    return new (ELeave) CTunnelAgentConnectionProviderFactory(TUid::Uid(CTunnelAgentConnectionProviderFactory::iUid),
      *reinterpret_cast<ESock::CConnectionFactoryContainer*>(aParentContainer));
    }
   
    
/**
Performs the actual creation of a Tunnel Agent CPr Factory
@param aFactoryId The TUid of this factory
@param aParentContainer The factory container to hold this factory
@return Factory for an Tunnel Connection Provider
*/
CTunnelAgentConnectionProviderFactory::CTunnelAgentConnectionProviderFactory (TUid aFactoryId, CConnectionFactoryContainer& aParentContainer)
	: CConnectionProviderFactoryBase (aFactoryId, aParentContainer)
   {
   }


/**
Creates the provider supplied by this factory
@return A pointer to the instance of an Tunnel Connection Provider
*/
ESock::ACommsFactoryNodeId* CTunnelAgentConnectionProviderFactory::DoCreateObjectL(ESock::TFactoryQueryBase& /* aQuery */)
    {
    CTunnelAgentConnectionProvider* provider = CTunnelAgentConnectionProvider::NewL (*this);

	ESOCK_DEBUG_REGISTER_GENERAL_NODE(iUid, provider);

    return provider;
    }


