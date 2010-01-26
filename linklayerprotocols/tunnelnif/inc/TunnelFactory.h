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
// TunnelFlowFactory.h
// Defines the Tunnel Flow and Protocol Interface Factories.
// 
//

/**
 @file
*/

#ifndef TUNNELFACTORY_H_
#define TUNNELFACTORY_H_

#include <e32base.h>
#include <e32std.h>
#include <comms-infras/ss_subconnflow.h>
#include <comms-infras/ss_protflow.h>

/**
Tunnel Flow Implementation UID
*/
const TInt KTunnelFlowImplUid = 0x10281DF7;
/**
Tunnel Protocol Interface Implementation UID
*/
const TInt KTunnelPintImplUid = 0x10281DFA;


class CTunnelFlowFactory : public ESock::CSubConnectionFlowFactoryBase
/**
Tunnel Flow Factory
@internalComponent
*/
	{
public:
	static CTunnelFlowFactory* NewL(TAny* aConstructionParameters);
	virtual ESock::CSubConnectionFlowBase* DoCreateFlowL(ESock::CProtocolIntfBase* aProtocol, ESock::TFactoryQueryBase& aQuery);
	~CTunnelFlowFactory();

protected:
	CTunnelFlowFactory(TUid aFactoryId, ESock::CSubConnectionFlowFactoryContainer& aParentContainer);
	};

#endif // TUNNELFACTORY_H_
