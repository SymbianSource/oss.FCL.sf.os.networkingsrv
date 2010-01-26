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
// QosTest factory class and DLL entry point
// 
//

/**
 @file
 @internalComponent
*/

#include "QoSTestFactories.h"
#include "QosTestLcp.h"

using namespace ESock;

//-=========================================================
// Data/functions required for instantiating ECOM Plugin
//-=========================================================
const TImplementationProxy ImplementationTable[] =
	{
	IMPLEMENTATION_PROXY_ENTRY( KQosTestFlowImplUid, CQosTestSubConnectionFlowFactory::NewL)
	};

/**
ECOM Implementation Factory
*/
EXPORT_C const TImplementationProxy* ImplementationGroupProxy(TInt& aTableCount)
    {
    aTableCount = sizeof(ImplementationTable) / sizeof(TImplementationProxy);
    return ImplementationTable;
    }


//-=========================================================
// CQosTestSubConnectionFlowFactory methods
//-=========================================================
CQosTestSubConnectionFlowFactory::CQosTestSubConnectionFlowFactory(TUid aFactoryId, CSubConnectionFlowFactoryContainer& aParentContainer)
:CSubConnectionFlowFactoryBase(aFactoryId, aParentContainer)
    {
    }

CQosTestSubConnectionFlowFactory* CQosTestSubConnectionFlowFactory::NewL(TAny* aConstructionParameters)
    {
	CQosTestSubConnectionFlowFactory* ptr = new (ELeave) CQosTestSubConnectionFlowFactory(TUid::Uid(KQosTestFlowImplUid), *(reinterpret_cast<CSubConnectionFlowFactoryContainer*>(aConstructionParameters)));
	return ptr;
    }

CSubConnectionFlowBase* CQosTestSubConnectionFlowFactory::DoCreateFlowL(ESock::CProtocolIntfBase* aProtocolIntf, ESock::TFactoryQueryBase& aQuery)
    {
	const TDefaultFlowFactoryQuery& query = static_cast<const TDefaultFlowFactoryQuery&>(aQuery);
    return CQosTestLcp::NewL(*this, query.iSCprId, aProtocolIntf);
    }

