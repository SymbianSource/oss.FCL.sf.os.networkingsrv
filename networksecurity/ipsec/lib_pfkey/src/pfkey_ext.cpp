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
// This file contains the implementations for the constructors 
// for the PF_KKEY extensions.
// 
//

#include "pfkey_ext.h"

static const TInt KWordLen = 8;

EXPORT_C TPfkeyExtSA::TPfkeyExtSA(
		TUint32 aSPI,
        TUint8 anAuth,
        TUint8 anEncrypt,
        TUint8 aState,
        TUint8 aReplay,
        TUint32 aFlags) 
		{
		const TInt extLen = sizeof(struct sadb_sa) / KWordLen;

		struct sadb_sa& sa = Ext();
        sa.sadb_sa_len = extLen;
        sa.sadb_sa_exttype = SADB_EXT_SA;
        sa.sadb_sa_spi = aSPI;
        sa.sadb_sa_state = aState;
        sa.sadb_sa_auth = anAuth;
        sa.sadb_sa_encrypt = anEncrypt;
        sa.sadb_sa_replay = aReplay;
        sa.sadb_sa_flags = aFlags;
		}

EXPORT_C TPfkeyExtSpirange::TPfkeyExtSpirange(TUint32 aMin, TUint32 aMax)
		{
		const TInt extLen = sizeof(struct sadb_spirange) / KWordLen;	// 31.07 2003 JPS
		struct sadb_spirange& spirange = Ext();
		spirange.sadb_spirange_len = extLen;					// 31.07 2003 JPS
		spirange.sadb_spirange_exttype = SADB_EXT_SPIRANGE;
        spirange.sadb_spirange_min=aMin;
        spirange.sadb_spirange_max=aMax;
		}

EXPORT_C TPfkeyExtAddress::TPfkeyExtAddress(const TInetAddr& /*anAddr*/, TUint8 aProtocol, TUint8 aPrefixlen, TUint8 aType)
		{
		const TInt byte_len = sizeof(struct sadb_address) + sizeof(TInetAddr);
    
		struct sadb_address& address = Ext();
	    address.sadb_address_len = (byte_len + 7) / KWordLen;
        address.sadb_address_exttype = aType;
        address.sadb_address_proto = aProtocol;
        address.sadb_address_prefixlen = aPrefixlen;
        address.sadb_address_reserved = 0;
		}

EXPORT_C TPfkeyExtKey::TPfkeyExtKey(const TDesC8& aKey, TUint16 aBits, TUint8 aType)
        {
		const TInt extLen = (sizeof(struct sadb_key) + aKey.Length() + 7) / KWordLen;
	
		struct sadb_key& keyhdr = Ext();
        keyhdr.sadb_key_len = (TUint16)extLen;
		keyhdr.sadb_key_exttype = aType;
        keyhdr.sadb_key_bits = (TUint16)(aBits ? aBits : aKey.Length() * KWordLen);
        keyhdr.sadb_key_reserved = 0;
        }

EXPORT_C TPfkeyExtIdent::TPfkeyExtIdent(const TDesC8& aIdentity, TUint16 aIdType, TUint16 aType)
		{
		const TInt byte_len = sizeof(struct sadb_ident) + aIdentity.Length() + 1;    //Non-aligned size 
		//(+1 to add a null at the end of the string included in the padding)
        
		struct sadb_ident& ident = Ext();
		ident.sadb_ident_len = (TUint16)((byte_len + 7) / KWordLen);   //Aligned size
		ident.sadb_ident_exttype = aType;   // SA_EXT_IDENTITY_SRC, _DST
		ident.sadb_ident_type = aIdType;    // Type of the following identify information
		ident.sadb_ident_reserved = 0;      // Padding
		ident.sadb_ident_id = 0;            // Not used
		}

EXPORT_C TPfkeyExtLifetime::TPfkeyExtLifetime(
		TUint32 aAllocations,
        const TInt64& aBytes,
        const TInt64& aAddtime,
        const TInt64& aUsetime,
		const TUint16& aType)
		{
		const TInt byte_len = sizeof(struct sadb_lifetime);
		
		struct sadb_lifetime& lifetime =  Ext();
		lifetime.sadb_lifetime_len = (TUint16)((byte_len + 7) / KWordLen);
		lifetime.sadb_lifetime_exttype = aType;
		lifetime.sadb_lifetime_allocations = aAllocations;
		lifetime.sadb_lifetime_bytes = aBytes;
		lifetime.sadb_lifetime_addtime = aAddtime;
		lifetime.sadb_lifetime_usetime = aUsetime;
		}

EXPORT_C TPfkeyExtPrivate::TPfkeyExtPrivate(const TDesC8& aExtensionData)
		{
		const TInt byte_len = sizeof(struct sadb_gen_ext) + aExtensionData.Length();		
		struct sadb_gen_ext& gen_ext = Ext();
		gen_ext.sadb_len = (TUint16)((byte_len + 7) / KWordLen);
		gen_ext.sadb_ext_type = SADB_PRIV_GENERIC_EXT;
		}
#ifdef SYMBIAN_NETWORKING_IPSEC_IKE_V2

EXPORT_C TPfkeyExtTrafficSelector::TPfkeyExtTrafficSelector(TUint32 aNumsel)
		{
		const TInt byte_len = sizeof(struct sadb_x_ts) + aNumsel*sizeof(TSadbSelector);		
		struct sadb_x_ts& x_ts = Ext();
		x_ts.sadb_x_ts_len = (TUint16)((byte_len + 7) / KWordLen);
		x_ts.sadb_x_ts_exttype = SADB_X_EXT_TS;
		x_ts.sadb_x_ts_numsel = aNumsel;
		}

EXPORT_C TPfkeyExtNamedEndPoint::TPfkeyExtNamedEndPoint(const TDesC8 &aIdentity, TUint16 aType)
		{
		const TInt byte_len = sizeof(struct sadb_ident) + aIdentity.Length() + 1;    //Non-aligned size 
		//(+1 to add a null at the end of the string included in the padding)
        
		struct sadb_ident& ident = Ext();
		ident.sadb_ident_len = (TUint16)((byte_len + 7) / KWordLen);   //Aligned size
		ident.sadb_ident_exttype = aType;   // SA_X_EXT_ENDPOINT_SRC, _DST
		ident.sadb_ident_type = 0;    // Type of the following identify information
		ident.sadb_ident_reserved = 0;      // Padding
		ident.sadb_ident_id = 0;            // Not used

		}

#endif
