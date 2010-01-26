/**
* Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* Header file for the Default SubConnection Flow
* 
*
*/



/**
 @file HungryNifflow.h
*/

#ifndef HUNGRYNIFFLOW_H_INCLUDED_
#define HUNGRYNIFFLOW_H_INCLUDED_

#include <e32base.h>
#include <e32std.h>
#include <comms-infras/ss_subconnprov.h>
#include <comms-infras/ss_subconnflow.h>
#include "Dummynifbinder.h"

/**
Dummy Nif SubConnFlow Implementation UID
*/
const TInt KHungryNifFlowImplementationUid = 0x10281C3D;

class CHungryNifSubConnectionFlowFactory : public CDummyNifSubConnectionFlowFactory
/**
*/
	{
public:
	static CHungryNifSubConnectionFlowFactory* NewL(TAny* aConstructionParameters);
	virtual ESock::CSubConnectionFlowBase* DoCreateFlowL(ESock::CProtocolIntfBase* aProtocolIntf, ESock::TFactoryQueryBase& aQuery);

protected:
	CHungryNifSubConnectionFlowFactory(TUid aFactoryId, ESock::CSubConnectionFlowFactoryContainer& aParentContainer);
	};

class CHungryNifSubConnectionFlow : public CDummyNifSubConnectionFlow
/**
*/
	{
	friend class CDummyNifSubConnectionFlowFactory;
public:
	static CHungryNifSubConnectionFlow* NewL(ESock::CSubConnectionFlowFactoryBase& aFactory, const Messages::TNodeId& aSubConnId, ESock::CProtocolIntfBase* aProtocolIntf);

	// from MBinderControl
	virtual ESock::MLowerControl* GetControlL(const TDesC8& aProtocol);

protected:
	CHungryNifSubConnectionFlow(ESock::CSubConnectionFlowFactoryBase& aFactory, const Messages::TNodeId& aSubConnId, ESock::CProtocolIntfBase* aProtocolIntf);

	};

#endif // HUNGRYNIFFLOW_H_INCLUDED_
