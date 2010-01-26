// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Implementation file for the Nif ProtocolIntf
// 
//

/**
 @file DummyNifFlow.cpp
*/

#include <e32std.h>
#include <ecom/ecom.h>
#include <ecom/implementationproxy.h>
#include "ss_subconnflow.h"
#include "DummyNifFlow.h"
#include "HungryNifFlow.h"
#include "hungrynifbinder.h"

using namespace ESock;

CHungryNifSubConnectionFlowFactory* CHungryNifSubConnectionFlowFactory::NewL(TAny* aConstructionParameters)
/**
Constructs a Default SubConnection Flow Factory

@param aConstructionParameters construction data passed by ECOM

@returns pointer to a constructed factory
*/
	{
	CHungryNifSubConnectionFlowFactory* ptr = new (ELeave) CHungryNifSubConnectionFlowFactory(TUid::Uid(KHungryNifFlowImplementationUid), *(reinterpret_cast<CSubConnectionFlowFactoryContainer*>(aConstructionParameters)));
	return ptr;
	}


CHungryNifSubConnectionFlowFactory::CHungryNifSubConnectionFlowFactory(TUid aFactoryId, CSubConnectionFlowFactoryContainer& aParentContainer)
	: CDummyNifSubConnectionFlowFactory(aFactoryId, aParentContainer)
/**
Default SubConnection Flow Factory Constructor

@param aFactoryId ECOM Implementation Id
@param aParentContainer Object Owner
*/
	{
	}

CHungryNifSubConnectionFlow::CHungryNifSubConnectionFlow(ESock::CSubConnectionFlowFactoryBase& aFactory, const Messages::TNodeId& aSubConnId, ESock::CProtocolIntfBase* aProtocolIntf)
  : CDummyNifSubConnectionFlow(aFactory, aSubConnId, aProtocolIntf)
	{
	}

CSubConnectionFlowBase* CHungryNifSubConnectionFlowFactory::DoCreateFlowL(ESock::CProtocolIntfBase* aProtocolIntf, ESock::TFactoryQueryBase& aQuery)
	{
	const TDefaultFlowFactoryQuery& query = static_cast<const TDefaultFlowFactoryQuery&>(aQuery);
	return CHungryNifSubConnectionFlow::NewL(*this, query.iSCprId, aProtocolIntf);
	}

//=======================================================================================
// CDummyNifSubConnectionFlow
//

CHungryNifSubConnectionFlow* CHungryNifSubConnectionFlow::NewL(ESock::CSubConnectionFlowFactoryBase& aFactory, const Messages::TNodeId& aSubConnId, ESock::CProtocolIntfBase* aProtocolIntf)
	{
	CHungryNifSubConnectionFlow* flow = new (ELeave) CHungryNifSubConnectionFlow(aFactory, aSubConnId, aProtocolIntf);

	return flow;
	}

// MBinderControl methods

MLowerControl* CHungryNifSubConnectionFlow::GetControlL(const TDesC8& aProtocol)
	{
	MLowerControl* lowerControl = NULL;

	if (aProtocol.CompareF(KProtocol4()) == 0)
		{
        CHungryNifBinder4* binder4 = CHungryNifBinder4::NewL(*this);
        SetBinder4(binder4);
		lowerControl = binder4;
		}
	else
	if (aProtocol.CompareF(KProtocol6()) == 0)
		{
        CHungryNifBinder6* binder6 = CHungryNifBinder6::NewL(*this);
        SetBinder6(binder6);
		lowerControl = binder6;
		}
	return lowerControl;
	}
