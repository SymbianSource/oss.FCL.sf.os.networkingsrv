// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// dnsserverconfig.h
// Header file for the DNS Proxy client side implementation.
//



/**
 @file
 @internalComponent
*/
#ifndef __DNSPROXYCLIENT_H__
#define __DNSPROXYCLIENT_H__

#include "e32std.h"

class RDNSClient : public RSessionBase
/** This class represents the client side implementation of DNS Proxy Server
*/
    {
public:
    IMPORT_C TInt Connect();
    IMPORT_C void ConfigureDnsProxyServer(const TDes8& aInfo, TRequestStatus& aStatus);
    IMPORT_C void AddDbEntry(const TDes8& aHostName, const TDes& aIpAddress, TRequestStatus& aStatus);
    IMPORT_C void RemoveDbEntry(const TDes& aIpAddress, TRequestStatus& aStatus);    
    IMPORT_C void UpdateDomainName(const TDes8& aName, TRequestStatus& aStatus);
    IMPORT_C void ConfigureUplinkInfo(const TDes8& aName, TRequestStatus& aStatus);
    };

#endif //__DNSPROXYCLIENT_H__