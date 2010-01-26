// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// User Prompt Service (UPS) support.
// 
//

#define SYMBIAN_NETWORKING_UPS

#ifdef SYMBIAN_NETWORKING_UPS

#include "netmcprups.h"
#include "netmcpractivities.h"		// for NetMCprActivities::netMCprActivities
#include <comms-infras/ss_log.h>
#include <comms-infras/netups.h>
#include <comms-infras/netupsserviceid.h>

#ifdef __CFLOG_ACTIVE
#define KNetMCprTag KESockMetaConnectionTag
_LIT8(KNetMCprSubTag, "netmcpr");
#endif

using namespace ESock;

CUpsNetworkMetaConnectionProvider::CUpsNetworkMetaConnectionProvider(CMetaConnectionProviderFactoryBase& aFactory, const TProviderInfo& aProviderInfo, const MeshMachine::TNodeActivityMap& aActivityMap)
:	CNetworkMetaConnectionProvider(aFactory,aProviderInfo,aActivityMap)
	{
//	LOG_NODE_CREATE(KNetMCprTag, CNetworkMetaConnectionProvider);
	}

CUpsNetworkMetaConnectionProvider* CUpsNetworkMetaConnectionProvider::NewL(CMetaConnectionProviderFactoryBase& aFactory, const TProviderInfo& aProviderInfo)
	{
	__CFLOG_VAR((KNetMCprTag, KNetMCprSubTag, _L8("CUpsNetworkMetaConnectionProvider:\tNewL()")));

	CUpsNetworkMetaConnectionProvider* self = new (ELeave) CUpsNetworkMetaConnectionProvider(aFactory,aProviderInfo,NetMCprActivities::netMCprActivities::Self());
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CUpsNetworkMetaConnectionProvider::~CUpsNetworkMetaConnectionProvider()
	{
	FreeApName();
	CloseNetUps();
	
	iUpsClientHandleRefCount.ResetAndDestroy();
	iUpsClientHandleRefCount.Close();
	}

void CUpsNetworkMetaConnectionProvider::ConstructL()
	{
	CNetworkMetaConnectionProvider::ConstructL();
	
	if (!NetUps::CNetUps::UpsDisabled(NetUps::EIpServiceId))
		{
		RMetaExtensionContainer mec;
		mec.Open(AccessPointConfig());
		CleanupClosePushL(mec);
		//
		// UPS is enabled - provision the UPS specific Access Point Config extension to the layer
		//
		CUPSAccessPointConfigExt* accessPointConfigExt = CUPSAccessPointConfigExt::NewL(ETSockAddress);
		CleanupStack::PushL(accessPointConfigExt);
		// set UPS specific ServiceId	
		accessPointConfigExt->SetServiceId(NetUps::EIpServiceId);
		mec.AppendExtensionL(accessPointConfigExt);
		CleanupStack::Pop(accessPointConfigExt);
		
		AccessPointConfig().Close();
		AccessPointConfig().Open(mec);
		CleanupStack::PopAndDestroy(&mec);
		}
	else
		{
		// UPS is disabled.
		SetUpsDisabled(ETrue);
		}
	} 

void CUpsNetworkMetaConnectionProvider::ShowAccessPointRecordL(CommsDat::CMDBSession* /*aSession*/, CommsDat::CCDAccessPointRecord* aApRec)
/**
Called from base class during reading of Access Point Record.

Store away the Access Point Name locally.
*/
	{
	SetApNameL(aApRec->iRecordName);
	}

void CUpsNetworkMetaConnectionProvider::CloseNetUps()
	{
	delete NetUps();
	SetNetUps(NULL);
	}

void  CUpsNetworkMetaConnectionProvider::AddUpsClientCommsIdL(const Messages::TNodeId& aCommsId)
	{
	TInt32 index = 0;
	TInt32 count = 0;	
	TBool found = FindUpsClientHandle(aCommsId, index, count);
	
	if (found == EFalse)
		{
		TUpsClientHandleRefCount* upsClientHandleRefCount = new (ELeave) TUpsClientHandleRefCount(aCommsId, 0);
		CleanupStack::PushL(upsClientHandleRefCount);
		iUpsClientHandleRefCount.AppendL(upsClientHandleRefCount);
		CleanupStack::Pop(upsClientHandleRefCount);
		}		
	}

TBool CUpsNetworkMetaConnectionProvider::FindUpsClientHandle(const Messages::TNodeId& aCommsId, TInt32& aIndex, TInt32& aCount)
	{
	TBool found = EFalse;
	for (TInt i = iUpsClientHandleRefCount.Count() - 1; i >=0; --i)
		{
		if (iUpsClientHandleRefCount[i]->iCommsId == aCommsId)
			{
			aIndex = i;
			aCount = iUpsClientHandleRefCount[i]->iCount;
			found = ETrue;
			break;
			}
		}
	return found;	
	}

void CUpsNetworkMetaConnectionProvider::IncrementUpsClientHandle(const Messages::TNodeId& aCommsId)
	{
	TInt32 index = 0;
	TInt32 count = 0;
	TBool found = FindUpsClientHandle(aCommsId, index, count);
		
	if (found == EFalse)
		{
		// TODO: ADD a panic code User::Panic(KNetMCprPanic, KPanicNoConnectionHandle);
		User::Panic(KNetMCprPanic, KPanicNoConnectionHandle);
		}
	else
		{
		++(iUpsClientHandleRefCount[index]->iCount);
		}				
	}

void CUpsNetworkMetaConnectionProvider::DecrementUpsClientHandle(const Messages::TNodeId& aCommsId, TBool& aAllHandlesDeleted)
	{
	TInt32 index = 0;
	TInt32 count = 0;	
	TBool found = FindUpsClientHandle(aCommsId, index, count);
	
	if (found == EFalse)
		{		
		User::Panic(KNetMCprPanic, KPanicNoConnectionHandle);
		}
	else
		{
		iUpsClientHandleRefCount[index]->iCount--;
		if (iUpsClientHandleRefCount[index]->iCount == 0)
			{
			delete iUpsClientHandleRefCount[index];
			iUpsClientHandleRefCount.Remove(index);
			}
		}
	aAllHandlesDeleted = (iUpsClientHandleRefCount.Count() == 0) ? ETrue : EFalse;
	}

#endif //SYMBIAN_NETWORKING_UPS
