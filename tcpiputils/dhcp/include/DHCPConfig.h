// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Provides DHCP options
// 
//

/**
 @file DHCPConfig.h
*/
#if (!defined __DHCPCONFIG_H__)
#define __DHCPCONFIG_H__

#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS

//dhcpv4 options
/**
@publishedPartner
@released
*/
const TUint KSubnetMask              = 1;

/**
@publishedPartner
@released
*/
const TUint KTimeOffset              = 2;

/**
@publishedPartner
@released
*/
const TUint KRouterAddress           = 3;

/**
@publishedPartner
@released
*/
const TUint KTimeServers             = 4;

/**
@publishedPartner
@released
*/
const TUint KIen116NameServers       = 5;

/**
@publishedPartner
@released
*/
const TUint KDomainNameServers       = 6;

/**
@publishedPartner
@released
*/
const TUint KLogServers              = 7;

/**
@publishedPartner
@released
*/
const TUint KCookieServers           = 8;

/**
@publishedPartner
@released
*/
const TUint KLprServers              = 9;

/**
@publishedPartner
@released
*/
const TUint KImpressServers          = 10;

/**
@publishedPartner
@released
*/
const TUint KRlpServers              = 11;

/**
@publishedPartner
@released
*/
const TUint KHostName                = 12;

/**
@publishedPartner
@released
*/
const TUint KBootFileSize            = 13;

/**
@publishedPartner
@released
*/
const TUint KMeritDumpFile           = 14;

/**
@publishedPartner
@released
*/
const TUint KDomainName              = 15;

/**
@publishedPartner
@released
*/
const TUint KSwapServerAddress       = 16;

/**
@publishedPartner
@released
*/
const TUint KRootDisk                = 17;

/**
@publishedPartner
@released
*/
const TUint KExtensionsPath          = 18;

// ip layer parameters - per host

/**
@publishedPartner
@released
*/
const TUint KBeARouter              = 19;

/**
@publishedPartner
@released
*/
const TUint KNonLocalSourceRouting  = 20;

/**
@publishedPartner
@released
*/
const TUint KPolicyFilterForNlsr    = 21;

/**
@publishedPartner
@released
*/
const TUint KMaxReassemblySize      = 22;

/**
@publishedPartner
@released
*/
const TUint KDefaultTtl             = 23;

/**
@publishedPartner
@released
*/
const TUint KPmtuAgingTimeout       = 24;

/**
@publishedPartner
@released
*/
const TUint KPmtuPlateauTable       = 25;

// link layer parameters - per interface.

/**
@publishedPartner
@released
*/
const TUint KMtu                    = 26;

/**
@publishedPartner
@released
*/
const TUint KAllSubnetsMtu          = 27;

/**
@publishedPartner
@released
*/
const TUint KBroadcastAddress       = 28;

/**
@publishedPartner
@released
*/
const TUint KPerformMaskDiscovery   = 29;

/**
@publishedPartner
@released
*/
const TUint KBeAMasksupplier        = 30;

/**
@publishedPartner
@released
*/
const TUint KPerformRouterDiscovery = 31;

/**
@publishedPartner
@released
*/
const TUint KRouterSolicitationAddr = 32;

/**
@publishedPartner
@released
*/
const TUint KStaticRoutes           = 33;

/**
@publishedPartner
@released
*/
const TUint KTrailers               = 34;

/**
@publishedPartner
@released
*/
const TUint KArpCacheTimeout        = 35;

/**
@publishedPartner
@released
*/
const TUint KEthernetEncapsulation  = 36;

// tcp paramters 

/**
@publishedPartner
@released
*/
const TUint KTtl                    = 37;

/**
@publishedPartner
@released
*/
const TUint KKeepAliveInterval      = 38;

/**
@publishedPartner
@released
*/
const TUint KKeepAliveDataSize      = 39;

// application layer parameters

/**
@publishedPartner
@released
*/
const TUint KNetworkInfoServiceDom  = 40;

/**
@publishedPartner
@released
*/
const TUint KNetworkInfoServers     = 41;

/**
@publishedPartner
@released
*/
const TUint KNetworkTimeServers     = 42;

// vender specific information option

/**
@publishedPartner
@released
*/
const TUint KVendorSpecInfo         = 43;

// netbios over tcp/ip name server option

/**
@publishedPartner
@released
*/
const TUint KNetbiosNameServer      = 44;

/**
@publishedPartner
@released
*/
const TUint KNetbiosDatagramServer  = 45;

/**
@publishedPartner
@released
*/
const TUint KNetbiosNodeType        = 46;

/**
@publishedPartner
@released
*/
const TUint KNetbiosScopeOption     = 47;

// x window system options.

/**
@publishedPartner
@released
*/
const TUint KXwindowFontServer      = 48;

/**
@publishedPartner
@released
*/
const TUint KXwindowDisplayManager  = 49;

// other extensions

/**
@publishedPartner
@released
*/
const TUint KRequestedAddress       = 50;

/**
@publishedPartner
@released
*/
const TUint KLeaseTime              = 51;

/**
@publishedPartner
@released
*/
const TUint KOkToOverlay            = 52;

/**
@publishedPartner
@released
*/
const TUint KMessageType            = 53;

/**
@publishedPartner
@released
*/
const TUint KServerIdentifier       = 54;

/**
@publishedPartner
@released
*/
const TUint KParameterRequestList   = 55;

/**
@publishedPartner
@released
*/
const TUint KMessage                = 56;

/**
@publishedPartner
@released
*/
const TUint KMessageLength          = 57;

/**
@publishedPartner
@released
*/
const TUint KRenewalTime            = 58;     // t1

/**
@publishedPartner
@released
*/
const TUint KRebindTime             = 59;     // t2

/**
@publishedPartner
@released
*/
const TUint KClientClassInfo        = 60;

/**
@publishedPartner
@released
*/
const TUint KClientId               = 61;

/**
@publishedPartner
@released
*/
const TUint KTFtpServerName         = 66;

/**
@publishedPartner
@released
*/
const TUint KBootFileName           = 67;

/**
@publishedPartner
@released
*/
const TUint KHomeAgentAddrs   		= 68;

/**
@publishedPartner
@released
*/
const TUint KSmtpServer 			= 69;      

/**
@publishedPartner
@released
*/
const TUint KPop3Server 			= 70;      

/**
@publishedPartner
@released
*/
const TUint KNntpServer 			= 71;      

/**
@publishedPartner
@released
*/
const TUint KWwwServer	 			= 72;      

/**
@publishedPartner
@released
*/
const TUint KFingerServer 			= 73;

/**
@publishedPartner
@released
*/
const TUint KIrcServer 				= 74;      

/**
@publishedPartner
@released
*/
const TUint KStreetTalkServer 		= 75;

/**
@publishedPartner
@released
*/
const TUint KSTdaServer 			= 76;

/**
@publishedPartner
@released
*/
const TUint KUserClass 				= 77;     

/**
@publishedPartner
@released
*/
const TUint KDirectoryAgent 		= 78;

/**
@publishedPartner
@released
*/
const TUint KServiceScope 			= 79;   

/**
@publishedPartner
@released
*/
const TUint KRapidCommit 			= 80;    

/**
@publishedPartner
@released
*/
const TUint KClientFQDN 			= 81; 

/**
@publishedPartner
@released
*/
const TUint KRelayAgentInformation 	= 82;

/**
@publishedPartner
@released
*/
const TUint KLDAP 					= 95;

/**
@publishedPartner
@released
*/
const TUint KGeoConfCivicOption		= 99;

/**
@publishedPartner
@released
*/
const TUint KAutoConfig 			= 116;     

/**
@publishedPartner
@released
*/
const TUint KNameServiceSearch		= 117;

/**
@publishedPartner
@released
*/
const TUint KSubnetSelectionOption	= 118;

/**
@publishedPartner
@released
*/
const TUint KDomainSearch 			= 119;

/**
@publishedPartner
@released
*/
const TUint KGeoConfOption 			= 123;

/**
@publishedPartner
@released
*/
const TUint KRemoteStatisticsServer = 131;

/**
@publishedPartner
@released
*/
const TUint KDiffserv 				= 134;

/**
@publishedPartner
@released
*/
const TUint KTFtpServerAddress 		= 150;

#endif // SYMBIAN_NETWORKING_DHCP_MSG_HEADERS

#ifdef SYMBIAN_TCPIPDHCP_UPDATE

//dhcpv6 options
/**
@publishedPartner
@released
*/
const TUint KV6ClientId                 = 1;

/**
@publishedPartner
@released
*/
const TUint KV6ServerId                 = 2;

/**
@publishedPartner
@released
*/ 
const TUint KV6IA_NA                    = 3;

/**
@publishedPartner
@released
*/ 
const TUint KV6IA_TA                    = 4;

/**
@publishedPartner
@released
*/ 
const TUint KV6IAADDR                   = 5;

/**
@publishedPartner
@released
*/ 
const TUint KV6ORO              	    = 6;

/**
@publishedPartner
@released
*/ 
const TUint KV6Preference               = 7;

/**
@publishedPartner
@released
*/ 
const TUint KV6ElapsedTime              = 8;

/**
@publishedPartner
@released
*/ 
const TUint KV6IRelayMsg                = 9;

/**
@publishedPartner
@released
*/ 
const TUint KV6Auth               	    = 11;

/**
@publishedPartner
@released
*/ 
const TUint KV6Unicast                  = 12;

/**
@publishedPartner
@released
*/ 
const TUint KV6StatusCode               = 13;
/**
@publishedPartner
@released
*/ 
const TUint KV6RapidCommit              = 14;
/**
@publishedPartner
@released
*/ 
const TUint KV6UserClas                 = 15;
/**
@publishedPartner
@released
*/ 
const TUint KV6VendorClass              = 16;

/**
@publishedPartner
@released
*/ 
const TUint KV6VendorOpts               = 17;

/**
@publishedPartner
@released
*/ 
const TUint KV6InterfaceId              = 18;
/**
@publishedPartner
@released
*/ 
const TUint KV6ReconfMsg                = 19;

/**
@publishedPartner
@released
*/ 
const TUint KV6ReconfAccept             = 20;

/**
@publishedPartner
@released
*/ 
const TUint KV6SipServerDomainNameList  = 21;


/**
@publishedPartner
@released
*/ 
const TUint KV6SipServerIpv6AddressList = 22;


/**
@publishedPartner
@released
*/ 
const TUint KV6DNSRecursiveNameServer   = 23;


/**
@publishedPartner
@released
*/ 
const TUint KV6DomainSearchList         = 24;


/**
@publishedPartner
@released
*/ 
const TUint KV6IA_PD                    = 25;


/**
@publishedPartner
@released
*/ 
const TUint KV6IA_Prefix                = 26;


/**
@publishedPartner
@released
*/ 
const TUint KV6NisServers               = 27;


/**
@publishedPartner
@released
*/ 
const TUint KV6NispServers              = 28;

/**
@publishedPartner
@released
*/ 
const TUint KV6NisDomainName            = 29;

/**
@publishedPartner
@released
*/ 
const TUint KV6NispDomainName           = 30;

/**
@publishedPartner
@released
*/ 
const TUint KV6SNTPServerList           = 31;

/**
@publishedPartner
@released
*/ 
const TUint KV6InformationRefreshTime   = 32;

/**
@publishedPartner
@released
*/ 
const TUint KV6NewPOSIXTimezone   = 41;
/**
@publishedPartner
@released
*/ 
const TUint KV6NewTZTBTimezone   = 42;

#endif //SYMBIAN_TCPIPDHCP_UPDATE 

#endif
