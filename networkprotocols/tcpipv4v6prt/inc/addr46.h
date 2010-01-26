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
// addr46.h - an extention to TIp6Addr
//



/**
 @internalComponent
*/
#ifndef __addr46_H__
#define __addr46_H__

#include <in_sock.h>

//
// TIp46Addr
// *********
// Provides some additional methods for handling
// both real IPv6 addresses and mangled IPv4 addresses
// in somewhat uniform way.
//
class TIp46Addr : public TIp6Addr
	{
public:
	inline TIp46Addr() {}	// inline to make declaring variable short in code
	TIp46Addr(const TUint32 aAddr);
	TIp46Addr(const TInetAddr &aAddr);
	inline static const TIp46Addr &Cast(const TIp6Addr &aAddr) { return *((TIp46Addr *)&aAddr); }
	inline static const TIp46Addr &Cast(const TIp6Addr *aAddr) { return *((TIp46Addr *)aAddr); }

	TBool IsMulticast() const;	// Return TRUE if address is IPv4 or IPv6 multicast
	TBool IsLinkLocal() const;	// Return TRUE if address is IPv4 or IPv6 linklocal
	TBool IsUnspecified() const;// Return TRUE if address is :: or ::ffff:0.0.0.0
	};

#endif
