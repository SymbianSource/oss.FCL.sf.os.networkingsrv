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
// IP SubConnection Provider factory class definition.
// 
//

/**
 @file
 @internalComponent
*/

#ifndef SYMBIAN_IPSCPR_FACTORY_H
#define SYMBIAN_IPSCPR_FACTORY_H

#include <comms-infras/ss_subconnprov.h>

NONSHARABLE_CLASS(CIpSubConnectionProviderFactory) : public ESock::CSubConnectionProviderFactoryBase
	{
public:
    enum { iUid = 0x102822D3 };
	IMPORT_C static CIpSubConnectionProviderFactory* NewL(TAny* aParentContainer);
	~CIpSubConnectionProviderFactory();
	
protected:
	CIpSubConnectionProviderFactory(TUid aFactoryId, ESock::CSubConnectionFactoryContainer& aParentContainer);
	virtual ESock::ACommsFactoryNodeId* DoCreateObjectL(ESock::TFactoryQueryBase& aQuery);
	};

#endif
//SYMBIAN_IPSCPR_FACTORY_H
