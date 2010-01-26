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
// IP Connection Provider factory class definition.
// 
//

/**
 @file
 @internalComponent
*/

#include "IPCprFactory.h"
#include "IPCpr.h"
#include <in_sock.h>
#include <comms-infras/ss_log.h>

#include <comms-infras/ss_msgintercept.h>
   
#ifdef __CFLOG_ACTIVE
	#define KIpCprFactoryTag KESockConnectionTag
	// _LIT8(KIpCprFactorySubTag, "ipcprfactory");
#endif // __CFLOG_ACTIVE

using namespace ESock;

//-=========================================================
//
// CIPConnectionProviderFactory methods
//
//-=========================================================	
EXPORT_C CIPConnectionProviderFactory* CIPConnectionProviderFactory::NewL(TAny* aParentContainer)
    {
    return new (ELeave) CIPConnectionProviderFactory(TUid::Uid(CIPConnectionProviderFactory::iUid), *reinterpret_cast<ESock::CConnectionFactoryContainer*>(aParentContainer));
    }
    
CIPConnectionProviderFactory::CIPConnectionProviderFactory(TUid aFactoryId, ESock::CConnectionFactoryContainer& aParentContainer)
	: ESock::CConnectionProviderFactoryBase(aFactoryId, aParentContainer)
    {
    }

ESock::ACommsFactoryNodeId* CIPConnectionProviderFactory::DoCreateObjectL(ESock::TFactoryQueryBase& /* aQuery */)
    {    
    CConnectionProviderBase* provider = CIPConnectionProvider::NewL(*this);
    
	ESOCK_DEBUG_REGISTER_GENERAL_NODE(iUid, provider);

    return provider;
    }





