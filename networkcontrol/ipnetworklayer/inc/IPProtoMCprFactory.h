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
// IPProto MCPR Factory
// 
//

/**
 @file
 @internalComponent
*/

#ifndef SYMBIAN_IPPROTOMCPR_FACTORY_H
#define SYMBIAN_IPPROTOMCPR_FACTORY_H

#include <comms-infras/ss_metaconnprov.h>

class CIPProtoMetaConnectionProviderFactory : public ESock::CMetaConnectionProviderFactoryBase
	{
public:
    enum { iUid = 0x10281DEE };
	static CIPProtoMetaConnectionProviderFactory* NewL(TAny* aParentContainer);
	
protected:
	CIPProtoMetaConnectionProviderFactory(TUid aFactoryUid, ESock::CMetaConnectionFactoryContainer& aParentContainer);
	ESock::ACommsFactoryNodeId* DoCreateObjectL(ESock::TFactoryQueryBase& aQuery);
	};

#endif
//SYMBIAN_IPPROTOMCPR_FACTORY_H
