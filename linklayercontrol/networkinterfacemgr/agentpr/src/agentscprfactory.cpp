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
// Agent SubConnection Provider ECOM Factory
// 
//

/**
 @file
 @internalComponent
*/


#include "agentscprfactory.h"
#include "agentscpr.h"

#include <comms-infras/ss_msgintercept.h>

using namespace ESock;


#ifdef _DEBUG
#define KAgentSCprFactoryTag KESockSubConnectionTag
_LIT8(KAgentSCprFactorySubTag, "agentscprfactory");
#endif



// ---------------- Factory Methods ----------------

/**
Creates an instance of the Agent SCPr Factory
@param aParentContainer the parent factory container which owns this factory
@return A pointer to the instance of the Agent SubConnection Provider factory 
*/
CAgentSubConnectionProviderFactory* CAgentSubConnectionProviderFactory::NewL(TAny* aParentContainer)
    {
    return new (ELeave) CAgentSubConnectionProviderFactory(TUid::Uid(CAgentSubConnectionProviderFactory::iUid),
      *reinterpret_cast<ESock::CSubConnectionFactoryContainer*>(aParentContainer));
    }
   
    
/**
Performs the actual creation of a Agent SCPr Factory
@param aFactoryId The TUid of this factory
@param aParentContainer The factory container to hold this factory
@return Factory for an Agent SubConnection Provider
*/
CAgentSubConnectionProviderFactory::CAgentSubConnectionProviderFactory (TUid aFactoryId, ESock::CSubConnectionFactoryContainer& aParentContainer)
	: ESock::CSubConnectionProviderFactoryBase (aFactoryId, aParentContainer)
   {
   }


/**
Creates the provider supplied by this factory
@return A pointer to the instance of an Agent SubConnection Provider
*/
ESock::ACommsFactoryNodeId* CAgentSubConnectionProviderFactory::DoCreateObjectL(TFactoryQueryBase& /* aQuery */)
    {
    CAgentSubConnectionProvider* provider = CAgentSubConnectionProvider::NewL(*this);

	ESOCK_DEBUG_REGISTER_GENERAL_NODE(iUid, provider);

    return provider;
    }

