/**
* Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* RStartServer Client side header
* Declares the R class to start server process,
* the class covers EPOC platform & emulator diferences
* 
*
*/



/**
 @file
 @internalComponent
*/

#ifndef GUARD_NETCFGEXTNMIP_H
#define GUARD_NETCFGEXTNMIP_H

#include <e32base.h>
#include <comms-infras/networkconfigextensionbase.h>
#include "es_enum.h"
#include <comms-infras/commsdebugutility.h>

__FLOG_STMT(_LIT8(KLogTagNetCfgExtnMip, "NetCfgExtnMip");)

/**
 Daemon configuration, uses RConfigDaemon generic config daemon client API 
 to configure network layer
 @internalTechnology
**/
class CNetworkConfigExtensionMip : public CNetworkConfigExtensionBase
	{
public:
  	static CNetworkConfigExtensionMip* NewL(TAny* aMNifIfNotify);
   	virtual ~CNetworkConfigExtensionMip();

protected:
   	CNetworkConfigExtensionMip(MNifIfNotify& aNifIfNotify);
  	void ConstructL();
	};
	

inline CNetworkConfigExtensionMip::CNetworkConfigExtensionMip(MNifIfNotify& aNifIfNotify) :
   CNetworkConfigExtensionBase(aNifIfNotify)
/**
 CNetworkConfigExtensionBase - constructor
 @internalComponent
 @param aNifIfNotify - client of the control
**/
	{
	}
#endif

