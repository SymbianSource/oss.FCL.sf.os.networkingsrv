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
// RStartServer Client side header
// Declares the R class to start server process,
// the class covers EPOC platform & emulator diferences
// 
//

/**
 @file CS_DeamonControl.h
 @internalComponent
*/

#if !defined (__NET_CFG_EXTN_DHCP_H__)
#define __NET_CFG_EXTN_DHCP_H__

#include <e32base.h>

#include <comms-infras/networkconfigextensionbase.h>
#include "es_enum.h"
#include <comms-infras/commsdebugutility.h>

//-- for some DHCPv4 constants and data structures definition
#include <comms-infras/es_config.h>
#include <nifman.h>


__FLOG_STMT(_LIT8(KLogTagNetCfgExtnDhcp, "NetCfgExtnDhcp");)

/**
 Daemon configuration, uses RConfigDaemon generic config daemon client API
 to configure network layer
 @internalTechnology
 @version 0.03
 @date	26/05/2004
*/
class CNetworkConfigExtensionDhcp : public CNetworkConfigExtensionBase
	{
public:
  	static CNetworkConfigExtensionDhcp* NewL(TAny* aMNifIfNotify);
   	virtual ~CNetworkConfigExtensionDhcp();

	// From CNetworkConfigExtensionBase
   	void SendIoctlMessageL(const ESock::RLegacyResponseMsg& aMessage);

protected:
   	CNetworkConfigExtensionDhcp(MNifIfNotify& aNifIfNotify);
  	void ConstructL();
	};


inline CNetworkConfigExtensionDhcp::CNetworkConfigExtensionDhcp(MNifIfNotify& aNifIfNotify) :
   CNetworkConfigExtensionBase(aNifIfNotify)
/**
 CNetworkConfigExtensionBase - constructor
 @internalComponent
 @param aNifIfNotify - client of the control
 @version 0.02
*/
	{
	}



#endif

