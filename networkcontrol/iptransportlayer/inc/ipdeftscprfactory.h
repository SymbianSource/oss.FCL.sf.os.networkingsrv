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

#ifndef SYMBIAN_IPDEFTSCPR_FACTORY_H
#define SYMBIAN_IPDEFTSCPR_FACTORY_H

#include <comms-infras/ss_subconnprov.h>

NONSHARABLE_CLASS(CIpDefaultSubConnectionProviderFactory) : public ESock::CSubConnectionProviderFactoryBase
	{
public:
    enum { iUid = 0x102752C5 };
	IMPORT_C static CIpDefaultSubConnectionProviderFactory* NewL(TAny* aParentContainer);
	~CIpDefaultSubConnectionProviderFactory();

protected:
	CIpDefaultSubConnectionProviderFactory(TUid aFactoryId, ESock::CSubConnectionFactoryContainer& aParentContainer);
	
	virtual ESock::ACommsFactoryNodeId* DoCreateObjectL(ESock::TFactoryQueryBase& aQuery);
	};

#endif
//SYMBIAN_IPSCPR_FACTORY_H
