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
// This class builds the DNS server list which are configured on all the available
// interfaces.This is derived from active object.
//



/**
 @file
 @internalTechnology
*/
#include "e32debug.h"
#include "inet6log.h"
#include <cflog.h>
#include <in_sock.h>
#include <es_sock.h>

#include "dnsproxyservconf.h"
#include "dnsproxyengine.h"
#include "dnsproxylistener.h"
#include "dnsproxylog.h"

//Constructor for dns server configuration class
CDnsServerConfig::CDnsServerConfig(CDnsProxyEngine &aEngine):CActive(CActive::EPriorityStandard),iProxyEngine(aEngine)
	{

	}
//Destructor for dns server config class
CDnsServerConfig::~CDnsServerConfig()
	{
	Cancel();
	//Free the list
	iServerList.ResetAndDestroy();
	//Close the open socket
	iSocket.Close();
	//connection close
	iConn.Close();
	//Socket server 
	iSocketServer.Close();
	}

//Creates an instance of
CDnsServerConfig* CDnsServerConfig::NewL(CDnsProxyEngine &aEngine)
/**
 *
 * Create an instance of config server
 *
 * @param	&aEngine 	Instance of engine
 * @return	The only CDnsServerConfig instance to be used
 *
 * @internalTechnology
 */
	{
	CDnsServerConfig* server = new(ELeave)CDnsServerConfig(aEngine);
	CleanupStack::PushL(server);
	server->ConstructL();
	CleanupStack::Pop();
	return server;
	}

void CDnsServerConfig::ConstructL()
/**
 * Second phase of construction
 * Initializes array list and creates socket server handle and
 * socket for retrieving DNS server configuration on the available
 * interface
 *
 * @internalTechnology
 */
	{
	User::LeaveIfError(iSocketServer.Connect());
    User::LeaveIfError(iConn.Open(iSocketServer,KConnectionTypeDefault));
    User::LeaveIfError(iSocket.Open(iSocketServer,KAfInet, KSockDatagram, KProtocolInetUdp));

	CActiveScheduler::Add(this);
	}

void CDnsServerConfig::RunL()
/**
 *
 * Builds server list on interface notification.
 * This is RunL of an active object.
 *
 * @internalTechnology
 */
	{
	__LOG1("\n CDnsServerConfig::RunL() Entry Status is:", iStatus.Int());
	iServerList.ResetAndDestroy();
	}

void CDnsServerConfig::DoCancel()
/**
 *
 * Implements DoCancel() method of an active object. This cancels the registration
 * for interface notification.
 *
 * @internalTechnology
 */
	{
	iConn.CancelAllInterfaceNotification();
	}

// CDnsServerConfig::AddDnsServerToList
// **********************************
void CDnsServerConfig::AddDnsServerToListL(const TInetAddr &aAddr, RSocket &aSocket, TUint32 aIapId)
/**
 * Add new address to the server list.
 *
 * @param	aAddr	address of the DNS server
 * @param	aSocket	Socket
 *
 * @internalTechnology
*/
	{
	__LOG("\n CDnsServerConfig::AddDnsServerToListL() Entry")
	if (aAddr.IsUnspecified())
		return;		// No address, nothing to add
	
	// Check if DNS server address is same as one we are listening on
	if(iProxyEngine.iProxyIfAddr.Address() == aAddr.Address())
		{
	    return;
		}

	//
	// Normalize all addresses into IPv6 format
	//
	TDnsServerInfo* sd = new(ELeave)TDnsServerInfo;
	if(sd == NULL)
		{
		return;
		}
	sd->iAddr = aAddr;
	if (sd->iAddr.Family() == KAfInet)
		sd->iAddr.ConvertToV4Mapped();
	else if (sd->iAddr.Family() != KAfInet6)
		return;		// Only IPv4 or IPv6 addresses are valid
	if (sd->iAddr.IsMulticast() || sd->iAddr.IsLoopback() || sd->iAddr.IsLinkLocal() || sd->iAddr.IsSiteLocal())
		{
		delete sd;
		return;
		}
	else
		sd->iScope = KDnsServerScopeGlobal;

	if (!sd->iAddr.Port())
		sd->iAddr.SetPort(KDnsProxyPort);

	TBuf<KIpAddr> dst;
	sd->iAddr.OutputWithScope(dst);
	TBuf<KIpAddr> src;
	// DNS Proxy does not bring up interfaces. Hence server addresses must have a
	// valid route, before they can be used. Thus, check it...
	TPckgBuf<TSoInetIfQuery> opt;
	opt().iDstAddr = aAddr;
	const TBool has_route =
		(aSocket.GetOpt(KSoInetIfQueryByDstAddr, KSolInetIfQuery, opt) == KErrNone) && !opt().iSrcAddr.IsUnspecified();

	if (!has_route)
		{
		__LOG2("\n CDnsServerConfig::AddDnsServerToListL() nameserver: [%S] has no route (src=%S)",&dst, &src)
		delete sd;
		return;				// No route, unusable for now -- ignore
		}
		
	//Add IapId for the interface	
    sd->iIapId = aIapId;
    
    //Check the connection status and add DNS server to the list
    if(GetConnectionStatus(*sd))
    	{
    	__LOG2("\n CDnsServerConfig::AddDnsServerToListL() nameserver: [%S] (src=%S)",&dst, &src)    
    	iServerList.AppendL(sd);
    	}
    else
    	{
    	//Connection is not existing, hence don't add DNS server address to the list
    	__LOG2("\n CDnsServerConfig::AddDnsServerToListL() no connection for nameserver: [%S] (src=%S)",&dst, &src)
    	delete sd;	
    	}
    __LOG("\n CDnsServerConfig::AddDnsServerToListL() Exit")    
	}

// CDnsServerConfig::BuildGlobalDnsServerListL
// **********************************
void CDnsServerConfig::BuildGlobalDnsServerListL(TUint32 aIapId)
/**
 * This method enumerates interfaces and builds the list of available.
 * DNS servers.
 * The method also gets called whenever any interface notification is received.
 *
 * @internalTechnology
*/

	{
	__LOG("\n CDnsServerConfig::BuildGlobalDnsServerListL() Entry")
    TInt err;
    TSoInetInterfaceInfo* info = new TSoInetInterfaceInfo; // allocate large struct from heap!
		
	iServerList.ResetAndDestroy();
	TInetAddr localaddr = iProxyEngine.GetProxyListener()->GetInterfaceIpAddress();
	iProxyEngine.UpdateLocalAddr(localaddr);
    if (info && (err = iSocket.SetOpt(KSoInetEnumInterfaces, KSolInetIfCtrl)) == KErrNone)
    	{
    	TPckg<TSoInetInterfaceInfo> opt(*info);
    	while (iSocket.GetOpt(KSoInetNextInterface, KSolInetIfCtrl, opt) == KErrNone)
			{
			TSoInetIfQuery ifquery;
    		TPckg<TSoInetIfQuery> queryopt(ifquery);
    
    		ifquery.iName = opt().iName;
    		err = iSocket.GetOpt(KSoInetIfQueryByName, KSolInetIfQuery, queryopt);
    	
    		if(err == KErrNone) 
    			{
    			if (opt().iName.Length() == 0)
    				continue;	// "null" interface, ignore
                // Add DNS servers configured on specified Iap only    				
				if (!opt().iAddress.IsUnspecified() && (aIapId == ifquery.iZone[1]))
					{
        	        // Add name server information to the list
					AddDnsServerToListL((TInetAddr&)(opt().iNameSer1), iSocket, ifquery.iZone[1]);
					AddDnsServerToListL((TInetAddr&)(opt().iNameSer2), iSocket, ifquery.iZone[1]);
					}
    			
    			}
    	
			}
    	}
    delete info;
    NotifyInterfaceChange();
    __LOG("\n CDnsServerConfig::BuildGlobalDnsServerListL() Exit")
  	}

TDnsServerInfo* CDnsServerConfig::GetDnsServerInfo(TInt aServerIndex)
/**
 * This method returns the DNS server info based on the list index/ server id.
 * @param	aServerId
 * @internalTechnology
*/

	{
	TDnsServerInfo* serverinfo = NULL; 
	// Check if there are any dns servers in the list and the index is less than the number
	// of servers available
	if(iServerList.Count() > 0 && aServerIndex < iServerList.Count())
		serverinfo = iServerList[aServerIndex];
	
	return serverinfo;	
	}	


//Returns number of servers in the list
TInt CDnsServerConfig::GetServerCount() const
/**
 * This method returns the DNS server count.
 *
 * @internalTechnology
*/

	{
	__LOG1("\n CDnsServerConfig::GetServerCount Server count is %d", iServerList.Count())
	return (iServerList.Count());
	}



void CDnsServerConfig::NotifyInterfaceChange()
/**
 * This method registers for interface notification and sets the active object.
 *
 * @internalTechnology
*/

	{
	__LOG("\n CDnsServerConfig::NotifyInterfaceChange() Entry")
	if(IsActive())
	   return;

	iConn.AllInterfaceNotification(info, iStatus);
	SetActive();
	__LOG("\n CDnsServerConfig::NotifyInterfaceChange() Exit")
	}

TBool CDnsServerConfig::GetConnectionStatus(TDnsServerInfo& aInfo)
/**
 * This method checks for uplink connection status. 
 * @param aInfo - Dns server info structure containing IAP id for the interface
 *
 *
 * @internalTechnology
*/
	{
	__LOG("\n CDnsServerConfig::GetConnectionStatus Entry")
	TBool connected = EFalse;
	TUint connectionCount = 0;
	TPckgBuf<TConnectionInfo> connectionInfo;
	TInt err = iConn.EnumerateConnections(connectionCount);

	if(err != KErrNone)
		{
		__LOG("\n CDnsServerConfig::GetConnectionStatus, Enumerate Connections failure");
		return connected;
		}
		
	for (TUint i = 1; i <= connectionCount; ++i)
		{
		iConn.GetConnectionInfo(i, connectionInfo);
		
		if (aInfo.iIapId == connectionInfo().iIapId)
			{
			__LOG("\n CDnsServerConfig::GetConnectionStatus Conn Info for IAP is present")
			//Set the Up flag to True
			connected = ETrue;
			//Copy this connection information to information
			aInfo.iConnInfo = connectionInfo;
        	}
		}
    __LOG("\n CDnsServerConfig::GetConnectionStatus Exit")		
	return connected;    
	}
