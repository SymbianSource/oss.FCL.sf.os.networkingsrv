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
// PPP Factories
// 
//

/**
 @file
 @internalComponent
*/

#ifndef PPPFACTORIES_H
#define PPPFACTORIES_H

#include <comms-infras/ss_protflow.h>
#include <comms-infras/ss_subconnflow.h>

const TInt KPPPFlowImplUid = 0x10281C4F;

class CPPPSubConnectionFlowFactory : public ESock::CSubConnectionFlowFactoryBase
/**
PPP flow factory.

@internalComponent
*/
	{
public:
	static CPPPSubConnectionFlowFactory* NewL(TAny* aConstructionParameters);
	virtual ESock::CSubConnectionFlowBase* DoCreateFlowL(ESock::CProtocolIntfBase* aProtocolIntf, ESock::TFactoryQueryBase& aQuery);
protected:
	CPPPSubConnectionFlowFactory(TUid aFactoryId, ESock::CSubConnectionFlowFactoryContainer& aParentContainer);
	};

#endif
//PPPFACTORIES_H

