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
// Implements the Tunnel Flow and Protocol Interface Factories.
// 
//

/**
 @file
*/

#include "TunnelFactory.h"
#include "tunnelFlow.h"
#include "tunnelProvision.h"		// for TTunnelMessages

using namespace ESock;

// =====================================================================================
//
// Tunnel Flow Factory
//

CTunnelFlowFactory* CTunnelFlowFactory::NewL(TAny* aConstructionParameters)
/**
Constructs a Tunnel SubConnection Flow Factory

@param aConstructionParameters construction data passed by ECOM

@return pointer to a constructed factory
*/
	{
	CTunnelFlowFactory* ptr = new (ELeave) CTunnelFlowFactory(TUid::Uid(KTunnelFlowImplUid), *(reinterpret_cast<CSubConnectionFlowFactoryContainer*>(aConstructionParameters)));
	CleanupStack::PushL(ptr);

	//
	// Register virtual constructors for Tunnel messages
	//
	// Note that factories are never unloaded at present, so this registration will
	// effectively exist until ESockSvr unloads.
	//

	TTunnelMessages::RegisterL();
	CleanupStack::Pop(ptr);
	return ptr;
	}


CTunnelFlowFactory::CTunnelFlowFactory(TUid aFactoryId, CSubConnectionFlowFactoryContainer& aParentContainer)
	: CSubConnectionFlowFactoryBase(aFactoryId, aParentContainer)
/**
Tunnel SubConnection Flow Factory Constructor

@param aFactoryId ECOM Implementation Id
@param aParentContainer Object Owner
*/
	{
	}

CTunnelFlowFactory::~CTunnelFlowFactory()
	{
	// De-register virtual constructors for Tunnel messages.
	TTunnelMessages::DeRegister();
	}

CSubConnectionFlowBase* CTunnelFlowFactory::DoCreateFlowL(ESock::CProtocolIntfBase* aProtocol, ESock::TFactoryQueryBase& aQuery)
	{
	const TDefaultFlowFactoryQuery& query = static_cast<const TDefaultFlowFactoryQuery&>(aQuery);
 	CTunnelFlow* s = new (ELeave) CTunnelFlow(*this, query.iSCprId, aProtocol);
	return s;
	}

