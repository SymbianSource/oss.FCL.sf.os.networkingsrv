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
// DummyProto MCPR Factory
// 
//

/**
 @file
 @internalComponent
*/

#ifndef SYMBIAN_DUMMYPROTOMCPR_FACTORY_H
#define SYMBIAN_DUMMYPROTOMCPR_FACTORY_H

#include <comms-infras/ss_metaconnprov.h>

class CDummyProtoMetaConnectionProviderFactory : public ESock::CMetaConnectionProviderFactoryBase
	{
public:
    enum { iUid = 0x10281E04 };
	static CDummyProtoMetaConnectionProviderFactory* NewL(TAny* aParentContainer);

protected:
	CDummyProtoMetaConnectionProviderFactory(TUid aFactoryId, ESock::CMetaConnectionFactoryContainer& aParentContainer);
	ESock::ACommsFactoryNodeId* DoCreateObjectL(ESock::TFactoryQueryBase& aQuery);
	};

#endif
//SYMBIAN_DUMMYPROTOMCPR_FACTORY_H
