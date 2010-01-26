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

#include "nif6.h"
#include "notify.h"
#include "panic.h"
#include "IPProtoDeMux.h"

using namespace ESock;

//
// CIPShimIfBase methods //
//

CIPShimIfBase6* CIPShimIfBase6::NewL(const TDesC8& aProtocolName)
	{
	CIPShimIfBase6* p = new (ELeave) CIPShimIfBase6(aProtocolName);
	CleanupStack::PushL(p);
	p->ConstructL();
	CleanupStack::Pop(p);
	return p;
	}

CIPShimIfBase6::CIPShimIfBase6(const TDesC8& aProtocolName)
  : CIPShimIfBase(aProtocolName)
	{
	iConfig6.iFamily = KAFUnspec;
	}

TInt CIPShimIfBase6::ServiceInfoControl(TDes8& aOption, TUint aName)
/**
Service KSoIfInfo/KSoIfInfo6 calls from upper protocol
*/
	{
	TBinderInfo* ourInfo = NULL;
	// Validate the size of structure passed in
	switch (aName)
		{
	case KSoIfInfo:
		return KErrNotSupported;
			
	case KSoIfInfo6:
		ASSERT(aOption.Length() == sizeof(TSoIfInfo6));
		if (iConfig6.iFamily != KAfInet6)
			{
			return KErrNotSupported;
			}
		ourInfo = &iConfig6.iInfo;
		break;
	default:
		Panic(EBadInfoControlOption);
		}

	TUint8* ptr = const_cast<TUint8*>(aOption.Ptr());
	TSoIfInfo& info = reinterpret_cast<TSoIfInfo&>(*ptr);

	// Fill in iFeatures, iMtu, iSpeedMetric
	info.iFeatures = ourInfo->iFeatures;
	info.iSpeedMetric = ourInfo->iSpeedMetric;
	info.iMtu = ourInfo->iMtu;
	
	if (aName == KSoIfInfo6)
		{
		TSoIfInfo6& info = reinterpret_cast<TSoIfInfo6&>(*ptr);
		// For IP6, also fill in iRMtu
		info.iRMtu = ourInfo->iRMtu;
		}
		
	return KErrNone;
	}

TInt CIPShimIfBase6::ServiceConfigControl(TDes8& aOption)
/**
Service KSoIfConfig calls from upper protocol
*/
	{
	TUint8* ptr = const_cast<TUint8*>(aOption.Ptr());

	TUint family = reinterpret_cast<TSoIfConfigBase&>(*ptr).iFamily;
	
	switch (family)
		{
	case KAfInet6:					// IP6
		{
		ASSERT(aOption.Length() == sizeof(TSoInet6IfConfig));
		TSoInet6IfConfig& cf = reinterpret_cast<TSoInet6IfConfig&>(*ptr);
		
		if (iConfig6.iFamily != KAfInet6)
			{
			return KErrNotSupported;
			}
		else
			{
			cf.iLocalId = iConfig6.iLocalId;
			cf.iRemoteId = iConfig6.iRemoteId;
			cf.iNameSer1 = iConfig6.iNameSer1;
			cf.iNameSer2 = iConfig6.iNameSer2;
			}
		}
		break;
	
	default:
		Panic(EBadConfigControlFamily);
		break;
		}
		
	return KErrNone;
	}


void CIPShimIfBase6::GetConfigFirstTime()
/**
Retrieve the IP4/IP6 configuration information from the lower binder, if
we haven't already done so.
*/
	{
	if (iConfig6.iFamily == KAFUnspec)
		{
		// retrieve the configuration information into the iConfig4/6 union (we are
		// just specifying the address of the first data type in the union, but
		// it equally specifies iConfig6).
		
		TBinderConfig& config = iConfig6;
		if(iProtoBinders[0]->iLowerControl->GetConfig(config) != KErrNone)
			Panic(EBinderConfigNotSupported);

		iConfig6.iFamily = config.iFamily;
		ASSERT(iConfig6.iFamily == KAfInet6);
		}		
	}

