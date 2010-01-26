// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Implements Network Config Extension for MobileIP
// Note that most of the functionality is in the CNetworkConfigExtensionBase class,
// so no overrides as yet.
// 
//

/**
 @file NIFConfigurationControl.cpp
 @internalTechnology
*/

#include "NetCfgExtnMip.h"
#include <comms-infras/nifif.h>
#include <comms-infras/ca_startserver.h>
#include "cdbcols.h"
#include <ecom/implementationproxy.h>
#include <comms-infras/commsdebugutility.h>

CNetworkConfigExtensionMip* CNetworkConfigExtensionMip::NewL( TAny* aMNifIfNotify )
	{
	MNifIfNotify* nifIfNotify = reinterpret_cast<MNifIfNotify*>(aMNifIfNotify);
	CNetworkConfigExtensionMip* pDaemon = new(ELeave)CNetworkConfigExtensionMip( *nifIfNotify );
	CleanupStack::PushL(pDaemon);
	pDaemon->ConstructL();
	CleanupStack::Pop(pDaemon);
	return pDaemon;
	}
    
void CNetworkConfigExtensionMip::ConstructL()
	{
	CNetworkConfigExtensionBase::ConstructL();
	}	
	
CNetworkConfigExtensionMip::~CNetworkConfigExtensionMip()
/**
~CNetworkConfigExtensionMip - destructor
@internalTechnology
**/
	{
	}	

// Define the interface UIDs
const TImplementationProxy ImplementationTable[] = 
    {
    IMPLEMENTATION_PROXY_ENTRY(0x102032C3, CNetworkConfigExtensionMip::NewL)
    };

EXPORT_C const TImplementationProxy* ImplementationGroupProxy(TInt& aTableCount)
    {
    aTableCount = sizeof(ImplementationTable) / sizeof(TImplementationProxy);

    return ImplementationTable;
    }
