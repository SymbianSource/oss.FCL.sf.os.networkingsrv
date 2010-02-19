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
// in6_if.h - control API between the stack and IPv6 interfaces
// Specifies the IPv6 extensions for CNifIfBase::Control() API
// defined in the standard EPOC header file in_iface.h.
//



/**
 @file in6_if.h
 @publishedAll
 @released
*/

#ifndef __IN6_IF_H__
#define __IN6_IF_H__

#include <e32std.h>
#include <in_sock.h>

//	CNifIfBase::Control(aLevel, aName, aOption, ..)
//  aLevel is KSOLInterface defined in in_iface.h in standard EPOC

//	IPv6 specific aName constants and aOption structures

/**
* Option to get the current network interface driver operation parameters to 
* the passed TSoIfInfo6 structure. 
* @since v7.0
* @publishedAll
* @released
*/
const TUint KSoIfInfo6	= 0x202;


/**
* Incoming RMBufPktInfo iFlag value for a loopback packet.
*
* The stack sets this flag for a packet, which is looped
* back by a call to IP layer Process function. A NIF should
* never set this flag.
*
* This flag is effective only when capabilities are enabled.
* A packet with this flag set can be delivered to sockets
* that do not posses NetworkServices cabability.
*/
const TUint KIpLoopbackPacket = 0x1;
/**
* Incoming and outgoing RMBufPktInfo iFlag value for broadcast packet.
*
* The packet uses link layer broadcast. The stack sets this bit for
* outgoing packets that are not unicast (e.g. multicast and broadcast
* destinations). A NIF can set this flag for incoming packet, if it
* was sent to a link layer broadcast address. The presence of this
* flag suppresses some error replies from the stack.
*/
const TUint KIpBroadcastOnLink = 0x2;


/**
* A TSoIfInfo::iFeatures flag to indicate that the interface requires Neighbour 
* Discovery.
*
* @note
*	For IPv4 this enables ARP for the interface. The NIF must
*	pass received ARP packets to the stack, and accept ARP
*	packets for sending from the stack.
* @since v7.0
* @publishedAll
* @released
*/
const TUint KIfNeedsND	= 0x00000100;

const TUint KMaxInterfaceName=32;

/** 
 * Holds the name of a network interface. 
 * 
 * This is used in TSoIfInfo. 
 * 
 */
typedef TBuf<KMaxInterfaceName> TInterfaceName;

class TSoIfInfo
// Socket option structure for KSoIfInfo
/** 
 * Current network interface operation parameters.
 * 
 * It is returned by RSocket::GetOpt(), when that function is called with anOptionLevel 
 * set to KSOLInterface and anOptionName set to KSoIfInfo. 
 *
 */
	{
public:
	/** Feature flags. Possible values are defined in in_iface.h. */
	TUint iFeatures;		// Feature flags
	/** Maximum transmission unit. */
	TInt iMtu;				// Max frame size
	/** An approximation of the interface speed in Kbps. */
	TInt iSpeedMetric;		// Indication of performance, approx to Kbps
	/** Interface protocol name, ipcp::\<port\>. */
	TInterfaceName iName;
	};


class TSoIfInfo6 : public TSoIfInfo		// aOption when aName == KSoIfInfo
	/**
	* Extends the TSoIfInfo for the receive MTU.
	*
	* The IPv6 capable interfaces must support this control option. The usage
	* template in the stack is:
@code
	CNifIfBase *iNif;
	...
	TPckgBuf<TSoIfInfo6> ifProp;
	TInt err = iNif->Control(KSOLInterface, KSoIfInfo6, ifProp);
@endcode
	* For the IPv4 interfaces, only the plain TSoIfInfo is used.
@code
	CNifIfBase *iNif;
	...
	TPckgBuf<TSoIfInfo> ifProp;
	TInt err = iNif->Control(KSOLInterface, KSoIfInfo, ifProp);
@endcode
	* @since v7.0
	* @publishedAll
	* @released
	*/
	{
public:
	/** Maximum transmission unit for receiving. */
	TInt iRMtu;
	};

class TSoIfConfigBase
/** 
 * Base class for TSoInetIfConfig, which simply identifies the protocol family 
 * using the interface. 
 *
 * @internalComponent
 */
	{
public:
	/** The protocol family, e.g. KAfInet. */
	TUint iFamily;
	};

class TSoInet6IfConfig : public TSoIfConfigBase
	/**
	* IPv6 interface configuration.
	*
	* This is the option when stack queries the interface configuration
	* information using
@code
	TPckgBuf<TSoInet6IfConfig> cfg;
	cfg().iFamily = KAfInet6;	// Query about IPv6 capability
	TInt res = iNif->Control(KSOLInterface, KSoIfConfig, cfg);
@endcode
	* The KErrNone return signifies that this NIF supports IPv6 on the
	* link layer. Note, similarly, the IPv4 support is detected by the
	* stack using:
@code
	TPckgBuf<TSoInetIfConfig> cfg;
	cfg().iFamily = KAfInet;	// Query about IPv4 capability.
	TInt res = iNif->Control(KSOLInterface, KSoIfConfig, cfg);
@endcode
	* The same NIF can support both IPv4 and IPv6.
	*
	* @since v7.0
	* @publishedAll
	* @released
	*/
	{
public:	
	/**
	* The local interface id.
	*
	* If the address family is not KAFUnspec, then this defines the id portion of
	* the IPv6 addresses for this host. The id portion is used in constructing the
	* link-local address (fe80::id) and combined with any other prefixes, which
	* are configured for this interface (prefix::id). Prefixes are configured via
	* Router Advertisement prefix option (TInet6OptionICMP_Prefix) with the A flag
	* set, or using interface control socket options (see TSoInet6InterfaceInfo).
	*
	* The length of the id is determined by the TSockAddr::GetUserLen. The normal
	* value is 8 (e.g. the standard id is always 64 bits). Other id lengths are
	* possibly activated by future RFC's for some special address formats.
	*
	* If the address family is KAFUnspec, then id is not configured (and for the
	* IPv6 interface to be functional, address(es), including the link-local address,
	* must be configured by some other means).
	*/
	TSockAddr iLocalId;
	/**
	* The remote interface id (or KAFUnspec, if not applicaple).
	*
	* If the address family is not KAFUnspec, then this defines the id portion of
	* another host on the link. The stack constructs a link-local address
	* (fe80::remote-id) and installs a host route for it.
	*
	* This might be useful for PPP links, if other end is not acting as a router.
	* If the other end is a router, it's address will become automaticly known,
	* when it sends the Router Advertisement.
	*/
	TSockAddr iRemoteId;
	/**
	* Unused highest significant bits in interface id (usually 0).
	*
	* This is reserved for future use, in case there is a need for id length
	* that is not multiple of 8.
	*/
	TUint idPaddingBits;
	/** 1st DNS address (or Unspecified address, if none) */
	TInetAddr iNameSer1;
	/** 2nd DNS address (or Unspecified address, if none) */
	TInetAddr iNameSer2;
	};

/**

@page nif_interface	The interface between a NIF and the TCP/IP stack.

  The network interfaces (NIF's) are registered with the stack using the
  MNifIfUser::IfUserNewInterfaceL function. Stack has an internal object that
  represents the interface and the given CNifIfBase object is attached to this.

  The stack communicates with the NIF using the public API defined by the CNifIfBase.
  The NIF sees the stack as an instance of CProtocolBase and can use a subset of
  public functions to communcite with the stack.

  The following CNifBase functions are used by the stack:

	- CNifIfBase::Open, (binding stack and NIF)
	- CNifIfBase::Close, (binding stack and NIF)
	- CNifIfBase::BindL, (binding stack and NIF)
	- CNifIfBase::Control, (for the configuration information)
	- CNifIfBase::Info, (retrieve the interface name)
	- CNifIfBase::Send, (send outbound packets to NIF)
	- CNifIfBase::Notify, (NIFMAN about packet activity)

  The following CProtocolBase functions are available for NIFs:

	- CProtocolBase::StartSending, (notify stack that NIF is ready)
	- CProtocolBase::Error, (notify stack about NIF error)
	- CProtocolBase::Process, (feed inbound packets to stack)

  The network interface is removed from the stack either by directly deleting it, or
  by NIFMAN using MNifIfUser::IfUserInterfaceDown.

  A pointer to the MNifIfUser object can be obtained from the network
  layer protocol.
@code
	MNetworkService *iNetwork;
	TPckgBuf<MNifIfUser*> ifUser;
	TInt err = iNetwork->Protocol()->GetOption(KNifOptLevel, KNifOptGetNifIfUser, ifUser);
@endcode


@section nif_binding			Binding the NIF and TCP/IP together

  MNifIfUser::IfUserNewInterfaceL introduces a new network interface (NIF) to the stack.
  The introduction consists of the following steps:

  -# retrieve interface info into TNifIfInfo by CNifIfBase::Info function. Stack uses
  only the interface name (iName) from this. The name cannot be an empty string.
  -# using the name, the stack searches for a matching internal interface object. If
  it does not exist, it is created. If there was an existing interface with the same
  name, the stack will disconnect that first.
  -# the stack gives itself to the new NIF by calling CNifIfBase::BindL.
  -# stack does not send any packets to the interface until the NIF has called
  CProtocolBase::StartSending at least once.
  -# stack executes the interface configuration when the first CProtocolBase::StartSending arrives
  after MNifIfUser::IfUserNewInterfaceL. The configuration uses the CNifIfBase::Control function
  with different options to retrieve additional information from the NIF.

  MNifIfUser::IfUserInterfaceDown disconnects the NIF from the stack. There is one
  exception: if the MNifIfUser::IfUserInterfaceDown aResult parameter has a special
  value #KErrLinkConfigChanged, then the internal interface state is only reset to the
  exact same state as if interface was just introduced by the
  MNifIfUser::IfUserNewInterfaceL, and a reconfiguration occurs when the NIF calls
  StartSending.

@section nif_control_api		The Control API

  The stack requires the NIF to implement a minimal set of #KSOLInterface level
  options via it's CNifIfBase::Control API.

	- at least one of the information options
		- TSoIfInfo6 with #KSoIfInfo6 (for IPv6)
		- TSoIfInfo with #KSoIfInfo (for IPv4)
		.
	- at least one of the configuration options
		- TSoInet6IfConfig (iFamily=#KAfInet6) with #KSoIfConfig
		- TSoInetIfConfig (iFamily=#KAfInet) with #KSoIfConfig
		.
	- TSoIfHardwareAddr with #KSoIfHardwareAddr if the link
	uses hardware addresses (only used #KIfNeedsND is also set.). The returned
	address is used in the neighbor discovery (ICMPv6 ND or ARP for IPv4), and
	in sending packets to NIF, the address family is used to indicate that the
	stack has chosen the destination link layer address (based on the neighbor
	cache).
	- TSoIfConnectionInfo with #KSoIfGetConnectionInfo (for IAP and NET numbers).
	If this is not supported, the stack will assign unique numbers for the
	IAP and NET. The scope vector (zone identifiers) is contructed as follows:
		-# [0] The unique interface index (node local scope id)
		-# [1] IAP number (link scope id)
		-# [2] IAP number (subnet scope id)
		-# [3] NET number
		-# [4] NET number (site local scope id)
		-# [5] NET number
		-# [6] NET number
		-# [7] NET number (organization scope id)
		-# [8] NET number
		-# [9] NET number
		-# [10] NET number
		-# [11] NET number
		-# [12] NET number
		-# [13] NET number (IPv6 global scope)
		-# [14] NET number
		-# [15] NET number (highest, NET id, IPv4 global)

  @note
	To build complete ARP packets in the stack, stack needs to know the hardware
	type value to be used in the packet (see TInet6HeaderArp). This 16 bit value
	is assumed to be in the Port() field of the returned hardware address
	(#KSoIfHardwareAddr). An IPv4 NIF that leaves the ARP to the stack,
	must provide this value (or sniff and fix the outgoing ARP packets).

@section nif_inbound_packets	Inbound packets from the NIF to stack.

  The NIF feeds the inbound packets to the stack through the CProtocolBase::Process
  function (see also MNetworkService::Process). The information block associated
  with the packet is RMBufPktInfo and the fields must have been set as follows:

	- RMBufPktInfo::iSrcAddr, the link layer source address (using the same
	address family as returned with the hardware address control option). If
	the link does not use addresses, then #KAFUnspec should be used.
	- RMBufPktInfo::iDstAddr, the link layer destination address (using the same
	address family as returned with the hardware address control option). If
	the link does not use addresses, then #KAFUnspec should be used.
	- RMBufPktInfo::iProtocol, the type of the packet:
		- #KProtocolInetIp, IPv4 packet
		- #KProtocolInet6Ip, IPv6 packet
		- #KProtocolArp, ARP packet
		.
	- RMBufPktInfo::iLength, the length of the packet in octets
	- RMBufPktInfo::iFlags, should be set to zero (reserved for future use).

@note
	The stack is relaxed about the checking of iProtocol field, and anything
	else except #KProtocolArp is assumed to be an IP packet. This is potentially
	to changed in future, and NIFs should set the protocol field correctly.
@note
	The link layer addresses in iSrcAddr and iDstAddr are informative. The
	values do not affect the processing of the packet within stack. They are
	made available for the inbound post hooks (CProtocolPosthook).

@section nif_outbound_packets	Outbound packets from the stack to NIF

  The stack feeds the outbound packets to the NIF through the CNifIfBase::Send
  function. The information block associated with the packet follows RMBufPktInfo
  and the fields need to be interpreted as follows:

	- RMBufPktInfo::iSrcAddr, undefined (must be ignored by the NIF).
	The NIF must choose the link layer source address.
	- RMBufPktInfo::iDstAddr, three variants, if link layer addresses are used
		- hardware address, using the same address family as NIF returned
		in harware address control option (TSoIfHardwareAddr ).
		The packet must be sent to this link layer destination.
		- #KAfInet, the address is IPv4 multicast address. and the NIF must
		select a suitable link layer broadcast address as a destination.
		- #KAfInet6, the address is IPv6 multicast address, and the NIF msut
		select a suitable link layer broadcast address as a destination.
		.
	If the NIF does not use link layer addresses, then iDstAddr is also
	undefined (must be ingnored byt the NIF). The link is a point-to-point
	interface.
	- RMBufPktInfo::iProtocol, defines the type of packet
		- #KProtocolInetIp, IPv4 packet
		- #KProtocolInet6Ip, IPv6 packet
		- #KProtocolArp, ARP packet
		.
	- RMBufPktInfo::iLength, the length of the packet in octets.
	- RMBufPktInfo::iFlags, undefined (must be igrnored by the NIF).

  The stack interprets the return value from the CNifIfBase::Send as follows:

  - return 1; the NIF is ready to receive more packets.
  - return 0; the NIF is blocked and cannot receive any more packets. The stack
  <b>does not send anything</b> to the NIF until it calls CProtocolBase::StartSending.
  - return < 0; unspecified, but currently, the error is passed on to
  all flows attached to this interface. The stack will continue sending
  packets to the interface (no StartSending is required).
  - return > 1; unspecified, but currently treated same as return 1.

*/

#endif
