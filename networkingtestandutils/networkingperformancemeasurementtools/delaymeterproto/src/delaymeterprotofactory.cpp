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
// Defines the factory class which is used to instantiate the delay meter layer.
// 
//

/**
 @file
 @internalTechnology
*/

#include "delaymeterprotofactory.h"
#include "delaymeterflow.h"
using namespace ESock;


//#include <comms-infras/msgintercept_std.h>


using namespace ESock;


// =====================================================================================
//
// DelayMeterProto Factory
//

CDelayMeterProtoFactory* CDelayMeterProtoFactory::NewL(TAny* aConstructionParameters)
/**
Constructs a Default SubConnection Flow Factory

@param aConstructionParameters construction data passed by ECOM

@return pointer to a constructed factory
*/
	{
	CDelayMeterProtoFactory* ptr = new (ELeave) CDelayMeterProtoFactory(TUid::Uid(EUid), *(reinterpret_cast<CSubConnectionFlowFactoryContainer*>(aConstructionParameters)));
	
	return ptr;
	}


CDelayMeterProtoFactory::CDelayMeterProtoFactory(TUid aFactoryId, CSubConnectionFlowFactoryContainer& aParentContainer)
	: CSubConnectionFlowFactoryBase(aFactoryId, aParentContainer)
/**
Default SubConnection Flow Factory Constructor

@param aFactoryId ECOM Implementation Id
@param aParentContainer Object Owner
*/
	{
	}


CSubConnectionFlowBase* CDelayMeterProtoFactory::DoCreateFlowL(CProtocolIntfBase* aProtocol, ESock::TFactoryQueryBase& aQuery)
	{
	const TDefaultFlowFactoryQuery& query = static_cast<const TDefaultFlowFactoryQuery&>(aQuery);
	CSubConnectionFlowBase *temp = new(ELeave) CDelayMeterFlow(*this, query.iSCprId, aProtocol);
	return temp;
	}

