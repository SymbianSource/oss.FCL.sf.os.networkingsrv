// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// engine.h - name resolver core engine header
//

#ifndef __ENGINE_H
#define __ENGINE_H
#include <s32file.h>
#include <es_sock.h>
#include <in_sock.h>

/**
@file engine.h
Engine component
@internalComponent	Domain Name Resolver
*/

#include "demon.h"
#include "hosts.h"

const TUint KQueryOrder_PreferA = 0x1;	//< if set, send A first, and then AAAA (preferred query) 
const TUint KQueryOrder_Parallel = 0x2;	//< if set, activate both queries without waiting the first to complete.
const TUint KQueryOrder_OneQuery = 0x4;	//< if set, send only the preferred query (either A or AAAA)
/**
// Configuration parameter which are extracted from the [resolver] sections
*/
class TDndConfigParameters
	{
public:
	TUint iRetries;			//< # of query retransmissions (not counting the first)
	TUint iMinTime;			//< Minimum time for retransmit
	TUint iMaxTime;			//< Maximum time for the total query to complete or fail (when server is known)
	TUint iSetupTime;		//< Maximum time to wait for DNS server to become known
	TUint iSetupPoll;		//< While waiting DNS server, interval for rechecking status (even if no monitor events)
	TUint iSprayMode:1;		//< Each query is sent to all servers with iMinTime interval (on each try)
	TUint iSkipNotFound:1;	//< Ignore "Name not found" answers from server and try another server.
	TUint iQueryHack:1;		//< Enable "QUERYTYPE?Name" special queries
	TUint iQueryOrder:3;	//< Define how A and AAAA queries are to be used.
	TUint iFlushOnConfig:1;	//< Flush DNS cache on intereface configuration change.
#ifdef LLMNR_ENABLED
    TUint iLlmnrLlOnly:1;   //< Accept only link-local replies to LLMNR queries
    TUint iLlmnrPort;       //< Default port of the Linklocal Multicast Name Resolver
	TUint iLlmnrRetries;	//< # of LLMNR query retransmissions (not counting the first)
	TUint iLlmnrMinTime;	//< Minimum time to retransmit the LLMNR query
	TUint iLlmnrMaxTime;	//< Maximum time to do LLMNR query
	TInt  iLlmnrHoplimit;	//< The IP Hoplimit (TTL) to be used in LLMR/MDNS packets.
	TIp6Addr iLlmnrIpv4;	//< The IPv4 multicast address
	TIp6Addr iLlmnrIpv6;	//< The IPv6 multicast address
#endif
	TUint iEDNS0;			//< Enable EDNS0, if >= KDnsMaxMessage, value is receive payload size.
	};

class MTimeoutManager;
class MDndListener;
class CESockIniData;
/**
// Engine part of the DND implementation.
// 
// Function
//
// @li	The engine (there will be only one instantiation) acts as the controller.
// @li	Creates and owns the CdndListener.
// @li	Maintains the communication between the other componens and the console.
//
// Contains
//
// @li	CDndListener object -  to accept socket connection from the applications requiring service
// @li	A view - to show error and log messages
// @li	A File Server handle
// @li	A Socket Server handle
// @li	A timeout manager instance for other components to use
*/
class CDndEngine : public CBase, public MDemonEngine
	{
public:
	CDndEngine(MDemonMain &aMain) : iMain(aMain), iHostsFile(iFS) {}
	~CDndEngine();
	void ConstructL();
	void HandleCommandL(TInt aCommand);

	MDemonMain &iMain;			//< The reference to the main application
public:
	RFs iFS;					//< A File Server session (opened/closed by engine)
	RSocketServ iSS;			//< A Socket Server session (opened/opened by engine)
	RHostsFile iHostsFile;		//< The hosts file source
	RSocket iSocket;			//< And Open UDP socket,for all to use for SetOpt/GetOpt only.

	// @brief If aResult < 0 (= error), output message and leave
	void CheckResultL(const TDesC &aText, TInt aResult);
	// @brief If aResult < 0 (= error), output messsage
	TInt CheckResult(const TDesC &aText, TInt aResult);
	// @brief Output text message (line)
	void ShowText(const TDesC &aText);
	//	@brief Oupput text message using format (line)
	void ShowTextf(TRefByValue<const TDesC> aFmt, ...);
	//
	// Access to the configuration file
	//
	// @brief	Find integer value from configuration with automatic min/max and default checking.
	TInt GetIniValue(const TDesC &aSection, const TDesC &aName, TInt aDefault = 0, TInt aMin = 0, TInt aMax = 1);
	// @brief	Find IP address value from configuration.
	void GetIniAddress(const TDesC &aSection, const TDesC &aName, const TDesC &aDefault, TIp6Addr &aAddr);
	// @brief	Find string value from configuration
	TBool FindVar(const TDesC &aSection, const TDesC &aVarName,TPtrC &aResult);
	// @brief	Find integer value from configuration
	TBool FindVar(const TDesC &aSection, const TDesC &aVarName,TInt &aResult);
	// @brief Release Configuration data
	void UnloadConfigurationFile();
	// @return parsed configuration parameters
	inline const TDndConfigParameters &GetConfig() const { return iParams; }
	// @return handle for the Timeout manager
	MTimeoutManager &Timer() { return *iTimer; }

private:
	// @brief Called implicitly by FindVar methods
	TBool LoadConfigurationFile();

	CESockIniData *iConfig;				//< Configuration data
	TInt iConfigErr;					//< Non-zero, if configuration file is not available
	TDndConfigParameters iParams;		//< Parsed configuration parameters

	MTimeoutManager *iTimer;			//< Generic timer handler (made available for all components)
	MDndListener *iListener;			//< The listener instance
	};

#endif
