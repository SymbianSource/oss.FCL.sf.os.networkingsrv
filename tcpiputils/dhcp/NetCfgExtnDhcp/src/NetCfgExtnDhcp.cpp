// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Implements Network Config Extension for DHCP
// 
//

/**
 @file NIFConfigurationControl.cpp
 @internalTechnology
*/

#include "NetCfgExtnDhcp.h"
#include <comms-infras/nifif.h>
#include <comms-infras/ca_startserver.h>
#include "cdbcols.h"
#include <ecom/implementationproxy.h>
#include <comms-infras/commsdebugutility.h>
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <nifman_internal.h>
#endif


CNetworkConfigExtensionDhcp* CNetworkConfigExtensionDhcp::NewL( TAny* aMNifIfNotify )
	{
	MNifIfNotify* nifIfNotify = reinterpret_cast<MNifIfNotify*>(aMNifIfNotify);
	CNetworkConfigExtensionDhcp* pDaemon = new(ELeave)CNetworkConfigExtensionDhcp( *nifIfNotify );
	CleanupStack::PushL(pDaemon);
	pDaemon->ConstructL();
	CleanupStack::Pop(pDaemon);
	return pDaemon;
	}
	
    
void CNetworkConfigExtensionDhcp::ConstructL()
	{
	CNetworkConfigExtensionBase::ConstructL();
	}


void CNetworkConfigExtensionDhcp::SendIoctlMessageL(const ESock::RLegacyResponseMsg& aMessage)
/**
* SendIoctlMessageL forwards Ioctl request to the daemon and activates the AO to wait for response
* 
@internalTechnology
@version 0.02
@param aMessage[in] a message to be processed (it's the caller's resposibility to forward just Ioctl
*                   messages)
**/
	{
  	TInt name = aMessage.Int1();
  	__FLOG_STATIC1(KLogSubSysNifman, KLogTagNetCfgExtnDhcp, _L("CNifDHCPDaemonProgress::SendIoctlMessageL(): name %x"), name);	
  	if (aMessage.Int0() != KCOLConfiguration)
  		User::Leave(KErrNotSupported);
  	else 
  		{
  		switch (name) 
  			{
  		case KConnAddrRelease:
  		case KConnAddrRenew:
#ifdef SYMBIAN_NETWORKING_DHCPSERVER
  		case KConnSetDhcpRawOptionData:
#endif // SYMBIAN_NETWORKING_DHCPSERVER  		
			if (aMessage.HasCapability(ECapabilityNetworkControl, "NetCfgExtnDhcp") == EFalse)
				{
				User::Leave(KErrPermissionDenied);
				}
  			}
  		}
	CNetworkConfigExtensionBase::SendIoctlMessageL(aMessage);
	}
	
	
CNetworkConfigExtensionDhcp::~CNetworkConfigExtensionDhcp()
/**
~CNetworkConfigExtensionDhcp - destructor
@internalTechnology
@version 0.02
**/
	{
	}	

// Define the interface UIDs
const TImplementationProxy ImplementationTable[] = 
    {
    IMPLEMENTATION_PROXY_ENTRY(0x102032C1, CNetworkConfigExtensionDhcp::NewL)
    };

EXPORT_C const TImplementationProxy* ImplementationGroupProxy(TInt& aTableCount)
    {
    aTableCount = sizeof(ImplementationTable) / sizeof(TImplementationProxy);

    return ImplementationTable;
    }
