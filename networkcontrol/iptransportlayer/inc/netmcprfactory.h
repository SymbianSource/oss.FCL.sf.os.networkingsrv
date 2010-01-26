// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

/**
 @file
 @internalTechnology
 @prototype
*/

#ifndef SYMBIAN_NETMCPR_FACTORY_H
#define SYMBIAN_NETMCPR_FACTORY_H

#include <comms-infras/ss_metaconnprov.h>
#include <in_sock.h>

NONSHARABLE_CLASS(CNetworkMetaConnectionProviderFactory) : public ESock::CMetaConnectionProviderFactoryBase
	{
public:
    enum { iUid = 0x10274547 };
	IMPORT_C static CNetworkMetaConnectionProviderFactory* NewL(TAny* aParentContainer);
	
protected:
	CNetworkMetaConnectionProviderFactory(TUid aFactoryUid, ESock::CMetaConnectionFactoryContainer& aParentContainer);
	ESock::ACommsFactoryNodeId* DoCreateObjectL(ESock::TFactoryQueryBase& aQuery);
	};

#endif
//SYMBIAN_NETMCPR_FACTORY_H
