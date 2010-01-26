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

#ifndef _DELAYMETERPROTOFACTORY_H__
#define _DELAYMETERPROTOFACTORY_H__

#include <e32base.h>
#include <e32std.h>
#include <comms-infras/ss_subconnflow.h>
#include <comms-infras/ss_protflow.h>
#include <comms-infras/ss_subconnprov.h>

#include <elements/nm_signatures.h>

class CDelayMeterProtoFactory : public ESock::CSubConnectionFlowFactoryBase
/**
Timestamping Flow Factory
The purpose of this protocol is to
 - add a timestamp to recognised packets (for incoming packets)
 ..or..
 - store the total delay (for outgoing packets)

It talks back to the client-side TEF library (or whoever) via a control method.

@internalComponent
*/
	{
public:

	enum { EUid = 0x10272F47 };
	static CDelayMeterProtoFactory* NewL(TAny* aConstructionParameters);

protected:
	CDelayMeterProtoFactory(TUid aFactoryId, ESock::CSubConnectionFlowFactoryContainer& aParentContainer);
	virtual ESock::CSubConnectionFlowBase* DoCreateFlowL(ESock::CProtocolIntfBase* aProtocol, ESock::TFactoryQueryBase& aQuery);
	};

#endif // _DELAYMETERPROTOFACTORY_H__
