/**
* Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* Header file declaring the Ethernet CFProtocol ECOM factories
* 
*
*/



/**
 @file ethcfp.h
*/

#ifndef ETHCFP_H_INCLUDED_
#define ETHCFP_H_INCLUDED_

#include <e32base.h>
#include <e32std.h>
#include <comms-infras/ss_subconnflow.h>
#include <comms-infras/ss_protflow.h>
#include <comms-infras/ss_nodemessages.h>
/**
Ethernet SubConnFlow Implementation UID
*/
const TInt KEthFlowImplementationUid = 0x10281DDB;


class CEthSubConnectionFlowFactory : public ESock::CSubConnectionFlowFactoryBase
/**
Ethernet Flow Factory
@internalComponent
*/
	{
public:
	static CEthSubConnectionFlowFactory* NewL(TAny* aConstructionParameters);
	virtual ESock::CSubConnectionFlowBase* DoCreateFlowL(ESock::CProtocolIntfBase* aProtocolIntf, ESock::TFactoryQueryBase& aQuery);
protected:
	CEthSubConnectionFlowFactory(TUid aFactoryId, ESock::CSubConnectionFlowFactoryContainer& aParentContainer);
	};

#endif // ETHCFP_H_INCLUDED_
