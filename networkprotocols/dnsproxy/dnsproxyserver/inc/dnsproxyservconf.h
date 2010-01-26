//dnsproxyserconf.h
/*
* Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* dnsproxyserconf.h - Dns Engine class
* @file
* @internalTechnology
*
*/




#ifndef __DNSPROXYSERVCONF_H__
#define __DNSPROXYSERVCONF_H__

#include <in_sock.h>
#include <commdbconnpref.h>
#include <e32cmn.h>
#include <es_enum_internal.h>
#include "es_enum.h"

//Forward declaration
class CDnsProxyEngine;

const TInt KDnsServerScopeGlobal 	= 0x0E;
const TInt KDnsServerListLen 		= 2;
const TInt KIpAddr 					= 80;


/**
*Defines an element in the DNS server list.This holds DNS server information
*TDnsServerInfo consists of DNS server address, scope, server id,
*and name
**/
class TDnsServerInfo 
	{
public:
	TInetAddr iAddr;			           //< DNS Server Address
	TInt iScope;				           //< The Sever Scope and type (MC/UC)
	TName iName;                           //< Name   
	TUint32 iIapId;                        //< Iapid for the interface on which DNS address is configured
	TConnectionInfoBuf iConnInfo;          //< Uplink connection info
	};
 
/**
* This class builds and maintains DNS server configuration. The configuration is then used by DNS query session
* class to forward incoming DNS requests to the external DNS server configured.
**/
class CDnsServerConfig:public CActive
	{
	public:
 	   ~CDnsServerConfig();
  	   static CDnsServerConfig* NewL(CDnsProxyEngine &aEngine);
 	   void BuildGlobalDnsServerListL(TUint32 aIapId);
  	   void NotifyInterfaceChange();
   	   TDnsServerInfo* GetDnsServerInfo(TInt aServerIndex);
  	   TInt GetServerCount() const;
  	   TBool GetConnectionStatus(TDnsServerInfo& aInfo);
  	   
 	private:
	    //construction
	 	CDnsServerConfig(CDnsProxyEngine &aEngine);
	    void ConstructL();
	    void AddDnsServerToListL(const TInetAddr &aAddr, RSocket &aSocket, TUint32 aIapId);
	    
 	private:
 		void DoCancel();
 		void RunL();
 		
 	public:
	    RSocketServ iSocketServer;
	    RConnection iConn;
	    TInterfaceNotificationBuf info;
	    RPointerArray<TDnsServerInfo> iServerList;//< Current list of servers
 		
 	private:
 		RSocket 					iSocket;
	    TUint32                     iServerCount;
	    CDnsProxyEngine&            iProxyEngine;
	};
	
#endif /*__DNSPROXYSERVCONF_H__*/
