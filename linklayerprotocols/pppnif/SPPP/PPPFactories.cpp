// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// PPP factory class and DLL entry point
// 
//

/**
 @file
 @internalComponent
*/

#include "PPPFactories.h"
#include "pppmcprfactory.h"
#include <networking/ppplcp.h>

using namespace ESock;

//-=========================================================
// CPPPSubConnectionFlowFactory methods
//-=========================================================
CPPPSubConnectionFlowFactory::CPPPSubConnectionFlowFactory(TUid aFactoryId, CSubConnectionFlowFactoryContainer& aParentContainer)
:CSubConnectionFlowFactoryBase(aFactoryId, aParentContainer)
    {
    }

CPPPSubConnectionFlowFactory* CPPPSubConnectionFlowFactory::NewL(TAny* aConstructionParameters)
    {
	CPPPSubConnectionFlowFactory* ptr = new (ELeave) CPPPSubConnectionFlowFactory(TUid::Uid(KPPPFlowImplUid), *(reinterpret_cast<CSubConnectionFlowFactoryContainer*>(aConstructionParameters)));
	return ptr;
    }

CSubConnectionFlowBase* CPPPSubConnectionFlowFactory::DoCreateFlowL(ESock::CProtocolIntfBase* aProtocolIntf, TFactoryQueryBase& aQuery)
    {
    const TDefaultFlowFactoryQuery& query = static_cast<const TDefaultFlowFactoryQuery&>(aQuery);
    return CPppLcp::NewL(*this, query.iSCprId, aProtocolIntf);
    }

