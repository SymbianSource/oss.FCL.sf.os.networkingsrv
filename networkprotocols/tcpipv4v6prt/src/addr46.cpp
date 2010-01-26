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
// addr46.cpp - an extention to TIp6Addr
//

#include "addr46.h"

static const union {TUint8 a[4]; TUint32 b;} v4Prefix = { {0, 0, 0xff, 0xff} };

//
// TIp46Addr::TIp46Addr
// ********************
// Construct TIp6Addr as IPv4 mapped address from IPv4 address
//
TIp46Addr::TIp46Addr(const TUint32 aAddr)
	{
	u.iAddr32[0] = 0;
	u.iAddr32[1] = 0;
	u.iAddr32[2] = v4Prefix.b;
	u.iAddr8[15] = (TUint8)aAddr;
	u.iAddr8[14] = (TUint8)(aAddr >>  8);
	u.iAddr8[13] = (TUint8)(aAddr >> 16);
	u.iAddr8[12] = (TUint8)(aAddr >> 24);
	}
//
// Construct TIp6Adr from generic TInetAddr,
//
TIp46Addr::TIp46Addr(const TInetAddr &aAddr)
	{
	if (aAddr.Family() == KAfInet6)
		(*(TIp6Addr *)this) = aAddr.Ip6Address();
	else if (aAddr.Family() == KAfInet)
		(*(TIp6Addr *)this) = TIp46Addr(aAddr.Address());
	else
		(*(TIp6Addr *)this) = KInet6AddrNone;
	}


//
// TIp46Addr::IsMulticast
// **********************
// Return TRUE if address is IPv4 or IPv6 multicast
//
TBool TIp46Addr::IsMulticast() const
	{
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

//
// TIp46Addr::IsLinklocal
// **********************
// Return TRUE if address is IPv4 or IPv6 multicast
//
TBool TIp46Addr::IsLinkLocal() const
	{
	return Scope() == KIp6AddrScopeLinkLocal;
	}


// TIp46Addr::IsUnspecified
// ************************
TBool TIp46Addr::IsUnspecified() const
	{
	return
		u.iAddr32[0] == 0 &&
		u.iAddr32[1] == 0 &&
		(u.iAddr32[2] == 0 || u.iAddr32[2] == v4Prefix.b) &&
		u.iAddr32[3] == 0;
	}
