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
// IPProto SubConnection Provider factory class definition.
// 
//

/**
 @file
 @internalComponent
*/

#ifndef SYMBIAN_IPPROTODEFTSCPR_FACTORY_H
#define SYMBIAN_IPPROTODEFTSCPR_FACTORY_H

#include <comms-infras/ss_subconnprov.h>

class CIPProtoSubConnectionProviderFactory : public ESock::CSubConnectionProviderFactoryBase
	{
public:
    enum { iUid = 0x10281DD1 };
	static CIPProtoSubConnectionProviderFactory* NewL(TAny* aParentContainer);
	~CIPProtoSubConnectionProviderFactory();

protected:
	CIPProtoSubConnectionProviderFactory(TUid aFactoryId, ESock::CSubConnectionFactoryContainer& aParentContainer);
	virtual ESock::ACommsFactoryNodeId* DoCreateObjectL(ESock::TFactoryQueryBase& aQuery);
	};

#endif
//SYMBIAN_IPPROTODEFTSCPR_FACTORY_H
