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
// ipaddress.h - Unify IPv4/IPv6 Raw Address handling
//



/**
 @internalComponent
*/
#ifndef __IPADDRESS_H__
#define __IPADDRESS_H__

#include <in_sock.h>

class TIpAddress : public TIp6Addr
	/**
	* Container for a raw IPv6 address with the scope id.
	*/
	{
public:
	TIpAddress() {}
	TIpAddress(const TIp6Addr &aAddr, const TUint32 aScope) : iScope(aScope) {(TIp6Addr &)*this = aAddr; }
	inline TIpAddress(const TSockAddr &aAddr);
	inline const TIp6Addr &Address() const { return *this; }

	TInt operator==(const TIpAddress &aAddr) const;
	TBool operator<=(const TIpAddress &aAddr) const;
	TInt operator!=(const TIpAddress &aAddr) const;

	TBool IsEqMask(const TIpAddress &aAddr, const TIpAddress &aMask) const;
	TBool IsMulticast() const;
	TInt SetAddress(const TDesC &aAddr, TInt aMask = 0);
	TInt SetAddress(const TSockAddr &aAddr);
	TInt SetMask(const TSockAddr &aAddr);
	void SetAddress(const TIp6Addr &aAddr, const TUint32 aScope);
	void SetAddress(const TUint32 aAddr);
	void SetAddressNone();						// Set all ZERO address.
	inline TBool IsNone() const;				// Test for all zero address
	// A dubious method. But, this returns the IPv4 address.
	// It will return the value of the last 4 bytes swapped into host order.
	// (basicly a reverse of SetAddress(TUint32), but without any checks)
	inline TUint32 Ip4Address() const;

	TUint32 iScope;
	};


//
//	...by EPOC convention, the following should be in "ipaddress.inl" I suppose...
//
inline TIpAddress::TIpAddress(const TSockAddr &aAddr)
	/**
	* Constructor.
	*
	* Set initial content from the TSockAddr.
	*
	* @param aAddr The initial address.
	*/
	{
	SetAddress(aAddr);
	}

inline TBool TIpAddress::IsNone() const
	/**
	* Test if address is unspecified.
	*
	* Note: This does not care about the scope id.
	*/
	{
	return IsUnspecified();
	}

inline TUint32 TIpAddress::Ip4Address() const
	/**
	* Return as IPv4 address.
	*
	* Assume the content is currently an IPv4 address in
	* IPv4 mapped format, and return that IPv4 address as
	* TUint32.
	*
	* @return The IPv4 address.
	*/
	{
	return
		(u.iAddr8[12] << 24) |
		(u.iAddr8[13] << 16) |
		(u.iAddr8[14] <<  8) |
		(u.iAddr8[15]);
 	}

#endif
