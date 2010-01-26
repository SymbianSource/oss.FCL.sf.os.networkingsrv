//dnsproxyengine.h
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
* dnsproxyengine.h - Dns Engine class
* @file
* @internalTechnology
*
*/



#ifndef DNS_PROXY_ENGINE_H
#define DNS_PROXY_ENGINE_H

#include <es_ini.h>
#include <es_enum.h> 
#include "dnsproxyserver.h"
#include "dnsproxydb.h"
#include "dns_hdr.h"

//Literals constants for reading .ini file
_LIT(DNS_PROXY_INI_DATA,             "resolver.ini");
_LIT(DNS_PROXY_INI_SECTION,          "proxyresolver");	
_LIT(DNS_PROXY_INI_NUM_SOCK_SESSION, "sessioncount");
_LIT(DNS_PROXY_INI_NUM_RETRIES,      "retries");	
_LIT(DNS_PROXY_INI_QUEUE_SIZE,       "queuesize");
_LIT(DNS_PROXY_INI_TTL,              "TTL");
_LIT(DNS_PROXY_INI_DB_SIZE,          "dbsize");
_LIT(DNS_PROXY_INI_TIMER_VAL,        "timerval");
_LIT(DNS_PROXY_INI_SERIAL_NUM,       "serialnum");
_LIT(DNS_PROXY_INI_REFRESH_TIME,     "refreshtime");
_LIT(DNS_PROXY_INI_RETRY_TIME,       "retrytime");
_LIT(DNS_PROXY_INI_EXPIRE_TIME,      "expiretime");

const TUint   KSessioncount = 1;
const TUint   KRetrycount   = 2;
const TUint   KQSize 		= 20;
const TUint   KTtl		    = 21600;//dhcp lease timer is 6 hrs
const TUint   KDbSize       = 10;
const TUint   KTimerVal     = 5;
const TUint32 KSerialNum   = 10000;
const TUint32 KRefreshTime = 900;
const TUint32 KRetryTime   = 600;
const TUint32 KExpireTime  = 300;
const TUint32 KAddrLength  = 100;




//T Type class for DNS Proxy Configuration parameters
class TDnsProxyConfigParams
	{
public:
    TUint iSessionCount;//< Max number of Socket sessions  
    TUint iRetryCount;	//< Possible number of retries 
    TUint iQSize;       //< Queue size limit 
    TUint iTTL;			//< Time to Live
    TUint iDbSize;      //< Max number of DB entries
	TUint iTimerVal;	//< Timeout value for response timer
	TUint32 iSerialNum; //< This is required for NS response
	TUint32 iRefreshTime; //< This is required for NS response
	TUint32 iRetryTime; //< This is required for NS response
	TUint32 iExpiretime; //< This is required for NS response
	};


//forward declaration
class CDnsProxyListener;
class CESockIniData;
class CDnsProxyDb;
class CDnsServerConfig;

	
/**
 *  This class is an entry point and reads all configuration values and creates 
 *  some necessary objects
 *  @internalTechnology
 * */
class CDnsProxyEngine:public CBase
	{
	public:
		//two phase construction
        static CDnsProxyEngine* GetInstanceL();
		~CDnsProxyEngine();
		//Starts DNSProxy. The method is called from DNSProxy server session.
		TInt Start(const RMessage2& aMessage);
		//Stops DNSProxy. The method is called from DNSProxy server session.
		void Stop();
		//Returns instance of socket server
		RSocketServ& GetSocketServer();
		//Returns an instance of Listener
		CDnsProxyListener* GetProxyListener();
	    // The method updates DNS Proxy DB. 
		TInt AddDbEntryL(const RMessage2& aMsg);
		//remove entry from db
		TInt RemoveDbEntry(const RMessage2& aMsg);
		//This method updates domain name
		void UpdateDomainName(const RMessage2& aMsg);
		// The method to update domain info
		TInt UpdateDomainInfo(const RMessage2& aMsg);
		void UpdateLocalAddr(TInetAddr& aAddr);
		// Suffix is available or not
		TInt IsSuffixAvailable();
	    //This method returns the configuration parameters read from .ini file.
		const TDnsProxyConfigParams& GetConfig() const;
		TInt SetDlinkConnectionInfo(const RMessage2 &aMessage);
		TInt SetUplinkConnectionInfo(const RMessage2 &aMessage);
					
	protected:
	    // Initializes the member variables
		void ConstructL();	
		
	private:
        static CDnsProxyEngine* NewL();
		//constructor
		CDnsProxyEngine();
		//reads the configuation file
		void ReadConfiguration();
		//reads the configuation file for specified name /value pair	
		TInt GetIniValue(const TDesC& aSection, const TDesC& aName, TInt aValue=0);
		//finds the value for given name 
		TBool FindVar(const TDesC& aSection, const TDesC& aVarName, TInt& aResult);
		//loads the cinfiguaration file 
		TBool LoadConfigurationFile();
		//unloads cinfiguration file
		void UnLoadConfigurationFile();
		
				
	public:
		TDnsProxyConfigParams 	    iConfigParams;		//< Parsed configuration parameters
		CDnsServerConfig* 		    iProxyServer;		//< DNS server configuration
		CDnsProxyDb* 				iDnsproxydb;          //< DNS Proxy db instance
		TInetAddr                   iProxyIfAddr;		//< Listener interface address
		TBuf8<KMaxName>             iSuffixInfo;
	
	private:
		CESockIniData* 			    iConfigData;		//< Used to read config data 		
		TInt 					    iConfigDataErr;     //< Contains config data error
		RSocketServ 			    iSockServ;          //< socket server handler
		CDnsProxyListener* 		    iListener;          //< Intance of listener object
		TBool                       iSuffixAvailable;   //< Suffix is available
		
					
	private:
		TBuf<KDnsMaxMessage> iBuf;	      //< Receive buffer
		TBuf<KDnsMaxMessage> iDomainName; 
		static CDnsProxyEngine* iDnsProxyEngine;

	};
	
 inline const TDnsProxyConfigParams& CDnsProxyEngine::GetConfig() const
 /**
 *  This method returns an instance of config params 
 *  @internalTechnology
 * */ 
 	{ 
 	return iConfigParams; 
 	}
 	
 inline RSocketServ& CDnsProxyEngine::GetSocketServer()
/**
 *  This method returns an instance of socket server 
 *  @internalTechnology
 * */
	{
	return iSockServ;
	}
	
inline CDnsProxyListener* CDnsProxyEngine::GetProxyListener()
/**
 *  This method returns an instance of listener object
 *  @internalTechnology
 * */
	{
	return iListener;
	}
	
#endif /* DNS_PROXY_ENGINE_H */
