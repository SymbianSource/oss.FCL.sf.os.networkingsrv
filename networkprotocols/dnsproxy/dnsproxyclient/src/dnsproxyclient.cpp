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
// dnsserverconfig.h
// Source file for the DNS Proxy client side implementation.
//



/**
 @file
 @internalComponent
*/

#include "dnsproxyclientconfigparams.h"
#include "dnsproxyclient.h"

// Runs client-side and starts the separate server process
static TInt StartTheServer()
    {
    _LIT(KServerExecutableName,"DNSProxy.exe");

    RProcess server;

    TInt err = server.Create(KServerExecutableName, KNullDesC);
    if (err != KErrNone)
        return err;

    TRequestStatus stat;
    server.Rendezvous(stat);
    if (stat!=KRequestPending)
    	//abort startup
        server.Kill(0);
    else
    	//logon OK - start the server
        server.Resume();

   // wait for start or death
    User::WaitForRequest(stat);

    err = server.ExitType();
    if (EExitPanic == err)
        err = KErrGeneral;
    else
        err = stat.Int();

    // This is no longer needed
    server.Close();
    return err;
    }


EXPORT_C TInt RDNSClient::Connect()
/**
 * This method is used to connect to the server
 * @return TInt
 *
 * @internalTechnology
 **/
    {
    TInt retry=2;

    for (;;)
        {
        // Uses system-pool message slots
        TInt r=CreateSession(KServerName,TVersion(1,0,0));
        if ( (KErrNotFound!=r) && (KErrServerTerminated!=r) )
            return (r);
        if (--retry==0)
            return (r);
        r=StartTheServer();
        if ( (KErrNone!=r) && (KErrAlreadyExists!=r) )
            return (r);
        }
    }

EXPORT_C void RDNSClient::ConfigureDnsProxyServer(const TDes8& aInfo, TRequestStatus& aStatus)
/**
 * Client side message to configure DNS Proxy Server.
 * @param aInfo of type TDes8& which represents connection info
 *
 * @internalTechnology
 **/
    {
    SendReceive(EProxyConfigure, TIpcArgs(&aInfo),aStatus);
    }

/**
 * Client side message to Add dns db
 * @param aAddress of type TDes& which represents IP address of the connected hosts.
 * @param aName of type TDes& which represents host name of the connected hosts.
 *
 * @internalTechnology
 **/
EXPORT_C void RDNSClient::AddDbEntry(const TDes8& aHostName, const TDes& aIpAddress, TRequestStatus& aStatus)
    {
    SendReceive(EProxyAddDb,TIpcArgs(&aIpAddress,&aHostName),aStatus);
    }

/**
 * Client side message to Remove dns db
 * @param aAddress of type TDes& which represents IP address of the connected hosts.
 * @param aName of type TDes& which represents host name of the connected hosts.
 *
 * @internalTechnology
 **/
EXPORT_C void RDNSClient::RemoveDbEntry(const TDes& aIpAddress, TRequestStatus& aStatus)
    {
    SendReceive(EProxyRemoveDb,TIpcArgs(&aIpAddress),aStatus);
    }

EXPORT_C void RDNSClient::UpdateDomainName(const TDes8& aName, TRequestStatus& aStatus)
/**
 * Client side message to set domain name in the Dns proxy server
 * @param aAddress of type TDes& which represents IP address of the connected hosts.
 * @param aName of type TDes& which represents host name of the connected hosts.
 *
 * @internalTechnology
 **/
    {
	SendReceive(EProxyUpdateDomainName,TIpcArgs(&aName),aStatus);
    }
   
EXPORT_C void RDNSClient::ConfigureUplinkInfo(const TDes8& aName, TRequestStatus& aStatus)
/**
 * Client side message to set the Uplink information
 * @param aInfo of type TDes8& which represents connection info 
 *
 * @internalTechnology
 **/
    {
    SendReceive(EProxyConfigureUplink,TIpcArgs(&aName),aStatus);
    }
