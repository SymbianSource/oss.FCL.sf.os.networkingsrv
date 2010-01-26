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

#include "ItfInfoConfigExt.h"
#include "IPProtoCPRFactory.h"
#include "IPProtoCPR.h"
#include <in_sock.h>
#include <elements/nm_interfaces.h>

#include <comms-infras/ss_legacyinterfaces.h>
#include <comms-infras/ss_msgintercept.h>

using namespace Messages;
using namespace ESock;
using namespace Factories;

//-=========================================================
//
// CIPProtoConnectionProviderFactory methods
//
//-=========================================================
CIPProtoConnectionProviderFactory* CIPProtoConnectionProviderFactory::NewL(TAny* aParentContainer)
    {
    return new (ELeave) CIPProtoConnectionProviderFactory(TUid::Uid(CIPProtoConnectionProviderFactory::iUid), *reinterpret_cast<ESock::CConnectionFactoryContainer*>(aParentContainer));
    }

CIPProtoConnectionProviderFactory::CIPProtoConnectionProviderFactory(TUid aFactoryId, ESock::CConnectionFactoryContainer& aParentContainer)
:	ESock::CConnectionProviderFactoryBase(aFactoryId, aParentContainer),
	TIfStaticFetcherNearestInHierarchy(this)
    {
    }

class TEnumerateConnectionsQuery : public MFactoryQuery
	{
public:
    TEnumerateConnectionsQuery(RPointerArray<TSourcedConnectionInfo>& aConnectionInfoPtrArray)
    :iConnectionInfoPtrArray(aConnectionInfoPtrArray)
    {}

	virtual TMatchResult Match(TFactoryObjectInfo& aObjectInfo);
	inline TInt Error() {return iError;}
private:
    RPointerArray<TSourcedConnectionInfo>& iConnectionInfoPtrArray;
    TInt iError;
	};

MFactoryQuery::TMatchResult TEnumerateConnectionsQuery::Match(TFactoryObjectInfo& aObjectInfo)
    {
    //This is a legacy feature
	CIPProtoConnectionProvider* prov = static_cast<CIPProtoConnectionProvider*>(aObjectInfo.iInfo.iFactoryObject);
	iError = KErrNone;
	if (prov)
    	{
        RNodeInterface* dc = prov->GetFirstClient<TDefaultClientMatchPolicy>(TClientType(TCFClientType::EData, TCFClientType::EDefault));
    	if (dc && /*dc->Flags() & RNodeInterface::EStarted &&*/
    	        !(dc->Flags() & TClientType::ELeaving))
    		{
    		const TItfInfoConfigExt* ext = static_cast<const TItfInfoConfigExt*>(prov->AccessPointConfig().FindExtension(STypeId::CreateSTypeId(KIpProtoConfigExtUid, EItfInfoConfigExt)));
    		ASSERT(ext);
            if (ext)
                {
                TSourcedConnectionInfo* copy;
                copy = new ESock::TSourcedConnectionInfo(ext->iConnectionInfo.iIapId, ext->iConnectionInfo.iNetId, EConnectionGeneric, prov->Id());
                if(copy == NULL)
                	{
                	iError = KErrNoMemory;
                	return MFactoryQuery::ECancel;
                	}
                	
                TInt err = iConnectionInfoPtrArray.Append(copy);
				if(err != KErrNone)
					{
					delete copy;
                	iError = err;
                	return MFactoryQuery::ECancel;
					}
                }
	        }
    	}
	return MFactoryQuery::EContinue;
    }

void CIPProtoConnectionProviderFactory::EnumerateConnectionsL(RPointerArray<ESock::TSourcedConnectionInfo>& aConnectionInfoPtrArray)
    {
    TEnumerateConnectionsQuery query(aConnectionInfoPtrArray);
    Find(query);
	User::LeaveIfError(query.Error());
    }

void CIPProtoConnectionProviderFactory::ReturnInterfacePtrL(ESock::MLinkCprFactApiExt*& aInterface)
    {
    aInterface = this;
    }

NetInterfaces::TInterfaceControl* CIPProtoConnectionProviderFactory::DoFetchInterfaceControlL(TSupportedCommsApiExt aInterfaceId)
    {
    ASSERT(aInterfaceId == ESock::MLinkCprFactApiExt::KInterfaceId);
    (void)aInterfaceId;
    return this;
    }

ESock::ACommsFactoryNodeId* CIPProtoConnectionProviderFactory::DoCreateObjectL(ESock::TFactoryQueryBase& /* aQuery */)
    {
	CConnectionProviderBase* provider = CIPProtoConnectionProvider::NewL(*this);

	ESOCK_DEBUG_REGISTER_GENERAL_NODE(iUid, provider);

	return provider;
    }






