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
 @file pfkey_ext.h
 @publishedPartner
 @released
*/

#ifndef __PFKEY_EXT_H__
#define __PFKEY_EXT_H__

#include <e32std.h>

#include <lib_pfkey.h>
#include <networking/pfkeyext.h>

/**
 *	The Pfkey extensions and some convenient functions for them
 */
class TPfkeyExtSA : public TPfkeyExt<struct sadb_sa>
	{
public:
	IMPORT_C TPfkeyExtSA(TUint32 aSPI,
        TUint8 anAuth = 0,
        TUint8 anEncrypt = 0,
        TUint8 aState = SADB_SASTATE_MATURE,
        TUint8 aReplay = 0,
        TUint32 aFlags = 0);
	};

class TPfkeyExtSpirange : public TPfkeyExt<struct sadb_spirange>
	{
public:
	IMPORT_C TPfkeyExtSpirange(TUint32 aMin, TUint32 aMax);
	};

class TPfkeyExtAddress : public TPfkeyExt<struct sadb_address>
	{
public:
	IMPORT_C TPfkeyExtAddress(const TInetAddr &anAddr, TUint8 aProtocol, TUint8 aPrefixlen, TUint8 aType);
	};

class TPfkeyExtKey : public TPfkeyExt<struct sadb_key>
	{
public:
	IMPORT_C TPfkeyExtKey(const TDesC8 &aKey, TUint16 aBits, TUint8 aType);
	};

class TPfkeyExtIdent : public TPfkeyExt<struct sadb_ident>
	{
public:
	IMPORT_C TPfkeyExtIdent(const TDesC8 &aIdentity, TUint16 aIdType, TUint16 aType);
	};

class TPfkeyExtLifetime : public TPfkeyExt<struct sadb_lifetime>
	{
public:
	IMPORT_C TPfkeyExtLifetime(
		TUint32 aAllocations,
        const TInt64 &aBytes,
        const TInt64 &aAddtime,
        const TInt64 &aUsetime,
		const TUint16 &aType);
	};

class TPfkeyExtPrivate : public TPfkeyExt<struct sadb_gen_ext>
	{
public:
	IMPORT_C TPfkeyExtPrivate(const TDesC8 &aExtensionData);
	};

#ifdef SYMBIAN_NETWORKING_IPSEC_IKE_V2
class TSadbSelector;
class TPfkeyExtTrafficSelector : public TPfkeyExt<struct sadb_x_ts>
	{
public:
	IMPORT_C TPfkeyExtTrafficSelector(TUint32 aNumsel);
	};
	
class TPfkeyExtNamedEndPoint : public TPfkeyExt<struct sadb_ident>
	{
public:
	IMPORT_C TPfkeyExtNamedEndPoint(const TDesC8 &aIdentity, TUint16 aType);
	};


class TSadbSelector:public sadb_x_selector
	{
public:
	TInetAddr iSrc;
	TInetAddr iDst;		
	};	
#endif //SYMBIAN_NETWORKING_IPSEC_IKE_V2

#endif
