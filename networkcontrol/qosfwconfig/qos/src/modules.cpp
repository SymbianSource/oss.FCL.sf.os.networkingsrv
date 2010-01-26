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
//

#include "qos_prot.h"
#include "policies.h"
#include "modules.h"

// CModuleData
CModuleData* CModuleData::NewLC(const TDesC& aModuleName, TUint aProtocolId, TUint aTimeoutDelay, TDblQue<CModuleData>& aList)
	{
	LOG(Log::Printf(_L("NewL\tqos module[%S] prot=%u delay=%u"), &aModuleName, aProtocolId, aTimeoutDelay));

	CModuleData* module = new (ELeave) CModuleData(aProtocolId, aTimeoutDelay);

	LOG(module->iName = aModuleName.Left(module->iName.MaxLength() < aModuleName.Length() ? module->iName.MaxLength() : aModuleName.Length()));

	aList.AddLast(*module);
	// Although the "module" is already recorded into the aList, the address is
	// still pushed into the cleanup stack. The destructor of the CModuleData
	// always removes the object the list, and if any of the following initializations
	// (including those in the caller of this method) fail, the half initialized
	// object will get destroyed (and not left in the list!).
	CleanupStack::PushL(module);
	module->ConstructL(aModuleName);
	return module;
	}

void CModuleData::ConstructL(const TDesC& aModuleName)
	{
	TCallBack callback(WatchDogCallBack, this);
	iTimeout = CModuleTimeout::NewL(callback);

	User::LeaveIfError(iLib.Load(aModuleName));
	TProtocolNew entry = (TProtocolNew)iLib.Lookup(1);
	if (!entry)
		User::Leave(KErrNoMemory);
	
	// Each module must be implemented as PRT module. One PRT
	// can contain one module (= protocol).
	// [Because the family and "protocol"
	// are created and deleted as a pair here].
	iFamily = (*entry)();
	if (!iFamily)
		User::Leave(KErrBadLibraryEntryPoint);
	iFamily->Install();

	iModule = (CModuleBase*)iFamily->NewProtocolL(KSockDatagram, iProtocolId);
	if (!iModule)
		User::LeaveIfError(KErrNotSupported);

	// Get the module capabilities from the module
	TInt flags=0;
	TPckg<TInt> option(flags);
	TInt ret = iModule->Configure(KSOLQoSModule, KSoCapabilities, option);
	if (ret == KErrNone)
		iFlags = flags;
	}

CModuleData::CModuleData(TUint aProtocolId, TUint aTimeoutDelay)
	{
	iTimeoutDelay = aTimeoutDelay;
	iProtocolId = aProtocolId;
	iModule = NULL;
	iFamily = NULL;
	iRefCount = 0;
	iFlags = 0;
	}

CModuleData::~CModuleData()
	{
	LOG(Log::Printf(_L("~\tqos module[%S] destructor -- start"), &iName));
	ASSERT(iRefCount == 0);	// Reference count MUST BE ZERO at this point!
	iLink.Deque();			// Remove from CModuleManager::iModuleList!
	delete iTimeout;		// ...cancels also the timer, if active.
	delete iModule;
	delete iFamily;
	iLib.Close();
	LOG(Log::Printf(_L("~\tqos module[%S] destructor -- end"), &iName));
	}

void CModuleData::Close()
	{
	ASSERT(iRefCount > 0);
	if (--iRefCount <= 0)
		{
		LOG(Log::Printf(_L("\tqos module[%S] StartWatchdog"), &iName));
		StartWatchDog();
		}
	}

TInt CModuleData::WatchDogCallBack(TAny* aProvider)
	{
	CModuleData* module = (CModuleData*)aProvider;
	LOG(Log::Printf(_L("<>\tqos module[%S] WatchDogCallback -- iRefCount=%d"), &module->iName, module->iRefCount));
	if (module->iRefCount<=0)
		{
		// No references have appeared while waiting -- delete object
		delete module;
		}
	return KErrNone;
	}


// CModuleManager
CModuleManager* CModuleManager::NewL(CProtocolQoS& aProtocol)
	{
	return new (ELeave) CModuleManager(aProtocol);
	}


CModuleManager::CModuleManager(CProtocolQoS& aProtocol) : iProtocol(aProtocol)
	{
	LOG(Log::Printf(_L("new\tqos Module Manager[%u] size=%d"), (TInt)this, sizeof(CModuleManager)));
	iModuleList.SetOffset(_FOFF(CModuleData, iLink));
	}

CModuleManager::~CModuleManager()
	{
	// UnloadAllModules();
	while (!iModuleList.IsEmpty())
		{
		CModuleData* module = iModuleList.First();
		delete module;	// ~CModuleData() deques the object from the iModuleList
		}
	LOG(Log::Printf(_L("~\tqos Module Manager[%u] deleted"), (TInt)this));
	}


RModule* CModuleManager::LoadModuleL(CModuleSpec& aModuleSpec, CProtocolBase* aIpProtocol)
	{
	return LoadModuleL(aIpProtocol, aModuleSpec.FileName(), aModuleSpec.ProtocolId(), aModuleSpec.PolicyData());
	}


RModule* CModuleManager::LoadModuleL(CProtocolBase* aIpProtocol, const TDesC& aModuleName, TUint aProtocolId, CExtension* aData)
	{
	TInt pops = 0;
	// ??? If a Lookup finds an existing object, does it matter if it was
	// ??? created with different value for aData (CExtension *) ?
	// ??? (Note: GUQOS currently ignores that parameter, and thus not a problem YET!)
	CModuleData* info = Lookup(aProtocolId);
	if (!info)
		{
		info = CModuleData::NewLC(aModuleName, aProtocolId, Protocol().ConfigOptions().iUnloadDelay, iModuleList);
		++pops;
		info->iModule->BindToIP6L(aIpProtocol);
		info->iModule->InitModuleL(Protocol(), aData);
		}
	RModule *module = new (ELeave) RModule(*info);
	// Note: Having the "Pop" here solves the case where CModuleData contruction is complete, but
	// the RModule allocation leaves. This prevents the case where the iModuleList contains object
	// with iRefCount == 0, but no timer running => it would not get deleted until module manager dies or
	// some reference is added and removed.
	CleanupStack::Pop(pops);
	return module;
	}

RModule* CModuleManager::OpenModuleL(TUint aProtocolId)
	{
	RModule* module=NULL;
	CModuleData* moduleData = Lookup(aProtocolId);
	if (moduleData)
		module = new (ELeave) RModule(*moduleData);
	return module;
	}


TBool CModuleManager::IsLoaded(TUint aProtocolId)
	{
	return (Lookup(aProtocolId) == NULL) ? EFalse : ETrue;
	}

void CModuleManager::Unbind(CProtocolBase* aProtocol, TInt aId)
	{
	TDblQueIter<CModuleData> iter(iModuleList);
	CModuleData* module;
	while ((module = iter++) != NULL)
		module->iModule->Unbind(aProtocol, aId);
	}


CModuleData* CModuleManager::Lookup(TUint aProtocolId)
	{
	TDblQueIter<CModuleData> iter(iModuleList);
	CModuleData* moduleData;

	while ((moduleData = iter++) != NULL)
		{
		if (moduleData->iProtocolId == aProtocolId)
			return moduleData;
		}
	return NULL;
	}

// RModule
RModule::RModule(CModuleData& aModuleData) : iModuleData(aModuleData)
	{ 
	iModuleData.Open();
	}

RModule::RModule(const RModule& aModule) : iModuleData(aModule.iModuleData)
	{
	iModuleData.Open();
	}

RModule::~RModule()
	{
	iModuleData.Close();
	}

