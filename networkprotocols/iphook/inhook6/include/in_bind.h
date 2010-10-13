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
// in_bind.h - services provided by the IP layer of the stack
// services provided by the IP layer of the stack
//



/**
 @file in_bind.h
 @publishedPartner
 @released
*/

#ifndef __IN_BIND_H__
#define __IN_BIND_H__

#include <es_sock.h>
#include <es_prot.h>

#include "ip6_hook.h"
#include "apibase.h"

class CProtocolInet6Binder;

// MInterface
// **********
class MInterface : public MInetBase
	/**
	* Public services from the interface instance.
	*
	* @note
	* The MInterface reference must not be stored
	* into any member variables. The reference may become
	* invalid, if thread change occurs.
	* @since v7.0s
	* @publishedPartner
	* @released
	*/
	{
public:
	/**
	* @return the interface index of the interface. Always > 0.
	*/
	virtual TUint32 Index() const = 0;
	/**
	* @return a reference to the name of the interface.
	*/
	virtual const TDesC &Name() const = 0;
	/**
	* Query the value of the specied scope id.
	*
	* @param aType (0..15), select the scope id to be queried
	* @return The scope id value.
	*/
	virtual TUint32 Scope(const TScopeType aType) const = 0;
	};

/**
* @name AddRouteL flags
*
* The flags define the type of the "route" entry and some optional
* processing instructions for the MInterfaceManager::AddRouteL.
*
* The diagram represents 32 bit host order integer.
@verbatim

    |       |       |       |       |       |       |       |       |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
MSB |        Reserved                   |P|U|H|R|O|S| Type Ext  |Typ| LSB
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

Typ     Major type of the entry (type).

Type Ext
        For some route types, the stack uses additional qualifiers
        to track the route state. Should be zeroed.

S       Solicited bit from Neighbor discovery (only used when type = 0).

O       Override bit from Neighbor discovery (only used when type = 0).

R       Neighbor cache for router (only used when type = 0). Change to Router.

H       Neighbor cache for host (only used when type = 0). Change to Host.

U       Update Only (do not create entry, if it does not exist).

P       Probing only.

Reserved
        Must be zero.

@endverbatim
*/
//@{
/**
* type=0, Neighbor cache entry. The "prefix" length is always
* 128 bits. This type of entry stores the link layer address
* for a neighbor on the link.
*
* @note
*	This type is automaticly managaged by the IPv6 neighbor
*	discovery process, or by the IPv4 ARP protocol.
*/
const TUint KRouteAdd_NEIGHBOR	= 0;
/**
* type=1, My own prefix or address entry. This is also used for
* joined multicast groups. The prefix is always 128 bits for
* IPv4 addressess (entry contains the full IPv4 address in
* IPv4 mapped format), for configured specific IPv6 addresses,
* or when entry is used to represent joined multicast group.
*
* The prefix is shorter (usually 64) when
* the entry is used to present IPv6 address prefix that has
* been received via router advertisement (with address
* configuration bit A set).
*
* @note
*	This type is mostly handled automaticly via IPv6 or IPv4
*	configuration. Additionally, for example, joining a multicast
*	group on the interface, adds an entry of this type automaticly.
*/
const TUint KRouteAdd_MYPREFIX	= 1;
/**
* type=2, Onlink route.
*/
const TUint KRouteAdd_ONLINK	= 2;
/**
* type=3, Gateway route, destination offlink, behind the indicated gateway
*/
const TUint KRouteAdd_GATEWAY	= 3;

/**
* Mask (0x3), extracts the main type of the "route" entry (NEIGHBOR, MYPREFIX, ONLINK, GATEWAY)
*/
const TUint KRouteAdd_TYPEMASK	= 0x003;
/**
* Mask (0xff), extracts bits of the route state. System uses internally more states than the four main types.
*/
const TUint KRouteAdd_STATEMASK	= 0x0ff;
//
// - for a host route (= neighbor cache entry), four additional bits can be present
//   (these bits are ignored for the other route types). (Leave 8 bits room for
//   internal state, and start assigning flags from bit 8!)
//
/**
* Usually set from ND solicited bit (usually marks host as reachable).  NEIGHBOR only.
*/
const TUint KRouteAdd_SOLICITED	= 0x0100;
/**
* Update the link layer address. NEIGHBOR only.
*/
const TUint KRouteAdd_OVERRIDE	= 0x0200;
/**
* This host is now a router (not host). NEIGHBOR only.
*/
const TUint KRouteAdd_ISROUTER	= 0x0400;
/**
* This host is now a host (not router). NEIGHBOR only.
*/
const TUint KRouteAdd_ISHOST	= 0x0800;
//
//   When set, it also prevents the processing of ISROUTER/ISHOST, iff entry is
//   in incomplete state, link needs addresses and no link address is specifified
//   (see. CIp6Route::Update). [this subtly implements the NA dropping rule in
//   RFC 2461, 7.2.5 and still passes redirect target on-link route installs,
//   don't mess with this]
//
/**
* Do not create route entry, if it does not exist previously.
*
* By default, AddRouteL always creates a new route entry, if no matching
* route is found. Setting UPDATEONLY flag prevents this.
*/
const TUint KRouteAdd_UPDATEONLY= 0x1000;
/**
* If route is created, mark it as "probing only".
*
* When a route has "probing only" flag set, it is prevented from
* being selected bind "find route". Only useful for Neighbor Cache
* entries. The flag is automaticly cleared, if route state changes
* from Incomplete to some other state.
*/
const TUint KRouteAdd_PROBINGONLY = 0x2000;
//@}

//
// MInterfaceManager
// *****************
// Public services from Interface Manager
class MInterfaceManager : public MInetBase
	/** Provides an interface to the IPv6 protocol module's interface manager.
	*
	* The functions of the MInterfaceManager define the externally visible
	* services of the interface manager.
	*
	* @publishedPartner
	* @released
	*/
	{
public:
	/**
	* Adds or modifies routes.
	*
	* AddRouteL adds the specified (aAddr/aPrefix) route pointing to the interface (aName).
	* If the interface with this name does not exist,
	* an empty interface instance is created for it.
	*
	* @note The route table can also be manipulated using socket options.
	*
	* AddRouteL succeeds always, and the leave condition is only invoked
	* when it fails to allocate the necessary objects to store the information.
	* 
	* The current route management is almost fully automatic,
	* based on information that is retrieved from the active interfaces
	* and from router advertisements.
	*
	* The current implementation uses the route table also for neighbor
	* cache, multicast group membership and stores IPv6 address generation
	* prefixes into route table.
	*
	* The aFlags arguments defines the type of the route entry and
	* some additional processing instructions. The type is one of the
	* following:
	* @li	#KRouteAdd_NEIGHBOR (neighbor cache entry)
	* @li	#KRouteAdd_MYPREFIX (own address/prefix/multicast entry)
	* @li	#KRouteAdd_ONLINK ("traditional" route entry)
	* @li	#KRouteAdd_GATEWAY ("traditional" route entry)
	*
	* Some modifiers can ored with the base type. The following
	* are effective <b>ONLY</b> with KRouteAdd_NEIGHBOR (and used
	* internally):
	* @li	#KRouteAdd_SOLICITED
	* @li	#KRouteAdd_OVERRIDE
	* @li	#KRouteAdd_ISROUTER
	* @li	#KRouteAdd_ISHOST
	*
	* #KRouteAdd_UPDATEONLY can be combined with any route type. It prevents
	* creation of the route entry, if it does not already exist.
	*
	* Type of KRouteAdd_NEIGHBOR routes are generated automaticly by the
	* neighbor discovery process. Careless use of this type
	* may disrupt the neighbor discovery processes.
	* 
	* @param aAddr Address part of the route prefix
	* @param aPrefix The number of bits in the prefix  (aAddr)
	* @param aName The interface name
	* @param aFlags Type of the route entry (KRouteAdd_MYPREFIX or KRouteAdd_ONLINK) and some other flags.
	* @param aGateway (ptr to) a gateway or link layer address depending on the type of route (GATEWAY/NEIGHBOR)
	* @param aLifetime (ptr to) the lifetime of the route in seconds (if zero, route is deleted) 
	*/
	virtual void AddRouteL(const TIp6Addr &aAddr, TInt aPrefix, const TDesC &aName,
		TUint aFlags = KRouteAdd_ONLINK, const TSockAddr *const aGateway = NULL, const TUint32 *const aLifetime = NULL) = 0;
	/** Tests if a route and source address exist for a given address.	
	* 
	* @param aAddr		Address to check
	* @param aScopeid	Address scope
	* @param aSrc		On return, the source address
	* @return 			KErrNone if the route exists, otherwise KErrNotFound
	*
	* @deprecated
	*/
	virtual TInt CheckRoute(const TIp6Addr &aAddr, const TUint32 aScopeid, TIp6Addr &aSrc) const = 0; 

	/**
	* Tests whether the address is usable as a source address within a
	* subset of interfaces limited by the <aLockType, aLock>.
	* 
	* @param aAddr The address (IPv6 or IPv4) to be tested in IPv6 format. IPv4
	* addresses are in IPv4 mapped format.
	* @param aLock The scope identifier for limiting the interfaces
	* @param aLockType The type of the scope identifier
	* @return NON-ZERO scope identifier which matches the tested address, if
	* address is a valid source address for the host. If address is not usable,
	* returns ZERO.
	*/
	virtual TUint32 LocalScope(const TIp6Addr &aAddr, const TUint32 aLock, const TScopeType aLockType) const = 0;
	/**
	* Finds the scope id for the remote address.
	*
	* Returns ZERO, if scope id cannot be determined.
	*
	* @param aAddr The address (IPv6 or IPv4) to be tested in IPv6 format. IPv4
	* addresses are in IPv4 mapped format.
	* @param aLock The scope identifier for limiting the interfaces
	* @param aLockType The type of the scope identifier
	* @return NON-ZERO scope identifier which matches the tested address, if
	* it can be determined (suitable route exists), and ZERO otherwise
	*/
	virtual TUint32 RemoteScope(const TIp6Addr &aAddr, const TUint32 aLock, const TScopeType aLockType) const = 0;
	/**
	* Tests whether a packet having a specified destination address should be processed 
	* by the current host.
	*
	* Suitable addresses include the source addresses covered by the IsMyAddress(), 
	* and also the applicable multicast and broadcast addresses.
	*
	* This method is mainly used by the IP layer.
	* 
	* @param aAddr The address (IPv6 or IPv4) to be tested (in network byte order)
	* @param aInterfaceIndex The source interface (the originating interface is
	* required for accurate determination of link local or multicast addresses)
	* @return NON-ZERO interface index, if destination applies to current node.
	*/
	virtual TUint32 IsForMeAddress(const TIp6Addr &aAddr, const TUint32 aInterfaceIndex) const = 0;

	/**
	* Finds MInterface by CNIfIfBase.
	*
	* Searches the internal inteface descriptions and looks for an interface
	* which is currently bound to the speficied NIF instance. The interface
	* manager does not prevent the situation where the same NIF is attached
	* to multiple internal interfaces. If such configuration is created, the
	* interface which is returned is always the first matching one.
	*
	* @param aIf	The NIF to be searched.
	* @return MInterface or NULL, if NIF not found.
	*/
	virtual const MInterface* Interface(const CNifIfBase *const aIf) const = 0;
	/**
	* Finds MInterface by name.
	*
	* Searches the internal interface descriptions and looks for an interface
	* with the specified name. The returned interface, if found, can be up
	* or down. There cannot be two interfaces with the same name, the name
	* is always a unique identifier of an interface.
	*
	* @param aName	The name to be searched.
	* @return MInterface or NULL, if NIF not found.
	*/
	virtual const MInterface* Interface(const TDesC &aName) const = 0;
	/**
	* Finds MInterface by interface index.
	*
	* Searches the internal interface descriptions and looks for an interface
	* with the specified index. The returned interface, if found, can be up
	* or down. Each interface is assigned a unique index, the index is always
	* a unique indentifier of an interface.
	*
	* @param aInterfaceIndex The index value to be searched.
	* @return MInterface or NULL, if NIF not found.
	*/
	virtual const MInterface* Interface(const TUint32 aInterfaceIndex) const = 0;
	/**
	* Enumerates interface information.
	*
	* This function gets information about the next interface after the interface 
	* identified by the input parameter aIndex.
	* 
	* To start, call first with aIndex=0, and after that always give the previously 
	* returned value as a parameter. When the return value is 0, all interfaces 
	* have been listed.
	* 
	* A single "real" interface has one index value for each possible source address. 
	* TSoInetInterfaceInfo::iName can be used to determine the "real" interface. 
	*
	* Note that TSoInetInterfaceInfo is the structure used by the Symbian OS v6.1 
	* IPv4 stack. The IPv6 interfaces do not provide automatic information about 
	* the name servers or gateways. Some of the information comes from the neighbor 
	* discovery or service location protocols.
	*
	* @param aIndex
	*	previous index value, use ZERO to restart scan from beginning. <b>Note</b>: This
	*	is not same as "interface index", which identifies a interface in the system.
	* @retval aInfo
	*	returns the information about the interface.
	* @return
	*	index of the current aInfo, or ZERO if there were no more entries to return.
	*/
	virtual TUint InterfaceInfo(TUint aIndex, TSoInetInterfaceInfo &aInfo) const = 0;
	/**
	* Enumerates route information.
	* 
	* This function returns information about the next route after the route identified 
	* by the input parameter aIndex. To start, call first with aIndex=0, and after 
	* that always give the previously returned value as a parameter. When the return 
	* is 0, all routes have been listed. 
	* 
	* The returned information is described by TSoInetRouteInfo,
	* which is the structure used by the Symbian OS v6.1 
	* IPv4 stack.
	* 
	* @param aIndex
	*	previous index, use ZERO to restart scan from beginning.
	* @retval aInfo
	*	returns the information about the route
	* @return
	*	index of the current aInfo, or ZERO if there were no more entries to return.
	*/
	virtual TUint RouteInfo(TUint aIndex, TSoInetRouteInfo &aInfo) const = 0;
	/**
	* Gets an option value.
	*
	* Implements some of the options at levels:
	* @li	KSOLInterface
	* @li	KSolInetIfQuery
	*
	* <b>note</b>: Does not necessarily implement all options on those
	* levels, just some.
	*
	* @param aLevel		Option level
	* @param aName		Option name
	* @retval aOption	On return, an option value
	* @return 			System-wide error code
	*/
	virtual TInt GetOption(TUint aLevel, TUint aName, TDes8 &aOption) const = 0;
	/**
	* Sets an option value.
	*
	* Implement some of the options at levels:
	* @li	KSolInetIfCtrl
	* @li	KSOLInterface
	* @li	KSolInetIfQuery
	* @li	KSolInetIp
	* @li	KSolInetRtCtrl
	*
	* <b>note</b>: Does not necessarily implement all options on those
	* levels, just some.
	*
	* @param aLevel 	Option level
	* @param aName 		Option name
	* @param aOption	An option value
	* @return 			System-wide error code
	*/
	virtual TInt SetOption(TUint aLevel, TUint aName, const TDesC8 &aOption) = 0;
	/**
	* Increments the "users" counter.
	*
	* The "users" count decides when the network/IP
	* has no users and the shutdown can be activated. It is
	* up to the upper layers to decide what exactly is counted as a
	* "user" in this sense. By default, any socket opened by an
	* application should count as one user.
	*
	* See also #KSoUserSocket socket option. The built-in protocols of the
	* TCPIP stack (UDP, TCP, ICMP, etc.) support this socket option. Other
	* protocols, which provide sockets to applications, should also support
	* this.
	*
	* When this count is non-ZERO, the "daemons" (as specified in tcpip.ini)
	* are running. When count becomes ZERO, the stack kills the daemon
	* processes.
	*/
	virtual void IncUsers() = 0;
	/**
	* Decrements the "users" counter.
	*
	* For details, see IncUsers().
	*/
	virtual void DecUsers() = 0;
	/**
	* Gets an string variable setting from the network configuration file (such 
	* as tcpip.ini).
	*
	* The function accesses tcpip.ini using the CESockIniData.
	*
	* @param aSection
	*	the section of INI file to be checked (the string inside
	*	the brackets in the INI file). Do not include brackets in
	*	call.
	* @param aVarName
	*	the variable within section. CEsockIniData is kludgy, it will
	*	search for "name=", so beware of using short variable names,
	*	which are part of the ending of another longer name!
	* @retval aResult
	*	returns the pointer to the buffer containing the requested value.
	*	The returned value must not be stored in any member or other long
	*	term variable.
	* @return
	* @li	TRUE, if value found (aResult has been initialized)
	* @li	FALSE, if value not found
	*/
	virtual TBool FindVar(const TDesC &aSection,const TDesC &aVarName,TPtrC &aResult) = 0;
	/**
	* Gets an integer variable setting from the network configuration file (such 
	* as tcpip.ini).
	*
	* The function accesses tcpip.ini using the CESockIniData.
	*
	* @param aSection
	*	the section of INI file to be checked (the string inside
	*	the brackets in the INI file). Do not include brackets in
	*	call.
	* @param aVarName
	*	the variable within section. CEsockIniData is kludgy, it will
	*	search for "name=", so beware of using short variable names,
	*	which are part of the ending of another longer name!
	* @retval aResult
	*	returns the value as an integer.
	* @return
	* @li	TRUE, if value found (aResult has been initialized)
	* @li	FALSE, if value not found
	*/
	virtual TBool FindVar(const TDesC &aSection,const TDesC &aVarName,TInt &aResult) = 0;
	/**
	* Reports a packet has been accepted by some upper layer service provider.
	*
	* An upper layer may call this function to ensure that the incoming interface
	* is not shut down due to lack of traffic. The intention is that only accepted
	* packet traffic can keep the interface up, and only the upper layer protocol
	* can know whether packet is accepted or not. The call is important, if the
	* protocol does not send any packets out to that interface (either because
	* it's a receive only application, or because outbound packets are routed via
	* another interface).
	*
	* @note
	*	The interface index should be the value from the RMBufRecvInfo::iOriginalIndex
	*	field, which represents the real incoming interface.
	*
	* @param aInterfaceIndex The index of the interface from which packet originated
	* @return KErrNotFound, if no such interface, and KErrNone otherwise.
	*/
	virtual TInt PacketAccepted(const TUint32 aInterfaceIndex) = 0;
	/**
	* Gets an option value.
	*
	* Implements some of the options at levels:
	* @li	KSOLInterface
	* @li	KSolInetIfQuery
	*
	* <b>note</b>: Does not necessarily implement all options on those
	* levels, just some.
	*
	* @param aLevel		Option level
	* @param aName		Option name
	* @retval aOption	On return, an option value
	* @param aChecker	The policy checker
	* @return 			System-wide error code
	*/
	virtual TInt GetOption(TUint aLevel, TUint aName, TDes8 &aOption, MProvdSecurityChecker &aChecker) const = 0;
	/**
	* Sets an option value.
	*
	* Implement some of the options at levels:
	* @li	KSolInetIfCtrl
	* @li	KSOLInterface
	* @li	KSolInetIfQuery
	* @li	KSolInetIp
	* @li	KSolInetRtCtrl
	*
	* <b>note</b>: Does not necessarily implement all options on those
	* levels, just some.
	*
	* @param aLevel 	Option level
	* @param aName 		Option name
	* @param aOption	An option value
	* @param aChecker	The policy checker
	* @return 			System-wide error code
	*/
	virtual TInt SetOption(TUint aLevel, TUint aName, const TDesC8 &aOption, MProvdSecurityChecker &aChecker) = 0;
	};


// MNetworkService
// ***************
// Basic services provided by the network layer
//
class MNetworkService : public MFlowManager
	/**
	* Represents the network layer (ip6) of the stack.
	*
	* In addition to own functions, the interface encapsulates some often-used
	* CProtocolBase functions. For other functions, Protocol() gets the real
	* CProtocolBase-derived object, which can be used for other basic functions
	* when required. 
	* @since v7.0
	* @publishedPartner
	* @released
	*/
	{
public:
	/**
	* Gets the underlying protocol object for the network layer (ip6) of the stack.
	* @return Protocol object
	*/
	virtual CProtocolInet6Binder *Protocol() const = 0;
	/**
	* Gets the interface manager for the network layer (ip6) of the stack.
	* @return Interface manager
	*/
	virtual MInterfaceManager *Interfacer() const = 0;

	/**
	* Sends outgoing packet.
	*
	* This is a direct access to the IP layer CProtocolBase::Send function.
	*
	* @param aPacket	The packet
	* @param aSource	Protocol sending the data.
	*
	* The IP layer CProtocolBase::Send is the function used by upper layer
	* protocols to feed in packet to the IP layer. This packet
	* does not normally have the IPv6 or IPv4 headers (unless the
	* #KIpHeaderIncluded flag is set the iFlags), and the packet begins
	* directly with the upper layer protocol header (for example
	* TInet6HeaderUDP or TInet6HeaderTCP).
	*
	* The information block is RMBufSendInfo, which extends the basic
	* RMBufPktInfo by RFlowContext member. This must be correctly
	* initialized. The three alternatives are
@code
	RFlowContext flow;			// some existing flow
	MNetworkService *manager;
	RMBufSendPacket packet;		// unpacked state.
	RMBufSendInfo *info = packet.Info();
	TInt res = KErrNone;

	// 1. No flow context (avoid this if you can)
	info->iFlow = RFlowContext();

	// 2. A new flow context (assuming the base part of info is already set).
	res = info->iFlow.Open(manager, info->iDstAddr, info->iSrcAddr,
			info->iProtocol, icmp_type, icmp_code);

	// 3. A reference to existing opened flow (this will load the base part
	// of the info from the attached flow).
	res = info->iFlow.Open(flow, info);

	// sending the packet.
	if (res == KErrNone)
		{
		aPacket.Pack();
		manager->Send(aPacket);
		}
	else
		{
		// creation failed (res < 0) or is blocked (res > 0)
		info->iFlow.Close();
		}
	aPacket.Free();
@endcode
	* The first two are inefficient. They require a full flow open/close sequence
	* for each packet. The third alternative is the most efficient, because the
	* same flow is re-used for multiple packets. This also enables the use of
	* RFlowContext::SetNotify for asynchronous detection of unblocking or error
	* on the flow.
	*
	* The packet goes through the following steps:
	* -# if a flow context is missing, allocate and connect a new flow
	* context for the packet. The flow selectors are based on address
	* and protocol fields of the RMBufPktInfo (alternative 2. in above).
	* -# if the packet does not have #KIpHeaderIncluded flag set in iFlags
	* of the info, an IPv4 or IPv6 header is added (based on the value of
	* CFlowContext::iHead.ip6.Version()). The content of CFlowContext::iHead.iPacket
	* is copied after the IP header.
	* -# the packet is passed through the MFlowHook::ApplyL function of
	* all attached outbound flow hooks.
	* -# if the packet is longer than the path MTU, it is fragmented. Unless
	* #KIpDontFragment is set. In that case the stack generates an ICMP error
	* message "packet too big".
	* -# the packet (or fragments) are passed through all outbound post
	* hooks (for example, CProtocolPosthook::Send).
	* -# the terminator post hook finally passes the packet(s) to the
	* CFlowContext::Send function, which eventually passes the packet to
	* the CNifIfBase::Send (the packet may need to be queued for a while
	* due to neighbor discovery, or just because NIF is blocked).
	* (see also @ref nif_outbound_packets
	* )
	*/
	virtual TInt Send(RMBufChain &aPacket, CProtocolBase* aSource = NULL) = 0;
	/**
	* Processes incoming packet.
	*
	* This is a direct accesss to the IP layer CProtocolBase::Process function.
	*
	* @param aPacket	The packet
	* @param aSource	The source of the packet (a NIF)
	*
	* The IP layer CProtocolBase::Process is the function used by NIFs to feed
	* in packets from the link layer. The aSource must be a CNifIfBase derived
	* object and known to the interface manager of the stack. Otherwise the
	* IP layer will not accept the packet. The passed packet must follow the rules
	* as described in @ref nif_inbound_packets
	* .
	*
	* The packet goes through the following process:
	* -# the packet is pushed through the inbound posthooks as is
	*	(for example, see CProtocolPosthook::Process), and then queued
	*	for IP processing.
	* -# the packet from the queue is processed as an IP packet and
	*	extension headers are processed by inbound hooks (MIp6Hook::ApplyL)
	* -# the packet is passed to the upper layer protcool (CProtocolBase::Process)
	*
	* If any hook or protocol in the inbound path decides to use this
	* function to re-inject a (modified) packet back to the system,
	* then it must remove the packet from the current inbound processing
	* path.
	*/
	virtual void Process(RMBufChain &aPacket, CProtocolBase* aSource = NULL) = 0;
	/**
	* Binds a protocol or hook to the network layer.
	*
	* This is the same as CProtocolBase::BindL implemented in the
	* ip6 protocol.
	*
	* This is the primary method of installing a upper layer (or some hook)
	* to the IP layer. The aId determines the type of binding
	* -	aId == 0, invalid
	* -	0 < aId <= 255, upper layer bind. The aId is the protocol number
	*	as defined for the IPv4 (protocol in TInet6HeaderIP4) or IPv6
	*	(next header in TInet6HeaderIP) header.
	*	The bind registers aProtocol as an upper layer receiver of all
	*	packets of this protocol. The receiver protocol must be derived
	*	from CProtocolBase (but see also CProtocolInet6Binder, which
	*	can make interfacing easier).
	* - aid > 255, hook bind. The aId determines the type of binding. The
	*	bind registers aProtocol as a hook. The hook protocol must be
	*	derived from CIp6Hook (but, see CProtocolPosthook, which is
	*	derived from CIp6Hook and provides some automatic support
	*	for the hook attachment).
	*
	* See also @ref bindl_interface
	* for more information.
	*
	* @param aProtocol
	*	The protocol or hook requesting the bind
	* @param aId
	*	The bind id.
	*/
	virtual void BindL(CProtocolBase* aProtocol, TUint aId) =  0;

	/**
	* Sends an ICMP (v4) error message based on a received IP packet.
	*
	* This function is used to send an ICMP error message based on
	* received IP packet (stored in aPacket). aPacket must begin
	* with the received IP header (either IPv4 or IPv6)
	* at offset 0.
	*
	* The aPacket must be in "unpacked state" for the info block
	* (assume RMBufRecvPacket::Unpack() has been called).
	*
	* The info block is assumed to be RMBufRecvInfo. But, only the
	* following data is significant:
	*
	* @li RMBufPktInfo::iFlags
	*	only KIpNeverIcmpError flag is tested, and if non-zero, then
	*	no ICMP error will be generated, and packet is just dropped.
	* @li RMBufRecvInfo::iIcmp
	*	must be ZERO. If non-zero, no ICMP error will be generated, and
	*	packet is just dropped. A non-zero iIcmp indicates that the
	*	received packet itself is being processed as an ICMP error
	*	message, and no ICMP error should be generated from ICMP error.
	* @li RMBufRecvInfo::iInterfaceIndex
	*	identifies the interface of the received packet. The ICMP error
	*	message is normally sent to the incoming interface. For any
	*	received packet, this field is properly initialized and should
	*	not be touched. If an ICMP error is to be generated from an
	*	outgoing packet for which no source interface is known,
	*	one can use ZERO here.
	* @li all other fields are ignored.
	*	The source and destination addresses for the ICMP error message
	*	are constructed from the IP header of the packet. The addresses
	*	in the info block are ignored.
	*
	* @param aPacket
	*	The received packet for which the ICMP error is being
	*	generated. This must start with correct IP header (either
	*	IPv4 or IPv6) at offset 0. (The RMbufRecvInfo::iOffset is
	*	ignored, and has no significance). The buffer is "consumed"
	*	by the call, caller does not need to call Free() for the
	*	Packet.
	* @param aType
	*	The type of the ICMP [0..255]
	* @param aCode
	*	The code of the ICMP [0..255]
	* @param aParameter
	*	The parameter value of the ICMP.
	* @param aMC
	*	If non-zero, send ICMP even if the original packet was
	*	sent to a multicast or broadcast address. Normally,
	*	ICMP error messages are not generated from multicast packets.
	*/
	virtual void Icmp4Send(
		RMBufRecvPacket &aPacket,	// The problem packet
		TInt aType,					// ICMP Type
		TInt aCode = 0,				// ICMP Code
		TUint32 aParameter = 0,		// (Depends on Type and Code)
		TInt aMC = 0				// Allow ICMP even if destination was Multicast (if non-zero)
		) = 0;
	/**
	* Sends an ICMP (v6) error message based on a received IP packet.
	*
	* See documentation on the MNetworkService::Icmp4Send method
	* for the parameters.
	*/
	virtual void Icmp6Send(
		RMBufRecvPacket &aPacket,	// The problem packet
		TInt aType,					// ICMP Type
		TInt aCode = 0,				// ICMP Code
		TUint32 aParameter = 0,		// (Depends on Type and Code)
		TInt aMC = 0				// Allow ICMP even if destination was Multicast (if non-zero)
		) = 0;
	//
	// Default Name services
	//
	/**
	* Gets the default name services provider from the network layer.
	*
	* The network layer (IP) provides a gateway to the name
	* resolver implementation for DNS.
	*
	* In EPOC, each protocol is responsible for implementing it's own
	* name resolution. Any protocol wishing to support RhostResolver
	* can get the full DNS support from the network layer by just
	* delegating the call via this method.
	*
	* @return Default name services provider
	*/
	virtual CHostResolvProvdBase *NewHostResolverL() = 0;
	/**
	* Gets the default service resolver provider from the network layer.
	* Not supported, always leaves.
	* @return Default service resolver provider
	*/
	virtual CServiceResolvProvdBase *NewServiceResolverL() = 0;
	/**
	* Gets the default net database provider from the network layer.
	* Not supported, always leaves.
	* @return Default net database provider
	*/
	virtual CNetDBProvdBase *NewNetDatabaseL() = 0;
	};

//
// CProtocolInet6Binder
// ********************
class CProtocolInet6Binder : public CProtocolBaseUnbind
	/**
	* Base class for protocols that bind to the network layer (IPv6 instance).
	*
	* It provides default BindToL() processing, name services, and a pathway to 
	* the IP layer and interface manager through the MNetworkService class.
	*
	* This class was designed for upper layer protocols and the implementation
	* assumes thet the TServerProtocolDesc::iProtocol contains the id of the
	* implemented protocol (for example, the id is 6 for TCP and 17 for UDP).
	* Upper layer protocols can also be implemented using other base classes,
	* for example CProtocolPosthook. The iProtocol field value is also the
	* protocol parameter in RSocket::Open function. This, and the address family
	* determine the protocol module to which the socket is created.
	*
	* @note This is also the base class of the IP protocol instance. A hook 
	* or protocol that binds to or is bound from IP layer, and recognizes it as 
	* being instance of IP layer, can cast the CProtocolBase into this class and 
	* then use the NetworkService() to get full access to the network layer. 
	* @since v7.0
	* @publishedPartner
	* @released
	*/
	{
public:
	IMPORT_C virtual ~CProtocolInet6Binder();
	IMPORT_C virtual void BindToL(CProtocolBase *aProtocol);
	IMPORT_C virtual CHostResolvProvdBase *NewHostResolverL();
	IMPORT_C virtual CServiceResolvProvdBase *NewServiceResolverL();
	IMPORT_C virtual CNetDBProvdBase *NewNetDatabaseL();

	/**
	* Gets the network layer (IPv6) of the stack.
	* @return Network layer (IPv6) of the stack
	*/
 	inline MNetworkService *NetworkService() const
		/**
		* Gets the network layer (IPv6) of the stack.
		* @return Network layer (IPv6) of the stack
		*/
		{ return iNetwork; }

protected:
	IMPORT_C TInt DoBindTo(CProtocolBase *aProtocol);

	/**
	* Network instance, if non-NULL.
	*
	* Initialized and maintained by DoBindTo, The pointer value
	* should be considered as "read-only" by the derived class.
	*/
	MNetworkService *iNetwork;
	};


class MNetworkInfo : public MInetBase
	/** Interface for getting information on interfaces and routes from IP stack.
	@publishedPartner
	@released
	*/
	{
public:
	/**
	* Report all interfaces in a single response.
	*
	* Fill the given memory block by an array of TInetInterfaceInfo objects
	* (Note: different from the old socket option structure).
	*
	* @param aBuffer Buffer where the data is written
	*
	* @return KErrNone (==0) if all interfaces were succesfully written in buffer. If the buffer
	*	  was too small for all interfaces, return the number of interface blocks that didn't
	*	  fit in the buffer. In order to get all interfaces, the caller should probably
	*	  try again with a buffer that is (N * sizeof(TInetInterfaceInfo)) larger. Negative
	*	  return value indicates some other error.
	*/
	virtual TInt GetInterfaces(TDes8& aBuffer) const = 0;

	/**
	* Report all addresses in a single response.
	*
	* Fill the given memory block by an array of TInetAddressInfo objects
	* (Note: different from the old socket option structure).
	*
	* @param aBuffer Buffer where the data is written
	*
	* @return KErrNone (==0) if all addresses were succesfully written in buffer. If the buffer
	*	  was too small for all addresses, return the number of address blocks that didn't
	*	  fit in the buffer. In order to get all addresses, the caller should probably
	*	  try again with a buffer that is (N * sizeof(TInetAddressInfo)) larger. Negative
	*	  return value indicates some other error.
	*/
	virtual TInt GetAddresses(TDes8& aBuffer) const = 0;

	/**
	* Report all routes in a single response. Usage is similar to GetInterfaces and GetAddresses.
	* Data format is an array of TInetRouteInfo structures.
	*/
	virtual TInt GetRoutes(TDes8& aBuffer) const = 0;
	};

const TUint KApiVer_MNetworkInfo = 2;


// Definitions for event service instance used by the tcpip6 stack

/** Number of MEventService event classes used by IP stack. */
const TUint KNumClassesTcpIp6 = 5;

/** MEventService event classes used by the tcpip6 stack.
@publishedPartner
@released
*/
enum
	{
	EClassRoute = 1,	//< Event on routing table. Data: TInetRouteInfo
	EClassAddress,		//< Address event on interface. Data: TInetAddressInfo
	EClassInterface,	//< Interface added or removed. Data: TInetInterfaceInfo
	EClassMulticast,	//< System has joined or left multicast group. Data: TInetMulticastInfo
	EClassNeighbour		//< Events on neighbour cache entries. Data: TInetNeighbourInfo
	};


/** MEventService event types used by the tcpip6 stack.
@publishedPartner
@released
*/
enum
	{
	EventTypeAdd = 1,//< Route or IPv6 address (prefix or id) added on an interface.
	EventTypeDelete,  //< Route or address deleted.
	EventTypeModify  //< Some parameter of an existing Route or address changed
	};

#endif
