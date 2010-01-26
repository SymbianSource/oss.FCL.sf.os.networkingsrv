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
// eui_addr.cpp - EUI-xx address structure
//

#include "eui_addr.h"	// This
#include <e32std.h>		// Mem::*, User::*, TTime
#include <e32math.h>	// Math::*

static void EuiPanic(TEuiPanic aReason);
static void EuiPanic(TEuiPanic aReason)
{
	User::Panic(_L("EUI"), aReason);
}

EXPORT_C TE64Addr::TE64Addr()
{
	SetAddrZero();
}

EXPORT_C TE64Addr::TE64Addr(const TE64Addr& aAddr)
{
	SetAddr(aAddr);
}

EXPORT_C TE64Addr::TE64Addr(const TInt64& aAddr)
{
	SetAddr(aAddr);
}

EXPORT_C TE64Addr::TE64Addr(const TUint8* aPtr, TUint aLength)
{
	SetAddr(aPtr, aLength);
}

EXPORT_C void TE64Addr::SetAddr(const TE64Addr& aAddr)
{
	u = aAddr.u;
}

EXPORT_C void TE64Addr::SetAddr(const TInt64& aAddr)
{
#ifdef I64HIGH
	u.iAddr32[0] = I64HIGH(aAddr);
	u.iAddr32[1] = I64LOW(aAddr);
#else
	u.iAddr32[0] = aAddr.High();
	u.iAddr32[1] = aAddr.Low();
#endif
}

EXPORT_C void TE64Addr::SetAddr(const TUint8* aPtr, TUint aLength)
{
	__ASSERT_DEBUG(aLength <= AddrLen(), EuiPanic(EEuiPanicSourceTooLong));
	Mem::Copy(AddrPtr(), aPtr, aLength);
}

EXPORT_C void TE64Addr::SetGroupBit(TBool aBit)
{
	__ASSERT_DEBUG(aBit == 0 || aBit == 1, EuiPanic(EEuiPanicBitIsNotBit));

	//   0       0 0       1 1       2 ..
	//  |0       7 8       5 6       3|..
	//  +----+----+----+----+----+----+
	//  |cccc|ccug|cccc|cccc|cccc|cccc|..
	//  +----+----+----+----+----+----+

	if (aBit)
	{
		u.iAddr8[0] |= 0x1;
	}
	else
	{
		u.iAddr8[0] &= 0xFE;
	}
}

EXPORT_C void TE64Addr::SetUniversalBit(TBool aBit)
{
	__ASSERT_DEBUG(aBit == 0 || aBit == 1, EuiPanic(EEuiPanicBitIsNotBit));

	//   0       0 0       1 1       2 ..
	//  |0       7 8       5 6       3|..
	//  +----+----+----+----+----+----+
	//  |cccc|ccug|cccc|cccc|cccc|cccc|..
	//  +----+----+----+----+----+----+

	if (aBit)
	{
		u.iAddr8[0] |= 0x2;
	}
	else
	{
		u.iAddr8[0] &= 0xFD;
	}
}

EXPORT_C void TE64Addr::SetAddrZero()
{
	Mem::FillZ(AddrPtr(), AddrLen());
}

EXPORT_C void TE64Addr::SetAddrRandom()
{
	TTime now;

#ifndef EUI_PSEUDORANDOM
	now.UniversalTime();
#endif

	TInt64 seed = now.Int64();
	u.iAddr32[0] = Math::Rand(seed);
	u.iAddr32[1] = Math::Rand(seed);
	SetUniversalBit(0);
	SetGroupBit(0);
}

EXPORT_C void TE64Addr::SetAddrRandomNZ()
{
	do {
		SetAddrRandom();
	}
	while (IsZero());
}

EXPORT_C void TE64Addr::SetAddrRandomNZButNot(const TE64Addr& aAddr)
{
	do {
		SetAddrRandomNZ();
	}
	while (Match(aAddr));
}

// Really should define TE48Addr for this... -tom

EXPORT_C void TE64Addr::SetAddrFromEUI48(const TUint8* aPtr)
{
	u.iAddr8[0] = aPtr[0];
	u.iAddr8[1] = aPtr[1];
	u.iAddr8[2] = aPtr[2];
	u.iAddr8[3] = 0xff;
	u.iAddr8[4] = 0xfe;
	u.iAddr8[5] = aPtr[3];
	u.iAddr8[6] = aPtr[4];
	u.iAddr8[7] = aPtr[5];

	// The following is really not part of EUI-64 standard
	// but required for IPv6 when forming an IPv6 interface
	// identifier from an EUI-64 adderss. See RFC-2464,
	// RFC-2373. Probably the stack should do this instead. -tom

	// > It is required that the "u" bit (universal/local bit
	// > in IEEE EUI-64 terminology) be inverted when forming
	// > the interface identifier from the EUI-64.  The "u" bit
	// > is set to one (1) to indicate global scope, and it is
	// > set to zero (0) to indicate local scope.

	// So at this moment the address complies EUI-64

	SetUniversalBit(!IsUniversal());

	// Now it doesn't, but complies to IPv6 an interface identifier
}

EXPORT_C TBool TE64Addr::Match(const TE64Addr& aAddr) const
{
	return Mem::Compare(AddrPtrC(), AddrLen(), aAddr.AddrPtrC(), AddrLen()) ? 0 : 1;
}

EXPORT_C TBool TE64Addr::IsZero() const
{
	return (u.iAddr32[0] == 0) && (u.iAddr32[1] == 0);
}

EXPORT_C TBool TE64Addr::IsGroup() const
{
	return (u.iAddr8[0] & 0x1);
}

EXPORT_C TBool TE64Addr::IsUniversal() const
{
	return (u.iAddr8[0] & 0x2);
}

EXPORT_C void TE64Addr::Output(TDes& aBuf) const
{
	TPtrC8 e64Addr(AddrPtrC(), AddrLen());
	TUint i;

	for (i = 0; i < AddrLen(); i++)
	{
		if (i) {
			aBuf.Append(':');
		}

		aBuf.AppendNum(TUint(e64Addr[i]), EHex);
	}
}

EXPORT_C TUint TE64Addr::AddrLen()
{
	return sizeof(TE64Addr);
}

EXPORT_C TUint8* TE64Addr::AddrPtr()
{
	return &u.iAddr8[0];
}

EXPORT_C const TUint8* TE64Addr::AddrPtrC() const
{
	return &u.iAddr8[0];
}

EXPORT_C TEui64Addr::TEui64Addr() : TSockAddr()
{
	Init();
	Address().SetAddrZero();
}

EXPORT_C TEui64Addr::TEui64Addr(const TSockAddr& aAddr) : TSockAddr()
{
	__ASSERT_ALWAYS(aAddr.Family() == KAfEui64 || aAddr.Family() == 0, EuiPanic(EEuiPanicFamilyMismatch));
	Init();
	SetAddress(TEui64Addr::Cast(aAddr).Address());
}

EXPORT_C void TEui64Addr::Init()
{
	SetFamily(KAfEui64);
	SetPort(0);
	SetUserLen(AddrLen());
}

EXPORT_C void TEui64Addr::SetAddress(const TE64Addr& aAddr)
{
	Address().SetAddr(aAddr);
}

EXPORT_C TE64Addr& TEui64Addr::Address() const
{
	return AddrPtr()->iAddr;
}

EXPORT_C TBool TEui64Addr::Match(const TEui64Addr& aAddr) const
{
	return Address().Match(aAddr.Address());
}

EXPORT_C TBool TEui64Addr::IsZero() const
{
	return Address().IsZero();
}

EXPORT_C TEui64Addr& TEui64Addr::Cast(const TSockAddr& aAddr)
{
	return *((TEui64Addr*)&aAddr); //lint !e1773 // standard way to implement Cast
}

EXPORT_C TEui64Addr& TEui64Addr::Cast(const TSockAddr* aAddr)
{
       return *((TEui64Addr*)aAddr); //lint !e1773 // standard way to implement Cast
}

EXPORT_C SE64Addr* TEui64Addr::AddrPtr() const
{
	return (SE64Addr*)UserPtr();
}

EXPORT_C TUint TEui64Addr::AddrLen()
{
	return TE64Addr::AddrLen();
}
