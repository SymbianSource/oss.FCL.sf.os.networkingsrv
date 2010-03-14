/**
* Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
*
*/



/**
 @file
*/

#if !defined(__IN_IFACE_H__)
#define __IN_IFACE_H__

#if !defined(__IN_SOCK_H__)
#include <in_sock.h>
#endif

#if !defined(__NIFVAR_H__)
#include <nifvar.h>
#endif
#include <in6_if.h>
// Feature Flags

/**
Is a loopback interface
@internalAll
*/
const TUint KIfIsLoopback			= 0x00000001;

/**
Is single point to point
@internalAll
*/
const TUint KIfIsPointToPoint		= 0x00000002;

/**
Supports broadcasting
@internalAll
*/
const TUint KIfCanBroadcast			= 0x00000004;

/**
Supports multicasting
@internalAll
*/	
const TUint KIfCanMulticast			= 0x00000008;

/**
Can have its MTU	set
@internalAll
*/	
const TUint KIfCanSetMTU			= 0x00000010;
	
/**
Has a hardware address (ie needs ARP)
@internalAll
*/
const TUint KIfHasHardwareAddr		= 0x00000020;	

/**
Can have its hardware address changed
@internalAll
*/
const TUint KIfCanSetHardwareAddr	= 0x00000040;	

/**
Dialup interface
@internalAll
*/
const TUint KIfIsDialup				= 0x00000080;	

// Control options level received by network interfaces
/** 
 * Option level for network interface driver options.
 * @internalTechnology 
 */
const TUint KSOLInterface			= 0x203;

// Option names
/** 
 * The current network interface driver operation parameters are written to the 
 * passed TSoIfInfo structure. 
 * 
 * An interface that supports only this is assumed IPv4 only.
 * 
 * anOption should be a TPckgBuf<TSoIfInfo>. 
 * 
 * @internalTechnology
 */
const TUint KSoIfInfo				= 0x101;		// Get Interface Information

/** 
 * Gets the interface's local hardware address, if the link layer is using addresses.
 * 
 * anOption should be a TPckgBuf<TSoIfInfo>.
 * 
 * The option is not supported until v7.0. 
 * 
 * @internalTechnology
 */
const TUint KSoIfHardwareAddr		= 0x102;		// Get Hardware Address

/** 
 * Gets the current network interface driver configuration options.
 * 
 * There may be three kind of interfaces: IPv4 only, IPv6 only and hybrid IPv4/IPv6. 
 * The last one is passing traffic for both protocols and thus accepts either 
 * family in a single KSoIfConfig call. Hybrid interfaces must be queried twice, 
 * once for IPv4 and once for IPv6, to get both the IPv4 and the IPv6 settings.
 * 
 * For IPv4, anOption should be a TPckgBuf<TSoInetIfConfig>, for IPv6 TPckgBuf<TSoInet6IfConfig>.
 * 
 * The iFamily field in either TSoInetIfConfig or TSoInet6IfConfig must be set 
 * to either KAfInet for IPv4 or KAfInet6 for IPv6) before the call. If the family 
 * is not supported by the interface, it returns KErrNotSupported. 
 * 
 * @internalTechnology
 */
const TUint KSoIfConfig				= 0x103;		// Get Network parameters

/** 
 * For the IPv4 only stack, compares the passed address with the current local 
 * address, and returns KErrBadName if not equal.
 * 
 * anOption should be a TPckgBuf< TInetAddr >.
 * 
 * This is not used for the IPv4/v6 stack. 
 * 
 * @internalTechnology
 */
const TUint KSoIfCompareAddr		= 0x104;		// Compare address with one passed in

/** Retrieve IAP and NID information 
 * 
 * @internalTechnology
 */
const TUint KSoIfGetConnectionInfo	= 0xf001;		// Retrieve IAP and NID information

class TInetIfConfig
// Information which allows IP to enter route table entries
/** 
 * Describes the IP routing options for a network interface. 
 * 
 * It is used in TSoInetIfConfig. 
 * 
 * @internalComponent
 */
	{
public:
	/** Interface IP address. */
	TInetAddr iAddress;
	/** IP netmask. */
	TInetAddr iNetMask;
	/** IP broadcast address. */
	TInetAddr iBrdAddr;
	/** IP default gateway or peer address (if known). */
	TInetAddr iDefGate;
	/** IP primary name server (if any). */
	TInetAddr iNameSer1;
	/** IP secondary name server (if any). */
	TInetAddr iNameSer2;
	};

class TSoIfHardwareAddr
// Socket option structure for KSoIfHardwareAddr 
/** 
 * An interface's local hardware address.
 * 
 * This is obtained using KSoIfHardwareAddr. 
 *
 * @internalComponent
 */
	{
public:
	/** Local hardware address. */
	TSockAddr iHardwareAddr;
	};

class TSoInetIfConfig : public TSoIfConfigBase
/** 
 * Describes the current interface routing configuration. 
 * 
 * It is returned by RSocket::GetOpt(), when this function is called with anOptionLevel 
 * set to KSOLInterface and anOptionName set to KSoIfConfig. 
 *
 * @internalTechnology
 */
	{
public:
	/** Current interface routing configuration parameters. */
	TInetIfConfig iConfig;
	};

enum TIfProgressNotification
/**
 * @internalAll
 *
 * @deprecated v7.0s - maintained for compatibility with v6.1
 *
 * New software should use the progress ranges defined in nifvar.h
 *
 */
	{
	EIfProgressLinkUp = KMinInterfaceProgress,
	EIfProgressLinkDown,
	EIfProgressAuthenticationComplete
	};

/**
@internalAll
*/
const TInt KErrIfAuthenticationFailure = -3050;

/**
@internalComponent
*/
const TInt KErrIfAuthNotSecure         = -3051;

/**
@internalComponent
*/
const TInt KErrIfAccountDisabled       = -3052;

/**
@internalComponent
*/
const TInt KErrIfRestrictedLogonHours  = -3053;

/**
@internalComponent
*/
const TInt KErrIfPasswdExpired         = -3054;

/**
@internalComponent
*/
const TInt KErrIfNoDialInPermission    = -3055;

/**
@internalComponent
*/
const TInt KErrIfChangingPassword      = -3056;

/**
@internalComponent
*/
const TInt KErrIfCallbackNotAcceptable = -3057;

/**
@internalComponent
*/
const TInt KErrIfDNSNotFound		   = -3058;

/**
@internalComponent
*/
const TInt KErrIfLRDBadLine			   = -3059;   

#endif // __IN_IFACE_H__
