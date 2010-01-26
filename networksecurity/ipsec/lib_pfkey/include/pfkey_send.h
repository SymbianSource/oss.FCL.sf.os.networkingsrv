// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

/**
 @file pfkey_send.h
 @publishedPartner
 @released
*/

#ifndef PFKEY_SEND_H
#define PFKEY_SEND_H

#include <pfkey_ext.h>

template <TInt v>
struct Int2Type
	{
	};

/**
 *  This class is used to dispatch the messages.
 *  The reason for having this is a bit ambiguous
 *  Really used to provide a uniform interface in a BC way
 */  
class TPfkeySendMsg : public TPfkeySendMsgBase
	{
public:
	/** 
	 *  The General syntax for these is 
	 *  Add(Int2Type<Ext_type>, TArgType& arg1, TArgType& arg2....)
	 *  The client writes, - dispatch.Add(Int2Type<int_type>(), arg1, arg2)
	 */
	/**
	 *	This will lead to the appropriate constructor being diapactched 
	 *  at compile time.
	 *  The add function calls the real constructor, and adds the packet to 
	 *  the message.
	 */  
	inline void Add(
		Int2Type<SADB_EXT_SA>,
		TUint32 aSPI = 0,
        TUint8 anAuth = 0,
        TUint8 anEncrypt = 0,
        TUint8 aState = SADB_SASTATE_MATURE,
        TUint8 aReplay = 0,
        TUint32 aFlags = 0);
	
	inline void Add(
		Int2Type<SADB_EXT_SPIRANGE>,
		TUint32 aMin = 0, 
		TUint32 aMax = 0xFFFFFFFF);

	inline void Add(
		Int2Type<SADB_EXT_ADDRESS_SRC>,
		const TInetAddr& anAddr,
        TUint8 aProtocol = 0,
        TUint8 aPrefixlen = 0);
	
	inline void Add(
		Int2Type<SADB_EXT_ADDRESS_DST>,
		const TInetAddr& anAddr,
        TUint8 aProtocol = 0,
        TUint8 aPrefixlen = 0);

	inline void Add(
		Int2Type<SADB_EXT_ADDRESS_PROXY>,
		const TInetAddr& anAddr,
        TUint8 aProtocol = 0,
        TUint8 aPrefixlen = 0);

	inline void Add(
		Int2Type<SADB_EXT_KEY_ENCRYPT>,
		const TDesC8& aKey,
        TUint16 aBits = 0);
	
	inline void Add(
		Int2Type<SADB_EXT_KEY_AUTH>,
		const TDesC8& aKey,
        TUint16 aBits = 0);

	inline void Add(
		Int2Type<SADB_EXT_IDENTITY_SRC>,
		const TDesC8& aIdentity, 
        TUint16 aIdType);

	inline void Add(
		Int2Type<SADB_EXT_IDENTITY_DST>,
		const TDesC8& aIdentity, 
        TUint16 aIdType);

	inline void Add(
		Int2Type<SADB_EXT_LIFETIME_HARD>,
        TUint32 aAllocations,
        const TInt64& aBytes,
        const TInt64& aAddtime,
        const TInt64& aUsetime);
	
	inline void Add(
		Int2Type<SADB_EXT_LIFETIME_SOFT>,
        TUint32 aAllocations,
        const TInt64& aBytes,
        const TInt64& aAddtime,
        const TInt64& aUsetime);

	//  
    // Private extension for ESP UDP encapsulation parameters
    // 
	inline void Add(
		Int2Type<SADB_PRIV_GENERIC_EXT>,   // 31.07 2003 JPS
		const TDesC8& aExtensionData);

	inline TPfkeySendMsg(TUint8 aType, TUint8 aSaType, TInt aRequestCount = 0, TInt aPid = 0);
	
	inline TPfkeySendMsg();
#ifdef SYMBIAN_NETWORKING_IPSEC_IKE_V2
	
	inline void Add(
		Int2Type<SADB_X_EXT_TS>,
        RArray<TSadbSelector> aSelectors);

    inline void Add(
		Int2Type<SADB_X_EXT_ENDPOINT_SRC>,
        const TDesC8 &aIdentity);
        
    inline void Add(
		Int2Type<SADB_X_EXT_ENDPOINT_DST>,
        const TDesC8 &aIdentity);
#endif
	};

#include "pfkey_send.inl"

#endif
