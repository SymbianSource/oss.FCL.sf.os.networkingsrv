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

#include <nifman.h>
#include <comms-infras/nifif.h>
#include <in_iface.h>

#include "qos_if.h"
#include "qos_prot.h"
#include "modules.h"
#include "interface.h"


CInterface* CInterface::NewL(CProtocolQoS& aProt, CNifIfBase* aNif)
	{
	CInterface* iface = new (ELeave) CInterface(aProt, aNif);
	CleanupStack::PushL(iface);
	iface->ConstructL();
	CleanupStack::Pop();
	//coverity[leave_without_push]
	LOG(Log::Printf(_L("\t\tIF [%S] '%S' created"), &iface->iName, &iface->iInterfaceName));
	return iface;
	}


CInterface::CInterface(CProtocolQoS& aProt, CNifIfBase* aNif) : iProtocol(aProt), iNif(aNif)
	{
	LOG(Log::Printf(_L("new\tqos IF for NIF[%u] size=%d"), (TInt)aNif, sizeof(CInterface)));
	iIapId = 0;
	}


CInterface::~CInterface()
	{
	LOG(Log::Printf(_L("~\tqos IF [%S] destructor -- start"), &iName));
	
	if (iTrafficControl)
		{
		iTrafficControl->Module()->InterfaceDetached(iInterfaceName, iNif);
		delete iTrafficControl;
		}

	LOG(Log::Printf(_L("~\tqos IF [%S] destructor -- end"), &iName));
	}

void CInterface::ConstructL()
	{
	iTrafficControl = NULL;
	iIapId = 0;
	GetInterfaceInfo();
	// If the control module is not available, for some reason,
	// then this status will cause all channel
	// negotiations for this interface to fail, but does not
	// prevent normal packet flow.
	iTrafficControlStatus = LoadControlModule();
	}


void CInterface::GetInterfaceInfo()
	{
	TNifIfInfo info;
	iNif->Info(info);
	iName = info.iName;

	TInt aPlace;
	TChar aMark('.');
	aPlace = iName.LocateReverse(aMark);
	if (aPlace > 0)
		iInterfaceName.Set(iName.Left(aPlace));
	else
		iInterfaceName.Set(iName);

	// Get NetworkId and IapId
	TSoIfConnectionInfo netinfo;
	TPckg<TSoIfConnectionInfo> option(netinfo);
	if (iNif->Control(KSOLInterface, KSoIfGetConnectionInfo, option) == KErrNone)
		iIapId = netinfo.iIAPId;
	LOG(Log::Printf(_L("\t\tIF [%S] '%S' NET=%d IAP=%d"), &iName, &iInterfaceName, netinfo.iNetworkId, iIapId));
	}


TInt CInterface::GetPlugIn(TSoIfControllerInfo& aPluginInfo)
	{
	TPckg<TSoIfControllerInfo> opt(aPluginInfo);
	TInt ret;

	ret = iNif->Control(KSOLInterface, KSoIfControllerPlugIn, opt);
	if (!ret)
		{
		_LIT(KDescPrt, ".prt");
		TInt aPlace;
		aPlace = aPluginInfo.iPlugIn.Match(KDescPrt);
		if (aPlace == KErrNotFound)
			aPluginInfo.iPlugIn.Append(KDescPrt);
		}
	return ret;
	}


// Load technology-specific control module
TInt CInterface::LoadControlModule()
	{
	TInt ret;
	TSoIfControllerInfo aPluginInfo;
	ret = GetPlugIn(aPluginInfo);
	if (ret == KErrNone)
		{
		RModule* module = NULL;
		TRAP(ret, module = iProtocol.ModuleMgr()->LoadModuleL(iProtocol.NetworkService()->Protocol(), aPluginInfo.iPlugIn, aPluginInfo.iProtocolId));
		if (ret == KErrNone)
			{
			SetTrafficControl(module);
			LOG(Log::Printf(_L("\t\tCalling module[%S]::InterfaceAttached(IF [%S])"), &aPluginInfo.iPlugIn, &iName));
			module->Module()->InterfaceAttached(iInterfaceName, iNif);
			}
		else
			{
			LOG(Log::Printf(_L("\t\tFailed to load module[%S] for IF [%S] , err=%d"), &aPluginInfo.iPlugIn, &iName, ret));
			}
		}
	return ret;
	}

CInterfaceManager* CInterfaceManager::NewL(CProtocolQoS& aProtocol)
	{
	return new (ELeave) CInterfaceManager(aProtocol);
	}

CInterfaceManager::CInterfaceManager(CProtocolQoS& aProtocol) : iProtocol(aProtocol)
	{
	LOG(Log::Printf(_L("new\tqos Interface Manager[%u] size=%d"), (TInt)this, sizeof(CInterfaceManager)));
	iInterfaces.SetOffset(_FOFF(CInterface, iLink));
	}

CInterfaceManager::~CInterfaceManager()
	{
	LOG(Log::Printf(_L("~\tqos Interface Manager[%u] -- start"), (TInt)this));
	while (!iInterfaces.IsEmpty())
		{
		CInterface* iface = iInterfaces.First();
		iInterfaces.Remove(*iface);
		delete iface;
		}
	iInterfaces.Reset();
	LOG(Log::Printf(_L("~\tqos Interface Manager[%u] -- done"), (TInt)this));
	}

CInterface* CInterfaceManager::AddInterfaceL(CNifIfBase* aNif)
	{
	CInterface* iface = CInterface::NewL(iProtocol, aNif);
	iInterfaces.AddLast(*iface);
	return iface;
	}

void CInterfaceManager::RemoveInterface(CInterface* aInterface)
	{
	iInterfaces.Remove(*aInterface);
	delete aInterface;
	}


CInterface* CInterfaceManager::FindInterface(CNifIfBase* aNif)
	{
	TSglQueIter<CInterface> iter(iInterfaces);
	CInterface* i;

	while ((i = iter++) != NULL)
		{
		if (i->Nif() == aNif)
			return i;
		}
	return NULL;
	}
