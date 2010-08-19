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
// servers.cpp - server manager module
//

#ifndef __SERVERS_H__
#define __SERVERS_H__

/**
@file servers.h
DNS server list maintenance
@internalComponent	Domain Name Resolver
*/

#include <in_sock.h>

/**
// @brief	The name resolution scope level and type (multicast or unicast)
//
// The server scope defines the scope level [1..16] which is
// served by the server. Additionally, the scope is stored
// either as positive integer (if resolution is based on
// multicast strategy), or negative number if resolution is
// based on traditional DNS server.
//
// Currently only two server scopes are used:
// @li	Link Local Scope, using multicast resolution (LLMNR)
// @li	Global Scope, using unicast resolution (the traditional DNS)
*/
typedef enum
	{
	// Node local hosts file Resolution
	EDnsServerScope_HOSTFILE = 0,
	// Globan Unicast Resolution
	EDnsServerScope_UC_GLOBAL = -KIp6AddrScopeGlobal,
	// Link Local Multicast Resolution
	EDnsServerScope_MC_LOCAL = KIp6AddrScopeLinkLocal
	} TDnsServerScope;


// Select a virtual sublist of currently known servers
class TDnsServerFilter
	{
public:
	inline TBool IsUnicast() const { return iServerScope < 0; }
	inline TBool IsMulticast() const { return iServerScope > 0; }
	TInt iServerId;					//< Current default server (0 = first matching server)
	TUint32 iLockId;				//< Eligible severs must be from "locked" scope
	TUint8 iLockType;				//< Locked scope level [1,,16]
	TDnsServerScope iServerScope:8;	//< Server scope and type
	THostName  iDomainName;         //< Domain name of the query for interface selection
	};

class MDnsServerListNotify
	{
public:
	// Called when the requested server list has become known (if request was pending)
	virtual void ServerListComplete(const TDnsServerFilter &aList, TInt aResult) = 0;
	};

class MDnsServerManager
	{
public:
	/**
	// @brief Virtual desctructor.
	//
	// The actual server manager object can be deleted by a delete of this mixin class.
	*/
	virtual ~MDnsServerManager() {}
	/**
	// @brief Informs that the system configuration has changed.
	// This triggers the rescanning of the interfaces and
	// availabla name server addresses.
	*/
	virtual void ConfigurationChanged() = 0;
	/**
	// @brief	Open the server list (if not already open)
	// @param	aFilter	the server filter
	// @param	aNotify	the callback, which is used only if server list is not yet available
	// @returns
	//	@li	< 0, fatal error (notify callback is not installed)
	//	@li = 0, server list is available (notify callback is not installed)
	//	@li > 0, server list is not available, the notify callback will be called later
	*/
	virtual TInt OpenList(const TDnsServerFilter &aFilter, MDnsServerListNotify *aNotify) = 0;
	/**
	// @brief	Count the number of servers matching the filter
	// @param	aFilter	the server filter
	// @returns	the number of servers matching the filter
	*/
	virtual TInt Count(const TDnsServerFilter &aFilter) const = 0;
	/**
	// @brief	Return id of the next matching server after the specified server
	// @param	aFilter	the server filter
	// @param	aServerId	id of the current server (if = 0, the first matching server is returned)
	// @returns
	//	@li	< 0, error [does not happen currently]
	//	@li = 0, there is no next server (server list is empty or only had the single server)
	//	@li > 0, is the id of the next server ( != aServerId)
	*/
	virtual TInt Next(const TDnsServerFilter &aFilter, TInt aServerId) const = 0;
	/**
	// @brief	Request is not using the serverlist anymore
	// @param	aFilter	the server filter (cancel notify callback for this)
	*/
	virtual void CloseList(const TDnsServerFilter &aFilter) = 0;
	/**
	// @brief	Return the IP addres of the server
	// @param	aServerId	identify the server
	// @retval	aAddr		the server address corresponding to the server id, if return OK
	// @returns
	//	@li	KErrNone (OK), if a server with the specified ID exist and address returned
	//	@li KErrNotFound, if there is no server corresponding the specified id
	*/
	virtual TInt Address(TInt aServerId, TInetAddr &aAddr) const = 0;
	/**
	// @brief	Assuming addr came from the server, return the matching scope id
	// @param	aServerId	identify the server
	// @param	aAddr		the address to examine
	// @returns
	//	@li = 0,  cannot find scope id for the address (probably no server matching the id)
	//	@li != 0, is the scope id for the address assuming it originated from the specified server
	*/
	virtual TUint32 Scope(TInt aServerId, const TInetAddr &aAddr) = 0;
	/**
	// @brief	Search server id by server address
	// @param	aAddr	a address of the server to be looked (must be KAfInet6)
	// @returns	the corresponding server id (> 0), if the list contained a
	//			matching server
	*/
	virtual TInt ServerId(const TInetAddr &aAddr) const = 0;
	/**
	// @brief	Return the name space id matching the filter
	// @param	aFilter	the server filter
	// @param	aServerId the specific server, if non-zero
	// @returns
	//	@li = 0, cannot find the name space id
	//	@li != 0, is the name space id corresponding the filter
	*/
	virtual TUint32 NameSpace(const TDnsServerFilter &aFilter, TInt aServerId) const = 0;
	/**
	// @brief	Rebuild interface and server lists
	// @returns
	//	@li KErrNone, if list rebuilt successfully (list can be empty!)
	//	@li < 0, if there are some fatal socket errors
	*/
	virtual TInt BuildServerList() = 0;
	/**
	// @brief	Add DNS server address for a interface
	//
	// The specified address will be included into the server list when the
	// named interface is active.
	//
	// @param	aInterface	is the name of the interface
	// @param	aAddr		DNS server address
	//
	*/
	virtual void AddServerAddress(const TName &aInterface, const TInetAddr &aAddr) = 0;
	/**
	// @brief	Lock filter scope by address
	// @param	aAddr	the address
	// @param	aNetwork	the network id
	// @param	aFilter	the server filter to modify
	*/
	virtual void LockByAddress(const TInetAddr &aAddr, TUint32 aNid, TDnsServerFilter &aFilter) = 0;

	/**
	// @brief	Retrieves the domain suffix list set on the interface associated with the connection
	// @param	aServerId	The id of the server associated with the connection
	// @param	aSuffixList reference to array for reading the interface specific domain suffices
	*/
	virtual void InterfaceSuffixList(TInt aServerId, RInetSuffixList& aSuffixList) = 0;
	/**
	// @brief	Modifies the outbound query properties so that the interface associated
	//				with the query supports the domain name of the query
	// @param	aFilter	Reference to server properties associated with the query
	*/
	virtual void UpdateDomain(TDnsServerFilter &aFilter) const = 0;
	};

class CDndEngine;
class DnsServerManager
	{
public:
	// Create a new server manager instance
	static MDnsServerManager *NewL(CDndEngine &aControl);
	};

#endif
