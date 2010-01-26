// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// CNifIfBase and CProtocolBase shim layer functionality
// 
//

/**
 @file nif.cpp
*/

#include <e32base.h>
#include <comms-infras/ss_subconnflow.h>
#include <comms-infras/ss_protflow.h>

#include "in_sock.h"
#include "in_iface.h"
#include "in6_if.h"

#include "nif4.h"
#include "notify.h"
#include "panic.h"
#include "IPProtoDeMux.h"

using namespace ESock;

//
// CIPShimIfBase methods //
//

CIPShimIfBase4* CIPShimIfBase4::NewL(const TDesC8& aProtocolName)
	{
	CIPShimIfBase4* p = new (ELeave) CIPShimIfBase4(aProtocolName);
	CleanupStack::PushL(p);
	p->ConstructL();
	CleanupStack::Pop(p);
	return p;
	}

CIPShimIfBase4::CIPShimIfBase4(const TDesC8& aProtocolName)
  : CIPShimIfBase(aProtocolName)
	{
	iConfig4.iFamily = KAFUnspec;
	}

TInt CIPShimIfBase4::ServiceInfoControl(TDes8& aOption, TUint aName)
/**
Service KSoIfInfo/KSoIfInfo6 calls from upper protocol
*/
	{
	TBinderInfo* ourInfo = NULL;
	// Validate the size of structure passed in
	switch (aName)
		{
	case KSoIfInfo:
		ASSERT(aOption.Length() == sizeof(TSoIfInfo));
		if (iConfig4.iFamily != KAfInet)
			{
			return KErrNotSupported;
			}
		ourInfo = &iConfig4.iInfo;
		break;
	case KSoIfInfo6:
		return KErrNotSupported;
	
	default:
		Panic(EBadInfoControlOption);
		}

	TUint8* ptr = const_cast<TUint8*>(aOption.Ptr());
	TSoIfInfo& info = reinterpret_cast<TSoIfInfo&>(*ptr);

	// Fill in iFeatures, iMtu, iSpeedMetric
	info.iFeatures = ourInfo->iFeatures;
	info.iSpeedMetric = ourInfo->iSpeedMetric;
	info.iMtu = ourInfo->iMtu;
	
	return KErrNone;
	}

TInt CIPShimIfBase4::ServiceConfigControl(TDes8& aOption)
/**
Service KSoIfConfig calls from upper protocol
*/
	{
	TUint8* ptr = const_cast<TUint8*>(aOption.Ptr());

	TUint family = reinterpret_cast<TSoIfConfigBase&>(*ptr).iFamily;
	
	switch (family)
		{
	case KAfInet:					// IP4
		{
		ASSERT(aOption.Length() == sizeof(TSoInetIfConfig));
		TSoInetIfConfig& cf = reinterpret_cast<TSoInetIfConfig&>(*ptr);
		
		if (iConfig4.iFamily != KAfInet)
			{
			return KErrNotSupported;
			}
		else 
			{
			cf.iConfig.iAddress = iConfig4.iAddress;
			cf.iConfig.iNetMask = iConfig4.iNetMask;
			cf.iConfig.iBrdAddr = iConfig4.iBrdAddr;
			cf.iConfig.iDefGate = iConfig4.iDefGate;
			cf.iConfig.iNameSer1 = iConfig4.iNameSer1;
			cf.iConfig.iNameSer2 = iConfig4.iNameSer2;
			}
		}
		break;
		
	default:
		Panic(EBadConfigControlFamily);
		break;
		}
		
	return KErrNone;
	}


void CIPShimIfBase4::GetConfigFirstTime()
/**
Retrieve the IP4/IP6 configuration information from the lower binder, if
we haven't already done so.
*/
	{
	if (iConfig4.iFamily == KAFUnspec)
		{
		TBinderConfig& config = iConfig4;
		if(iProtoBinders[0]->iLowerControl->GetConfig(config) != KErrNone)
			Panic(EBinderConfigNotSupported);

		iConfig4.iFamily = config.iFamily;
		ASSERT(iConfig4.iFamily == KAfInet);
		}		
	}
