// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// ipaddress.cpp - Basic IP address class for IPSEC use
// @internalComponent	for IPSEC
//

#include "ipaddress.h"

TInt TIpAddress::operator==(const TIpAddress &aAddr) const
	/**
	* Equality operator.
	*
	* Two TIpAddress are equal if the TIp6Addr part matches
	* exactly (TIp6addr::IsEqual()), and either scope id's
	* are equal or either one is zero, which acts as a wild
	* card.
	*
	* @param aAddr The other address.
	*/
	{
	// iScope == 0 acting as a "wildcard" is *DUBIOUS* for == operator. need to check -- msa
	return IsEqual(aAddr) && (iScope == aAddr.iScope || iScope == 0 || aAddr.iScope == 0);
	}

TBool TIpAddress::operator<=(const TIpAddress &aAddr) const
	/**
	* Relation operator.
	*
	* The presense of the scope confuse the issue. The nature of the
	* scope id is such, that only exact equality test makes sense.
	*
	* Thus, the "<=" is true if one address is less or equal to the
	* other, and if scope ids are equal or either one is zero, which acts
	* as a wild card.
	*
	* Thus, EFalse return does not mean that the other address
	* is "larger", e.g. it is possible that
	* @code (addr1 <= addr2 || addr2 <= addr1)
	* @endcode
	* is false; addresses are not comparable (because scope ids do not match).
	*/
	{
	return
		(iScope == 0 || aAddr.iScope == 0 || iScope == aAddr.iScope) &&
		Mem::Compare(&u.iAddr8[0], 16, &aAddr.u.iAddr8[0], 16) <= 0;
	}

TInt TIpAddress::operator!=(const TIpAddress &aAddr) const
	{
	return ! (*this == aAddr);
	}

TBool TIpAddress::IsEqMask(const TIpAddress &aAddr, const TIpAddress &aMask) const
	/**
	* Compare address under mask.
	*
	* Tests if
	* @code (*this & aMask) == (aAddr & aMask)
	* @endcode
	* is true.
	*
	* @param aAddr The other address.
	* @param aMask The mask.
	* @return ETrue, if equal.
	*/
	{
	return
		((u.iAddr32[0] ^ aAddr.u.iAddr32[0]) & aMask.u.iAddr32[0]) == 0 &&
		((u.iAddr32[1] ^ aAddr.u.iAddr32[1]) & aMask.u.iAddr32[1]) == 0 &&
		((u.iAddr32[2] ^ aAddr.u.iAddr32[2]) & aMask.u.iAddr32[2]) == 0 &&
		((u.iAddr32[3] ^ aAddr.u.iAddr32[3]) & aMask.u.iAddr32[3]) == 0 &&
		((iScope ^ aAddr.iScope) & aMask.iScope)  == 0;
	}

TBool TIpAddress::IsMulticast() const
	/**
	* Test if address is multicast address.
	*
	* The base class TIp6Addr::IsMulticast() returns true only for
	* real IPv6 multicast addresses. This function expands the test
	* to work also for IPv4 multicast addresses, which are stored
	* as IPv4 mapped format.
	*
	* @return True, if address is multicast (IPv4 or IPv6).
	*/
	{
	static const union {TUint8 a[4]; TUint32 b;} v4Prefix = { {0, 0, 0xff, 0xff} };

	// TRUE, if real IPv6 multicast
	if (TIp6Addr::IsMulticast())
		return TRUE;
	//
	// Otherwise, return TRUE, if address is the IPv4
	// multicast address.
	return
		u.iAddr32[0] == 0 &&
		u.iAddr32[1] == 0 &&
		u.iAddr32[2] == v4Prefix.b &&
		(u.iAddr8[12] & 0xF0) == 0xE0;
	}


TInt TIpAddress::SetAddress(const TDesC &aStr, TInt aMask)
	/**
	* Set address from a string
	*
	* Take a string as a parameter and attempt to parse this as
	* an IP address.
	*
	* The aMask setting is needed when IP address is used
	* as a bit mask. IPv4 addresses are expanded into IPv4 mapped format.
	* This would not work right when address is used as a mask, and thus a
	* special flag is required.
	*
	* @param aStr	Literal IPv4 or IPv6 address
	* @param aMask	Non-zero, if address is to be used as a bitmask
	* @return KErrNone, if address literal was valid.
	*/
	{
	TInetAddr addr;
	const TInt ret = addr.Input(aStr);
	if (ret == KErrNone)
		return aMask ? SetMask(addr) : SetAddress(addr);
	return ret;
	}

TInt TIpAddress::SetMask(const TSockAddr &aAddr)
	/**
	* Set address to be used as a mask.
	*
	* This function is needed. when aAddr contains a IPv4
	* address. The resulting IPv4 mapped format would not
	* be usable as  mask bits, and the high order bits must
	* also be set.
	*
	* @param aAddr The mask
	* @return KErrNone
	*/
	{
	const TInetAddr &addr = TInetAddr::Cast(aAddr);

	if (addr.Family() == KAfInet || addr.IsV4Mapped())
		{
		// Loading IPv4 address requires some extra shuffles..
		SetAddress(addr.Address());
		// When an IPv4 address intended to be a mask is converted
		// to IPv6 address, the rest of IPv6 address bits must
		// be set to all ones too.
		u.iAddr32[0] = ~0U;
		u.iAddr32[1] = ~0U;
		u.iAddr32[2] = ~0U;
		}
	else
		(TIp6Addr &)*this = addr.Ip6Address();
	iScope = addr.Scope();
	return KErrNone;
	}

TInt TIpAddress::SetAddress(const TSockAddr &aAddr)
	/**
	* Set address.
	*
	* @param aAddr The address.
	* @return KErrNone
	*/
	{
	const TInetAddr &addr = TInetAddr::Cast(aAddr);
	if (addr.Family() == KAfInet)
		// Loading IPv4 address requires some extra shuffles..
		SetAddress(addr.Address());
	else
		{
		(TIp6Addr &)*this = addr.Ip6Address();
		}
	iScope = addr.Scope();
	return KErrNone;
	}

void TIpAddress::SetAddressNone()
	/**
	* Set address to none.
	*
	* Fill with zeroes. 
	*/
	{
	(TIp6Addr &)*this = KInet6AddrNone;
	iScope = 0;
	}

void TIpAddress::SetAddress(const TIp6Addr &aAddr, const TUint32 aScope)
	/**
	* Set address.
	*
	* @param aAddr The IPv6 address
	* @param aScope The scope id.
	*/
	{
	(TIp6Addr &)*this = aAddr;
	iScope = aScope;
	}

void TIpAddress::SetAddress(const TUint32 aAddr)
	/**
	* Set address.
	*
	* Set address from IPv4 address.
	*
	* @param aAddr The IPv4 address
	*/
	{
	u.iAddr32[0] = 0;
	u.iAddr32[1] = 0;
	u.iAddr8[8] = 0;
	u.iAddr8[9] = 0;
	u.iAddr8[10] = 0xff;
	u.iAddr8[11] = 0xff;

	u.iAddr8[12] = (TUint8)(aAddr >> 24);
	u.iAddr8[13] = (TUint8)(aAddr >> 16);
	u.iAddr8[14] = (TUint8)(aAddr >>  8);
	u.iAddr8[15] = (TUint8)aAddr;
	// The scope id is not set? should clear it?
	}

