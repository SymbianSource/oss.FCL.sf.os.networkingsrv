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
// llmnrconf.h - LLMNR (linklocal multicast name resolver) configuration
// file parser module header
//

#ifndef _LLMNRCONF_H_
#define _LLMNRCONF_H_
/**
@file llmnrconf.h
LLMNR configuration
@internalComponent	Domain Name Resolver
*/

_LIT8(FORMATHWADDR,"%hw");
_LIT8(WILDCARDPRIMARYHOSTNAME,"*");

class TLlmnrEntry;
class CESockIniData;

enum TIpVer
    {
    EIPany = 0,
    EIPv4 = 4,
    EIPv6 = 6
    };

class THostNameEntry
/**
Describe a template for the answer.

  THis is the base information from which the actual answer
  records (CLlmnrEntry) are generated, based on what interfaces
  are currently up.
*/
	{
public:
	THostName iName;	//< The configured (host) name
	TName iIfName;		//< Interface name
	TIpVer iVersion;	//< indicates, whether LLMNR is enabled in IPv4 or IPv6 interfaces, or both
	};

class CLlmnrConf : public CBase
/**
Load and maintain LLMNR configuration information.
*/
    {
public:
    CLlmnrConf(CDndEngine &aControl);
    ~CLlmnrConf();
    void ConstructL();
	void GetHostNamesL();
    TUint iLlmnrEntries;            //< Number of LLMNR entries in config file
    TUint iNotifyTime;              //< Timeout after notify to rescan the interfaces
    TUint iRescans;                 //< Number of rescans after notify
    TUint iTTL;                     //< TTL of the LLMNR replies
	CArrayFixFlat<THostNameEntry> *iHostList;//< Host Name Entries from configuration

private:
 	CDndEngine &iControl;		    //< The dnd engine
    };

#endif
