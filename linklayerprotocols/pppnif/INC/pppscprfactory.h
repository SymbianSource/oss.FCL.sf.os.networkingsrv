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
// PPP SCpr Factory
// 
//

/**
 @file
 @internalComponent
*/

#ifndef SYMBIAN_PPPSCPR_FACTORY_H
#define SYMBIAN_PPPSCPR_FACTORY_H

#include <comms-infras/ss_subconnprov.h>

class CPppSubConnectionProvider;

class CPppSubConnectionProviderFactory : public ESock::CSubConnectionProviderFactoryBase
	{
public:
    enum { iUid = 0x102822FC };
	static CPppSubConnectionProviderFactory* NewL(TAny* aParentContainer);

protected:
	CPppSubConnectionProviderFactory(TUid aFactoryId, ESock::CSubConnectionFactoryContainer& aParentContainer);
	ESock::ACommsFactoryNodeId* DoCreateObjectL(ESock::TFactoryQueryBase& aQuery);
	};

#endif
// SYMBIAN_PPPSCPR_FACTORY_H

