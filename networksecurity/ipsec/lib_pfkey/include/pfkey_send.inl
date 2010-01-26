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
 @file pfkey_send.inl
 @publishedPartner
 @released
*/
//	class TPfkeySendMsg
inline void TPfkeySendMsg::Add(
		Int2Type<SADB_EXT_SA>,
		TUint32 aSPI,
        TUint8 anAuth,
        TUint8 anEncrypt,
        TUint8 aState,
        TUint8 aReplay,
        TUint32 aFlags)
		{
		TPfkeyExtSA sa(aSPI, anAuth, anEncrypt, aState, aReplay, aFlags);
		AddExt(sa);
		}
	
inline void TPfkeySendMsg::Add(
		Int2Type<SADB_EXT_SPIRANGE>,
		TUint32 aMin, 
		TUint32 aMax)
		{
		TPfkeyExtSpirange spirange(aMin, aMax);
		AddExt(spirange);
		}

inline void TPfkeySendMsg::Add(
		Int2Type<SADB_EXT_ADDRESS_SRC>,
		const TInetAddr& anAddr,
        TUint8 aProtocol,
        TUint8 aPrefixlen)
		{
		TPfkeyExtAddress address_src(anAddr, aProtocol, aPrefixlen, SADB_EXT_ADDRESS_SRC);
		// Packaging should not be required by ipsec6.prt
		TPckgC<TInetAddr> addr(anAddr);
		AddExt(address_src, addr);
		}
	
inline void TPfkeySendMsg::Add(
		Int2Type<SADB_EXT_ADDRESS_DST>,
		const TInetAddr& anAddr,
        TUint8 aProtocol,
        TUint8 aPrefixlen)
		{	
		TPfkeyExtAddress address_src(anAddr, aProtocol, aPrefixlen, SADB_EXT_ADDRESS_DST);
		// Packaging should not be required by ipsec6.prt
		TPckgC<TInetAddr> addr(anAddr);
		AddExt(address_src, addr);
		}
	
inline void TPfkeySendMsg::Add(
		Int2Type<SADB_EXT_ADDRESS_PROXY>,
		const TInetAddr& anAddr,
        TUint8 aProtocol,
        TUint8 aPrefixlen)
		{	
		TPfkeyExtAddress address_proxy(anAddr, aProtocol, aPrefixlen, SADB_EXT_ADDRESS_PROXY);
		TPckgC<TInetAddr> addr(anAddr);
		AddExt(address_proxy, addr);
		}

inline void TPfkeySendMsg::Add(
		Int2Type<SADB_EXT_KEY_ENCRYPT>,
		const TDesC8& aKey,
        TUint16 aBits)
        {
		TPfkeyExtKey key_encrypt(aKey, aBits, SADB_EXT_KEY_ENCRYPT);
		AddExt(key_encrypt, aKey);
		}
	
inline void TPfkeySendMsg::Add(
		Int2Type<SADB_EXT_KEY_AUTH>,
		const TDesC8& aKey,
        TUint16 aBits)
        {
		TPfkeyExtKey key_encrypt(aKey, aBits, SADB_EXT_KEY_AUTH);
		AddExt(key_encrypt, aKey);
		}

inline void TPfkeySendMsg::Add(
		Int2Type<SADB_EXT_IDENTITY_SRC>,
		const TDesC8& aIdentity, 
        TUint16 aIdType)
        {
		TPfkeyExtIdent identity_src(aIdentity, aIdType, SADB_EXT_IDENTITY_SRC);
		AddExt(identity_src, aIdentity);
		}

inline void TPfkeySendMsg::Add(
		Int2Type<SADB_EXT_IDENTITY_DST>,
		const TDesC8& aIdentity, 
        TUint16 aIdType)
        {
		TPfkeyExtIdent identity_dst(aIdentity, aIdType, SADB_EXT_IDENTITY_DST);
		AddExt(identity_dst, aIdentity);
		}

inline void TPfkeySendMsg::Add(
		Int2Type<SADB_EXT_LIFETIME_HARD>,
        TUint32 aAllocations,
        const TInt64& aBytes,
        const TInt64& aAddtime,
        const TInt64& aUsetime)
        {
        TPfkeyExtLifetime lifetime(aAllocations, aBytes, aAddtime, aUsetime, SADB_EXT_LIFETIME_HARD);
		AddExt(lifetime);
		}
	
inline void TPfkeySendMsg::Add(
		Int2Type<SADB_EXT_LIFETIME_SOFT>,
        TUint32 aAllocations,
        const TInt64& aBytes,
        const TInt64& aAddtime,
        const TInt64& aUsetime)
        {
        TPfkeyExtLifetime lifetime(aAllocations, aBytes, aAddtime, aUsetime, SADB_EXT_LIFETIME_SOFT);
		AddExt(lifetime);
		}

inline void TPfkeySendMsg::Add(
		Int2Type<SADB_PRIV_GENERIC_EXT>,   // 31.07 2003 JPS
		const TDesC8& aExtensionData)
        {
		TPfkeyExtPrivate pvt(aExtensionData);
		AddExt(pvt, aExtensionData);
		}

inline TPfkeySendMsg::TPfkeySendMsg(TUint8 aType, TUint8 aSaType, TInt aRequestCount, TInt aPid) 
		: TPfkeySendMsgBase(aType, aSaType, aRequestCount, aPid)
		{
		}
	
inline TPfkeySendMsg::TPfkeySendMsg()
		: TPfkeySendMsgBase()
		{
		}

#ifdef SYMBIAN_NETWORKING_IPSEC_IKE_V2

inline void TPfkeySendMsg::Add(
		Int2Type<SADB_X_EXT_TS>,   
		RArray<TSadbSelector> aSelectors)
        {
		TPfkeyExtTrafficSelector ts(aSelectors.Count());
		TBuf8<KPfkeyMsgMaxLen> buf;
		for(int i = 0; i<aSelectors.Count(); i++)
			{
			buf.Append(TPtrC8(reinterpret_cast<TUint8 *>(&(aSelectors[i])), sizeof(TSadbSelector)));
			}
			
		AddExt(ts, buf);
		}

inline void TPfkeySendMsg::Add(
		Int2Type<SADB_X_EXT_ENDPOINT_SRC>,
		const TDesC8& aIdentity)
        {
		TPfkeyExtNamedEndPoint namedEndPoint_src(aIdentity,SADB_X_EXT_ENDPOINT_SRC);
		AddExt(namedEndPoint_src, aIdentity);
		}

inline void TPfkeySendMsg::Add(
		Int2Type<SADB_X_EXT_ENDPOINT_DST>,
		const TDesC8& aIdentity)
        {
		TPfkeyExtNamedEndPoint namedEndPoint_dst(aIdentity,SADB_X_EXT_ENDPOINT_DST);
		AddExt(namedEndPoint_dst, aIdentity);
		}

#endif
