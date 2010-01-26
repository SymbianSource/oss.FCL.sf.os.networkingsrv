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
// pfkeymsg.cpp - IPv6/IPv4 IPSEC PFKEY message handling utilities
//

#include <networking/pfkeyv2.h>
#include "pfkeymsg.h"
#include "pfkeyext.h"  // PFKEY API General extension 
#include "ipseclog.h"
#include "sadb.h"


T_sadb_msg::T_sadb_msg(TUint8 aMsgType, TUint8 aSaType, TUint32 aSeq)
	{
	sadb_msg_version = PF_KEY_V2;
	sadb_msg_type = aMsgType;
	sadb_msg_errno = 0;
	sadb_msg_satype = aSaType;
	sadb_msg_len = ((sizeof(*this) + 7) / 8),
	sadb_msg_reserved = 0;
	sadb_msg_seq = aSeq;
	sadb_msg_pid = 0;
	}


T_sadb_sa::T_sadb_sa(TUint32 aSPI, TUint8 aWindow, TUint8 aState, TUint8 aAalg, TUint8 aEalg, TUint32 aFlags)
	{
	sadb_sa_len = (sizeof(*this) + 7) / 8;
	sadb_sa_exttype = SADB_EXT_SA;
	sadb_sa_spi = aSPI;
	sadb_sa_replay = aWindow;
	sadb_sa_state = aState;
	sadb_sa_auth = aAalg;
	sadb_sa_encrypt = aEalg;
	sadb_sa_flags = aFlags;
	}
	

// Construct sadb_lifetime for the CURRENT
T_sadb_lifetime::T_sadb_lifetime(const TLifetime &aLt)
	{
	sadb_lifetime_len = (sizeof(*this) + 7) / 8;
	sadb_lifetime_exttype = SADB_EXT_LIFETIME_CURRENT;
	sadb_lifetime_allocations = aLt.iAllocations;
	sadb_lifetime_bytes = aLt.iBytes;

	// Convert time stamps
	TTimeIntervalSeconds seconds;
	TTime time_now;
	time_now.UniversalTime();

	// Lifetime doesn't fit the 32bit int? Do I need to consider
	// probes to Alpha Centauri?
	if (time_now.SecondsFrom(aLt.iAddtime, seconds))
		sadb_lifetime_addtime = 0;
	else
		sadb_lifetime_addtime = seconds.Int();

	// If iCurrent.iUsetime still NullTime, then usetime
	// has not yet defined, return 0 (aSa.iUsed == 0)
	if (aLt.iUsetime == Time::NullTTime() ||  time_now.SecondsFrom(aLt.iUsetime, seconds))
		sadb_lifetime_usetime = 0;
	else
		sadb_lifetime_usetime = seconds.Int();
	}

// Construct sadb_lifetime for HARD and SOFT, relative to CURRENT (aRef)
T_sadb_lifetime::T_sadb_lifetime(TUint8 aType, const TLifetime &aLt, const TLifetime &aRef)
	{
	sadb_lifetime_len = (sizeof(*this) + 7) / 8;
	sadb_lifetime_exttype = aType;
	sadb_lifetime_allocations = aLt.iAllocations;
	sadb_lifetime_bytes = aLt.iBytes;

	// An implementation decision: if the expiration time
	// is so far in the future that the result in seconds
	// does not fit into 32 bit integer, then report it as
	// being infinite. (0?)
	TTimeIntervalSeconds seconds;

	if (aLt.iAddtime == Time::MaxTTime() || aLt.iAddtime.SecondsFrom(aRef.iAddtime, seconds))
		sadb_lifetime_addtime = 0;
	else
		sadb_lifetime_addtime = seconds.Int();

	// If the reference iUsetime is NULL, then usetime is not
	// yet frozen, just return the value as is.
	if (aRef.iUsetime == Time::NullTTime())
		sadb_lifetime_usetime = aLt.iUsetime.Int64();
	else if (aLt.iUsetime == Time::MaxTTime() || aLt.iUsetime.SecondsFrom(aRef.iUsetime, seconds))
		sadb_lifetime_usetime = 0;
	else
		sadb_lifetime_usetime = seconds.Int();
	}

T_sadb_address::T_sadb_address(TUint8 aType, TUint8 aProto, TUint8 aPrefix)
	{
	sadb_address_len = (sizeof(*this) + sizeof(TInetAddr) + 7) / 8;
	sadb_address_exttype = aType;
	sadb_address_proto = aProto;
	sadb_address_prefixlen = aPrefix;
	sadb_address_reserved = 0;
	}

T_sadb_key::T_sadb_key(TUint8 aType, TInt aKeyBytes, TInt aKeyBits)
	{
	sadb_key_len = (uint16_t)((sizeof(*this) + aKeyBytes + 7) / 8);
	sadb_key_exttype = aType;
	sadb_key_bits = (uint16_t)(aKeyBits < 0 ? aKeyBytes * 8 : aKeyBits);
	sadb_key_reserved = 0;
	}
	
T_sadb_supported::T_sadb_supported(TUint8 aType, TInt aNum)
	{
	sadb_supported_len = (uint16_t)((sizeof(*this) + sizeof(sadb_alg) * aNum + 7) / 8);
	sadb_supported_exttype = aType;
	sadb_supported_reserved = 0;
	}
	
T_sadb_ident::T_sadb_ident(TUint8 aType, TInt aLength)
	{
	sadb_ident_len = (uint16_t)((sizeof(*this) + aLength + 1 + 7) / 8);
	sadb_ident_exttype = aType;
	sadb_ident_reserved = 0;
	sadb_ident_type = 0;
	sadb_ident_id = 0;
	}

T_sadb_sens::T_sadb_sens()
	{
	sadb_sens_len = (uint16_t)((sizeof(*this) + 7) / 8);
	sadb_sens_exttype = SADB_EXT_SENSITIVITY;
	sadb_sens_dpd = 0;
	sadb_sens_sens_level = 0;
	sadb_sens_sens_len = 0;
	sadb_sens_integ_level = 0;
	sadb_sens_integ_len = 0;
	sadb_sens_reserved = 0;
	}

T_sadb_prop::T_sadb_prop(TUint8 aReplay, TInt aNum)
	{
	sadb_prop_len = (uint16_t)((sizeof(*this) + aNum * sizeof(struct sadb_comb) + 7) / 8);
	sadb_prop_exttype = SADB_EXT_PROPOSAL;
	sadb_prop_replay = aReplay;
	//sadb_prop_reserved[0..3] = 0;
	}


T_sadb_ts::T_sadb_ts(TInt aNum)
	{
	sadb_x_ts_len = (uint16_t)((sizeof(*this) + aNum * sizeof(T_sadb_selector) + 7) / 8);
	sadb_x_ts_exttype = SADB_X_EXT_TS;
	sadb_x_ts_numsel = aNum;
	}


TInt TPfkeyBase::ByteStream(RMBufChain &aPacket, TInt aTotal) const
	{
	if (iMsg)
		{
		struct sadb_msg base = *iMsg;
		base.sadb_msg_len = (TUint16)(aTotal / 8);
		aPacket.CopyIn(TPtrC8((TUint8 *)&base, sizeof(base)), 0);
		return Length();
		}
	else
		return 0;
	}

TInt TPfkeyAssociation::ByteStream(RMBufChain &aPacket, TInt aOffset) const
	{
	if (iExt)
		{
		aPacket.CopyIn(TPtrC8((TUint8 *)iExt, sizeof(*iExt)), aOffset);
		aOffset += Length();
		}
	return aOffset;
	}

TInt TPfkeyLifetime::ByteStream(RMBufChain &aPacket, TInt aOffset) const
	{
	if (iExt)
		{
		aPacket.CopyIn(TPtrC8((TUint8 *)iExt, sizeof(*iExt)), aOffset);
		aOffset += Length();
		}
	return aOffset;
	}

TInt TPfkeyTs::ByteStream(RMBufChain &aPacket, TInt aOffset) const
	{
	if (iExt && iTS)
		{
		aPacket.CopyIn(TPtrC8((TUint8 *)iExt, sizeof(*iExt)), aOffset);
		aOffset += sizeof(*iExt);
		for (TInt i = 0; i < iExt->sadb_x_ts_numsel; ++i)
			{
			T_sadb_selector sel;
			const RTrafficSelector &ts = (*iTS)[i];
			sel.sadb_x_selector_proto = ts.iProtocol;
			sel.iSrc.SetAddress(ts.iSrc());
			sel.iSrc.SetPort(ts.iPortSrc);
			sel.iSrc.SetScope(ts.iSrc().iScope);
			sel.iDst.SetAddress(ts.iDst());
			sel.iDst.SetPort(ts.iPortDst);
			sel.iDst.SetScope(ts.iDst().iScope);
			// Setup next sel.
			aPacket.CopyIn(TPtrC8((TUint8 *)&sel, sizeof(sel)), aOffset);
			aOffset += sizeof(sel);
			}
		}
	return aOffset;
	}

TInt TPfkeyAddress::ByteStream(RMBufChain &aPacket, TInt aOffset) const
	{
	if (iExt)
		{
		TInetAddr addr(iPort);
		addr.SetAddress(iAddr());
		addr.SetScope(iAddr().iScope);
		if (iAddr().iScope == 0 && addr.IsV4Mapped())
			addr.SetAddress(addr.Address());
		aPacket.CopyIn(TPtrC8((TUint8 *)iExt, sizeof(*iExt)), aOffset);
		aPacket.CopyIn(TPtrC8((TUint8 *)&addr, sizeof(addr)), aOffset + sizeof(*iExt));
		aOffset += Length();
		}
	return aOffset;
	}

TInt TPfkeyAddress::LoadFromStream(const TInt aLength, const TUint8 *aPtr, REndPoints &aEp)
	{
	if (iExt)
		return KErrGeneral;
	if (aLength != sizeof(struct sadb_address) + sizeof(TInetAddr))
		return KErrGeneral;
	iExt = (struct sadb_address *)aPtr;

	const TInetAddr &addr = *(TInetAddr *)(aPtr + sizeof(struct sadb_address));
	TIpAddress tmp(addr);
	const TInt err = iAddr.Open(aEp, tmp);
	iPort = (TUint16)addr.Port();
	return err;
	}


	
TInt TPfkeyIdentity::ByteStream(RMBufChain &aPacket, TInt aOffset) const
	{
	if (iExt)
		{
		aPacket.CopyIn(TPtrC8((TUint8 *)iExt, sizeof(*iExt)), aOffset);
		aPacket.CopyIn(iData, aOffset + sizeof(*iExt));
		aPacket.CopyIn(KZeroByte, aOffset + sizeof(*iExt) + iData.Length());	// Patch in the NUL terminator.
		aOffset += Length();
		}
	return aOffset;
	}

TInt TPfkeyIdentity::LoadFromStream(const TInt aLength, const TUint8 *aPtr)
	{
	if (iExt)
		return KErrGeneral;
	iExt = (struct sadb_ident *)aPtr;
	TInt len = aLength - sizeof(struct sadb_ident);
	if (len < 0)
		return KErrGeneral;
	iData.Set(aPtr + sizeof(struct sadb_ident), len);
	
	// strip off the trailing zero byte, if present.
	len = iData.Locate(0);
	if (len >= 0)
		iData.Set(iData.Ptr(), len);
	return KErrNone;
	}
	
TInt TPfkeyKey::ByteStream(RMBufChain &aPacket, TInt aOffset) const
	{
	if (iExt)
		{
		aPacket.CopyIn(TPtrC8((TUint8 *)iExt, sizeof(*iExt)), aOffset);
		aPacket.CopyIn(iData, aOffset + sizeof(*iExt));
		aOffset += Length();
		}
	return aOffset;
	}

TInt TPfkeySensitivity::ByteStream(RMBufChain &aPacket, TInt aOffset) const
	{
	if (iExt)
		{
		aPacket.CopyIn(TPtrC8((TUint8 *)iExt, sizeof(*iExt)), aOffset);
		aPacket.CopyIn(iSensBitmap, aOffset + sizeof(*iExt));
		aPacket.CopyIn(iIntegBitmap, aOffset + sizeof(*iExt) + iSensBitmap.Length());
		aOffset += Length();
		}
	return aOffset;
	}

TInt TPfkeyProposal::ByteStream(RMBufChain &aPacket, TInt aOffset) const
	{
	if (iExt)
		{
		aPacket.CopyIn(TPtrC8((TUint8 *)iExt, sizeof(*iExt)), aOffset);
		aPacket.CopyIn(TPtrC8((TUint8 *)iComb, sizeof(*iComb) * iNumComb),
			aOffset + sizeof(*iExt));
		aOffset += Length();
		}
	return aOffset;
	}

TInt TPfkeySupported::ByteStream(RMBufChain &aPacket, TInt aOffset) const
	{
	if (iExt)
		{
		aPacket.CopyIn(TPtrC8((TUint8 *)iExt, sizeof(*iExt)), aOffset);
		aPacket.CopyIn(TPtrC8((TUint8 *)iAlg, sizeof(*iAlg) * iNumAlg),
			aOffset + sizeof(*iExt));
		aOffset += Length();
		}
	return aOffset;
	}

void TPfkeySupported::Init(struct sadb_supported *aExt, TInt aNumAlg, struct sadb_alg *aAlg)
	{
	iExt = aExt;
	iAlg = aAlg;
	iNumAlg = aNumAlg;
	}

TInt TPfkeySpirange::ByteStream(RMBufChain &aPacket, TInt aOffset) const
	{
	if (iExt)
		{
		aPacket.CopyIn(TPtrC8((TUint8 *)iExt, sizeof(*iExt)), aOffset);
		aOffset += Length();
		}
	return aOffset;
	}

TInt TPfkeyAddress::BindToEndPoint(TPfkeyIdentity &aName, REndPoints &aEp)
	{
	TInt err = KErrNone;
	if (aName.iExt)
		{
		HBufC *buf = HBufC::New(aName.iData.Length());
		if (buf == NULL)
			return KErrNoMemory;
		buf->Des().Copy(aName.iData);
		// Do not override existing address, if iAddr does not specify a new address.
		TIpAddress tmp;
		// Need to save previous address, as Open below may release
		// the temporary TIpAddr referenced by iAddr().
		tmp = iAddr();
		err = iAddr.Open(aEp, *buf, tmp, tmp.IsNone());
		delete buf;
		}
	return err;
	}

//	TPfkeyMessage
//
//	Construct TPfkeyMesage from a PF_KEY v2 byte stream (aMsg)
TPfkeyMessage::TPfkeyMessage(const TDesC8& aMsg, REndPoints &aEp)
	{
	const TUint8 *p = aMsg.Ptr();
	TInt length = aMsg.Length();

	iError = KErrArgument;
	if (length < (TInt)sizeof(sadb_msg))
		return;		// EMSGSIZE (impossible message size)

	// Base Message Header
	iBase.iMsg = (struct sadb_msg *)p;
	if (iBase.iMsg->sadb_msg_version != PF_KEY_V2)
		return;		// EINVAL
	// SADB_ACQUIRE response can have sadb_msg_errno set to non-zero value  
	if (iBase.iMsg->sadb_msg_errno && (iBase.iMsg->sadb_msg_type != SADB_ACQUIRE))
		return;		// EINVAL (should be set zero by sender)
	if (iBase.iMsg->sadb_msg_len * 8 != length)
		return;		// EMSGSIZE (incorrect message length)
	// SADB_ACQUIRE response can have sadb_msg_reserved set to non-zero value            
	if (iBase.iMsg->sadb_msg_reserved && (iBase.iMsg->sadb_msg_type != SADB_ACQUIRE))
		return;		// EINVAL (unused parts must be zeroed)
	p += sizeof(struct sadb_msg);
	length -= sizeof(struct sadb_msg);

	// Extension headers
	// Some general rules:
	// - only one instance of an extension type is valid
	while (length > 0)
		{
		struct sadb_ext *ext = (struct sadb_ext *)p;
		int ext_len = ext->sadb_ext_len;
		int data_len, data_len2;

		if (ext_len < 1)
			return;		// EINVAL (bad message format)
		ext_len *= 8;
		if (ext_len > length)
			return;		// EINVAL
		switch (ext->sadb_ext_type)
			{
			case SADB_EXT_RESERVED:
				return;		// EINVAL (bad mesage format)

			case SADB_EXT_SA:
				if (iSa.iExt)
						return;	// EINVAL
				iSa.iExt = (struct sadb_sa *)p;
				break;

			case SADB_EXT_LIFETIME_CURRENT:
				if (iCurrent.iExt)
					return;	// EINVAL;
				iCurrent.iExt = (struct sadb_lifetime *)p;
				break;

			case SADB_EXT_LIFETIME_HARD:
				if (iHard.iExt)
					return;
				iHard.iExt = (struct sadb_lifetime *)p;
				break;

			case SADB_EXT_LIFETIME_SOFT:
				if (iSoft.iExt)
					return;
				iSoft.iExt = (struct sadb_lifetime *)p;
				break;

			case SADB_EXT_ADDRESS_SRC:
				if (iSrcAddr.LoadFromStream(ext_len, p, aEp) != KErrNone)
					return;
				break;  

			case SADB_EXT_ADDRESS_DST:
				if (iDstAddr.LoadFromStream(ext_len, p, aEp) != KErrNone)
					return;
				break;

			case SADB_EXT_ADDRESS_PROXY:
				if (iProxyAddr.LoadFromStream(ext_len, p, aEp) != KErrNone)
					return;
				break;

			case SADB_EXT_KEY_AUTH:
				if (iAuthKey.iExt)
					return;
				iAuthKey.iExt = (struct sadb_key *)p;
				data_len = (iAuthKey.iExt->sadb_key_bits + 7) / 8;
				if (data_len == 0 || data_len + (int)sizeof(struct sadb_key) > ext_len)
					return;
				iAuthKey.iData.Set(p + sizeof(struct sadb_key), data_len);
					break;

			case SADB_EXT_KEY_ENCRYPT:
				if (iEncryptKey.iExt)
					return;
				iEncryptKey.iExt = (struct sadb_key *)p;
				data_len = (iEncryptKey.iExt->sadb_key_bits + 7) / 8;
				if (data_len == 0 || data_len + (int)sizeof(struct sadb_key) > ext_len)
					return;
				iEncryptKey.iData.Set(p + sizeof(struct sadb_key), data_len);
				break;

			case SADB_EXT_IDENTITY_SRC:
				if (iSrcIdent.LoadFromStream(ext_len, p) != KErrNone)
					return;
				break;

			case SADB_EXT_IDENTITY_DST:
				if (iDstIdent.LoadFromStream(ext_len, p) != KErrNone)
					return;
				break;

			case SADB_EXT_SENSITIVITY:
				if (iSensitivity.iExt)
					return;
				iSensitivity.iExt = (struct sadb_sens *)p;
				data_len = iSensitivity.iExt->sadb_sens_sens_len * 8;
				iSensitivity.iSensBitmap.Set(p + sizeof(struct sadb_sens), data_len);
				data_len2 = iSensitivity.iExt->sadb_sens_integ_len * 8;
				iSensitivity.iSensBitmap.Set(p + (sizeof(struct sadb_sens) + data_len),
						 data_len2);
				if (data_len + data_len2 + (int)sizeof(struct sadb_sens) > ext_len)
					return;
				break;

			case SADB_EXT_PROPOSAL:
				if (iProposal.iExt)
					return;
				iProposal.iExt = (struct sadb_prop *)p;
				iProposal.iNumComb = (ext_len - sizeof(struct sadb_prop)) / sizeof(struct sadb_comb);
				iProposal.iComb = (struct sadb_comb *)(p + sizeof(struct sadb_prop));
				break;

			case SADB_EXT_SUPPORTED_AUTH:
				if (iAuthAlgs.iExt)
					return;
				iAuthAlgs.iExt = (struct sadb_supported *)p;
				iAuthAlgs.iNumAlg = (ext_len - sizeof(struct sadb_supported)) / sizeof(struct sadb_alg);
				iAuthAlgs.iAlg = (struct sadb_alg *)(p + sizeof(struct sadb_supported));
				break;

			case SADB_EXT_SUPPORTED_ENCRYPT:
				if (iEncryptAlgs.iExt)
					return;
				iEncryptAlgs.iExt = (struct sadb_supported *)p;
				iEncryptAlgs.iNumAlg = (ext_len - sizeof(struct sadb_supported)) / sizeof(struct sadb_alg);
				iEncryptAlgs.iAlg = (struct sadb_alg *)(p + sizeof(struct sadb_supported));
				break;

			case SADB_EXT_SPIRANGE:
				if (iSpirange.iExt)
					return;
				iSpirange.iExt = (struct sadb_spirange *)p;
				break;

			/**---------------------------------------------------------------
			 *
			 *  PFKEY API general private extension.
			 *
			 *----------------------------------------------------------------*/                
			case SADB_PRIV_GENERIC_EXT:
				if (iPrivateExtension.iExt)
					return;
				iPrivateExtension.iExt = (struct sadb_gen_ext *)p;
				data_len = (ext_len - sizeof(struct sadb_gen_ext));
				if (data_len > ext_len)
					return;
				iPrivateExtension.iData.Set(p + sizeof(struct sadb_gen_ext), data_len);
				break;

			// End Point Extensions
			case SADB_X_EXT_ENDPOINT_SRC:
				if (iSrcEndpoint.LoadFromStream(ext_len, p) != KErrNone)
					return;
				break;
			case SADB_X_EXT_ENDPOINT_DST:
				if (iDstEndpoint.LoadFromStream(ext_len, p) != KErrNone)
					return;
				break;

			case SADB_X_EXT_TS:
				if (iTs.iExt)
					return;
				iTs.iExt = (struct sadb_x_ts *)p;
				break;

			default:
				// Unknown extensions must be ignored, not an error!
				break;
			}
			p += ext_len;
			length -= ext_len;
		}
	if (length != 0)
		return;

	// Do the "End Point Hack"
	if ((iError = iSrcAddr.BindToEndPoint(iSrcEndpoint, aEp)) == KErrNone)
		iError = iDstAddr.BindToEndPoint(iDstEndpoint, aEp);
	}

TUint16 TPfkeyMessage::Length64() const
	{
	// Always make sure that all possible fields in PFKEY are included!
	return (TUint16)(
		(iBase.Length() +
		iSa.Length() +
		iCurrent.Length() +
		iHard.Length() +
		iSoft.Length() +
		iSrcAddr.Length() +
		iDstAddr.Length() +
		iProxyAddr.Length() +
		iAuthKey.Length() +
		iEncryptKey.Length() +
		iSrcIdent.Length() +
		iDstIdent.Length() +
		iSrcEndpoint.Length() +
		iDstEndpoint.Length() +
		iSensitivity.Length() +
		iProposal.Length() +
		iAuthAlgs.Length() +
		iEncryptAlgs.Length() +
		iTs.Length() +
		iSpirange.Length()) / 8);
	}

void TPfkeyMessage::ByteStreamL(RMBufChain &aPacket) const
	{
	TInt totlen = Length64() * 8;

	// This code basicly assumes the RMBuf is empty before!!
	// (Any possible previous content is lost)
	//
//	aPacket.TrimEnd(0);	// does not like empty buffer!!
	aPacket.AppendL(totlen);


	TInt offset = iBase.ByteStream(aPacket, totlen);
	
	// Always make sure that all possible extension fields in PFKEY are included!

	offset = iSa.ByteStream(aPacket, offset);
	offset = iCurrent.ByteStream(aPacket, offset);
	offset = iHard.ByteStream(aPacket, offset);
	offset = iSoft.ByteStream(aPacket, offset);
	offset = iSrcAddr.ByteStream(aPacket, offset);
	offset = iDstAddr.ByteStream(aPacket, offset);
	offset = iProxyAddr.ByteStream(aPacket, offset);
	offset = iAuthKey.ByteStream(aPacket, offset);
	offset = iEncryptKey.ByteStream(aPacket, offset);
	offset = iSrcIdent.ByteStream(aPacket, offset);
	offset = iDstIdent.ByteStream(aPacket, offset);
	offset = iSrcEndpoint.ByteStream(aPacket, offset);
	offset = iDstEndpoint.ByteStream(aPacket, offset);
	offset = iSensitivity.ByteStream(aPacket, offset);
	offset = iProposal.ByteStream(aPacket, offset);
	offset = iAuthAlgs.ByteStream(aPacket, offset);
	offset = iEncryptAlgs.ByteStream(aPacket, offset);
	offset = iSpirange.ByteStream(aPacket, offset);
	offset = iTs.ByteStream(aPacket, offset);
	}

#ifdef _LOG

// Convert the message type into string literal
static const TDesC &LogMessageType(TInt aType)
	{
	_LIT(Kgetspi,	"getspi");
	_LIT(Kupdate,	"update");
	_LIT(Kadd,		"add");
	_LIT(Kdelete,	"delete");
	_LIT(Kget,		"get");
	_LIT(Kacquire,	"acquire");
	_LIT(Kregister,	"register");
	_LIT(Kexpire,	"expire");
	_LIT(Kflush,	"flush");
	_LIT(Kdump,		"dump");
	_LIT(Kunknown,	"unknown");
	switch (aType)
		{
		case SADB_GETSPI:	return Kgetspi;
		case SADB_UPDATE:	return Kupdate;
		case SADB_ADD:		return Kadd;
		case SADB_DELETE:	return Kdelete;
		case SADB_GET:		return Kget;
		case SADB_ACQUIRE:	return Kacquire;
		case SADB_REGISTER:	return Kregister;
		case SADB_EXPIRE:	return Kexpire;
		case SADB_FLUSH:	return Kflush;
		case SADB_DUMP:		return Kdump;
		default:			break;
		}
	Log::Printf(_L("\t\t*invalid PFKEY message type %d*"), aType);
	return Kunknown;
	}

// Convert association (SADB_SATYPE_*) into string literal
static const TDesC &LogAssociationType(TInt aType)
	{
	_LIT(Kah,		" ah");
	_LIT(Kesp,		" esp");
	_LIT(Kinvalid,	" invalid");

	switch (aType)
		{
		case SADB_SATYPE_UNSPEC:	return KNullDesC;
		case SADB_SATYPE_AH:		return Kah;
		case SADB_SATYPE_ESP:		return Kesp;
		default:					break;
		}
	Log::Printf(_L("\t\t*invalid PFKEY SA type %d*"), aType);
	return Kinvalid;
	}

// Convert Association state (SADB_SASTATE_*) into string literal
static const TDesC &LogAssociationState(TInt aState)
	{
	_LIT(Klarval,	" larval");
	_LIT(Kdying,	" dying");
	_LIT(Kdead,		" dead");
	_LIT(Kinvalid,	" invalid");

	switch (aState)
		{
		case SADB_SASTATE_LARVAL:	return Klarval;
		case SADB_SASTATE_MATURE:	return KNullDesC;	// The default state
		case SADB_SASTATE_DYING:	return Kdying;
		case SADB_SASTATE_DEAD:		return Kdead;
		default:					break;
		}
	Log::Printf(_L("\t\t*invalid PFKEY state %d*"), aState);
	return Kinvalid;	
	}

// Convert authentication algorithm (SADB_AALG_*) into string literal
static const TDesC &LogAuthAlg(TInt aAlg,  const CAlgorithmList *aAlgorithms)
	{
	_LIT(Kunknown,	"auth_alg unknown");
	_LIT(Knone,		"auth none");

	if (!aAlg)
		return Knone;
	
	if (aAlgorithms)
		{
		const TAlgorithmMap *map = aAlgorithms->Lookup(EAlgorithmClass_Digest, aAlg);
		if (map)
			return map->iAlgorithm;
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
		map = aAlgorithms->Lookup(EAlgorithmClass_Mac, aAlg);
			return map->iAlgorithm;
#endif
		}
	Log::Printf(_L("\t\t*unknown PFKEY auth_alg %d*"), aAlg);
	return Kunknown;
	}

// Convert encryption algorithm (SADB_EALG_*) into string literal
static const TDesC &LogEncrAlg(TInt aAlg,  const CAlgorithmList *aAlgorithms)
	{
	_LIT(Kunknown,	"encr_alg unknown");
	_LIT(Knull,		"null");
	_LIT(Knone,		"encr none");

	if (!aAlg)
		return Knone;
	
	if (aAlgorithms)
		{
		const TAlgorithmMap *map = aAlgorithms->Lookup(EAlgorithmClass_Cipher, aAlg);
		if (map)
			{
			if (map->iAlgorithm.Length() == 0)
				return Knull();
			return map->iAlgorithm;
			}
		}
	Log::Printf(_L("\t\t*unknown PFKEY encr_alg %d*"), aAlg);
	return Kunknown;	
	}

// Conditional return either "" or aStr
static const TDesC &LogTest(TInt aCondition, const TDesC &aStr)
	{
	return aCondition ? aStr : KNullDesC;
	}
	
static void LogTestAppend(TDes &aBuf, const TDesC &aLabel, TInt aValue)
	{
	if (aValue)
		{
		aBuf.AppendFormat(_L("%S %d"), &aLabel, aValue);
		}
	}

	
// Convert identity type (SADB_IDENTTYP_*) into string literal
static const TDesC8 &LogIdentityType(TInt aType)
	{
	_LIT8(Kprefix,		"prefix");
	_LIT8(Kfqdn,		"fqdn");
	_LIT8(Kuserfqdn,	"user_fqdn");
	_LIT8(Kinvalid,		"invalid");

	switch (aType)
		{
		case SADB_IDENTTYPE_RESERVED:	return KNullDesC8;	// Used by EndPoint
		case SADB_IDENTTYPE_PREFIX:		return Kprefix;
		case SADB_IDENTTYPE_FQDN:		return Kfqdn;
		case SADB_IDENTTYPE_USERFQDN:	return Kuserfqdn;
		default:						break;
		}
	Log::Printf(_L("\t\t*invalid PFKEY identity type %d*"), aType);
	return Kinvalid;
	}

void TPfkeyMessage::LogPrint(const TDesC &aLabel, const CAlgorithmList *aAlgorithms) const
	{
	iBase.LogPrint(aLabel);
	iSa.LogPrint(_L("\t"), aAlgorithms);
	iCurrent.LogPrint(_L("\tcurrent"));
	iHard.LogPrint(_L("\thard"));
	iSoft.LogPrint(_L("\tsoft"));
	iSrcAddr.LogPrint(_L("\tsrc"));
	iDstAddr.LogPrint(_L("\tdst"));
	iProxyAddr.LogPrint(_L("\tproxy"));
	iAuthKey.LogPrint(_L("\tauthkey"));
	iEncryptKey.LogPrint(_L("\tencrkey"));
	iSrcIdent.LogPrint(_L8("\tsrcid"));
	iDstIdent.LogPrint(_L8("\tdstid"));
	iSrcEndpoint.LogPrint(_L8("\tsrcep"));
	iDstEndpoint.LogPrint(_L8("\tdstep"));
	iSensitivity.LogPrint(_L("\tsensitivity"));
	iTs.LogPrint(_L("\tts"));
	iProposal.LogPrint(_L("\tproposal"), aAlgorithms);
	iAuthAlgs.LogPrint(_L("\t"), aAlgorithms);
	iEncryptAlgs.LogPrint(_L("\t"), aAlgorithms);
	iSpirange.LogPrint(_L("\tspirange"));
	Log::Printf(_L("\tmsg length=%d"), (TInt)8*Length64());
	}

void TPfkeyBase::LogPrint(const TDesC &aLabel) const
	{
	if (iMsg)
		{
		TBuf<20> errbuf;
		// High order bits of the error code are in sadb_msg_reserved!
		const TInt error = (iMsg->sadb_msg_reserved << 8) | iMsg->sadb_msg_errno;
		if (error)
			{
			errbuf.Format(_L(" error %d"), -error);
			}
		Log::Printf(_L("%S\t%S%S seq %d pid %d%S"),
			&aLabel,
			&LogMessageType(iMsg->sadb_msg_type),
			&LogAssociationType(iMsg->sadb_msg_satype),
			(TInt)iMsg->sadb_msg_seq,
			(TInt)iMsg->sadb_msg_pid,
			&errbuf);
		}
	}

void TPfkeyAssociation::LogPrint(const TDesC &aLabel,  const CAlgorithmList *aAlgorithms) const
	{
	_LIT(Kpfs,		" pfs");
	_LIT(Ktunnel,	" tunnel");
	_LIT(Knatt,		" nat-t");
	_LIT(Kaddr,		" int-addr");
	_LIT(Kesn,		" esn");

	if (iExt)
		{
		const TInt flags = iExt->sadb_sa_flags;
		TBuf<20> replay;
		if (iExt->sadb_sa_replay)
			{
			replay.Format(_L(" replay %d"), (TInt)iExt->sadb_sa_replay);
			}
		Log::Printf(_L("%Sspi %d%S%S %S(%d) %S(%d)%S%S%S%S%S"),
			&aLabel,
			(TInt)ByteOrder::Swap32(iExt->sadb_sa_spi),	// kinky definition in PFKEYv2 (network order used)
			&replay,
			&LogAssociationState(iExt->sadb_sa_state),
			&LogAuthAlg(iExt->sadb_sa_auth, aAlgorithms), iExt->sadb_sa_auth,
			&LogEncrAlg(iExt->sadb_sa_encrypt, aAlgorithms), iExt->sadb_sa_encrypt,
			&LogTest(flags & SADB_SAFLAGS_PFS, Kpfs),
			&LogTest(flags & SADB_SAFLAGS_TUNNEL, Ktunnel),
			&LogTest(flags & SADB_SAFLAGS_NAT_T, Knatt),
			&LogTest(flags & SADB_SAFLAGS_INT_ADDR, Kaddr),
			&LogTest(flags & SABD_SAFLAGS_ESN, Kesn));
		}
	}
	
void TPfkeyLifetime::LogPrint(const TDesC &aLabel) const
	{
	if (iExt)
		{
		TBuf<200> buf;
		if (iExt->sadb_lifetime_allocations)
			buf.AppendFormat(_L("%S_allocations %d"), &aLabel, (TInt)iExt->sadb_lifetime_allocations);
		if (iExt->sadb_lifetime_bytes)
			buf.AppendFormat(_L("%S_bytes %d"), &aLabel, (TInt)iExt->sadb_lifetime_bytes);
		if (iExt->sadb_lifetime_addtime)
			buf.AppendFormat(_L("%S_addtime %d"), &aLabel, (TInt)iExt->sadb_lifetime_addtime);
		if (iExt->sadb_lifetime_usetime)
			buf.AppendFormat(_L("%S_usetime %d"), &aLabel, (TInt)iExt->sadb_lifetime_usetime);
		if (buf.Length())
			Log::Printf(_L("%S"), &buf);
		}
	}
	
void TPfkeyKey::LogPrint(const TDesC &aLabel) const
	{
	if (iExt)
		{
		Log::Printf(_L("%S bitlength %d"),
		&aLabel,
		(TInt)iExt->sadb_key_bits);
		}
	}

void TPfkeyAddress::LogPrint(const TDesC &aLabel) const
	{
	if (iExt)
		{
		TBuf<70> tmp;
		TBuf<100> name;
		name.Copy(iAddr.Name());
		TInetAddr addr(iAddr(), iPort);
		addr.SetScope(iAddr().iScope);
		addr.OutputWithScope(tmp);
		Log::Printf(_L("%S %S#%d %S"),
			&aLabel,
			&tmp,
			(TInt)iPort,
			&name
			);
		}
	}

void TPfkeyIdentity::LogPrint(const TDesC8 &aLabel) const
	{
	if (iExt)
		{
		Log::Printf(_L8("%S %S %S"),
			&aLabel,
			&LogIdentityType(iExt->sadb_ident_type),
			&iData);
		}
	}
	
void TPfkeySensitivity::LogPrint(const TDesC &aLabel) const
	{
	if (iExt)
		{
		Log::Printf(_L("%S"),
		&aLabel);
		}
	}

void TPfkeyProposal::LogPrint(const TDesC &aLabel,  const CAlgorithmList *aAlgorithms) const
	{
	if (iComb)
		{
		for (TInt i = 0; i < iNumComb; ++i)
			{
			TBuf<200> buf;
			if (iComb[i].sadb_comb_auth)
				{
				buf.AppendFormat(_L(" auth min %d max %d"),
					(TInt)iComb[i].sadb_comb_auth_minbits,
					(TInt)iComb[i].sadb_comb_auth_maxbits);
				}
			if (iComb[i].sadb_comb_encrypt)
				{
				buf.AppendFormat(_L(" encr min %d max %d"),
					(TInt)iComb[i].sadb_comb_encrypt_minbits,
					(TInt)iComb[i].sadb_comb_encrypt_maxbits);
				}
			LogTestAppend(buf, _L(" soft_allocations"), iComb[i].sadb_comb_soft_allocations);
			LogTestAppend(buf, _L(" hard_allocations"), iComb[i].sadb_comb_hard_allocations);
			LogTestAppend(buf, _L(" soft_bytes"), iComb[i].sadb_comb_soft_bytes);
			LogTestAppend(buf, _L(" hard_bytes"), iComb[i].sadb_comb_hard_bytes);
			LogTestAppend(buf, _L(" soft_addtime"), iComb[i].sadb_comb_soft_addtime);
			LogTestAppend(buf, _L(" hard_addtime"), iComb[i].sadb_comb_hard_addtime);
			LogTestAppend(buf, _L(" soft_usetime"), iComb[i].sadb_comb_soft_usetime);
			LogTestAppend(buf, _L(" hard_usetime"), iComb[i].sadb_comb_hard_usetime);
			
			Log::Printf(_L("%S[%d] %S(%d) %S(%d)%S"),
				&aLabel, i,
				&LogAuthAlg(iComb[i].sadb_comb_auth, aAlgorithms), iComb[i].sadb_comb_auth,
				&LogEncrAlg(iComb[i].sadb_comb_encrypt, aAlgorithms), iComb[i].sadb_comb_encrypt,
				&buf
				);
			}
		}
	}
	
void TPfkeySupported::LogPrint(const TDesC &aLabel, const CAlgorithmList *aAlgorithms) const
	{
	if (iAlg)
		{
		const TBool auth = iExt->sadb_supported_exttype == SADB_EXT_SUPPORTED_AUTH;
		for (TInt i = 0; i < iNumAlg; ++i)
			{
			Log::Printf(_L("%S %S(%d) ivlen %d minbits %d maxbits %d"),
				&aLabel,
				auth ? &LogAuthAlg(iAlg[i].sadb_alg_id, aAlgorithms) : &LogEncrAlg(iAlg[i].sadb_alg_id, aAlgorithms),
				iAlg[i].sadb_alg_id,
				iAlg[i].sadb_alg_ivlen,
				iAlg[i].sadb_alg_minbits,
				iAlg[i].sadb_alg_maxbits);
			}
		}
	}

void TPfkeySpirange::LogPrint(const TDesC &aLabel) const
	{
	if (iExt)
		{
		Log::Printf(_L("%S min %d max %d"),
		&aLabel,
		(TInt)iExt->sadb_spirange_min,
		(TInt)iExt->sadb_spirange_max);
		}
	}
	
void TPfkeyTs::LogPrint(const TDesC &aLabel) const
	{
	if (iExt && iTS)
		{
		for (TInt i = 0; i < iTS->Count(); ++i)
			{
			TInetAddr addr;
			TBuf<50> src;
			TBuf<50> dst;
			const RTrafficSelector &ts = (*iTS)[i];
			addr.SetAddress(ts.iSrc());
			addr.SetScope(ts.iSrc().iScope);
			addr.OutputWithScope(src);
			addr.SetAddress(ts.iDst());
			addr.SetScope(ts.iDst().iScope);
			addr.OutputWithScope(dst);
			Log::Printf(_L("%S[%d] proto=%d src=%S#%d dst=%S#%d"),
				&aLabel, i,
				(TInt)ts.iProtocol,
				&src, (TInt)ts.iPortSrc,
				&dst, (TInt)ts.iPortDst);
			}
		}
	}
#endif
