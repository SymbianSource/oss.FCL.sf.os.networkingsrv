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
// IPProto Tier Manager Factory
// 
//

/**
 @file
 @internalComponent
*/

#include "IPProtoTierManagerFactory.h"
#include "IPProtoTierManager.h"
#include <comms-infras/ss_log.h>

#ifdef _DEBUG
#define KIPProtoTierMgrTag KESockMetaConnectionTag
_LIT8(KIPProtoTierMgrSubTag, "ipprototiermgr");
#endif

using namespace ESock;

//-=========================================================
//
// CIPProtoTierManagerFactory implementation
//
//-=========================================================	
CIPProtoTierManagerFactory* CIPProtoTierManagerFactory::NewL(TAny* aParentContainer)
	{
	__CFLOG_VAR((KIPProtoTierMgrTag, KIPProtoTierMgrSubTag, _L8("CIPProtoTierManagerFactory::\tNewL(%08x)"), aParentContainer));
 	return new (ELeave) CIPProtoTierManagerFactory(TUid::Uid(CIPProtoTierManagerFactory::iUid),TUid::Uid(CIPProtoTierManagerFactory::iUid),*(reinterpret_cast<CTierManagerFactoryContainer*>(aParentContainer)));
	}

CIPProtoTierManagerFactory::CIPProtoTierManagerFactory(TUid aTierTypeId,TUid aFactoryUid, ESock::CTierManagerFactoryContainer& aParentContainer)
:	CTierManagerFactoryBase(aTierTypeId,aFactoryUid,aParentContainer)
	{
	__CFLOG_VAR((KIPProtoTierMgrTag, KIPProtoTierMgrSubTag, _L8("CIPProtoTierManagerFactory %08x:\tCIPProtoTierManagerFactory"), this));
	}

ESock::ACommsFactoryNodeId* CIPProtoTierManagerFactory::DoCreateObjectL(ESock::TFactoryQueryBase& /* aQuery */)
	{
	return CIPProtoTierManager::NewL(*this);
	}
