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
// IPProto Connection Provider factory class definition.
// 
//

/**
 @file
 @internalComponent
*/

#ifndef SYMBIAN_IPPROTOCPR_FACTORY_H
#define SYMBIAN_IPPROTOCPR_FACTORY_H


#include <comms-infras/ss_subconnprov.h>
#include <comms-infras/ss_nodemessages_legacy.h>
#include <comms-infras/ss_legacyinterfaces.h>

class CIPProtoConnectionProviderFactory : public ESock::CConnectionProviderFactoryBase,
                                          public ESock::MLinkCprFactApiExt,
                                          protected NetInterfaces::TInterfaceControl,
                                          public ITFHIERARCHY_1(CIPProtoConnectionProviderFactory, ESock::MLinkCprFactApiExt)
	{
public:
	typedef ITFHIERARCHY_1(CIPProtoConnectionProviderFactory, ESock::MLinkCprFactApiExt) TIfStaticFetcherNearestInHierarchy;
public:
    enum { iUid = 0x10281DD3 };
	static CIPProtoConnectionProviderFactory* NewL(TAny* aParentContainer);
    void ReturnInterfacePtrL(ESock::MLinkCprFactApiExt*& aInterface);
	virtual void EnumerateConnectionsL(RPointerArray<ESock::TSourcedConnectionInfo>& aConnectionInfoPtrArray);
    NetInterfaces::TInterfaceControl* DoFetchInterfaceControlL(TSupportedCommsApiExt aInterfaceId);

protected:
	CIPProtoConnectionProviderFactory(TUid aFactoryId, ESock::CConnectionFactoryContainer& aParentContainer);
	virtual ESock::ACommsFactoryNodeId* DoCreateObjectL(ESock::TFactoryQueryBase& aQuery);
	};

#endif
//SYMBIAN_IPPROTOCPR_FACTORY_H
