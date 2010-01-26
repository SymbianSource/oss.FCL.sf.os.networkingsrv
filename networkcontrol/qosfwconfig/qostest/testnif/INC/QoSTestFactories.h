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
// QosTest Factories
// 
//

/**
 @file
 @internalComponent
*/

#ifndef QOSTESTFACTORIES_H
#define QOSTESTACTORIES_H

#include <ss_subconnflow.h>

const TInt KQosTestFlowImplUid = 0x10281C54;

class CQosTestSubConnectionFlowFactory : public ESock::CSubConnectionFlowFactoryBase
/**
QosTest flow factory.

@internalComponent
*/
	{
public:
	static CQosTestSubConnectionFlowFactory* NewL(TAny* aConstructionParameters);
    virtual ESock::CSubConnectionFlowBase* DoCreateFlowL(ESock::CProtocolIntfBase* aProtocolIntf, ESock::TFactoryQueryBase& aQuery);
protected:
	CQosTestSubConnectionFlowFactory(TUid aFactoryId, ESock::CSubConnectionFlowFactoryContainer& aParentContainer);
	};


#endif
//QOSTESTFACTORIES_H
