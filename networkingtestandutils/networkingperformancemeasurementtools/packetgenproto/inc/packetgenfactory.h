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
//  Defines the factory class which is used to instantiate the PacketGen layer.
//

/**
 @file
 @internalTechnology
*/

#ifndef PACKETGENFLOWFACTORY_H__
#define PACKETGENFLOWFACTORY_H__

#include <e32base.h>
#include <e32std.h>
#include <comms-infras/ss_subconnflow.h>
#include <comms-infras/ss_protflow.h>
#include <comms-infras/ss_subconnprov.h>
#include <elements/nm_signatures.h>


class CPacketGenFlowFactory : public ESock::CSubConnectionFlowFactoryBase
/**
PacketGen Flow Factory

@internalComponent
*/
	{
public:

	enum { EUid = 0x10272F4A };
	static CPacketGenFlowFactory* NewL(TAny* aConstructionParameters);

protected:
	CPacketGenFlowFactory(TUid aFactoryId, ESock::CSubConnectionFlowFactoryContainer& aParentContainer);
	virtual ESock::CSubConnectionFlowBase* DoCreateFlowL(ESock::CProtocolIntfBase* aProtocol, ESock::TFactoryQueryBase& aQuery);
	};

#endif // PACKETGENFLOWFACTORY_H__
