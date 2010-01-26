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
// Agent Connection Provider ECOM Factory
// 
//

/**
 @file
 @internalComponent
*/


#include "agentcprfactory.h"
#include "agentcpr.h"

#include <comms-infras/ss_msgintercept.h>

using namespace ESock;


#ifdef _DEBUG
#define KAgentCprFactoryTag KESockConnectionTag
_LIT8(KAgentCprFactorySubTag, "agentcprfactory");
#endif


// ---------------- Factory Methods ----------------

/**
Creates an instance of the Agent CPr Factory
@param aParentContainer the parent factory container which owns this factory
@return A pointer to the instance of the Agent Connection Provider factory 
*/
CAgentConnectionProviderFactory* CAgentConnectionProviderFactory::NewL(TAny* aParentContainer)
    {
    return new (ELeave) CAgentConnectionProviderFactory(TUid::Uid(CAgentConnectionProviderFactory::iUid),
      *reinterpret_cast<ESock::CConnectionFactoryContainer*>(aParentContainer));
    }
   
    
/**
Performs the actual creation of a Agent CPr Factory
@param aFactoryId The TUid of this factory
@param aParentContainer The factory container to hold this factory
@return Factory for an Agent Connection Provider
*/
CAgentConnectionProviderFactory::CAgentConnectionProviderFactory (TUid aFactoryId, CConnectionFactoryContainer& aParentContainer)
	: CConnectionProviderFactoryBase (aFactoryId, aParentContainer)
   {
   }


/**
Creates the provider supplied by this factory
@return A pointer to the instance of an Agent Connection Provider
*/
ESock::ACommsFactoryNodeId* CAgentConnectionProviderFactory::DoCreateObjectL(TFactoryQueryBase& /* aQuery */)
    {    
   __CFLOG_VAR((KAgentCprFactoryTag, KAgentCprFactorySubTag, _L8("CAgentConnectionProviderFactory [this=%08x]:DoCreateObjectL()"), this));

    CAgentConnectionProvider* provider = CAgentConnectionProvider::NewL (*this);

	ESOCK_DEBUG_REGISTER_GENERAL_NODE(iUid, provider);

    return provider;
    }


