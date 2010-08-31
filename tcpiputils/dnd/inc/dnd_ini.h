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
// dnd_ini.h - INI File literals
//

#ifndef __DND_INI_H__
#define __DND_INI_H__
/**
@file dnd_ini.h
Define the literals and defaults for DND ini file.

The "dnd.ini" content defined by this is currently included
in "tcpip.ini" as [resolver] and [llmnr] sections.

The current "dnd.ini" file is a different thing, more like resolv.conf
(choice of name "dnd.ini" for that is unfortunate accident, but
too late to change)

@internalComponent	Domain Name Resolver
*/

//
// INI File literals
// *****************
//

_LIT(DND_INI_DATA,        "resolver.ini");	//< the name of the ini-file

_LIT(DND_INI_RESOLVER,    "resolver");		    //< [resolver]
_LIT(DND_INI_RETRIES,     "retries");	        //< retries= 3
_LIT(DND_INI_MINTIME,     "mintime");	        //< mintime= 1
_LIT(DND_INI_MAXTIME,     "maxtime");	        //< maxtime= 30
_LIT(DND_INI_SETUPTIME,   "setuptime");			//< setuptime= 120
_LIT(DND_INI_SETUPPOLL,   "setuppoll");			//< setuppoll= 5
_LIT(DND_INI_SPRAYMODE,   "spraymode");			//< spraymode= 0
_LIT(DND_INI_SKIPNOTFOUND,"skipnotfound");		//< skipnotfound= 0
_LIT(DND_INI_QUERYHACK,	  "queryhack");			//< queryhack= 1
_LIT(DND_INI_QUERYORDER,  "queryorder");		//< queryorder= 0
_LIT(DND_INI_FLUSHONCONFIG, "flushonconfig");	//< flushonconfig= 0
_LIT(DND_INI_EDNS0,       "edns0");				//< edns0= 0 (payload length for received DNS packets, if > 512)

_LIT(DND_INI_HOST,			"host");			//< [host]
_LIT(DND_INI_HOSTNAME,		"hostname");		//< hostname= localhost

#ifdef LLMNR_ENABLED
_LIT(DND_INI_LLMNR, "llmnr");					//< [llmnr]
_LIT(LLMNR_INI_ENTRIES, "llmnrentries");		//< llmnrentries= 0..KLlmnrMaxEnabled
_LIT(LLMNR_INI_ENTRY,   "llmnrentry");			//< prefix for llmnrentryN, 1<=N<=llmnrentries. LLMNR enabled hostname string, no default value
_LIT(LLMNR_INI_LLONLY, "llmnrllonly");			//< llmnrllonly= 0
_LIT(LLMNR_INI_PORT, "llmnrport");				//< llmnrport= 5353
_LIT(LLMNR_INI_RETRIES, "llmnrretries");		//< llmnrretries= 3
_LIT(LLMNR_INI_MINTIME, "llmnrmintime");		//< llmnrmintime= 1
_LIT(LLMNR_INI_MAXTIME, "llmnrmaxtime");		//< llmnrmaxtime= 8
_LIT(LLMNR_INI_NOTIFYTIME, "llmnrnotifytime");	//< llmnrnotifytime= 5
_LIT(LLMNR_INI_RESCANS, "llmnrrescans");		//< llmnrrescans= 4
_LIT(LLMNR_INI_TTL,		"llmnrttl");			//< llmnrttl= 10 (TTL for RR records!)
_LIT(LLMNR_INI_IPV4ADDR,"llmnripv4addr");		//< llmnripv4addr= 224.0.0.251
_LIT(LLMNR_INI_IPV6ADDR,"llmnripv6addr");		//< llmnripv6addr= ff02::fb
_LIT(LLMNR_INI_HOPLIMIT,"llmnrhoplimit");		//< llmnrhoplimit= 255 (TTL for IP packets!)
#endif
//
// Many file parameters will have a hard coded default,
// if the value is not specified in the ini file. Some of the
// default constants are defined below.
//
// The name of the constant is generated directly from the string
// literal name according to the following template
//
//    DND_INI_PARAMETER_NAME => KDndIni_ParameterName
//
_LIT(KDndIni_Hostname, "localhost");	// default host name

const TInt KDndIni_Retries = 3;			//< [1..255]
const TInt KDndIni_MinTime = 1;			//< [1..maxint] [seconds]
const TInt KDndIni_MaxTime = 30;		//< [0..maxint] [seconds]
const TInt KDndIni_SetupPoll = 5;		//< [0..maxint] [seconds]
const TInt KDndIni_QueryHack = 1;		//< [0..1] (off/on)
const TInt KDndIni_EDNS0 = 0;			//< [0..65000]
/**
* How to send A and AAAA queries [0..3].
*
* @li = 0, AAAA and A sequentially
* @li = 1, A and AAAA sequentially
* @li = 2, AAAA and A parallel
* @li = 3, A and AAAA parallel
*/
const TInt KDndIni_QueryOrder = 0;
const TInt KDndIni_SprayMode = 0;		//< [0..1] (off/on)
const TInt KDndIni_SkipNotFound = 0;	//< [0..1] (off/on)
const TInt KDndIni_SetupTime = 30;		//< [0..maxint] [seconds]
#ifdef LLMNR_ENABLED
const TInt KLlmnrIni_LlOnly = 0;        //< [0..1] (off/on)
const TInt KLlmnrIni_Port = 5353;       //< [0..65535]
const TInt KLlmnrIni_Retries = 3;       //< [1..255]
const TInt KLlmnrIni_MinTime = 1;       //< [0..maxint] [seconds]
const TInt KLlmnrIni_MaxTime = 8;
const TInt KLlmnrIni_NotifyTime = 5;    //< [0..maxint] [seconds]
const TInt KLlmnrIni_Rescans = 4;       //< [1..255]
const TInt KLlmnrIni_Ttl = 10;          //< [1..maxint] [seconds]
const TInt KLlmnrIni_Hoplimit = 255;	//< [-1..255] (LLMNR-31 draft)
_LIT(KLlmnrIni_Ipv4Addr, "224.0.0.251");
_LIT(KLlmnrIni_Ipv6Addr, "ff02::fb");
#endif
#endif
