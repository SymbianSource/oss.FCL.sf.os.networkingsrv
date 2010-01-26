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
// PPP SCpr Factory
// 
//

/**
 @file
 @internalComponent
*/

#include "pppscprfactory.h"
#include "pppscpr.h"
#include <comms-infras/ss_log.h>

#include <comms-infras/ss_msgintercept.h>

#ifdef __CFLOG_ACTIVE
#define KPPPSCprFactoryTag KESockSubConnectionTag
_LIT8(KPPPSCprFactorySubTag, "pppscprfactory");
#endif


using namespace ESock;


//-=========================================================
//
// CPppSubConnectionProviderFactory methods
//
//-=========================================================	
CPppSubConnectionProviderFactory* CPppSubConnectionProviderFactory::NewL(TAny* aParentContainer)
	{
	__CFLOG_VAR((KPPPSCprFactoryTag, KPPPSCprFactorySubTag,
	    _L8("CPppSubConnectionProviderFactory ::\tNewL(%08x)"), aParentContainer));
	    
 	return new (ELeave) CPppSubConnectionProviderFactory(
 	    TUid::Uid(CPppSubConnectionProviderFactory::iUid),
 	    *(reinterpret_cast<CSubConnectionFactoryContainer*>(aParentContainer)));
	}


CPppSubConnectionProviderFactory::CPppSubConnectionProviderFactory(TUid aFactoryId, CSubConnectionFactoryContainer& aParentContainer)
	: CSubConnectionProviderFactoryBase(aFactoryId, aParentContainer)
	{
	__CFLOG_VAR((KPPPSCprFactoryTag, KPPPSCprFactorySubTag, _L8("CPppSubConnectionProviderFactory %08x:\tCPppSubConnectionProviderFactory Constructor"), this));
	}


ACommsFactoryNodeId* CPppSubConnectionProviderFactory::DoCreateObjectL(ESock::TFactoryQueryBase& /* aQuery */)
	{
	CSubConnectionProviderBase* provider = CPppSubConnectionProvider::NewL(*this);	

	ESOCK_DEBUG_REGISTER_GENERAL_NODE(iUid, provider);

	return provider;	    
	}

