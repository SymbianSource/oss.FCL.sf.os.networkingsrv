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
// This file consists of implementation of DNS Proxy engine. This is the controller
// class which reads configuration from resolver.ini, configures DNS Proxy components
// and starts and stops the listener.
//



/**
 @file
 @internalTechnology
*/

#include <es_sock.h>
#include <cflog.h>
#include "e32debug.h"
#include "dnsproxylistener.h"
#include "dnsproxyengine.h"
#include "dnsproxyservconf.h"
#include "dnsproxylog.h"
#include "inet6log.h"

//intializing the static object
CDnsProxyEngine* CDnsProxyEngine::iDnsProxyEngine = NULL;

CDnsProxyEngine* CDnsProxyEngine::NewL()
/**
 * This is the two phase construction method and is leaving function
 * @param - None
 * @return - None
 *
 * @internalTechnology
 */
	{
	__LOG("\n CDnsProxyEngine::NewL Entry")
	CDnsProxyEngine* pEngine = new(ELeave)CDnsProxyEngine();
	CleanupStack::PushL(pEngine);
	pEngine->ConstructL();
	CleanupStack::Pop();
	__LOG("\n CDnsProxyEngine::NewL Exit")
	return pEngine;
	}


CDnsProxyEngine::CDnsProxyEngine()
/**
 * This is the constructor for this class
 * @param - None
 * @return - None
 *
 * @internalTechnology
 */

	{
	__LOG("\n CDnsProxyEngine::CDnsProxyEngine Entry")
	iConfigData = NULL;
	iConfigDataErr = 0;
	__LOG("\n  CDnsProxyEngine::CDnsProxyEngine Exit")
	}
/* This is the destructor  and deletes all the member variables and thus
 * releases all the memory assosicated with those objects
 */
CDnsProxyEngine::~CDnsProxyEngine()
/**
 * This is the destructor  and deletes all the member variables and thus
 * releases all the memory assosicated with those objects
 * @param - None
 * @return - None
 *
 * @internalTechnology
 **/

	{
	__LOG("\n CDnsProxyEngine::~CDnsProxyEngine Entry")
	delete iConfigData;
	delete iProxyServer;
	delete iListener;
	delete iDnsproxydb;
	iSockServ.Close();
	__LOG("\n CDnsProxyEngine::~CDnsProxyEngine Exit")
	}
/* This is second phase construction where in all objects will be created
 * which potentially may leave due to resource problem
 */
void CDnsProxyEngine::ConstructL()
/**
 * This is the second phase of construction.
 * Create intances of proxy database, config server and listener classes.
 * Reads configuration values from .ini file
 * @param - None
 * @return - None
 *
 * @internalTechnology
 **/
	{
	__LOG("\n CDnsProxyEngine::ConstructL Entry")
	//Reads the configuration from .ini file
	ReadConfiguration();
	User::LeaveIfError(iSockServ.Connect());
	__LOG("After Reading configuration")
	iDnsproxydb = CDnsProxyDb::CreateInstanceL(*this);
	iProxyServer = CDnsServerConfig::NewL(*this);
	iListener    = CDnsProxyListener::NewL(*this);
	
	__LOG("\n CDnsProxyEngine::ConstructL Exit")
	}

/* This is function reads configuration values from configuration file
 *
 */
void CDnsProxyEngine::ReadConfiguration()
/**
 *
 * This method reads configuration values from .ini file
 * @param - None
 * @return - None
 *
 * @internalTechnology
 **/

	{
	__LOG("\n CDnsProxyEngine::ReadConfiguration Entry")
	iConfigParams.iSessionCount = GetIniValue(DNS_PROXY_INI_SECTION,DNS_PROXY_INI_NUM_SOCK_SESSION,KSessioncount);
	iConfigParams.iRetryCount   = GetIniValue(DNS_PROXY_INI_SECTION,DNS_PROXY_INI_NUM_RETRIES,KRetrycount);
	iConfigParams.iQSize        = GetIniValue(DNS_PROXY_INI_SECTION,DNS_PROXY_INI_QUEUE_SIZE,KQSize);
	iConfigParams.iTTL          = GetIniValue(DNS_PROXY_INI_SECTION,DNS_PROXY_INI_TTL,KTtl);
	iConfigParams.iDbSize       = GetIniValue(DNS_PROXY_INI_SECTION,DNS_PROXY_INI_DB_SIZE,KDbSize);
	iConfigParams.iTimerVal     = GetIniValue(DNS_PROXY_INI_SECTION,DNS_PROXY_INI_TIMER_VAL,KTimerVal);

	iConfigParams.iSerialNum    = GetIniValue(DNS_PROXY_INI_SECTION,DNS_PROXY_INI_SERIAL_NUM,KSerialNum);
	iConfigParams.iRefreshTime  = GetIniValue(DNS_PROXY_INI_SECTION,DNS_PROXY_INI_REFRESH_TIME,KRefreshTime);
	iConfigParams.iRetryTime    = GetIniValue(DNS_PROXY_INI_SECTION,DNS_PROXY_INI_RETRY_TIME,KRetryTime);
	iConfigParams.iExpiretime   = GetIniValue(DNS_PROXY_INI_SECTION,DNS_PROXY_INI_EXPIRE_TIME,KExpireTime);	

	__LOG1("\n CDnsProxyEngine::ReadConfiguration iSessionCount (%d)",iConfigParams.iSessionCount)
	__LOG1("\n CDnsProxyEngine::ReadConfiguration iRetryCount (%d)",iConfigParams.iRetryCount)
	__LOG1("\n CDnsProxyEngine::ReadConfiguration iTimerVal (%d)",iConfigParams.iTimerVal)
	UnLoadConfigurationFile();
	}

TInt CDnsProxyEngine::Start(const RMessage2& aMessage)
/**
 * This is entry point of DNS Proxy functionality. The DHCP server session
 * calls this using client API to start dnsproxy.
 * @param aMessage - Message send by the client
 * @return - None
 *
 * @internalTechnology
 **/

	{
    TInt err;
	__LOG("\n CDnsProxyEngine::Start Entry")
	err = SetDlinkConnectionInfo(aMessage);
	if(err!=KErrNone)
		return err;

	err = iListener->Activate();
	__LOG("\n CDnsProxyEngine::ReadConfiguration Exit")
	return err;
	}

void CDnsProxyEngine::Stop()
/**
 * This stops the listener and deletes all the objects.The method is called at the time
 * of shutdown.
 * @param - None
 * @return - None
 *
 * @internalTechnology
 **/
	{
	__LOG("\n CDnsProxyEngine::Stop Entry")
	iListener->StopListener();
	__LOG("\n CDnsProxyEngine::Stop Exit")
	}

TBool CDnsProxyEngine::FindVar(const TDesC &aSection, const TDesC &aVarName, TInt &aResult)
/**
 * This will find specified value for variable in the configuation file
 * @param aSection - represents the section name in configuration file
 * @param aVarName - represents name in the specified section
 * @param aResult - sets the corresponding values for above given varname
 * @return - returns the boolean flag - if name value pair or loading configuration
 * 			- fails then returns false
 * @panic - system wide errors
 *
 * @internalTechnology
 **/
	{
	__LOG("\n DnsProxyEngine::FindVar")
	if(LoadConfigurationFile())
	  {
	   ASSERT(iConfigData);	// <-- lint gag
	   return iConfigData->FindVar(aSection, aVarName, aResult);
	  }
	__LOG("\n DnsProxyEngine::FindVar Exit")
	return EFalse;
	}

TBool CDnsProxyEngine::LoadConfigurationFile()
/**
 * This will load the configuaration file into memory.
 * If it is already loaded then returns true otherwise loads the file
 * @param - None
 * @return boolean - retursn boolean value either true or false
 *
 * @internalTechnology
 **/

	{
	__LOG("\n CDnsProxyEngine::LoadConfigurationFile Entry")
	if (iConfigData)
		return TRUE;	// Already loaded!
	if (iConfigDataErr)
		return FALSE;
	TRAP(iConfigDataErr, iConfigData = CESockIniData::NewL(DNS_PROXY_INI_DATA));
	__LOG("\n CDnsProxyEngine::LoadConfigurationFile Exit")
	return (iConfigData != NULL);
	}

void CDnsProxyEngine::UnLoadConfigurationFile()
/**
 * This will unload the configuaration file from memory.
 * @param - None
 * @return - None
 *
 * @internalTechnology
 **/

	{
	__LOG("\n CDnsProxyEngine::UnloadConfigurationFile Entry")
	delete iConfigData;
	iConfigData = NULL;
	iConfigDataErr = 0;
	__LOG("\n CDnsProxyEngine::UnloadConfigurationFile Exit")
	}

TInt CDnsProxyEngine::GetIniValue(const TDesC &aSection, const TDesC &aName, TInt aValue)
/**
 * This function reads the value for given name in the specified section
 * @param aSection - represents the section name in configuration file
 * @param aName - represents name in specified section
 * @param aValue - represents default value
 * @return boolean - retursn boolean value either true or false
 *
 * @internalTechnology
 **/

	{
	__LOG("\n CDnsProxyEngine::GetIniValue Entry")
	LOG(_LIT(KFormat, "\t[%S] %S = %d"));
	TInt value;
	if (!FindVar(aSection, aName, value))
		value = aValue;
    LOG(Log::Printf(KFormat, &aSection, &aName, value));
    __LOG1("CDnsProxyEngine::GetIniValue %d",value);
    
	return value;
	}

TInt CDnsProxyEngine::AddDbEntryL(const RMessage2& aMsg)
/**
 * This method updates DB.
 * @param aMessage - Message send by the client
 * @return - None
 *
 * @internalTechnology
 **/
	{
	if(!iDnsproxydb)
	   return KErrBadHandle;
	
	TBuf8<KMaxName> dbdata;
	TBuf16<KAddrLength> ipaddress;

	aMsg.Read(0,ipaddress);
	aMsg.Read(1,dbdata);

	TInetAddr address;
    address.Input(ipaddress);
    
    TInt current_size = iDnsproxydb->GetDbSize();
    if(current_size < iConfigParams.iDbSize)
    	{
  		iDnsproxydb->UpdateDbL(dbdata,address);
    	}
    else
	    {
	    return KErrGeneral;	
	    }	
	__LOG("\n CDnsProxyEngine::AddDbEntryL Exit")
	return KErrNone;
	}

TInt CDnsProxyEngine::RemoveDbEntry(const RMessage2& aMsg)
/**
 * This method updates DB.
 * @param aMessage - Message send by the client
 * @return - None
 *
 * @internalTechnology
 **/
	{
	if(!iDnsproxydb)
		return KErrBadHandle;

	__LOG("\n CDnsProxyEngine::RemoveDbEntry Entry")
	TBuf16<KAddrLength> ipaddress;

	aMsg.Read(0,ipaddress);

	TInetAddr address;
    address.Input(ipaddress);
    iDnsproxydb->DeleteDbEntry(address);
    __LOG("\n CDnsProxyEngine::RemoveDbEntry Exit")
    return KErrNone;
	}

void CDnsProxyEngine::UpdateLocalAddr(TInetAddr& aAddr)
/**
 * This method updates interface address in the engine.
 * @param addr - Listen interface address
 * @return - None
 *
 * @internalTechnology
 **/
	{
	__LOG("\n CDnsProxyEngine::UpdateLocalAddr Entry/Exit")
	iProxyIfAddr = aAddr;
	}

TInt CDnsProxyEngine::UpdateDomainInfo(const RMessage2& aMsg)
/**
 * This method updates DB.
 * @param aMessage - Message send by the client
 * @return - None
 *
 * @internalTechnology
 **/
	{
	TInt err = KErrNone;
	__LOG("\n CDnsProxyEngine::UpdateDomainInfo Entry")
	err = aMsg.Read(0,iSuffixInfo);
	return err;
	}

    
TInt CDnsProxyEngine::IsSuffixAvailable()
/**
 * This method checks if suffix is available. 
 * @param  None
 * @return Length- Length of suffix data
 *
 * @internalTechnology
 **/

	{
	__LOG("\n CDnsProxyEngine::IsSuffixAvailable Entry/Exit")
	return iSuffixInfo.Length();
	}
    


CDnsProxyEngine* CDnsProxyEngine::GetInstanceL()
    {
/**
 * This method returns single ton pointer to CDnsProxyEngine 
 * @param  None
 * @return iUplinkConnection
 *
 * @internalTechnology
 **/    
    __LOG("\n CDnsProxyEngine::GetInstanceL Entry")    
    if(!iDnsProxyEngine)
        {
        __LOG("\n first time creation ")            
        iDnsProxyEngine = CDnsProxyEngine::NewL();
        }
    __LOG("\n CDnsProxyEngine::GetInstanceL  Exit")        
    return iDnsProxyEngine;
    }


TInt CDnsProxyEngine::SetUplinkConnectionInfo(const RMessage2 &aMessage)
/**
 * This method reads uplink connection info from the message. 
 * @param aMessage - Message send by the client
 * @return - None
 *
 * @internalTechnology
 **/

	{
	
	__LOG("\n CDnsProxyEngine::SetUplinkConnectionInfo Entry")
	
	TInt err = iListener->SetUplinkConnectionInfo(aMessage);
	__LOG("\n CDnsProxyEngine::SetUplinkConnectionInfo Exit")
	return err;
	
    }
    
TInt CDnsProxyEngine::SetDlinkConnectionInfo(const RMessage2 &aMessage)
/**
 * This method reads downlink connection info from the message. 
 * @param aMessage - Message send by the client
 * @return - None
 *
 * @internalTechnology
 **/

	{
	__LOG("\n CDnsProxyEngine::SetDlinkConnectionInfo Entry")
	TInt err = iListener->SetDlinkConnectionInfo(aMessage);
	__LOG("\n CDnsProxyEngine::SetDlinkConnectionInfo End")
	return err;
	}
