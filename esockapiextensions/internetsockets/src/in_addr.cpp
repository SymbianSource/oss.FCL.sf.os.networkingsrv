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
// in_addr.cpp - IPv6/IPv4 socket library
//

#include <in_sock.h>
#include "in6_opt.h"

//
// DLL entry point
//

const TInt KIPv4AddressPartCount = 4;


//
//
//	*************************************
//	Primitive methods on raw IPv6 Address
//	*************************************
//	(some could be inlined later)
//

EXPORT_C TBool TIp6Addr::IsUnicast() const
	/**
	* Tests if the address is IPv6 unicast.
	* @return
	*	ETrue, if the address is unicast (not IPv6 unspecified or multicast);
	*	EFalse, otherwise.
	* @since 7.0
	*/
	{
	return !(IsMulticast() || IsUnspecified());
	}

EXPORT_C TBool TIp6Addr::IsMulticast() const
	/**
	* Tests if the IPv6 address is multicast.
	* @return
	*	ETrue, if the IP address value is IPv6 multicast (<tt>ff00::/8</tt>);
	*	EFalse, otherwise.
	* @since 7.0
	*/
	{
	return u.iAddr8[0] == 0xFF;
	}

EXPORT_C TBool TIp6Addr::IsLoopback() const
	/**
	* Tests if the address is IPv6 loopback. 
	* @return
	*	ETrue, if the address is loopback  (<tt>::1</tt>);
	*	EFalse, otherwise.
	* @since 7.0
	*/
	{
	const union {TUint8 a[4]; TUint32 b;} one = { {0, 0, 0, 1} };

	return
		u.iAddr32[0] == 0 &&
		u.iAddr32[1] == 0 &&
		u.iAddr32[2] == 0 &&
		u.iAddr32[3] == one.b;
	}

EXPORT_C TBool TIp6Addr::IsUnspecified() const
	/**
	* Tests if the address is IPv6 unspecified.
	* @return
	*	ETrue, if the IP address value is zero (= <tt>::</tt>);
	*	EFalse, otherwise.
	* @since 7.0
	*/
	{
	return
		u.iAddr32[0] == 0 &&
		u.iAddr32[1] == 0 &&
		u.iAddr32[2] == 0 &&
		u.iAddr32[3] == 0;
	}

EXPORT_C TBool TIp6Addr::IsLinkLocal() const
	/**
	* Tests if the address is an IPv6 link-local address.
	* @return
	*	ETrue, if this address is an IPv6 link-local address (<tt>fe80::/10</tt>);
	*	EFalse, otherwise.
	* @since 7.0
	*/
	{
	return u.iAddr8[0] == 0xFE && (u.iAddr8[1] & 0xC0) == 0x80;
	}

EXPORT_C TBool TIp6Addr::IsSiteLocal() const
	/**
	* Tests if this address is an IPv6 site-local address (fec0::/10).
	* @return
	*	ETrue, if this is an IPv6 site-local address (<tt>fec0::/10</tt>);
	*	EFalse, otherwise.
	* @since 7.0
	*/
	{
	return u.iAddr8[0] == 0xFE && (u.iAddr8[1] & 0xC0) == 0xC0;
	}

EXPORT_C TBool TIp6Addr::IsV4Compat() const
	/**
	* Tests if this address is an IPv4-compatible address.
	*
	* @note
	*	returns EFalse for <tt>::0.0.0.0</tt> (= <tt>::</tt>) and
	*	<tt>::0.0.0.1</tt> (= <tt>::1</tt>).
	*
	* @return
	*	ETrue, if this is a IPv4-compatible address (<tt>::x.x.x.x</tt>);
	*	EFalse, otherwise.
	* @deprecated
	* @since 7.0
	*/
	{
	// Note: This must return false for true IPv6 "::" and "::1".
	const union {TUint8 a[4]; TUint32 b;} one = { {0, 0, 0, 1} };
	return
		u.iAddr32[0] == 0 &&
		u.iAddr32[1] == 0 &&
		u.iAddr32[2] == 0 &&
                u.iAddr32[3] != 0 &&
                u.iAddr32[3] != one.b;
	}

EXPORT_C TBool TIp6Addr::IsV4Mapped() const
	/**
	* Tests if this address is an IPv4-mapped address.
	* @return
	*	ETrue, if this address is an IPv4-mapped address (<tt>::ffff:x.x.x.x</tt>);
	*	EFalse, otherwise.
	* @since 7.0
	*/
	{
	const union {TUint8 a[4]; TUint32 b;} v4Prefix = { {0, 0, 0xff, 0xff} };
	return
		u.iAddr32[0] == 0 &&
		u.iAddr32[1] == 0 &&
		u.iAddr32[2] == v4Prefix.b;
	}

EXPORT_C TBool TIp6Addr::IsEqual(const TIp6Addr &aAddr) const
	/**
	* Tests if two addresses are equal.
	*
	* @param aAddr
	*	Address to compare with
	* @return
	*	ETrue, if the addresses are equal;
	*	EFalse, otherwise.
	* @since 7.0
	*/
	{
	return
		u.iAddr32[3] == aAddr.u.iAddr32[3] &&
		u.iAddr32[2] == aAddr.u.iAddr32[2] &&
		u.iAddr32[1] == aAddr.u.iAddr32[1] &&
		u.iAddr32[0] == aAddr.u.iAddr32[0];
	}

EXPORT_C TInt TIp6Addr::Match(const TIp6Addr &aAddr) const
	/**
	* Compares two raw IPv6 addresses from left to right and returns
	* the number of bits that match.
	*
	* @param aAddr
	*	Address to compare with
	* @return
	*	The number of bits that match (the common prefix length).
	* @since 7.0
	*/
	{
	TInt i = 0;

	while (u.iAddr32[i] == aAddr.u.iAddr32[i])
		if (++i == 4)
			return 128;
	i <<= 1;
	if (u.iAddr16[i] == aAddr.u.iAddr16[i])
		i++;

	i <<= 1;
	if (u.iAddr8[i] == aAddr.u.iAddr8[i])
		i++;

	TUint8 diff = (TUint8)(u.iAddr8[i] ^ aAddr.u.iAddr8[i]);
	for (i <<= 3; !(diff & 0x80); diff <<= 1)
		i++;

	return i;
    }

EXPORT_C TInt TIp6Addr::Scope() const
	/**
	* Return the scope level of the addres.
	*
	* All IP addresses (IPv4 and IPv6) have a scope level as defined by
	* this function. The scope level is a positive integer in range [1..16].
	*
	* This function may also return 0, which is an invalid level. This can
	* happen only, because IPv6 multicast addresses specify their scope
	* level explicitly.
	*
	* The scope level determines the interpretation of the scope identifier
	* in TInetAddr (see TInetAddr::Scope(), TInetAddr::SetScope()).
	*
	* Each network interface has a vector of 16 scope identifiers. The scope
	* identifier in the destination address selects a subset of the possible
	* interfaces: only the interfaces which have the correct matching scope
	* identifier in the vector entry matching the scope level, are valid
	* destinations for that address.
	*
	* The scope level of an IP address is computed as follows:
	* 
	* @li <tt>ffyz::/10</tt>
	* -- IPv6 multicast addressess, the scope level is extracted from the address (= z)
	* @li <tt>fe80::/10</tt>
	* -- IPv6 link local addresses, return KIp6AddrScopeLinkLocal
	* @li <tt>fec0::/10</tt>
	* -- IPv6 site local addresses, return KIp6AddrScopeSiteLocal
	* @li <tt>::1/128</tt>
	* -- IPv6 loopback address, return KIp6AddrScopeNodeLocal
	* @li <tt>::/128</tt>
	* -- IPv6 unspecified address, return KIp6AddrScopeNodeLocal
	* @li <tt> ::ffff:0.0.0.0</tt>
	* -- IPv4 unspecified address, return KIp6AddrScopeNodeLocal
	* @li <tt>::ffff:169.254.x.x/112</tt>
	* -- IPv4 link local range, return KIp6AddrScopeLinkLocal
	* @li <tt>::ffff:224.0.0.x/120</tt>
	* -- IPv4 link local multicast range, return KIp6AddrScopeLinkLocal
	* @li <tt>::ffff:127.x.x.x/104</tt>
	* -- IPv4 loopback addresses, return KIp6AddrScopeNodeLocal
	* @li <tt>::ffff:0.0.0.0/96</tt>
	* -- all other IPv4 addresses, return KIp6AddrScopeNetwork
	* @li None of the above
	* -- assume global IPv6 addresses, return KIp6AddrScopeGlobal
	*
	* @return
	*	The scope value of the address (e.g. #KIp6AddrScopeNodeLocal,
	*	#KIp6AddrScopeLinkLocal, #KIp6AddrScopeSiteLocal,
	*	#KIp6AddrScopeOrganization, #KIp6AddrScopeGlobal or
	*	#KIp6AddrScopeNetwork).
	* @since 7.0
	*/
	{
	if (u.iAddr8[0] == 0xFF)
		return u.iAddr8[1] & 0x0F;	// Multicast scope is explicit in the address.
	//
	// Other addresses must be mapped "by hand"
	//
	if (u.iAddr8[0] == 0x00)
		{
		const union {TUint8 a[2]; TUint16 b;} ipv4_linklocal = { {169, 254} };
		const union {TUint8 a[4]; TUint32 b;} ipv4_linklocal_mc = { {224, 0, 0, 0} };
		const union {TUint8 a[4]; TUint32 b;} ipv4_linklocal_mc_mask = { {255, 255, 255, 0} };

		if (IsLoopback())
			return KIp6AddrScopeNodeLocal;// Loopback ==> Node Local Scope
		if (IsUnspecified())
			return KIp6AddrScopeNodeLocal;// Return Node Local for unspecified address
		if (IsV4Mapped())
			{
			if (u.iAddr16[6] == ipv4_linklocal.b)
				return KIp6AddrScopeLinkLocal;// IPv4 Link local (169.254/16)
			else if ((u.iAddr32[3] & ipv4_linklocal_mc_mask.b) == ipv4_linklocal_mc.b)
				return KIp6AddrScopeLinkLocal;
			else if (u.iAddr8[12] == 127 || u.iAddr32[3] == 0)
				return KIp6AddrScopeNodeLocal;// IPv4 Loopback (127.0/8) and 0.0.0.0
			return KIp6AddrScopeNetwork;// all other IPv4 addresses
			}
		}
	else if (u.iAddr8[0] == 0xFE)
		{
		if ((u.iAddr8[1] & 0xC0) == 0x80)
			return KIp6AddrScopeLinkLocal;// Link Local ==> Link Local Scope
		else if ((u.iAddr8[1] & 0xC0) == 0xC0)
			return KIp6AddrScopeSiteLocal;// Site Local ==> Site Local Scope
		// Fall through to global scope
		}
	//
	// All rest and unassigned ranges are treated as Global Scope
	//
	return KIp6AddrScopeGlobal;
	}

//
//
//	********************************
//	Actual TInetAddr implementation
//	********************************
//
EXPORT_C void TInetAddr::Init(TUint aFamily)
	/**
	* Initialises the object properly according to the address family
	* passed in.
	*
	* @param aFamily The address family. Valid values are
	* @li #KAfInet,
	*	plain IPv4 address format
	* @li #KAfInet6,
	*	combined IPv4 and IPv6 format, including fields for the scope id and
	*	flow label.
	* @li anything else,
	*	initialized to KAFUnspec, with empty content.
	*
	* @post
	*	In all above cases, IsUnspecified() returns ETrue after
	*	this function.
	* @since 7.0
	*/
	{
	if (aFamily == KAfInet)
		SetUserLen(sizeof(SInetAddr));
	else if (aFamily == KAfInet6)
		SetUserLen(sizeof(SInet6Addr));
	else
		{
		SetUserLen(0);
		aFamily = KAFUnspec;
		}
	SetFamily(aFamily);
	// Always clear the IPv6 address variant, it is also
	// sufficient for IPv4 (even though a bit overkill).
	// For IPv6 this means that Scope and Flow Label
	// are zeroed.
	Mem::FillZ(UserPtr(), sizeof(SInet6Addr));
	}

EXPORT_C TInetAddr::TInetAddr()
	: TSockAddr(KAFUnspec)
	/** 
	* Constructs a basic TInetAddr object.
	*
	* The port is initialised to 0, and the IP address is unspecified.
	* The resulting address family is KAFUnspec.
	*/
	{
	SetPort(KInetPortNone);
	}

EXPORT_C TInetAddr::TInetAddr(const TSockAddr &aAddr) : TSockAddr(aAddr)
	/** 
	* Constructs a TInetAddr from a TSockAddr.
	*
	* The port and IP address values are copied from aAddr.
	* The resulting address family is same as that of aAddr.
	*
	* The function does a raw copy operation of the TSockAddr.
	* No checks are made at this point.
	* 
	* @param aAddr  TSockAddr to be used as initial content. 
	*/
	{
	}

EXPORT_C TInetAddr::TInetAddr(TUint aPort)
	: TSockAddr(KAFUnspec)
	/**
	* Constructs a TInetAddr and initialises the port to the specified value, 
	* and the IP address is unspecified.
	*
	* The resulting address family is KAFUnspec.
	*
	* @param aPort Value to which to set the port. 
	*/
	{
	SetPort(aPort);
	}

EXPORT_C TInetAddr::TInetAddr(TUint32 aAddr, TUint aPort)
	/**
	* Constructs a TInetAddr and initialise the specified port value
	* to aPort and the IP address to aAddr.
	*
	* The resulting address family is #KAfInet
	*
	* @param aAddr Value to which to set the IP address.
	* @param aPort Value to which to set the port.
	*/
	{
	Init(KAfInet);
	SetPort(aPort);
	Addr4Ptr()->iAddr = aAddr;
	}

EXPORT_C TInetAddr::TInetAddr(const TIp6Addr& aAddr, TUint aPort)
	/**
	* Constructs a TInetAddr and initialises the port to the
	* specified value, and the IPv6 address to aAddr.
	*
	* The resulting address family is #KAfInet6.
	*
	* @param aAddr Value to which to set the IPv6 address.  
	* @param aPort Value to which to set the port.
	* @since 7.0
	*/
	{
	Init(KAfInet6);
	AddrPtr()->iAddr = aAddr;
	SetPort(aPort);
	}


EXPORT_C void TInetAddr::SetAddress(TUint32 aAddr)
	/**
	* Sets the IPv4 address value.
	*
	* @note
	*	An IP address in dotted-decimal form can be converted into
	*	a TUint32 by the INET_ADDR macro.
	*
	* The resulting address family is #KAfInet.
	*
	* @param aAddr Value to which to set the IPv4 address.
	*/
	{
	Init(KAfInet);
	Addr4Ptr()->iAddr = aAddr;
	}

EXPORT_C void TInetAddr::SetAddress(const TIp6Addr &aAddr)
	/**
	* Sets the IPv6 address value. 
	*
	* The resulting address family is #KAfInet6.
	* The scope id and flow label are zeroed.
	*
	* @param aAddr Value to which to set the IPv6 address. 
	* @since 7.0
	*/	
	{
	Init(KAfInet6);
	AddrPtr()->iAddr = aAddr;
	}

EXPORT_C void TInetAddr::SetV4CompatAddress(TUint32 aAddr)
	/**
	* Creates an IPv4-compatible IPv6 address.
	*
	* The <em>IPv4-compatible format</em> is <tt>::x.x.x.x</tt>. 
	*
	* The resulting address family is #KAfInet6.
	* The scope id and flow label are zeroed.
	*
	* @param aAddr IPv4 address from which the IPv6 address is generated.
	*
	* @deprecated This address format is deprecated and should not be used.
	* @since 7.0
	*/
	{
	Init(KAfInet6);

	struct SInet6Addr *const a = AddrPtr();
	a->iAddr.u.iAddr32[0] = 0;
	a->iAddr.u.iAddr32[1] = 0;
	a->iAddr.u.iAddr32[2] = 0;
	a->iAddr.u.iAddr8[12] = (TUint8)(aAddr >> 24);
	a->iAddr.u.iAddr8[13] = (TUint8)(aAddr >> 16);
	a->iAddr.u.iAddr8[14] = (TUint8)(aAddr >>  8);
	a->iAddr.u.iAddr8[15] = (TUint8)aAddr;
	}

EXPORT_C void TInetAddr::SetV4MappedAddress(TUint32 aAddr)
	/**
	* Creates an IPv4-mapped IPv6 address.
	*
	* The <em>IPv4-mapped format</em> is <tt>::ffff:x.x.x.x</tt>. Internally, all
	* IPv4 addresses are stored in this format. 
	*
	* The resulting address family is #KAfInet6.
	* The scope id and flow label are zeroed.
	*
	* @param aAddr IPv4 address from which the IPv6 address is generated.
	* @since 7.0
	*/
	{
	Init(KAfInet6);

	const union {TUint8 a[4]; TUint32 b;} cpatPrefix = { {0, 0, 0xff, 0xff} };

	struct SInet6Addr *const a = AddrPtr();
	a->iAddr.u.iAddr32[0] = 0;
	a->iAddr.u.iAddr32[1] = 0;
	a->iAddr.u.iAddr32[2] = cpatPrefix.b;
	a->iAddr.u.iAddr8[12] = (TUint8)(aAddr >> 24);
	a->iAddr.u.iAddr8[13] = (TUint8)(aAddr >> 16);
	a->iAddr.u.iAddr8[14] = (TUint8)(aAddr >>  8);
	a->iAddr.u.iAddr8[15] = (TUint8)aAddr;
	}

EXPORT_C void TInetAddr::ConvertToV4Compat()
	/**
	* Converts an IPv4 address to an IPv4-compatible IPv6 address. 
	*
	* The function assumes that the previous content is IPv4 address and
	* accesses this using TInetAddr::Address() function. If the previous
	* content was not IPv4 address (in any format), function acts as if
	* the old content was <tt>0.0.0.0</tt>.
	*
	* The <em>IPv4-compatible format</em> is <tt>::x.x.x.x</tt>.
	*
	* The resulting address family is #KAfInet6.
	* The scope id and flow label are zeroed.
	*
	* @deprecated This address format is deprecated and should not be used.
	* @since 7.0
	*/
	{
	SetV4CompatAddress(Address());
	}

EXPORT_C void TInetAddr::ConvertToV4Mapped()
	/**
	* Converts an IPv4 address to an IPv4-mapped IPv6 address.
	*
	* The function assumes that the previous content is IPv4 address and
	* accesses this using TInetAddr::Address() function. If the previous
	* content was not IPv4 address (in any format), function acts as if
	* the old content was "0.0.0.0".
	*
	* The <em>IPv4-mapped format</em> is <tt>::ffff:x.x.x.x</tt>. Internally, all
	* IPv4 addresses are stored in this format.
	*
	* The resulting address family is #KAfInet6.
	* The scope id and flow label are zeroed.
	* @since 7.0
	*/
	{
	SetV4MappedAddress(Address());
	}

EXPORT_C void TInetAddr::ConvertToV4()
	/**
	* Converts an IPv4-mapped or IPv4-comptatible address to IPv4.
	*
	* The function assumes that the previous content is IPv4 address
	* and accesses this using TInetAddr::Address()
	* function. If the previous content was not IPv4 address (in any format),
	* function acts as if the old content was <tt>0.0.0.0</tt>.
	*
	* The address family is set to #KAfInet.
	* @since 7.0
	*/
	{
	SetAddress(Address());
	}

EXPORT_C const TIp6Addr &TInetAddr::Ip6Address() const
	/**
	* Gets the IPv6 address
	*
	* Only for #KAfInet6, undefined for #KAfInet.
	*
	* This function returns a reference to a location where IPv6
	* address would be stored.
	* No check on family is made, and
	* using this function for any other address family except KAfInet6
	* does not cause an error, but returned reference does not point
	* to valid IPv6 address (unless content is converted to KAfInet6).
	*
	* @return The IPv6 address
	* @since 7.0
	*/
	{
	return AddrPtr()->iAddr;
	}

EXPORT_C void TInetAddr::SetFlowLabel(TInt aLabel)
	/**
	* Sets the Flow Label.
	*
	* Only for #KAfInet6, undefined (ignored) for #KAfInet.
	*
	* @param aLabel The flow label (only low 20 bits used)
	* @since 7.0
	*/
	{
	// Family not checked, but if it is not KAfInet6, the stored
	// value has no significance.
	AddrPtr()->iFlow = aLabel & 0xFFFFF;
	}

EXPORT_C TInt TInetAddr::FlowLabel() const
	/**
	* Gets the Flow Label.
	*
	* Only for #KAfInet6, return zero for #KAfInet.
	*
	* @return The flow label
	* @since 7.0
	*/
	{
	return Family() == KAfInet6 ? AddrPtr()->iFlow : 0;
	}

EXPORT_C void TInetAddr::SetScope(TUint32 aScope)
	/**
	* Sets the scope id value
	*
	* Only for #KAfInet6, undefined (ignored) for #KAfInet.
	*
	* When an IPv4 address is stored in IPv4-mapped format (KAfInet6),
	* then non-zero scopes are also possible for IPv4 (and needed for
	* proper handling of overlapping private address ranges or for
	* IPv4 link local addresses).
	*
	* @param aScope The scope id of the address.
	* @since 7.0
	*/
	{
	// Family not checked, but if it is not KAfInet6, the stored
	// value has no significance.
	AddrPtr()->iScope = aScope;
	}

EXPORT_C TUint32 TInetAddr::Scope() const
	/**
	* Gets the scope id value
	*
	* Only for #KAfInet6, return zero for #KAfInet.
	*
	* When an IPv4 address is stored in IPv4-mapped format (KAfInet6),
	* then non-zero scopes are also possible for IPv4 (and needed for
	* proper handling of overlapping private address ranges or for
	* IPv4 link local addresses).
	*
	* @return The scope id of the address.
	* @since 7.0
	*/
	{
	return Family() == KAfInet6 ? AddrPtr()->iScope : 0;
	}

EXPORT_C TBool TInetAddr::IsUnicast() const
	/**
	* Tests if address is unicast address.
	*
	* This is just a "shorthand" notation for a test
	* that none of the following is true
	*
	* @li TInetAddr::IsMulticast()
	* @li TInetAddr::IsBroadcast()
	* @li TInetAddr::IsUnspecified()
	*
	* For exact semantics of IsUnicast, see the descriptions
	* of the above functions.
	*
	* Caution: the test is based purely on the address, and it should be
	* kept in mind that this test has no way of detecting IPv4 net
	* broadcast addresses (because the netmask is not known).
	*
	* @return
	*	ETrue if the address is unicast:
	*	EFalse, otherwise
	* @since 7.0
	*/
	{
	return !(IsMulticast() || IsBroadcast() || IsUnspecified());
	}

EXPORT_C TBool TInetAddr::IsMulticast() const
	/**
	* Tests if the IP address is a multicast address.
	*
	* For IPv4 addresses this returns true, if address
	* is Class D (multicast). IPv4 address can be in
	* #KAfInet or #KAfInet6 (<em>IPv4-mapped</em> or
	* <em>IPv4-compatible</em>) formats.
	*
	* For IPv6 addresses this returns true, if address
	* is IPv6 multicast address (start with <tt>0xff</tt> byte).
	*
	* @return
	*	ETrue if the IP address value is multicast;
	*	EFalse, otherwise
	* @since 7.0
	*/
	{
	return (Family() == KAfInet6 && AddrPtr()->iAddr.IsMulticast()) ||
		(Address() & KInetAddrIdMaskD) == KInetAddrIdValD;
	}

EXPORT_C TBool TInetAddr::IsUnspecified() const
	/**
	* Tests if the IP address is unspecified.
	*
	* The address is unspecified if the
	* @li family is KAFUnspec
	* @li family is #KAfInet6 and address is <tt>::</tt>
	* @li family is #KAfInet6 and address is IPv4-mapped <tt>0.0.0.0</tt>
	* @li family is #KAfInet and address is <tt>0.0.0.0</tt>
	*
	* @return
	*	ETrue if the IP address is unspecified;
	*	EFalse, otherwise
	* @since 7.0
	*/
	{
	return Family() == KAfInet6 ?
		AddrPtr()->iAddr.IsUnspecified() || (AddrPtr()->iAddr.IsV4Mapped() && Address() == KInetAddrNone) :
		Address() == KInetAddrNone;
	}

EXPORT_C TBool TInetAddr::IsLoopback() const
	/**
	* Tests if the IP address is a loopback address.
	*
	* For IPv4 addresses this returns true, if address
	* belongs to the loopback net (<tt>127.x.x.x</tt>). IPv4 address
	* can be in KAfInet or in IPv4-mapped/compatible
	* formats.
	*
	* For IPv6 address this returns true for IPv6 loopback
	* address (= <tt>::1</tt>).
	*
	* @return
	*	ETrue, if the address is loopback;
	*	EFalse, otherwise
	* @since 7.0
	*/
	{
	return (Family() == KAfInet6 && AddrPtr()->iAddr.IsLoopback()) ||
		(Address() & INET_ADDR(255,0,0,0)) == INET_ADDR(127,0,0,0);
	}

EXPORT_C TBool TInetAddr::IsLinkLocal() const
	/**
	* Tests if IP address is link-local address
	*
	* For IPv4 this returns true, if the address belongs to the
	* link-local net (<tt>169.254.x.x</tt>).
	*
	* For IPv6 this returns true, if the address is IPv6 link-local
	* address (<tt>fe80:..</tt>).
	*
	* @note
	*	Does not return true for multicast addresses
	*	which are in the link-local scope.
	*
	* @return
	*	ETrue, if this address is a link-local address;
	*	EFalse, otherwise
	* @since 7.0
	*/
	{
	return (Family() == KAfInet6 && AddrPtr()->iAddr.IsLinkLocal()) ||
		(Address() & KInetAddrLinkLocalNetMask) == KInetAddrLinkLocalNet;
	}

EXPORT_C TBool TInetAddr::IsSiteLocal() const
	/**
	* Tests if IP address is site-local address.
	*
	* Always false for IPv4 addressess.
	*
	* For IPv6 this returns true, if the address is IPv6 site-local
	* address (<tt>fec0:...</tt>)
	* 
	* @note
	*	Does not return true for multicast addresses
	*	which are in the site-local scope.
	*
	* @return
	*	ETrue, if this is a site-local address;
	*	EFalse, otherwise
	* @since 7.0
	*/
	{
	return Family() == KAfInet6 && AddrPtr()->iAddr.IsSiteLocal();
	}

EXPORT_C TBool TInetAddr::IsV4Compat() const
	/**
	* Tests if this address is an IPv4-compatible address.
	*
	* This is always false, if address is in #KAfInet format.
	* 
	* @return
	*	ETrue, if this is a IPv4 compatible address;
	*	EFalse, otherwise.
	*
	* @deprecated IPv4-compatible addresses should not be used.
	* @since 7.0
	*/
	{
	return Family() == KAfInet6 && AddrPtr()->iAddr.IsV4Compat();
	}

EXPORT_C TBool TInetAddr::IsV4Mapped() const
	/**
	* Tests if this address is an IPv4-mapped address.
	*
	* This is always false, if address is in #KAfInet format.
	* 
	* @return
	*	ETrue, if this address is an IPv4-mapped address;
	*	EFalse, otherwise.
	* @since 7.0
	*/
	{
	return Family() == KAfInet6 && AddrPtr()->iAddr.IsV4Mapped();
	}

EXPORT_C TUint32 TInetAddr::Address() const
	/**
	* Gets the IPv4 address value.
	*
	* If the stored address is not an IPv4 address, the returned
	* value is ZERO.
	*
	* This works also for IPv4 addresses which
	* are stored in <em>IPv4-mapped</em> or <em>IPv4-compatible</em>
	* format. (since 7.0).
	* 
	* @return The IP address value.
	*/
	{
	if (Family() == KAfInet)
		return Addr4Ptr()->iAddr;
	if (Family() == KAfInet6 && (IsV4Mapped() || IsV4Compat()))
		{
	    const struct SInet6Addr *a = AddrPtr();
        return
			(a->iAddr.u.iAddr8[12] << 24) |
			(a->iAddr.u.iAddr8[13] << 16) |
			(a->iAddr.u.iAddr8[14] <<  8) |
			a->iAddr.u.iAddr8[15];
		}
	// Note: This is returned for KAFUnspec and any unknown family
    return KInetAddrNone;
	}


EXPORT_C TBool TInetAddr::CmpAddr(const TInetAddr &aAddr) const
	/**
	* Compares IP address and port values with 
	* those in another TInetAddr.
	* 
	* @param aAddr
	*	TInetAddr with which to compare
	* @return
	*	ETrue if IP address and port values are the same;
	*	EFalse, otherwise.
	*/
	{
	return Match(aAddr) && CmpPort(aAddr);
	}

EXPORT_C TBool TInetAddr::Match(const TInetAddr &aHost) const
	/**
	* Tests the IP address value with that in another TInetAddr.
	*
	* @note
	*	The function matches IPv4 addresses even if they are
	*	stored in different formats (#KAfInet or #KAfInet6 using
	*	<em>IPv4-mapped</em> format) (since Symbian OS 7.0s).
	*
	* @param aHost TInetAddr with which to compare
	* @return
	*	ETrue if IP address value is the same as this,
	*	EFalse, otherwise.
	*/
	{
	const struct SInet6Addr *a = AddrPtr();
	const struct SInet6Addr *b = aHost.AddrPtr();

	if (Family() != aHost.Family())
		{
		//
		// Do some heavier testing, to return true for
		// equal IPv4 addresses, when one is in KAfInet
		// format and other presented as IPv4 mapped (KAfInet6)
		// format.
		if ((Family() == KAfInet && aHost.IsV4Mapped()) ||
			(IsV4Mapped() && aHost.Family() == KAfInet))
			return Address() == aHost.Address();
		else
			return EFalse;
		}
	if (Family() == KAfInet)
		return Addr4Ptr()->iAddr == aHost.Addr4Ptr()->iAddr;
	else if (Family() == KAfInet6)
		return
			a->iAddr.u.iAddr32[3] == b->iAddr.u.iAddr32[3] &&
			a->iAddr.u.iAddr32[2] == b->iAddr.u.iAddr32[2] &&
			a->iAddr.u.iAddr32[1] == b->iAddr.u.iAddr32[1] &&
			a->iAddr.u.iAddr32[0] == b->iAddr.u.iAddr32[0];
	// Any other than KAfInet, KAfInet6 or KAFUnspec is always FALSE
	return Family() == KAFUnspec;
	}

EXPORT_C TBool TInetAddr::Match(const TInetAddr &aNet, const TInetAddr &aMask) const
	/**
	* Tests if another TInetAddr is in the same subnet.
	*
	* The function applies the subnet mask passed through aMask to the IP
	* address and to the IP address in aNet. If the resulting values are
	* the same, this indicates that the addresses are part of the same subnet,
	* and ETrue is returned.
	* 
	* @note
	*	The function matches IPv4 addresses even if they are
	*	stored in different formats (#KAfInet or #KAfInet6 using
	*	<em>IPv4-mapped</em> format). (since Symbian OS 7.0s).
	*
	* @param aNet
	*	TInetAddr with which to compare. 
	* @param aMask
	*	TInetAddr object with the IP address set to the relevant subnet mask. 
	* @return
	*	ETrue if both IP address values are the members of the same subnet;
	*	EFalse, otherwise.
	*/
	{
	const struct SInet6Addr *a = AddrPtr();
	const struct SInet6Addr *b = aNet.AddrPtr();
	const struct SInet6Addr *m = aMask.AddrPtr();

	if (Family() != aNet.Family() || Family() != aMask.Family())
		{
		// Mixed address families, only case that could return true
		// is some combination of KAfInet and IPv4 mapped formats.
		if (!(Family() == KAfInet || IsV4Mapped()))
			return EFalse;
		if (!(aNet.Family() == KAfInet || aNet.IsV4Mapped()))
			return EFalse;
		if (!(aMask.Family() == KAfInet || aMask.IsV4Mapped()))
			return EFalse;
		return !(aMask.Address() & (Address() ^ aNet.Address()));
		}
	if (Family() == KAfInet)
		return !(aMask.Addr4Ptr()->iAddr & (Addr4Ptr()->iAddr ^ aNet.Addr4Ptr()->iAddr));
	else if (Family() == KAfInet6)
		return
			!((m->iAddr.u.iAddr32[0] & (a->iAddr.u.iAddr32[0] ^ b->iAddr.u.iAddr32[0])) ||
			  (m->iAddr.u.iAddr32[1] & (a->iAddr.u.iAddr32[1] ^ b->iAddr.u.iAddr32[1])) ||
			  (m->iAddr.u.iAddr32[2] & (a->iAddr.u.iAddr32[2] ^ b->iAddr.u.iAddr32[2])) ||
			  (m->iAddr.u.iAddr32[3] & (a->iAddr.u.iAddr32[3] ^ b->iAddr.u.iAddr32[3])));
	// Any other than KAfInet, KAfInet6 or KAFUnspec is always FALSE
	return Family() == KAFUnspec;
	}

EXPORT_C TBool TInetAddr::Match(const TInetAddr &aNet, TInt aPrefixLen) const
	/**
	* Tests if the specified number of left-most bits on addresses are same.
	*
	* Return ETrue if leftmost aPrefixLen bits on addresses are same (both 
	* families must be the same).
	*
	* @param aNet
	*	TInetAddr with which to compare.
	* @param aPrefixLen
	*	Number of left-most bits to compare.
	* @return
	*	ETrue, if the bits match;
	*	EFalse, otherwise.
	* @since 7.0
	*/
	{
	if (aPrefixLen > 127)
		{
		// If the prefix length is 128 or larger, then this
		// is just simple address match..
		return Match(aNet);
		}
	else if (aPrefixLen <= 0)
		{
		// If prefix is 0 (or < 0 for sanity), then match
		// result is implicitly true.
		return ETrue;
		}

	const struct SInet6Addr *a = AddrPtr();
	const struct SInet6Addr *b = aNet.AddrPtr();

	TInt i;

	if (Family() != aNet.Family())
		{
		if (!(Family() == KAfInet || IsV4Mapped()))
			return EFalse;
		if (aNet.IsV4Mapped())
			aPrefixLen -= 96;
		else if (aNet.Family() != KAfInet)
			return EFalse;
		return !(~(~0UL >> (aPrefixLen & 31)) & (Address() ^ aNet.Address()));
		}

	if (Family() != KAfInet6)
		return !(~(~0UL >> (aPrefixLen & 31)) & (Addr4Ptr()->iAddr ^ aNet.Addr4Ptr()->iAddr));

	for (i = 0; i < (aPrefixLen >> 5); i++)
		if (a->iAddr.u.iAddr32[i] != b->iAddr.u.iAddr32[i])
			return EFalse;

	for (i <<= 2; i < (aPrefixLen >> 3); i++)
		if (a->iAddr.u.iAddr8[i] != b->iAddr.u.iAddr8[i])
			return EFalse;

	if (~(0xff >> (aPrefixLen & 7)) & (a->iAddr.u.iAddr8[i] ^ b->iAddr.u.iAddr8[i]))
		return EFalse;

	return ETrue;
	}

EXPORT_C void TInetAddr::PrefixMask(TInt aPrefixLen)
	/**
	* Creates an IPv6 address with the specified number of left-most bits set to 
	* 1, and the rest set to 0. 
	* 
	* The previous content does not matter, and is overwritten.
	* 
	*  @param aPrefixLen
	*	Number of left-most bits to set to 1.
	* @since 7.0
	*/
	{
  	Init(KAfInet6);

	__ASSERT_ALWAYS(aPrefixLen >= 0 && aPrefixLen <= 128, User::Panic(_L("INSOCK"), aPrefixLen));

	struct SInet6Addr *const a = AddrPtr();
	const TUint32 m = ~(~0UL >> (aPrefixLen & 31));
    TInt i;

	aPrefixLen >>= 5;
    for (i = 0; i < aPrefixLen; i++)
		a->iAddr.u.iAddr32[i] = ~0UL;
	i <<= 2;
	a->iAddr.u.iAddr8[i++] = (TUint8)(m >> 24);
	a->iAddr.u.iAddr8[i++] = (TUint8)(m >> 16);
	a->iAddr.u.iAddr8[i++] = (TUint8)(m >> 8);
    a->iAddr.u.iAddr8[i++] = (TUint8)m;
    i >>= 2;
    while (i < 4)
		a->iAddr.u.iAddr32[i++] = 0;
	}

EXPORT_C void TInetAddr::Prefix(const TInetAddr& aAddr, TInt aPrefixLen)
	/**
	* Creates an IPv6 address with the specified number of left-most bits
	* copied from another address, and remaining bits set to 0.
	*
	* The function does not check the family of the aAddr, and for anything
	* else but KAfInet6 the resulting prefix value is undefined.
	*
 	* The resulting address family is #KAfInet6.
	* The scope id and flow label are zeroed.
	*
	* @param aAddr
	*	The address to copy the prefix from.
	* @param aPrefixLen
	*	Number of left-most bits to set to copy from aAddr.
	* @since 7.0
	*/
	{
	Init(KAfInet6);

	__ASSERT_ALWAYS(aPrefixLen >= 0 && aPrefixLen <= 128, User::Panic(_L("INSOCK"), aPrefixLen));

	struct SInet6Addr *const a = AddrPtr();
	const struct SInet6Addr *const b = aAddr.AddrPtr();
	TUint32 m = ~(~0UL >> (aPrefixLen & 31));
	TInt i;
	
	aPrefixLen >>= 5;
	for (i = 0; i < aPrefixLen; i++)
		a->iAddr.u.iAddr32[i] = b->iAddr.u.iAddr32[i];

	i <<= 2;
	a->iAddr.u.iAddr8[i] = (TUint8)(b->iAddr.u.iAddr8[i] & (m >> 24)); i++;
	a->iAddr.u.iAddr8[i] = (TUint8)(b->iAddr.u.iAddr8[i] & (m >> 16)); i++;
	a->iAddr.u.iAddr8[i] = (TUint8)(b->iAddr.u.iAddr8[i] & (m >> 8)); i++;
	a->iAddr.u.iAddr8[i] = (TUint8)(b->iAddr.u.iAddr8[i] & m); i++;

	i >>= 2;
	while (i < 4)
		a->iAddr.u.iAddr32[i++] = 0;
	}


//

EXPORT_C void TInetAddr::Output(TDes &aBuf) const
	/**
	* Writes the IP address into a string.
	*
	* For an IPv4 address, the format is <tt>d.d.d.d</tt>, where "d" is an 8-bit
	* decimal number. (An example IPv4 address: <tt>127.0.0.1</tt>). 
	*
	* For an IPv6 address, the format is <tt>h:h:h:h:h:h:h:h</tt>,
	* where "h" is a 16-bit hexadecimal number.
	* (An example IPv6 address: <tt>2001:618:40C:20:2C0:4FFF:FE24:AA79</tt>). 
	*
	* If address family is not #KAfInet or #KAfInet6, then empty
	* string is returned (aBuf.Length() == 0).
	*
	* If the family is KAfInet6, the output buffer must be at least
	* 39 characters. If less, the buffer is filled with '*' characters.
	*
	* @retval aBuf
	*	contains a string representation of the IP address.
	* @since 7.0 (IPv6 support)
	*/
	{
	aBuf.SetLength(0);
	if (Family() != KAfInet && Family() != KAfInet6)
		return;

	TUint32 a = Address();
	if (Family() == KAfInet)
		{
	    aBuf.Format(_L("%d.%d.%d.%d"), a>>24, (a>>16)&0xff, (a>>8)&0xff, a&0xff);
		return;
	    }

	// Address family is IPv6. At worst case, the output buffer must be large
	// enough for full IPv6 address = 39 (8 * 4 + 7). To catch errors, require
	// the output to be at least this long, even though the current output might
	// fit. However, just fill the output with *'s in release environment, so
	// that existing IPv4 binaries do not crash, if they get IPv6 addresses.
	// Remember to update the magic 39, if you change the output format!

	//__ASSERT_DEBUG(aBuf.MaxLength() >= 39, User::Panic(_L("DEBUG"), aBuf.MaxLength()));
	if (aBuf.MaxLength() < 39)
		{
		aBuf.Fill('*', aBuf.MaxLength());
		return;
		}

	if (IsV4Compat())
		aBuf.Format(_L("::%d.%d.%d.%d"), a>>24, (a>>16)&0xff, (a>>8)&0xff, a&0xff);
	else if (IsV4Mapped())
		aBuf.Format(_L("%d.%d.%d.%d"), a>>24, (a>>16)&0xff, (a>>8)&0xff, a&0xff);
	else
		{
		TPtrC16 i6addr(AddrPtr()->iAddr.u.iAddr16, 8);
		TInt start = 0, count = 0, longest_count = 0, longest_start = 0;
		TInt i;
		for (i = 0; i < 8; i++)
			{
			if (i6addr[i])
				{
				if (count > longest_count)
					{
					longest_count = count;
					longest_start = start;
					}
				start = i + 1;
				count = 0;
				}
			else
				count++;
			}
		if (count > longest_count)
			{
			longest_count = count;
			longest_start = start;
			}
		i = 0;
		if (longest_count > 1)
			{
			if (longest_start == 0)
				aBuf.Append(':');
			else while (i < longest_start)
				{
				aBuf.AppendNum(TUint(BigEndian::Get16((TUint8 *)&i6addr[i++])), EHex);
				aBuf.Append(':');
				}
			aBuf.Append(':');
			i += longest_count;
			}
		if (i < 8)
			for (;;)
				{
				aBuf.AppendNum(TUint(BigEndian::Get16((TUint8 *)&i6addr[i++])), EHex);
				if (i == 8)
					break;
				aBuf.Append(':');
				}
		}
	}

EXPORT_C void TInetAddr::OutputWithScope(TDes &aBuf) const
	/**
	* Writes the IP address into a string and appends the scope id.
	*
	* The IP address is formatted with as with the standard
	* Output.
	*
	* If the scope id has non-zero value and if there is
	* room in the output buffer, the scope id is appended to the address
	* using the "%scope-id"-notation.
	*
	* @retval aBuf
	*	contains a string representation of the address.
	* @since after 7.0s
	*/
	{
	Output(aBuf);
	if (Scope() && aBuf.MaxLength() - aBuf.Length() > 11)
		aBuf.AppendFormat(_L("%%%d"), Scope());
	}

//
//	IPv6address = hexpart [ ":" IPv4address ]
//	IPv4address = 1*3DIGIT "." 1*3DIGIT "." 1*3DIGIT "." 1*3DIGIT
//	IPv6prefix  = hexpart "/" 1*2DIGIT
//
//	hexpart = hexseq | hexseq "::" [ hexseq ] | "::" [ hexseq ]
//	hexseq  = hex4 *( ":" hex4)
//	hex4    = 1*4HEXDIG

typedef enum
	{
	EInvalid,		// Invalid address characters detected
	EColon,			// Plain ':'
	EHexColon,		// Hex sequence ending with ':'
	EDecPeriod,		// Decimal sequence ending with '.'
	ELast			// Last sequence at end of string
	} TAddressToken;

class TInetAddrParser : public TLex
	{
	friend class TInetAddr;
	TInetAddrParser(const TDesC &anInput, TIp6Addr &aAddr);
	TAddressToken NextComponent();	// Extract the next component of the address
	TInt GetHexValue(TUint16 &aComp);
	TAddressToken HeadPart();
	TBool TailPart(TAddressToken);
	TBool Ipv4address();
	TInt iFill;		// Number of filled components in i6addr (0..8)
	TPtr16 i6addr;	// Collects IPv6 address components (max 8)
	};

#ifdef __VC32__
#pragma warning(disable : 4097) // typedef-name used as synonym for class-name
#endif
 
//
// TInetAddrParser::TInetAddrParser
//
TInetAddrParser::TInetAddrParser(const TDesC &anInput, TIp6Addr &aAddr) :
	TLex(anInput), iFill(0), i6addr(aAddr.u.iAddr16, 8, 8)
	{
	}

//
// TInetAddrParser::GetHexValue
//	Extract hexadecimal value terminated by ':' or Eos (any other
//	termination is considered as invalid syntax). The resulting
//	value must fit into unsigned 16 bits (checked by the Val()).
//
//	On input, the Mark on TLex must point to the beginning
//	of the number!
//
//	Returns
//		= KErrNone and value in 'comp', if no errors detected
//			The current point is left after the number or
//			terminator, if present.
//		!= KErrNone, otherwise. 'comp' and current point
//			may have indeterminate values.
//
TInt TInetAddrParser::GetHexValue(TUint16 &comp)
	{
	TInt err, ch;

	UnGetToMark();
	err = Val(comp, EHex);
	BigEndian::Put16((TUint8 *)&comp, comp);
        if (err == KErrNone && !Eos() && (ch = Get(), (ch != ':')))
		err = KErrArgument;
	return err;
	}

//
// TInetAddrParser::NextComponent
//	Mark the beginning and determine the type of the next component
//	by looking how the sequence is terminated.
//
//		1*<hexdigit> ":"	-> EHexColon
//		1*<hexdigit> "."	-> EDecPeriod
//		1*<hexdigit> <EOS>	-> ELast
//		":"					-> EColon
//		anything else		-> EInvalid
//
//	Note that this not check the lengths of the sequences,
//	nor any validity within context (like only decimal digits before
//	period. There is no point in checking such issues here, as they
//	must be noted later in any case (in TLex Val() function in part)
//
TAddressToken TInetAddrParser::NextComponent()
	{
	TAddressToken type = EInvalid;
	Mark();
	while (!Eos())
		{
		TChar ch = Get();

		if (ch.IsHexDigit())
			type = ELast;			// Have digits!
		else if (ch == '.')
			if (type == ELast)		// we have digits?
				return EDecPeriod;
			else
				return EInvalid;	// Component just can't start with "."
		else if (ch == ':')
			{
			if (type == EInvalid)	// Any preceding digits?
				return EColon;
			else
				return EHexColon;
			}
		else
			return EInvalid;
		}
	return type;
	}

//
// TInetAddrParser::HeadPart
//	Parse the beginning part of a IPv6 address, up to the "::",
//	first IPv4 component or last component. Parse a sequence of
//	components (EHexColon)
//		0*7(<hexseq> ":")
//	and fills the i6addr with the converted results. Returns the
//	type of the first address component that is not EHexColon.
//
TAddressToken TInetAddrParser::HeadPart()
	{
	TAddressToken t = EInvalid;
	while (iFill < i6addr.Length() && (t = NextComponent()) == EHexColon)
		if (GetHexValue(i6addr[iFill++]))
			return EInvalid;
	return t;
	}

//
// TInetAddrParser::TailPart
//	Parse the tail part of a IPv6 address. The tail part is
//		*(<hexseq> ":") (<hexseq>| <ivp4address>)
//
//	Returns ETrue on correct input, and EFalse otherwise
//
TBool TInetAddrParser::TailPart(TAddressToken t)
	{
	for (;;)
		{
		if (t == EDecPeriod)
			return Ipv4address();
		else if (iFill == i6addr.Length() || GetHexValue(i6addr[iFill++]) != KErrNone)
			return EFalse;
		else if (t == ELast)
			return ETrue;
		else if (t != EHexColon)
			return EFalse;
		t = NextComponent();
		}
	}

//
// TInetAddrParser::Ipv4address
//	Parse the IPv4 address as a part of IPv6 address (must be at end)
//		<decseq> "." <decseq> "." <decseq> "." <decseq>
//		^
//		Should only be called when the current Mark points to this!
//
//	Returns ETrue, on correct input and EFalse on any error.
//
TBool TInetAddrParser::Ipv4address()
	{
	TPtr8 i4addr((TUint8 *)(&i6addr[iFill]), 4, 4);

	iFill += 2;
	if (iFill > 8)
		return EFalse;	// No room for the IPv4 address!

	UnGetToMark();
	for (TInt i = 0;;)
		{
		if (Val(i4addr[i++], EDecimal) != KErrNone)
			return EFalse;	// Overflow or some other number error
		if (i == 4)
			return Eos();	// Ok, if all parsed.
		if (Eos() || Get() != '.')
			return EFalse;	// Premature end or number not '.'-terminated
		}
	}

EXPORT_C TInt TInetAddr::Input(const TDesC& aDes)
	/**
	* Sets the IP address from a string containing a representation of an 
	* IP address, such as might be entered in a dialog.
	*
	* The string must contain an ASCII representation of the address. 
	* The format may be one of the following examples:
	* @li Four 8-bit decimal numbers separated by '.' (eg. <tt>192.168.40.4</tt>)
	* @li Two 8-bit decimal numbers and a 16 bit decimal (eg. <tt>192.168.10244</tt>)
	* @li One 8-bit decimal number and a 24-bit decimal (eg. <tt>192.11020292</tt>)
	* @li One 32 decimal number (eg. <tt>3232245764</tt>)
	* @li Eight 16-bit hex numbers separated by ':' (eg.<tt>2001:618:400:6a:0:0:0:abc</tt>)
	*
	* Use at most one '<tt>::</tt>' to denote consecutive zeroes
	* (eg. <tt>2001:618:400:6a::abc</tt>),
	* IPv4-compatible address (eg. <tt>::192.168.40.4</tt>), and
	* IPv4-mapped address (eg. <tt>::ffff:192.168.40.4</tt>)
	*
	* Any of the address notations above can be followed by "%scope-id" and
	* the scope id of the TInetAddress is also set accordingly. If a dotted
	* IPv4 address is associated with scope id, the address is stored as
	* IPv4-mapped. If scope is not specified, the value defaults to zero.
	* The flow label is set to zero.
	*
	* The hexadecimal numbers may be either upper or lower case.
	*
	* @param aDes
	*	Descriptor containing a string representation of an IP address 
	* @return
	*	KErrNone, if address syntax was correct and input succeeded;
	*	otherwise, one of the system-wide error codes.
	* @since 7.0 (IPv6 support)
	*/
	{
	TInt32 scope_id = 0;
	TPtrC ptr(aDes);
	//
	// recognize "%<numeric-scopeid>" notation
	// (for non-numeric "%<interface-name>" notation, the
	// RHostResolver must be used).
	//
	const TInt i = ptr.LocateReverse('%');
	if (i >= 0)
		{
		TLex num(ptr.Right(ptr.Length() - i - 1));
		if (num.Val(scope_id) != KErrNone)
			return KErrArgument; // aDes is not directly convertible.
		ptr.Set(ptr.Left(i));
		}
	TInt ret = Ipv4Input(ptr);
	if (ret != KErrNone)
		ret = Ipv6Input(ptr);
	if (ret == KErrNone && scope_id != 0)
		{
		// Augment returned address with the scope id
		if (Family() != KAfInet6)
			ConvertToV4Mapped();
		SetScope(scope_id);
		}
	return ret;
	}

TInt TInetAddr::Ipv4Input(const TDesC& aDes)
	/**
	* Parse string as IPv4 address.
	*/
	{
	TUint32 addr[KIPv4AddressPartCount];
	TInt current;
	TRadix radix;
	TChar ch;
	TInt err;

	TLex lex(aDes);
	// skip leading white spaces
	lex.SkipSpaceAndMark();

	// Extract upto 4 dot separated parts
	current = 0;
	do
		{
		// Get radix prefix if any
		radix = EDecimal;
		ch = lex.Peek();

		if (ch=='0') // might be "0x" for hex
			{
			lex.Mark();
			lex.Inc();
			ch = lex.Get();
			if (ch=='x' || ch=='X')
				radix = EHex;
			else
				lex.UnGetToMark();
			}

		// Get value of part
		if (err = lex.Val(addr[current++], radix), err!=KErrNone)
			return err;
		if (current == KIPv4AddressPartCount)
			{
			// skip trailing white spaces
			lex.SkipSpaceAndMark();
			}

		// Next char will be '.' if another part
		if (!lex.Eos())
			{
			ch = lex.Get();
			if(ch!='.')
				return KErrArgument;
			}
		}
	while (!lex.Eos() && ch=='.' && current<KIPv4AddressPartCount);

	if (ch=='.' && current>=KIPv4AddressPartCount)
		return KErrOverflow;

	// Assemble the address according to the
	// number of parts
	switch (current)
		{
		case 0: // Nothing could be extracted
		// lex.GotoMark();
			return KErrArgument;

		case 1: // a - 32 bits
			SetAddress(addr[0]);
			break;

		case 2: // a.b - 8.24 bits
			if (addr[0]>0xff || addr[1]>0xffffff)
				return KErrOverflow;
			SetAddress((addr[0]<<24) | addr[1]);
			break;
		case 3:	// a.b.c - 8.8.16 bits
			if (addr[0]>0xff || addr[1]>0xff || addr[2]>0xffff)
				return KErrOverflow;
			SetAddress((addr[0]<<24) | (addr[1]<<16) | addr[2]);
			break;
		case 4:	// a.b.c.d - 8.8.8.8 bits
			if (addr[0]>0xff || addr[1]>0xff || addr[2]>0xff || addr[3]>0xff)
				return KErrOverflow;
			SetAddress((addr[0]<<24) | (addr[1]<<16) | (addr[2]<<8) | addr[3]);
			break;
		default: // Too many parts
			return KErrOverflow;
		}

	return KErrNone;
	}

TInt TInetAddr::Ipv6Input(const TDesC& aDes)
	/**
	* Parse string as IPv6 address.
	*/
	{
	TUint32 addr;
	TInt head;
	TInetAddrParser lex(aDes, AddrPtr()->iAddr);
	lex.SkipSpaceAndMark();

	//
	// The first address token determines whether this is to be
	// parsed as IPv4 address (KAfInet) or IPv6 address (KAfInet6).
	//
	// If the first token is plain decimal number or a decimal number
	// terminating with a period, then assume this is a request to
	// return plain old KAfInet type address.

	switch (lex.HeadPart())
		{
		case ELast:	// IPv4 as single integer?
			if (lex.iFill == 0)
				{
				// The input "address" is specified as a single string of
				// hex or decimal digits. This is not valid IPv6 address,
				// and even dubious as IPv4, but accept it as IPv4 if it
				// is decimal number and fits into 32 bits.
				// Or, should things like "f", "ffe" be accepted as IPv6
				// addresses??
				lex.UnGetToMark();
				if (lex.Val(addr, EDecimal) == KErrNone && lex.Eos())
					{
					// All decimal digits and full string accepted by Val
					SetAddress(addr);
					return KErrNone;
					}
				}
			else if (lex.TailPart(ELast) && lex.iFill == 8)
				//
				// Now the address is given as "XX:XX:XX... :XX".
				//
				break;
			return KErrArgument;

		case EDecPeriod:
			if (lex.Ipv4address())
				if (lex.iFill == 2) // == 4 bytes!
					{
					// The input cannot be IPv6 address, it was a plain
					// IPv4 address.
					SetAddress(ByteOrder::Swap32(AddrPtr()->iAddr.u.iAddr32[0]));
					return KErrNone;
					}
				else if (lex.iFill == 8)
					// The were some IPv6 components followed by an IPv4. For
					// acceptable input, all components must be defined!
					break;
			return KErrArgument;

		case EColon:
			// IPv6 address with zerofill part. A special kludge: if the
			// address begins with "::", we must check the other colon
			// here, because HeadPart has not skipped it.
			//
			if (lex.iFill == 0 && lex.NextComponent() != EColon)
				return KErrArgument;	// Not a valid address format
			head = lex.iFill;
			if (!lex.Eos() && !lex.TailPart(lex.NextComponent()))
				return KErrArgument;
			// The [head,iTail] portion must be moved to the end of the
			// address and the middle part (if any) needs to be filled
			// with zeroes.
			if (lex.iFill < 8)
				{
				TInt i = 8;
				while (head < lex.iFill)
					lex.i6addr[--i] = lex.i6addr[--lex.iFill];
				while (head < i)
					lex.i6addr[--i] = 0;
				}
			break;

		default:
			return KErrArgument;		// Not a valid address format
		}
	SetFamily(KAfInet6);
	SetUserLen(sizeof(SInet6Addr));
	SetScope(0);
	SetFlowLabel(0);
	return KErrNone;
	}


//
//  IPv4 compatibility stuff
//
EXPORT_C void TInetAddr::NetMask(const TInetAddr& aAddr)
	/**
	* Sets the IP address to a mask suitable for extracting the network number
	* part of the IP address (deprecated).
	*
	* This function assumes the IPv4 address classes (A, B, C). Applications
	* using this function may not work properly in the current internet
	* environment. 
	*
	* The function tests whether the IP 
	* address in aAddr is a Class A, B, C, or other address. It then sets 
	* the IP address to suitable mask values as follows:
	*
	* @li Class A addresses: mask 255.0.0.0
	* @li Class B addresses: mask 255.255.0.0
	* @li Class C addresses: mask 255.255.255.0
	* @li other addresses: mask 255.255.255.255
	*
 	* The resulting address family is #KAfInet.
	*
	* @param aAddr
	*	TInetAddr from which to obtain IP address
	*
	* @deprecated
	*	Works only for IPv4. The length of the network prefix needs to be
	*	aqcuired by other means. When prefix length is known, PrefixMask()
	*	function can be used.
	*/
	{
	TUint32 mask = 0;
	TUint32 addr = aAddr.Address();
	if ((addr & KInetAddrIdMaskA) == KInetAddrIdValA)
		mask = KInetAddrNetMaskA;
	else if ((addr & KInetAddrIdMaskB) == KInetAddrIdValB)
		mask = KInetAddrNetMaskB;
	else if ((addr & KInetAddrIdMaskC) == KInetAddrIdValC)
		mask = KInetAddrNetMaskC;
	else
		mask = KInetAddrMaskHost;
	SetAddress(mask);
	}

EXPORT_C void TInetAddr::Net(const TInetAddr& aAddr)
	/**
	* Obtains a network mask corresponding to the class of the IP address,
	* using the NetMask() function, and then applies this mask to the current
	* IP address (deprecated).
	*
 	* The resulting address family is #KAfInet
	* 
	* @param aAddr
	*	TInetAddr from which to obtain IP address for creating the
	*	network mask. 
	* 
	* @deprecated
	*	Works only for IPv4. Prefix() should be used instead.
	*/
	{
	NetMask(aAddr);
	SetAddress(aAddr.Address() & Address());
	}

EXPORT_C void TInetAddr::SubNet(const TInetAddr& aAddr, const TInetAddr& aMask)
	/**
	* Sets the IP address to the network number and subnet parts of the IP 
	* address (that is, the original IP address with the host number part 
	* cleared).
	*
	* This is obtained by applying the mask value in aMask to the 
	* IP address in aAddr. The result is undefined if address and
	* mask have different address family.
	*
	* @param aAddr
	*	TInetAddr from which to obtain IP address
	* @param aMask
	*	TInetAddr object with the IP address set to the relevant subnet mask. 
	*/
	{
	if (aAddr.Family() != KAfInet6)
		SetAddress(aAddr.Address() & aMask.Address());
	else
		{
		Init(KAfInet6);

		struct SInet6Addr *const a = AddrPtr();
		const struct SInet6Addr *const b = aAddr.AddrPtr();
		const struct SInet6Addr *const m = aMask.AddrPtr();
		a->iAddr.u.iAddr32[0] = m->iAddr.u.iAddr32[0] & b->iAddr.u.iAddr32[0];
		a->iAddr.u.iAddr32[1] = m->iAddr.u.iAddr32[1] & b->iAddr.u.iAddr32[1];
		a->iAddr.u.iAddr32[2] = m->iAddr.u.iAddr32[2] & b->iAddr.u.iAddr32[2];
		a->iAddr.u.iAddr32[3] = m->iAddr.u.iAddr32[3] & b->iAddr.u.iAddr32[3];
        }
	}

EXPORT_C void TInetAddr::SubNetBroadcast(const TInetAddr& aAddr, const TInetAddr& aMask)
	/**
	* Sets the IP address to be suitable for a subnet-directed broadcast
	* address. 
	*
	* To achieve this, the subnet mask value in aMask is applied to the IP
	* address in aAddr. The mask is then used again to set all host number
	* bits to 1 (which  signifies, broadcast to all hosts on the subnet).
	*
	* Only for IPv4. The function does not have a sensible interpretation in 
	* IPv6. An application that uses this needs to code separate branches for 
	* IPv4 and IPv6 cases.
	* 
	* @param aAddr
	*	TInetAddr from which to obtain IP address
	* @param aMask
	*	TInetAddr object with the IP address set to the relevant subnet mask.
	*
	* @deprecated
	*	Works only for IPv4. IPv6 has all-nodes link local multicast
	*	address (ff02::1) for this purpose.
	*/
	{
	const TUint32 addr = aAddr.Address();
	const TUint32 mask = aMask.Address();
	SetAddress((addr & mask) | (KInetAddrBroadcast & ~mask));
	}

EXPORT_C void TInetAddr::NetBroadcast(const TInetAddr& aAddr)
	/**
	* Sets the IP address to be suitable for a network-directed broadcast
	* address (deprecated).
	*
	* The function obtains a network mask corresponding to the class of
	* the IP address in aAddr, using the NetMask() function. It then
	* applies this mask to the current IP address to separate the network
	* number part of the address. The mask is then used again to set all
	* host number bits to 1 (which signifies, broadcast to all hosts on
	* the network).
	*
 	* The resulting address family is #KAfInet
	* 
	* @param aAddr
	*	TInetAddr from which to obtain IP address for creating the network mask. 
	* 
	* @deprecated Works only for IPv4.
	* @deprecated
	*	This function works only for IPv4. It assumes the old 
	*	IPv4 address classes (A, B, C). Applications using this
	*	function may not work properly in the current internet environment.
	*/
	{
	TInetAddr netmask;
	netmask.NetMask(aAddr);
	const TUint32 addr = aAddr.Address();
	const TUint32 mask = netmask.Address();
	SetAddress((addr & mask) | (KInetAddrBroadcast & ~mask));
	}

EXPORT_C TInt TSoInetRouteInfo::GetLinkAddr( TLinkAddr &aInfo ) const
	{
	/**
	* Checks to make sure the gateway member is actually a link layer
	* address and not the IP address of a gateway
	* If the function is successful, the link layer address
	* of the neighbour is stored in the parameter and KErrNone is
	* returned. If the function is unsuccessful because
	* the link layer address cannot be obtained, KErrNotFound is
	* returned and the parameter is not modified.
	*
 	* @param aInfo parameter to store link layer address if found
	* @return KErrNone if link layer address found else KErrNotFound 
	*/
	TInt len = const_cast<TSoInetRouteInfo*>( this )->iGateway.GetUserLen();
	if(len < 1)
		{
		return KErrNotFound;
		}

	TInt family = iGateway.Family();
	if ((family != KAFUnspec) && (family != KAfInet) && (family != KAfInet6))
		{
		aInfo = TLinkAddr::Cast( iGateway );
		return KErrNone;
		}
	return KErrNotFound;
	}

EXPORT_C TLinkAddr::TLinkAddr()
	/**
	* Default constructor initialises the address to be zero-filled.
	*/
	{;}

EXPORT_C void TLinkAddr::SetAddress(const TDesC8 &aAddr)
	/**
	* Assigns a string of octets as the address.
	* @param aAddr The link layer address.
	*/
	{
	const TInt len = aAddr.Length();
	SetUserLen(len);
	TPtr8((TUint8 *)UserPtr(), len).Copy(aAddr);
	}

EXPORT_C TPtrC8 TLinkAddr::Address() const
	/**
	* Returns the current stored address as a pointer to string of octets.
	* @return The link layer address.
	*/
	{
	return TPtrC8((TUint8 *)UserPtr(), ((TSockAddr *)this)->GetUserLen());
	}

EXPORT_C const TLinkAddr& TLinkAddr::Cast(const TSockAddr& aAddr)
	/**
	* This function will cast the address to a TLinkAddr.
	* This function has no way of verifying that the source address is actually a link layer address and should
	* not be passed an IP address.
	* @param address to be cast
	* @return link layer cast of passed parameter
	*/
	{
	return *((TLinkAddr *)&aAddr);
	}

EXPORT_C TLinkAddr& TLinkAddr::Cast(TSockAddr& aAddr)
	/**
	* This function will cast the address to a TLinkAddr.
	* This function has no way of verifying that the source address is actually a link layer address and should
	* not be passed an IP address.
	* @param address to be cast
	* @return link layer cast of passed parameter
	*/
	{
	return *((TLinkAddr *)&aAddr);
	}

EXPORT_C const TLinkAddr* TLinkAddr::Cast(const TSockAddr* aAddr)
	/**
	* This function will cast the address to a TLinkAddr.
	* This function has no way of verifying that the source address is actually a link layer address and should
.	* not be passed an IP address.
	*/
	{
	return (TLinkAddr *)aAddr;
	}

EXPORT_C TLinkAddr* TLinkAddr::Cast(TSockAddr* aAddr)
	/**
	* This function will cast the address to a TLinkAddr.
	* This function has no way of verifying that the source address is actually a link layer address and should
.	* not be passed an IP address.
	*/
	{
	return (TLinkAddr *)aAddr;
	}
