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
// in6_opt.h - new socket options and MEventService data e.g.
// for accessing the network interface and route
// information.
// New socket options and MEventService data e.g. for accessing the network interface
// and route information.
//



/**
 @file in6_opt.h
 @publishedAll
 @released
*/

#ifndef __INSOCK_IN6_ROUTE_H__
#define __INSOCK_IN6_ROUTE_H__

#include <in_sock.h>


// PS: I'm declaring the following rule:
// ** Use socket option names above 0x1000 for all options defined in this file **
// (don't want to accidentally collide with in_sock.h, even though the probability is
// small considering the number of different sockopt levels)


// -- Socket option level: KSolInetIp --

/**
Controls the use of Explicit Congestion Notification. Values:
@li 0 = ECN disabled
@li 1 = ECN enabled with ECT(1)
@li 2 = ECN enabled with ECT(0) (recommended over ECT(1), because some implementations may
not implement ECT(1))

See RFC 3168 for more information.
@publishedAll
@released
*/
const TUint KSoIpEcn = 0x1010;

/**
Next hop route selection.

Set forces the next hop route selection on the flow.
The option parameter is not used in set.

Get returns information about the current next hop
selection. The option parameter is TInetRouteInfo.

If the link layer is using addresses, the information
refers to neighbor cache entry. If the interface is not
using link layer addresses, the returned information just
describes the currently attached route entry.

To be successful, the flow must be assigned to the
interface and the source address must be set at the
time of the call.

This option is provided for hook implementations, for
example ISATAP tunneling hook can use this¨to force
next hop selection on the virtual interface to find the
actual link layer address (= outer IPv4 address).
*/
const TUint KSoNextHop = 0x1011;


// -- Socket option level: KSolInetTcp --
/**
If set, only full-sized TCP segments are sent before closing the connection. This is like
Nagle, but stricter.
@publishedAll
@released
*/
const TUint KSoTcpCork = 0x1020;

/**
Send only full-sized TCP segments. Separate option in addition to KSoTcpCork is needed for
BSD compatibility.
@publishedAll
@released
*/
const TUint KSoTcpNoPush = 0x1021;

/**
Do not return from close immediately, but linger for given maximum time to wait that the
send buffers are emptied. Socket option parameter is TSoTcpLingerOpt struct.
@publishedAll
@released
*/
const TUint KSoTcpLinger = 0x1022;


/**
Parameter struct for KSoTcpLinger socket option. The following combinations are possible:

@li <b>iOnOff == 0</b>: Close() call returns immediately, but TCP still
		tries to transmit the data remaining in its send buffers.
@li <b>iOnOff == 1, iLinger == 0</b>: Close() returns immediately, and the TCP sender discards
		all data in its send buffers. TCP RST is sent to the other end. Note: the TCP sender
		avoids the TIME_WAIT state.
@li <b>iOnOff == 1, iLinger > 0</b>: Close() call blocks until the data in TCP send buffers
		is succesfully transmitted and the connection is graciously terminated.
		If the sender cannot transmit all data before the linger time expires,
		the Close() call wakes up, but the stack continues towards terminating the
		connection as usual.
		
A similar structure is used in BSD Unix sockets, hence porting Unix apps using linger option
should be straight forward.
@publishedAll
@released
*/
class TSoTcpLingerOpt
	{
public:
	TInt	iOnOff;		//< 0=Linger off; nonzero=Linger on.
	TInt	iLinger;	//< Linger time in seconds.
	};
	

// -- Socket option level: KSolInetIfQuery --

// The options below are on KSolInetIfQuery level, although they use different option format
// than the rest of the options.
// There are no incoming parameters for these queries

/**
Return array of TInetInterfaceInfo objects as the response of GetOptions call.
@publishedAll
@released
*/
const TUint KSoInetInterfaceInfo = 0x1001;

/**
Return array of TInetAddressInfo objects as the response of GetOptions call.
@publishedAll
@released
*/
const TUint KSoInetAddressInfo = 0x1002;

/**
Return array of TInetRouteInfo objects as the response of GetOptions call.
@publishedAll
@released
*/
const TUint KSoInetRouteInfo = 0x1003;

/**
Information of an address attached to interface.
Used by the event service (EClassAddress events) and KSoInetAddressInfo socket option.
@publishedAll
@released
*/
class TInetAddressInfo
	{
public:
	TUint32	    iInterface;	    //< Network interface index to which this address is bound.
	TIp6Addr    iAddress;	    //< Prefix or Id part of the address described.
	TUint8	    iPrefixLen;	    //< Length of the prefix part in bits.
	TUint32	    iScopeId;	    //< ScopeId of this address.
	TUint32	    iPrefLifetime;  //< Remaining Preferred lifetime of this address.
	TUint32	    iValidLifetime; //< Remaining Valid lifetime of this address.
	TUint	    iFlags;	    //< Is address entry for prefix or id, etc. See enum TFlags
	TUint	    iState;	    //< Address state, copied from TIp6AddressInfo, see enum TAddressState
	TUint	    iType;	    //< Address type copied from TIp6AddressInfo, see enum TAddressType
	TUint	    iGenerations;   //< Number of times the address Id is generated (or randomly re-generated)
	TUint	    iNS;	    //< Number of neighbour solicitations sent for DAD.

	// Values used in iFlags field.
	enum TFlags
		{
	    EF_Prefix = 0x1,	//< This address entry specifies prefix
	    EF_Id = 0x2,    	//< This address entry specifies id part of the address
	    EF_Deprecated = 0x4 //< Address is deprecated
		};

	// Values used in iState field. The field is directly copied from iface.cpp.
	enum TAddressState
		{
	    ENoAddress	= 0,	//< 0 0 - unassigned initial state (no address present)
	    EDuplicate	= 1,	//< 0 1 - address is duplicate
	    EAssigned	= 2,	//< 1 0 - address fully available
	    ETentative	= 3	//< 1 1 - address is tentative (DAD in progress)
		};

	// Values used in iType field. The field is directly copied from iface.cpp.
	enum TAddressType
		{
	    EProxy	= 2,	//< Do DAD, is not for me (forward)
	    EAnycast	= 1,	//< Don't do DAD, is for me address
	    ENormal	= 0	//< Do DAD, is for me
		};
	};


/**
Information of a network interface.
Used by event service (EClassInterface events) and KSoInetInterfaceInfo socket option.
@publishedAll
@released
*/
class TInetInterfaceInfo
	{
public:
	TUint32	    iIndex;
	TName	    iName;		//< Interface name
	TInt	    iState;		//< State
	TInt	    iSMtu;		//< Maximum transmit unit size
	TInt	    iRMtu;		//< Maximum receive unit size
	TInt	    iSpeedMetric;	//< Metric - bigger is better
	TUint	    iFeatures;		//< Feature flags
	TSockAddr   iHwAddr;		//< Hardware address

	// Possible interface states.
	// Can also have negative values when on error state.
	enum
		{
	    IfState_READY   = 0,  //< Ready to receive data from protocol
	    IfState_PENDING = 1,  //< Not ready for data yet
	    IfState_HOLD    = 2
		};
	};


/**
Information of a route entry in IP stack.
Used by event service (EClassRoute events) and KSoInetRouteInfo socket option.
@publishedAll
@released
*/
class TInetRouteInfo
	{
public:
	TUint32	    iIndex;	//< Route index
	TUint	    iType;	//< Type of route (kernel generated have 0 at the present)
	TUint	    iState;	//< State of route (copied from iState in CIp6Route)
	TInt	    iMetric;	//< Smaller is better (less hops and/or faster link)
	TUint32	    iInterface; //< Network interface index of the route
	TIp6Addr    iGateway;	//< IP address of gateway (might be the interface)
	TIp6Addr    iDstAddr;	//< Destination network or host
	TUint8	    iPrefixLen;	//< Length of the route prefix in bits
	TUint32	    iScopeId;	//< Scope Id of this route
	TUint32	    iLifetime;  //< Route lifetime in seconds

	enum
		{ 
		EDeprecated = 0x80000000	//< This bit is set in iType if the route is deprecated
		};

	// Values used in iState field
	enum TState
	  	{
	    EIncomplete = 0,
	    ELoopback = 1,
	    EOnlink = 2,
	    EGateway = 3,
	    EAnycast = 5,
	    ERedirect = 7,
	    EReachable = 8,
	    EStale = 16
	 	 };
	};
	

/**
Information on a neighbour cache entry in the IP stack.
Used by event service (EClassNeighbour events).
@publishedAll
@released
*/
class TInetNeighbourInfo
	{
public:
	TUint32	    iIndex;		//< Route index.
	TIp6Addr    iDstAddr;	//< Neighbour's IP address.
	TUint	    iState;		//< State of neigbour entry. @see TInetRouteInfo::TState.
	TInt	    iMetric;	//< Smaller is better (less hops and/or faster link).
	TUint32	    iInterface; //< Network interface index of the route.
	TUint32	    iScopeId;	//< Scope Id of this neighbour.
	TUint32	    iLifetime;  //< Cache entry lifetime in seconds.

	// Hardware address (e.g. Ethernet MAC).
	TBuf8<KMaxSockAddrSize>	iHwAddr;
	};


/**
For building an array on top of TDes8. This is like casting a TDes8 data pointer to an array, but
it provides protection against array boundary violations, and some small helpful utilities.
The motivation of this class is to help in handling the information accessed by MNetworkInfo
interface.
@publishedAll
@released
*/
template<class T> class TOverlayArray
	{
public:
	inline TOverlayArray(TDes8& aDes) : iDes(aDes)
	{ }

	/**
	* Returns pointer to the given element location in the array. If the index exceeds the
	* maximum length of the array, NULL is returned.
	*/
	inline T* IndexPtr(TInt aIndex)
	{ if (aIndex >= MaxLength()) return NULL; else return &((T*)iDes.Ptr())[aIndex]; }

	/**
	* Return the given element of the array. No boundary checking.
	*/
	inline T& operator[](TInt aIndex)
	{ return ((T*)iDes.Ptr())[aIndex]; }

	/**
	* Returns the maximum length of the array in the number of array elements.
	*/
	inline TInt MaxLength()
	{ return (iDes.MaxLength() / sizeof(T)); }

	/**
	* Returns the current length of the array in the number of array elements.
	* The length information is based
	* on the current underlaying descriptor length, which may not always be the desired result.
	*/
	inline TInt Length()
	{ return (iDes.Length() / sizeof(T)); }

	/**
	* Set the length of the underlaying descriptor. The parameter is given in the number of array
	* elements, which is then multiplied by a length of one element to determine the needed
	* descriptor length.
	*/
	inline void SetLength(TInt aLength)
	{ iDes.SetLength(aLength * sizeof(T)); }

private:
	TDes8&	  iDes;
	};


/**
Information of a multicast group joined by the IP stack. This class is not currently
used by socket options, only EClassMulticast events.
@publishedAll
@released
*/
class TInetMulticastInfo
	{
public:
	TIp6Addr	iMulticastGroup;	//< IP address of the multicast group.
	TUint32		iInterface;		//< Interface index of the group.
	TUint32		iLifetime;		//< Lifetime of the group in seconds.
	};


// -- Socket option level: KSolInetIfCtrl --

/**
Control the use of link-local addresses per interface.
Argument: TSoInetIpv4LinkLocalInfo  (SetOpt only).
@publishedAll
@released

@capability ECapabilityNetworkControl Configuring IPv4 Link-local addresses is restricted.
@ref RSocket::SetOpt()
*/
const TUint KSoIpv4LinkLocal = 0x1001;

/**
Used as a parameter in KSoIpv4LinkLocal.
@publishedAll
@released
*/
class TSoInetIpv4LinkLocalInfo
	{
public:
	TUint	iInterface;		//< Interface index to be affected.
	TUint	iFlag;			//< Indicates whether IPv4 link locals are used  (0='no'; 1='yes').
	};

#endif  // __INSOCK_IN6_ROUTE_H__
