// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
// dnsserverconfigparams.h
// This file contains dns proxy server configuration parameters
//



/**
 @file
 @internalComponent
*/

#ifndef __DNSPROXYCLIENTCONFIGPARAMS_H__
#define __DNSPROXYCLIENTCONFIGPARAMS_H__

#include "e32std.h"

// The server's identity within the client-server framework
_LIT(KServerName,"DNSProxy");
_LIT(KServerExeName,"DNSProxy.exe");


/**
* The enum defines the client side commands which are handled by the server.
*
**/
enum TDnsProxyServerConfigParams
    {    
    EProxyConfigure,
    EProxyAddDb,
    EProxyRemoveDb,
    EProxyUpdateDomainName,
    EProxyConfigureUplink,
    ENotSupported    
    };

#endif // __DNSPROXYCLIENTCONFIGPARAMS_H__