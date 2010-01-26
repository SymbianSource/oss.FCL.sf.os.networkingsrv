// Copyright (c) 1999-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// napt_ini.h - INI File literals
// 
//

#ifndef __NAPT_INI_H__
#define __NAPT_INI_H__

/**
* @file 
*  
* Define NAPT.INI configuration parameters
*/

/**
* napt_ini_parameters	NAPT.INI configuration parameters
*
* @{
*/

//
// INI File literals
// *****************
//

/** The name of the ini-file */
_LIT(NAPT_INI_DATA,                "napt.ini");


/**
* Napt Timer Section  
*/
_LIT(NAPT_INI_TIMER,				"napt_timer");

/**
* Protocol Timeout Configuration parameters
*/


/**
* Napt Translation Table Scan Interval in Seconds
*/
_LIT(NAPT_INI_TABLESCANINTERVAL,            "napt_table_scan_interval");



/**
* Napt Udp Inactive Connection Timout in Seconds
*/
_LIT(NAPT_INI_ICMPIDLETIMEOUT,                 "napt_icmp_idle_timeout");		    

/**
* Napt Udp Inactive Connection Timout in Seconds
*/
_LIT(NAPT_INI_UDPIDLETIMEOUT,              "napt_udp_idle_timeout");	            



/**
* Napt Tcp Inactive Connection Timeout in Seconds
*/
_LIT(NAPT_INI_TCPIDLETIMEOUT,		"napt_tcp_idle_timeout");

/**
* Napt Tcp Close Timewait in Seconds
*/
_LIT(NAPT_INI_TCPCLOSETIMEOUT,		"napt_tcp_close_timeout");

/**
* Napt Tcp Open Timewait in Seconds
*/
_LIT(NAPT_INI_TCPOPENTIMEOUT,		"napt_tcp_open_timeout");

#endif
