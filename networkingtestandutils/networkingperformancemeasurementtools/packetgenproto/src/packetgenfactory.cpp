// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
//  Implements the factory class which is used to instantiate the Packet Gen Flow.
//

#include "packetgenfactory.h"
#include "packetgenflow.h"
using namespace ESock;




using namespace ESock;


// =====================================================================================
//
// PacketGen Flow Factory
//

CPacketGenFlowFactory* CPacketGenFlowFactory::NewL(TAny* aConstructionParameters)
/**
Constructs a Default SubConnection Flow Factory

@param aConstructionParameters construction data passed by ECOM

@returns pointer to a constructed factory
*/
	{
	CPacketGenFlowFactory* ptr = new (ELeave) CPacketGenFlowFactory(TUid::Uid(CPacketGenFlowFactory::EUid), *(reinterpret_cast<CSubConnectionFlowFactoryContainer*>(aConstructionParameters)));
	
	return ptr;
	}


CPacketGenFlowFactory::CPacketGenFlowFactory(TUid aFactoryId, CSubConnectionFlowFactoryContainer& aParentContainer)
	: CSubConnectionFlowFactoryBase(aFactoryId, aParentContainer)
/**
Default SubConnection Flow Factory Constructor

@param aFactoryId ECOM Implementation Id
@param aParentContainer Object Owner
*/
	{
	}


CSubConnectionFlowBase* CPacketGenFlowFactory::DoCreateFlowL(CProtocolIntfBase* aProtocol, ESock::TFactoryQueryBase& aQuery)
	{
	const TDefaultFlowFactoryQuery& query = static_cast<const TDefaultFlowFactoryQuery&>(aQuery);
	CSubConnectionFlowBase *temp = new(ELeave)CPacketGenFlow(*this, query.iSCprId, aProtocol);
	return temp;
	}

