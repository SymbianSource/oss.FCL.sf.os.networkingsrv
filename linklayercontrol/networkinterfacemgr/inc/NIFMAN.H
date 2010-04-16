// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Interface Manager API
// 
//

/**
 @file
*/


#if !defined(__NIFMAN_H__)
#define __NIFMAN_H__

#include <nifvar.h>

#ifndef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <nifman_internal.h>
#else
#include <es_sock_partner.h>
#endif

/**
@publishedPartner
*/
enum TAgentConnectType 
	{
	EAgentStartDialOut,
	EAgentReconnect,
	EAgentStartCallBack,
	EAgentNone,
	EAgentStartDialIn
	};

class CNifMan;
class MNifIfUser;
class CNifFactory : public CObject
/**
Manager classes

@publishedPartner
@released
*/
	{
friend class CNifMan;
public:
	IMPORT_C CNifFactory();
	IMPORT_C virtual TInt Open();
	IMPORT_C virtual void Close();
	IMPORT_C static void Cleanup(TAny* aObject);
    IMPORT_C static TInt ControlledDelete(TAny* aFactory);
	IMPORT_C void InitL(RLibrary& aLib, CObjectCon& aCon);

protected:
	IMPORT_C ~CNifFactory();
	virtual void InstallL()=0;
	RLibrary iLib;
	CAsyncCallBack* iAsyncDtor;
	};


/**
Client side classes
async message for progress notifier

@publishedAll
@deprecated 7.0s - replaced with RConnection API
*/
const TInt KDefaultNifSlots = 1;	


/**
@publishedPartner
@released
@capability NetworkControl These control options affect configuration at the designated level.  
@ref RConnection::Control
*/
const TUint KCOLInterface = 100;

/**
@publishedPartner
@released
@capability NetworkControl These control options affect configuration at the designated level.  
@ref RConnection::Control
*/
const TUint KCOLAgent     = 200;


// RConnection::Ioctl() level for NIFMAN

/**
@publishedPartner
@released
@ref RConnection::Ioctl
*/
const TUint KCOLConfiguration = 300;

// RConnection::Control() options

/**
@publishedPartner
@released
@capability NetworkControl Restrict ability to switch on/off idle timers  
@ref RConnection::Control
*/
const TUint KConnDisableTimers = KConnReadUserDataBit | 1;

/**
@publishedPartner
@released
@ref RConnection::Control
*/
const TUint KConnGetInterfaceName = KConnReadUserDataBit | KConnWriteUserDataBit | 2;


// RConnection::Ioctl() options

/**
@publishedPartner
@released
@ref RConnection::Ioctl
*/
const TUint KConnGetCurrentAddr = KConnWriteUserDataBit | 3;

/**
@publishedPartner
@released
@ref RConnection::Ioctl
*/
const TUint KConnGetServerAddr = KConnWriteUserDataBit | 4;

/**
@publishedPartner
@released
@ref RConnection::Ioctl
*/
const TUint KConnGetAddrLeaseTimeRemain = KConnWriteUserDataBit | 5;

/**
@publishedPartner
@released
@capability NetworkControl Restrict ability to release a configured address  
@ref RConnection::Ioctl
*/
const TUint KConnAddrRelease = 6;

/**
@publishedPartner
@released
@capability NetworkControl Restrict ability to renew a configured address  
@ref RConnection::Ioctl 

This option is used for user initiated RENEW request where an attempt 
is made to renew the lease obtained from the orginal DHCP server.
If the server response is not received before the default timeout(RebindTimeT2 - RenewalTimeT1)
the dhcp client will then initiate a REBIND.An user defined timeout can also be
supplied when using this option which will override the default timeout value. 

@code
	RConnection conn; 
	TRequestStatus stat;
	//Start a connection 
	..... 
	.....	
	//Option1: Initiate a Renew request. 
	conn.Ioctl(KCOLConfiguration, KConnAddrRenew, stat);

	//Option2: Initiate a Renew request with a user defined timeout 
	TInt secValue(2); //Eg timeout set to 2secs
	TPckg<TInt> val(secValue);		
	conn.Ioctl(KCOLConfiguration, KConnAddrRenew, stat,&val);
@endcode
*/
const TUint KConnAddrRenew = 7;

// DHCP specific RConnection::Ioctl options
/**
@publishedPartner
@released
@ref RConnection::Ioctl
*/const TUint KConnGetDhcpRawOptionData = KConnWriteUserDataBit|KConnReadUserDataBit|100;

/**
@publishedPartner
@released
@ref RConnection::Ioctl
*/
const TUint KConnGetSipServerAddr = KConnWriteUserDataBit|KConnReadUserDataBit|101;

/**
@publishedPartner
@released
@ref RConnection::Ioctl
*/
const TUint KConnGetSipServerDomain = KConnWriteUserDataBit|KConnReadUserDataBit|102;

/**
* This constant is used to retrieve the DHCP Header Sname which is the 
* host name of the next available server. This is sometimes overloaded 
* to carry option value 66 which is the TftpServerName. 
* @publishedPartner
* @released
* @see RConnection::Ioctl()
*/
const TUint KConnGetDhcpHdrSname = KConnWriteUserDataBit|KConnReadUserDataBit|104;

/**
* This constant is used to retrieve the DHCP Header Siaddr which is the 
* IPAddress of the next available server.
* @publishedPartner
* @released
* @see RConnection::Ioctl()
*/
const TUint KConnGetDhcpHdrSiaddr = KConnWriteUserDataBit|KConnReadUserDataBit|105;

/**
* This constant is used to retrieve the DHCP Option 66, Tftp Server Name.
* @publishedPartner
* @released
* @see RConnection::Ioctl()
*/
const TUint KConnGetTftpServerName = KConnWriteUserDataBit|KConnReadUserDataBit|106;

/**
* This constant is used to retrieve the DHCP Option 150, Tftp Server Address.
* @publishedPartner
* @released
* @see RConnection::Ioctl()
*/
const TUint KConnGetTftpServerAddr = KConnWriteUserDataBit|KConnReadUserDataBit|107;


/**
* This constant is used to retrieve multiple opcode data in a raw format.
* @publishedPartner
* @released
* @see RConnection::Ioctl()
*/
const TUint KConnDhcpGetMultipleParams  = KConnWriteUserDataBit|KConnReadUserDataBit|108;

/**
@publishedPartner
@released
*/
const TUint KConnMaxInterfaceName = 32;

class TConnInterfaceName
/**
@publishedPartner
@released
*/
	{
public:
	TUint iIndex;
	TBuf<KConnMaxInterfaceName> iName;
	};


/**
* This constant is used to provision hardware address in the DHCP server. This enables DHCP server to assign the only available IP address in
* its pool to the authorised hardware address as configured by the application.
* @publishedPartner
* @released
* @see RConnection::Ioctl()
*/
const TUint KConnDhcpSetHwAddressParams = KConnWriteUserDataBit|KConnReadUserDataBit|109;
 
#ifdef SYMBIAN_TCPIPDHCP_UPDATE 
/**
 * This constant is used to retrieve list of domain names to be searched during name resolution.
 * Ref : RFC 3646 sec 4
 * @publishedPartner
 * @released
 * @see RConnection::Ioctl
*/
const TUint KConnGetDomainSearchList = KConnWriteUserDataBit|KConnReadUserDataBit|110;

/**
 * This constant is used to retrieve list of IPv6 addresses of DNS recursive name servers to which a client's DNS
   resolver will send DNS queries.
 * Ref: RFC 3646 sec 3
 * @publishedPartner
 * @released
 * @see RConnection::Ioctl
*/
const TUint KConnGetDNSServerList = KConnWriteUserDataBit|KConnReadUserDataBit|111;
#endif //SYMBIAN_TCPIPDHCP_UPDATE 

#endif // __NIFMAN_H__
