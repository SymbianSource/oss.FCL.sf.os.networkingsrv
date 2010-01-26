// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Extended address class with host/network byte order options
// 
//

/**
 @file
 @internalTechnology
*/

#ifndef IP_ADDR_EX_H
#define IP_ADDR_EX_H

#include <ss_std.h>
#include <in_sock.h>        // for TInetAddr

// lightweight specialisation of TInetAddr
// - required to access some of the needed protected methods, also some useful helper methods
// - 'Ex' suffix is an abbreviation for "Extended"
struct TInetAddrEx : public TInetAddr
	{
public:
    inline TInetAddrEx() {}
    inline TInetAddrEx(TUint32 aAddr)
        {
        SetFamily(KAfInet);
        SetAddress(aAddr);
        }
    inline TInt AddrLen()
        {
        TInt len;
        switch (Family())
            {
            case KAfInet:
                len = AddrLen4();
                break;
            case KAfInet6:
                len = AddrLen6();
                break;
            default:    // eg. KAfUnspec
                len = 0;    
                break;
            }
        return len;
        }
    static inline TInt AddrLen6()
        {
        return sizeof(SInet6Addr);
        }
    static inline TInt AddrLen4()
        {
        return sizeof(SInetAddr);
        }
    inline static TInetAddrEx &Cast(const TSockAddr &aAddr)
		{
		return *((TInetAddrEx *)&aAddr);
		}

    // posix style host to network order - using C++ overload instead of C style suffix to denote type
    static inline TUint16 hton(TUint16 aValue)
        {
        return ((aValue >> 8) & 0x00ff) | 
               ((aValue << 8) & 0xff00);
        }
    static inline TUint32 hton(TUint32 aValue)
        {
        return ((aValue >> 24) & 0x000000ff) | ((aValue >> 8) & 0x0000ff00) | 
               ((aValue << 24) & 0xff000000) | ((aValue << 8) & 0x00ff0000);
        }
    static inline TUint64 hton(TUint64 aValue)
        {
		TUint64 val1, val2, val3, val4, val5, val6, val7, val8;

		val1 = ((aValue >> 56) & 0x00000000000000ff);
		val2 = ((aValue >> 40) & 0x000000000000ff00);
		val3 = ((aValue >> 24) & 0x0000000000ff0000);
		val4 = ((aValue >>  8) & 0x00000000ff000000);
		val5 = ((aValue << 56) & (((TUint64)(0xff)) << 56));
		val6 = ((aValue << 40) & (((TUint64)(0xff)) << 48));
		val7 = ((aValue << 24) & (((TUint64)(0xff)) << 40));
		val8 = ((aValue <<  8) & (((TUint64)(0xff)) << 32));

		return val1 | val2 | val3 | val4 | val5 | val6 | val7 | val8;

//        return ((aValue >> 56) & 0x00000000000000ff) | ((aValue >> 40) & 0x000000000000ff00) | ((aValue >> 24) & 0x0000000000ff0000) | ((aValue >> 8) & 0x00000000ff000000) | 
//               ((aValue << 56) & 0xff00000000000000) | ((aValue << 40) & 0x00ff000000000000) | ((aValue << 24) & 0x0000ff0000000000) | ((aValue << 8) & 0x000000ff00000000);
        }
    static inline TUint hton(const TUint8 *aUnaturallyUnalignedValue)
        {
        // if the overload using (TUint8 *) is used, then we can not assume the data is naturally aligned (eg. Int32 on 4B boundary)
        TUint32 value;
        memcpy(&value, aUnaturallyUnalignedValue, sizeof(value));
        return hton(value);
        }

    static inline TUint16 ntoh(TUint16 aValue)
        {
        return hton(aValue);
        }
    static inline TUint32 ntoh(TUint32 aValue)
        {
        return hton(aValue);
        }
    static inline TUint64 ntoh(TUint64 aValue)
        {
        return hton(aValue);
        }
    static inline TUint ntoh(const TUint8 *aUnaturallyUnalignedValue)
        {
        return hton(aUnaturallyUnalignedValue);
        }

    // host to network order conversion of multiple 32bit ints
    static inline void hton(TUint32 *aData, TUint aLen)
        {
        for (int i = 0; i < aLen/sizeof(TUint32); i++)
            {
            aData[i] = TInetAddrEx::hton(aData[i]);
            }
        }
    static inline void ntoh(TUint32 *aData, TUint aLen)
        {
        hton(aData, aLen);
        }


    // return descriptor copy of address (by value) in network order (ipv4 addr are stored in host order)
    // - use ::Address() to retrieve as a TInt in host order
    // - return as a const to allow result to be assigned to a TPtr8 via it's assignment overload without invoking
    //   lint error(?); "E1058: Initializing a non-const reference 'TPtr8 &' with a non-lvalue"
    inline const TBuf8<4> AddrNetwork()
		{ 
        TInt addrNetwork = hton(Address());

        TBuf8<4> addr;
        addr.Copy(TPtrC8((TUint8 *) &addrNetwork, sizeof(addrNetwork)));
        return addr;
		}
	}; 

#endif // IP_ADDR_EX_H
