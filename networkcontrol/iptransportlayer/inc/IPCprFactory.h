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
// IP Connection Provider factory class definition.
// 
//

/**
 @file
 @internalComponent
*/

#ifndef SYMBIAN_IPCPR_FACTORY_H
#define SYMBIAN_IPCPR_FACTORY_H

#include <comms-infras/ss_subconnprov.h>


NONSHARABLE_CLASS(CIPConnectionProviderFactory) : public ESock::CConnectionProviderFactoryBase
	{
public:
    enum { iUid = 0x102070EF };
	IMPORT_C static CIPConnectionProviderFactory* NewL(TAny* aParentContainer);

protected:
	CIPConnectionProviderFactory(TUid aFactoryId, ESock::CConnectionFactoryContainer& aParentContainer);
	virtual ESock::ACommsFactoryNodeId* DoCreateObjectL(ESock::TFactoryQueryBase& aQuery);
	};

#endif
//SYMBIAN_IPCPR_FACTORY_H
